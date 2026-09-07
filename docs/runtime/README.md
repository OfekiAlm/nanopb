# Nanopb Runtime Library Architecture

This document describes the architecture of the nanopb runtime library - the C code that actually encodes and decodes Protocol Buffer messages on embedded systems.

## Table of Contents

- [Overview](#overview)
- [Core Components](#core-components)
- [Memory Management](#memory-management)
- [Stream Architecture](#stream-architecture)
- [Encoding Process](#encoding-process)
- [Decoding Process](#decoding-process)
- [Validation System](#validation-system)
- [Development Guide](#development-guide)

## Overview

The runtime library consists of ~150KB of ANSI C code providing:
- Message encoding and decoding
- Stream-based I/O abstraction
- Optional validation support
- Zero dependencies beyond standard C library

**Design Philosophy:**
- **Embedded-first** - Optimized for resource-constrained systems
- **Predictable** - No hidden allocations or recursion limits
- **Portable** - Works on 8-bit to 64-bit systems
- **Configurable** - Features can be disabled to save space

## Core Components

### File Organization

```
pb.h              (44KB)  - Core types, configuration, wire format
pb_common.h/c     (12KB)  - Shared utilities
pb_encode.h/c     (30KB)  - Encoding functions
pb_decode.h/c     (55KB)  - Decoding functions
pb_validate.h/c   (23KB)  - Runtime validation (optional)
```

### 1. pb.h - Core Definitions

**Purpose:** Central header with type definitions and configuration options

**Key Types:**

```c
/* Field descriptor - describes one field in a message */
typedef struct pb_field_s {
    uint32_t tag;              /* Field tag number */
    pb_type_t type;            /* Field type (varint, fixed, string, etc.) */
    uint8_t data_offset;       /* Offset in message struct */
    int8_t size_offset;        /* Offset of size/count field */
    uint8_t data_size;         /* Size of field data */
    uint8_t array_size;        /* Maximum array size */
    const void *ptr;           /* Submessage descriptor or default value */
} pb_field_t;

/* Message descriptor - describes entire message */
typedef struct pb_msgdesc_s {
    const pb_field_t *fields;      /* Array of field descriptors */
    const void *default_value;     /* Default values */
    bool (*field_callback)(pb_field_iter_t *iter);
    pb_size_t field_count;
    pb_size_t required_field_count;
} pb_msgdesc_t;

/* Output stream for encoding */
typedef struct pb_ostream_s {
    bool (*callback)(pb_ostream_t *stream, const uint8_t *buf, size_t count);
    void *state;
    size_t max_size;
    size_t bytes_written;
} pb_ostream_t;

/* Input stream for decoding */
typedef struct pb_istream_s {
    bool (*callback)(pb_istream_t *stream, uint8_t *buf, size_t count);
    void *state;
    size_t bytes_left;
} pb_istream_t;
```

**Configuration Options:**

```c
/* Enable malloc support for dynamic allocation */
#define PB_ENABLE_MALLOC

/* Disable 64-bit support to save code space */
#define PB_WITHOUT_64BIT

/* Use only memory buffers (no stream callbacks) */
#define PB_BUFFER_ONLY

/* Disable error messages to save code space */
#define PB_NO_ERRMSG

/* Support tag numbers > 65536 */
#define PB_FIELD_32BIT

/* Validate UTF-8 strings */
#define PB_VALIDATE_UTF8
```

### 2. pb_common.c - Shared Utilities

**Purpose:** Helper functions used by both encoder and decoder

**Key Functions:**

```c
/* Check if field is present in message */
bool pb_field_iter_begin(pb_field_iter_t *iter, 
                          const pb_msgdesc_t *desc, 
                          void *message);

/* Move to next field */
bool pb_field_iter_next(pb_field_iter_t *iter);

/* Find field by tag number */
bool pb_field_iter_find(pb_field_iter_t *iter, uint32_t tag);

/* Get default value for field */
void pb_get_default_value(pb_field_iter_t *iter);
```

**Common Data Structures:**

```c
/* Iterator for walking through message fields */
typedef struct pb_field_iter_s {
    const pb_msgdesc_t *descriptor;
    void *message;
    pb_size_t field_index;
    pb_size_t required_field_index;
    void *pField;
    void *pSize;
} pb_field_iter_t;
```

### 3. pb_encode.c - Encoding

**Purpose:** Convert C structs to Protocol Buffer wire format

**Main Entry Points:**

```c
/* Encode message to stream */
bool pb_encode(pb_ostream_t *stream, 
               const pb_msgdesc_t *fields,
               const void *src_struct);

/* Encode with extensions support */
bool pb_encode_ex(pb_ostream_t *stream,
                  const pb_msgdesc_t *fields,
                  const void *src_struct,
                  unsigned int flags);

/* Get encoded size without actually encoding */
bool pb_get_encoded_size(size_t *size,
                         const pb_msgdesc_t *fields,
                         const void *src_struct);
```

**Encoding Strategy:**
1. Iterate through fields in tag order
2. For each field with a value:
   - Encode tag (field number + wire type)
   - Encode value based on type
3. Handle submessages recursively
4. Use callbacks for large/dynamic data

### 4. pb_decode.c - Decoding

**Purpose:** Parse Protocol Buffer wire format into C structs

**Main Entry Points:**

```c
/* Decode message from stream */
bool pb_decode(pb_istream_t *stream,
               const pb_msgdesc_t *fields,
               void *dest_struct);

/* Decode with extensions support */
bool pb_decode_ex(pb_istream_t *stream,
                  const pb_msgdesc_t *fields,
                  void *dest_struct,
                  unsigned int flags);

/* Decode without clearing destination first */
bool pb_decode_noinit(pb_istream_t *stream,
                      const pb_msgdesc_t *fields,
                      void *dest_struct);
```

**Decoding Strategy:**
1. Read tag from stream
2. Find corresponding field descriptor
3. Decode value based on type
4. Handle unknown fields gracefully
5. Verify required fields are present

### 5. pb_validate.c - Validation (Optional)

**Purpose:** Validate decoded messages against constraints

**Main Functions:**

```c
/* Validate entire message */
bool pb_validate(const pb_msgdesc_t *fields,
                 const void *message,
                 pb_validation_error_t *error);

/* Validation error information */
typedef struct {
    uint32_t field_tag;
    const char *field_name;
    const char *error_message;
} pb_validation_error_t;
```

## Memory Management

### Allocation Strategies

The runtime supports three allocation modes per field:

#### 1. Static Allocation (Default)

```c
/* Field with fixed size */
struct Message {
    char name[40];           /* Fixed 40-byte buffer */
    int32_t values[10];      /* Fixed 10-element array */
};
```

**Advantages:**
- No malloc required
- Predictable memory usage
- Fast allocation

**Disadvantages:**
- Memory wasted if not fully used
- Maximum size must be known at compile time

#### 2. Pointer Allocation

```c
/* Field with dynamic allocation */
struct Message {
    char *name;              /* Pointer, allocated with malloc */
    pb_size_t values_count;
    int32_t *values;         /* Pointer to array */
};
```

**Advantages:**
- Memory used only when needed
- No maximum size limit

**Disadvantages:**
- Requires malloc/free
- Risk of memory leaks
- Not suitable for all embedded systems

#### 3. Callback Allocation

```c
/* Field with custom handling */
struct Message {
    pb_callback_t large_data;  /* User-provided encode/decode */
};

/* User implements: */
bool encode_callback(pb_ostream_t *stream, 
                     const pb_field_t *field,
                     void * const *arg) {
    /* Custom encoding logic */
    /* Can read from file, generate on-the-fly, etc. */
}
```

**Advantages:**
- Handle arbitrarily large data
- Stream from external sources
- Maximum flexibility

**Disadvantages:**
- More complex to implement
- User must manage resources

### Memory Layout

Messages are packed C structs:

```c
struct Person {
    int32_t id;              /* Offset 0, size 4 */
    char name[40];           /* Offset 4, size 40 */
    bool has_email;          /* Offset 44, size 1 */
    char email[100];         /* Offset 48, size 100 */
};  /* Total: 148 bytes (with padding) */
```

**Alignment:**
- Fields are aligned according to C struct rules
- Use `packed` option to minimize padding
- Watch out for unaligned access on some CPUs

## Stream Architecture

### Stream Abstraction

Nanopb uses streams to abstract I/O:

```
Application
     │
     ▼
pb_encode/decode
     │
     ▼
pb_ostream_t / pb_istream_t
     │
     ├─► Memory buffer
     ├─► File
     ├─► Network socket
     ├─► Serial port
     └─► Custom callback
```

### Output Streams

```c
/* Create stream from buffer */
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

/* Create stream from callback */
pb_ostream_t pb_ostream_from_callback(
    bool (*callback)(pb_ostream_t*, const uint8_t*, size_t),
    void *state,
    size_t max_size
);
```

**Example - Write to File:**
```c
bool file_callback(pb_ostream_t *stream, 
                   const uint8_t *buf, 
                   size_t count) {
    FILE *file = (FILE*)stream->state;
    return fwrite(buf, 1, count, file) == count;
}

FILE *file = fopen("output.bin", "wb");
pb_ostream_t stream = pb_ostream_from_callback(
    file_callback, file, SIZE_MAX
);
pb_encode(&stream, Message_fields, &message);
fclose(file);
```

### Input Streams

```c
/* Create stream from buffer */
pb_istream_t stream = pb_istream_from_buffer(buffer, sizeof(buffer));

/* Create stream from callback */
pb_istream_t pb_istream_from_callback(
    bool (*callback)(pb_istream_t*, uint8_t*, size_t),
    void *state,
    size_t bytes_left
);
```

## Encoding Process

### High-Level Flow

```
1. Application calls pb_encode()
   └─► Validates message descriptor
   
2. Iterate through fields
   ├─► Check if field has value
   ├─► Calculate required size
   └─► Encode if present
   
3. For each field:
   ├─► Encode tag (wire type + field number)
   ├─► Encode value (based on type)
   └─► Handle submessages recursively
   
4. Return success/failure
```

### Wire Format Encoding

**Varint Encoding:**
```c
/* Encode 32-bit varint */
bool pb_encode_varint32(pb_ostream_t *stream, uint32_t value) {
    uint8_t buffer[10];
    size_t i = 0;
    
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0)
            byte |= 0x80;  /* More bytes coming */
        buffer[i++] = byte;
    } while (value != 0);
    
    return pb_write(stream, buffer, i);
}
```

**Fixed-Size Encoding:**
```c
/* Encode 32-bit fixed */
bool pb_encode_fixed32(pb_ostream_t *stream, uint32_t value) {
    uint8_t buffer[4];
    /* Little-endian encoding */
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
    return pb_write(stream, buffer, 4);
}
```

**String/Bytes Encoding:**
```c
/* Encode length-delimited value */
bool pb_encode_string(pb_ostream_t *stream, 
                      const uint8_t *data,
                      size_t size) {
    /* Encode length as varint */
    if (!pb_encode_varint(stream, size))
        return false;
    
    /* Encode data */
    return pb_write(stream, data, size);
}
```

### Submessage Encoding

Submessages are length-delimited:

```c
bool pb_encode_submessage(pb_ostream_t *stream,
                          const pb_msgdesc_t *fields,
                          const void *message) {
    /* Calculate encoded size */
    size_t size;
    pb_get_encoded_size(&size, fields, message);
    
    /* Encode size */
    pb_encode_varint(stream, size);
    
    /* Encode message */
    return pb_encode(stream, fields, message);
}
```

## Decoding Process

### High-Level Flow

```
1. Application calls pb_decode()
   └─► Initialize message to defaults
   
2. Read wire format tags
   ├─► Extract field number
   ├─► Extract wire type
   └─► Find field descriptor
   
3. For each tag:
   ├─► Decode value based on type
   ├─► Store in message struct
   └─► Handle submessages recursively
   
4. After all data read:
   ├─► Verify required fields present
   └─► Return success/failure
```

### Wire Format Decoding

**Varint Decoding:**
```c
bool pb_decode_varint32(pb_istream_t *stream, uint32_t *dest) {
    uint32_t result = 0;
    int bitpos = 0;
    uint8_t byte;
    
    do {
        if (!pb_read(stream, &byte, 1))
            return false;
            
        result |= (uint32_t)(byte & 0x7F) << bitpos;
        bitpos += 7;
    } while (byte & 0x80);  /* Continue if MSB set */
    
    *dest = result;
    return true;
}
```

**Unknown Field Handling:**
```c
bool pb_skip_field(pb_istream_t *stream, pb_wire_type_t wire_type) {
    switch (wire_type) {
        case PB_WT_VARINT:
            return pb_skip_varint(stream);
        case PB_WT_64BIT:
            return pb_read(stream, NULL, 8);
        case PB_WT_STRING:
            uint32_t len;
            pb_decode_varint32(stream, &len);
            return pb_read(stream, NULL, len);
        case PB_WT_32BIT:
            return pb_read(stream, NULL, 4);
        default:
            return false;
    }
}
```

### Required Field Tracking

The decoder tracks required fields using a bitfield:

```c
/* Check if all required fields are present */
bool pb_check_required_fields(const pb_msgdesc_t *desc,
                               uint64_t *required_fields) {
    for (size_t i = 0; i < desc->required_field_count; i++) {
        if (!(required_fields[i / 64] & (1ULL << (i % 64)))) {
            /* Required field missing */
            return false;
        }
    }
    return true;
}
```

## Validation System

### Runtime Validation

Validation happens after decoding:

```c
/* Decode message */
Message msg = Message_init_zero;
pb_istream_t stream = pb_istream_from_buffer(data, size);
if (!pb_decode(&stream, Message_fields, &msg)) {
    /* Decoding failed */
}

/* Validate message */
if (!validate_Message(&msg)) {
    /* Validation failed */
}

/* Use validated message */
process_message(&msg);
```

### Validation Error Handling

```c
/* Get detailed validation error */
pb_validation_error_t error;
if (!pb_validate(Message_fields, &msg, &error)) {
    printf("Validation failed:\n");
    printf("  Field: %s (tag %u)\n", 
           error.field_name, error.field_tag);
    printf("  Error: %s\n", error.error_message);
}
```

## Development Guide

### Adding a New Field Type

1. **Define wire type in pb.h:**
   ```c
   #define PB_LTYPE_MY_TYPE 0x0F
   ```

2. **Add encoding in pb_encode.c:**
   ```c
   bool pb_encode_my_type(pb_ostream_t *stream, ...) {
       /* Encoding logic */
   }
   ```

3. **Add decoding in pb_decode.c:**
   ```c
   bool pb_decode_my_type(pb_istream_t *stream, ...) {
       /* Decoding logic */
   }
   ```

4. **Update field iterator in pb_common.c**

5. **Add tests**

### Optimizing Code Size

**Disable unused features:**
```c
#define PB_NO_ERRMSG      /* -2KB */
#define PB_WITHOUT_64BIT  /* -4KB */
#define PB_BUFFER_ONLY    /* -1KB */
```

**Use callbacks instead of pointer fields** (saves malloc code)

**Share message descriptors** when possible

### Debugging Tips

1. **Enable error messages:**
   Remove `PB_NO_ERRMSG` to get detailed errors

2. **Check stream state:**
   ```c
   printf("Bytes written: %zu\n", stream.bytes_written);
   printf("Bytes left: %zu\n", stream.bytes_left);
   ```

3. **Dump encoded data:**
   ```c
   for (size_t i = 0; i < size; i++) {
       printf("%02X ", buffer[i]);
   }
   ```

4. **Use validation:**
   Validate both before encoding and after decoding

## Performance Characteristics

### Encoding Speed
- ~1-5 MB/s on ARM Cortex-M4 @ 168 MHz
- Dominated by stream callback overhead
- Linear in message size

### Decoding Speed
- ~0.5-2 MB/s on ARM Cortex-M4
- Includes field lookup overhead
- Unknown field skipping is fast

### Memory Usage
- Stack: ~1KB for encoding/decoding
- Message size: Defined by generated structs
- No heap required (unless using pointers)

### Code Size
- Minimal: ~5KB (encode only, no 64-bit)
- Typical: ~15KB (encode + decode)
- Full: ~25KB (with all features)

## Further Reading

- [../../ARCHITECTURE.md](../../ARCHITECTURE.md) - Overall architecture
- [../reference.md](../reference.md) - API reference
- [../concepts.md](../concepts.md) - Usage guide
- [../security.md](../security.md) - Security considerations

## Contributing

When modifying the runtime:
1. Maintain ANSI C compatibility
2. Test on multiple platforms (8/16/32/64-bit)
3. Measure code size impact
4. Add comprehensive tests
5. Update documentation

---

**Note:** This document describes nanopb 1.0.0-dev. Earlier versions may differ.
