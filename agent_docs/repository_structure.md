# Repository structure

## Top-level map

- `/home/runner/work/nanopb/nanopb/pb*.c` and `/home/runner/work/nanopb/nanopb/pb*.h` — handwritten runtime sources and public headers
- `/home/runner/work/nanopb/nanopb/generator/` — Python generator, validator generator, plugin wrappers, and generator-owned proto files
- `/home/runner/work/nanopb/nanopb/tests/` — the main executable spec; every subdirectory is usually one focused scenario or regression
- `/home/runner/work/nanopb/nanopb/examples/` — consumer-facing usage examples and integration samples
- `/home/runner/work/nanopb/nanopb/extra/` — reusable integration assets for downstream build systems
- `/home/runner/work/nanopb/nanopb/build-tests/` — CI-oriented packaging and integration test fixtures
- `/home/runner/work/nanopb/nanopb/docs/` — user docs and reference material
- `/home/runner/work/nanopb/nanopb/.github/workflows/` — the authoritative CI matrix for supported environments

## Examples worth knowing

- `/home/runner/work/nanopb/nanopb/examples/simple/` — smallest end-to-end encode/decode example
- `/home/runner/work/nanopb/nanopb/examples/validation_simple/` — validation-enabled example
- `/home/runner/work/nanopb/nanopb/examples/cmake_simple/`, `/home/runner/work/nanopb/nanopb/examples/meson_simple/`, and `/home/runner/work/nanopb/nanopb/examples/conan_dependency/` — downstream integration references
- `/home/runner/work/nanopb/nanopb/examples/platformio/` — PlatformIO packaging and generator integration

## Tests worth knowing

- `/home/runner/work/nanopb/nanopb/tests/common/` — shared harness pieces
- `/home/runner/work/nanopb/nanopb/tests/regression/` — issue-driven regressions; check here before changing behavior
- `/home/runner/work/nanopb/nanopb/tests/validation/` — validation feature coverage
- `/home/runner/work/nanopb/nanopb/tests/site_scons/` — SCons platform adapters and generator integration for the test harness

## Build/integration files worth knowing

- `/home/runner/work/nanopb/nanopb/CMakeLists.txt`
- `/home/runner/work/nanopb/nanopb/meson.build`
- `/home/runner/work/nanopb/nanopb/BUILD.bazel`
- `/home/runner/work/nanopb/nanopb/Package.swift`
- `/home/runner/work/nanopb/nanopb/conanfile.py`
- `/home/runner/work/nanopb/nanopb/extra/nanopb.mk`

## Docs worth knowing

- `/home/runner/work/nanopb/nanopb/docs/index.md` — overview
- `/home/runner/work/nanopb/nanopb/docs/concepts.md` — type mapping and stream model
- `/home/runner/work/nanopb/nanopb/docs/reference.md` — API and generator options
- `/home/runner/work/nanopb/nanopb/docs/validation.md` — validation feature behavior
