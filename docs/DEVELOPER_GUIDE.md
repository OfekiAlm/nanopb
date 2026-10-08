# Developer Guide

## Overview

This guide is for contributors and advanced users who want to understand the internal structure of nanopb, modify its behavior, or add new features. If you're looking to use nanopb in your project, start with the [user documentation](USING_NANOPB_GENERATOR.md) instead.

## Project Structure

```
nanopb/
├── pb.h                    # Core type definitions and macros
├── pb_encode.c/h           # Message encoding (serialization)
├── pb_decode.c/h           # Message decoding (deserialization)
├── pb_common.c/h           # Shared utilities
├── pb_validate.c/h         # Validation runtime support
│
├── generator/              # Code generation tools
│   ├── nanopb_generator.py      # Main code generator
│   ├── nanopb_validator.py      # Validation code generator
│   ├── nanopb_data_generator.py # Test data generator
│   │
│   ├── proto/              # Internal proto definitions
│   │   ├── nanopb.proto    # Nanopb-specific options
│   │   ├── validate.proto  # Validation rules
│   │   ├── nanopb_pb2.py   # Generated from nanopb.proto
│   │   └── _utils.py       # Protoc invocation utilities
│   │
│   └── protoc-gen-nanopb   # Protoc plugin wrapper
│
├── examples/               # Example projects
├── tests/                  # Test suite
├── extra/                  # Build system integration
└── docs/                   # Documentation
```

## Core Modules

### 1. Runtime Library (pb_*.c/h)

The runtime library handles message encoding and decoding at runtime using generated field descriptors.

#### pb.h - Core Definitions

**Key types:**
- `pb_field_t` - Field descriptor structure
- `pb_msgdesc_t` - Message descriptor
- `pb_istream_t` - Input stream for decoding
- `pb_ostream_t` - Output stream for encoding
- `pb_callback_t` - Callback for streaming fields

**Key macros:**
- `PB_ENCODE` / `PB_DECODE` - Main API macros
- Field type constants (`PB_LTYPE_*`, `PB_HTYPE_*`)
- Size calculation macros

**Design principle:** Pure data structures, no state.

#### pb_encode.c - Serialization

**Main functions:**
- `pb_encode()` - Encode a complete message
- `pb_encode_tag()` - Encode field tag
- `pb_encode_varint()` - Encode variable-length integer
- `pb_encode_string()` - Encode string or bytes
- `pb_encode_submessage()` - Recursively encode nested message

**Flow:**
```
pb_encode()
  ↓
For each field in message descriptor:
  ↓
pb_encode_field()
  ↓
Based on field type:
  → pb_encode_varint()
  → pb_encode_string()
  → pb_encode_submessage()
```

**Key insight:** The encoder walks the field descriptor array, not the message structure itself. This allows encoding to be generic.

#### pb_decode.c - Deserialization

**Main functions:**
- `pb_decode()` - Decode a complete message
- `pb_decode_tag()` - Decode field tag
- `pb_decode_varint()` - Decode variable-length integer
- `pb_decode_string()` - Decode string or bytes
- `pb_decode_submessage()` - Recursively decode nested message

**Flow:**
```
pb_decode()
  ↓
While bytes remain:
  ↓
pb_decode_tag() - get field number
  ↓
Find field in descriptor
  ↓
pb_decode_field() - decode field value
```

**Key insight:** Decoder tolerates unknown fields (forward compatibility) and handles fields in any order.

#### pb_common.c - Utilities

**Functions:**
- Field descriptor navigation
- Error reporting
- Helper functions shared by encoder/decoder

#### pb_validate.c - Validation Runtime

**Provides:**
- Validation result structures
- Helper macros for generated validation code
- Path tracking for error messages
- Violation accumulation (bypass mode)

**Design principle:** Minimal runtime support, most logic in generated code.

### 2. Generator Modules (generator/*.py)

#### nanopb_generator.py - Code Generator

**Purpose:** Transform `.proto` files into C code.

**Main classes:**

**`ProtoFile`**
- Represents a parsed `.proto` file
- Contains messages, enums, services
- Manages dependencies and imports

**`Message`**
- Represents a protobuf message
- Contains fields and options
- Generates C struct definition

**`Field`**
- Represents a message field
- Handles type mapping (protobuf type → C type)
- Determines allocation strategy (static/callback/pointer)

**`Enum`**
- Represents an enumeration
- Generates C enum

**Generation flow:**
```
1. Invoke protoc to parse .proto → FileDescriptorSet
2. Load .options file
3. Create ProtoFile object hierarchy
4. For each message:
   a. Generate struct definition
   b. Generate field descriptors
   c. Generate initialization macros
5. Write .pb.h and .pb.c files
6. If --validate: invoke nanopb_validator.py
```

