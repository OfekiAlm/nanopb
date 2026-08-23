# Building

## Baseline prerequisites

For most local work, install:

- `protoc`
- Python 3 packages from `/home/runner/work/nanopb/nanopb/requirements.txt`: `python3 -m pip install protobuf grpcio-tools`
- `scons` for the main test harness

Extra tools are only needed for the integration you are touching: `cmake`, `meson` + `ninja`, `bazelisk`, `swift`, or PlatformIO.

## Common local commands

### Generate code from a schema

```bash
python3 /home/runner/work/nanopb/nanopb/generator/nanopb_generator.py path/to/file.proto
```

Enable validation generation when needed with the same flags used by `/home/runner/work/nanopb/nanopb/examples/validation_simple/Makefile` and `/home/runner/work/nanopb/nanopb/tests/validation/SConscript`.

### CMake

```bash
cmake -S /home/runner/work/nanopb/nanopb -B /home/runner/work/nanopb/nanopb/build
cmake --build /home/runner/work/nanopb/nanopb/build
```

`/home/runner/work/nanopb/nanopb/CMakeLists.txt` requires `protoc` and can also install the Python generator package.

### Meson

```bash
meson setup build -Dexamples=enabled
ninja -C build
```

This is the same shape used in `/home/runner/work/nanopb/nanopb/.github/workflows/meson.yml`.

### Bazel

```bash
bazelisk build //...
bazelisk test --//:nanopb_extension=.pb //...
```

Use the alternate `.nanopb` extension mode only when working on Bazel-specific generation behavior.

### SwiftPM

```bash
swift build
swift test
```

Relevant when changing `/home/runner/work/nanopb/nanopb/Package.swift`, `/home/runner/work/nanopb/nanopb/spm_headers/`, or `/home/runner/work/nanopb/nanopb/spm-test/`.

## Example-oriented sanity checks

- `/home/runner/work/nanopb/nanopb/examples/simple/` is the fastest way to understand the normal generator + runtime loop
- `/home/runner/work/nanopb/nanopb/examples/validation_simple/` is the fastest way to understand validation-enabled generation
- `/home/runner/work/nanopb/nanopb/build-tests/` contains packaging/integration checks that mirror CI more than day-to-day development

## Environment notes

- `/home/runner/work/nanopb/nanopb/tests/SConstruct` prefers clang on macOS when `CC` is not set
- `/home/runner/work/nanopb/nanopb/CMakeLists.txt` errors out immediately if `protoc` is missing
- The development container under `/home/runner/work/nanopb/nanopb/.devcontainer/` already includes the common compiler, Python, protobuf, and debugging tools
