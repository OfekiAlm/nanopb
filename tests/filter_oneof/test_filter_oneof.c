/*
 * Test filter_tcp/filter_udp validation with oneof-based messages
 * 
 * This test exercises the full validation flow through proto_filter:
 * - Constructs messages with header opcode + oneof payload
 * - Serializes them to bytes
 * - Calls filter_tcp/filter_udp to decode and validate
 * - Asserts valid cases pass and invalid cases fail
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"
#include "proto_filter.h"

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

#define EXPECT_FILTER_OK(result, msg) do { \
    if (result == PROTO_FILTER_OK) { \
        tests_passed++; \
        printf("    [PASS] Valid message accepted: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected PROTO_FILTER_OK, got %d: %s\n", result, msg); \
    } \
} while(0)

#define EXPECT_FILTER_INVALID(result, msg) do { \
    if (result == PROTO_FILTER_ERR_DECODE) { \
        tests_passed++; \
        printf("    [PASS] Invalid message rejected: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected PROTO_FILTER_ERR_DECODE, got %d: %s\n", result, msg); \
    } \
} while(0)

/* Validator adapter for proto_filter */
static bool validate_filter_oneof_message(const void *msg, pb_violations_t *violations)
{
    return pb_validate_FilterOneofMessage((const FilterOneofMessage *)msg, violations);
}

/* proto_filter spec for FilterOneofMessage */
static const proto_filter_spec_t filter_oneof_spec = {
    .msg_desc = &FilterOneofMessage_msg,
    .msg_size = sizeof(FilterOneofMessage),
    .validate = validate_filter_oneof_message,
    .prepare_decode = NULL
};

/* Helper to encode a message and return buffer + size */
static bool encode_message(const FilterOneofMessage *msg, uint8_t *buffer, size_t buffer_size, size_t *out_size)
{
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    if (!pb_encode(&stream, &FilterOneofMessage_msg, msg))
    {
        printf("      [ERROR] Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *out_size = stream.bytes_written;
    return true;
}

int main(void)
{
    printf("===== Testing filter_tcp/filter_udp with oneof validation =====\n\n");

    /* Register the filter spec */
    proto_filter_register(&filter_oneof_spec);

    uint8_t buffer[1024];
    size_t size;
    int result;

    /* Test 1: Valid auth_username (opcode=1) */
    TEST("Valid auth_username - opcode 1 with valid username (>= 3 chars)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");  /* >= 3 chars: valid */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_tcp(NULL, (char *)buffer, size, true);
            EXPECT_FILTER_OK(result, "valid auth_username with good length");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 2: Invalid auth_username - too short */
    TEST("Invalid auth_username - username too short (< 3 chars)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "ab");  /* < 3 chars: invalid */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_tcp(NULL, (char *)buffer, size, true);
            EXPECT_FILTER_INVALID(result, "username too short should fail validation");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 3: Invalid auth_username - empty string */
    TEST("Invalid auth_username - empty username");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "");  /* empty: invalid */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_tcp(NULL, (char *)buffer, size, true);
            EXPECT_FILTER_INVALID(result, "empty username should fail validation");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 4: Valid data_value (opcode=2) */
    TEST("Valid data_value - opcode 2 with non-negative value");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = 42;  /* >= 0: valid */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_udp(NULL, (char *)buffer, size, false);
            EXPECT_FILTER_OK(result, "valid data_value with non-negative value");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 5: Invalid data_value - negative value */
    TEST("Invalid data_value - negative value (< 0)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = -10;  /* < 0: invalid */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_udp(NULL, (char *)buffer, size, false);
            EXPECT_FILTER_INVALID(result, "negative value should fail validation");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 6: Valid StatusPayload (opcode=3) - nested message 
     * NOTE: The current nanopb validator generator doesn't automatically
     * validate nested messages within oneofs. This test demonstrates that
     * the message structure can contain nested messages, but validation
     * of such nested messages would require manual validation code.
     */
    TEST("Valid StatusPayload - opcode 3 with nested message (validation limitation)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 200;  /* 0-999 range */
        strcpy(msg.payload.status.status_message, "OK");  /* ASCII */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_tcp(NULL, (char *)buffer, size, true);
            /* This passes because nested message validation is not auto-generated */
            EXPECT_FILTER_OK(result, "nested message in oneof (no auto-validation)");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 7: Demonstrate nested message validation can be called manually */
    TEST("Manual validation of StatusPayload");
    {
        StatusPayload status = StatusPayload_init_zero;
        status.status_code = 1000;  /* > 999: invalid */
        strcpy(status.status_message, "Error");

        pb_violations_t viol;
        pb_violations_init(&viol);
        bool ok = pb_validate_StatusPayload(&status, &viol);
        
        if (!ok) {
            tests_passed++;
            printf("    [PASS] Manual validation correctly rejects invalid StatusPayload\n");
        } else {
            tests_failed++;
            printf("    [FAIL] Manual validation should have rejected invalid StatusPayload\n");
        }
    }

    /* Test 8: Demonstrate valid StatusPayload via manual validation */
    TEST("Manual validation of valid StatusPayload");
    {
        StatusPayload status = StatusPayload_init_zero;
        status.status_code = 200;  /* 0-999: valid */
        strcpy(status.status_message, "OK");  /* ASCII: valid */

        pb_violations_t viol;
        pb_violations_init(&viol);
        bool ok = pb_validate_StatusPayload(&status, &viol);
        
        if (ok) {
            tests_passed++;
            printf("    [PASS] Manual validation correctly accepts valid StatusPayload\n");
        } else {
            tests_failed++;
            printf("    [FAIL] Manual validation should have accepted valid StatusPayload\n");
        }
    }

    /* Test 9: Invalid opcode */
    TEST("Invalid opcode - out of range (opcode=0)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 0;  /* < 1: invalid */
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_tcp(NULL, (char *)buffer, size, true);
            EXPECT_FILTER_INVALID(result, "opcode out of range should fail validation");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Test 10: Valid edge case - opcode at boundary */
    TEST("Valid edge case - opcode at upper boundary (3)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;  /* max valid value */
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 0;  /* min valid value */
        strcpy(msg.payload.status.status_message, "");  /* empty is valid ASCII */

        if (encode_message(&msg, buffer, sizeof(buffer), &size))
        {
            result = filter_udp(NULL, (char *)buffer, size, true);
            EXPECT_FILTER_OK(result, "boundary values should pass validation");
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to encode message\n");
        }
    }

    /* Summary */
    printf("\n===== Test Summary =====\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed > 0)
    {
        printf("\n[FAIL] Some tests failed\n");
        return 1;
    }
    else
    {
        printf("\n[PASS] All tests passed\n");
        return 0;
    }
}
