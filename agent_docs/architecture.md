# Architecture

## Core mental model

Nanopb is split into a tiny portable C runtime and a Python generator that turns protobuf schemas into C structs plus descriptor tables. Most tasks fall on one side of that boundary:

- Runtime-side work changes encoding, decoding, descriptors, stream handling, or validation behavior in `/home/runner/work/nanopb/nanopb/pb*.c` and `/home/runner/work/nanopb/nanopb/pb*.h`
- Generator-side work changes how `.proto` input becomes generated `.pb.h`, `.pb.c`, and optional `*_validate.*` files under `/home/runner/work/nanopb/nanopb/generator/`

## Runtime pieces

- `/home/runner/work/nanopb/nanopb/pb.h` defines compile-time feature switches and shared types. Start here when behavior depends on macros such as malloc support, 32-bit field sizes, packed structs, or UTF-8 validation.
- `/home/runner/work/nanopb/nanopb/pb_common.c` contains descriptor and shared support code used by both encode and decode paths.
- `/home/runner/work/nanopb/nanopb/pb_encode.c` implements stream-oriented protobuf encoding.
- `/home/runner/work/nanopb/nanopb/pb_decode.c` implements stream-oriented protobuf decoding.
- `/home/runner/work/nanopb/nanopb/pb_validate.c` adds optional generated-message validation support.

The runtime intentionally uses stream abstractions instead of depending only on memory buffers. That keeps it suitable for embedded transport and file/network use without requiring large temporary allocations.

## Generator pieces

- `/home/runner/work/nanopb/nanopb/generator/nanopb_generator.py` is the main entrypoint and owns parsing, IR construction, naming, and C emission.
- `/home/runner/work/nanopb/nanopb/generator/nanopb_validator.py` extends generation when validation is enabled.
- `/home/runner/work/nanopb/nanopb/generator/proto/` contains generator-owned schema files used by the plugin itself.
- `/home/runner/work/nanopb/nanopb/generator/protoc-gen-nanopb` and related wrappers expose the generator as a `protoc` plugin.

The generator is the architectural center for schema semantics: if generated code shape looks wrong, the fix is usually here rather than in handwritten runtime code.

## Validation path

Validation is a full cross-cutting feature:

- Schema options live in `/home/runner/work/nanopb/nanopb/generator/proto/validate.proto`
- Generation logic lives in `/home/runner/work/nanopb/nanopb/generator/nanopb_validator.py`
- Runtime support lives in `/home/runner/work/nanopb/nanopb/pb_validate.c` and `/home/runner/work/nanopb/nanopb/pb_validate.h`
- End-to-end tests live in `/home/runner/work/nanopb/nanopb/tests/validation/`

If you change any one of those layers, verify the others still agree.

## Design choices that show up in reviews

- Generated descriptors replace runtime reflection to minimize code size and RAM use.
- The runtime can be built as encode-only or decode-only, so avoid unnecessary coupling between those paths.
- Compatibility is validated across many build systems; changes to packaging or integration files are treated as product code, not as secondary tooling.
- Tests act as executable specification. Look for an existing test directory before deciding behavior is undefined.