**Key functions:**
- `generate()` - Main entry point
- `parse_file()` - Parse protoc output
- `generate_header()` - Create .pb.h
- `generate_source()` - Create .pb.c

#### nanopb_validator.py - Validation Generator

**Purpose:** Generate validation code from validation rules.

**Main classes:**

**`ValidationRule`** (dataclass)
- Represents a single constraint
- Fields: rule_type, value, field_path

**`FieldValidator`**
- Parses validation rules for a field
- Stores all rules for that field
- Generates validation code for field

**`MessageValidator`**
- Aggregates field validators
- Handles message-level rules
- Manages oneof validation

**`ValidatorGenerator`**
- Main code generator
- Creates .val.h and .val.c files
- Emits validation functions

**Generation flow:**
```
1. Extract validation options from field descriptors
2. Create ValidationRule objects
3. Create FieldValidator for each field
4. Create MessageValidator for each message
5. Generate validation function per message
6. Write .val.h and .val.c
```

**Key functions:**
- `parse_validation_rules()` - Extract rules from options
- `generate_field_validation()` - Generate code for field
- `generate_message_validation()` - Generate function for message
- `emit_validation_header()` - Write .val.h
- `emit_validation_source()` - Write .val.c

#### nanopb_data_generator.py - Data Generator

**Purpose:** Generate test data based on validation rules.

**Main classes:**

**`DataGenerator`**
- Main generator class
- Parses proto files
- Generates data

**`ProtoFieldInfo`**
- Information about a field
- Parsed validation constraints
- Type information

**`ValidationConstraint`** (dataclass)
- Represents a constraint to satisfy or violate

**Generation flow:**
```
1. Parse .proto file
2. Extract validation rules
3. For valid data:
   - Generate random value satisfying all constraints
4. For invalid data:
   - Generate value violating specified constraint
5. Encode to protobuf binary
6. Output in requested format
```

**Key functions:**
- `generate_valid()` - Create valid message
- `generate_invalid()` - Create invalid message
- `generate_field_value()` - Generate value for field
- `encode_message()` - Serialize to protobuf
- `format_output()` - Convert to output format

## Design Principles

### 1. Minimal Runtime Overhead

**Goal:** Keep generated code and runtime library as small as possible.

**Strategies:**
- No runtime reflection
- No string parsing at runtime
- Field descriptors are const data
- Code generation moves work to compile time

**Example:**
```c
// Instead of runtime parsing:
// parse_field("temperature", &msg);

// Use direct access:
msg.temperature = 23.5f;
```

### 2. Static Memory Allocation

**Goal:** Avoid malloc/free for embedded systems.

**Strategies:**
- Fixed-size arrays determined at compile time
- `.options` files specify maximum sizes
- Callbacks for truly dynamic data

**Example:**
```c
// Static allocation
typedef struct {
    char name[32];        // Fixed size
    int32_t values[10];   // Fixed count
    pb_size_t values_count;
} Message;
```

### 3. Zero-Copy Where Possible

**Goal:** Minimize data copying.

**Strategies:**
- Encode directly from struct to buffer
- Decode directly from buffer to struct
- Callbacks allow streaming without buffers

**Example:**
```c
// No intermediate buffer needed
pb_ostream_t stream = pb_ostream_from_buffer(output, sizeof(output));
pb_encode(&stream, Message_msg, &msg);
```

### 4. Forward/Backward Compatibility

**Goal:** Allow protocol evolution.

**Strategies:**
- Unknown fields are skipped (not errors)
- Optional fields handled gracefully
- Field numbers never reused
- Encodings are stable

**Example:**
```c
// Old code can decode new messages
// Unknown fields are silently ignored
pb_decode(&stream, OldMessage_msg, &msg);
```

### 5. Deterministic Behavior

**Goal:** Predictable performance for real-time systems.

**Strategies:**
- No unbounded recursion
- No dynamic allocation
- Bounded loops
- Compile-time size limits

**Example:**
```c
// Max recursion depth known at compile time
// Max array sizes known at compile time
// Max message size can be computed
```

## Adding New Features

### Adding a New Validation Rule

**Step 1: Update validate.proto**

```protobuf
// In validate.proto
message Int32Rules {
    // ... existing rules ...
    
    // Add new rule
    optional int32 divisible_by = 10;
}
```

**Step 2: Update nanopb_validator.py**

