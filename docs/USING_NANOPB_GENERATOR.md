# Using nanopb_generator

## Overview

The `nanopb_generator.py` tool converts Protocol Buffer `.proto` files into C code optimized for embedded systems. This guide covers typical usage patterns, options, and customization.

## Basic Usage

### Simple Code Generation

```bash
# Generate C code from a .proto file
python generator/nanopb_generator.py message.proto

# Output:
#   message.pb.h  - Header with struct definitions
#   message.pb.c  - Field descriptors and metadata
```

### With Validation

```bash
# Generate code with validation functions
python generator/nanopb_generator.py --validate message.proto

# Additional output:
#   message.val.h - Validation declarations
#   message.val.c - Validation implementation
```

### Using with protoc

```bash
# As a protoc plugin
protoc --nanopb_out=. message.proto

# With options
protoc --nanopb_out=. --nanopb_opt=--validate message.proto
```

## Input Files

### .proto Files

Standard Protocol Buffer definition files:

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

**Requirements:**
- Must use `syntax = "proto3"` or `syntax = "proto2"`
- Follow standard Protocol Buffers syntax
- Can import other `.proto` files
- Can use nanopb-specific options

### .options Files

Control memory allocation and code generation:

```
# sensor.options

# Set maximum string length
SensorReading.sensor_id max_size:16

# Set maximum array size
SensorReading.tags max_count:5

# Make field use a callback instead of static allocation
SensorReading.large_data type:FT_CALLBACK
```

**Key points:**
- Optional file, same base name as `.proto`
- One setting per line
- Applies to specific messages and fields
- Controls memory allocation strategy

## Command-Line Options

### Common Options

```bash
# Specify output directory
python generator/nanopb_generator.py -D output/ message.proto

# Include paths for imported .proto files
python generator/nanopb_generator.py -I proto -I third_party message.proto

# Quiet mode (less verbose)
python generator/nanopb_generator.py -q message.proto

# Enable validation
python generator/nanopb_generator.py --validate message.proto

# Specify .options file explicitly
python generator/nanopb_generator.py --options-file=custom.options message.proto
```

### Advanced Options

```bash
# Generate header only (no .pb.c)
python generator/nanopb_generator.py --header-only message.proto

# Use callback fields by default
python generator/nanopb_generator.py --default-callback message.proto

# Add strip prefix for type names
python generator/nanopb_generator.py --strip-prefix=MyCompany_ message.proto

# Generate with extension support
python generator/nanopb_generator.py --extension message.proto
```

### Help

```bash
# See all available options
python generator/nanopb_generator.py --help
```

## Output Files

### .pb.h Header File

Contains C struct definitions for your messages:

```c
// sensor.pb.h

typedef struct _SensorReading {
    char sensor_id[16];      // Fixed-size string buffer
    float temperature;
    float humidity;
    int64_t timestamp;
    bool has_timestamp;      // Optional field presence
} SensorReading;

// Initialization macro
#define SensorReading_init_zero {0, 0, 0, 0, false}

// Field descriptors for runtime
extern const pb_msgdesc_t SensorReading_msg;

// Size macros
#define SensorReading_size 64
```

### .pb.c Source File

Contains field descriptor arrays:

```c
// sensor.pb.c

PB_BIND(SensorReading, SensorReading, AUTO)

const pb_msgdesc_t SensorReading_msg = {
    4,  // Number of fields
    SensorReading_default,
    SensorReading_fields,
    // ... more metadata
};
```

**These descriptors are used by:**
- `pb_encode()` - To serialize messages
- `pb_decode()` - To deserialize messages
- Runtime library - To navigate message structure

## Memory Allocation Strategies

### 1. Static Arrays (Default)

**For strings:**
```protobuf
message Device {
    string name = 1;
}
```

**Options file:**
```
Device.name max_size:32
```

**Generated code:**
```c
typedef struct {
    char name[32];  // Fixed-size buffer
} Device;
```

**Use when:** Size is known and reasonable.

### 2. Static Arrays with Count (Repeated Fields)

**For repeated fields:**
```protobuf
message Sensor {
    repeated int32 values = 1;
}
```

**Options file:**
```
Sensor.values max_count:10
```

**Generated code:**
```c
typedef struct {
    int32_t values[10];
    pb_size_t values_count;  // Actual count
} Sensor;
```

**Use when:** Maximum element count is known.

### 3. Callbacks (For Large or Variable Data)

**Options file:**
```
Sensor.large_data type:FT_CALLBACK
```

**Generated code:**
```c
typedef struct {
    pb_callback_t large_data;  // Function pointer + arg
} Sensor;

// User provides callback
bool read_large_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    // Read data in chunks
    return true;
}
```

**Use when:**
- Data is too large for static buffer
- Streaming processing needed
- Size unknown at compile time

### 4. Pointers (Optional Messages)

**Options file:**
```
Container.optional_message type:FT_POINTER
```

**Generated code:**
```c
typedef struct {
    SubMessage *optional_message;  // NULL if not present
} Container;
```

**Use when:**
- Submessage is large and often absent
- Need to save memory when field not used
- Can allocate from a static pool

## Common Patterns

### Pattern 1: Simple Device Protocol

**Use case:** Small IoT device sending sensor readings.

**sensor.proto:**
```protobuf
syntax = "proto3";

message Reading {
    string device_id = 1;
    float value = 2;
    int64 timestamp = 3;
}
```

**sensor.options:**
```
Reading.device_id max_size:16
```

**Generation:**
```bash
python generator/nanopb_generator.py sensor.proto
```

**Result:** Compact struct using ~30 bytes.

