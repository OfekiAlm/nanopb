# Nanopb: Protocol Buffers for Embedded Systems

![Latest change](https://github.com/nanopb/nanopb/actions/workflows/trigger_on_code_change.yml/badge.svg)
![Weekly build](https://github.com/nanopb/nanopb/actions/workflows/trigger_on_schedule.yml/badge.svg)

## Overview

Nanopb is a **Protocol Buffers tooling ecosystem** designed for resource-constrained embedded systems. This fork extends the original nanopb library beyond basic code generation, providing a complete suite of tools for working with Protocol Buffers in memory-limited environments.

### What This Project Provides

This project offers three core tools that work together:

1. **nanopb_generator.py** - Generates C code from `.proto` files with memory-aware optimizations
2. **nanopb_validator.py** - Adds declarative validation constraints to protobuf messages
3. **nanopb_data_generator.py** - Generates test data for validation, fuzzing, and simulation

Together, these tools form a **Protobuf tooling ecosystem** that enables:
- Type-safe message encoding/decoding with minimal memory footprint
- Declarative validation without runtime reflection or heap allocation
- Automated test data generation for quality assurance
- Embedded-first design principles throughout the stack

### Key Characteristics

- **Small code size**: Typically 2-10 kB of code, 1-10 kB of RAM per message
- **No dynamic memory allocation**: All memory usage is predictable and static
- **Pure ANSI C99**: Portable to virtually any microcontroller or embedded platform
- **Validation support**: Define constraints in `.proto` files, enforce them in C
- **Test data generation**: Automated creation of valid and invalid test cases

## Quick Start

### 1. Install Dependencies

```bash
pip install protobuf grpcio-tools
```

### 2. Generate C Code from Protocol Buffers

```bash
# From a source checkout
python generator/nanopb_generator.py message.proto

# From a binary package
generator-bin/nanopb_generator message.proto
```

This creates `message.pb.h` and `message.pb.c` files ready for compilation.

### 3. Include in Your Project

Add these files to your build system:
- `pb_encode.c`, `pb_decode.c`, `pb_common.c` - Runtime library
- `message.pb.c` - Generated message structures
- `message.pb.h` - Message headers

See `examples/simple` for a complete working example.

## Documentation

### Getting Started
- **[Protocol Buffers Fundamentals](docs/PROTOBUF_FUNDAMENTALS.md)** - Introduction to Protocol Buffers, what they are, and why they matter
- **[Why Nanopb?](docs/WHY_NANOPB.md)** - Why choose nanopb over other implementations, embedded systems focus, and design trade-offs

### Understanding the Project
- **[Architecture & Data Flow](docs/ARCHITECTURE.md)** - How the tools work together from `.proto` to runtime validation
- **[Concepts](docs/concepts.md)** - Core nanopb concepts: streams, callbacks, encoding/decoding

### Using the Tools
- **[Using nanopb_generator](docs/USING_NANOPB_GENERATOR.md)** - Code generation guide, options, and customization
- **[Using nanopb_data_generator](docs/USING_NANOPB_DATA_GENERATOR.md)** - Test data generation for validation and fuzzing
- **[Validation Guide](docs/validation.md)** - Declarative validation constraints and generated validation code

### Advanced Topics
- **[Developer Guide](docs/DEVELOPER_GUIDE.md)** - Contributing, code structure, and design principles
- **[Reference Manual](docs/reference.md)** - Complete API and option reference
- **[Migration Guide](docs/migration.md)** - Upgrading from older versions

## Extended Tooling

### Code Generator (nanopb_generator.py)

Converts `.proto` files into memory-efficient C code:

```bash
python generator/nanopb_generator.py --help
```

**Key features:**
- Static memory allocation with configurable field sizes
- Optional fields, repeated fields, and oneofs
- Callback-based streaming for large messages
- Custom options via `.options` files

[→ Complete Generator Documentation](docs/USING_NANOPB_GENERATOR.md)

### Validation Support (nanopb_validator.py)

Enables declarative validation constraints in your `.proto` files:

```protobuf
message User {
    string username = 1 [(validate.rules).string.min_len = 3];
    int32 age = 2 [(validate.rules).int32.gte = 0];
}
```

Generate validation code with:
```bash
python generator/nanopb_generator.py --validate message.proto
```

[→ Complete Validation Documentation](docs/validation.md)

### Data Generator (nanopb_data_generator.py)

Automatically generates test data based on validation rules:

```bash
# Generate valid test data
python generator/nanopb_data_generator.py test.proto User

# Generate data that violates a specific constraint
python generator/nanopb_data_generator.py test.proto User --invalid --field age --rule gte
```

**Use cases:**
- Unit testing with valid and invalid inputs
- Fuzz testing for robustness
- Simulation data generation
- Validation rule verification

[→ Complete Data Generator Documentation](docs/USING_NANOPB_DATA_GENERATOR.md)

## Build System Integration

Nanopb integrates with multiple build systems:

| Build System | Integration File | Example |
|--------------|------------------|---------|
| **Make** | `extra/nanopb.mk` | `examples/simple` |
| **CMake** | `extra/FindNanopb.cmake` | `examples/cmake_simple` |
| **Bazel** | `BUILD.bazel` | Root directory |
| **Meson** | `meson.build` | `examples/meson_simple` |
| **Conan** | `conanfile.py` | `examples/conan_dependency` |
| **PlatformIO** | Library ID 431 | `examples/platformio` |

## Who This Is For

### Embedded Systems Developers
If you're working on microcontrollers, IoT devices, or any memory-constrained platform, nanopb provides efficient serialization without sacrificing code size or RAM.

### Firmware Engineers
Need structured communication protocols with validation? Nanopb generates type-safe C code with built-in constraint checking.

### Test Engineers
The data generator creates comprehensive test cases automatically, including edge cases and invalid inputs for robust testing.

### System Architects
Design protocol-based systems with clear message definitions, validation rules, and cross-platform compatibility.

## Running Tests

```bash
cd tests
scons
```

On Mac OS X with Clang:
```bash
cd tests
scons CC=clang CXX=clang++
```

For embedded platforms (STM32, AVR simulator):
```bash
cd tests
scons PLATFORM=STM32  # or PLATFORM=AVR
```

## Project Resources

- **Homepage**: https://jpa.kapsi.fi/nanopb/
- **Original Repository**: https://github.com/nanopb/nanopb/
- **Documentation**: https://jpa.kapsi.fi/nanopb/docs/
- **Forum**: https://groups.google.com/forum/#!forum/nanopb
- **Downloads**: https://jpa.kapsi.fi/nanopb/download/

## Contributing

Contributions are welcome! Please see [DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) for information on:
- Code structure and organization
- Design principles and philosophy
- How to add features or fix bugs
- Testing requirements

See [CONTRIBUTING.md](CONTRIBUTING.md) for general contribution guidelines.

## License

This project uses the same zlib-style license as the original nanopb project. See [LICENSE.txt](LICENSE.txt) for details.
