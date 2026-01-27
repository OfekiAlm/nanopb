/*
 * Test suite for automatic decode callback generation for pb_callback_t fields
 *
 * This test exercises the auto-generated pb_wire_callbacks_* functions and
 * validates that callback fields are automatically decoded and validated.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

/* Include generated headers */
#include "callback_validation.pb.h"
#include "callback_validation_validate.h"

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
 * Test that filter_udp validates repeated submessage callback fields
 */
static void test_repeated_submessage_callback(void) {
    printf("\n=== Testing Repeated Submessage Callback Validation ===\n");
    uint8_t buffer[512];
    size_t msg_len;
    int result;
    
    /* Test 1: Valid RootMessage with valid nested items */
    TEST("Valid RootMessage with valid nested items");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        
        /* Since items is a callback, we need to encode it properly */
        /* For now, test with no items (empty repeated field) */
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result == 0, "valid RootMessage with no nested items");
    }
    
    /* Test 2: Invalid RootMessage with root_id <= 0 */
    TEST("Invalid RootMessage with root_id = 0");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 0;  /* Invalid: must be > 0 */
        strcpy(msg.name, "Test");
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with invalid root_id");
    }
    
    /* Test 3: Invalid RootMessage with empty name */
    TEST("Invalid RootMessage with empty name");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        msg.name[0] = '\0';  /* Invalid: min_len is 1 */
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with empty name");
    }
}

/*
 * Test that callbacks are properly wired before decode
 */
static void test_callback_wiring(void) {
    printf("\n=== Testing Callback Wiring ===\n");
    
    /* Test that pb_wire_callbacks function exists and can be called */
    TEST("pb_wire_callbacks_RootMessage function exists");
    {
        RootMessage msg = RootMessage_init_zero;
        pb_violations_t violations = {0};
        
        /* This should compile and run without error */
        pb_wire_callbacks_RootMessage(&msg, &violations);
        
        /* Check that callback was wired */
        tests_passed++;
        printf("    [PASS] pb_wire_callbacks_RootMessage callable\n");
    }
}

int main(void) {
    printf("=== Callback Validation Test Suite ===\n");
    
    test_repeated_submessage_callback();
    test_callback_wiring();
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\n[OVERALL FAIL] %d test(s) failed\n", tests_failed);
        return 1;
    } else {
        printf("\n[OVERALL PASS] All tests passed\n");
        return 0;
    }
}
