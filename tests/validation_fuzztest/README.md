# Validation Fuzzing Test

## Overview

This test fuzzes the nanopb validation functionality to ensure it handles corrupted and malicious data safely without crashes. It exercises the `--validate` flag of the nanopb generator, which generates runtime validation code for proto messages with validation constraints.

## What It Tests

The test verifies that:

1. **Validation constraints work correctly** - Valid messages pass validation while invalid ones are caught
2. **No crashes occur** - The validation code handles corrupted/random data safely
3. **Decode failures are handled** - Messages that fail to decode don't cause issues
4. **Validation properly detects violations** - Constraint violations are correctly identified

## Test Message Structure

The `FuzzMessage` proto includes various field types with validation constraints:

- `int32` with range constraints (0-150)
- `uint32` with upper bound constraint (≤1000)
- `int64` with range constraints (0-1000000)
- `float` with range constraints (0.0-100.0)
- `double` with range constraints (0.0-5.0)
- `enum` with defined_only validation
- `bool` field (no constraints)

## How It Works

1. **Generate valid message** - Creates a properly formatted message with valid values
2. **Encode** - Serializes the message to bytes
3. **Test valid message** - Decodes and validates the original message
4. **Corrupt data** - Flips random bytes in the encoded message
5. **Test corrupted message** - Attempts to decode and validate the corrupted data
6. **Test with random length** - Tests with truncated/extended message lengths

The test runs multiple iterations with different seeds to explore various corruption patterns.

## Output Statistics

The test reports:
- **Valid messages** - Messages that decoded successfully and passed validation
- **Validation detected** - Messages that decoded but failed validation (expected for corrupted data)
- **Decode failed** - Messages that couldn't be decoded (expected for heavily corrupted data)

## Integration with Build System

The test is integrated into the SCons build system and runs automatically as part of the test suite:

```bash
cd tests
scons validation_fuzztest
```

The test generates:
- `fuzz_validation.pb.c/h` - Generated nanopb structures
- `fuzz_validation_validate.c/h` - Generated validation functions
- `validation_fuzztest` - Compiled test binary

## Standalone Usage

The test can also be run manually:

```bash
# Run with specific seed and iteration count
./build/validation_fuzztest/validation_fuzztest 12345 500

# Run as stdin fuzzer (for AFL, etc.)
echo "random data" | ./build/validation_fuzztest/validation_fuzztest
```

## Extending the Test

To add more validation rules:

1. Edit `fuzz_validation.proto` to add new fields with constraints
2. Update `fuzz_validation.options` if needed (for allocation settings)
3. Update `generate_valid_message()` in `validation_fuzztest.c` to set new fields
4. Rebuild with `scons validation_fuzztest`

## Related Tests

- `/tests/fuzztest` - Core nanopb fuzzing (decode/encode robustness)
- `/tests/validation` - Validation functionality tests (not fuzz-based)
- `/examples/validation_simple` - Simple validation example