```python
# Add constant
RULE_DIVISIBLE_BY = 'DIVISIBLE_BY'

# In FieldValidator.parse_rules()
if hasattr(rules, 'divisible_by') and rules.divisible_by:
    self.rules.append(ValidationRule(
        rule_type=RULE_DIVISIBLE_BY,
        value=rules.divisible_by,
        field_path=self.field_path
    ))

# In ValidatorGenerator.generate_field_validation()
elif rule.rule_type == RULE_DIVISIBLE_BY:
    code += f"    if ({field_name} % {rule.value} != 0) {{\n"
    code += f"        PB_VALIDATION_FAIL(\"must be divisible by {rule.value}\");\n"
    code += f"    }}\n"
```

**Step 3: Update nanopb_data_generator.py**

```python
# In generate_field_value()
if 'divisible_by' in constraints:
    divisor = constraints['divisible_by']
    # Generate value that's divisible by divisor
    base = random.randint(min_val // divisor, max_val // divisor)
    return base * divisor

# In generate_invalid_field_value()
elif rule == 'divisible_by':
    divisor = constraints['divisible_by']
    # Generate value that's NOT divisible by divisor
    value = random.randint(min_val, max_val)
    while value % divisor == 0:
        value += 1
    return value
```

**Step 4: Add tests**

```python
# In tests/validation/test_divisible_by.proto
message TestDivisibleBy {
    int32 value = 1 [(validate.rules).int32.divisible_by = 5];
}

# Test with valid value (10) and invalid value (11)
```

### Adding a New Generator Option

**Step 1: Update nanopb.proto**

```protobuf
// In nanopb.proto
message NanoPBOptions {
    // ... existing options ...
    
    optional bool my_new_option = 20 [default = false];
}
```

**Step 2: Update nanopb_generator.py**

```python
# In Field class
class Field:
    def __init__(self, ...):
        # Parse option
        self.my_new_option = field_options.Extensions[
            nanopb_pb2.nanopb
        ].my_new_option if hasattr(field_options, 'Extensions') else False
    
    def generate_declaration(self):
        if self.my_new_option:
            # Generate different code
            return "/* my_new_option enabled */\n" + base_code
        else:
            return base_code
```

**Step 3: Document in .options format**

```
# Update documentation
Message.field my_new_option:true
```

**Step 4: Add tests**

Create test proto with new option and verify generated code.

### Adding a New Output Format

**Step 1: Update nanopb_data_generator.py**

```python
# In OutputFormat enum
class OutputFormat(Enum):
    BINARY = "binary"
    C_ARRAY = "c_array"
    PYTHON_DICT = "python_dict"
    HEX_STRING = "hex_string"
    MY_FORMAT = "my_format"  # New format

# Add formatting function
def format_as_my_format(self, data):
    """Convert binary data to my custom format."""
    # Your formatting logic
    result = "..."
    return result

# In format_output()
elif format_type == OutputFormat.MY_FORMAT:
    return self.format_as_my_format(data)
```

**Step 2: Update CLI parsing**

```python
# In main()
parser.add_argument('--format',
                   choices=['binary', 'c_array', 'hex_string', 
                           'python_dict', 'my_format'],
                   default='c_array')
```

## Testing

### Running the Test Suite

```bash
cd tests
scons
```

**Test organization:**
- `tests/basic/` - Core encoding/decoding tests
- `tests/validation/` - Validation tests
- `tests/generators/` - Generator tests
- `tests/regression/` - Regression tests

### Adding a Test

**Step 1: Create test proto**

```protobuf
// tests/my_feature/my_test.proto
syntax = "proto3";

message MyTest {
    int32 value = 1;
}
```

**Step 2: Create test code**

```c
// tests/my_feature/test_my_feature.c
#include <stdio.h>
#include "my_test.pb.h"

int main() {
    MyTest msg = MyTest_init_zero;
    msg.value = 42;
    
    // Test encoding
    uint8_t buffer[128];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, MyTest_msg, &msg)) {
        return 1;  // Fail
    }
    
    // Test decoding
    MyTest decoded = MyTest_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, stream.bytes_written);
    if (!pb_decode(&istream, MyTest_msg, &decoded)) {
        return 1;  // Fail
    }
    
    if (decoded.value != 42) {
        return 1;  // Fail
    }
    
    return 0;  // Success
}
```

**Step 3: Add to SCons build**

```python
# tests/my_feature/SConscript
Import('env')

env.NanopbProto(['my_test.proto'])
env.Program(['test_my_feature.c', 'my_test.pb.c'])
```

**Step 4: Run test**

```bash
cd tests
scons my_feature
```

## Code Style

### Python Code

Follow PEP 8 with these specifics:
- 4 spaces for indentation
- Max line length: 100 characters
- Use type hints where appropriate
- Docstrings for public functions

