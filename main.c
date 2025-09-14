#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "simple_user.pb.h"

// Custom validation function for our simple user
bool validate_simple_user(const SimpleUser *user, char *error_msg, size_t error_size) {
    // Check required fields
    if (strlen(user->username) == 0) {
        snprintf(error_msg, error_size, "Username is required");
        return false;
    }
    
    if (strlen(user->email) == 0) {
        snprintf(error_msg, error_size, "Email is required");
        return false;
    }
    
    // Check username length
    if (strlen(user->username) < 3) {
        snprintf(error_msg, error_size, "Username must be at least 3 characters");
        return false;
    }
    
    if (strlen(user->username) > 20) {
        snprintf(error_msg, error_size, "Username must be at most 20 characters");
        return false;
    }
    
    // Check age range
    if (user->age < 13 || user->age > 120) {
        snprintf(error_msg, error_size, "Age must be between 13 and 120");
        return false;
    }
    
    // Check email contains @
    if (strchr(user->email, '@') == NULL) {
        snprintf(error_msg, error_size, "Email must contain @ symbol");
        return false;
    }
    
    // Check score range if provided
    if (user->has_score && (user->score < 0.0f || user->score > 100.0f)) {
        snprintf(error_msg, error_size, "Score must be between 0.0 and 100.0");
        return false;
    }
    
    return true;
}

void print_user(const SimpleUser *user) {
    printf("User Profile:\n");
    printf("  Username: %s\n", user->username);
    printf("  Age: %d\n", (int)user->age);
    printf("  Email: %s\n", user->email);
    if (user->has_phone) {
        printf("  Phone: %s\n", user->phone);
    }
    if (user->has_score) {
        printf("  Score: %.2f\n", user->score);
    }
    printf("\n");
}

int test_encoding_decoding() {
    printf("=== Testing Encoding and Decoding ===\n");
    
    // Create a user profile
    SimpleUser user = SimpleUser_init_zero;
    strcpy(user.username, "john_doe");
    user.age = 25;
    strcpy(user.email, "john@example.com");
    user.has_phone = true;
    strcpy(user.phone, "+1234567890");
    user.has_score = true;
    user.score = 85.5f;
    
    printf("Original user:\n");
    print_user(&user);
    
    // Encode the message
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&ostream, SimpleUser_fields, &user)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    
    printf("Encoded %zu bytes\n", ostream.bytes_written);
    
    // Decode the message
    SimpleUser decoded_user = SimpleUser_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);
    
    if (!pb_decode(&istream, SimpleUser_fields, &decoded_user)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    
    printf("Decoded user:\n");
    print_user(&decoded_user);
    
    return 0;
}

int test_validation() {
    printf("=== Testing Validation ===\n");
    
    char error_msg[256];
    
    // Test 1: Valid user
    printf("Test 1: Valid user\n");
    SimpleUser valid_user = SimpleUser_init_zero;
    strcpy(valid_user.username, "alice");
    valid_user.age = 30;
    strcpy(valid_user.email, "alice@example.com");
    valid_user.has_score = true;
    valid_user.score = 92.0f;
    
    if (validate_simple_user(&valid_user, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user(&valid_user);
    
    // Test 2: Invalid username (too short)
    printf("Test 2: Invalid username (too short)\n");
    SimpleUser invalid_user1 = SimpleUser_init_zero;
    strcpy(invalid_user1.username, "ab");
    invalid_user1.age = 25;
    strcpy(invalid_user1.email, "ab@example.com");
    
    if (validate_simple_user(&invalid_user1, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user(&invalid_user1);
    
    // Test 3: Invalid age
    printf("Test 3: Invalid age\n");
    SimpleUser invalid_user2 = SimpleUser_init_zero;
    strcpy(invalid_user2.username, "bob");
    invalid_user2.age = 5;  // Too young
    strcpy(invalid_user2.email, "bob@example.com");
    
    if (validate_simple_user(&invalid_user2, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user(&invalid_user2);
    
    // Test 4: Invalid email (no @)
    printf("Test 4: Invalid email (no @)\n");
    SimpleUser invalid_user3 = SimpleUser_init_zero;
    strcpy(invalid_user3.username, "charlie");
    invalid_user3.age = 35;
    strcpy(invalid_user3.email, "charlie.example.com");  // Missing @
    
    if (validate_simple_user(&invalid_user3, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user(&invalid_user3);
    
    // Test 5: Invalid score
    printf("Test 5: Invalid score\n");
    SimpleUser invalid_user4 = SimpleUser_init_zero;
    strcpy(invalid_user4.username, "david");
    invalid_user4.age = 28;
    strcpy(invalid_user4.email, "david@example.com");
    invalid_user4.has_score = true;
    invalid_user4.score = 150.0f;  // Too high
    
    if (validate_simple_user(&invalid_user4, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user(&invalid_user4);
    
    return 0;
}

int test_roundtrip_with_validation() {
    printf("=== Testing Roundtrip with Validation ===\n");
    
    // Create a valid user
    SimpleUser user = SimpleUser_init_zero;
    strcpy(user.username, "eve");
    user.age = 27;
    strcpy(user.email, "eve@example.com");
    user.has_phone = true;
    strcpy(user.phone, "+9876543210");
    user.has_score = true;
    user.score = 78.5f;
    
    char error_msg[256];
    
    // Validate before encoding
    if (!validate_simple_user(&user, error_msg, sizeof(error_msg))) {
        printf("Pre-encoding validation failed: %s\n", error_msg);
        return 1;
    }
    
    printf("Pre-encoding validation passed\n");
    print_user(&user);
    
    // Encode
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&ostream, SimpleUser_fields, &user)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    
    printf("Encoded %zu bytes\n", ostream.bytes_written);
    
    // Decode
    SimpleUser decoded_user = SimpleUser_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);
    
    if (!pb_decode(&istream, SimpleUser_fields, &decoded_user)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    
    // Validate after decoding
    if (!validate_simple_user(&decoded_user, error_msg, sizeof(error_msg))) {
        printf("Post-decoding validation failed: %s\n", error_msg);
        return 1;
    }
    
    printf("Post-decoding validation passed\n");
    print_user(&decoded_user);
    
    return 0;
}

int main() {
    printf("Nanopb Validation Example\n");
    printf("========================\n\n");
    
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
