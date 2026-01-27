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

/* Helper to encode a NestedItem for callback testing */
static bool encode_nested_item(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg) {
    NestedItem *item = (NestedItem *)*arg;
    
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    if (!pb_encode_submessage(stream, &NestedItem_msg, item)) {
        return false;
    }
    
    return true;
}

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
    
    /* Test 1: Valid RootMessage with no nested items */
    TEST("Valid RootMessage with no nested items");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        
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
    
    /* Test 4: Valid RootMessage with valid nested item (callback field) */
    TEST("Valid RootMessage with one valid nested item");
    {
        /* Create the nested item */
        NestedItem item1 = NestedItem_init_zero;
        item1.item_id = 42;
        strcpy(item1.item_name, "Item42");
        
        /* Create root message with callback for nested_items */
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        msg.nested_items.funcs.encode = encode_nested_item;
        msg.nested_items.arg = &item1;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result == 0, "RootMessage with valid nested item");
    }
    
    /* Test 5: Invalid RootMessage with invalid nested item (item_id <= 0) */
    TEST("Invalid RootMessage with invalid nested item (item_id = 0)");
    {
        /* Create an invalid nested item */
        NestedItem item1 = NestedItem_init_zero;
        item1.item_id = 0;  /* Invalid: must be > 0 */
        strcpy(item1.item_name, "InvalidItem");
        
        /* Create root message */
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        msg.nested_items.funcs.encode = encode_nested_item;
        msg.nested_items.arg = &item1;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with invalid nested item");
    }
    
    /* Test 6: Invalid RootMessage with nested item having empty item_name */
    TEST("Invalid RootMessage with nested item having empty name");
    {
        /* Create nested item with invalid name */
        NestedItem item1 = NestedItem_init_zero;
        item1.item_id = 42;
        item1.item_name[0] = '\0';  /* Invalid: min_len is 1 */
        
        /* Create root message */
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        msg.nested_items.funcs.encode = encode_nested_item;
        msg.nested_items.arg = &item1;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with nested item having empty name");
    }
}

/*
 * Test that callback fields for strings and bytes work correctly
 */
static void test_string_bytes_callback(void) {
    printf("\n=== Testing String/Bytes Callback Validation ===\n");
    uint8_t buffer[512];
    size_t msg_len;
    int result;
    
    /* Helper to encode callback strings */
    typedef struct {
        const char *str;
    } string_encode_ctx;
    
    bool encode_callback_string(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg) {
        string_encode_ctx *ctx = (string_encode_ctx *)*arg;
        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }
        return pb_encode_string(stream, (const pb_byte_t *)ctx->str, strlen(ctx->str));
    }
    
    /* Test 1: Valid callback string (meets min_len requirement) */
    TEST("Valid RootMessage with valid callback_description");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        
        /* Wire the callback string encoder */
        string_encode_ctx str_ctx = {"This is a valid description that is long enough"};
        msg.callback_description.funcs.encode = encode_callback_string;
        msg.callback_description.arg = &str_ctx;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result == 0, "RootMessage with valid callback_description");
    }
    
    /* Test 2: Invalid callback string (too short - less than min_len=10) */
    TEST("Invalid RootMessage with callback_description too short");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        
        /* Wire the callback string encoder with short string */
        string_encode_ctx str_ctx = {"Short"};  /* Only 5 chars, min is 10 */
        msg.callback_description.funcs.encode = encode_callback_string;
        msg.callback_description.arg = &str_ctx;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with callback_description too short");
    }
    
    /* Test 3: Invalid callback string (too long - exceeds max_len=200) */
    TEST("Invalid RootMessage with callback_description too long");
    {
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        
        /* Wire the callback string encoder with very long string */
        char long_str[250];
        memset(long_str, 'A', 249);
        long_str[249] = '\0';  /* 249 chars, max is 200 */
        
        string_encode_ctx str_ctx = {long_str};
        msg.callback_description.funcs.encode = encode_callback_string;
        msg.callback_description.arg = &str_ctx;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result == 0, "RootMessage with callback_description too long");
    }
    
    /* Test 4: Validator can still validate static fields even with callback fields present */
    TEST("CallbackTestMessage validators work for non-callback fields");
    {
        CallbackTestMessage msg = CallbackTestMessage_init_zero;
        msg.static_field = 500;  /* Valid: 0 <= 500 <= 1000 */
        
        pb_violations_t violations = {0};
        bool valid = pb_validate_CallbackTestMessage(&msg, &violations);
        
        EXPECT_VALID(valid, "CallbackTestMessage with only static_field set");
    }
    
    TEST("CallbackTestMessage with invalid static_field");
    {
        CallbackTestMessage msg = CallbackTestMessage_init_zero;
        msg.static_field = 2000;  /* Invalid: > 1000 */
        
        pb_violations_t violations = {0};
        bool valid = pb_validate_CallbackTestMessage(&msg, &violations);
        
        EXPECT_INVALID(valid, "CallbackTestMessage with static_field > 1000");
    }
}

/*
 * Test that callbacks are properly wired before decode
 */
static void test_callback_wiring(void) {
    printf("\n=== Testing Callback Wiring ===\n");
    
    /* Test indirectly by checking that nested validation occurs during filter_udp */
    TEST("Callback wiring via filter_udp with nested validation");
    {
        uint8_t buffer[512];
        size_t msg_len;
        int result;
        
        /* Create a message with an invalid nested item to verify callbacks are wired */
        NestedItem item1 = NestedItem_init_zero;
        item1.item_id = 0;  /* Invalid: must be > 0 */
        strcpy(item1.item_name, "InvalidItem");
        
        RootMessage msg = RootMessage_init_zero;
        msg.root_id = 123;
        strcpy(msg.name, "Test Root");
        msg.nested_items.funcs.encode = encode_nested_item;
        msg.nested_items.arg = &item1;
        
        assert(encode_message(&RootMessage_msg, &msg, buffer, sizeof(buffer), &msg_len));
        
        result = filter_udp(NULL, buffer, msg_len);
        
        /* If callbacks are properly wired, nested validation should fail */
        if (result != 0) {
            tests_passed++;
            printf("    [PASS] Callback validation detected invalid nested item\n");
        } else {
            tests_failed++;
            printf("    [FAIL] Callback validation did not detect invalid nested item\n");
        }
    }
}

int main(void) {
    printf("=== Callback Validation Test Suite ===\n");
    
    test_repeated_submessage_callback();
    test_string_bytes_callback();
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
