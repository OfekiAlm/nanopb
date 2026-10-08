# Nanopb Test Suite Documentation

Welcome to the nanopb test suite! This directory contains comprehensive test coverage for the nanopb protocol buffer implementation, along with detailed documentation to help you understand and extend the test infrastructure.

## Quick Links

### For Different Audiences

**I'm new and want to understand how tests work:**
→ Start with [TESTING_GUIDE.md](TESTING_GUIDE.md) - A comprehensive guide from the ground up

**I want to write a new test quickly:**
→ See [WRITING_TESTS_QUICKSTART.md](WRITING_TESTS_QUICKSTART.md) - Get started in minutes

**I need technical details about site_scons:**
→ Read [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md) - Complete technical reference

**I want to understand the architecture:**
→ Check [TEST_ARCHITECTURE_OVERVIEW.md](TEST_ARCHITECTURE_OVERVIEW.md) - Visual diagrams and flow charts

## What's in This Directory?

```
tests/
├── README.md                              ← You are here
├── TESTING_GUIDE.md                       ← Complete guide (start here!)
├── WRITING_TESTS_QUICKSTART.md            ← Quick start for new tests
├── SITE_SCONS_REFERENCE.md                ← Technical reference
├── TEST_ARCHITECTURE_OVERVIEW.md          ← Architecture diagrams
│
├── SConstruct                             ← Main build configuration
├── Makefile                               ← Simple wrapper (runs scons)
│
├── site_scons/                            ← Custom SCons extensions
│   ├── site_init.py                       ← Custom builders
│   ├── site_tools/nanopb.py               ← Proto code generation
│   └── platforms/                         ← Cross-compilation configs
│
├── common/                                ← Shared test resources
│   ├── person.proto                       ← Example message
│   └── test_helpers.h                     ← Utility macros
│
└── 70+ test directories/                  ← Individual test cases
    ├── basic_buffer/                      ← Simple round-trip test
    ├── alltypes/                          ← Comprehensive type test
    ├── callbacks/                         ← Callback functionality
    └── ...
```

## Running Tests

### Prerequisites

```bash
pip3 install scons protobuf grpcio-tools
```

### Basic Usage

```bash
# Run all tests
make
# or
scons

# Run specific test
scons basic_buffer

# Run multiple tests
scons basic_buffer alltypes

# Clean build
scons -c

# Parallel build (4 jobs)
scons -j4

# Different compiler
scons CC=clang CXX=clang++

# Cross-compile for AVR
scons PLATFORM=AVR

# Disable valgrind for faster iteration
scons NOVALGRIND=1
```

## Documentation Guide

### 1. TESTING_GUIDE.md

**Purpose**: Comprehensive guide to understanding the test infrastructure from scratch

**What you'll learn**:
- How SCons works and why we use it
- What `site_scons/` is and why it matters
- How custom builders work (RunTest, Decode, Encode, etc.)
- Platform support for cross-compilation
- How to write tests from scratch
- Advanced topics and troubleshooting

**When to read**: When you want a complete understanding of the system

**Length**: ~600 lines, comprehensive

---

### 2. WRITING_TESTS_QUICKSTART.md

**Purpose**: Get started writing tests immediately with minimal explanation

**What you'll learn**:
- Quick test template (copy-paste ready)
- Common patterns and examples
- Builder reference (quick lookup)
- Troubleshooting common issues

**When to read**: When you need to write a test right now

**Length**: ~300 lines, practical

---

### 3. SITE_SCONS_REFERENCE.md

**Purpose**: Technical deep-dive into the `site_scons/` directory

**What you'll learn**:
- Detailed breakdown of `site_init.py`
- How each custom builder works internally
- Complete analysis of `site_tools/nanopb.py`
- Platform configuration details
- Extension points for customization

**When to read**: When you need to modify or extend the build system

**Length**: ~700 lines, technical

---

### 4. TEST_ARCHITECTURE_OVERVIEW.md

**Purpose**: Visual architectural understanding with diagrams

