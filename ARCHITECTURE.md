# Nanopb Architecture Overview

Welcome to the nanopb architecture documentation! This guide will help you understand the system design, making it easier to maintain and develop nanopb.

## Table of Contents

- [Introduction](#introduction)
- [High-Level Architecture](#high-level-architecture)
- [Core Components](#core-components)
- [Data Flow](#data-flow)
- [Directory Structure](#directory-structure)
- [Further Reading](#further-reading)

## Introduction

Nanopb is a Protocol Buffers implementation for embedded systems, consisting of two main parts:

1. **Code Generator** (`generator/`) - Python tools that convert `.proto` files to C code
2. **Runtime Library** (root `.c/.h` files) - ANSI C library for encoding/decoding messages

This architecture is designed for:
- **Minimal Memory Footprint** - Suitable for microcontrollers
- **Zero Dependencies** - Pure ANSI C runtime, no malloc required
- **Build System Agnostic** - Works with any C compiler and build system

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Development Time                          │
│                                                              │
│  .proto files  ──►  nanopb_generator.py  ──►  .pb.h/.pb.c  │
│                           │                                  │
│                           ├──► nanopb_validator.py          │
│                           │    (validation support)          │
│                           │                                  │
│                           └──► proto descriptors             │
│                                (nanopb_pb2, validate_pb2)    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Runtime / Embedded                      │
│                                                              │
│  Your Application                                            │
│       │                                                      │
│       ├──► pb_encode.h/c  (Message encoding)                │
│       ├──► pb_decode.h/c  (Message decoding)                │
│       ├──► pb_common.h/c  (Shared utilities)                │
│       ├──► pb_validate.h/c (Runtime validation)             │
│       └──► .pb.h/.pb.c    (Generated message definitions)   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Code Generator (`generator/`)

The code generator transforms Protocol Buffer definitions into C code optimized for embedded systems.

#### Main Components:

- **`nanopb_generator.py`** - The main generator script
  - Parses `.proto` files using Google's protobuf compiler
  - Generates C struct definitions and message descriptors
  - Handles options for memory allocation, field sizes, callbacks
  - Supports 3600+ lines of sophisticated code generation logic

- **`nanopb_validator.py`** - Validation code generator
  - Extends generator with declarative validation constraints
  - Supports field-level and message-level validation rules
  - Generates runtime validation functions
  - Based on protoc-gen-validate specification

**Key Concepts:**
- **Message Descriptors** - Runtime metadata describing message structure
- **Field Options** - Customize generation (max_size, type, etc.)
- **Naming Styles** - Convert proto names to C identifiers
- **Callbacks** - Handle dynamic or large data structures

For detailed architecture, see [docs/generator/GENERATOR_ARCHITECTURE.md](docs/generator/GENERATOR_ARCHITECTURE.md)

### 2. Runtime Library (Root Directory)

The runtime library provides encoding/decoding functionality with minimal overhead.

#### Core Files:

- **`pb.h`** - Main header with type definitions and configuration
  - Compilation options (malloc, 64-bit, packed structs)
  - Field descriptors and message descriptors
  - Core types: `pb_field_t`, `pb_msgdesc_t`

- **`pb_encode.h/c`** - Message encoding
  - Stream-based encoding for memory efficiency
  - Supports callbacks for large/dynamic data
  - ~30KB of highly optimized C code

- **`pb_decode.h/c`** - Message decoding
  - Stream-based decoding
  - Handles unknown fields gracefully
  - ~55KB of robust decoding logic

- **`pb_common.h/c`** - Shared utilities
  - Helper functions used by both encoder and decoder
  - Message descriptor utilities
  - ~12KB of common code

- **`pb_validate.h/c`** - Runtime validation
  - Validates decoded messages against constraints
  - Field-level and message-level checks
  - ~23KB of validation logic

**Key Design Patterns:**
- **Zero-copy Streaming** - Process data without full buffering
- **Static Allocation** - No malloc required (optional malloc support available)
- **Callback Mechanism** - Handle arbitrarily large messages
- **Union-based Oneofs** - Efficient oneof field representation

For detailed architecture, see [docs/runtime/README.md](docs/runtime/README.md)

### 3. Proto Support Files (`generator/proto/`)

Helper modules for the generator:
- `nanopb_pb2.py` - Generated from `nanopb.proto`, defines nanopb options
- `validate_pb2.py` - Generated from `validate.proto`, defines validation rules
- `_utils.py` - Protoc invocation utilities

## Data Flow

### Code Generation Flow

```
1. User writes .proto file
   └─► Defines messages, fields, services
   
2. User optionally creates .options file
   └─► Customizes generation (max_size, callbacks, etc.)
   
3. nanopb_generator.py runs
   ├─► Invokes protoc to parse .proto
   ├─► Loads FileDescriptorSet
   ├─► Processes field options
   ├─► Generates C structs in .pb.h
   ├─► Generates message descriptors in .pb.c
   └─► Optionally generates validation code (nanopb_validator.py)
   
4. Output: .pb.h and .pb.c files
   └─► Ready to compile with your application
```

### Runtime Encoding Flow

```
1. Application populates message struct
   
2. Calls pb_encode() or pb_encode_ex()
   ├─► Encoder iterates through message fields
   ├─► Uses message descriptor for field metadata
   ├─► Encodes each field in Protocol Buffers wire format
   └─► Handles submessages recursively
   
3. Encoded data written to stream or buffer
```

### Runtime Decoding Flow

```
1. Application receives encoded data

2. Calls pb_decode() or pb_decode_ex()
   ├─► Decoder reads wire format tags and values
   ├─► Uses message descriptor to find fields
   ├─► Populates message struct fields
   └─► Handles submessages recursively
   
3. Optionally validates with pb_validate()
   
4. Application uses decoded message struct
```

## Directory Structure

```
nanopb/
├── README.md              # Main project readme
├── ARCHITECTURE.md        # This file - system architecture overview
├── CHANGELOG.txt          # Version history
├── CONTRIBUTING.md        # Contribution guidelines
│
├── pb.h                   # Main runtime header
├── pb_common.[ch]         # Common runtime utilities  
├── pb_encode.[ch]         # Encoding functionality
├── pb_decode.[ch]         # Decoding functionality
├── pb_validate.[ch]       # Validation functionality
│
├── generator/             # Code generation tools
│   ├── nanopb_generator.py      # Main generator (3600+ lines)
│   ├── nanopb_validator.py      # Validation generator (1100+ lines)
│   ├── proto/                   # Proto support files
│   │   ├── nanopb.proto         # Nanopb options definition
│   │   ├── validate.proto       # Validation rules definition
│   │   └── _utils.py            # Protoc utilities
│   └── README.md                # Generator documentation
│
├── docs/                  # Comprehensive documentation
│   ├── index.md           # Docs entry point
│   ├── concepts.md        # Core concepts and usage
│   ├── reference.md       # API reference
│   ├── validation.md      # Validation feature guide
│   ├── generator/         # Generator-specific docs
│   │   ├── GENERATOR_ARCHITECTURE.md  # Generator internals
│   │   └── VALIDATOR_ARCHITECTURE.md  # Validator internals
│   └── runtime/           # Runtime-specific docs
│       └── README.md      # Runtime architecture
│
├── examples/              # Example projects
│   ├── simple/            # Basic usage example
│   ├── network_server/    # Network application
│   ├── validation_simple/ # Validation example
│   └── ...
│
├── tests/                 # Comprehensive test suite
│   ├── SConstruct         # SCons build file
│   └── */                 # Individual test cases
│
├── extra/                 # Build system integrations
│   ├── nanopb.mk          # Makefile rules
│   ├── FindNanopb.cmake   # CMake module
│   └── ...
│
└── tools/                 # Additional utilities
```

## Further Reading

### For New Developers

Start here to understand the codebase:
1. [README.md](README.md) - Quick start and basic usage
2. [docs/index.md](docs/index.md) - Documentation overview
3. [docs/concepts.md](docs/concepts.md) - Core concepts
4. [examples/simple/](examples/simple/) - Working example

### For Generator Development

Deep dive into code generation:
1. [generator/README.md](generator/README.md) - Generator overview
2. [docs/generator/GENERATOR_ARCHITECTURE.md](docs/generator/GENERATOR_ARCHITECTURE.md) - Generator internals
3. [docs/generator/VALIDATOR_ARCHITECTURE.md](docs/generator/VALIDATOR_ARCHITECTURE.md) - Validator internals

### For Runtime Development

Understand the runtime library:
1. [docs/runtime/README.md](docs/runtime/README.md) - Runtime architecture
2. [docs/reference.md](docs/reference.md) - API reference
3. [pb.h](pb.h) - Core type definitions

### For Contributors

1. [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
2. [tests/README_validation_test.md](tests/README_validation_test.md) - Testing guide
3. [docs/security.md](docs/security.md) - Security considerations

## Key Design Principles

1. **Embedded-First Design**
   - Minimal memory usage (configurable limits)
   - No dynamic allocation by default
   - Predictable resource usage

2. **Portability**
   - ANSI C (C99 minimum)
   - No OS dependencies
   - Works on 8-bit to 64-bit systems

3. **Flexibility**
   - Multiple allocation strategies (static, stack, malloc)
   - Stream or buffer based I/O
   - Optional features can be disabled

4. **Maintainability**
   - Clear separation: generator vs runtime
   - Extensive test coverage
   - Well-documented code

## Getting Help

- **Documentation**: https://jpa.kapsi.fi/nanopb/docs/
- **Forum**: https://groups.google.com/forum/#!forum/nanopb
- **Issues**: https://github.com/nanopb/nanopb/issues
- **Examples**: See `examples/` directory

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

Areas that often need help:
- Additional validation rules
- Build system integrations
- Documentation improvements
- Platform-specific testing
- Example applications

---

**Last Updated**: November 2024
**Nanopb Version**: 1.0.0-dev
