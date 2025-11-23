# Nanopb Validation Complete Example

This demonstrates how to use nanopb with validation, showing both the .proto file with validation rules and the proper C implementation.

## Files Created

### 1. Protocol Buffer Definition with Validation Rules
**`validation_example.proto`**
```protobuf
syntax = "proto3";

import "generator/proto/validate.proto";

option (validate.validate) = true;

message ValidationExample {
    string username = 1 [
        (validate.rules).string.min_len = 3,
        (validate.rules).string.max_len = 20,
        (validate.rules).required = true
    ];
    
    int32 age = 2 [
        (validate.rules).int32.gte = 13,
        (validate.rules).int32.lte = 120
    ];
    
    string email = 3 [
        (validate.rules).string.contains = "@",
        (validate.rules).required = true
    ];
    
    float score = 4 [
        (validate.rules).float.gte = 0.0,
        (validate.rules).float.lte = 100.0
    ];
}
```

### 2. Nanopb Options File
**`validation_example.options`**
```
# Nanopb options for validation_example.proto

ValidationExample.username max_size:20
ValidationExample.email max_size:100
```

### 3. Manual Validation Implementation
**`validation_demo.c`** - Shows how to implement validation using the nanopb validation system:

```c
#include <pb_validate.h>
#include "validation_example.pb.h"

bool validate_validation_example(const ValidationExample *msg, pb_violations_t *violations) {
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    if (violations) {
        ctx.violations = violations;
        ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    }
    
    // Validate username (required, 3-20 characters)
    if (!pb_validate_context_push_field(&ctx, "username")) return false;
    
    if (strlen(msg->username) == 0) {
        pb_violations_add(violations, ctx.path_buffer, "required", "Field is required");
        if (ctx.early_exit) return false;
    } else {
        size_t len = strlen(msg->username);
        if (len < 3) {
            pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
            if (ctx.early_exit) return false;
        }
        if (len > 20) {
            pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
            if (ctx.early_exit) return false;
        }
    }
    
    pb_validate_context_pop_field(&ctx);
    
    // ... more validation rules ...
    
    return !violations || !pb_violations_has_any(violations);
}
```

## How It Works

### 1. **Protocol Buffer Definition**
The `.proto` file defines validation rules using the `validate.proto` extension:
- `(validate.rules).string.min_len = 3` - Minimum string length
- `(validate.rules).string.max_len = 20` - Maximum string length  
- `(validate.rules).required = true` - Field is required
- `(validate.rules).int32.gte = 13` - Integer greater than or equal to 13
- `(validate.rules).int32.lte = 120` - Integer less than or equal to 120
- `(validate.rules).string.contains = "@"` - String must contain "@"
- `(validate.rules).float.gte = 0.0` - Float greater than or equal to 0.0
- `(validate.rules).float.lte = 100.0` - Float less than or equal to 100.0

### 2. **Code Generation**
The nanopb generator creates:
- `validation_example.pb.h` - C struct definitions
- `validation_example.pb.c` - Field descriptors and encoding/decoding functions

### 3. **Validation Implementation**
The manual validation function:
- Uses `pb_validate_context_t` for field path tracking
- Uses `pb_violations_t` for error collection
- Implements the same validation rules as defined in the .proto file
- Provides detailed error messages with field paths and constraint IDs

### 4. **Usage Example**
```c
ValidationExample msg = ValidationExample_init_zero;
strcpy(msg.username, "john_doe");
msg.age = 25;
strcpy(msg.email, "john@example.com");
msg.score = 85.5f;

pb_violations_t violations;
pb_violations_init(&violations);

if (validate_validation_example(&msg, &violations)) {
    printf("Validation passed!\n");
} else {
    printf("Validation failed:\n");
    for (int i = 0; i < violations.count; i++) {
        printf("  %s: %s (%s)\n", 
               violations.violations[i].field_path,
               violations.violations[i].message,
               violations.violations[i].constraint_id);
    }
}
```

## Key Features Demonstrated

1. **Declarative Validation Rules** - Rules defined in .proto file
2. **Type-Safe Validation** - C structs with proper field types
3. **Detailed Error Reporting** - Field paths, constraint IDs, and error messages
4. **Context-Aware Validation** - Field path tracking for nested validation
5. **Early Exit Support** - Stop validation on first error if desired
6. **Memory Efficient** - No dynamic allocation, suitable for embedded systems

## Validation Rules Supported

- **String Rules**: min_len, max_len, contains, prefix, suffix, required
- **Numeric Rules**: gte, lte, gt, lt, eq, in, not_in
- **Float Rules**: gte, lte, gt, lt
- **Enum Rules**: defined_only, in, not_in
- **Required Fields**: Field must be present and non-empty

## Integration with Your Project

1. **Include nanopb validation headers**:
   ```c
   #include <pb_validate.h>
   #include "your_message.pb.h"
   ```

2. **Implement validation functions** following the pattern shown

3. **Use validation in your application**:
   ```c
   // Before encoding
   if (!validate_your_message(&msg, &violations)) {
       // Handle validation errors
   }
   
   // After decoding  
   if (!validate_your_message(&decoded_msg, &violations)) {
       // Handle validation errors
   }
   ```

This approach provides the benefits of declarative validation rules in the .proto file while giving you full control over the validation implementation in C.
