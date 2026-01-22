# Project Architecture & Data Flow

## Overview

This document describes the high-level architecture of the nanopb tooling ecosystem and how data flows through the system from `.proto` definitions to runtime validation.

## The Big Picture

```
┌─────────────┐
│ .proto file │  Define your message structure
└──────┬──────┘
       │
       ↓
┌─────────────────────┐
│ nanopb_generator.py │  Generate C code
└──────┬──────────────┘
       │
       ├──→ message.pb.h    (Structures and declarations)
       ├──→ message.pb.c    (Field descriptors)
       ├──→ message.val.h   (Validation declarations, if enabled)
       └──→ message.val.c   (Validation implementation, if enabled)
       │
       ↓
┌──────────────────────┐
│   Your Application   │  Encode/decode/validate messages
│  + pb_encode.c       │
│  + pb_decode.c       │
│  + pb_validate.c     │
└──────────────────────┘
```

## The Three Core Tools

### 1. nanopb_generator.py - Code Generator

**Purpose:** Converts `.proto` files into C code that can encode and decode messages.

**Input:**
- `.proto` files (Protocol Buffer definitions)
- `.options` files (optional - memory allocation hints)
- Command-line flags (optional features like validation)

**Output:**
- `.pb.h` - C header with struct definitions
- `.pb.c` - Field descriptors and metadata

**What it does:**
1. Invokes `protoc` to parse `.proto` files into a FileDescriptorSet
2. Analyzes message structures and field types
3. Determines memory layouts based on `.options` file or defaults
4. Generates C struct definitions with appropriate sizes
5. Creates field descriptor arrays for runtime encoding/decoding
6. Optionally generates validation code (if `--validate` flag is used)

**Example:**
```bash
python generator/nanopb_generator.py sensor.proto
# Creates: sensor.pb.h, sensor.pb.c
```

### 2. nanopb_validator.py - Validation Code Generator

**Purpose:** Generates C validation functions based on constraints defined in `.proto` files.

**Input:**
- Parsed `.proto` files with validation rules from `validate.proto`
- Message and field descriptors

**Output:**
- `.val.h` - Validation function declarations
- `.val.c` - Validation implementation

**What it does:**
1. Extracts validation rules from field options (e.g., `(validate.rules).int32.gte = 0`)
2. Generates C code to check each constraint
3. Creates validation functions for each message type
4. Handles nested messages recursively
5. Provides detailed error reporting without dynamic allocation

**Example proto:**
```protobuf
import "validate.proto";

message Sensor {
    int32 temperature = 1 [(validate.rules).int32.gte = -40,
                           (validate.rules).int32.lte = 85];
}
```

**Generated validation function:**
```c
bool validate_Sensor(const Sensor *msg, pb_validation_result_t *result);
```

### 3. nanopb_data_generator.py - Test Data Generator

**Purpose:** Generates valid and invalid test data for messages with validation rules.

**Input:**
- `.proto` files with validation constraints
- Message type to generate data for
- Flags for valid vs invalid data

**Output:**
- Binary protobuf data
- C array initializers
- Hex strings
- Python dictionaries

**What it does:**
1. Parses validation rules from `.proto` files
2. For valid data: generates values that satisfy all constraints
3. For invalid data: generates values that violate specific constraints
4. Outputs in multiple formats for different use cases

**Example:**
```bash
# Generate valid data as C array
python generator/nanopb_data_generator.py sensor.proto Sensor

# Generate invalid data violating the temperature >= -40 rule
python generator/nanopb_data_generator.py sensor.proto Sensor \
    --invalid --field temperature --rule gte
```

## Complete Data Flow

Let's trace a message through the entire system:

### Step 1: Define Protocol

```protobuf
// sensor.proto
syntax = "proto3";
import "validate.proto";

message SensorReading {
    string sensor_id = 1 [(validate.rules).string.min_len = 1];
    float temperature = 2 [(validate.rules).float.gte = -40.0,
                           (validate.rules).float.lte = 85.0];
    int64 timestamp = 3;
}
```

### Step 2: Generate Code

```bash
python generator/nanopb_generator.py --validate sensor.proto
```

**Generated files:**

**sensor.pb.h:**
```c
typedef struct _SensorReading {
    char sensor_id[64];  // From .options or default
    float temperature;
    int64_t timestamp;
} SensorReading;

extern const pb_msgdesc_t SensorReading_msg;
```

**sensor.pb.c:**
```c
const pb_msgdesc_t SensorReading_msg = {
    3, // field count
    SensorReading_fields,
    sizeof(SensorReading),
    // ... field descriptors
};
```

**sensor.val.h:**
```c
bool validate_SensorReading(const SensorReading *msg, 
                           pb_validation_result_t *result);
```

**sensor.val.c:**
```c
bool validate_SensorReading(const SensorReading *msg, 
                           pb_validation_result_t *result) {
    // Check sensor_id min_len
    if (strlen(msg->sensor_id) < 1) {
        return false;
    }
    
    // Check temperature range
    if (msg->temperature < -40.0f || msg->temperature > 85.0f) {
        return false;
    }
    
    return true;
}
```

### Step 3: Use in Application

**Encoding:**
```c
#include "sensor.pb.h"

SensorReading reading = SensorReading_init_zero;
strcpy(reading.sensor_id, "TEMP-001");
reading.temperature = 23.5f;
reading.timestamp = 1234567890;

uint8_t buffer[128];
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
pb_encode(&stream, SensorReading_msg, &reading);

// buffer now contains binary protobuf data
size_t message_length = stream.bytes_written;
```

**Decoding:**
```c
SensorReading received = SensorReading_init_zero;
pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);
pb_decode(&stream, SensorReading_msg, &received);
```

