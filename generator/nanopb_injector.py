#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb_injector.py - Inject custom code into nanopb-generated files
===================================================================

This script extends nanopb-generated .pb.h and .pb.c files by injecting
custom decode/validate helper functions at protoc insertion points.

The script follows a minimal-fork philosophy:
- Reuses nanopb's ProtoFile, Message, and Field models
- Does not modify nanopb_generator.py core logic
- Injects code at /* @@protoc_insertion_point(eof) */ markers
- Uses BEGIN/END markers for idempotent injection

Architecture
------------
1. Parse the .proto file using nanopb_generator.parse_file()
2. Generate custom function declarations and definitions from the model
3. Inject code into existing .pb.h and .pb.c files
4. Maintain idempotency with generated block markers

Usage
-----
After running nanopb_generator with --protoc-insertion-points:
    python nanopb_injector.py -I proto -D generated proto/file.proto

Or use the wrapper script:
    python nanopb_generate_extended.py -I proto -D generated proto/file.proto
"""

from __future__ import unicode_literals

import sys
import os
import re

# Import nanopb_generator to reuse its models
try:
    # Add generator directory to path
    generator_dir = os.path.dirname(os.path.abspath(__file__))
    if generator_dir not in sys.path:
        sys.path.insert(0, generator_dir)
    
    import nanopb_generator
except ImportError as e:
    sys.stderr.write(f"Error: Could not import nanopb_generator: {e}\n")
    sys.exit(1)

# Markers for idempotent injection
BEGIN_MARKER = "/* BEGIN NANOPB INJECTED CODE */"
END_MARKER = "/* END NANOPB INJECTED CODE */"

class CodeInjector:
    """
    Handles injection of custom code into nanopb-generated files.
    
    The injector:
    - Reads existing .pb.h and .pb.c files
    - Finds the /* @@protoc_insertion_point(eof) */ markers
    - Injects custom code with BEGIN/END markers
    - Ensures idempotency by replacing existing injected blocks
    """
    
    def __init__(self, proto_file, options):
        """
        Initialize the injector with a parsed ProtoFile.
        
        Args:
            proto_file: ProtoFile instance from nanopb_generator.parse_file()
            options: Command line options
        """
        self.proto_file = proto_file
        self.options = options
    
    def generate_custom_header_code(self):
        """
        Generate custom function declarations for the header file.
        
        Returns:
            String containing C function declarations
        """
        lines = []
        lines.append("\n" + BEGIN_MARKER)
        lines.append("/* Custom decode/validate helper functions */")
        lines.append("")
        
        # Generate a helper declaration for each message
        for message in self.proto_file.messages:
            msg_name = str(message.name)
            
            # Example: Custom decode helper
            lines.append(f"/* Decode helper for {msg_name} */")
            lines.append(f"bool {msg_name}_decode_helper(pb_istream_t *stream, {msg_name} *msg);")
            lines.append("")
            
            # Example: Custom validate helper
            lines.append(f"/* Validate helper for {msg_name} */")
            lines.append(f"bool {msg_name}_validate_helper(const {msg_name} *msg);")
            lines.append("")
        
        lines.append(END_MARKER)
        return '\n'.join(lines)
    
    def generate_custom_source_code(self):
        """
        Generate custom function definitions for the source file.
        
        Returns:
            String containing C function definitions
        """
        lines = []
        lines.append("\n" + BEGIN_MARKER)
        lines.append("/* Custom decode/validate helper function implementations */")
        lines.append("")
        
        # Generate a helper implementation for each message
        for message in self.proto_file.messages:
            msg_name = str(message.name)
            
            # Example: Custom decode helper implementation
            lines.append(f"/* Decode helper for {msg_name} */")
            lines.append(f"bool {msg_name}_decode_helper(pb_istream_t *stream, {msg_name} *msg) {{")
            lines.append(f"    /* Custom decode logic for {msg_name} */")
            lines.append(f"    if (!pb_decode(stream, {msg_name}_fields, msg)) {{")
            lines.append("        return false;")
            lines.append("    }")
            
            # Add custom field-level processing
            # Note: 'skip_message' is set by nanopb for fields that shouldn't be processed
            # (e.g., OneOf union placeholders). We skip those to avoid duplicates.
            for field in message.fields:
                if hasattr(field, 'name') and not hasattr(field, 'skip_message'):
                    field_name = field.name
                    lines.append(f"    /* Custom processing for field: {field_name} */")
            
            lines.append("    return true;")
            lines.append("}")
            lines.append("")
            
            # Example: Custom validate helper implementation
            lines.append(f"/* Validate helper for {msg_name} */")
            lines.append(f"bool {msg_name}_validate_helper(const {msg_name} *msg) {{")
            lines.append(f"    /* Custom validation logic for {msg_name} */")
            lines.append("    if (msg == NULL) {")
            lines.append("        return false;")
            lines.append("    }")
            
            # Add custom field-level validation
            # Note: 'skip_message' is set by nanopb for fields that shouldn't be processed
            # (e.g., OneOf union placeholders). We skip those to avoid duplicates.
            for field in message.fields:
                if hasattr(field, 'name') and not hasattr(field, 'skip_message'):
                    field_name = field.name
                    lines.append(f"    /* Custom validation for field: {field_name} */")
            
            lines.append("    return true;")
            lines.append("}")
            lines.append("")
        
        lines.append(END_MARKER)
        return '\n'.join(lines)
    
    def inject_into_file(self, filepath, custom_code):
        """
        Inject custom code into a file at the eof insertion point.
        
        Args:
            filepath: Path to the .pb.h or .pb.c file
            custom_code: String containing the code to inject
        
        Returns:
            True if injection was successful, False otherwise
        """
        if not os.path.exists(filepath):
            sys.stderr.write(f"Warning: File not found: {filepath}\n")
            return False
        
        # Read the existing file
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Check if insertion point exists
        insertion_point = "/* @@protoc_insertion_point(eof) */"
        if insertion_point not in content:
            sys.stderr.write(f"Warning: No eof insertion point found in {filepath}\n")
            sys.stderr.write("         Run nanopb_generator with --protoc-insertion-points flag\n")
            return False
        
        # Remove existing injected code if present
        pattern = re.compile(
            re.escape(BEGIN_MARKER) + r'.*?' + re.escape(END_MARKER),
            re.DOTALL
        )
        content = pattern.sub('', content)
        
        # Find the insertion point and inject the code
        parts = content.split(insertion_point)
        if len(parts) != 2:
            sys.stderr.write(f"Warning: Multiple or no eof insertion points in {filepath}\n")
            return False
        
        # Inject code before the insertion point marker
        new_content = parts[0] + custom_code + "\n\n" + insertion_point + parts[1]
        
        # Write back to file
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        return True

def inject_file(filename, options):
    """
    Process a single .proto file and inject custom code into generated files.
    
    Args:
        filename: Path to the .proto file
        options: Command line options
    
    Returns:
        True if successful, False otherwise
    """
    # Compile .proto file to .pb if needed
    fdesc = None
    if filename.endswith(".proto"):
        try:
            import tempfile
            import google.protobuf.descriptor_pb2 as descriptor
            from nanopb_generator import TemporaryDirectory, invoke_protoc
            
            # Compile .proto to .pb
            include_path = ['-I%s' % p for p in options.options_path]
            with TemporaryDirectory() as tmpdir:
                tmpname = os.path.join(tmpdir, os.path.basename(filename) + ".pb")
                args = ["protoc"] + include_path
                args += options.protoc_opts
                args += ['--include_imports', '--include_source_info', '-o' + tmpname, filename]
                status = invoke_protoc(args)
                if status != 0:
                    sys.stderr.write(f"Error: protoc failed with exit code {status}\n")
                    return False
                data = open(tmpname, 'rb').read()
                fdescs = descriptor.FileDescriptorSet.FromString(data).file
                fdesc = fdescs[-1]
        except Exception as e:
            sys.stderr.write(f"Error compiling {filename}: {e}\n")
            return False
    
    # Parse the proto file using nanopb_generator
    try:
        # Use fdesc.name (the proto-relative path) for parsing, not the filesystem path
        proto_file = nanopb_generator.parse_file(fdesc.name if fdesc else filename, fdesc, options)
    except Exception as e:
        sys.stderr.write(f"Error parsing {filename}: {e}\n")
        return False
    
    # Determine the generated file paths using the same logic as process_file
    # Use fdesc.name which is the proto-relative path (e.g., "simple.proto")
    proto_name = fdesc.name if fdesc else filename
    noext = os.path.splitext(proto_name)[0]
    
    headername = noext + options.extension + options.header_extension
    sourcename = noext + options.extension + options.source_extension
    
    # Write to output_dir if specified
    base_dir = options.output_dir or ''
    header_path = os.path.join(base_dir, headername)
    source_path = os.path.join(base_dir, sourcename)
    
    # Create injector
    injector = CodeInjector(proto_file, options)
    
    # Generate custom code
    header_code = injector.generate_custom_header_code()
    source_code = injector.generate_custom_source_code()
    
    # Inject into files
    success = True
    
    if not options.quiet:
        sys.stderr.write(f"Injecting custom code into {header_path}\n")
    if not injector.inject_into_file(header_path, header_code):
        success = False
    
    if not options.quiet:
        sys.stderr.write(f"Injecting custom code into {source_path}\n")
    if not injector.inject_into_file(source_path, source_code):
        success = False
    
    return success

def main():
    """Main entry point for the injector script."""
    
    # Reuse nanopb_generator's option parser
    from nanopb_generator import optparser, process_cmdline, Globals
    
    # Update usage message
    optparser.usage = "python nanopb_injector.py [options] file.proto"
    optparser.epilog = "Injects custom code into .pb.h and .pb.c files at eof insertion points."
    
    # Parse command line options
    options, filenames = process_cmdline(sys.argv[1:], is_plugin=False)
    
    if not filenames:
        optparser.print_help()
        sys.stderr.write("\nError: No input files specified.\n")
        sys.exit(1)
    
    # Process each file
    success = True
    for filename in filenames:
        if not filename.endswith('.proto'):
            sys.stderr.write(f"Warning: Skipping non-.proto file: {filename}\n")
            continue
        
        if not inject_file(filename, options):
            success = False
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
