#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb_generate_extended.py - Wrapper for extended nanopb code generation
==========================================================================

This script provides a single-command interface for generating nanopb code
with custom decode/validate helpers injected at insertion points.

The script orchestrates a two-phase generation:
1. Run nanopb_generator.py with --protoc-insertion-points
2. Run nanopb_injector.py to inject custom helpers

This follows the minimal-fork philosophy by keeping the core generator
unchanged and adding extensions through a separate injection pass.

Usage
-----
    python nanopb_generate_extended.py [options] file.proto

All standard nanopb_generator options are supported. The --protoc-insertion-points
flag is automatically added.

Examples
--------
Generate extended code for a single proto file:
    python nanopb_generate_extended.py -I proto -D generated proto/file.proto

Generate with custom options:
    python nanopb_generate_extended.py -I proto -D generated --extension=.pb proto/file.proto

Architecture
------------
This wrapper simply invokes two existing scripts in sequence:
1. nanopb_generator.py (with --protoc-insertion-points automatically added)
2. nanopb_injector.py (with the same options passed through)

The result is .pb.h and .pb.c files with custom helpers injected at EOF.
"""

from __future__ import unicode_literals

import sys
import os
import subprocess

def main():
    """
    Main entry point for the extended generator wrapper.
    
    Executes:
    1. nanopb_generator.py with --protoc-insertion-points
    2. nanopb_injector.py with the same arguments
    """
    
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Paths to the two scripts we'll invoke
    generator_script = os.path.join(script_dir, 'nanopb_generator.py')
    injector_script = os.path.join(script_dir, 'nanopb_injector.py')
    
    # Verify scripts exist
    if not os.path.exists(generator_script):
        sys.stderr.write(f"Error: nanopb_generator.py not found at {generator_script}\n")
        sys.exit(1)
    
    if not os.path.exists(injector_script):
        sys.stderr.write(f"Error: nanopb_injector.py not found at {injector_script}\n")
        sys.exit(1)
    
    # Get command line arguments
    args = sys.argv[1:]
    
    if not args or '-h' in args or '--help' in args:
        print("nanopb_generate_extended.py - Generate nanopb code with custom helpers")
        print("")
        print("Usage: python nanopb_generate_extended.py [options] file.proto")
        print("")
        print("This wrapper runs:")
        print("  1. nanopb_generator.py --protoc-insertion-points [options] file.proto")
        print("  2. nanopb_injector.py [options] file.proto")
        print("")
        print("All standard nanopb_generator options are supported.")
        print("The --protoc-insertion-points flag is automatically added.")
        print("")
        print("Examples:")
        print("  python nanopb_generate_extended.py -I proto -D generated proto/file.proto")
        print("  python nanopb_generate_extended.py -D output --extension=.pb proto/*.proto")
        print("")
        
        if '--help' in args:
            print("For detailed nanopb_generator options, run:")
            print("  python nanopb_generator.py --help")
        
        sys.exit(0 if '--help' in args else 1)
    
    # Prepare arguments for nanopb_generator
    # Add --protoc-insertion-points if not already present
    generator_args = args[:]
    if '--protoc-insertion-points' not in generator_args:
        generator_args = ['--protoc-insertion-points'] + generator_args
    
    # Phase 1: Run nanopb_generator.py
    sys.stderr.write("=" * 70 + "\n")
    sys.stderr.write("Phase 1: Running nanopb_generator.py with insertion points\n")
    sys.stderr.write("=" * 70 + "\n")
    
    try:
        result = subprocess.run(
            [sys.executable, generator_script] + generator_args,
            check=True
        )
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"\nError: nanopb_generator.py failed with exit code {e.returncode}\n")
        sys.exit(e.returncode)
    except Exception as e:
        sys.stderr.write(f"\nError running nanopb_generator.py: {e}\n")
        sys.exit(1)
    
    # Phase 2: Run nanopb_injector.py
    sys.stderr.write("\n" + "=" * 70 + "\n")
    sys.stderr.write("Phase 2: Running nanopb_injector.py to inject custom code\n")
    sys.stderr.write("=" * 70 + "\n")
    
    try:
        result = subprocess.run(
            [sys.executable, injector_script] + args,
            check=True
        )
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"\nError: nanopb_injector.py failed with exit code {e.returncode}\n")
        sys.exit(e.returncode)
    except Exception as e:
        sys.stderr.write(f"\nError running nanopb_injector.py: {e}\n")
        sys.exit(1)
    
    # Success
    sys.stderr.write("\n" + "=" * 70 + "\n")
    sys.stderr.write("Extended generation completed successfully!\n")
    sys.stderr.write("=" * 70 + "\n")
    sys.exit(0)

if __name__ == '__main__':
    main()
