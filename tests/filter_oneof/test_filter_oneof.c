/*
 * Test suite for filter_tcp/filter_udp with oneof envelope pattern
 *
 * This test exercises the filter functions generated with --envelope-mode=oneof
 * to validate messages using the opcode + oneof pattern.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

/* Include generated headers */
#include "filter_oneof.pb.h"
#include "filter_oneof_validate.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper macros */
#define TEST(name) do { \
    printf("  Testing: %s\n", name); \
} while(0)

#define EXPECT_VALID(result, msg) do { \
    if (result) { \
        tests_passed++; \
        printf("    [PASS] Valid message accepted: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected valid, got invalid: %s\n", msg); \
    } \
} while(0)

#define EXPECT_INVALID(result, msg) do { \
    if (!result) { \
        tests_passed++; \
        printf("    [PASS] Invalid message rejected: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected invalid, got valid: %s\n", msg); \
    } \
} while(0)

/* Helper function to encode a message to a buffer */
static bool encode_message(const pb_msgdesc_t *fields, const void *src_struct, 
                          uint8_t *buffer, size_t buffer_size, size_t *msg_len) {
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    bool status = pb_encode(&stream, fields, src_struct);
    if (status) {
        *msg_len = stream.bytes_written;
    }
    return status;
}

/*
 * Test valid messages with different oneof variants
 */
static void test_valid_messages(void) {
    printf("\n=== Testing Valid Messages ===\n");
    uint8_t buffer[256];
    size_t msg_len;
    pb_violations_t viol;
    bool ok;
    
    /* Test 1: Valid auth message */
    TEST("Valid auth message with username >= 3 chars");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "auth message with valid username");
    }
    
    /* Test 2: Valid data message */
    TEST("Valid data message with non-negative value");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = 42;
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "data message with valid value");
    }
    
    /* Test 3: Valid status message */
    TEST("Valid status message with nested validation");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 200;
        strcpy(msg.payload.status.status_message, "OK");
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "status message with valid nested payload");
    }
}

/*
 * Test invalid messages with validation rule violations
 */
static void test_invalid_messages(void) {
    printf("\n=== Testing Invalid Messages ===\n");
    uint8_t buffer[256];
    size_t msg_len;
    pb_violations_t viol;
    bool ok;
    
    /* Test 1: Invalid opcode */
    TEST("Invalid opcode (out of range)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 0;  /* Invalid: must be >= 1 */
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "opcode out of range");
    }
    
    /* Test 2: Invalid auth username (too short) */
    TEST("Invalid auth username (< 3 chars)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "ab");  /* Invalid: min_len is 3 */
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "username too short");
    }
    
    /* Test 3: Invalid data value (negative) */
    TEST("Invalid data value (negative)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = -1;  /* Invalid: must be >= 0 */
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "negative data value");
    }
    
    /* Test 4: Invalid status code (out of range) */
    TEST("Invalid status code (> 999)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 1000;  /* Invalid: must be <= 999 */
        strcpy(msg.payload.status.status_message, "Error");
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "status code out of range");
    }
    
    /* Test 5: Invalid status message (non-ASCII) */
    TEST("Invalid status message (non-ASCII)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 200;
        strcpy(msg.payload.status.status_message, "Error\xC2\xA9");  /* Contains non-ASCII */
        
        /* Encode to buffer */
        assert(encode_message(&FilterOneofMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        /* Validate directly */
        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "non-ASCII status message");
    }
}

int main(void) {
    printf("Filter Oneof Validation Tests\n");
    printf("==============================\n");
    
    test_valid_messages();
    test_invalid_messages();
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    } else {
        printf("\nSUCCESS: All tests passed\n");
        return 0;
    }
}