```python
def generate_code(message: Message, options: Dict[str, Any]) -> str:
    """Generate C code for a message.
    
    Args:
        message: The message to generate code for
        options: Generation options
        
    Returns:
        Generated C code as string
    """
    # Implementation
```

### C Code

- ANSI C99 standard
- 4 spaces for indentation
- Max line length: 80 characters
- Comments for non-obvious code

```c
/* Encode a varint value to stream */
bool pb_encode_varint(pb_ostream_t *stream, uint64_t value)
{
    /* Use 7-bit encoding */
    while (value > 0x7F)
    {
        /* More bytes follow */
        uint8_t byte = (uint8_t)(value & 0x7F) | 0x80;
        if (!pb_write(stream, &byte, 1))
            return false;
        value >>= 7;
    }
    
    /* Last byte */
    uint8_t byte = (uint8_t)value;
    return pb_write(stream, &byte, 1);
}
```

## Common Tasks

### Debugging Code Generation

**Enable verbose output:**
```bash
python generator/nanopb_generator.py --verbose message.proto
```

**Check intermediate files:**
```bash
# See FileDescriptorSet
protoc --descriptor_set_out=message.pb --include_imports message.proto
protoc --decode=google.protobuf.FileDescriptorSet google/protobuf/descriptor.proto < message.pb
```

**Add debug prints:**
```python
# In nanopb_generator.py
def generate_message(self, message):
    print(f"Generating message: {message.name}")
    for field in message.fields:
        print(f"  Field: {field.name}, type: {field.type}")
    # ... rest of generation
```

### Profiling Generator Performance

```python
import cProfile
import pstats

cProfile.run('generate_code()', 'profile.stats')
stats = pstats.Stats('profile.stats')
stats.sort_stats('cumulative')
stats.print_stats(20)
```

### Memory Analysis

**For generated code:**
```c
// Add to generated code
printf("sizeof(Message) = %zu\n", sizeof(Message));
printf("sizeof(field descriptors) = %zu\n", sizeof(Message_fields));
```

**For runtime library:**
```bash
# Check object file sizes
size pb_encode.o pb_decode.o pb_common.o
```

## Contributing

### Before Submitting

1. **Run tests**: `cd tests && scons`
2. **Check style**: Ensure code follows style guidelines
3. **Add tests**: New features need tests
4. **Update docs**: Document user-facing changes
5. **Test on target**: If possible, test on embedded hardware

### Pull Request Process

1. Fork the repository
2. Create a feature branch
3. Make changes with clear commit messages
4. Add tests for new features
5. Update documentation
6. Submit pull request with description

### Review Criteria

- Code quality and style
- Test coverage
- Documentation completeness
- Backward compatibility
- Memory efficiency
- Code size impact

## Resources

### Learning Resources

- [Protocol Buffers Fundamentals](PROTOBUF_FUNDAMENTALS.md)
- [Why Nanopb?](WHY_NANOPB.md)
- [Architecture Overview](ARCHITECTURE.md)
- [Official protobuf documentation](https://protobuf.dev/)

### Tools

- **protoc**: Protocol buffer compiler
- **scons**: Build system for tests
- **gdb**: Debugging on embedded targets
- **valgrind**: Memory leak detection

### Community

- **Forum**: https://groups.google.com/forum/#!forum/nanopb
- **Issues**: GitHub issue tracker
- **Original repo**: https://github.com/nanopb/nanopb

## FAQ for Developers

### How does nanopb achieve small code size?

1. No reflection - field layout is fixed at compile time
2. No text parsing - binary only
3. Minimal runtime - most logic in generated code
4. Optional features are truly optional
5. Shared code between encoder/decoder

### Why use field descriptors instead of direct access?

Field descriptors allow the runtime library to be generic. One `pb_encode()` function handles all message types by reading the descriptor array.

### Can I use C++ features?

No, nanopb is pure C for maximum portability. C++ projects can use the C code, but the generator produces only C.

### How do I optimize for my specific use case?

1. Use `.options` to set exact buffer sizes
2. Disable unused features
3. Use callbacks for large data
4. Consider message design (smaller field numbers = smaller encoding)

### Why isn't feature X supported?

Nanopb focuses on embedded systems. Features requiring:
- Dynamic allocation
- Large code size
- Runtime reflection
- Complex dependencies

are intentionally not supported to keep nanopb suitable for constrained systems.

## Next Steps

- **[Architecture Overview](ARCHITECTURE.md)** - Understand the system design
- **[Validation Guide](validation.md)** - Learn validation internals
- Start contributing! Check GitHub issues for "good first issue" labels
