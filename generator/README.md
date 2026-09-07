# Nanopb Generator

This directory contains the Python-based code generator that converts Protocol Buffer definitions (`.proto` files) into C code optimized for embedded systems.

## Table of Contents

- [Overview](#overview)
- [Files in This Directory](#files-in-this-directory)
- [Quick Start](#quick-start)
- [Usage Examples](#usage-examples)
- [Options System](#options-system)
- [Architecture](#architecture)
- [Development](#development)

## Overview

The nanopb generator takes `.proto` files and produces:
- **`.pb.h`** - C header with struct definitions and declarations
- **`.pb.c`** - C source with message descriptors and metadata

These generated files work with the nanopb runtime library to encode and decode Protocol Buffer messages on embedded systems.

## Files in This Directory

### Main Generator Files

- **`nanopb_generator.py`** (~3600 lines)
  - Main code generator
  - Parses `.proto` files via protoc
  - Generates C structs and message descriptors
  - Handles field options and customization

- **`nanopb_validator.py`** (~1100 lines)
  - Validation code generator extension
  - Parses validation constraints from `.proto` files
  - Generates runtime validation functions
  - Based on protoc-gen-validate specification

### Proto Support Files

- **`proto/`** - Helper modules and proto definitions
  - `nanopb.proto` - Defines nanopb-specific options
  - `validate.proto` - Defines validation rules
  - `nanopb_pb2.py` - Generated from nanopb.proto
  - `validate_pb2.py` - Generated from validate.proto
  - `_utils.py` - Protoc invocation utilities

### Platform-Specific Files

- **`nanopb_generator`** - Unix shell wrapper
- **`nanopb_generator.bat`** - Windows batch wrapper
- **`protoc-gen-nanopb`** - Protoc plugin wrapper (Unix)
- **`protoc-gen-nanopb.bat`** - Protoc plugin wrapper (Windows)
- **`protoc`** / **`protoc.bat`** - Bundled protoc compiler

### Additional Tools

- **`nanopb_data_generator.py`** - Generate test data from proto definitions
- **`platformio_generator.py`** - PlatformIO integration
- **`__init__.py`** - Python package initialization

## Quick Start

### Basic Usage

Generate code from a proto file:

```bash
python nanopb_generator.py myfile.proto
```

This creates:
- `myfile.pb.h` - Header with struct definitions
- `myfile.pb.c` - Source with message descriptors

### With Custom Options

Create `myfile.options` to customize generation:

```
# myfile.options
MyMessage.my_string max_size:40
MyMessage.my_array max_count:10
```

Then run:

```bash
python nanopb_generator.py myfile.proto
```

The generator automatically finds and applies `myfile.options`.

### As Protoc Plugin

Use as a protoc plugin:

```bash
protoc --plugin=protoc-gen-nanopb=nanopb_generator \
       --nanopb_out=. \
       myfile.proto
```

This is useful when integrating with build systems that already use protoc.

## Usage Examples

### Example 1: Simple Message

**Input:** `person.proto`
```protobuf
syntax = "proto3";

message Person {
    int32 id = 1;
    string name = 2;
    string email = 3;
}
```

**Generate:**
```bash
python nanopb_generator.py person.proto
```

**Output:** `person.pb.h`
```c
typedef struct _Person {
    int32_t id;
    char name[40];    /* Default max_size */
    char email[40];
} Person;

extern const pb_msgdesc_t Person_msg;
#define Person_fields &Person_msg
```

### Example 2: With Options

**Input:** `person.options`
```
Person.name max_size:100
Person.email max_size:200
```

**Generate:**
```bash
python nanopb_generator.py person.proto
```

**Output:** `person.pb.h` (with custom sizes)
```c
typedef struct _Person {
    int32_t id;
    char name[100];    /* Custom size */
    char email[200];   /* Custom size */
} Person;
```

### Example 3: Repeated Fields

**Input:** `data.proto`
```protobuf
message Data {
    repeated int32 values = 1;
}
```

**Options:** `data.options`
```
Data.values max_count:50
```

**Output:**
```c
typedef struct _Data {
    pb_size_t values_count;
    int32_t values[50];
} Data;
```

### Example 4: Callbacks for Large Data

**Input:** `file.proto`
```protobuf
message FileData {
    bytes content = 1;
}
```

**Options:** `file.options`
```
FileData.content type:FT_CALLBACK
```

**Output:**
```c
typedef struct _FileData {
    pb_callback_t content;  /* User provides encode/decode */
} FileData;
```

### Example 5: Validation

**Input:** `user.proto`
```protobuf
syntax = "proto3";
import "validate.proto";

message User {
    string email = 1 [(validate.rules).string = {
        email: true,
        min_len: 5,
        max_len: 100
    }];
    
    int32 age = 2 [(validate.rules).int32 = {
        gte: 0,
        lte: 120
    }];
}
```

**Generate:**
```bash
python nanopb_generator.py user.proto
```

**Output:** `user.pb.h` includes:
```c
bool validate_User(const User *msg);
```

**Output:** `user.pb.c` includes:
```c
bool validate_User(const User *msg) {
    /* Email validation */
    if (!validate_email_format(msg->email)) return false;
    if (strlen(msg->email) < 5) return false;
    if (strlen(msg->email) > 100) return false;
    
    /* Age validation */
    if (msg->age < 0) return false;
    if (msg->age > 120) return false;
    
    return true;
}
```

## Options System

### Option File Format

Options files use a simple text format:

```
# Comment
MessageName.field_name option:value
* option:value              # Apply to all
PackageName.* option:value  # Apply to package
```

### Common Field Options

```
max_size:N         # Maximum size for strings/bytes/arrays
max_count:N        # Maximum count for repeated fields
type:FT_STATIC     # Allocation: STATIC/POINTER/CALLBACK
type:FT_POINTER    # Use pointers (requires malloc)
type:FT_CALLBACK   # Use callbacks for large data
int_size:IS_8      # Integer size override
anonymous_oneof:true   # Generate anonymous union for oneof
proto3:true        # Use proto3 semantics
no_unions:true     # Don't use unions for oneofs
```

### Message Options

```
msgid:N            # Message identifier
skip_message:true  # Don't generate this message
```

### File Options

```
package:name       # Override package name
long_names:false   # Use shorter names
```

### Applying Options

#### 1. From .options File

Most common method:

```bash
# myfile.proto + myfile.options
python nanopb_generator.py myfile.proto
```

#### 2. From Command Line

```bash
python nanopb_generator.py \
    -s "MyMessage.field max_size:100" \
    myfile.proto
```

#### 3. In Proto File (Annotations)

```protobuf
import "nanopb.proto";

message MyMessage {
    string field = 1 [(nanopb).max_size = 100];
}
```

#### 4. Separate Options File

```bash
python nanopb_generator.py \
    -f custom_options.txt \
    myfile.proto
```

## Architecture

### Generator Pipeline

```
.proto file
    │
    ▼
┌─────────────────────┐
│  protoc parses      │
│  .proto file        │
└──────┬──────────────┘
       │ FileDescriptorSet
       ▼
┌─────────────────────┐
│  nanopb_generator   │
│  processes:         │
│  - Messages         │
│  - Fields           │
│  - Options          │
└──────┬──────────────┘
       │
       ├─► .pb.h (structs)
       └─► .pb.c (descriptors)
```

### Key Classes

- **`ProtoFile`** - Represents entire .proto file
  - Orchestrates code generation
  - Manages dependencies
  - Generates header and source

- **`Message`** - Represents a message type
  - Stores fields and nested types
  - Generates struct definition
  - Generates message descriptor

- **`Field`** - Represents a message field
  - Determines C type
  - Handles allocation strategy
  - Generates field descriptor

- **`Enum`** - Represents enum type
  - Generates C enum
  - Handles value naming

- **`Names`** - Manages C identifier naming
  - Converts proto names to C
  - Handles nested names
  - Applies naming styles

### Code Generation Phases

1. **Parse** - Load FileDescriptorSet from protoc
2. **Process Options** - Apply .options file and annotations
3. **Resolve Dependencies** - Sort messages by dependencies
4. **Generate Header** - Create .pb.h with structs
5. **Generate Source** - Create .pb.c with descriptors
6. **Validate** - Optionally generate validation code

For detailed architecture, see:
- [../docs/generator/GENERATOR_ARCHITECTURE.md](../docs/generator/GENERATOR_ARCHITECTURE.md)
- [../docs/generator/VALIDATOR_ARCHITECTURE.md](../docs/generator/VALIDATOR_ARCHITECTURE.md)

## Development

### Prerequisites

```bash
# Install dependencies
pip install protobuf grpcio-tools

# Or use requirements file
pip install -r ../requirements.txt
```

### Running Tests

```bash
# Run all tests
cd ../tests
scons

# Run specific test
cd ../tests/basic_buffer
make
```

### Modifying the Generator

1. **Make changes** to `nanopb_generator.py` or `nanopb_validator.py`

2. **Test with example:**
   ```bash
   python nanopb_generator.py ../examples/simple/simple.proto
   ```

3. **Run test suite:**
   ```bash
   cd ../tests
   scons
   ```

4. **Check generated code:**
   - Review .pb.h for correct struct definitions
   - Review .pb.c for correct descriptors
   - Compile and test with runtime

### Adding Features

#### Adding a New Option

1. Define in `proto/nanopb.proto`:
   ```protobuf
   extend google.protobuf.FieldOptions {
       optional bool my_option = 1050;
   }
   ```

2. Regenerate `nanopb_pb2.py`:
   ```bash
   ./proto/regenerate.sh
   ```

3. Process in generator:
   ```python
   if field.has_option('my_option'):
       # Handle option
   ```

4. Add tests and documentation

#### Adding a New Field Type

1. Add to `datatypes` dictionary
2. Implement encoding/decoding in runtime
3. Update Field class methods
4. Add tests

### Debugging

#### Enable Verbose Output

```bash
python nanopb_generator.py -v myfile.proto
```

This shows:
- Option matching
- Field processing
- Size calculations

#### Check Generated Descriptors

Look at the `.pb.c` file to verify field descriptors:

```c
static const pb_field_t MyMessage_fields[3] = {
    PB_FIELD(1, INT32, REQUIRED, STATIC, ...),
    PB_FIELD(2, STRING, OPTIONAL, STATIC, ...),
    PB_LAST_FIELD
};
```

#### Test with Simple Proto

Create minimal test case:

```protobuf
message Test {
    int32 value = 1;
}
```

Generate and inspect output.

## Command-Line Reference

### Basic Syntax

```bash
python nanopb_generator.py [options] file.proto [file2.proto ...]
```

### Common Options

```
-h, --help              Show help message
-v, --verbose           Verbose output
-D, --output-dir DIR    Output directory
-I DIR                  Add import path for .proto files
-f FILE                 Load options from file
-s OPTION               Set option from command line
-e, --extension EXT     Set extension for generated files
-L, --no-strip-path     Don't strip path from output files
--protoc-insertion-points  Enable protoc insertion points
```

### Examples

```bash
# Basic generation
python nanopb_generator.py message.proto

# Output to specific directory
python nanopb_generator.py -D output/ message.proto

# Add import path
python nanopb_generator.py -I../proto message.proto

# Set options from command line
python nanopb_generator.py \
    -s "Message.field max_size:100" \
    message.proto

# Multiple proto files
python nanopb_generator.py msg1.proto msg2.proto msg3.proto
```

## Common Issues

### Issue: "Could not import Google protobuf"

**Solution:**
```bash
pip install --upgrade protobuf grpcio-tools
```

### Issue: "Option not applied"

**Causes:**
- Typo in option name
- Wrong message/field name
- Option file not found

**Debug:**
```bash
python nanopb_generator.py -v myfile.proto
# Shows which options were matched
```

### Issue: "Field size too large"

**Solution:** Use callbacks or pointers:
```
LargeMessage.data type:FT_CALLBACK
```

### Issue: "Circular dependency"

**Solution:** Use forward declarations or pointer fields

## Best Practices

1. **Use .options files** for field customization
   - Keep proto files clean
   - Easy to maintain sizes

2. **Set appropriate max_size** for strings
   - Balance memory vs functionality
   - Consider typical use cases

3. **Use callbacks** for large/dynamic data
   - Saves memory
   - More flexible

4. **Validate after decoding**
   - Use validation rules in .proto
   - Catch invalid data early

5. **Test generated code**
   - Compile and run tests
   - Check memory usage
   - Verify functionality

## Further Reading

- [../ARCHITECTURE.md](../ARCHITECTURE.md) - Overall nanopb architecture
- [../docs/generator/GENERATOR_ARCHITECTURE.md](../docs/generator/GENERATOR_ARCHITECTURE.md) - Generator internals
- [../docs/generator/VALIDATOR_ARCHITECTURE.md](../docs/generator/VALIDATOR_ARCHITECTURE.md) - Validator internals
- [../docs/concepts.md](../docs/concepts.md) - Generator usage guide
- [../docs/reference.md](../docs/reference.md) - API reference
- [../docs/validation.md](../docs/validation.md) - Validation guide

## Contributing

Contributions welcome! See [../CONTRIBUTING.md](../CONTRIBUTING.md)

When contributing to the generator:
1. Maintain backward compatibility
2. Add comprehensive tests
3. Update documentation
4. Follow existing code style
5. Test on multiple platforms

---

**Version:** nanopb 1.0.0-dev
**Maintainers:** See [../AUTHORS.txt](../AUTHORS.txt)
