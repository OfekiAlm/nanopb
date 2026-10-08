# Protocol Buffers Fundamentals

## What Are Protocol Buffers?

Protocol Buffers (protobuf) is a language-neutral, platform-neutral, extensible mechanism for serializing structured data. Developed by Google, it provides a way to define data structures once and generate code to read and write those structures in multiple programming languages.

Think of Protocol Buffers as a more efficient, strictly-typed alternative to JSON or XML for data interchange.

## The Core Concept

Protocol Buffers work in three steps:

```
1. Define your data structure in a .proto file
   ↓
2. Compile the .proto file to generate code
   ↓
3. Use the generated code to serialize/deserialize data
```

### Example: Defining a Message

```protobuf
// person.proto
syntax = "proto3";

message Person {
    string name = 1;
    int32 age = 2;
    string email = 3;
}
```

This simple definition can be compiled into C, C++, Java, Python, Go, and many other languages. Each language gets type-safe code for working with `Person` objects.

## Why Use Protocol Buffers?

### 1. Compact Binary Format

Protocol Buffers serialize data into a compact binary format that is typically **3-10x smaller** than JSON or XML.

**Example comparison:**

```json
// JSON: 62 bytes
{"name":"Alice","age":30,"email":"alice@example.com"}
```

```
// Protobuf: ~35 bytes (binary, shown as hex)
0a 05 41 6c 69 63 65 10 1e 1a 11 61 6c 69 63 65
40 65 78 61 6d 70 6c 65 2e 63 6f 6d
```

This size reduction matters significantly in:
- Embedded systems with limited bandwidth
- Network protocols where every byte counts
- Storage systems with capacity constraints
- Battery-powered devices conserving energy

### 2. Fast Serialization and Parsing

Binary encoding/decoding is computationally simpler than text parsing:
- No string-to-number conversions
- No parsing of nested structures with braces/brackets
- Direct memory mapping in many cases
- Typically **5-100x faster** than JSON parsing

### 3. Strongly Typed

Unlike JSON, protobuf enforces types at compile time:

```protobuf
message Config {
    int32 timeout = 1;        // Must be an integer
    bool enabled = 2;         // Must be true/false
    repeated string tags = 3; // Must be array of strings
}
```

This eliminates entire classes of runtime errors:
- No "undefined is not a function"
- No type coercion bugs
- No silent failures from misspelled field names

### 4. Schema Evolution

Protocol Buffers are designed for forward and backward compatibility. You can evolve your data structures over time without breaking existing code:

**Version 1:**
```protobuf
message Device {
    string id = 1;
    string name = 2;
}
```

**Version 2 (adds field):**
```protobuf
message Device {
    string id = 1;
    string name = 2;
    string location = 3;  // New field
}
```

Old code reading new messages ignores unknown fields. New code reading old messages gets default values for missing fields.

### 5. Language Interoperability

Define your protocol once, use it everywhere:

```
     person.proto
          ↓
    ┌─────┴─────┐
    ↓           ↓
Python Client   C Server
(IoT device)    (Gateway)
```

The Python code and C code can exchange data seamlessly because they both implement the same protocol specification.

## What Protocol Buffers Replace

### Instead of JSON

**JSON drawbacks:**
- Text-based: larger message size
- Loose typing: runtime errors
- No schema: field names repeated in every message
- Parsing overhead: string parsing is slow

**Protobuf advantages:**
- Binary: 3-10x smaller
- Strongly typed: compile-time safety
- Schema separate from data: more compact
- Fast: direct binary encoding

**When to use JSON:**
- Human readability is important
- Debugging and development
- Web APIs where JavaScript parsing is needed
- Simple configurations

**When to use Protobuf:**
- Performance matters
- Bandwidth is limited
- Strong typing is required
- Cross-language systems

### Instead of XML

**XML drawbacks:**
- Very verbose (opening and closing tags)
- Slow to parse
- Complex specifications (namespaces, attributes, etc.)
- Large memory footprint

**Protobuf advantages:**
- 10-100x smaller than XML
- Much faster parsing
- Simpler specification
- Minimal memory usage

### Instead of Ad-Hoc Binary Formats

Many embedded systems use custom binary formats:

```c
// Custom format - fragile and error-prone
struct packet {
    uint8_t type;
    uint16_t length;
    uint8_t data[256];
} __attribute__((packed));
```

**Problems with custom formats:**
- Manual serialization code (error-prone)
- No versioning strategy
- Endianness issues
- Alignment problems
- No language interoperability