### Pattern 2: Configuration Storage

**Use case:** Store device configuration in EEPROM.

**config.proto:**
```protobuf
syntax = "proto3";

message DeviceConfig {
    string wifi_ssid = 1;
    string wifi_password = 2;
    int32 update_interval = 3;
    repeated string allowed_hosts = 4;
}
```

**config.options:**
```
DeviceConfig.wifi_ssid max_size:32
DeviceConfig.wifi_password max_size:64
DeviceConfig.allowed_hosts max_count:5
DeviceConfig.allowed_hosts max_size:64
```

### Pattern 3: Telemetry with Validation

**Use case:** Validated sensor data.

**telemetry.proto:**
```protobuf
syntax = "proto3";
import "validate.proto";

message Telemetry {
    int32 battery_percent = 1 [(validate.rules).int32.gte = 0,
                                (validate.rules).int32.lte = 100];
    float temperature = 2 [(validate.rules).float.gte = -40.0,
                           (validate.rules).float.lte = 85.0];
}
```

**Generation:**
```bash
python generator/nanopb_generator.py --validate telemetry.proto
```

**Result:** Struct + validation functions.

### Pattern 4: Streaming Large Data

**Use case:** Firmware update over network.

**firmware.proto:**
```protobuf
syntax = "proto3";

message FirmwareChunk {
    int32 sequence = 1;
    bytes data = 2;  // Large, variable-size
}
```

**firmware.options:**
```
FirmwareChunk.data type:FT_CALLBACK
```

**Usage:**
```c
bool write_firmware_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    uint8_t buffer[256];
    while (stream->bytes_left) {
        size_t read = stream->bytes_left < sizeof(buffer) ? 
                      stream->bytes_left : sizeof(buffer);
        if (!pb_read(stream, buffer, read))
            return false;
        
        // Write to flash
        write_flash(buffer, read);
    }
    return true;
}
```

## Customization with .options

### Field-Level Options

```
# String maximum length
MessageType.field_name max_size:64

# Array maximum count
MessageType.repeated_field max_count:10

# Use callback instead of static allocation
MessageType.large_field type:FT_CALLBACK

# Use pointer for optional submessage
MessageType.optional_msg type:FT_POINTER

# Set default value
MessageType.field_name default:"default_value"

# Mark field as fixed length (not null-terminated string)
MessageType.byte_field fixed_length:true
```

### Message-Level Options

```
# Apply to all fields in message
MessageType.* max_size:32

# Make all submessages use pointers
MessageType.* type:FT_POINTER

# Enable validation
* validate:true
```

### File-Level Options

```
# Apply to all messages in file
* msgid:uint32_t

# Set default string size
* max_size:64
```

## Integration with Build Systems

### Makefile

```makefile
# Generate during build
%.pb.c %.pb.h: %.proto
	python generator/nanopb_generator.py $<

# Include generated files
SOURCES += message.pb.c
```

### CMake

```cmake
# Find nanopb
find_package(Nanopb REQUIRED)

# Generate code
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS message.proto)

# Add to target
add_executable(app main.c ${PROTO_SRCS})
```

See `examples/cmake_simple` for complete example.

### SCons

```python
# Use nanopb builder
env = Environment()
env.Tool('nanopb', toolpath=['path/to/nanopb/extra'])

# Generate from .proto
env.NanopbProto(['message.proto'])
```

### Bazel

```python
# BUILD.bazel
load("@nanopb//bazel:nanopb.bzl", "nanopb_proto_library")

nanopb_proto_library(
    name = "message_nanopb",
    deps = [":message_proto"],
)
```

## Troubleshooting

### Import Errors

**Problem:** `proto file not found`

**Solution:** Add include paths with `-I`:
```bash
python generator/nanopb_generator.py -I proto -I vendor message.proto
```

### Size Errors

**Problem:** `error: field exceeds maximum size`

**Solution:** Increase size in `.options`:
```
Message.field max_size:128
```

Or use callback:
```
Message.field type:FT_CALLBACK
```

### Missing Dependencies

**Problem:** `ImportError: No module named 'google.protobuf'`

**Solution:** Install dependencies:
```bash
pip install protobuf grpcio-tools
```

### Protoc Version Mismatch

**Problem:** `protoc version too old`

**Solution:** Update protoc:
```bash
pip install --upgrade grpcio-tools
```

Minimum required version: protoc 3.6+

## Best Practices

### 1. Always Use .options Files

Even for defaults, document your intent:
```
# sensor.options
Sensor.id max_size:16  # Device ID format: "SENSOR-XXXX"
```

### 2. Choose Appropriate Sizes

Don't over-allocate:
```
# Bad: Wastes memory
Message.name max_size:1024

# Good: Fits typical use case
Message.name max_size:32
```

### 3. Use Callbacks for Large Data

Don't use huge static buffers:
```
# Bad: 10KB buffer
Message.large_data max_size:10240

# Good: Callback-based streaming
Message.large_data type:FT_CALLBACK
```

### 4. Validate Early

Generate with validation for data from external sources:
```bash
python generator/nanopb_generator.py --validate external_api.proto
```

### 5. Version Your .proto Files

Track changes carefully:
```protobuf
// Version 1.2.0
// Added: optional_field (field 5)
// Deprecated: legacy_field (field 3)
```

## Next Steps

- **[Validation Guide](validation.md)** - Add validation to your messages
- **[Using nanopb_data_generator](USING_NANOPB_DATA_GENERATOR.md)** - Generate test data
- **[Concepts](concepts.md)** - Understand encoding/decoding
- **[Reference Manual](reference.md)** - Complete option reference