**Validation:**
```c
pb_validation_result_t result = PB_VALIDATION_RESULT_INIT;

if (!validate_SensorReading(&received, &result)) {
    printf("Validation failed: %s\n", result.error_message);
    // Handle invalid data
} else {
    // Process valid data
}
```

### Step 4: Testing with Data Generator

```bash
# Generate valid test data
python generator/nanopb_data_generator.py sensor.proto SensorReading \
    --format c_array > test_data_valid.h

# Generate invalid test data (temperature too high)
python generator/nanopb_data_generator.py sensor.proto SensorReading \
    --invalid --field temperature --rule lte \
    --format c_array > test_data_invalid.h
```

**Generated test data:**
```c
// test_data_valid.h
const uint8_t valid_data[] = {
    0x0a, 0x08, 0x54, 0x45, 0x4d, 0x50, 0x2d, 0x30, 0x30, 0x31,  // sensor_id
    0x15, 0x00, 0x00, 0xbc, 0x41,  // temperature = 23.5
    0x18, 0xd2, 0x85, 0xd8, 0xcc, 0x04  // timestamp
};
```

## Component Interactions

### Generator → Runtime Libraries

```
nanopb_generator.py
        ↓
   Generates field descriptors
        ↓
   pb_encode.c / pb_decode.c
        ↓
   Read descriptors at runtime
        ↓
   Encode/decode messages
```

The generator creates field descriptor arrays that the runtime library uses to understand message structure. This separation allows the runtime library to be generic—it doesn't need to know about your specific message types.

### Validator → Runtime Validation

```
nanopb_validator.py
        ↓
   Analyzes validation rules
        ↓
   Generates validation functions
        ↓
   pb_validate.c
        ↓
   Provides validation macros/helpers
        ↓
   Generated .val.c uses helpers
```

The validator generates standalone validation functions that can be called independently of encoding/decoding. This allows validation at any point in your application.

### Data Generator → Testing

```
nanopb_data_generator.py
        ↓
   Reads validation rules
        ↓
   Generates test data
        ↓
   Output in multiple formats
        ↓
   Used in unit tests / fuzzing
```

The data generator creates test data automatically, ensuring your validation code is exercised with both valid and invalid inputs.

## File Organization

```
nanopb/
├── pb.h                    # Core nanopb types
├── pb_encode.c/h           # Encoding runtime
├── pb_decode.c/h           # Decoding runtime
├── pb_common.c/h           # Shared utilities
├── pb_validate.c/h         # Validation runtime
│
├── generator/
│   ├── nanopb_generator.py   # Main code generator
│   ├── nanopb_validator.py   # Validation code generator
│   ├── nanopb_data_generator.py  # Test data generator
│   └── proto/              # Internal proto definitions
│       ├── nanopb.proto    # Nanopb options
│       └── validate.proto  # Validation rules
│
└── examples/
    ├── simple/             # Basic encoding/decoding
    └── validation_simple/  # Validation example
```

## Memory Architecture

### Static Allocation Strategy

**Message structure in memory:**
```c
typedef struct {
    char name[32];           // Fixed-size buffer
    int32_t values[10];      // Fixed-size array
    pb_size_t values_count;  // Element count
    bool has_optional_field; // Presence flag
    int32_t optional_field;  // Optional value
} ExampleMessage;
```

**Key points:**
- All memory allocated at compile time
- No malloc/free calls
- Stack or static storage
- Size known before runtime

### Streaming for Large Data

For data too large for fixed buffers, nanopb uses callbacks:

```c
// Callback-based field
typedef struct {
    pb_callback_t large_data;  // Function pointer + arg
} StreamingMessage;

bool read_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    // Process data in chunks
    return true;
}
```

This allows handling arbitrarily large data without allocating large buffers.

## Validation Architecture

### Two-Phase Design

**Phase 1: Code Generation**
- Parse validation rules at build time
- Generate C validation functions
- Compile validation code into firmware

**Phase 2: Runtime Execution**
- Call validation functions on messages
- Get immediate pass/fail result
- Optional detailed error reporting

### Validation Strategies

**1. Pre-encoding validation:**
```c
// Validate before encoding
if (validate_Message(&msg, &result)) {
    pb_encode(&stream, Message_msg, &msg);
}
```

**2. Post-decoding validation:**
```c
// Validate after decoding
pb_decode(&stream, Message_msg, &msg);
if (!validate_Message(&msg, &result)) {
    // Reject invalid incoming message
}
```

**3. Business logic validation:**
```c
// Validate at any point in application
Message msg = create_message();
if (validate_Message(&msg, &result)) {
    process_message(&msg);
}
```

## Design Principles

### 1. Separation of Concerns

Each tool has a single responsibility:
- Generator: .proto → C code
- Validator: Validation rules → C validation code
- Data generator: Validation rules → Test data

### 2. Compile-Time Over Runtime

Decisions made at code generation time:
- Memory layouts
- Field sizes
- Validation logic structure

This minimizes runtime overhead.

### 3. No Hidden Dependencies

Generated code explicitly includes only what it needs:
- Field descriptors are self-contained
- Validation code includes validation helpers
- No global state or hidden initialization

### 4. Predictable Performance

- No recursive algorithms with unbounded depth
- No dynamic allocation
- Fixed-size buffers
- Bounded loops

### 5. Extensibility

- Options system for customization
- Plugin-based protoc integration
- Callback system for streaming
- User-defined validation extensions

## Next Steps

Now that you understand the architecture:

- **[Using nanopb_generator](USING_NANOPB_GENERATOR.md)** - Start generating code
- **[Using nanopb_data_generator](USING_NANOPB_DATA_GENERATOR.md)** - Generate test data
- **[Developer Guide](DEVELOPER_GUIDE.md)** - Contribute to the project
- **[Validation Guide](validation.md)** - Add validation to your messages