**Protobuf advantages:**
- Generated code (no manual serialization)
- Built-in versioning
- Handles endianness automatically
- Portable across platforms
- Multi-language support

## Core Protobuf Concepts

### Field Numbers

Each field has a unique number:

```protobuf
message Example {
    string name = 1;   // Field number 1
    int32 value = 2;   // Field number 2
}
```

**Important:** Field numbers are permanent identifiers. Never reuse a field number for a different purpose.

### Field Types

Protocol Buffers support various types:

**Scalar types:**
- `int32`, `int64`, `uint32`, `uint64` - Integers
- `sint32`, `sint64` - Signed integers (optimized for negative numbers)
- `fixed32`, `fixed64` - Fixed-size integers (4 or 8 bytes)
- `bool` - Boolean
- `string` - UTF-8 text
- `bytes` - Raw binary data
- `float`, `double` - Floating point

**Complex types:**
- `message` - Nested messages
- `enum` - Enumeration
- `repeated` - Arrays/lists
- `map` - Key-value pairs

### Optional vs Required Fields

**Proto3 (recommended):**
All fields are optional by default. Missing fields get default values (0, false, empty string, etc.).

```protobuf
syntax = "proto3";

message User {
    string name = 1;    // Optional, defaults to ""
    int32 age = 2;      // Optional, defaults to 0
}
```

**Proto2 (legacy):**
Explicit `optional` and `required` keywords.

### Nested Messages

Messages can contain other messages:

```protobuf
message Address {
    string street = 1;
    string city = 2;
    string country = 3;
}

message Person {
    string name = 1;
    Address home_address = 2;
    Address work_address = 3;
}
```

### Repeated Fields (Arrays)

Use `repeated` for lists:

```protobuf
message Team {
    string name = 1;
    repeated string members = 2;  // Array of strings
}
```

### Enumerations

Define fixed sets of values:

```protobuf
enum Status {
    UNKNOWN = 0;
    ACTIVE = 1;
    INACTIVE = 2;
    SUSPENDED = 3;
}

message Account {
    string id = 1;
    Status status = 2;
}
```

## The Wire Format

Protocol Buffers use a binary wire format based on field numbers and types:

```
[field_number << 3 | wire_type][value]
```

**Example encoding:**

```protobuf
message Test {
    int32 value = 1;
}
```

Setting `value = 150` encodes to:
```
08 96 01
```

Where:
- `08` = field number 1, wire type 0 (varint)
- `96 01` = 150 encoded as varint

This compact encoding is why protobuf is so efficient.

## Practical Workflow

### 1. Write the .proto file

```protobuf
// sensor.proto
syntax = "proto3";

message SensorReading {
    string sensor_id = 1;
    float temperature = 2;
    float humidity = 3;
    int64 timestamp = 4;
}
```

### 2. Generate code

```bash
# For C/C++
protoc --cpp_out=. sensor.proto

# For Python
protoc --python_out=. sensor.proto

# For nanopb (C for embedded)
python generator/nanopb_generator.py sensor.proto
```

### 3. Use generated code

**Python example:**
```python
from sensor_pb2 import SensorReading

# Create and populate
reading = SensorReading()
reading.sensor_id = "TEMP-001"
reading.temperature = 23.5
reading.humidity = 45.2
reading.timestamp = 1234567890

# Serialize to binary
data = reading.SerializeToString()

# Deserialize
reading2 = SensorReading()
reading2.ParseFromString(data)
```

**C example (with nanopb):**
```c
#include "sensor.pb.h"

SensorReading reading = SensorReading_init_zero;
reading.temperature = 23.5;
reading.humidity = 45.2;

// Encode to buffer
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
pb_encode(&stream, SensorReading_fields, &reading);
```

## Key Takeaways

1. **Protocol Buffers are a serialization format** - like JSON but binary, typed, and efficient
2. **Define once, use everywhere** - one `.proto` file generates code for multiple languages
3. **Smaller and faster** - compact binary format with fast encoding/decoding
4. **Type-safe** - compile-time checking prevents runtime errors
5. **Evolvable** - designed for backward and forward compatibility
6. **Ideal for constrained environments** - perfect for embedded systems, IoT, and network protocols

## Next Steps

- **[Why Nanopb?](WHY_NANOPB.md)** - Learn why nanopb is the ideal protobuf implementation for embedded systems
- **[Architecture & Data Flow](ARCHITECTURE.md)** - Understand how nanopb's tools work together
- **[Using nanopb_generator](USING_NANOPB_GENERATOR.md)** - Start generating C code from your `.proto` files