**What you'll learn**:
- System architecture diagrams
- Component interaction flows
- Data flow through test execution
- Dependency graphs
- File organization

**When to read**: When you're a visual learner or need the big picture

**Length**: ~500 lines, visual

---

## Test Categories

The test suite covers:

### Core Functionality
- **basic_buffer**, **basic_stream**: Round-trip encode/decode
- **alltypes**: All protobuf data types
- **alltypes_callback**: Callback-based fields
- **alltypes_pointer**: Pointer-based fields
- **alltypes_proto3**: Proto3 syntax support

### Advanced Features
- **callbacks**: Field callbacks
- **oneof**: Oneof field handling
- **extensions**: Extension support
- **map**: Map field support
- **anonymous_oneof**: Anonymous oneof handling

### Code Generation
- **options**: Nanopb options handling
- **namingstyle**: Naming convention control
- **multiple_files**: Multiple .proto files
- **comments**: Comment preservation
- **special_characters**: Unicode handling

### Memory Management
- **mem_release**: Memory release callbacks
- **buffer_only**: Buffer-only mode
- **fixed_count**: Fixed-count arrays
- **without_64bit**: 32-bit systems

### Validation and Error Handling
- **decode_unittests**, **encode_unittests**: Unit tests
- **io_errors**: Error condition handling
- **missing_fields**: Required field validation
- **backwards_compatibility**: Version compatibility

### Platform-Specific
- **stackusage**: Stack usage analysis
- **splint**: Static analysis
- **fuzztest**: Fuzzing tests

## Understanding Test Structure

Each test typically follows this pattern:

```
test_directory/
├── SConscript              Build configuration
├── message.proto           Message definition
├── message.options         Nanopb-specific options (optional)
├── encode_test.c          Encoder implementation
└── decode_test.c          Decoder implementation
```

The `SConscript` file orchestrates:

1. **Generate**: `.proto` → `.pb.c` + `.pb.h`
2. **Compile**: Link with nanopb core
3. **Execute**: Run encoder and decoder
4. **Verify**: Cross-check with protoc

## How Site_scons Works

The `site_scons/` directory is **the key** to understanding the test system:

### site_init.py
Provides custom builders:
- `RunTest`: Execute programs and capture output
- `Decode`: Decode binary protobuf with protoc
- `Encode`: Encode text protobuf with protoc
- `Compare`: Assert files are identical
- `Match`: Pattern matching in output

### site_tools/nanopb.py
Provides the `NanopbProto` builder:
- Automatically generates `.pb.c` and `.pb.h` from `.proto`
- Detects protoc and nanopb generator
- Handles `.options` files

### platforms/
Cross-compilation support:
- `avr/`: AVR microcontrollers (simavr)
- `stm32/`: ARM Cortex-M (OpenOCD)
- `mips/`: MIPS architecture (QEMU)
- `mipsel/`: MIPS little-endian
- `riscv64/`: RISC-V 64-bit

## Common Tasks

### Adding a New Test

1. Create directory: `mkdir my_test`
2. Add `.proto` file: `my_test/message.proto`
3. Add test code: `encode_test.c`, `decode_test.c`
4. Create `SConscript`:
   ```python
   Import("env")
   env.NanopbProto("message")
   enc = env.Program(["encode_test.c", "message.pb.c", ...])
   dec = env.Program(["decode_test.c", "message.pb.c", ...])
   env.RunTest(enc)
   env.RunTest([dec, "encode_test.output"])
   ```
5. Run: `scons my_test`

See [WRITING_TESTS_QUICKSTART.md](WRITING_TESTS_QUICKSTART.md) for details.

### Adding a New Platform

1. Create `site_scons/platforms/myplatform/`
2. Add `myplatform.py` with `set_myplatform_platform(env)` function
3. Register in `site_scons/site_init.py`:
   ```python
   platforms['MYPLATFORM'] = set_myplatform_platform
   ```
4. Use: `scons PLATFORM=MYPLATFORM`

See [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md) for details.

### Debugging Test Failures

