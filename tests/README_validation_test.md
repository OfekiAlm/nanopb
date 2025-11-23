# Validation Test Harness

This repository contains a tiny C test that exercises the generated validation logic for `test_basic_validation.proto` using nanopb.

## Prerequisites

- A C11-capable compiler (e.g., `cc`/`gcc`)
- This repo checked out and built/generated once so that `gengen/test_basic_validation.pb.c` and `gengen/test_basic_validation_validate.c` exist.

## Build and Run

The following commands build the test binary and run it. They match what our CI does:

```sh
# Build
cc -std=c11 -O2 \
  -I/workspace \
  -I/workspace/gengen \
  -o /workspace/build/validation_test \
  /workspace/pb_common.c \
  /workspace/pb_encode.c \
  /workspace/pb_decode.c \
  /workspace/pb_validate.c \
  /workspace/gengen/test_basic_validation.pb.c \
  /workspace/gengen/test_basic_validation_validate.c \
  /workspace/tests/validation_test_main.c

# Run
/workspace/build/validation_test
```

### Expected output (abridged)

- One violation printed for the invalid case (e.g., `int32.gte` for age)
- Encoded size printed for a valid message
- `All tests passed.`

## Notes

- String validation for callback-allocated fields is intentionally not generated yet (left as TODO in generated C). Numeric validations (gt/gte/lt/lte/eq) are in place.
- If you regenerate code (`generator/nanopb_generator.py --validate ...`), ensure the output `gengen/test_basic_validation_validate.*` exists before building this test.

## Command generation
```bash
/workspace/.venv/bin/python workspace/generator/nanopb_generator.py --validate -I /workspace -D /workspace/gengen -x generator/proto/nanopb.proto -x generator/proto/validate.proto /workspace/test_basic_validation.proto```