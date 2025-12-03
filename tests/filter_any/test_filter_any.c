/*
 * Test filter_tcp/filter_udp validation with google.protobuf.Any fields
 * 
 * This test exercises the full validation flow through proto_filter with Any:
 * - Packs payload messages into Any fields
 * - Serializes them to bytes
 * - Calls filter_tcp/filter_udp to decode and validate
 * - Asserts valid Any payloads pass and invalid ones fail
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
#include "filter_any.pb.h"
#include "filter_any_validate.h"
#include "google/protobuf/any.pb.h"

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

/* Validator adapters for proto_filter */
static bool validate_filter_any_allowed(const void *msg, pb_violations_t *violations)
{
    return pb_validate_FilterAnyAllowed((const FilterAnyAllowed *)msg, violations);
}

static bool validate_filter_any_disallowed(const void *msg, pb_violations_t *violations)
{
    return pb_validate_FilterAnyDisallowed((const FilterAnyDisallowed *)msg, violations);
}

/* proto_filter specs */
static const proto_filter_spec_t filter_any_allowed_spec = {
    .msg_desc = &FilterAnyAllowed_msg,
    .msg_size = sizeof(FilterAnyAllowed),
    .validate = validate_filter_any_allowed,
    .prepare_decode = NULL
};

static const proto_filter_spec_t filter_any_disallowed_spec = {
    .msg_desc = &FilterAnyDisallowed_msg,
    .msg_size = sizeof(FilterAnyDisallowed),
    .validate = validate_filter_any_disallowed,
    .prepare_decode = NULL
};

/* Helper to pack a message into Any */
static bool pack_any(google_protobuf_Any *any, const char *type_url, 
                     const pb_msgdesc_t *msg_desc, const void *msg)
{
    uint8_t buffer[512];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    if (!pb_encode(&stream, msg_desc, msg))
    {
        printf("      [ERROR] Failed to encode message for Any\n");
        return false;
    }
    
    /* Set type_url */
    strncpy(any->type_url, type_url, sizeof(any->type_url) - 1);
    any->type_url[sizeof(any->type_url) - 1] = '\0';
    
    /* Set value bytes */
    if (stream.bytes_written > sizeof(any->value.bytes))
    {
        printf("      [ERROR] Encoded message too large for Any.value\n");
        return false;
    }
    
    memcpy(any->value.bytes, buffer, stream.bytes_written);
    any->value.size = (pb_size_t)stream.bytes_written;
    
    return true;
}