```bash
# View build output
cat build/my_test/encode_test.output

# View generated code
cat build/my_test/message.pb.c

# Compare files manually
diff build/my_test/output1.txt build/my_test/output2.txt

# Rebuild with verbose output
scons --debug=explain my_test

# Clean and rebuild
scons -c my_test
scons my_test
```

### Coverage Reports

```bash
make coverage
# Generates HTML report in build/coverage/
```

## Understanding SCons

SCons is a Python-based build system used by nanopb tests. Key concepts:

- **Environment**: Build configuration container (`env`)
- **Builders**: Commands that create targets (`Program`, `RunTest`, etc.)
- **Nodes**: Files and their dependencies
- **Variant directories**: Separate source from build outputs

For more on SCons, see [TESTING_GUIDE.md](TESTING_GUIDE.md).

## Test Execution Flow

```
1. User runs: scons my_test

2. SCons reads SConstruct
   - Loads site_scons/site_init.py
   - Loads site_scons/site_tools/nanopb.py
   - Configures environment

3. SCons reads my_test/SConscript
   - Generates .pb.c from .proto
   - Compiles test programs
   - Runs tests
   - Verifies output

4. Results displayed
   - [OK] or [FAIL] for each step
   - Build continues or stops on error
```

## Contributing Tests

When adding tests:

1. **Minimal changes**: Don't modify working code unnecessarily
2. **Follow patterns**: Look at existing tests for examples
3. **Document**: Add comments for complex logic
4. **Verify**: Ensure tests pass on multiple platforms if possible
5. **Clean build**: Test with `scons -c && scons my_test`

## Getting Help

### Where to Look

1. **Quick answer**: Check [WRITING_TESTS_QUICKSTART.md](WRITING_TESTS_QUICKSTART.md)
2. **Detailed explanation**: See [TESTING_GUIDE.md](TESTING_GUIDE.md)
3. **Technical details**: Read [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md)
4. **Architecture**: View [TEST_ARCHITECTURE_OVERVIEW.md](TEST_ARCHITECTURE_OVERVIEW.md)
5. **Example tests**: Browse directories like `basic_buffer/` or `alltypes/`

### Common Questions

**Q: What is site_scons?**  
A: A special directory where SCons looks for custom extensions. Read [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md).

**Q: How do I write a test?**  
A: Follow the template in [WRITING_TESTS_QUICKSTART.md](WRITING_TESTS_QUICKSTART.md).

**Q: How does RunTest work?**  
A: It's a custom builder defined in `site_scons/site_init.py`. See [TESTING_GUIDE.md](TESTING_GUIDE.md).

**Q: Why use SCons instead of Make?**  
A: Better dependency tracking, cross-platform support, and Python extensibility. See [TESTING_GUIDE.md](TESTING_GUIDE.md).

**Q: How do I add a new platform?**  
A: Create a module in `site_scons/platforms/`. See [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md).

## Additional Resources

- **Main nanopb documentation**: `../docs/`
- **Examples**: `../examples/`
- **Generator documentation**: `../generator/README.md`
- **SCons manual**: https://scons.org/doc/production/HTML/scons-user/index.html

## Summary

The nanopb test infrastructure is built on:

1. **SCons**: Python-based build system with dependency tracking
2. **site_scons/**: Custom extensions for protocol buffer testing
3. **Custom builders**: RunTest, Decode, Encode, Compare, Match
4. **Platform support**: Native and cross-compilation (AVR, STM32, MIPS, RISC-V)
5. **70+ tests**: Comprehensive coverage of nanopb functionality

**Start here:**
- New to the system? → [TESTING_GUIDE.md](TESTING_GUIDE.md)
- Need to write a test? → [WRITING_TESTS_QUICKSTART.md](WRITING_TESTS_QUICKSTART.md)
- Modifying the build system? → [SITE_SCONS_REFERENCE.md](SITE_SCONS_REFERENCE.md)
- Want the big picture? → [TEST_ARCHITECTURE_OVERVIEW.md](TEST_ARCHITECTURE_OVERVIEW.md)

Happy testing!
