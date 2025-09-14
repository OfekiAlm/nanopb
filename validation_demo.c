#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb_validate.h>
#include "validation_example.pb.h"

// Manual validation function that implements the same rules as defined in the .proto file
bool validate_validation_example(const ValidationExample *msg, pb_violations_t *violations) {
    if (!msg) {
        if (violations) {
            pb_violations_add(violations, "", "null_check", "Message cannot be null");
        }
        return false;
    }
    
    pb_validate_context_t ctx = {0};
    if (violations) {
        ctx.violations = violations;
        ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    }
    
    // Validate username (required, 3-20 characters)
    if (!pb_validate_context_push_field(&ctx, "username")) {
        return false;
    }
    
    if (strlen(msg->username) == 0) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "required", "Field is required");
        }
        if (ctx.early_exit) return false;
    } else {
        size_t len = strlen(msg->username);
        if (len < 3) {
            if (violations) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
            }
            if (ctx.early_exit) return false;
        }
        if (len > 20) {
            if (violations) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
            }
            if (ctx.early_exit) return false;
        }
    }
    
    pb_validate_context_pop_field(&ctx);
    
    // Validate age (13-120)
    if (!pb_validate_context_push_field(&ctx, "age")) {
        return false;
    }
    
    if (msg->age < 13) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Age too young");
        }
        if (ctx.early_exit) return false;
    }
    if (msg->age > 120) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "int32.lte", "Age too old");
        }
        if (ctx.early_exit) return false;
    }
    
    pb_validate_context_pop_field(&ctx);
    
    // Validate email (required, must contain @)
    if (!pb_validate_context_push_field(&ctx, "email")) {
        return false;
    }
    
    if (strlen(msg->email) == 0) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "required", "Field is required");
        }
        if (ctx.early_exit) return false;
    } else {
        if (strchr(msg->email, '@') == NULL) {
            if (violations) {
                pb_violations_add(violations, ctx.path_buffer, "string.contains", "Email must contain @");
            }
            if (ctx.early_exit) return false;
        }
    }
    
    pb_validate_context_pop_field(&ctx);
    
    // Validate score (0.0-100.0)
    if (!pb_validate_context_push_field(&ctx, "score")) {
        return false;
    }
    
    if (msg->score < 0.0f) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "float.gte", "Score too low");
        }
        if (ctx.early_exit) return false;
    }
    if (msg->score > 100.0f) {
        if (violations) {
            pb_violations_add(violations, ctx.path_buffer, "float.lte", "Score too high");
        }
        if (ctx.early_exit) return false;
    }
    
    pb_validate_context_pop_field(&ctx);
    
    return !violations || !pb_violations_has_any(violations);
}

void print_validation_example(const ValidationExample *msg) {
    printf("ValidationExample:\n");
    printf("  Username: %s\n", msg->username);
    printf("  Age: %d\n", (int)msg->age);
    printf("  Email: %s\n", msg->email);
    printf("  Score: %.2f\n", msg->score);
    printf("\n");
}

void print_violations(const pb_violations_t *violations) {
    if (!violations || violations->count == 0) {
        printf("No validation errors\n");
        return;
    }
    
    printf("Validation errors (%d):\n", (int)violations->count);
    for (int i = 0; i < violations->count; i++) {
        printf("  %s: %s (%s)\n", 
               violations->violations[i].field_path,
               violations->violations[i].message,
               violations->violations[i].constraint_id);
    }
    if (violations->truncated) {
        printf("  ... (more errors were truncated)\n");
    }
    printf("\n");
}

int test_encoding_decoding() {
    printf("=== Testing Encoding and Decoding ===\n");
    
    // Create a valid message
    ValidationExample msg = ValidationExample_init_zero;
    strcpy(msg.username, "john_doe");
    msg.age = 25;
    strcpy(msg.email, "john@example.com");
    msg.score = 85.5f;
    
    printf("Original message:\n");
    print_validation_example(&msg);
    
    // Encode
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&ostream, ValidationExample_fields, &msg)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    
    printf("Encoded %zu bytes\n", ostream.bytes_written);
    
    // Decode
    ValidationExample decoded_msg = ValidationExample_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);
    
    if (!pb_decode(&istream, ValidationExample_fields, &decoded_msg)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    
    printf("Decoded message:\n");
    print_validation_example(&decoded_msg);
    
    return 0;
}