/* Helper to encode a FilterAnyAllowed message and return buffer + size */
static bool encode_allowed_message(const FilterAnyAllowed *msg, uint8_t *buffer, 
                                    size_t buffer_size, size_t *out_size)
{
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    if (!pb_encode(&stream, &FilterAnyAllowed_msg, msg))
    {
        printf("      [ERROR] Encoding FilterAnyAllowed failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *out_size = stream.bytes_written;
    return true;
}

/* Helper to encode a FilterAnyDisallowed message and return buffer + size */
static bool encode_disallowed_message(const FilterAnyDisallowed *msg, uint8_t *buffer,
                                       size_t buffer_size, size_t *out_size)
{
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    if (!pb_encode(&stream, &FilterAnyDisallowed_msg, msg))
    {
        printf("      [ERROR] Encoding FilterAnyDisallowed failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *out_size = stream.bytes_written;
    return true;
}

int main(void)
{
    printf("===== Testing filter_tcp/filter_udp with Any validation =====\n\n");

    uint8_t buffer[1024];
    size_t size;
    int result;

    /* ===== Test FilterAnyAllowed (any.in validation) ===== */
    printf("\n--- Testing FilterAnyAllowed (any.in) ---\n\n");

    /* Register the allowed filter spec */
    proto_filter_register(&filter_any_allowed_spec);

    /* Test 1: Valid UserInfo in Any (allowed type) */
    TEST("Valid Any - UserInfo (allowed type) with valid fields");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 123;  /* > 0: valid */
        strcpy(user.email, "user@example.com");  /* valid email */
        user.age = 25;  /* 0-150: valid */

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/UserInfo", &UserInfo_msg, &user))
        {
            if (encode_allowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_tcp(NULL, (char *)buffer, size, true);
                EXPECT_FILTER_OK(result, "valid UserInfo in allowed Any");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* Test 2: Valid ProductInfo in Any (allowed type) */
    TEST("Valid Any - ProductInfo (allowed type) with valid fields");
    {
        ProductInfo product = ProductInfo_init_zero;
        product.product_id = 456;  /* > 0: valid */
        strcpy(product.name, "Widget");  /* >= 1 char: valid */
        product.price = 19.99;  /* >= 0: valid */

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/ProductInfo", &ProductInfo_msg, &product))
        {
            if (encode_allowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_udp(NULL, (char *)buffer, size, false);
                EXPECT_FILTER_OK(result, "valid ProductInfo in allowed Any");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* Test 3: Invalid - OrderInfo not in allowed list */
    TEST("Invalid Any - OrderInfo (not in allowed list)");
    {
        OrderInfo order = OrderInfo_init_zero;
        order.order_id = 789;
        order.total = 99.99;

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/OrderInfo", &OrderInfo_msg, &order))
        {
            if (encode_allowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_tcp(NULL, (char *)buffer, size, true);
                EXPECT_FILTER_INVALID(result, "OrderInfo not in allowed type list should fail");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* Test 4: Invalid - Unknown type not in allowed list */
    TEST("Invalid Any - Unknown type (not in allowed list)");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 100;
        strcpy(user.email, "test@example.com");
        user.age = 30;

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        /* Use wrong type_url */
        if (pack_any(&msg.payload, "type.googleapis.com/UnknownType", &UserInfo_msg, &user))
        {
            if (encode_allowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_tcp(NULL, (char *)buffer, size, true);
                EXPECT_FILTER_INVALID(result, "unknown type not in allowed list should fail");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* ===== Test FilterAnyDisallowed (any.not_in validation) ===== */
    printf("\n--- Testing FilterAnyDisallowed (any.not_in) ---\n\n");

    /* Register the disallowed filter spec */
    proto_filter_register(&filter_any_disallowed_spec);

    /* Test 5: Valid - UserInfo (not in disallowed list) */
    TEST("Valid Any - UserInfo (not in disallowed list)");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 200;
        strcpy(user.email, "admin@example.com");
        user.age = 40;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/UserInfo", &UserInfo_msg, &user))
        {
            if (encode_disallowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_tcp(NULL, (char *)buffer, size, true);
                EXPECT_FILTER_OK(result, "UserInfo not in disallowed list should pass");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* Test 6: Valid - ProductInfo (not in disallowed list) */
    TEST("Valid Any - ProductInfo (not in disallowed list)");
    {
        ProductInfo product = ProductInfo_init_zero;
        product.product_id = 999;
        strcpy(product.name, "Gadget");
        product.price = 49.99;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/ProductInfo", &ProductInfo_msg, &product))
        {
            if (encode_disallowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_udp(NULL, (char *)buffer, size, false);
                EXPECT_FILTER_OK(result, "ProductInfo not in disallowed list should pass");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
        }
    }

    /* Test 7: Invalid - OrderInfo in disallowed list */
    TEST("Invalid Any - OrderInfo (in disallowed list)");
    {
        OrderInfo order = OrderInfo_init_zero;
        order.order_id = 555;
        order.total = 199.99;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;  /* Set the has_ field */
        if (pack_any(&msg.payload, "type.googleapis.com/OrderInfo", &OrderInfo_msg, &order))
        {
            if (encode_disallowed_message(&msg, buffer, sizeof(buffer), &size))
            {
                result = filter_tcp(NULL, (char *)buffer, size, true);
                EXPECT_FILTER_INVALID(result, "OrderInfo in disallowed list should fail");
            }
            else
            {
                tests_failed++;
                printf("    [FAIL] Failed to encode message\n");
            }
        }
        else
        {
            tests_failed++;
            printf("    [FAIL] Failed to pack Any\n");
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
