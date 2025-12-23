/*
 * Test oneof validation with nanopb generated validators
 * 
 * This test exercises validation of messages with oneof fields:
 * - Constructs messages with header opcode + oneof payload
 * - Calls generated pb_validate_* functions directly
 * - Asserts valid cases pass and invalid cases fail with correct violations
 *
 * Note: This test uses strcpy() for string literals that are known to be
 * within the nanopb max_size bounds (64 or 128 bytes). All test strings
 * are short literals ("alice", "OK", etc.) well under these limits.
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

int main(void)
{
    printf("===== Testing oneof validation =====\n\n");

    pb_violations_t viol;
    bool ok;

    /* Test 1: Valid auth_username (opcode=1) */
    TEST("Valid auth_username - opcode 1 with valid username (>= 3 chars)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");  /* >= 3 chars: valid */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "valid auth_username with good length");
    }

    /* Test 2: Invalid auth_username - too short */
    TEST("Invalid auth_username - username too short (< 3 chars)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "ab");  /* < 3 chars: invalid */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "username too short should fail validation");
        EXPECT_VIOLATION(viol, "string.min_len");
    }

    /* Test 3: Invalid auth_username - empty string */
    TEST("Invalid auth_username - empty username");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 1;
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "");  /* empty: invalid */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "empty username should fail validation");
        EXPECT_VIOLATION(viol, "string.min_len");
    }

    /* Test 4: Valid data_value (opcode=2) */
    TEST("Valid data_value - opcode 2 with non-negative value");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = 42;  /* >= 0: valid */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "valid data_value with non-negative value");
    }

    /* Test 5: Invalid data_value - negative value */
    TEST("Invalid data_value - negative value (< 0)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 2;
        msg.which_payload = FilterOneofMessage_data_value_tag;
        msg.payload.data_value = -10;  /* < 0: invalid */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "negative value should fail validation");
        EXPECT_VIOLATION(viol, "int32.gte");
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

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        /* This passes because nested message validation is not auto-generated */
        EXPECT_VALID(ok, "nested message in oneof (no auto-validation)");
    }

    /* Test 7: Demonstrate nested message validation can be called manually */
    TEST("Manual validation of StatusPayload - invalid");
    {
        StatusPayload status = StatusPayload_init_zero;
        status.status_code = 1000;  /* > 999: invalid */
        strcpy(status.status_message, "Error");

        pb_violations_init(&viol);
        ok = pb_validate_StatusPayload(&status, &viol);
        EXPECT_INVALID(ok, "StatusPayload with out-of-range status_code");
        EXPECT_VIOLATION(viol, "int32.lte");
    }

    /* Test 8: Demonstrate valid StatusPayload via manual validation */
    TEST("Manual validation of valid StatusPayload");
    {
        StatusPayload status = StatusPayload_init_zero;
        status.status_code = 200;  /* 0-999: valid */
        strcpy(status.status_message, "OK");  /* ASCII: valid */

        pb_violations_init(&viol);
        ok = pb_validate_StatusPayload(&status, &viol);
        EXPECT_VALID(ok, "valid StatusPayload");
    }

    /* Test 9: Invalid opcode */
    TEST("Invalid opcode - out of range (opcode=0)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 0;  /* < 1: invalid */
        msg.which_payload = FilterOneofMessage_auth_username_tag;
        strcpy(msg.payload.auth_username, "alice");

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_INVALID(ok, "opcode out of range should fail validation");
        EXPECT_VIOLATION(viol, "int32.gte");
    }

    /* Test 10: Valid edge case - opcode at boundary */
    TEST("Valid edge case - opcode at upper boundary (3)");
    {
        FilterOneofMessage msg = FilterOneofMessage_init_zero;
        msg.opcode = 3;  /* max valid value */
        msg.which_payload = FilterOneofMessage_status_tag;
        msg.payload.status.status_code = 0;  /* min valid value */
        strcpy(msg.payload.status.status_message, "");  /* empty is valid ASCII */

        pb_violations_init(&viol);
        ok = pb_validate_FilterOneofMessage(&msg, &viol);
        EXPECT_VALID(ok, "boundary values should pass validation");
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
