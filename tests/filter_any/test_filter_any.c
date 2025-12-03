/*
 * Test google.protobuf.Any validation with nanopb generated validators
 * 
 * This test exercises validation of messages with Any fields:
 * - Packs payload messages into Any fields
 * - Calls generated pb_validate_* functions directly
 * - Tests any.in (whitelist) and any.not_in (blacklist) validation
 * - Asserts valid Any payloads pass and invalid ones fail with correct violations
 *
 * Note: This test uses strcpy() for string literals that are known to be
 * within the nanopb max_size bounds (128 or 256 bytes). All test strings
 * are short email addresses and names well under these limits.
 * The pack_any() function properly uses strncpy() with null termination
 * for type_url since it can come from external input.
 * This is consistent with existing test patterns in the repository.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

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

#define EXPECT_VALID(result, msg) do { \
    if (result) { \
        tests_passed++; \
        printf("    [PASS] Valid message accepted\n"); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected valid, got invalid: %s\n", msg); \
    } \
} while(0)

#define EXPECT_INVALID(result, msg) do { \
    if (!result) { \
        tests_passed++; \
        printf("    [PASS] Invalid message rejected\n"); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected invalid, got valid: %s\n", msg); \
    } \
} while(0)

#define EXPECT_VIOLATION(viol, expected_id) do { \
    if (pb_violations_has_any(&viol) && \
        viol.violations[0].constraint_id != NULL && \
        strcmp(viol.violations[0].constraint_id, expected_id) == 0) { \
        tests_passed++; \
        printf("    [PASS] Got expected violation: %s\n", expected_id); \
    } else { \
        tests_failed++; \
        const char *got = (pb_violations_has_any(&viol) && viol.violations[0].constraint_id) ? \
                          viol.violations[0].constraint_id : "(none)"; \
        printf("    [FAIL] Expected violation '%s', got '%s'\n", expected_id, got); \
    } \
} while(0)

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

int main(void)
{
    printf("===== Testing Any validation =====\n\n");

    pb_violations_t viol;
    bool ok;

    /* ===== Test FilterAnyAllowed (any.in validation) ===== */
    printf("\n--- Testing FilterAnyAllowed (any.in) ---\n\n");

    /* Test 1: Valid UserInfo in Any (allowed type) */
    TEST("Valid Any - UserInfo (allowed type) with valid fields");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 123;  /* > 0: valid */
        strcpy(user.email, "user@example.com");  /* valid email */
        user.age = 25;  /* 0-150: valid */

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/UserInfo", &UserInfo_msg, &user);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyAllowed(&msg, &viol);
        EXPECT_VALID(ok, "valid UserInfo in allowed Any");
    }

    /* Test 2: Valid ProductInfo in Any (allowed type) */
    TEST("Valid Any - ProductInfo (allowed type) with valid fields");
    {
        ProductInfo product = ProductInfo_init_zero;
        product.product_id = 456;  /* > 0: valid */
        strcpy(product.name, "Widget");  /* >= 1 char: valid */
        product.price = 19.99;  /* >= 0: valid */

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/ProductInfo", &ProductInfo_msg, &product);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyAllowed(&msg, &viol);
        EXPECT_VALID(ok, "valid ProductInfo in allowed Any");
    }

    /* Test 3: Invalid - OrderInfo not in allowed list */
    TEST("Invalid Any - OrderInfo (not in allowed list)");
    {
        OrderInfo order = OrderInfo_init_zero;
        order.order_id = 789;
        order.total = 99.99;

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/OrderInfo", &OrderInfo_msg, &order);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyAllowed(&msg, &viol);
        EXPECT_INVALID(ok, "OrderInfo not in allowed type list should fail");
        EXPECT_VIOLATION(viol, "any.in");
    }

    /* Test 4: Invalid - Unknown type not in allowed list */
    TEST("Invalid Any - Unknown type (not in allowed list)");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 100;
        strcpy(user.email, "test@example.com");
        user.age = 30;

        FilterAnyAllowed msg = FilterAnyAllowed_init_zero;
        msg.has_payload = true;
        /* Use wrong type_url */
        pack_any(&msg.payload, "type.googleapis.com/UnknownType", &UserInfo_msg, &user);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyAllowed(&msg, &viol);
        EXPECT_INVALID(ok, "unknown type not in allowed list should fail");
        EXPECT_VIOLATION(viol, "any.in");
    }

    /* ===== Test FilterAnyDisallowed (any.not_in validation) ===== */
    printf("\n--- Testing FilterAnyDisallowed (any.not_in) ---\n\n");

    /* Test 5: Valid - UserInfo (not in disallowed list) */
    TEST("Valid Any - UserInfo (not in disallowed list)");
    {
        UserInfo user = UserInfo_init_zero;
        user.user_id = 200;
        strcpy(user.email, "admin@example.com");
        user.age = 40;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/UserInfo", &UserInfo_msg, &user);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyDisallowed(&msg, &viol);
        EXPECT_VALID(ok, "UserInfo not in disallowed list should pass");
    }

    /* Test 6: Valid - ProductInfo (not in disallowed list) */
    TEST("Valid Any - ProductInfo (not in disallowed list)");
    {
        ProductInfo product = ProductInfo_init_zero;
        product.product_id = 999;
        strcpy(product.name, "Gadget");
        product.price = 49.99;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/ProductInfo", &ProductInfo_msg, &product);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyDisallowed(&msg, &viol);
        EXPECT_VALID(ok, "ProductInfo not in disallowed list should pass");
    }

    /* Test 7: Invalid - OrderInfo in disallowed list */
    TEST("Invalid Any - OrderInfo (in disallowed list)");
    {
        OrderInfo order = OrderInfo_init_zero;
        order.order_id = 555;
        order.total = 199.99;

        FilterAnyDisallowed msg = FilterAnyDisallowed_init_zero;
        msg.has_payload = true;
        pack_any(&msg.payload, "type.googleapis.com/OrderInfo", &OrderInfo_msg, &order);

        pb_violations_init(&viol);
        ok = pb_validate_FilterAnyDisallowed(&msg, &viol);
        EXPECT_INVALID(ok, "OrderInfo in disallowed list should fail");
        EXPECT_VIOLATION(viol, "any.not_in");
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
