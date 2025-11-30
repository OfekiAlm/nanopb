# Nanopb Test Suite

This directory contains the test suite for nanopb. Tests are orchestrated using pytest, while SCons handles the actual compilation of C code and generation of Protocol Buffer files.

## Quick Start

### Prerequisites

1. Install Python dependencies:
   ```bash
   pip install pytest protobuf grpcio-tools scons
   ```

2. Make sure you have a C compiler (gcc or clang) installed.

### Running Tests

**Run all tests:**
```bash
# From the repository root
pytest tests/

# Or from the tests directory
cd tests && pytest
```

**Run a specific test file:**
```bash
pytest tests/test_encoding_decoding.py
```

**Run a specific test:**
```bash
pytest tests/test_encoding_decoding.py::TestAllTypesBasic::test_encode_alltypes_runs_successfully
```

**Run tests by marker:**
```bash
# Only encoding tests
pytest tests/ -m encoding

# Only decoding tests
pytest tests/ -m decoding

# Only generator tests
pytest tests/ -m generator

# Only regression tests
pytest tests/ -m regression

# Skip slow tests
pytest tests/ -m "not slow"
```

## Test Structure

```
tests/
├── conftest.py              # Shared fixtures and configuration
├── test_encoding_decoding.py    # Basic encode/decode tests
├── test_generator_options.py    # Code generator tests
├── test_unittests.py           # Unit tests for encoder/decoder
├── test_special_types.py       # Oneof, extensions, maps, etc.
├── test_regression.py          # Regression tests for specific issues
├── test_compiler_features.py   # C++ and compiler-specific tests
├── SConstruct               # SCons build configuration
├── common/                  # Shared test utilities
├── site_scons/              # SCons tools and platform support
└── [test_dirs]/             # Individual test cases
```

## How It Works

### Build System

The tests use a two-layer approach:

1. **SCons** - Handles all C compilation and protobuf generation:
   - Compiles `.proto` files to `.pb.c` and `.pb.h`
   - Builds test binaries from C source files
   - Links against nanopb core libraries

2. **pytest** - Handles test orchestration and assertions:
   - Runs compiled test binaries
   - Verifies exit codes, output, and file contents
   - Reports test results

### Build Directory

All build artifacts are placed in `tests/build/`. This includes:
- Compiled object files (`.o`)
- Test executables
- Generated Protocol Buffer files (`.pb.c`, `.pb.h`)
- Test output files (`.output`, `.decoded`, etc.)

### Fixtures

Key pytest fixtures defined in `conftest.py`:

| Fixture | Description |
|---------|-------------|
| `ensure_build_exists` | Ensures SCons build has been run |
| `binary_runner` | Helper to run compiled test binaries |
| `protobuf_generator` | Helper to generate/encode/decode protobufs |
| `build_dir` | Path to the SCons build directory |
| `tests_dir` | Path to the tests directory |

## Writing New Tests

### Adding a pytest Test for an Existing SCons Test

1. Identify what the SCons test validates (look at the `SConscript` file)
2. Add a test function to the appropriate test module
3. Use the `binary_runner` fixture to run test binaries
4. Use `ensure_build_exists` to ensure binaries are built

Example:
```python
def test_my_feature(self, binary_runner, ensure_build_exists):
    """Test description of what we're validating."""
    result = binary_runner.run_binary("my_test/my_test_binary")
    
    assert result.returncode == 0, f"Test failed: {result.stderr}"
```

### Adding a New Test Case

1. Create a new directory under `tests/` (if needed)
2. Add `.proto` files and C source code
3. Create a `SConscript` file for building
4. Add pytest tests that exercise the built binaries

### Test Markers

Use markers to categorize tests:

```python
@pytest.mark.encoding      # Encoding-related tests
@pytest.mark.decoding      # Decoding-related tests
@pytest.mark.generator     # Code generator tests
@pytest.mark.regression    # Regression tests
@pytest.mark.slow          # Slow tests (fuzz, etc.)
@pytest.mark.integration   # Integration tests
```

## Running SCons Directly

You can still run SCons directly for building or debugging:

```bash
cd tests

# Full build
scons NODEFARGS=1

# Build specific target
scons NODEFARGS=1 build/alltypes

# Clean build
scons -c
```

## Coverage

To generate code coverage reports:

```bash
cd tests
make coverage
```

Coverage output will be in `tests/build/coverage/`.

## Troubleshooting

### Build Errors

If you see build errors, try:
1. Clean the build: `cd tests && rm -rf build`
2. Run with verbose output: `scons -j1 NODEFARGS=1`

### Test Failures

For debugging test failures:
1. Check the test output in `tests/build/[test_name]/`
2. Run the failing binary directly
3. Use `pytest -v --tb=long` for detailed tracebacks

### Missing Binaries

If tests fail because binaries don't exist:
1. Run `scons NODEFARGS=1` in the tests directory
2. Check for build errors in the SCons output