int test_validation() {
    printf("=== Testing Validation ===\n");
    
    pb_violations_t violations;
    
    // Test 1: Valid message
    printf("Test 1: Valid message\n");
    ValidationExample valid_msg = ValidationExample_init_zero;
    strcpy(valid_msg.username, "alice");
    valid_msg.age = 30;
    strcpy(valid_msg.email, "alice@example.com");
    valid_msg.score = 92.0f;
    
    pb_violations_init(&violations);
    if (validate_validation_example(&valid_msg, &violations)) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed\n");
        print_violations(&violations);
    }
    print_validation_example(&valid_msg);
    
    // Test 2: Invalid username (too short)
    printf("Test 2: Invalid username (too short)\n");
    ValidationExample invalid_msg1 = ValidationExample_init_zero;
    strcpy(invalid_msg1.username, "ab");
    invalid_msg1.age = 25;
    strcpy(invalid_msg1.email, "ab@example.com");
    invalid_msg1.score = 75.0f;
    
    pb_violations_init(&violations);
    if (validate_validation_example(&invalid_msg1, &violations)) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed\n");
        print_violations(&violations);
    }
    print_validation_example(&invalid_msg1);
    
    // Test 3: Invalid age
    printf("Test 3: Invalid age\n");
    ValidationExample invalid_msg2 = ValidationExample_init_zero;
    strcpy(invalid_msg2.username, "bob");
    invalid_msg2.age = 5;  // Too young
    strcpy(invalid_msg2.email, "bob@example.com");
    invalid_msg2.score = 75.0f;
    
    pb_violations_init(&violations);
    if (validate_validation_example(&invalid_msg2, &violations)) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed\n");
        print_violations(&violations);
    }
    print_validation_example(&invalid_msg2);
    
    // Test 4: Invalid email (no @)
    printf("Test 4: Invalid email (no @)\n");
    ValidationExample invalid_msg3 = ValidationExample_init_zero;
    strcpy(invalid_msg3.username, "charlie");
    invalid_msg3.age = 35;
    strcpy(invalid_msg3.email, "charlie.example.com");  // Missing @
    invalid_msg3.score = 75.0f;
    
    pb_violations_init(&violations);
    if (validate_validation_example(&invalid_msg3, &violations)) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed\n");
        print_violations(&violations);
    }
    print_validation_example(&invalid_msg3);
    
    // Test 5: Invalid score
    printf("Test 5: Invalid score\n");
    ValidationExample invalid_msg4 = ValidationExample_init_zero;
    strcpy(invalid_msg4.username, "david");
    invalid_msg4.age = 28;
    strcpy(invalid_msg4.email, "david@example.com");
    invalid_msg4.score = 150.0f;  // Too high
    
    pb_violations_init(&violations);
    if (validate_validation_example(&invalid_msg4, &violations)) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed\n");
        print_violations(&violations);
    }
    print_validation_example(&invalid_msg4);
    
    return 0;
}

int test_roundtrip_with_validation() {
    printf("=== Testing Roundtrip with Validation ===\n");
    
    // Create a valid message
    ValidationExample msg = ValidationExample_init_zero;
    strcpy(msg.username, "eve");
    msg.age = 27;
    strcpy(msg.email, "eve@example.com");
    msg.score = 78.5f;
    
    pb_violations_t violations;
    
    // Validate before encoding
    pb_violations_init(&violations);
    if (!validate_validation_example(&msg, &violations)) {
        printf("Pre-encoding validation failed\n");
        print_violations(&violations);
        return 1;
    }
    
    printf("Pre-encoding validation passed\n");
    print_validation_example(&msg);
    
    // Encode
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&ostream, ValidationExample_fields, &msg)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    
    printf("Encoded %zu bytes\n", ostream.bytes_written);
    
    // Decode
    ValidationExample decoded_msg = ValidationExample_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);
    
    if (!pb_decode(&istream, ValidationExample_fields, &decoded_msg)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    
    // Validate after decoding
    pb_violations_init(&violations);
    if (!validate_validation_example(&decoded_msg, &violations)) {
        printf("Post-decoding validation failed\n");
        print_violations(&violations);
        return 1;
    }
    
    printf("Post-decoding validation passed\n");
    print_validation_example(&decoded_msg);
    
    return 0;
}

int main() {
    printf("Nanopb Validation Demo\n");
    printf("=====================\n\n");
    
    int result = 0;
    
    result += test_encoding_decoding();
    printf("\n");
    
    result += test_validation();
    printf("\n");
    
    result += test_roundtrip_with_validation();
    
    if (result == 0) {
        printf("\n✓ All tests completed successfully!\n");
    } else {
        printf("\n✗ Some tests failed\n");
    }
    
    return result;
}
