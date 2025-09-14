#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "user_profile.pb.h"

// Custom validation function for our user profile
bool validate_user_profile(const UserProfile *profile, char *error_msg, size_t error_size) {
    // Check required fields
    if (strlen(profile->username) == 0) {
        snprintf(error_msg, error_size, "Username is required");
        return false;
    }
    
    if (strlen(profile->email) == 0) {
        snprintf(error_msg, error_size, "Email is required");
        return false;
    }
    
    // Check username length (3-20 characters)
    size_t username_len = strlen(profile->username);
    if (username_len < 3) {
        snprintf(error_msg, error_size, "Username must be at least 3 characters");
        return false;
    }
    if (username_len > 20) {
        snprintf(error_msg, error_size, "Username must be at most 20 characters");
        return false;
    }
    
    // Check age range (13-120)
    if (profile->age < 13 || profile->age > 120) {
        snprintf(error_msg, error_size, "Age must be between 13 and 120");
        return false;
    }
    
    // Check email contains @
    if (strchr(profile->email, '@') == NULL) {
        snprintf(error_msg, error_size, "Email must contain @ symbol");
        return false;
    }
    
    // Check phone number format if provided
    if (strlen(profile->phone) > 0) {
        if (profile->phone[0] != '+') {
            snprintf(error_msg, error_size, "Phone number must start with +");
            return false;
        }
        size_t phone_len = strlen(profile->phone);
        if (phone_len < 10 || phone_len > 15) {
            snprintf(error_msg, error_size, "Phone number must be 10-15 characters");
            return false;
        }
    }
    
    // Check status is valid enum value
    if (profile->status < UserProfile_Status_INACTIVE || 
        profile->status > UserProfile_Status_PENDING) {
        snprintf(error_msg, error_size, "Status must be a valid enum value");
        return false;
    }
    
    // Check score range (0.0-100.0)
    if (profile->score < 0.0f || profile->score > 100.0f) {
        snprintf(error_msg, error_size, "Score must be between 0.0 and 100.0");
        return false;
    }
    
    // Check bio length if provided
    if (strlen(profile->bio) > 500) {
        snprintf(error_msg, error_size, "Bio must be at most 500 characters");
        return false;
    }
    
    return true;
}

void print_user_profile(const UserProfile *profile) {
    printf("User Profile:\n");
    printf("  Username: %s\n", profile->username);
    printf("  Age: %d\n", (int)profile->age);
    printf("  Email: %s\n", profile->email);
    if (strlen(profile->phone) > 0) {
        printf("  Phone: %s\n", profile->phone);
    }
    printf("  Status: ");
    switch (profile->status) {
        case UserProfile_Status_INACTIVE:
            printf("INACTIVE");
            break;
        case UserProfile_Status_ACTIVE:
            printf("ACTIVE");
            break;
        case UserProfile_Status_SUSPENDED:
            printf("SUSPENDED");
            break;
        case UserProfile_Status_PENDING:
            printf("PENDING");
            break;
        default:
            printf("UNKNOWN");
            break;
    }
    printf("\n");
    printf("  Score: %.2f\n", profile->score);
    if (strlen(profile->bio) > 0) {
        printf("  Bio: %s\n", profile->bio);
    }
    printf("\n");
}

// Callback function for tags array
bool tags_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    // This is a simplified callback - in a real implementation you'd handle
    // the repeated string field properly
    return true;
}

int test_user_profile_encoding() {
    printf("=== Testing User Profile Encoding/Decoding ===\n");
    
    // Create a user profile
    UserProfile profile = UserProfile_init_zero;
    strcpy(profile.username, "jane_smith");
    profile.age = 28;
    strcpy(profile.email, "jane@example.com");
    strcpy(profile.phone, "+1234567890");
    profile.status = UserProfile_Status_ACTIVE;
    profile.score = 88.5f;
    strcpy(profile.bio, "Software engineer passionate about embedded systems");
    
    // Set up tags callback (simplified)
    profile.tags.funcs.decode = tags_callback;
    profile.tags.arg = NULL;
    
    printf("Original profile:\n");
    print_user_profile(&profile);
    
    // Encode the message
    uint8_t buffer[512];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&ostream, UserProfile_fields, &profile)) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    
    printf("Encoded %zu bytes\n", ostream.bytes_written);
    
    // Decode the message
    UserProfile decoded_profile = UserProfile_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, ostream.bytes_written);
    
    if (!pb_decode(&istream, UserProfile_fields, &decoded_profile)) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    
    printf("Decoded profile:\n");
    print_user_profile(&decoded_profile);
    
    return 0;
}

