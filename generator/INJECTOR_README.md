# Nanopb Injector System

This directory contains an extension system for nanopb code generation that follows a minimal-fork philosophy.

## Overview

The injector system allows you to add custom decode/validate helper functions to nanopb-generated files without modifying the core generator. It works by:

1. Running `nanopb_generator.py` with `--protoc-insertion-points` to add insertion point markers
2. Using `nanopb_injector.py` to inject custom code at those markers
3. Maintaining idempotency with BEGIN/END markers

## Files

### nanopb_injector.py

A separate script that:
- Imports and reuses nanopb_generator's ProtoFile, Message, and Field models
- Compiles .proto files using protoc (just like the generator)
- Generates custom decode/validate helper functions from the parsed model
- Injects code at `/* @@protoc_insertion_point(eof) */` markers in .pb.h and .pb.c
- Uses `/* BEGIN/END NANOPB INJECTED CODE */` markers for idempotent injection

### nanopb_generate_extended.py

A wrapper script that:
- Runs nanopb_generator.py with --protoc-insertion-points automatically
- Runs nanopb_injector.py with the same arguments
- Provides a single command for extended generation

## Usage

### Two-step approach

```bash
# Step 1: Generate normal .pb.h/.pb.c with insertion points
python generator/nanopb_generator.py --protoc-insertion-points -I proto -D generated proto/file.proto

# Step 2: Inject custom code at insertion points
python generator/nanopb_injector.py -I proto -D generated proto/file.proto
```

### One-command wrapper

```bash
python generator/nanopb_generate_extended.py -I proto -D generated proto/file.proto
```

Both approaches produce the same result: .pb.h and .pb.c files with custom helpers injected at EOF.

## Example Output

### Generated Header (.pb.h)

```c
/* ... normal nanopb declarations ... */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* BEGIN NANOPB INJECTED CODE */
/* Custom decode/validate helper functions */

/* Decode helper for MyMessage */
bool MyMessage_decode_helper(pb_istream_t *stream, MyMessage *msg);

/* Validate helper for MyMessage */
bool MyMessage_validate_helper(const MyMessage *msg);

/* END NANOPB INJECTED CODE */

/* @@protoc_insertion_point(eof) */

#endif
```

### Generated Source (.pb.c)

```c
/* ... normal nanopb definitions ... */

/* BEGIN NANOPB INJECTED CODE */
/* Custom decode/validate helper function implementations */

/* Decode helper for MyMessage */
bool MyMessage_decode_helper(pb_istream_t *stream, MyMessage *msg) {
    /* Custom decode logic for MyMessage */
    if (!pb_decode(stream, MyMessage_fields, msg)) {
        return false;
    }
    /* Custom processing for field: field1 */
    /* Custom processing for field: field2 */
    return true;
}

/* Validate helper for MyMessage */
bool MyMessage_validate_helper(const MyMessage *msg) {
    /* Custom validation logic for MyMessage */
    if (msg == NULL) {
        return false;
    }
    /* Custom validation for field: field1 */
    /* Custom validation for field: field2 */
    return true;
}

/* END NANOPB INJECTED CODE */

/* @@protoc_insertion_point(eof) */
```

## Idempotency

Running the injector multiple times on the same files is safe. The injector:
1. Removes any existing `BEGIN/END NANOPB INJECTED CODE` blocks
2. Injects fresh code at the insertion point
3. Never duplicates the injected code

## Customization

To customize the generated helper functions, modify the `CodeInjector` class in `nanopb_injector.py`:

- `generate_custom_header_code()`: Customize function declarations
- `generate_custom_source_code()`: Customize function implementations

The injector has access to the full nanopb model (ProtoFile, Message, Field) so you can inspect:
- Message names and fields
- Field types, rules, and options
- Nested messages and enums
- Validation rules (if enabled)

## Design Philosophy

This system follows the minimal-fork philosophy:

✅ **Prefer new files over editing existing generator logic**
- Created separate nanopb_injector.py instead of modifying nanopb_generator.py
- Created wrapper script instead of changing the generator interface

✅ **Keep generator as unchanged as possible**
- Zero changes to nanopb_generator.py core logic
- Reuses existing --protoc-insertion-points feature
- Imports and reuses existing ProtoFile/Message/Field models

✅ **Extension by second pass, not a deep fork**
- Generator runs first and completes normally
- Injector runs second as a post-processing step
- Clean separation of concerns

✅ **Idempotent with markers**
- BEGIN/END markers allow re-running safely
- No accumulation of duplicate code

## Limitations

- Requires nanopb_generator.py to be run with --protoc-insertion-points
- Only injects at EOF insertion points (not at includes or per-struct points)
- The example helper functions are templates - customize for your use case

## Future Extensions

Possible enhancements while maintaining the minimal-fork philosophy:

- Add command-line options to customize generated function names/signatures
- Support injection at other insertion points (includes, per-struct)
- Generate different types of helpers (serialization, deep copy, etc.)
- Read configuration from .options files
- Support custom templates for generated code
