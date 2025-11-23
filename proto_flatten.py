#!/usr/bin/env python3
"""
proto_flatten.py — flatten/concatenate multiple .proto files into a single file.

Key features
-----------
- Recursively scans one or more input directories (and/or files).
- Strips all `import` lines (including `import public`), so the result is self-contained.
- Collects and deduplicates top-level definitions: `message`, `enum`, `service`, `extend`.
- Preserves leading comments that belong to a definition.
- Merges file-level `option ...;` statements (deduplicated by full text).
- Ensures exactly one `syntax = "proto3";` or "proto2" at the top (must agree across files).
- Ensures at most one `package ...;` (overridable via `--package`).
- Writes a clean, deterministic output (optionally sorted by kind/name).

Limitations
-----------
- This is a lightweight parser based on regex + brace counting for top-level items.
  It handles common .proto layouts well, but it is not a full protobuf parser.
- If the same named type (e.g., message Foo) is declared differently in two files, it will error
  unless `--prefer-first` is used (then the first definition wins, others are skipped with a warning).
- Nested types are kept inside their parent as-is (we don't lift nested declarations).
- If different files declare different `syntax` versions, we error unless `--force-syntax` is provided.

Usage
-----
  python proto_flatten.py \
    --in enums --in structures --in messages \
    --out all_in_one.proto \
    --package my.company.api.v1 \
    --sort

  You can also pass files directly:
  python proto_flatten.py --in ./foo.proto --in ./bar/baz.proto --out merged.proto
"""
from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple

SYNTAX_RE = re.compile(r'^\s*syntax\s*=\s*"(proto2|proto3)";\s*$')
PACKAGE_RE = re.compile(r'^\s*package\s+([a-zA-Z_][\w\.]*)\s*;\s*$')
IMPORT_RE = re.compile(r'^\s*import\s+(?:public\s+)?(?:"([^"]+)"|\'([^\']+)\')\s*;\s*$')
OPTION_RE = re.compile(r'^\s*option\s+.*;\s*$')

# Drop file-level options we don't want to carry into the merged file
# - option (validate.validate) = ...
# - language/tooling specific: java_*, go_package, csharp_namespace, php_*, swift_prefix,
#   objc_class_prefix, ruby_package, cc_*, optimize_for, php_namespace, php_class_prefix,
#   php_metadata_namespace
FILE_OPTION_DROP_RE = re.compile(
    r'^\s*option\s+(?:\(\s*validate\.validate\s*\)\s*=|'
    r'java_\w+\s*=|'
    r'go_package\s*=|'
    r'csharp_namespace\s*=|'
    r'php_\w+\s*=|'
    r'swift_prefix\s*=|'
    r'objc_class_prefix\s*=|'
    r'ruby_package\s*=|'
    r'cc_\w+\s*=|'
    r'optimize_for\s*=|'
    r'php_namespace\s*=|'
    r'php_class_prefix\s*=|'
    r'php_metadata_namespace\s*=)')

# Start of top-level declarations we dedupe by signature
DECL_KINDS = ("message", "enum", "service", "extend")
DECL_START_RE = re.compile(r'^\s*(message|enum|service|extend)\s+([A-Za-z_]\w*)\b')

# For attaching comments above a declaration
COMMENT_RE = re.compile(r'^\s*(//.*|/\*.*\*/\s*)$')

@dataclass(frozen=True)
class DeclSig:
    kind: str
    name: str

@dataclass
class DeclBlock:
    sig: DeclSig
    text: str
    source_file: Path
    start_line: int  # 1-based
    end_line: int    # 1-based, inclusive

def iter_proto_files(inputs: List[Path]) -> List[Path]:
    allp: List[Path] = []
    for p in inputs:
        if p.is_dir():
            for child in p.rglob("*.proto"):
                allp.append(child)
        elif p.suffix == ".proto" and p.exists():
            allp.append(p)
    # Stable order
    return sorted(set(allp))

