# Nanopb Validation Test

This test demonstrates and validates the nanopb validation feature using the `--validate` flag with `nanopb_generator.py`.

## Overview

The validation feature allows you to define constraints in your `.proto` files using the `validate.proto` extensions. The generator then creates validation functions that can check if messages conform to these constraints at runtime.

## Files

- `validation_test.proto` - Proto file with validation constraints
- `validation_test.options` - Nanopb options for field sizes
- `test_validation.c` - C test code that exercises the validation functions
- `Makefile` - Build system for the test
- `validate_stub.pb.h` - Stub header for validate.proto (only used by Python generator)

## Building

```bash
cd tests/validation
make
```

This will:
1. Run `protoc` with the `--nanopb_opt=--validate` flag
2. Generate:
   - `validation_test.pb.c/h` - Standard nanopb message structures
   - `validation_test_validate.c/h` - Validation functions
3. Compile everything with the nanopb runtime and pb_validate.c
4. Link into the `test_validation` executable

## Running

```bash
cd tests/validation
./test_validation
```

Expected output:
```
===================================
  Nanopb Validation Test
===================================

Test 1: Valid Person
  PASS - Valid person accepted

Test 2: Invalid Person (empty name)
  PASS - Empty name rejected

===================================
  All tests PASSED!
===================================
```

## Validation Rules Tested

The test validates:

### Person message
- **name**: 
  - `min_len = 1` - Name must not be empty
  - `max_len = 50` - Name cannot exceed 50 characters
  - `required = true` - Name field must be set
- **email**:
  - `min_len = 3` - Minimum 3 characters
  - `max_len = 100` - Maximum 100 characters
  - `contains = "@"` - Must contain @ symbol
- **age**:
  - `gte = 0` - Must be >= 0
  - `lte = 150` - Must be <= 150
- **gender**:
  - `defined_only = true` - Must be a valid enum value
- **tags** (repeated):
  - `min_items = 0` - At least 0 items
  - `max_items = 10` - At most 10 items
- **phone** (optional):
  - `min_len = 10` - If set, must be at least 10 characters

### Contact message
- Tests oneof validation (various constraints on union fields)

### Company message
- **name**: `required = true`

## How It Works

1. **Proto Definition**: Define validation rules using `(validate.rules)` extensions in your `.proto` file
2. **Code Generation**: Run nanopb generator with `--validate` flag
3. **Generated Functions**: For each message `Foo`, a `pb_validate_Foo()` function is generated
4. **Runtime Validation**: Call the validation function to check if a message is valid:
   ```c
   pb_violations_t violations;
   pb_violations_init(&violations);
   
   bool valid = pb_validate_Person(&person, &violations);
   if (!valid) {
       // Handle validation errors in violations
   }
   ```

## Notes

- The validation feature requires C99 (uses `inline` keyword in pb_validate.h)
- Validation is performed on constructed messages before encoding or after decoding
- The `pb_violations_t` structure can accumulate multiple violations for detailed error reporting
- Message-level constraints (like `requires`) are not fully implemented yet
- Nested message validation is under development

## See Also

- [Main validation documentation](../../README_validation_test.md)
- [validate.proto](../../generator/proto/validate.proto) - Full validation rule definitions
- [pb_validate.h](../../pb_validate.h) - Runtime validation API
