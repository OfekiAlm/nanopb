# CLAUDE.md

This file orients AI coding agents to the nanopb codebase. For API details and end-user usage, prefer the authoritative docs under `/home/runner/work/nanopb/nanopb/docs/`.

## Why this project exists

Nanopb is a small Protocol Buffers implementation for memory-constrained systems, especially microcontrollers. The project keeps the runtime in portable C and moves schema interpretation into a generator, so most protobuf complexity is handled ahead of time instead of at runtime.

Two architectural choices matter for most tasks:

- The runtime is intentionally small and split by responsibility: common support in `/home/runner/work/nanopb/nanopb/pb_common.c`, encoding in `/home/runner/work/nanopb/nanopb/pb_encode.c`, decoding in `/home/runner/work/nanopb/nanopb/pb_decode.c`, and optional validation in `/home/runner/work/nanopb/nanopb/pb_validate.c`.
- `.proto` files are turned into C structs and descriptor tables by the Python generator in `/home/runner/work/nanopb/nanopb/generator/nanopb_generator.py`; the generated code is the bridge between protobuf schemas and the C runtime.

## What is here

### Languages and tooling

- Runtime: ANSI C in `/home/runner/work/nanopb/nanopb/pb*.c` and `/home/runner/work/nanopb/nanopb/pb*.h`
- Generator: Python 3 in `/home/runner/work/nanopb/nanopb/generator/`
- Main test runner: SCons from `/home/runner/work/nanopb/nanopb/tests/SConstruct`
- Supported integration/build systems: CMake, Meson, Bazel, Conan, Make, SwiftPM, PlatformIO
- Core Python deps: `protobuf`, `grpcio-tools` (`/home/runner/work/nanopb/nanopb/requirements.txt`)

### High-level layout

- `/home/runner/work/nanopb/nanopb/pb.h` — compile-time feature flags and common types
- `/home/runner/work/nanopb/nanopb/pb_encode.*` / `/home/runner/work/nanopb/nanopb/pb_decode.*` — public runtime APIs and implementations
- `/home/runner/work/nanopb/nanopb/pb_validate.*` — optional declarative validation runtime
- `/home/runner/work/nanopb/nanopb/generator/` — schema-to-C generator, validator generator, packaged entrypoints
- `/home/runner/work/nanopb/nanopb/generator/proto/` — generator-owned protobuf definitions such as `nanopb.proto` and `validate.proto`
- `/home/runner/work/nanopb/nanopb/tests/` — canonical behavior and regression coverage
- `/home/runner/work/nanopb/nanopb/examples/` — minimal consumer-facing examples for major integration styles
- `/home/runner/work/nanopb/nanopb/extra/` — reusable integration files (`nanopb.mk`, CMake helpers, Bazel support)
- `/home/runner/work/nanopb/nanopb/build-tests/` — CI-only integration checks for packaging/build systems
- `/home/runner/work/nanopb/nanopb/docs/` — authoritative user and API documentation

### Where to look first

- Runtime bug or feature: start with `/home/runner/work/nanopb/nanopb/pb.h` plus the relevant `/home/runner/work/nanopb/nanopb/pb_*.c`
- Generator bug or generated-code shape issue: `/home/runner/work/nanopb/nanopb/generator/nanopb_generator.py` and the nearest case in `/home/runner/work/nanopb/nanopb/tests/`
- Validation work: `/home/runner/work/nanopb/nanopb/pb_validate.*`, `/home/runner/work/nanopb/nanopb/generator/nanopb_validator.py`, `/home/runner/work/nanopb/nanopb/tests/validation/`
- Build-system integration: matching files in `/home/runner/work/nanopb/nanopb/CMakeLists.txt`, `/home/runner/work/nanopb/nanopb/meson.build`, `/home/runner/work/nanopb/nanopb/BUILD.bazel`, `/home/runner/work/nanopb/nanopb/Package.swift`, or `/home/runner/work/nanopb/nanopb/extra/`

## How to work in this repo

### Setup and code generation

- Install the baseline generator deps: `python3 -m pip install protobuf grpcio-tools`
- Install `protoc`; both the generator and several build systems expect it to be available
- Generate C from schemas with `python3 /home/runner/work/nanopb/nanopb/generator/nanopb_generator.py your_file.proto`

### Build and run

- Canonical library/example build for local development: `cmake -S /home/runner/work/nanopb/nanopb -B /home/runner/work/nanopb/nanopb/build && cmake --build /home/runner/work/nanopb/nanopb/build`
- Canonical example-first sanity check: `cd /home/runner/work/nanopb/nanopb/examples/simple && make && ./simple`
- Meson, Bazel, PlatformIO, SwiftPM, and Conan support are real and CI-covered, but only exercise the one you changed

### Tests and verification

- The default verification path is `cd /home/runner/work/nanopb/nanopb/tests && scons`
- Use that test suite for almost any runtime or generator change; it is the main CI smoke test and the densest source of expected behavior
- There is no single repo-wide lint or typecheck command; local quality checks come mainly from warning-as-error compiler builds in SCons and optional valgrind coverage in the same harness
- If you touch integration-specific files, also run the matching workflow command:
  - CMake: `cmake -S ... -B ... && cmake --build ...`
  - Meson: `meson setup build -Dexamples=enabled && ninja -C build`
  - Bazel: `bazelisk test --//:nanopb_extension=.pb //...`
  - SwiftPM: `swift build && swift test`
  - PlatformIO: use `/home/runner/work/nanopb/nanopb/.github/workflows/platformio_tests.yml` as the source of truth
- On macOS, prefer `scons CC=clang CXX=clang++`; `/home/runner/work/nanopb/nanopb/tests/SConstruct` does this automatically when `CC` is unset

### Non-obvious workflow details

- The runtime is heavily configuration-driven via macros in `/home/runner/work/nanopb/nanopb/pb.h`; changes there can affect many tests and integrations
- The test harness adds strict compiler flags and optional valgrind checks in `/home/runner/work/nanopb/nanopb/tests/SConstruct`, so failures may be toolchain-specific rather than functional
- Validation code generation requires both generator and runtime changes to stay in sync
- Prefer existing examples and regression tests over inventing new ad hoc verification flows

## Read more only when needed

- `/home/runner/work/nanopb/nanopb/agent_docs/architecture.md` — runtime/generator split, validation path, and key design tradeoffs
- `/home/runner/work/nanopb/nanopb/agent_docs/building.md` — concise build/setup commands across supported build systems
- `/home/runner/work/nanopb/nanopb/agent_docs/testing.md` — which tests to run for which kinds of changes
- `/home/runner/work/nanopb/nanopb/agent_docs/repository_structure.md` — where major directories and examples fit
- `/home/runner/work/nanopb/nanopb/docs/reference.md` — public API details
- `/home/runner/work/nanopb/nanopb/docs/concepts.md` and `/home/runner/work/nanopb/nanopb/docs/validation.md` — protocol mapping and validation behavior
