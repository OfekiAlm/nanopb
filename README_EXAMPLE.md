# Nanopb Validation Example

This directory contains a complete working example demonstrating how to use nanopb for Protocol Buffers encoding/decoding with custom validation in C.

## Files Overview

### Protocol Buffer Definitions
- `simple_user.proto` - Basic user message with required/optional fields
- `simple_user_profile.proto` - Advanced user profile with enum and more fields
- `validate.proto` - Validation rules definition (for reference)

### Configuration Files
- `simple_user.options` - Nanopb options for simple_user.proto
- `simple_user_profile.options` - Nanopb options for simple_user_profile.proto

### Generated Files
- `simple_user.pb.h/.c` - Generated C code for simple_user.proto
- `simple_user_profile.pb.h/.c` - Generated C code for simple_user_profile.proto

### Source Code
- `main.c` - Simple example demonstrating basic encoding/decoding and validation
- `simple_advanced_main.c` - Advanced example with more complex validation rules

### Build System
- `Makefile` - Build configuration for both examples

## How It Works

### 1. Protocol Buffer Definition

```protobuf
syntax = "proto2";

message SimpleUser {
    required string username = 1;
    required int32 age = 2;
    required string email = 3;
    optional string phone = 4;
    optional float score = 5;
}
```

### 2. Code Generation

The nanopb generator converts the .proto file into C structures:

```c
typedef struct _SimpleUser {
    char username[50];
    int32_t age;
    char email[100];
    char phone[20];
    float score;
    bool has_phone;
    bool has_score;
} SimpleUser;
```

### 3. Custom Validation

We implement custom validation functions that check:
- Required fields are present
- String length constraints
- Numeric range validation
- Format validation (e.g., email must contain @)
- Enum value validation

### 4. Encoding Process

```c
// Create message
SimpleUser user = SimpleUser_init_zero;
strcpy(user.username, "john_doe");
user.age = 25;
strcpy(user.email, "john@example.com");

// Encode to buffer
uint8_t buffer[256];
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
pb_encode(&stream, SimpleUser_fields, &user);
```

### 5. Decoding Process

```c
// Decode from buffer
SimpleUser decoded_user = SimpleUser_init_zero;
pb_istream_t istream = pb_istream_from_buffer(buffer, stream.bytes_written);
pb_decode(&istream, SimpleUser_fields, &decoded_user);
```

### 6. Validation Integration

```c
char error_msg[256];
if (!validate_simple_user(&user, error_msg, sizeof(error_msg))) {
    printf("Validation failed: %s\n", error_msg);
}
```

## Building and Running

### Prerequisites
- Python 3
- GCC compiler
- Make

### Build Commands

```bash
# Build both examples
make

# Build only simple example
make validation_example

# Build only advanced example
make advanced_validation_example

# Clean generated files
make clean
```

### Run Commands

```bash
# Run simple example
make run

# Run advanced example
make run-advanced

# Or run directly
./validation_example
./advanced_validation_example
```

## Example Output

### Simple Example
```
Nanopb Validation Example
========================

=== Testing Encoding and Decoding ===
Original user:
User Profile:
  Username: john_doe
  Age: 25
  Email: john@example.com
  Phone: +1234567890
  Score: 85.50

Encoded 48 bytes
Decoded user:
User Profile:
  Username: john_doe
  Age: 25
  Email: john@example.com
  Phone: +1234567890
  Score: 85.50

=== Testing Validation ===
Test 1: Valid user
✓ Validation passed

Test 2: Invalid username (too short)
✗ Validation failed: Username must be at least 3 characters

Test 3: Invalid age
✗ Validation failed: Age must be between 13 and 120

Test 4: Invalid email (no @)
✗ Validation failed: Email must contain @ symbol

Test 5: Invalid score
✗ Validation failed: Score must be between 0.0 and 100.0

=== Testing Roundtrip with Validation ===
Pre-encoding validation passed
Encoded 42 bytes
Post-decoding validation passed

✓ All tests completed successfully!
```

## Key Features Demonstrated

1. **Protocol Buffers Integration**: Using .proto files to define data structures
2. **Code Generation**: Automatic C code generation from .proto files
3. **Encoding/Decoding**: Converting between C structs and binary format
4. **Custom Validation**: Implementing business logic validation rules
5. **Error Handling**: Proper error reporting and validation feedback
6. **Memory Management**: Efficient memory usage for embedded systems
7. **Cross-Platform**: Works on any system with a C compiler

## Validation Rules Implemented

### Simple User
- Username: 3-20 characters, required
- Age: 13-120, required
- Email: must contain @, required
- Phone: must start with +, 10-15 characters (optional)
- Score: 0.0-100.0 (optional)

### Advanced User Profile
- All simple user rules plus:
- Status: must be valid enum value (optional)
- Bio: max 500 characters (optional)

## Integration with Your Project

To use this in your own project:

1. Copy the nanopb core files (`pb.h`, `pb_common.c`, `pb_encode.c`, `pb_decode.c`)
2. Define your .proto files
3. Create .options files for field size limits
4. Generate C code using the nanopb generator
5. Implement your validation functions
6. Use the encoding/decoding functions in your application

## Memory Usage

This example is designed for embedded systems with minimal memory requirements:
- No dynamic memory allocation (except for optional malloc support)
- Fixed-size buffers for all fields
- Efficient binary encoding format
- Small code footprint

The generated code is optimized for size and can run on microcontrollers with limited RAM and flash memory.
