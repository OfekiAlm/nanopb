# Complete Guide to Repeated Submessages in nanopb

**A Comprehensive Tutorial for Beginners**

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [What Are Repeated Submessages?](#2-what-are-repeated-submessages)
3. [Protocol Buffers Basics](#3-protocol-buffers-basics)
4. [How nanopb Handles Repeated Submessages](#4-how-nanopb-handles-repeated-submessages)
5. [Encoding Repeated Submessages](#5-encoding-repeated-submessages)
6. [Decoding Repeated Submessages](#6-decoding-repeated-submessages)
7. [Memory Management Strategies](#7-memory-management-strategies)
8. [Configuration and Options](#8-configuration-and-options)
9. [Complete Working Example](#9-complete-working-example)
10. [Validation of Repeated Submessages](#10-validation-of-repeated-submessages)
11. [Advanced Topics](#11-advanced-topics)
12. [Best Practices](#12-best-practices)
13. [Common Pitfalls and Troubleshooting](#13-common-pitfalls-and-troubleshooting)
14. [Real-World Examples](#14-real-world-examples)
15. [Performance Considerations](#15-performance-considerations)
16. [References](#16-references)

---

## 1. Introduction

### What is nanopb?

**nanopb** is a small, ANSI C implementation of Google's Protocol Buffers designed specifically for embedded systems and memory-constrained environments. Unlike the standard Protocol Buffers C++ library, nanopb:

- Has a **small code footprint** (typically 2-10 kB)
- Requires **minimal RAM** (configurable, often < 1 kB)
- Works without **dynamic memory allocation** (malloc-free by default)
- Uses **static code generation** for maximum efficiency
- Is **portable** across different platforms and compilers

### Why This Guide?

Understanding how to work with repeated submessages is crucial when building complex embedded applications that need to:
- Send lists of sensor readings
- Manage arrays of configuration objects
- Handle collections of events or logs
- Process batches of commands

This guide will teach you **everything** you need to know about repeated submessages in nanopb, from basic theory to advanced implementation details.

---

## 2. What Are Repeated Submessages?

### 2.1 Repeated Fields in Protocol Buffers

In Protocol Buffers, a **repeated field** is an array-like field that can contain zero or more values of the same type. When that type is a **message** (a structured data type with multiple fields), we call it a **repeated submessage**.

#### Example in .proto Syntax

```protobuf
syntax = "proto2";

// Define a submessage
message SensorReading {
    required float temperature = 1;
    required float humidity = 2;
    required int32 timestamp = 3;
}

// Main message with repeated submessage
message SensorLog {
    repeated SensorReading readings = 1;
}
```

In this example:
- `SensorReading` is a **submessage** (a message used within another message)
- `SensorLog.readings` is a **repeated submessage field** (an array of `SensorReading`)

### 2.2 How It Differs from Simple Repeated Fields

**Repeated Primitive Field:**
```protobuf
message Data {
    repeated int32 values = 1;  // Array of integers
}
```

**Repeated Submessage:**
```protobuf
message Data {
    repeated SensorReading readings = 1;  // Array of structured messages
}
```

The key difference:
- **Primitives**: Single values (int32, string, etc.)
- **Submessages**: Structured data with multiple fields each

### 2.3 Wire Format (Binary Encoding)

Understanding the wire format helps you appreciate nanopb's design:

For repeated primitives with `[packed = true]`:
```
[field_tag] [length] [value1] [value2] [value3] ...
```

For repeated submessages (always unpacked):
```
[field_tag] [length1] [submessage1_data]
[field_tag] [length2] [submessage2_data]
[field_tag] [length3] [submessage3_data]
...
```

**Key insight**: Each submessage occurrence repeats the field tag, allowing the decoder to identify each element independently.

---

## 3. Protocol Buffers Basics

Before diving into repeated submessages, let's review Protocol Buffers fundamentals.

### 3.1 What Are Protocol Buffers?

Protocol Buffers (protobuf) is a language-neutral, platform-neutral, extensible mechanism for serializing structured data. Think of it as XML or JSON, but:
- **Smaller** (binary format, typically 3-10x smaller)
- **Faster** (binary parsing, 20-100x faster)
- **Simpler** (strongly typed schema)
- **More portable** (same format across languages and platforms)

### 3.2 .proto File Structure

```protobuf
syntax = "proto2";  // or "proto3"

// Message definition
message MyMessage {
    // Field rules: required, optional, or repeated
    // Field type: int32, string, bool, message, etc.
    // Field name: identifier
    // Field tag: unique number (1-536870911, excluding 19000-19999)
    
    required int32 id = 1;
    optional string name = 2;
    repeated float values = 3;
}
```

### 3.3 Field Rules

- **required**: Must be provided (proto2 only, removed in proto3)
- **optional**: May or may not be present
- **repeated**: Zero or more values (array)

### 3.4 Field Types

**Scalar types**: `int32`, `int64`, `uint32`, `uint64`, `sint32`, `sint64`, `fixed32`, `fixed64`, `sfixed32`, `sfixed64`, `float`, `double`, `bool`, `string`, `bytes`

**Complex types**: `message` (submessage), `enum`

---

## 4. How nanopb Handles Repeated Submessages

### 4.1 Architecture Overview

nanopb processes messages through three main components:

```
┌─────────────────────────────────────────────────────┐
│                   Your Application                   │
│                                                      │
│  ┌────────────────┐        ┌────────────────┐      │
│  │  Encode Data   │        │  Decode Data   │      │
│  │   (Struct)     │        │   (Struct)     │      │
│  └───────┬────────┘        └────────▲───────┘      │
│          │                          │               │
│          ▼                          │               │
│  ┌──────────────────────────────────┴───────┐      │
│  │         nanopb Runtime Library            │      │
│  │                                           │      │
│  │  • pb_encode.c  (encoding logic)         │      │
│  │  • pb_decode.c  (decoding logic)         │      │
│  │  • pb_common.c  (shared utilities)       │      │
│  │  • pb_validate.c (validation)            │      │
│  └───────┬────────────────────▲──────────────┘      │
│          │                    │                     │
└──────────┼────────────────────┼─────────────────────┘
           │                    │
           ▼                    ▼
    ┌──────────────┐    ┌──────────────┐
    │ Wire Format  │    │ Wire Format  │
    │  (Binary)    │    │  (Binary)    │
    └──────────────┘    └──────────────┘
```

### 4.2 Generated Code Structure

When you compile a .proto file with repeated submessages, nanopb generates:

**Header file (.pb.h):**
```c
// Generated from: sensor.proto

#define SensorLog_readings_ARRAYSIZE 100  // Configurable max count

typedef struct {
    float temperature;
    float humidity;
    int32_t timestamp;
} SensorReading;

typedef struct {
    pb_size_t readings_count;              // Current number of elements
    SensorReading readings[100];            // Static array (default)
} SensorLog;

extern const pb_msgdesc_t SensorLog_msg;
```

**Source file (.pb.c):**
```c
// Field descriptors for encoding/decoding
PB_BIND(SensorLog, SensorLog, AUTO)
```

### 4.3 Key Data Structures

**Field Descriptor:**
```c
typedef struct pb_msgdesc_s pb_msgdesc_t;
struct pb_msgdesc_s {
    const uint32_t *field_info;     // Encoded field information
    const pb_msgdesc_t * const *submsg_info;  // Submessage descriptors
    const pb_byte_t *default_value;
    bool (*field_callback)(pb_istream_t *istream, pb_ostream_t *ostream, const pb_field_iter_t *field);
    
    pb_size_t field_count;
    pb_size_t required_field_count;
    pb_size_t largest_tag;
};
```

This descriptor tells nanopb:
- How many fields the message has
- What type each field is
- Where each field is located in the struct
- How to encode/decode each field

---

## 5. Encoding Repeated Submessages

### 5.1 The Two-Pass Encoding Algorithm

nanopb uses a clever two-pass algorithm for encoding submessages to ensure correctness and prevent buffer overruns:

**Pass 1: Size Calculation**
```c
// Create a sizing stream that counts bytes without writing
pb_ostream_t sizestream = PB_OSTREAM_SIZING;
sizestream.callback = NULL;  // No actual writing

// Encode the submessage to calculate its size
if (!pb_encode(&sizestream, SubMessage_fields, &submsg_data)) {
    return false;
}

size_t submsg_size = sizestream.bytes_written;
```

**Pass 2: Verification and Encoding**
```c
// Encode the size as a varint
if (!pb_encode_varint(stream, submsg_size)) {
    return false;
}

// Create a limiting substream to prevent writing more than calculated
pb_ostream_t substream = *stream;
substream.max_size = submsg_size;

// Encode again - must write exactly submsg_size bytes
if (!pb_encode(&substream, SubMessage_fields, &submsg_data)) {
    return false;
}

// Verify the sizes match
if (substream.bytes_written != submsg_size) {
    return false;  // Callback changed its behavior!
}
```

**Why two passes?**
- Prevents buffer overruns if callbacks behave inconsistently
- Ensures wire format correctness (size must be exact)
- Allows streaming without seeking backward

### 5.2 Encoding Flow for Repeated Submessages

Here's what happens when encoding a repeated submessage field:

```
encode_basic_field()
    │
    ├── For each element in the array:
    │   │
    │   ├── Encode field tag
    │   │
    │   └── pb_enc_submessage()
    │       │
    │       └── pb_encode_submessage()
    │           │
    │           ├── Pass 1: Calculate size
    │           │   └── pb_encode() with SIZING stream
    │           │
    │           ├── Encode size as varint
    │           │
    │           └── Pass 2: Encode and verify
    │               └── pb_encode() with limiting substream
    │
    └── Done
```

### 5.3 Code Example: Encoding

```c
#include <pb_encode.h>
#include "sensor.pb.h"

bool encode_sensor_log(uint8_t *buffer, size_t buffer_size, size_t *message_length)
{
    // Initialize the message
    SensorLog log = SensorLog_init_zero;
    
    // Add sensor readings
    log.readings_count = 3;
    
    log.readings[0].temperature = 23.5f;
    log.readings[0].humidity = 45.2f;
    log.readings[0].timestamp = 1000;
    
    log.readings[1].temperature = 24.1f;
    log.readings[1].humidity = 46.8f;
    log.readings[1].timestamp = 2000;
    
    log.readings[2].temperature = 22.9f;
    log.readings[2].humidity = 44.5f;
    log.readings[2].timestamp = 3000;
    
    // Create output stream
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    
    // Encode the message
    if (!pb_encode(&stream, SensorLog_fields, &log)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    
    *message_length = stream.bytes_written;
    return true;
}
```

### 5.4 What Gets Encoded?

For the above example, the binary output would be structured as:

```
[Tag 1] [Size of reading[0]] [temperature] [humidity] [timestamp]
[Tag 1] [Size of reading[1]] [temperature] [humidity] [timestamp]
[Tag 1] [Size of reading[2]] [temperature] [humidity] [timestamp]
```

Each submessage is self-contained with its own size prefix.

---

## 6. Decoding Repeated Submessages

### 6.1 Decoding Flow

Decoding is more straightforward than encoding because the size is already in the wire format:

```
pb_decode()
    │
    ├── Initialize message to defaults
    │   └── Set readings_count = 0
    │
    ├── While not end-of-stream:
    │   │
    │   ├── Read field tag
    │   │
    │   ├── Find field descriptor
    │   │
    │   └── If repeated submessage:
    │       │
    │       ├── Check array bounds (count < max_count)
    │       │
    │       ├── pb_dec_submessage()
    │       │   │
    │       │   ├── Read size varint
    │       │   │
    │       │   ├── Create limiting substream
    │       │   │   └── substream.bytes_left = size
    │       │   │
    │       │   └── pb_decode_inner()
    │       │       └── Recursively decode fields
    │       │
    │       ├── Increment readings_count
    │       │
    │       └── Advance array pointer
    │
    └── Done
```

### 6.2 Key Decoding Concepts

**Array Bounds Checking:**
```c
// Before adding each element
if (field->pSize != NULL && *field->pSize >= field->array_size) {
    PB_RETURN_ERROR(stream, "array overflow");
}
```

**Count Tracking:**
```c
// After successfully decoding an element
if (field->pSize != NULL) {
    (*field->pSize)++;
}
```

**Substream Isolation:**
```c
// Each submessage has a limited stream
pb_istream_t substream;
if (!pb_make_string_substream(stream, &substream)) {
    return false;
}

// Decode can't read beyond the submessage boundary
status = pb_decode_inner(&substream, fields, dest);

pb_close_string_substream(stream, &substream);
```

### 6.3 Code Example: Decoding

```c
#include <pb_decode.h>
#include "sensor.pb.h"

bool decode_sensor_log(const uint8_t *buffer, size_t message_length)
{
    // Initialize the message structure
    SensorLog log = SensorLog_init_zero;
    
    // Create input stream
    pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);
    
    // Decode the message
    if (!pb_decode(&stream, SensorLog_fields, &log)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    
    // Process the decoded data
    printf("Received %d sensor readings:\n", (int)log.readings_count);
    
    for (pb_size_t i = 0; i < log.readings_count; i++) {
        printf("  Reading %d:\n", (int)i + 1);
        printf("    Temperature: %.1f°C\n", log.readings[i].temperature);
        printf("    Humidity: %.1f%%\n", log.readings[i].humidity);
        printf("    Timestamp: %d\n", (int)log.readings[i].timestamp);
    }
    
    return true;
}
```

### 6.4 Initialization and Default Values

When a message is decoded:

1. **Structure is zeroed** (using `_init_zero`) or **set to defaults** (using `_init_default`)
2. **Count fields are set to 0** for repeated fields
3. **Array contents are not initialized** until populated during decoding
4. **Missing optional fields** retain their initialized values

Example:
```c
// Zero initialization
SensorLog log = SensorLog_init_zero;
// log.readings_count == 0
// log.readings[*] == uninitialized (don't access!)

// After decoding, only access log.readings[0..readings_count-1]
```

---

## 7. Memory Management Strategies

nanopb offers multiple strategies for managing repeated submessages, each with different trade-offs.

### 7.1 Static Allocation (Default)

**Description**: Fixed-size array allocated at compile time.

**.proto file:**
```protobuf
message SensorLog {
    repeated SensorReading readings = 1;
}
```

**.options file:**
```
SensorLog.readings max_count:100
```

**Generated code:**
```c
#define SensorLog_readings_ARRAYSIZE 100

typedef struct {
    pb_size_t readings_count;
    SensorReading readings[100];  // 100 elements always allocated
} SensorLog;
```

**Pros:**
- ✅ No malloc required (embedded-friendly)
- ✅ Predictable memory usage
- ✅ Fast access (no pointer indirection)
- ✅ Simple to use

**Cons:**
- ❌ Wastes memory if array is sparsely populated
- ❌ Fixed maximum limit
- ❌ Large arrays increase struct size

**When to use:**
- Embedded systems without malloc
- Predictable, bounded data
- Small to medium array sizes
- Stack allocation is acceptable

### 7.2 Pointer Allocation (Dynamic)

**Description**: Array allocated dynamically with malloc.

**.proto file:**
```protobuf
message SensorLog {
    repeated SensorReading readings = 1;
}
```

**.options file:**
```
SensorLog.readings type:FT_POINTER
SensorLog.readings max_count:100
```

**Generated code:**
```c
#define SensorLog_readings_ARRAYSIZE 100

typedef struct {
    pb_size_t readings_count;
    SensorReading *readings;  // Pointer to dynamically allocated array
} SensorLog;
```

**Usage:**
```c
#define PB_ENABLE_MALLOC 1  // Enable in your build
#include <pb_encode.h>
#include "sensor.pb.h"

// Encoding - allocate array
SensorLog log = SensorLog_init_zero;
log.readings_count = 3;
log.readings = malloc(3 * sizeof(SensorReading));
// ... populate readings ...
pb_encode(&stream, SensorLog_fields, &log);
free(log.readings);

// Decoding - nanopb allocates automatically
SensorLog log = SensorLog_init_zero;
pb_decode(&stream, SensorLog_fields, &log);
// ... use log.readings[0..readings_count-1] ...
pb_release(SensorLog_fields, &log);  // Free allocated memory
```

**Pros:**
- ✅ Memory only used when needed
- ✅ Supports larger arrays
- ✅ Smaller struct size

**Cons:**
- ❌ Requires malloc/free
- ❌ Potential fragmentation
- ❌ Must remember to call `pb_release()`
- ❌ Slower (heap allocation, pointer indirection)

**When to use:**
- Desktop/server applications
- Highly variable array sizes
- Large arrays that don't fit on stack
- When malloc is acceptable

### 7.3 Callback-Based Processing

**Description**: Process elements one at a time via callback, no storage.

**.proto file:**
```protobuf
message SensorLog {
    repeated SensorReading readings = 1;
}
```

**.options file:**
```
SensorLog.readings type:FT_CALLBACK
```

**Generated code:**
```c
typedef struct {
    pb_callback_t readings;  // Callback function
} SensorLog;
```

**Encoding callback:**
```c
bool encode_readings_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    // Your data source
    SensorDataSource *source = (SensorDataSource*)*arg;
    
    // Encode each reading
    for (int i = 0; i < source->count; i++) {
        SensorReading reading;
        get_reading_from_source(source, i, &reading);
        
        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }
        if (!pb_encode_submessage(stream, SensorReading_fields, &reading)) {
            return false;
        }
    }
    
    return true;
}

// Usage
SensorDataSource my_source = ...;
SensorLog log = SensorLog_init_zero;
log.readings.funcs.encode = encode_readings_callback;
log.readings.arg = &my_source;
pb_encode(&stream, SensorLog_fields, &log);
```

**Decoding callback:**
```c
bool decode_readings_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    SensorReading reading;
    
    // Decode one reading
    if (!pb_decode(stream, SensorReading_fields, &reading)) {
        return false;
    }
    
    // Process it immediately (e.g., save to database, compute average, etc.)
    process_reading(&reading);
    
    return true;
}

// Usage
SensorLog log = SensorLog_init_zero;
log.readings.funcs.decode = decode_readings_callback;
log.readings.arg = NULL;  // Or your context
pb_decode(&stream, SensorLog_fields, &log);
```

**Pros:**
- ✅ Zero memory for array storage
- ✅ Can handle unlimited elements
- ✅ Stream processing (process as received)
- ✅ Maximum flexibility

**Cons:**
- ❌ More complex to implement
- ❌ Can't random access elements later
- ❌ Must process immediately

**When to use:**
- Very large or unbounded arrays
- Stream processing requirements
- Extremely memory-constrained systems
- When you don't need to store all elements

### 7.4 Comparison Table

| Feature | Static | Pointer | Callback |
|---------|--------|---------|----------|
| malloc required | No | Yes | No |
| Memory efficiency | Low | High | Highest |
| Max elements | Fixed | Fixed | Unlimited |
| Random access | Yes | Yes | No |
| Complexity | Simple | Medium | Complex |
| Typical size overhead | Large | Small | None |

---

## 8. Configuration and Options

### 8.1 Setting Options via .options File

The `.options` file controls how nanopb generates code for your messages.

**Example: sensor.options**
```
# Set maximum count for repeated field
SensorLog.readings max_count:50

# Use specific memory strategy
SensorLog.readings type:FT_STATIC

# Set maximum size for strings in submessage
SensorReading.sensor_id max_size:32
```

### 8.2 Setting Options via .proto Annotations

You can also use nanopb-specific annotations directly in .proto files:

```protobuf
syntax = "proto2";

import "nanopb.proto";

message SensorReading {
    required float temperature = 1;
    required float humidity = 2;
    required int32 timestamp = 3;
    optional string sensor_id = 4 [(nanopb).max_size = 32];
}

message SensorLog {
    repeated SensorReading readings = 1 [(nanopb).max_count = 50];
}
```

### 8.3 Available Options for Repeated Fields

**max_count** (most important for repeated fields):
```
# In .options file
MyMessage.array_field max_count:100

# In .proto file
repeated int32 array_field = 1 [(nanopb).max_count = 100];
```

**type** (memory strategy):
```
# Static allocation (default)
MyMessage.items type:FT_STATIC

# Pointer-based (requires PB_ENABLE_MALLOC)
MyMessage.items type:FT_POINTER

# Callback-based
MyMessage.items type:FT_CALLBACK

# Ignore field completely
MyMessage.items type:FT_IGNORE
```

**fixed_count** (for fixed-size arrays):
```
# Array size is always max_count, no count field generated
MyMessage.items fixed_count:true
```

This generates:
```c
typedef struct {
    SensorReading readings[50];  // No readings_count field
} SensorLog;
```

### 8.4 Combining Options

```
# Complex configuration example
NetworkPacket.payload_data max_count:1024 type:FT_STATIC fixed_count:true
NetworkPacket.metadata type:FT_POINTER max_count:100
NetworkPacket.events type:FT_CALLBACK
```

---

## 9. Complete Working Example

Let's build a complete example from scratch: a sensor network that collects temperature readings.

### 9.1 Define the Protocol (sensor.proto)

```protobuf
syntax = "proto2";

import "nanopb.proto";

// Individual sensor reading
message SensorReading {
    required int32 sensor_id = 1;
    required float temperature = 2;
    required int32 timestamp = 3;
    optional string location = 4 [(nanopb).max_size = 32];
}

// Batch of readings
message SensorBatch {
    required int32 batch_id = 1;
    repeated SensorReading readings = 2 [(nanopb).max_count = 10];
    optional int32 total_sensors = 3;
}
```

### 9.2 Configuration (sensor.options)

```
# Already using inline options in .proto, but could also set here:
# SensorBatch.readings max_count:10
# SensorReading.location max_size:32
```

### 9.3 Generate Code

```bash
cd /path/to/your/project
python nanopb/generator/nanopb_generator.py sensor.proto
```

This creates `sensor.pb.h` and `sensor.pb.c`.

### 9.4 Encoding Example (encode_sensors.c)

```c
#include <stdio.h>
#include <string.h>
#include <pb_encode.h>
#include "sensor.pb.h"

int main(void)
{
    uint8_t buffer[256];
    size_t message_length;
    bool status;
    
    /* Initialize the message structure */
    SensorBatch batch = SensorBatch_init_zero;
    
    /* Fill in the batch header */
    batch.batch_id = 12345;
    batch.has_total_sensors = true;
    batch.total_sensors = 3;
    
    /* Add sensor readings */
    batch.readings_count = 3;
    
    // Reading 1
    batch.readings[0].sensor_id = 101;
    batch.readings[0].temperature = 23.5f;
    batch.readings[0].timestamp = 1609459200;
    batch.readings[0].has_location = true;
    strcpy(batch.readings[0].location, "Room A");
    
    // Reading 2
    batch.readings[1].sensor_id = 102;
    batch.readings[1].temperature = 24.2f;
    batch.readings[1].timestamp = 1609459260;
    batch.readings[1].has_location = true;
    strcpy(batch.readings[1].location, "Room B");
    
    // Reading 3
    batch.readings[2].sensor_id = 103;
    batch.readings[2].temperature = 22.8f;
    batch.readings[2].timestamp = 1609459320;
    batch.readings[2].has_location = false;  // No location
    
    /* Create a stream that writes to buffer */
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    /* Encode the message */
    status = pb_encode(&stream, SensorBatch_fields, &batch);
    message_length = stream.bytes_written;
    
    if (!status) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return 1;
    }
    
    printf("Successfully encoded %zu bytes\n", message_length);
    
    /* Write to file for decoding example */
    FILE *file = fopen("sensor_data.bin", "wb");
    fwrite(buffer, 1, message_length, file);
    fclose(file);
    
    return 0;
}
```

### 9.5 Decoding Example (decode_sensors.c)

```c
#include <stdio.h>
#include <pb_decode.h>
#include "sensor.pb.h"

int main(void)
{
    uint8_t buffer[256];
    size_t message_length;
    bool status;
    
    /* Read from file */
    FILE *file = fopen("sensor_data.bin", "rb");
    if (!file) {
        printf("Could not open file\n");
        return 1;
    }
    message_length = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);
    
    /* Initialize the message structure */
    SensorBatch batch = SensorBatch_init_zero;
    
    /* Create a stream that reads from buffer */
    pb_istream_t stream = pb_istream_from_buffer(buffer, message_length);
    
    /* Decode the message */
    status = pb_decode(&stream, SensorBatch_fields, &batch);
    
    if (!status) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return 1;
    }
    
    /* Print the decoded data */
    printf("Batch ID: %d\n", batch.batch_id);
    
    if (batch.has_total_sensors) {
        printf("Total sensors: %d\n", batch.total_sensors);
    }
    
    printf("Number of readings: %zu\n", (size_t)batch.readings_count);
    
    for (pb_size_t i = 0; i < batch.readings_count; i++) {
        printf("\nReading %zu:\n", (size_t)(i + 1));
        printf("  Sensor ID: %d\n", batch.readings[i].sensor_id);
        printf("  Temperature: %.1f°C\n", batch.readings[i].temperature);
        printf("  Timestamp: %d\n", batch.readings[i].timestamp);
        
        if (batch.readings[i].has_location) {
            printf("  Location: %s\n", batch.readings[i].location);
        } else {
            printf("  Location: (not specified)\n");
        }
    }
    
    return 0;
}
```

### 9.6 Makefile

```makefile
# Paths
NANOPB_DIR = /path/to/nanopb
PROTOC = protoc

# Compiler settings
CC = gcc
CFLAGS = -Wall -Werror -I$(NANOPB_DIR) -I.
LDFLAGS =

# Source files
NANOPB_CORE = $(NANOPB_DIR)/pb_encode.c $(NANOPB_DIR)/pb_decode.c $(NANOPB_DIR)/pb_common.c

all: encode_sensors decode_sensors

# Generate .pb.c and .pb.h from .proto
sensor.pb.c sensor.pb.h: sensor.proto
	python $(NANOPB_DIR)/generator/nanopb_generator.py sensor.proto

# Build encoder
encode_sensors: encode_sensors.c sensor.pb.c
	$(CC) $(CFLAGS) -o $@ $^ $(NANOPB_CORE) $(LDFLAGS)

# Build decoder
decode_sensors: decode_sensors.c sensor.pb.c
	$(CC) $(CFLAGS) -o $@ $^ $(NANOPB_CORE) $(LDFLAGS)

# Clean
clean:
	rm -f encode_sensors decode_sensors sensor.pb.c sensor.pb.h sensor_data.bin

# Test
test: encode_sensors decode_sensors
	./encode_sensors
	./decode_sensors
```

### 9.7 Running the Example

```bash
make clean
make
make test
```

Expected output:
```
Successfully encoded 89 bytes
Batch ID: 12345
Total sensors: 3
Number of readings: 3

Reading 1:
  Sensor ID: 101
  Temperature: 23.5°C
  Timestamp: 1609459200
  Location: Room A

Reading 2:
  Sensor ID: 102
  Temperature: 24.2°C
  Timestamp: 1609459260
  Location: Room B

Reading 3:
  Sensor ID: 103
  Temperature: 22.8°C
  Timestamp: 1609459320
  Location: (not specified)
```

---

## 10. Validation of Repeated Submessages

nanopb supports validation using `pb_validate.c` with constraints from the `validate.proto` extensions.

### 10.1 Defining Validation Rules

```protobuf
syntax = "proto2";

import "nanopb.proto";
import "validate/validate.proto";

message SensorReading {
    required float temperature = 1 [
        (validate.rules).float = {gte: -40.0, lte: 85.0}
    ];
    required int32 sensor_id = 2 [
        (validate.rules).int32 = {gte: 1, lte: 1000}
    ];
}

message SensorBatch {
    repeated SensorReading readings = 1 [
        (nanopb).max_count = 100,
        (validate.rules).repeated = {
            min_items: 1,
            max_items: 50,
            items: {
                message: {required: true}
            }
        }
    ];
}
```

Validation rules for repeated fields:
- **min_items**: Minimum number of elements
- **max_items**: Maximum number of elements
- **unique**: All elements must be unique
- **items**: Validation rules for each element

### 10.2 Running Validation

```c
#include <pb_validate.h>
#include "sensor.pb.h"

bool validate_sensor_batch(const SensorBatch *batch)
{
    // Validate the message
    if (!pb_validate(&SensorBatch_msg, batch)) {
        printf("Validation failed\n");
        return false;
    }
    
    printf("Validation passed\n");
    return true;
}
```

### 10.3 Common Validation Scenarios

**Enforce minimum count:**
```protobuf
repeated SensorReading readings = 1 [
    (validate.rules).repeated.min_items = 1  // At least one reading
];
```

**Enforce maximum count:**
```protobuf
repeated SensorReading readings = 1 [
    (validate.rules).repeated.max_items = 100  // No more than 100
];
```

**Validate each element:**
```protobuf
repeated string tags = 1 [
    (validate.rules).repeated = {
        items: {
            string: {min_len: 1, max_len: 32}
        }
    }
];
```

**Ensure uniqueness:**
```protobuf
repeated int32 sensor_ids = 1 [
    (validate.rules).repeated.unique = true
];
```

---

## 11. Advanced Topics

### 11.1 Nested Repeated Submessages

You can have repeated submessages within repeated submessages:

```protobuf
message Event {
    required int32 event_id = 1;
    required int32 timestamp = 2;
}

message Sensor {
    required int32 sensor_id = 1;
    repeated Event events = 2 [(nanopb).max_count = 10];
}

message Network {
    repeated Sensor sensors = 1 [(nanopb).max_count = 5];
}
```

Generated structure:
```c
typedef struct {
    int32_t event_id;
    int32_t timestamp;
} Event;

typedef struct {
    int32_t sensor_id;
    pb_size_t events_count;
    Event events[10];
} Sensor;

typedef struct {
    pb_size_t sensors_count;
    Sensor sensors[5];  // Total: 5 * (1 + 10) = 55 Event structures
} Network;
```

### 11.2 Oneof with Repeated Submessages

Protocol Buffers `oneof` can contain repeated fields:

```protobuf
message DataPacket {
    oneof payload {
        string text_data = 1;
        bytes binary_data = 2;
        SensorBatch sensor_data = 3;  // Contains repeated submessages
    }
}
```

### 11.3 Extensions and Repeated Submessages

nanopb supports Protocol Buffers extensions:

```protobuf
message BaseMessage {
    extensions 100 to 200;
}

message ExtendedData {
    required string data = 1;
}

extend BaseMessage {
    repeated ExtendedData extended_list = 100 [(nanopb).max_count = 10];
}
```

### 11.4 Unknown Fields

By default, nanopb skips unknown fields during decoding. You can handle them explicitly:

```c
#define PB_ENABLE_MALLOC 1
#include <pb_decode.h>

// Enable unknown field handling
SensorBatch batch = SensorBatch_init_zero;
pb_extension_t *extensions = NULL;
batch.extensions = &extensions;

pb_decode(&stream, SensorBatch_fields, &batch);

// Unknown fields are now stored in extensions
// Remember to free with pb_release()
```

### 11.5 Custom Encoding/Decoding

You can implement custom field handlers for special cases:

```c
bool custom_encode_readings(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    // Compress, encrypt, or transform data before encoding
    CustomDataSource *source = (CustomDataSource*)*arg;
    
    for (int i = 0; i < source->count; i++) {
        SensorReading reading;
        
        // Custom data retrieval and transformation
        transform_data(source, i, &reading);
        
        // Standard encoding
        if (!pb_encode_tag_for_field(stream, field))
            return false;
        if (!pb_encode_submessage(stream, SensorReading_fields, &reading))
            return false;
    }
    
    return true;
}
```

---

## 12. Best Practices

### 12.1 Memory Management

✅ **DO:**
- Always initialize messages with `_init_zero` or `_init_default`
- Use `pb_release()` when using `PB_ENABLE_MALLOC`
- Set realistic `max_count` values based on actual requirements
- Consider using callbacks for very large or unbounded arrays

❌ **DON'T:**
- Access array elements beyond `count`
- Forget to set `count` when encoding
- Assume arrays are initialized to zero (use `_init_zero`)
- Over-allocate static arrays unnecessarily

### 12.2 Error Handling

✅ **DO:**
```c
if (!pb_encode(&stream, MyMessage_fields, &msg)) {
    printf("Error: %s\n", PB_GET_ERROR(&stream));
    // Handle error appropriately
    return false;
}
```

❌ **DON'T:**
```c
pb_encode(&stream, MyMessage_fields, &msg);  // Ignoring return value!
// Continue without checking...
```

### 12.3 Performance

✅ **DO:**
- Use static allocation for small, predictable arrays
- Use callbacks for streaming large datasets
- Batch multiple small messages together
- Pre-allocate buffers to avoid dynamic resizing

❌ **DON'T:**
- Use pointer allocation for small arrays
- Encode/decode messages one element at a time
- Create unnecessarily deep nesting
- Ignore the `max_size` stream parameter

### 12.4 Portability

✅ **DO:**
- Use `pb_size_t` for counts and sizes
- Use fixed-width integer types (`int32_t`, `uint16_t`)
- Test on target hardware early
- Use `-Werror` to catch warnings

❌ **DON'T:**
- Use platform-specific types (`long`, `int`, `size_t` in .proto)
- Assume endianness or alignment
- Forget to test with different compilers

### 12.5 Schema Evolution

✅ **DO:**
- Use `optional` for fields that might be added later
- Never reuse field numbers
- Add new fields at the end
- Document breaking changes

❌ **DON'T:**
- Change field types
- Remove `required` fields
- Change field numbers
- Change `repeated` to non-repeated or vice versa

---

## 13. Common Pitfalls and Troubleshooting

### 13.1 Array Overflow

**Problem:**
```
PB_RETURN_ERROR: array overflow
```

**Cause:**
More elements in the wire format than `max_count`.

**Solution:**
```
# Increase max_count in .options
MyMessage.items max_count:200
```

Or use callbacks for unbounded arrays.

### 13.2 Uninitialized Count

**Problem:**
Encoder sends garbage or crashes.

**Cause:**
```c
SensorBatch batch;  // NOT initialized!
batch.readings[0] = ...;  // Setting data
// batch.readings_count is garbage!
pb_encode(&stream, SensorBatch_fields, &batch);  // Undefined behavior
```

**Solution:**
```c
SensorBatch batch = SensorBatch_init_zero;  // Always initialize!
batch.readings_count = 1;
batch.readings[0] = ...;
pb_encode(&stream, SensorBatch_fields, &batch);
```

### 13.3 Buffer Too Small

**Problem:**
```
Encoding failed: end of stream
```

**Cause:**
Output buffer is too small for the message.

**Solution:**
```c
// Calculate required size first
pb_ostream_t sizestream = PB_OSTREAM_SIZING;
pb_encode(&sizestream, MyMessage_fields, &msg);
size_t required_size = sizestream.bytes_written;

// Allocate appropriate buffer
uint8_t *buffer = malloc(required_size);
pb_ostream_t stream = pb_ostream_from_buffer(buffer, required_size);
pb_encode(&stream, MyMessage_fields, &msg);
```

### 13.4 Memory Leaks with Pointers

**Problem:**
Memory usage grows over time.

**Cause:**
Forgetting to call `pb_release()` after decoding with `PB_ENABLE_MALLOC`.

**Solution:**
```c
SensorBatch batch = SensorBatch_init_zero;
pb_decode(&stream, SensorBatch_fields, &batch);

// Use the data...

pb_release(SensorBatch_fields, &batch);  // Free allocated memory!
```

### 13.5 Incompatible Options

**Problem:**
```
Error: FT_POINTER requires PB_ENABLE_MALLOC
```

**Cause:**
Using `type:FT_POINTER` without enabling malloc.

**Solution:**
```c
// In your build configuration or before including nanopb headers:
#define PB_ENABLE_MALLOC 1
#include <pb_encode.h>
#include <pb_decode.h>
```

### 13.6 Debugging Tips

**Enable detailed error messages:**
```c
#define PB_DEBUG 1
#include <pb_encode.h>
```

**Check stream state:**
```c
if (!pb_encode(&stream, fields, msg)) {
    printf("Bytes written: %zu\n", stream.bytes_written);
    printf("Error: %s\n", PB_GET_ERROR(&stream));
}
```

**Validate intermediate steps:**
```c
// Test encoding
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
bool status = pb_encode(&stream, fields, &msg);
printf("Encode status: %d, bytes: %zu\n", status, stream.bytes_written);

// Decode it back immediately
pb_istream_t istream = pb_istream_from_buffer(buffer, stream.bytes_written);
MyMessage decoded = MyMessage_init_zero;
status = pb_decode(&istream, fields, &decoded);
printf("Decode status: %d\n", status);
```

---

## 14. Real-World Examples

### 14.1 IoT Sensor Network

Collect environmental data from multiple sensors:

```protobuf
message EnvironmentalReading {
    required int32 sensor_id = 1;
    optional float temperature = 2;
    optional float humidity = 3;
    optional float pressure = 4;
    optional int32 battery_level = 5;
    required int64 timestamp_ms = 6;
}

message SensorReport {
    required string gateway_id = 1 [(nanopb).max_size = 16];
    repeated EnvironmentalReading readings = 2 [(nanopb).max_count = 50];
    required int64 report_timestamp = 3;
}
```

### 14.2 Industrial Control System

Monitor and control factory equipment:

```protobuf
message EquipmentStatus {
    required int32 equipment_id = 1;
    required bool is_running = 2;
    required float current_speed = 3;
    optional string error_message = 4 [(nanopb).max_size = 64];
}

message FactorySnapshot {
    repeated EquipmentStatus equipment = 1 [(nanopb).max_count = 100];
    required int64 snapshot_time = 2;
    optional int32 total_power_consumption = 3;
}
```

### 14.3 Vehicle Telemetry

Track vehicle sensor data:

```protobuf
message VehicleSensor {
    required int32 sensor_type = 1;  // enum: speed, rpm, temp, etc.
    required float value = 2;
    required int32 timestamp_offset_ms = 3;  // Offset from batch time
}

message TelemetryBatch {
    required string vehicle_id = 1 [(nanopb).max_size = 20];
    required int64 batch_start_time = 2;
    repeated VehicleSensor sensors = 3 [(nanopb).max_count = 200];
    required int32 odometer_km = 4;
}
```

### 14.4 Medical Device Data

Store patient monitoring data:

```protobuf
message VitalSign {
    required int32 heart_rate = 1;
    required int32 blood_pressure_sys = 2;
    required int32 blood_pressure_dia = 3;
    required float temperature_celsius = 4;
    required int32 spo2_percent = 5;
    required int64 measurement_time = 6;
}

message PatientRecord {
    required string patient_id = 1 [(nanopb).max_size = 32];
    repeated VitalSign vitals = 2 [(nanopb).max_count = 1440];  // 24h @ 1/min
    required int64 record_start_time = 3;
}
```

---

## 15. Performance Considerations

### 15.1 Encoding Performance

Factors affecting encoding speed:

1. **Message complexity**: More fields = more work
2. **Repeated field size**: Larger arrays take longer
3. **Submessage depth**: Deep nesting adds overhead
4. **Memory access pattern**: Static arrays are faster than pointers
5. **Validation**: Adds processing time

**Benchmark example** (approximate, varies by platform):
```
Static array (10 submessages):     ~100 µs
Pointer array (10 submessages):    ~150 µs
Callback (10 submessages):         ~120 µs
```

### 15.2 Decoding Performance

Similar factors as encoding, plus:

1. **Unknown field handling**: Skipping unknown fields has small overhead
2. **Buffer management**: Avoiding multiple copies improves speed
3. **Validation**: Can double processing time

### 15.3 Memory Usage

**Static allocation:**
```
Memory = sizeof(struct) = sum of all fields
For repeated: array_size × sizeof(element)
```

**Pointer allocation:**
```
Struct size = just pointers
Heap usage = actual_count × sizeof(element)
```

**Example:**
```c
// Static: always 1000 × sizeof(SensorReading) bytes
typedef struct {
    pb_size_t readings_count;
    SensorReading readings[1000];
} SensorBatch_Static;

// Pointer: struct is small, heap usage varies
typedef struct {
    pb_size_t readings_count;
    SensorReading *readings;  // Only allocated as needed
} SensorBatch_Pointer;
```

### 15.4 Wire Format Size

Repeated submessages use more bytes than packed primitives:

```
Repeated int32 (packed):
  [tag] [length] [value1] [value2] ... [valueN]
  Overhead: ~2-5 bytes total

Repeated submessages:
  [tag] [len1] [msg1] [tag] [len2] [msg2] ... [tag] [lenN] [msgN]
  Overhead: ~2-5 bytes PER message
```

**Optimization tip**: Use packed primitives when possible:
```protobuf
// More efficient
repeated int32 sensor_ids = 1 [packed = true];

// Less efficient (but necessary for complex data)
repeated SensorReading readings = 2;
```

### 15.5 Optimization Strategies

1. **Batch messages together** to amortize overhead
2. **Use appropriate max_count** to avoid wasted memory
3. **Consider callbacks** for large datasets that don't need full storage
4. **Profile on target hardware** to identify bottlenecks
5. **Use packed encoding** for primitive arrays
6. **Flatten deeply nested structures** when possible

---

## 16. References

### 16.1 Official Documentation

- **nanopb homepage**: https://jpa.kapsi.fi/nanopb/
- **nanopb documentation**: https://jpa.kapsi.fi/nanopb/docs/
- **GitHub repository**: https://github.com/nanopb/nanopb
- **Protocol Buffers**: https://protobuf.dev/

### 16.2 Key Files in nanopb

- **pb_encode.c**: Encoding implementation
  - `pb_encode_submessage()`: Lines 724-777 (two-pass algorithm)
  - `encode_array()`: Lines 180-239 (repeated field handling)

- **pb_decode.c**: Decoding implementation
  - `pb_dec_submessage()`: Lines 1622-1672 (substream creation)
  - `decode_static_field()`: Lines 500-541 (array element iteration)

- **pb_common.h**: Common definitions and field iteration API

- **pb_validate.c**: Validation implementation

### 16.3 Example Code Locations

In the nanopb repository:

- **tests/alltypes/**: Comprehensive test covering all field types
- **tests/regression/issue_395/**: Nested repeated submessages
- **tests/validation/**: Validation constraints examples
- **tests/callbacks/**: Callback-based encoding/decoding
- **examples/simple/**: Basic usage example

### 16.4 Further Reading

- **Protocol Buffers Language Guide**: https://protobuf.dev/programming-guides/proto2/
- **nanopb concepts**: docs/concepts.md
- **nanopb reference**: docs/reference.md
- **nanopb migration guide**: docs/migration.md
- **nanopb security**: docs/security.md

---

## Conclusion

Repeated submessages are a powerful feature of Protocol Buffers, enabling you to work with arrays of complex, structured data efficiently. nanopb's implementation provides multiple strategies (static, pointer, callback) to fit different use cases, from tiny microcontrollers to larger embedded systems.

**Key takeaways:**

1. **Always initialize messages** with `_init_zero` or `_init_default`
2. **Set `max_count` appropriately** based on your requirements
3. **Choose the right memory strategy** for your constraints
4. **Handle errors** from encode/decode functions
5. **Test on target hardware** early and often

With this guide, you should now understand:
- What repeated submessages are and how they work
- How nanopb encodes and decodes them
- Different memory management strategies
- How to configure, validate, and optimize them
- Best practices and common pitfalls

Happy coding with nanopb! 🚀