int test_user_profile_validation() {
    printf("=== Testing User Profile Validation ===\n");
    
    char error_msg[256];
    
    // Test 1: Valid profile
    printf("Test 1: Valid profile\n");
    UserProfile valid_profile = UserProfile_init_zero;
    strcpy(valid_profile.username, "alice");
    valid_profile.age = 30;
    strcpy(valid_profile.email, "alice@example.com");
    strcpy(valid_profile.phone, "+1234567890");
    valid_profile.status = UserProfile_Status_ACTIVE;
    valid_profile.score = 92.0f;
    strcpy(valid_profile.bio, "Valid user profile");
    
    if (validate_user_profile(&valid_profile, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&valid_profile);
    
    // Test 2: Invalid username (too short)
    printf("Test 2: Invalid username (too short)\n");
    UserProfile invalid_profile1 = UserProfile_init_zero;
    strcpy(invalid_profile1.username, "ab");
    invalid_profile1.age = 25;
    strcpy(invalid_profile1.email, "ab@example.com");
    invalid_profile1.status = UserProfile_Status_ACTIVE;
    invalid_profile1.score = 75.0f;
    
    if (validate_user_profile(&invalid_profile1, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&invalid_profile1);
    
    // Test 3: Invalid age
    printf("Test 3: Invalid age\n");
    UserProfile invalid_profile2 = UserProfile_init_zero;
    strcpy(invalid_profile2.username, "bob");
    invalid_profile2.age = 5;  // Too young
    strcpy(invalid_profile2.email, "bob@example.com");
    invalid_profile2.status = UserProfile_Status_ACTIVE;
    invalid_profile2.score = 75.0f;
    
    if (validate_user_profile(&invalid_profile2, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&invalid_profile2);
    
    // Test 4: Invalid email (no @)
    printf("Test 4: Invalid email (no @)\n");
    UserProfile invalid_profile3 = UserProfile_init_zero;
    strcpy(invalid_profile3.username, "charlie");
    invalid_profile3.age = 35;
    strcpy(invalid_profile3.email, "charlie.example.com");  // Missing @
    invalid_profile3.status = UserProfile_Status_ACTIVE;
    invalid_profile3.score = 75.0f;
    
    if (validate_user_profile(&invalid_profile3, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&invalid_profile3);
    
    // Test 5: Invalid phone number
    printf("Test 5: Invalid phone number\n");
    UserProfile invalid_profile4 = UserProfile_init_zero;
    strcpy(invalid_profile4.username, "david");
    invalid_profile4.age = 28;
    strcpy(invalid_profile4.email, "david@example.com");
    strcpy(invalid_profile4.phone, "1234567890");  // Missing +
    invalid_profile4.status = UserProfile_Status_ACTIVE;
    invalid_profile4.score = 75.0f;
    
    if (validate_user_profile(&invalid_profile4, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&invalid_profile4);
    
    // Test 6: Invalid score
    printf("Test 6: Invalid score\n");
    UserProfile invalid_profile5 = UserProfile_init_zero;
    strcpy(invalid_profile5.username, "eve");
    invalid_profile5.age = 28;
    strcpy(invalid_profile5.email, "eve@example.com");
    invalid_profile5.status = UserProfile_Status_ACTIVE;
    invalid_profile5.score = 150.0f;  // Too high
    
    if (validate_user_profile(&invalid_profile5, error_msg, sizeof(error_msg))) {
        printf("✓ Validation passed\n");
    } else {
        printf("✗ Validation failed: %s\n", error_msg);
    }
    print_user_profile(&invalid_profile5);
    
    return 0;
}

int main() {
    printf("Advanced Nanopb Validation Example\n");
    printf("==================================\n\n");
    
    int result = 0;
    
    result += test_user_profile_encoding();
    printf("\n");
    
    result += test_user_profile_validation();
    
    if (result == 0) {
        printf("\n✓ All tests completed successfully!\n");
    } else {
        printf("\n✗ Some tests failed\n");
    }
    
    return result;
}
