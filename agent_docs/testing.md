# Testing

## Default test command

For most runtime and generator changes, use:

```bash
cd /home/runner/work/nanopb/nanopb/tests
scons
```

`/home/runner/work/nanopb/nanopb/.github/workflows/trigger_on_code_change.yml` uses this as the primary smoke test, and `/home/runner/work/nanopb/nanopb/tests/SConstruct` is the canonical local harness.

## Which tests to run

### Runtime or generator changes

Run the default `scons` suite first. The test tree covers encoding, decoding, callbacks, oneofs, proto3, regressions, validation, naming, and many generator edge cases.

### Validation changes

Make sure `/home/runner/work/nanopb/nanopb/tests/validation/` still passes, because validation spans schema options, generator output, and runtime support.

### Build-system or packaging changes

Run the command that matches the integration you edited:

- CMake: the commands from `/home/runner/work/nanopb/nanopb/.github/workflows/cmake.yml`
- Meson: the commands from `/home/runner/work/nanopb/nanopb/.github/workflows/meson.yml`
- Bazel: the commands from `/home/runner/work/nanopb/nanopb/.github/workflows/bazel.yml`
- PlatformIO: the flow in `/home/runner/work/nanopb/nanopb/.github/workflows/platformio_tests.yml`
- SwiftPM: `swift build && swift test` from `/home/runner/work/nanopb/nanopb/.github/workflows/ios_swift_tests.yml`

### Toolchain-sensitive changes

If a change affects portability, integer widths, warnings, or compile-time macros in `/home/runner/work/nanopb/nanopb/pb.h`, copy the relevant matrix from `/home/runner/work/nanopb/nanopb/.github/workflows/compiler_tests.yml` instead of relying on one compiler.

## Non-obvious harness behavior

- `/home/runner/work/nanopb/nanopb/tests/SConstruct` adds strict warning flags and often treats warnings as errors
- If valgrind is available, the harness may use it unless `NOVALGRIND=1` is set
- Some embedded/simulator coverage is available through `PLATFORM=AVR`, `PLATFORM=MIPS`, `PLATFORM=MIPSEL`, `PLATFORM=RISCV64`, and `PLATFORM=STM32`
- The harness may switch to a compatibility system header path when standard C headers are unavailable

There is no separate repo-wide lint or type-check target to run first; the practical local checks are compiler warnings-as-errors plus whatever valgrind-backed coverage the SCons harness enables.

## Coverage and focused runs

- `/home/runner/work/nanopb/nanopb/tests/Makefile` provides `make coverage`
- For narrow investigation, start from the closest test directory under `/home/runner/work/nanopb/nanopb/tests/` and its `SConscript`, but prefer the full suite before finalizing a behavior change