def split_top_level_decls(lines: List[str], file_path: Path) -> Tuple[List[str], List[str], List[DeclBlock], List[str]]:
    """
    Returns (headers_without_imports, options, decl_blocks, imports)
    headers_without_imports includes everything before first decl except import/option,
    plus any other file-scope bits that aren't declarations (e.g., comments, empty lines).
    We also pick up options separately to dedupe later.
    """
    headers: List[str] = []
    options: List[str] = []
    imports: List[str] = []
    decls: List[DeclBlock] = []

    i = 0
    n = len(lines)

    def collect_decl(start_idx: int, kind: str, name: str) -> Tuple[int, DeclBlock]:
        # Capture from start_idx, count braces until balanced back to 0.
        brace = 0
        j = start_idx
        # Track preceding comment block captured already in pre-scan
        # We assume start_idx points at the first line of the decl including any comments.
        while j < n:
            line = lines[j]
            brace += line.count("{")
            brace -= line.count("}")
            j += 1
            if brace == 0 and line.strip().endswith("}"):
                break
        block_text = "".join(lines[start_idx:j]) + "\n"
        # Convert to 1-based line numbers
        return j, DeclBlock(DeclSig(kind, name), block_text, file_path, start_idx + 1, j)

    # We will walk, accumulating comments to attach to next decl if contiguous.
    pending_comments: List[str] = []

    while i < n:
        line = lines[i]

        # syntax / package / import / option are considered headers (handled elsewhere)
        mimp = IMPORT_RE.match(line)
        if mimp:
            # Collect all imports; we'll decide later which to keep
            # Extract the quoted path from the capture groups
            path = mimp.group(1) or mimp.group(2) or ""
            if path:
                imports.append(path)
            i += 1
            continue

        if OPTION_RE.match(line):
            # Keep only file-level options that are not in the drop list
            if not FILE_OPTION_DROP_RE.match(line):
                options.append(line.rstrip() + "\n")
            i += 1
            continue

        # Accumulate comments/blank lines to attach to next decl or keep in headers if before any decl
        if line.strip() == "" or line.lstrip().startswith("//") or line.lstrip().startswith("/*"):
            pending_comments.append(line)
            i += 1
            continue

        m = DECL_START_RE.match(line)
        if m:
            kind, name = m.group(1), m.group(2)
            # Attach any immediate leading comments to the declaration
            start_idx = i - len(pending_comments) if pending_comments else i
            j, block = collect_decl(start_idx, kind, name)
            decls.append(block)
            pending_comments = []  # consumed
            i = j
            continue

        # Not a decl: keep in headers and reset comment buffer (they are just file headers)
        if pending_comments:
            headers.extend(pending_comments)
            pending_comments = []
        headers.append(line)
        i += 1

    # trailing pending comments belong to headers
    if pending_comments:
        headers.extend(pending_comments)
        pending_comments = []

    # Remove any imports that might have slipped into headers (double safety)
    headers = [h for h in headers if not IMPORT_RE.match(h)]
    return headers, options, decls, imports

def read_file_text(p: Path) -> List[str]:
    with p.open("r", encoding="utf-8") as f:
        return f.readlines()

