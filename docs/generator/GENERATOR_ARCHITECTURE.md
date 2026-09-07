# Nanopb Generator Architecture

This document describes the internal architecture of `nanopb_generator.py`, the tool that converts Protocol Buffer definitions (`.proto` files) into C code optimized for embedded systems.

## Table of Contents

- [Overview](#overview)
- [Execution Flow](#execution-flow)
- [Proto Data Extraction: From .proto to Python Objects](#proto-data-extraction-from-proto-to-python-objects)
- [How Nanopb Handles Custom Options](#how-nanopb-handles-custom-options)
- [Adding Your Own Custom Options](#adding-your-own-custom-options)
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

## Proto Data Extraction: From .proto to Python Objects

Understanding how `.proto` files become Python objects is fundamental to working with the generator.

### Step 1: Protoc Parses .proto Files

When you run the generator, it first invokes Google's `protoc` compiler to parse your `.proto` files:

```python
# From main_cli() - simplified
args = ["protoc"] + include_path
args += ['--include_imports', '--include_source_info', '-o' + tmpname, filename]
status = invoke_protoc(args)  # Calls protoc via grpcio-tools or system binary
```

The `invoke_protoc()` function (in `proto/_utils.py`) handles the protoc invocation:

```python
def invoke_protoc(argv):
    """
    Invoke protoc using grpcio-tools if available, 
    otherwise fall back to system protoc.
    """
    # Add include paths for standard protos
    for incpath in get_proto_builtin_include_path():
        argv.append('-I' + incpath)

    if has_grpcio_protoc():
        import grpc_tools.protoc as protoc
        return protoc.main(argv)
    else:
        return subprocess.call(argv)
```

**Output:** Protoc produces a binary `FileDescriptorSet` (a serialized protobuf message describing your `.proto` file).

### Step 2: Load FileDescriptorSet into Python

The binary descriptor is loaded using Google's protobuf Python library:

```python
# Load the binary descriptor
data = open(tmpname, 'rb').read()

# Parse into Python objects using descriptor_pb2
fdescs = descriptor.FileDescriptorSet.FromString(data).file
```

The `FileDescriptorSet` is defined in `google/protobuf/descriptor.proto` and contains:
- `FileDescriptorProto` for each `.proto` file
- `MessageDescriptorProto` for each message
- `FieldDescriptorProto` for each field
- `EnumDescriptorProto` for each enum

### Step 3: Python Descriptor Objects

After parsing, you have Python objects representing your proto definitions:

```python
# fdesc is a FileDescriptorProto
fdesc.name           # "myfile.proto"
fdesc.package        # "mypackage"
fdesc.message_type   # List of DescriptorProto (messages)
fdesc.enum_type      # List of EnumDescriptorProto (enums)
fdesc.options        # FileOptions (including custom extensions)

# For each message
for msg in fdesc.message_type:
    msg.name         # "MyMessage"
    msg.field        # List of FieldDescriptorProto
    msg.nested_type  # Nested messages
    msg.enum_type    # Nested enums
    msg.options      # MessageOptions

# For each field
for field in msg.field:
    field.name       # "my_field"
    field.number     # 1 (tag number)
    field.type       # TYPE_INT32, TYPE_STRING, etc.
    field.label      # LABEL_OPTIONAL, LABEL_REQUIRED, LABEL_REPEATED
    field.options    # FieldOptions (including custom extensions)
    field.default_value  # Default value as string
```

### Step 4: Nanopb Wraps Descriptors in Custom Classes

The generator creates nanopb-specific objects from the descriptors:

```python
# In parse_file()
file_options = get_nanopb_suboptions(fdesc, toplevel_options, Names([filename]))
f = ProtoFile(fdesc, file_options)

# ProtoFile creates Message, Field, Enum objects
# Each wraps the corresponding descriptor + nanopb options
```

**Key relationship:**
```
FileDescriptorProto (Google)  →  ProtoFile (Nanopb)
DescriptorProto (Google)      →  Message (Nanopb)
FieldDescriptorProto (Google) →  Field (Nanopb)
EnumDescriptorProto (Google)  →  Enum (Nanopb)
```

## How Nanopb Handles Custom Options

### Default Values: The `[default = ...]` Option

Default values in Protocol Buffers are handled through the `FieldDescriptorProto.default_value` field:

```protobuf
// In your .proto file
message Config {
    optional int32 timeout = 1 [default = 30];
    optional string name = 2 [default = "unnamed"];
    optional MyEnum status = 3 [default = STATUS_OK];
}
```

**How the generator processes defaults:**

```python
# In Field.__init__()
if desc.HasField('default_value'):
    self.default = desc.default_value  # Stored as string

# In Field.get_initializer() - generates C initialization code
def get_initializer(self, null_init, inner_init_only=False):
    '''Return literal expression for this field's default value.'''
    if self.default is None or null_init:
        if self.pbtype == 'STRING':
            return '""'
        elif self.pbtype in ('ENUM', 'UENUM'):
            return '_%s_MIN' % self.ctype
        else:
            return '0'
    else:
        if self.pbtype == 'STRING':
            # Escape string for C
            data = codecs.escape_encode(self.default.encode('utf-8'))[0]
            return '"' + data.decode('ascii') + '"'
        elif self.pbtype in ('ENUM', 'UENUM'):
            # Use enum value name
            return Globals.naming_style.enum_entry(self.default)
        elif self.pbtype in ['FIXED32', 'UINT32']:
            return str(self.default) + 'u'
        else:
            # Other numeric types
            return str(self.default)
```

**Generated C code:**
```c
// In .pb.h
#define Config_init_default { 30, "unnamed", STATUS_OK }
#define Config_init_zero { 0, "", 0 }
```

### The Nanopb Options System

Nanopb extends protobuf with custom options defined in `nanopb.proto`:

```protobuf
// generator/proto/nanopb.proto
extend google.protobuf.FieldOptions {
    optional NanoPBOptions nanopb = 1010;
}

extend google.protobuf.MessageOptions {
    optional NanoPBOptions nanopb_msgopt = 1010;
}

extend google.protobuf.FileOptions {
    optional NanoPBOptions nanopb_fileopt = 1010;
}
```

**Using nanopb options in your .proto:**
```protobuf
import "nanopb.proto";

message User {
    // Field-level option
    required string name = 1 [(nanopb).max_size = 40];
    
    // Using max_length (equivalent to max_size + 1 for strings)
    optional string email = 2 [(nanopb).max_length = 100];
    
    // Repeated field with max count
    repeated int32 scores = 3 [(nanopb).max_count = 10];
    
    // Force callback allocation
    optional bytes data = 4 [(nanopb).type = FT_CALLBACK];
}

// Message-level option
message Config {
    option (nanopb_msgopt).msgid = 42;
}

// File-level option
option (nanopb_fileopt).long_names = false;
```

### How Options Are Extracted and Merged

The `get_nanopb_suboptions()` function handles option extraction:

```python
def get_nanopb_suboptions(subdesc, options, name):
    '''Get copy of options, and merge information from subdesc.'''
    # Start with copy of parent options
    new_options = nanopb_pb2.NanoPBOptions()
    new_options.CopyFrom(options)

    # Check proto3 syntax
    if hasattr(subdesc, 'syntax') and subdesc.syntax == "proto3":
        new_options.proto3 = True

    # Merge options from separate .options file
    dotname = '.'.join(name.parts)
    for namemask, file_options in Globals.separate_options:
        if fnmatchcase(dotname, namemask):
            new_options.MergeFrom(file_options)

    # Merge options from .proto annotations
    if isinstance(subdesc.options, descriptor.FieldOptions):
        ext_type = nanopb_pb2.nanopb
    elif isinstance(subdesc.options, descriptor.FileOptions):
        ext_type = nanopb_pb2.nanopb_fileopt
    elif isinstance(subdesc.options, descriptor.MessageOptions):
        ext_type = nanopb_pb2.nanopb_msgopt
    # ...

    if subdesc.options.HasExtension(ext_type):
        ext = subdesc.options.Extensions[ext_type]
        new_options.MergeFrom(ext)  # Proto annotations override file options

    return new_options
```

**Option priority (highest to lowest):**
1. Field-level annotations in `.proto`
2. Message-level annotations in `.proto`
3. Options in separate `.options` file
4. File-level annotations in `.proto`
5. Command-line options
6. Default values

### The .options File Format

For large projects, you can use a separate `.options` file:

```
# myfile.options
# Format: field_pattern option:value [option:value ...]

# Apply to specific field
MyMessage.name              max_size:64
MyMessage.items             max_count:100

# Wildcards supported
*.timestamp                 int_size:IS_32
MyPackage.*                 long_names:false

# Multiple options
MyMessage.data              type:FT_CALLBACK callback_datatype:"custom_cb_t"
```

The file is parsed by `read_options_file()`:

```python
def read_options_file(infile):
    results = []
    for line in data.split('\n'):
        parts = line.split(None, 1)  # Split on whitespace
        opts = nanopb_pb2.NanoPBOptions()
        text_format.Merge(parts[1], opts)  # Parse as text protobuf
        results.append((parts[0], opts))   # (pattern, options)
    return results
```

## Adding Your Own Custom Options

### Method 1: Extend nanopb.proto (Recommended for Nanopb Features)

If you're adding features to nanopb itself:

1. **Add to NanoPBOptions in `generator/proto/nanopb.proto`:**
   ```protobuf
   message NanoPBOptions {
       // ... existing options ...
       
       // Your new option
       optional bool my_custom_option = 100;  // Use high number
   }
   ```

2. **Regenerate nanopb_pb2.py:**
   ```bash
   cd generator/proto
   protoc --python_out=. nanopb.proto
   ```

3. **Process in generator:**
   ```python
   # In Field.__init__() or wherever appropriate
   if field_options.HasField("my_custom_option"):
       self.my_custom_option = field_options.my_custom_option
   ```

### Method 2: Create Your Own Extension (For External Tools)

For options used by your own tools (not nanopb itself):

1. **Create your options proto:**
   ```protobuf
   // myoptions.proto
   syntax = "proto2";
   import "google/protobuf/descriptor.proto";

   message MyOptions {
       optional string custom_annotation = 1;
       optional int32 custom_size = 2;
   }

   extend google.protobuf.FieldOptions {
       optional MyOptions myopt = 50000;  // Get a unique extension number
   }
   ```

2. **Use in your .proto files:**
   ```protobuf
   import "myoptions.proto";

   message MyMessage {
       string field = 1 [(myopt).custom_annotation = "special"];
   }
   ```

3. **Access in your Python tool:**
   ```python
   import myoptions_pb2  # Generated from myoptions.proto
   
   for field in message.field:
       if field.options.HasExtension(myoptions_pb2.myopt):
           my_opts = field.options.Extensions[myoptions_pb2.myopt]
           print(f"Custom annotation: {my_opts.custom_annotation}")
   ```

### Method 3: Using Unknown Fields (Advanced)

For truly custom extensions that nanopb doesn't need to understand:

```python
# Access raw extension data
if field.options.HasExtension(my_extension):
    ext = field.options.Extensions[my_extension]
    # Process extension data
```

### Best Practices for Custom Options

1. **Register your extension number** at [protobuf global extension registry](https://github.com/protocolbuffers/protobuf/blob/main/docs/options.md) for public extensions

2. **Use high extension numbers** (50000+) for private/internal extensions

3. **Document your options** with comments in the proto file

4. **Provide defaults** for optional behavior:
   ```protobuf
   optional bool my_option = 1 [default = false];
   ```

5. **Test option inheritance** - ensure options cascade correctly from file → message → field levels

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
