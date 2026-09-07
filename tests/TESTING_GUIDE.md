# Nanopb Testing Infrastructure Guide

This guide provides a comprehensive understanding of the nanopb test infrastructure from the ground up, including how tests work, the role of `site_scons`, and how to extend and enhance the test suite.

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [SCons Build System](#scons-build-system)
4. [The site_scons Directory](#the-site_scons-directory)
5. [How Tests Work](#how-tests-work)
6. [Writing New Tests](#writing-new-tests)
7. [Platform Support](#platform-support)
8. [Advanced Topics](#advanced-topics)

---

## Overview

The nanopb test infrastructure is built on top of **SCons**, a Python-based build system. The tests validate the nanopb protocol buffer implementation by:

1. Encoding messages using nanopb
2. Decoding messages using nanopb  
3. Cross-validating with Google's protoc compiler
4. Running on multiple platforms (native, embedded simulators)

### Key Components

- **SConstruct**: Main build configuration file (root of build system)
- **site_scons/**: Custom SCons extensions specific to nanopb
- **SConscript**: Per-test build files (one in each test directory)
- **common/**: Shared test resources and core library builds
- **Test directories**: Individual test cases (e.g., `basic_buffer`, `alltypes`)

---

## Architecture

### Build Flow

```
User runs: scons
    ↓
SConstruct (main config)
    ↓
site_scons/site_init.py (custom builders + platform configs)
    ↓
site_scons/site_tools/nanopb.py (protoc integration)
    ↓
common/SConscript (build pb_encode.o, pb_decode.o, etc.)
    ↓
Each test's SConscript (build + run test programs)
    ↓
Test results (pass/fail)
```

### Directory Structure

```
tests/
├── SConstruct               # Main build file - entry point
├── Makefile                 # Simple wrapper around scons
├── site_scons/              # Custom SCons extensions
│   ├── site_init.py         # Custom builders and platform registry
│   ├── site_tools/          # SCons tool plugins
│   │   └── nanopb.py        # NanopbProto builder for .proto files
│   └── platforms/           # Platform-specific configs
│       ├── __init__.py
│       ├── avr/             # AVR microcontroller support
│       ├── stm32/           # ARM Cortex-M support
│       ├── mips/            # MIPS support
│       ├── mipsel/          # MIPS little-endian
│       └── riscv64/         # RISC-V 64-bit support
├── common/                  # Shared resources
│   ├── SConscript          # Builds pb_encode.o, pb_decode.o
│   ├── person.proto        # Common test message
│   └── unittestproto.proto # For unit tests
├── basic_buffer/            # Example test case
│   ├── SConscript          # Test-specific build
│   ├── encode_buffer.c     # Encoder test
│   └── decode_buffer.c     # Decoder test
├── alltypes/                # Another test case
│   ├── SConscript
│   ├── alltypes.proto      # Message definitions
│   ├── alltypes.options    # Nanopb-specific options
│   ├── encode_alltypes.c
│   └── decode_alltypes.c
└── ... (70+ test directories)
```

---

## SCons Build System

### What is SCons?

SCons is a software construction tool (build system) written in Python:

- **Declarative**: You describe what to build, not how
- **Dependency tracking**: Automatically rebuilds only what changed
- **Cross-platform**: Works on Windows, Linux, macOS
- **Extensible**: Easy to add custom builders and tools

### Basic SCons Concepts

#### Environment

An `Environment` is a context for building:

```python
env = Environment()           # Create environment
env.Append(CFLAGS = '-g')     # Add compiler flags
env.Program('test', 'test.c') # Build a program
```

#### Builders

Builders are commands that create targets from sources:

```python
# Built-in builders
env.Program(target, source)   # Compile and link C program
env.Object(target, source)    # Compile to object file
env.Library(target, source)   # Build static library

# Custom builders (defined in site_scons)
env.NanopbProto('message')    # Generate .pb.c/.pb.h from .proto
env.RunTest(program)          # Run test and capture output
env.Decode(target, source)    # Decode with protoc
```

#### Nodes

SCons uses nodes to represent files and dependencies:

```python
# File nodes are automatically tracked
source = File('test.c')
program = env.Program('test', source)
# If test.c changes, 'test' will be rebuilt
```

---

## The site_scons Directory

The `site_scons/` directory is where SCons looks for custom extensions. This is the **heart** of the nanopb test system.

### site_scons/site_init.py

This file is **automatically imported** by SCons before processing SConstruct. It provides:

#### 1. Platform Registry

```python
platforms = {
    'STM32': set_stm32_platform,
    'AVR': set_avr_platform,
    'MIPS': set_mips_platform,
    'MIPSEL': set_mipsel_platform,
    'RISCV64': set_riscv64_platform,
}
```

Usage:
```bash
# Build for STM32 platform
scons PLATFORM=STM32
```

#### 2. Custom Builders

The `add_nanopb_builders(env)` function adds several custom builders:

##### RunTest Builder

Runs a test program and captures output:

```python
def run_test(target, source, env):
    # Runs source[0] (executable)
    # Optionally reads from source[1] (input file)
    # Writes stdout to target[0] (output file)
    # Returns exit code (0 = success)
```

Usage in SConscript:
```python
enc = env.Program('encode', 'encode.c')
env.RunTest(enc)  # Creates encode.output
```

##### Decode Builder

Decodes a binary protobuf file using protoc:

```python
env.Decode(target, [binary_file, proto_file], MESSAGE='MessageName')
# Runs: protoc --decode=MessageName proto_file < binary_file > target
```

##### Encode Builder

Encodes a text protobuf file using protoc:

```python
env.Encode(target, [text_file, proto_file], MESSAGE='MessageName')
# Runs: protoc --encode=MessageName proto_file < text_file > target
```

##### Compare Builder

Asserts that two files are byte-for-byte identical:

```python
env.Compare([file1, file2])
# Succeeds if files are identical, fails otherwise
```

##### Match Builder

Checks that patterns exist (or don't exist) in a file:

```python
# patterns.txt contains regular expressions, one per line
# Lines starting with "! " are inverted (must NOT match)
env.Match([output_file, patterns_file])
```

### site_scons/site_tools/nanopb.py

This is an SCons **tool** that provides the `NanopbProto` builder.

#### What Does It Do?

Automatically generates C code from `.proto` files:

```python
env.NanopbProto('message')
# Converts message.proto → message.pb.c + message.pb.h
```

#### How It Works

1. **Detects nanopb location**: Looks for `pb.h` in parent directories
2. **Detects protoc**: Finds protoc compiler (bundled or system)
3. **Detects Python**: Finds Python for running generator scripts
4. **Configures flags**: Sets up protoc plugin path
5. **Registers builder**: Adds `NanopbProto` to environment

#### Key Functions

```python
# Tool setup
def generate(env):
    env['NANOPB'] = _detect_nanopb(env)
    env['PROTOC'] = _detect_protoc(env)
    env['PROTOCFLAGS'] = _detect_protocflags(env)
    env['BUILDERS']['NanopbProto'] = _nanopb_proto_builder

# Build action
def _nanopb_proto_actions(source, target, env, for_signature):
    # Generates the protoc command line:
    # $PROTOC --nanopb_out=FLAGS:. source.proto
    
# Emitter (determines output files and dependencies)
def _nanopb_proto_emitter(target, source, env):
    # For input.proto, generates:
    # - input.pb.c
    # - input.pb.h
    # Also adds input.options as dependency if it exists
```

#### Options Files

Files named `*.options` are automatically detected and control code generation:

```
# alltypes.options
int32_t         max_size:5
string_t        max_size:40
repeated_field  max_count:10
```

### site_scons/platforms/

Each platform subdirectory contains a module that configures the build environment for cross-compilation or simulation.

#### Structure

```
platforms/
├── __init__.py              # Empty (makes it a package)
├── avr/
│   ├── __init__.py         # Empty
│   ├── avr.py              # Platform configuration
│   ├── avr_io.c            # I/O helpers for simulator
│   └── run_test.c          # Test runner using simavr
├── stm32/
│   ├── __init__.py
│   ├── stm32.py            # Platform configuration
│   ├── run_test.sh         # Test runner script
│   ├── vectors.c           # Interrupt vectors
│   └── stm32_ram.ld        # Linker script
└── ... (other platforms)
```

#### Example: AVR Platform

From `platforms/avr/avr.py`:

```python
def set_avr_platform(env):
    # 1. Build native test runner (uses simavr)
    native = env.Clone()
    runner = native.Program("build/run_test", "site_scons/platforms/avr/run_test.c")
    
    # 2. Configure for AVR cross-compilation
    env.Replace(CC = "avr-gcc", CXX = "avr-g++")
    env.Append(CFLAGS = "-mmcu=atmega1284 -Os")
    env.Replace(TEST_RUNNER = "build/run_test")
    
    # 3. Build I/O helper library
    avr_io = env.Object("build/avr_io.o", "site_scons/platforms/avr/avr_io.c")
    env.Append(LIBS = avr_io)
```

What this does:
1. Builds a **native** (x86) program that runs simavr
2. Configures environment to **cross-compile** tests for AVR
3. The test runner loads AVR binaries into simavr and executes them

---

## How Tests Work

### Anatomy of a Test Case

Let's dissect the `basic_buffer` test:

#### Files

```
basic_buffer/
├── SConscript          # Build configuration
├── encode_buffer.c     # Encodes Person message
└── decode_buffer.c     # Decodes Person message
```

#### SConscript

```python
Import("env")

# Build encoder: links encode_buffer.c with person.pb.c and nanopb core
enc = env.Program([
    "encode_buffer.c",
    "$COMMON/person.pb.c",      # Generated from person.proto
    "$COMMON/pb_encode.o",      # Nanopb encoder
    "$COMMON/pb_common.o"       # Nanopb common utilities
])

# Build decoder
dec = env.Program([
    "decode_buffer.c",
    "$COMMON/person.pb.c",
    "$COMMON/pb_decode.o",
    "$COMMON/pb_common.o"
])

# Run encoder (output goes to encode_buffer.output)
env.RunTest(enc)

# Run decoder with encoder output as input
env.RunTest([dec, "encode_buffer.output"])

# Decode the binary output with protoc for verification
env.Decode(["encode_buffer.output", "$COMMON/person.proto"],
           MESSAGE = "Person")

# Compare nanopb decoder output with protoc output
env.Compare(["decode_buffer.output", "encode_buffer.decoded"])
```

#### Test Flow

```
1. encode_buffer.c runs
   → Writes binary protobuf to encode_buffer.output

2. decode_buffer.c runs with encode_buffer.output as input
   → Writes decoded text to decode_buffer.output

3. protoc --decode runs on encode_buffer.output
   → Writes decoded text to encode_buffer.decoded

4. Compare decode_buffer.output vs encode_buffer.decoded
   → Verifies nanopb produces same result as protoc
```

### More Complex Example: alltypes

The `alltypes` test is more comprehensive:

```python
# Generate C code from proto
env.NanopbProto(["alltypes", "alltypes.options"])

# Build programs
enc = env.Program(["encode_alltypes.c", "alltypes.pb.c", ...])
dec = env.Program(["decode_alltypes.c", "alltypes.pb.c", ...])

# Test with no optional fields
env.RunTest(enc)
env.RunTest([dec, "encode_alltypes.output"])
env.Decode("encode_alltypes.output.decoded", [...], MESSAGE='AllTypes')
env.Encode("encode_alltypes.output.recoded", [...], MESSAGE='AllTypes')
env.Compare(["encode_alltypes.output", "encode_alltypes.output.recoded"])

# Test with optional fields present (ARGS = ['1'])
env.RunTest("optionals.output", enc, ARGS = ['1'])
env.RunTest("optionals.decout", [dec, "optionals.output"], ARGS = ['1'])
# ... more comparisons

# Test zero initializer (ARGS = ['2'])
env.RunTest("zeroinit.output", enc, ARGS = ['2'])
# ... more tests
```

This tests multiple scenarios with the same programs by passing different arguments.

---

## Writing New Tests

### Step-by-Step Guide

#### 1. Create Test Directory

```bash
cd tests
mkdir my_new_test
cd my_new_test
```

#### 2. Create Protocol Definition

`my_message.proto`:
```protobuf
syntax = "proto2";

message MyMessage {
    required int32 value = 1;
    optional string name = 2;
}
```

Optional: `my_message.options`:
```
MyMessage.name  max_size:50
```

#### 3. Write Test Code

`encode_test.c`:
```c
#include <stdio.h>
#include <pb_encode.h>
#include "my_message.pb.h"
#include "test_helpers.h"

int main() {
    uint8_t buffer[MyMessage_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    MyMessage msg = {42, true, "test"};
    
    if (pb_encode(&stream, MyMessage_fields, &msg)) {
        SET_BINARY_MODE(stdout);
        fwrite(buffer, 1, stream.bytes_written, stdout);
        return 0;
    }
    return 1;
}
```

`decode_test.c`:
```c
#include <stdio.h>
#include <pb_decode.h>
#include "my_message.pb.h"
#include "test_helpers.h"

int main() {
    uint8_t buffer[MyMessage_size];
    pb_istream_t stream;
    MyMessage msg = MyMessage_init_zero;
    
    SET_BINARY_MODE(stdin);
    size_t count = fread(buffer, 1, sizeof(buffer), stdin);
    stream = pb_istream_from_buffer(buffer, count);
    
    if (pb_decode(&stream, MyMessage_fields, &msg)) {
        printf("value: %d\n", msg.value);
        if (msg.has_name) printf("name: %s\n", msg.name);
        return 0;
    }
    return 1;
}
```

#### 4. Create SConscript

`SConscript`:
```python
Import("env")

# Generate C code from proto
env.NanopbProto(["my_message", "my_message.options"])

# Build encoder
enc = env.Program([
    "encode_test.c",
    "my_message.pb.c",
    "$COMMON/pb_encode.o",
    "$COMMON/pb_common.o"
])

# Build decoder  
dec = env.Program([
    "decode_test.c",
    "my_message.pb.c",
    "$COMMON/pb_decode.o",
    "$COMMON/pb_common.o"
])

# Run tests
env.RunTest(enc)
env.RunTest([dec, "encode_test.output"])

# Verify with protoc
env.Decode(["encode_test.output", "my_message.proto"], MESSAGE="MyMessage")
env.Compare(["decode_test.output", "encode_test.decoded"])
```

#### 5. Run Test

```bash
cd tests
scons my_new_test
```

### Common Patterns

#### Testing Optional Fields

```python
# Test with optional fields absent
env.RunTest("no_optional.output", enc, ARGS=['0'])
env.RunTest("no_optional.decout", [dec, "no_optional.output"], ARGS=['0'])

# Test with optional fields present  
env.RunTest("with_optional.output", enc, ARGS=['1'])
env.RunTest("with_optional.decout", [dec, "with_optional.output"], ARGS=['1'])
```

#### Testing Error Conditions

```python
# Create a deliberately malformed input
env.Command("bad_input.bin", [], "echo 'invalid' > $TARGET")

# Run decoder - expect it to fail (return non-zero)
# Note: You'll need to handle this specially or use pattern matching
```

#### Pattern Matching

Create `expected_patterns.txt`:
```
value: 42
name: test
! ERROR
! failed
```

Then:
```python
env.Match([output_file, "expected_patterns.txt"])
```

#### Using Valgrind

Valgrind is automatically used if available and not disabled:

```bash
# Disable valgrind for faster iteration
scons NOVALGRIND=1
```

---

## Platform Support

### Running on Different Platforms

#### Native (default)

```bash
scons
```

#### AVR Microcontroller

```bash
scons PLATFORM=AVR
```

Requirements:
- avr-gcc
- simavr library
- libelf

#### STM32 (ARM Cortex-M)

```bash
scons PLATFORM=STM32  
```

Requirements:
- arm-none-eabi-gcc
- OpenOCD or st-link tools
- Hardware: STM32 Discovery board

#### MIPS/MIPSEL

```bash
scons PLATFORM=MIPS
```

Requirements:
- mips-linux-gnu-gcc
- qemu-mips

### Adding a New Platform

1. Create directory: `site_scons/platforms/myplatform/`

2. Create `__init__.py`: (empty file)

3. Create `myplatform.py`:

```python
def set_myplatform_platform(env):
    # Set cross-compilation tools
    env.Replace(CC = "myplatform-gcc")
    env.Replace(CXX = "myplatform-g++")
    
    # Set compiler flags
    env.Append(CFLAGS = "-march=myarch")
    
    # Set test runner (if needed)
    env.Replace(TEST_RUNNER = "qemu-myplatform")
    
    # Mark as embedded (disables some tests)
    env.Replace(EMBEDDED = "MYPLATFORM")
    
    # Add platform-specific defines
    env.Append(CPPDEFINES = {
        'MY_PLATFORM': 1,
        'BUFFER_SIZE': 1024
    })
```

4. Register in `site_scons/site_init.py`:

```python
from platforms.myplatform.myplatform import set_myplatform_platform

platforms = {
    # ... existing platforms ...
    'MYPLATFORM': set_myplatform_platform,
}
```

5. Test:

```bash
scons PLATFORM=MYPLATFORM
```

---

## Advanced Topics

### Build Variants

SCons uses "variant directories" to keep build files separate from source:

```python
env['VARIANT_DIR'] = 'build'
SConscript('test/SConscript', variant_dir='build/test')
```

This means:
- Source: `tests/basic_buffer/encode.c`
- Object: `tests/build/basic_buffer/encode.o`
- Executable: `tests/build/basic_buffer/encode`

### Dependency Tracking

SCons automatically tracks dependencies:

```python
# If person.proto changes, person.pb.c is regenerated
env.NanopbProto("person")

# If person.pb.c changes, encode is rebuilt
enc = env.Program(["encode.c", "person.pb.c", ...])

# If encode changes, test is re-run
env.RunTest(enc)
```

Manual dependencies:
```python
env.Depends(target, source)
```

### Cleaning

```bash
scons -c              # Clean all build artifacts
scons -c basic_buffer # Clean specific test
```

### Parallel Builds

```bash
scons -j4  # Use 4 parallel jobs
```

### Debugging SCons

```bash
scons --debug=tree    # Show dependency tree
scons --debug=explain # Show why targets are rebuilt
scons -n              # Dry run (don't actually build)
```

### Custom Compiler Flags

```bash
scons CCFLAGS="-O3 -march=native"
scons CXXFLAGS="-std=c++14"
```

### Coverage Reports

```bash
make coverage
# Generates HTML coverage report in build/coverage/
```

This uses lcov to collect coverage data from gcov.

### Environment Variables

Key variables set by SConstruct:

- `$NANOPB`: Path to nanopb root
- `$COMMON`: Path to common build directory
- `$BUILD`: Path to build root
- `$PROTOC`: Path to protoc compiler
- `$PROTOC_VERSION`: Detected protoc version
- `$CORECFLAGS`: Extra strict flags for core library

### Conditional Compilation

```python
if env.get('EMBEDDED'):
    # Skip tests that need filesystem
    pass
else:
    env.RunTest(program)
```

### Multiple Test Variants

```python
# Test different buffer sizes
for size in [128, 256, 512]:
    variant = env.Clone()
    variant.Append(CPPDEFINES = {'BUFFER_SIZE': size})
    prog = variant.Program('test_%d' % size, 'test.c')
    variant.RunTest(prog)
```

---

## Quick Reference

### Common SCons Commands

```bash
scons                      # Build and run all tests
scons basic_buffer         # Run specific test
scons -c                   # Clean build
scons -j4                  # Parallel build (4 jobs)
scons --help               # Show custom options
scons PLATFORM=AVR         # Cross-compile for AVR
scons NOVALGRIND=1         # Disable valgrind
scons CC=clang CXX=clang++ # Use different compiler
```

### SConscript Template

```python
Import("env")

# Generate proto files
env.NanopbProto(["myproto", "myproto.options"])

# Build programs
enc = env.Program(["encode.c", "myproto.pb.c",
                   "$COMMON/pb_encode.o", "$COMMON/pb_common.o"])
dec = env.Program(["decode.c", "myproto.pb.c",
                   "$COMMON/pb_decode.o", "$COMMON/pb_common.o"])

# Run tests
env.RunTest(enc)
env.RunTest([dec, "encode.output"])

# Verify
env.Decode(["encode.output", "myproto.proto"], MESSAGE="MyMessage")
env.Compare(["decode.output", "encode.decoded"])
```

### Key Paths

- `$COMMON/pb_encode.o`: Nanopb encoder
- `$COMMON/pb_decode.o`: Nanopb decoder  
- `$COMMON/pb_common.o`: Nanopb common code
- `$NANOPB`: Nanopb root directory
- `#../`: Repository root (# means top-level)

### Custom Builders Reference

| Builder | Purpose | Example |
|---------|---------|---------|
| `NanopbProto` | Generate C from .proto | `env.NanopbProto("msg")` |
| `RunTest` | Execute program | `env.RunTest(program)` |
| `Decode` | Decode with protoc | `env.Decode([bin, proto], MESSAGE="Msg")` |
| `Encode` | Encode with protoc | `env.Encode([txt, proto], MESSAGE="Msg")` |
| `Compare` | Compare two files | `env.Compare([file1, file2])` |
| `Match` | Pattern matching | `env.Match([output, patterns])` |

---

## Conclusion

You now have a comprehensive understanding of the nanopb test infrastructure:

1. **SCons**: The Python-based build system
2. **site_scons/**: Custom extensions and platform support
3. **site_init.py**: Custom builders (RunTest, Decode, etc.)
4. **site_tools/nanopb.py**: Proto file code generation
5. **platforms/**: Cross-compilation configurations
6. **Test structure**: Proto → Generate → Compile → Run → Verify

To extend the test suite:
1. Create a new directory under `tests/`
2. Add `.proto`, `.options`, and `.c` files
3. Write a `SConscript` that uses the custom builders
4. Run with `scons your_test_name`

The test system is designed to be:
- **Incremental**: Only rebuilds what changed
- **Cross-platform**: Supports native and embedded targets
- **Comprehensive**: Tests encoding, decoding, and compatibility
- **Extensible**: Easy to add new tests and platforms

Happy testing!