def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description="Flatten multiple .proto files into one.")
    ap.add_argument("--in", dest="inputs", action="append", required=True,
                    help="Input file or directory (repeatable)")
    ap.add_argument("--out", dest="out", required=True, help="Output .proto path")
    ap.add_argument("--package", dest="package", default=None,
                    help="Override/force the package name for the merged file")
    ap.add_argument("--force-syntax", dest="force_syntax", choices=["proto2", "proto3"], default=None,
                    help="Force a particular syntax if files disagree")
    ap.add_argument("--prefer-first", action="store_true",
                    help="If duplicate declarations appear, keep the first and skip the rest with warnings")
    ap.add_argument("--sort", action="store_true",
                    help="Sort declarations by kind then name for deterministic output")
    ap.add_argument("--verbose", action="store_true", help="Verbose logging")
    ap.add_argument("--conflict-report", dest="conflict_report", action="store_true",
                    help="Report duplicates and reserved range overlaps instead of stopping")
    ap.add_argument("--annotate-source", dest="annotate_source", action="store_true",
                    help="Annotate each declaration with original file and line span")
    ap.add_argument("--emit-toc", dest="emit_toc", action="store_true",
                    help="Emit a table-of-contents listing merged declarations and their origins")
    args = ap.parse_args(argv)

    in_paths = [Path(p) for p in args.inputs]
    files = iter_proto_files(in_paths)
    if not files:
        print("No .proto files found.", file=sys.stderr)
        return 2

    if args.verbose:
        for f in files:
            print(f"[+] found: {f}", file=sys.stderr)

    syntaxes: Set[str] = set()
    packages_found: List[str] = []
    file_options_all: Set[str] = set()
    all_imports: Set[str] = set()
    decl_blocks: Dict[DeclSig, DeclBlock] = {}
    duplicates: Dict[DeclSig, List[DeclBlock]] = {}

    headers_all: List[str] = []  # collect pre-decl content (w/o imports/options)

    for fp in files:
        lines = read_file_text(fp)

        # collect syntax and package from file header (where they usually are), but don't enforce position
        for line in lines:
            m = SYNTAX_RE.match(line)
            if m:
                syntaxes.add(m.group(1))
            pm = PACKAGE_RE.match(line)
            if pm:
                packages_found.append(pm.group(1))

        headers, options, decls, imports = split_top_level_decls(lines, fp)

        # Keep non-import, non-option headers (comments, custom file banners, etc.) in the merged header once
        # We'll de-duplicate later by text to avoid repetition.
        headers_all.extend(headers)

        # Merge options: dedupe by full line text
        for opt in options:
            file_options_all.add(opt)

        # Collect imports as paths (dedup later)
        for imp in imports:
            all_imports.add(imp)

        for d in decls:
            if d.sig in decl_blocks:
                # possible duplicate
                if args.conflict_report or args.prefer_first:
                    # Keep the first, record duplicate
                    duplicates.setdefault(d.sig, [decl_blocks[d.sig]]).append(d)
                    if args.verbose:
                        print(f"[!] duplicate {d.sig.kind} {d.sig.name} in {fp}, keeping first", file=sys.stderr)
                    continue
                else:
                    print(f"[X] duplicate top-level declaration: {d.sig.kind} {d.sig.name} also in another file", file=sys.stderr)
                    print("    Use --prefer-first or --conflict-report to proceed.", file=sys.stderr)
                    return 3
            decl_blocks[d.sig] = d

    # Helper to detect reserved range overlaps inside a declaration block
    def parse_reserved_ranges(block_text: str) -> List[Tuple[int, int]]:
        ranges: List[Tuple[int, int]] = []
        for line in block_text.splitlines():
            if not line.strip().startswith("reserved"):
                continue
            if '"' in line or "'" in line:
                # name reservations, ignore for numeric overlap purposes
                continue
            # Extract tokens between 'reserved' and ';'
            m = re.search(r"reserved(.*?);", line)
            if not m:
                continue
            body = m.group(1)
            tokens = [t.strip() for t in body.split(',') if t.strip()]
            for tok in tokens:
                mt = re.match(r"^(\d+)\s+to\s+(\d+)$", tok)
                ms = re.match(r"^(\d+)$", tok)
                if mt:
                    a, b = int(mt.group(1)), int(mt.group(2))
                    if a > b:
                        a, b = b, a
                    ranges.append((a, b))
                elif ms:
                    v = int(ms.group(1))
                    ranges.append((v, v))
        # Merge-sort to detect overlaps later
        return sorted(ranges)

    def find_overlaps(ranges: List[Tuple[int, int]]) -> List[Tuple[Tuple[int, int], Tuple[int, int]]]:
        overlaps: List[Tuple[Tuple[int, int], Tuple[int, int]]] = []
        prev: Optional[Tuple[int, int]] = None
        for r in ranges:
            if prev and r[0] <= prev[1]:
                overlaps.append((prev, r))
                # expand prev to union for chained overlap detection
                prev = (prev[0], max(prev[1], r[1]))
            else:
                prev = r
        return overlaps

    # Resolve syntax
    if args.force_syntax:
        syntax = args.force_syntax
    else:
        if len(syntaxes) == 0:
            # default to proto3 if nothing stated
            syntax = "proto3"
        elif len(syntaxes) == 1:
            (syntax,) = tuple(syntaxes)
        else:
            print(f"[X] conflicting syntaxes detected: {sorted(syntaxes)} (use --force-syntax to choose)", file=sys.stderr)
            return 4

    # Resolve package
    final_package = args.package
    if not final_package:
        # If multiple packages are present, pick the most common one and warn.
        if packages_found:
            from collections import Counter
            c = Counter(packages_found)
            final_package, count = c.most_common(1)[0]
            if len(c) > 1:
                print(f"[!] multiple packages detected {dict(c)}; choosing '{final_package}'. Use --package to override.", file=sys.stderr)

    # Dedupe header comment lines and keep only commenty stuff (no imports/options)
    seen_header_lines: Set[str] = set()
    merged_header_lines: List[str] = []
    for h in headers_all:
        # keep only comments or blank lines in the big merged header
        if h.strip() == "" or h.lstrip().startswith("//") or h.lstrip().startswith("/*"):
            if h not in seen_header_lines:
                merged_header_lines.append(h)
                seen_header_lines.add(h)

    # Prepare declaration list
    decl_list = list(decl_blocks.values())
    if args.sort:
        kind_order = {k: i for i, k in enumerate(DECL_KINDS)}
        decl_list.sort(key=lambda b: (kind_order.get(b.sig.kind, 99), b.sig.name))

    # Compose output
    out_lines: List[str] = []
    out_lines.append(f'syntax = "{syntax}";\n\n')
    if final_package:
        out_lines.append(f'package {final_package};\n\n')

    # Determine which external imports are still needed in the merged file.
    # Strategy:
    # - Never re-import files that are part of the merged content (internal).
    # - Add validate.proto if validation extensions are used anywhere in merged content.
    # - Add well-known type imports if their types appear in merged content.
    # Build set of internal import names relative to provided source roots.
    source_roots: List[Path] = [p for p in in_paths if p.is_dir()]
    internal_import_names: Set[str] = set()
    internal_basenames: Set[str] = set()
    for fp in files:
        for root in source_roots:
            try:
                rel = Path(fp).resolve().relative_to(root.resolve())
            except Exception:
                continue
            internal_import_names.add(rel.as_posix())
            internal_basenames.add(Path(rel).name)
            # Also add without leading './' just in case
            break

    # Prepare the merged declarations text to search for type/extension usage
    merged_decls_text = "".join([b.text for b in (list(decl_blocks.values()))])

    needed_imports: Set[str] = set()
    # If any validation extensions are present, import validate.proto if available in inputs
    if re.search(r'\(\s*validate\.', merged_decls_text):
        # Prefer an existing path from collected imports; else use default path in this repo
        candidate = None
        for imp in all_imports:
            if imp.endswith('validate.proto'):
                candidate = imp
                break
        if candidate is None:
            candidate = 'generator/proto/validate.proto'
        needed_imports.add(candidate)

    # Map of well-known type name (as it appears in .proto) to import path
    WELL_KNOWN_TYPES = {
        'google.protobuf.Timestamp': 'google/protobuf/timestamp.proto',
        'google.protobuf.Duration': 'google/protobuf/duration.proto',
        'google.protobuf.Any': 'google/protobuf/any.proto',
        'google.protobuf.Empty': 'google/protobuf/empty.proto',
        'google.protobuf.Struct': 'google/protobuf/struct.proto',
        'google.protobuf.Value': 'google/protobuf/struct.proto',
        'google.protobuf.ListValue': 'google/protobuf/struct.proto',
        'google.protobuf.FieldMask': 'google/protobuf/field_mask.proto',
        # Wrappers
        'google.protobuf.DoubleValue': 'google/protobuf/wrappers.proto',
        'google.protobuf.FloatValue': 'google/protobuf/wrappers.proto',
        'google.protobuf.Int64Value': 'google/protobuf/wrappers.proto',
        'google.protobuf.UInt64Value': 'google/protobuf/wrappers.proto',
        'google.protobuf.Int32Value': 'google/protobuf/wrappers.proto',
        'google.protobuf.UInt32Value': 'google/protobuf/wrappers.proto',
        'google.protobuf.BoolValue': 'google/protobuf/wrappers.proto',
        'google.protobuf.StringValue': 'google/protobuf/wrappers.proto',
        'google.protobuf.BytesValue': 'google/protobuf/wrappers.proto',
    }

    for typename, proto_path in WELL_KNOWN_TYPES.items():
        if typename in merged_decls_text:
            needed_imports.add(proto_path)

    # Only include imports that are not internal
    final_imports = []
    for imp in sorted(needed_imports):
        base = Path(imp).name
        if imp in internal_import_names:
            continue
        if base in internal_basenames:
            # An internal file with same basename is already merged
            continue
        final_imports.append(imp)
    if final_imports:
        for imp in final_imports:
            out_lines.append(f'import "{imp}";\n')
        out_lines.append("\n")

    # File-level options (filtered earlier)
    if file_options_all:
        for opt in sorted(file_options_all):
            out_lines.append(opt if opt.endswith("\n") else opt + "\n")
        out_lines.append("\n")

    # Header comments
    if merged_header_lines:
        out_lines.extend(merged_header_lines)
        if not out_lines[-1].endswith("\n"):
            out_lines.append("\n")
        out_lines.append("\n")

    # Optional TOC
    if args.emit_toc:
        out_lines.append("// Table of contents\n")
        for b in decl_list:
            rel = None
            try:
                # Show path relative to CWD
                rel = b.source_file.as_posix()
            except Exception:
                rel = str(b.source_file)
            out_lines.append(f"// - {b.sig.kind} {b.sig.name}  ({rel}:{b.start_line}-{b.end_line})\n")
        out_lines.append("\n")

    # Definitions
    for b in decl_list:
        if args.annotate_source:
            rel = None
            try:
                rel = b.source_file.as_posix()
            except Exception:
                rel = str(b.source_file)
            out_lines.append(f"// begin: {b.sig.kind} {b.sig.name} ({rel}:{b.start_line}-{b.end_line})\n")
        out_lines.append(b.text)
        if args.annotate_source:
            out_lines.append(f"// end: {b.sig.kind} {b.sig.name}\n")
        if not out_lines[-1].endswith("\n"):
            out_lines.append("\n")

    # Final sanitize: remove any stray import lines or dropped option lines outside the header import block
    composed = "".join(out_lines)
    lines = composed.splitlines(keepends=False)

    # Build a set of allowed import lines as inserted earlier
    allowed_import_lines = {f'import "{imp}";' for imp in final_imports}

    sanitized: List[str] = []
    for idx, line in enumerate(lines):
        # Drop file-level options we don't want anywhere in the file
        if FILE_OPTION_DROP_RE.match(line):
            continue
        # Drop any import lines that are not in our allowed set
        if IMPORT_RE.match(line):
            if line.strip().rstrip(";") + ";" in allowed_import_lines:
                sanitized.append(line)
            # else skip stray import
            continue
        sanitized.append(line)

    # Ensure a trailing newline at EOF
    if sanitized and sanitized[-1] != "":
        sanitized_text = "\n".join(sanitized) + "\n"
    else:
        sanitized_text = "\n".join(sanitized)

    # Rewrite qualified references from original packages to local names for merged types
    # e.g., demo.structures.ColoredPoint -> ColoredPoint, demo.enums.Color -> Color
    # Skip google.protobuf.*
    decl_type_names: Set[str] = {d.sig.name for d in decl_blocks.values() if d.sig.kind in {"message", "enum"}}
    packages_unique: Set[str] = set(packages_found)
    if packages_unique and decl_type_names:
        for pkg in sorted(packages_unique, key=len, reverse=True):
            if pkg == "google.protobuf":
                continue
            for name in decl_type_names:
                pattern = re.compile(rf"\b{re.escape(pkg)}\.{re.escape(name)}\b")
                sanitized_text = pattern.sub(name, sanitized_text)

    # Write
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(sanitized_text, encoding="utf-8")

    if args.verbose:
        print(f"[OK] Wrote {out_path} ({len(decl_list)} declarations).", file=sys.stderr)

    # Conflict / overlap reporting
    if args.conflict_report:
        if duplicates:
            print("[Conflict report] Duplicate declarations detected:", file=sys.stderr)
            for sig, blocks in duplicates.items():
                # First is the kept one (inserted when we first saw dup)
                kept = blocks[0]
                others = blocks[1:]
                print(f"  - {sig.kind} {sig.name}", file=sys.stderr)
                print(f"      kept: {kept.source_file}:{kept.start_line}-{kept.end_line}", file=sys.stderr)
                for ob in others:
                    print(f"      skip: {ob.source_file}:{ob.start_line}-{ob.end_line}", file=sys.stderr)
                # Suggest prefer-first mask
                print(f"      suggest: --prefer-first   (will keep first occurrence of {sig.kind} {sig.name})", file=sys.stderr)
        # Reserved overlaps within each block
        any_overlap = False
        for b in decl_list:
            ranges = parse_reserved_ranges(b.text)
            overlaps = find_overlaps(ranges)
            if overlaps:
                any_overlap = True
                print(f"[Reserved overlap] {b.sig.kind} {b.sig.name} in {b.source_file}:{b.start_line}-{b.end_line}", file=sys.stderr)
                for a, c in overlaps:
                    print(f"    ranges {a[0]}-{a[1]} overlaps {c[0]}-{c[1]}", file=sys.stderr)
        if not duplicates and not any_overlap:
            print("[Conflict report] No duplicates or reserved range overlaps detected.", file=sys.stderr)

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
