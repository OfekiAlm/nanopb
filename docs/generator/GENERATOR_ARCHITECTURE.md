# Nanopb Generator Architecture

This document describes the internal architecture of `nanopb_generator.py`, the tool that converts Protocol Buffer definitions (`.proto` files) into C code optimized for embedded systems.

## Table of Contents

- [Overview](#overview)
- [Execution Flow](#execution-flow)
- [Core Classes](#core-classes)
- [Code Generation Pipeline](#code-generation-pipeline)
- [Key Algorithms](#key-algorithms)
- [Extension Points](#extension-points)
- [Development Guide](#development-guide)

## Overview

The generator is a ~3600-line Python script that:
1. Invokes `protoc` to parse `.proto` files
2. Processes the resulting `FileDescriptorSet`
3. Generates optimized C struct definitions (`.pb.h`)
4. Generates message descriptors for runtime (`.pb.c`)

**Key Design Goals:**
- Minimize generated code size
- Support static memory allocation
- Provide flexible field sizing options
- Enable callback mechanisms for large data

## Execution Flow

### Main Entry Points

The generator can be invoked in two modes:

1. **CLI Mode** (`main_cli()`): Direct command-line execution
   ```bash
   python nanopb_generator.py myfile.proto
   ```

2. **Plugin Mode** (`main_plugin()`): As a `protoc` plugin
   ```bash
   protoc --nanopb_out=. myfile.proto
   ```

### Processing Pipeline

```
┌──────────────────┐
│  Parse Arguments │
│  process_cmdline │
└────────┬─────────┘
         │
         ▼
┌──────────────────────────┐
│  Invoke protoc           │
│  - Generates descriptor  │
│  - Parses .proto files   │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────────────┐
│  Load FileDescriptorSet  │
│  parse_file()            │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────────────┐
│  Process Each File       │
│  process_file()          │
│  - Read .options         │
│  - Build ProtoFile       │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────────────┐
│  Generate Code           │
│  ProtoFile.generate()    │
│  - Generate .pb.h        │
│  - Generate .pb.c        │
└──────────────────────────┘
```

## Core Classes

### 1. ProtoElement (Base Class)

Abstract base class for all protobuf elements (messages, fields, enums).

**Key Methods:**
- `__init__()` - Initialize from descriptor
- `str()` - Get C identifier name
- `comment_lines` - Extract and format comments from proto

**Subclasses:**
- `Enum` - Enumeration types
- `Field` - Message fields
- `Message` - Message types
- `Service` - RPC services
- `OneOf` - Oneof groups
- `ExtensionField` - Extension fields

### 2. ProtoFile

Represents a complete `.proto` file and orchestrates code generation.

**Attributes:**
```python
class ProtoFile:
    def __init__(self, fdesc, file_options):
        self.fdesc = fdesc              # FileDescriptorProto
        self.file_options = file_options # Parsed options
        self.messages = []              # List of Message objects
        self.enums = []                 # List of Enum objects
        self.extensions = []            # List of ExtensionField objects
        self.dependencies = {}          # Imported files
```

**Key Methods:**
- `generate()` - Main code generation entry point
- `generate_header()` - Generate .pb.h file
- `generate_source()` - Generate .pb.c file

### 3. Message

Represents a Protocol Buffer message type.

**Responsibilities:**
- Store message fields and nested types
- Calculate message size and alignment
- Generate struct definition
- Generate message descriptor

**Key Attributes:**
```python
class Message(ProtoElement):
    self.fields = []           # Field objects
    self.oneofs = []           # OneOf groups
    self.submessages = []      # Nested messages
    self.subenums = []         # Nested enums
    self.packed = False        # Packed struct option
```

**Key Methods:**
- `get_dependencies()` - Resolve message dependencies
- `fields_declaration()` - Generate C struct fields
- `fields_definition()` - Generate field descriptors

### 4. Field

Represents a single field in a message.

**Field Types Handled:**
- Scalar types (int32, bool, float, etc.)
- String and bytes
- Submessages
- Repeated fields
- Oneofs
- Maps (as repeated fields)

**Key Attributes:**
```python
class Field(ProtoElement):
    self.pbtype = None         # Protocol Buffer type
    self.ctype = None          # C type name
    self.rules = None          # REQUIRED/OPTIONAL/REPEATED
    self.allocation = None     # STATIC/CALLBACK/POINTER
    self.max_size = None       # Maximum size for arrays/strings
    self.default = None        # Default value
```

**Key Methods:**
- `ctype_declaration()` - Generate C type for field
- `default_decl()` - Generate default value
- `pb_field_t()` - Generate field descriptor
- `largest_field_value()` - Calculate encoded size

### 5. Enum

Represents an enumeration type.

**Key Responsibilities:**
- Store enum values
- Generate C enum definition
- Handle enum value naming

**Key Methods:**
- `enum_declaration()` - Generate C enum
- `has_negative()` - Check for negative values (affects type)

### 6. Names

Manages hierarchical naming and C identifier generation.

**Purpose:**
- Convert proto package.Message.field to C identifiers
- Handle nested message names
- Apply naming style (underscore vs camelCase)

**Example:**
```python
names = Names(('mypackage', 'MyMessage', 'my_field'))
str(names)  # 'mypackage_MyMessage_my_field'
```

### 7. Globals

Stores global configuration (singleton-like pattern).

**Attributes:**
```python
class Globals:
    verbose_options = False      # Print option matching
    separate_options = []        # Options from separate files
    matched_namemasks = set()    # Track which options were used
    protoc_insertion_points = False  # Enable protoc insertion points
    naming_style = NamingStyle() # Naming convention
```

## Code Generation Pipeline

### Phase 1: Parsing and Option Processing

```python
def parse_file(filename, fdesc, options):
    """
    1. Create ProtoFile object
    2. Load file-level options
    3. Iterate through messages, enums, services
    4. Apply options to each element
    5. Resolve dependencies
    """
```

**Option Resolution:**
- File-level options from `.options` file
- Message-level options from proto annotations
- Field-level options (override file/message options)
- Default options (fallback)

**Option Types:**
```python
# Field options
max_size: 40           # Array/string size
max_count: 10          # Repeated field count
type: FT_CALLBACK      # Allocation type
anonymous_oneof: true  # Generate anonymous union

# Message options
msgid: 42             # Message identifier
skip_message: true    # Don't generate this message

# File options
package: "mypackage"  # Override package name
```

### Phase 2: Dependency Resolution

```python
def sort_dependencies(messages):
    """
    Topologically sort messages so that dependencies
    are declared before they are used.
    """
```

**Dependency Types:**
- Direct field references (Message A has field of type Message B)
- Callback function references
- Extension dependencies

### Phase 3: Header Generation

```python
def generate_header(self):
    """
    Generate .pb.h file:
    1. File header and includes
    2. Forward declarations
    3. Enum definitions
    4. Struct definitions (in dependency order)
    5. Message descriptor extern declarations
    6. Helper macros
    7. Default value #defines
    """
```

**Generated Header Structure:**
```c
/* Automatically generated */
#ifndef PB_MYFILE_PB_H_INCLUDED
#define PB_MYFILE_PB_H_INCLUDED

#include <pb.h>

/* Enum definitions */
typedef enum _MyEnum { ... } MyEnum;

/* Struct forward declarations */
typedef struct _MyMessage MyMessage;

/* Struct definitions */
struct _MyMessage {
    int32_t field1;
    char field2[40];
    ...
};

/* Message descriptors */
extern const pb_msgdesc_t MyMessage_msg;
#define MyMessage_fields &MyMessage_msg

/* Helper macros */
#define MyMessage_init_default { ... }
#define MyMessage_init_zero { ... }

#endif
```

### Phase 4: Source Generation

```python
def generate_source(self):
    """
    Generate .pb.c file:
    1. File header and includes
    2. Field descriptor arrays
    3. Message descriptors
    4. Submessage tables
    """
```

**Generated Source Structure:**
```c
/* Automatically generated */
#include "myfile.pb.h"

/* Field descriptor array */
static const pb_field_t MyMessage_fields[N] = {
    PB_FIELD(...),
    PB_FIELD(...),
    PB_LAST_FIELD
};

/* Message descriptor */
const pb_msgdesc_t MyMessage_msg = {
    MyMessage_fields,
    MyMessage_DEFAULT,
    NULL,  // submsg_info
    ...
};
```

## Key Algorithms

### 1. Size Calculation

The generator calculates maximum encoded size for messages:

```python
class EncodedSize:
    """
    Calculate worst-case encoded size:
    - Tag size (varint)
    - Length delimiter (for strings/bytes/submessages)
    - Value size
    - Repeated field overhead
    """
    def __init__(self, value, declarations):
        self.value = value  # Size expression
        self.declarations = declarations  # Required symbols
```

**Size Calculation Rules:**
- Varint: 1-10 bytes depending on maximum value
- Fixed types: Fixed size (4 or 8 bytes)
- Strings/bytes: max_size + length delimiter
- Submessages: recursive calculation
- Repeated: max_count × single element size

### 2. Field Descriptor Generation

Each field generates a descriptor for runtime:

```c
PB_FIELD(tag, type, allocation, placement, struct, field, prev_field)
```

**Descriptor Information:**
- Tag number (for wire format)
- Data type (PB_LTYPE_*, PB_HTYPE_*)
- Allocation type (STATIC, CALLBACK, POINTER)
- Struct offset
- Size information
- Submessage pointer (if applicable)

### 3. Oneof Handling

Oneofs generate C union types:

```c
struct MyMessage {
    pb_size_t which_myoneof;  // Discriminator
    union {
        int32_t option1;
        char option2[40];
    } myoneof;
};
```

**Discriminator Values:**
- 0 = no field set
- Field tag number = that field is set

### 4. Callback Mechanism

For dynamic/large data, generate callback fields:

```c
struct MyMessage {
    pb_callback_t large_field;  // User-provided callbacks
};

// User provides:
typedef struct {
    bool (*encode)(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
    bool (*decode)(pb_istream_t *stream, const pb_field_t *field, void **arg);
} pb_callback_t;
```

## Extension Points

### 1. Custom Naming Styles

Override `NamingStyle` class:

```python
class MyNamingStyle(NamingStyle):
    def var_name(self, name):
        return "m_" + name  # Hungarian notation
```

### 2. Custom Options

Add to `nanopb.proto`:

```protobuf
extend google.protobuf.FieldOptions {
    optional bool my_option = 50000;
}
```

Then process in generator:
```python
if field_options.HasExtension(my_option):
    # Custom handling
```

### 3. Validation Integration

The generator can invoke `nanopb_validator.py` to add validation:

```python
from nanopb_validator import ValidatorGenerator

validator_gen = ValidatorGenerator(proto_file)
validator_gen.add_message_validator(message, message_rules)
validation_code = validator_gen.generate_validation_functions()
```

## Development Guide

### Adding a New Field Type

1. Add type mapping in `datatypes` dictionary
2. Update `Field.ctype_declaration()` for C type
3. Update `Field.pb_field_t()` for descriptor
4. Add encoding/decoding logic (in runtime)
5. Add test cases

### Adding a New Option

1. Define in `generator/proto/nanopb.proto`
2. Regenerate `nanopb_pb2.py`
3. Process in `get_nanopb_suboptions()`
4. Apply in relevant class (`Field`, `Message`, etc.)
5. Update documentation
6. Add test cases

### Debugging Tips

1. **Enable verbose options:**
   ```bash
   python nanopb_generator.py -v myfile.proto
   ```

2. **Check generated descriptors:**
   Look at `.pb.c` file to verify field descriptors

3. **Test with simple proto:**
   Start with minimal example to isolate issues

4. **Use print statements:**
   Add debug output in generator code

5. **Compare with working example:**
   Check `examples/` directory for reference

### Testing

Run generator tests:
```bash
cd tests
scons  # Runs all tests including generator tests
```

Key test directories:
- `tests/basic_buffer/` - Basic functionality
- `tests/options/` - Option processing
- `tests/field_size/` - Size calculations
- `tests/oneof/` - Oneof handling

## Common Patterns

### Iterating Through Messages

```python
from nanopb_generator import iterate_messages

for message in iterate_messages(file_desc, flatten=True):
    # Process message
    for field in message.fields:
        # Process field
```

### Building Names

```python
from nanopb_generator import Names

# Hierarchical naming
names = Names(('Package', 'Message', 'Field'))
c_identifier = str(names)  # 'Package_Message_Field'

# Add components
names = names + 'Submessage'
```

### Checking Options

```python
# Check if option is set
if hasattr(field, 'max_size') and field.max_size:
    # Handle max_size option

# Get option with default
max_count = getattr(field, 'max_count', 0)
```

## Performance Considerations

1. **Generator Speed:**
   - Cached descriptor parsing
   - Minimal file I/O
   - Usually completes in <1 second

2. **Generated Code Size:**
   - Minimize field descriptors
   - Share submessage tables
   - Remove unused features

3. **Generated Code Quality:**
   - Use const for read-only data
   - Generate comments for readability
   - Follow C naming conventions

## Further Reading

- [VALIDATOR_ARCHITECTURE.md](VALIDATOR_ARCHITECTURE.md) - Validation generator
- [../../ARCHITECTURE.md](../../ARCHITECTURE.md) - Overall architecture
- [../concepts.md](../concepts.md) - Generator usage guide
- [../reference.md](../reference.md) - Generated code reference

## Contributing

When modifying the generator:
1. Maintain backward compatibility
2. Add test cases for new features
3. Update documentation
4. Follow existing code style
5. Test on multiple platforms

---

**Note:** This document describes nanopb 1.0.0-dev. Earlier versions may differ.
