# Nanopb Validation

Nanopb validation provides declarative constraints for Protocol Buffer messages that are enforced through generated C validation code. This feature enables embedded systems to validate messages efficiently without heap allocation or runtime reflection.

## Overview

The validation feature allows you to:
- Define constraints directly in `.proto` files using custom options
- Generate C validation functions automatically
- Validate messages before encoding or after decoding
- Get detailed violation reports without dynamic memory allocation

## Enabling Validation

There are three ways to enable validation:

### 1. Command Line Option
```bash
protoc --nanopb_out=. --nanopb_opt=--validate message.proto
# or
nanopb_generator.py --validate message.pb
```

### 2. Options File
Create a `.options` file with:
```
# Enable validation for all messages
* validate:true

# Or enable for specific messages
MyMessage validate:true
```

### 3. Proto File Option
```protobuf
syntax = "proto3";
import "validate.proto";

// Enable validation for entire file
option (validate.validate) = true;
```

## Field-Level Constraints

### Numeric Constraints

```protobuf
message Product {
    // Basic numeric constraints
    int32 quantity = 1 [
        (validate.rules).int32.gte = 0,      // Greater than or equal
        (validate.rules).int32.lte = 1000    // Less than or equal
    ];
    
    // Exact value constraint
    int32 version = 2 [(validate.rules).int32.const = 1];
    
    // Value must be in list
    int32 status = 3 [(validate.rules).int32.in = [1, 2, 3]];
    
    // Value must not be in list
    int32 type = 4 [(validate.rules).int32.not_in = [99, 100]];
}
```

Supported for: `int32`, `int64`, `uint32`, `uint64`, `sint32`, `sint64`, `fixed32`, `fixed64`, `sfixed32`, `sfixed64`, `float`, `double`

### String Constraints

```protobuf
message User {
    // Length constraints
    string username = 1 [
        (validate.rules).string.min_len = 3,
        (validate.rules).string.max_len = 20
    ];
    
    // Pattern matching
    string email = 2 [
        (validate.rules).string.contains = "@",
        (validate.rules).string.suffix = ".com"
    ];
    
    // ASCII only
    string id = 3 [(validate.rules).string.ascii = true];
    
    // Exact match or in list
    string role = 4 [(validate.rules).string.in = ["admin", "user", "guest"]];
}
```

**Note**: String length constraints only work when strings are generated as static arrays (using `max_size` option). Callback-based strings skip length validation.

### Bytes Constraints

```protobuf
message Data {
    // Length constraints
    bytes payload = 1 [
        (validate.rules).bytes.min_len = 10,
        (validate.rules).bytes.max_len = 1024
    ];
    
    // Prefix/suffix matching
    bytes header = 2 [(validate.rules).bytes.prefix = "\x00\x01\x02"];
}
```

### Enum Constraints

```protobuf
enum Status {
    UNKNOWN = 0;
    ACTIVE = 1;
    INACTIVE = 2;
}

message Record {
    // Ensure enum value is defined (default: true)
    Status status = 1 [(validate.rules).enum.defined_only = true];
    
    // Restrict to subset of values
    Status filtered = 2 [(validate.rules).enum.in = [1, 2]];
}
```

### Repeated Field Constraints

```protobuf
message Collection {
    // Item count constraints
    repeated string items = 1 [
        (nanopb).max_count = 100,  // Required for static allocation
        (validate.rules).repeated.min_items = 1,
        (validate.rules).repeated.max_items = 50
    ];
    
    // Unique items only
    repeated int32 ids = 2 [(validate.rules).repeated.unique = true];
}
```

### Required Fields

```protobuf
message Config {
    // Make optional field required for validation
    optional string name = 1 [(validate.rules).required = true];
    
    // In a oneof, require this specific arm
    oneof setting {
        string text = 2 [(validate.rules).oneof_required = true];
        int32 number = 3;
    }
}
```

## Message-Level Constraints

### Field Dependencies

```protobuf
message Address {
    string city = 1;
    string state = 2;
    string country = 3;
    
    // If city is set, state must also be set
    option (validate.message).requires = "state";
}
```

### Mutual Exclusion

```protobuf
message Settings {
    bool use_default = 1;
    string custom_value = 2;
    
    // Only one of these fields can be set
    option (validate.message).mutex = {
        fields: ["use_default", "custom_value"]
    };
}
```

### At Least N Fields

```protobuf
message Contact {
    string email = 1;
    string phone = 2;
    string address = 3;
    
    // At least 2 contact methods required
    option (validate.message).at_least = {
        n: 2
        fields: ["email", "phone", "address"]
    };
}
```

## Using Generated Validation Code

### Basic Validation

```c
#include "message.pb.h"
#include "message_validate.h"

void validate_example() {
    MyMessage msg = MyMessage_init_zero;
    pb_violations_t violations;
    
    // Initialize violations collector
    pb_violations_init(&violations);
    
    // Validate the message
    if (!pb_validate_MyMessage(&msg, &violations)) {
        // Handle validation errors
        for (size_t i = 0; i < violations.count; i++) {
            printf("Validation error: %s - %s: %s\n",
                   violations.violations[i].field_path,
                   violations.violations[i].constraint_id,
                   violations.violations[i].message);
        }
    }
}
```

### Validation Hooks

Enable automatic validation during encode/decode:

```c
// In your build configuration or source file:
#define PB_VALIDATE_BEFORE_ENCODE
#define PB_VALIDATE_AFTER_DECODE

// Then use the validation-aware macros:
pb_ostream_t stream = ...;
if (!pb_validate_encode(&stream, MyMessage, &msg)) {
    // Validation or encoding failed
}

pb_istream_t stream = ...;
if (!pb_validate_decode(&stream, MyMessage, &msg)) {
    // Decoding or validation failed
}
```

## Configuration Options

### Compile-Time Settings

```c
// Maximum number of violations to collect (default: 16)
#define PB_VALIDATE_MAX_VIOLATIONS 32

// Stop validation on first error (default: 1)
#define PB_VALIDATE_EARLY_EXIT 0

// Maximum length for field paths in violations (default: 128)
#define PB_VALIDATE_MAX_PATH_LENGTH 256
```

### Generator Options

- `--validate`: Enable validation code generation
- `--validate-consolidated`: Generate single validation file pair for all messages (future feature)

## Limitations

1. **No Regex Support**: Pattern matching is limited to simple string operations (contains, prefix, suffix)
2. **Callback Fields**: Length validation doesn't work for callback-based string/bytes fields
3. **No Heap Usage**: All validation data structures are statically allocated
4. **Proto3 Only**: Currently only supports proto3 syntax

## Integration with Build Systems

### Make
```makefile
PROTOC_OPTS += --nanopb_opt=--validate
```

### CMake
```cmake
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS PROTO_OPTIONS "--validate" message.proto)
```

### Bazel
```python
nanopb_proto_library(
    name = "message_proto",
    protos = ["message.proto"],
    options = ["--validate"],
)
```

## Performance Considerations

- Validation functions are generated as static inline where possible
- Rule data is stored in const arrays in program memory
- Early exit can be disabled for complete error collection
- No dynamic memory allocation during validation

## Error Messages

Validation errors include:
- `field_path`: Dotted path to the field (e.g., "user.email")
- `constraint_id`: Type of constraint violated (e.g., "string.min_len")
- `message`: Human-readable error description

Example violations:
```
user.email: string.contains - Field must contain '@'
user.age: int32.gte - Value must be >= 0
items[2]: string.max_len - String exceeds maximum length of 50
```
