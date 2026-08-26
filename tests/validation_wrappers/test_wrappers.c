/*
 * Test suite for google.protobuf wrapper types validation
 *
 * This tests validation of messages containing wrapper types like:
 * - google.protobuf.StringValue
 * - google.protobuf.Int32Value
 * - google.protobuf.BoolValue
 * - google.protobuf.BytesValue
 * - google.protobuf.DoubleValue
 *
 * Rules apply to the inner .value field, not the wrapper message itself.
 * Presence semantics: wrapper absent + rules -> PASS (unless required)
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

/* Include generated headers */
#include "wrappers_rules.pb.h"
#include "wrappers_rules_validate.h"

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

/* ========================================================================
 * STRING WRAPPER TESTS
 * ======================================================================== */
void test_string_wrapper_min_len(void)
{
    printf("\n=== StringValue wrapper min_len tests ===\n");
    
    /* Test: Valid string (length >= 3) */
    {
        TEST("StringValue min_len with valid value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_name = true;
        strcpy(msg.name.value, "abc");  /* 3 chars, meets min_len = 3 */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "String 'abc' should pass min_len=3");
    }
    
    /* Test: Invalid string (length < 3) */
    {
        TEST("StringValue min_len with too short value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_name = true;
        strcpy(msg.name.value, "ab");  /* 2 chars, violates min_len = 3 */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "String 'ab' should fail min_len=3");
        EXPECT_VIOLATION(viol, "string.min_len");
    }
    
    /* Test: Absent wrapper should pass (presence semantics) */
    {
        TEST("StringValue min_len with absent wrapper");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_name = false;  /* Wrapper absent */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Absent wrapper should pass validation");
    }
}

void test_string_wrapper_prefix(void)
{
    printf("\n=== StringValue wrapper prefix tests ===\n");
    
    /* Test: Valid prefix */
    {
        TEST("StringValue prefix with valid value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_code = true;
        strcpy(msg.code.value, "CODE_123");
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "String 'CODE_123' should pass prefix='CODE_'");
    }
    
    /* Test: Invalid prefix */
    {
        TEST("StringValue prefix with invalid value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_code = true;
        strcpy(msg.code.value, "INVALID123");
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "String 'INVALID123' should fail prefix='CODE_'");
        EXPECT_VIOLATION(viol, "string.prefix");
    }
}

void test_string_wrapper_email(void)
{
    printf("\n=== StringValue wrapper email tests ===\n");
    
    /* Test: Valid email */
    {
        TEST("StringValue email with valid value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_email = true;
        strcpy(msg.email.value, "test@example.com");
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Email 'test@example.com' should pass");
    }
    
    /* Test: Invalid email */
    {
        TEST("StringValue email with invalid value");
        StringWrapperTest msg = StringWrapperTest_init_zero;
        msg.has_email = true;
        strcpy(msg.email.value, "not-an-email");
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_StringWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "String 'not-an-email' should fail email validation");
        EXPECT_VIOLATION(viol, "string.email");
    }
}

/* ========================================================================
 * INT32 WRAPPER TESTS
 * ======================================================================== */
void test_int32_wrapper_gt(void)
{
    printf("\n=== Int32Value wrapper gt tests ===\n");
    
    /* Test: Valid value > 0 */
    {
        TEST("Int32Value gt with valid value");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_positive_number = true;
        msg.positive_number.value = 5;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Value 5 should pass gt=0");
    }
    
    /* Test: Invalid value = 0 (not > 0) */
    {
        TEST("Int32Value gt with boundary value");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_positive_number = true;
        msg.positive_number.value = 0;  /* Equal to, not greater than */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Value 0 should fail gt=0");
        EXPECT_VIOLATION(viol, "int32.gt");
    }
    
    /* Test: Invalid value < 0 */
    {
        TEST("Int32Value gt with negative value");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_positive_number = true;
        msg.positive_number.value = -1;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Value -1 should fail gt=0");
    }
    
    /* Test: Absent wrapper should pass */
    {
        TEST("Int32Value gt with absent wrapper");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_positive_number = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Absent wrapper should pass validation");
    }
}

void test_int32_wrapper_lte(void)
{
    printf("\n=== Int32Value wrapper lte tests ===\n");
    
    /* Test: Valid value <= 100 */
    {
        TEST("Int32Value lte with valid value");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_max_hundred = true;
        msg.max_hundred.value = 100;  /* Equal to max */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Value 100 should pass lte=100");
    }
    
    /* Test: Invalid value > 100 */
    {
        TEST("Int32Value lte with too large value");
        Int32WrapperTest msg = Int32WrapperTest_init_zero;
        msg.has_max_hundred = true;
        msg.max_hundred.value = 101;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_Int32WrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Value 101 should fail lte=100");
        EXPECT_VIOLATION(viol, "int32.lte");
    }
}

/* ========================================================================
 * BOOL WRAPPER TESTS
 * ======================================================================== */
void test_bool_wrapper(void)
{
    printf("\n=== BoolValue wrapper tests ===\n");
    
    /* Test: Valid true value */
    {
        TEST("BoolValue const with valid true");
        BoolWrapperTest msg = BoolWrapperTest_init_zero;
        msg.has_must_be_true = true;
        msg.must_be_true.value = true;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_BoolWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Bool true should pass const=true");
    }
    
    /* Test: Invalid false value (should be true) */
    {
        TEST("BoolValue const with invalid false");
        BoolWrapperTest msg = BoolWrapperTest_init_zero;
        msg.has_must_be_true = true;
        msg.must_be_true.value = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_BoolWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Bool false should fail const=true");
        EXPECT_VIOLATION(viol, "bool.const");
    }
    
    /* Test: Absent wrapper should pass */
    {
        TEST("BoolValue const with absent wrapper");
        BoolWrapperTest msg = BoolWrapperTest_init_zero;
        msg.has_must_be_true = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_BoolWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Absent wrapper should pass validation");
    }
}

/* ========================================================================
 * BYTES WRAPPER TESTS
 * ======================================================================== */
void test_bytes_wrapper_min_len(void)
{
    printf("\n=== BytesValue wrapper min_len tests ===\n");
    
    /* Test: Valid bytes length >= 4 */
    {
        TEST("BytesValue min_len with valid value");
        BytesWrapperTest msg = BytesWrapperTest_init_zero;
        msg.has_data = true;
        msg.data.value.size = 5;
        memcpy(msg.data.value.bytes, "12345", 5);
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_BytesWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Bytes of length 5 should pass min_len=4");
    }
    
    /* Test: Invalid bytes length < 4 */
    {
        TEST("BytesValue min_len with too short value");
        BytesWrapperTest msg = BytesWrapperTest_init_zero;
        msg.has_data = true;
        msg.data.value.size = 2;
        memcpy(msg.data.value.bytes, "12", 2);
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_BytesWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Bytes of length 2 should fail min_len=4");
        EXPECT_VIOLATION(viol, "bytes.min_len");
    }
}

/* ========================================================================
 * DOUBLE WRAPPER TESTS
 * ======================================================================== */
void test_double_wrapper_gt(void)
{
    printf("\n=== DoubleValue wrapper gt tests ===\n");
    
    /* Test: Valid positive value */
    {
        TEST("DoubleValue gt with valid positive");
        DoubleWrapperTest msg = DoubleWrapperTest_init_zero;
        msg.has_positive = true;
        msg.positive.value = 0.1;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_DoubleWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Value 0.1 should pass gt=0.0");
    }
    
    /* Test: Invalid zero value */
    {
        TEST("DoubleValue gt with zero");
        DoubleWrapperTest msg = DoubleWrapperTest_init_zero;
        msg.has_positive = true;
        msg.positive.value = 0.0;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_DoubleWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Value 0.0 should fail gt=0.0");
        EXPECT_VIOLATION(viol, "double.gt");
    }
}

/* ========================================================================
 * REQUIRED WRAPPER TESTS
 * ======================================================================== */
void test_required_wrapper(void)
{
    printf("\n=== Required wrapper tests ===\n");
    
    /* Test: Required wrapper present and valid */
    {
        TEST("Required wrapper present with valid value");
        RequiredWrapperTest msg = RequiredWrapperTest_init_zero;
        msg.has_required_name = true;
        strcpy(msg.required_name.value, "valid");
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_RequiredWrapperTest(&msg, &viol);
        EXPECT_VALID(result, "Present wrapper with valid value should pass");
    }
    
    /* Test: Required wrapper absent - should fail */
    {
        TEST("Required wrapper absent");
        RequiredWrapperTest msg = RequiredWrapperTest_init_zero;
        msg.has_required_name = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_RequiredWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Absent required wrapper should fail");
        EXPECT_VIOLATION(viol, "required");
    }
    
    /* Test: Required wrapper present but invalid value */
    {
        TEST("Required wrapper present with invalid value");
        RequiredWrapperTest msg = RequiredWrapperTest_init_zero;
        msg.has_required_name = true;
        msg.required_name.value[0] = '\0';  /* Empty string */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_RequiredWrapperTest(&msg, &viol);
        EXPECT_INVALID(result, "Present wrapper with empty string should fail min_len");
    }
}

/* ========================================================================
 * PRESENCE SEMANTICS TESTS  
 * ======================================================================== */
void test_presence_semantics(void)
{
    printf("\n=== Presence semantics tests ===\n");
    
    /* Test: All optional wrappers absent - should pass */
    {
        TEST("All optional wrappers absent");
        PresenceTest msg = PresenceTest_init_zero;
        msg.has_optional_name = false;
        msg.has_optional_count = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_PresenceTest(&msg, &viol);
        EXPECT_VALID(result, "All absent optional wrappers should pass");
    }
    
    /* Test: One present, one absent - rules only apply to present */
    {
        TEST("Mixed present/absent wrappers");
        PresenceTest msg = PresenceTest_init_zero;
        msg.has_optional_name = true;
        strcpy(msg.optional_name.value, "longname");  /* >= 5 chars, valid */
        msg.has_optional_count = false;  /* Absent, should pass */
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_PresenceTest(&msg, &viol);
        EXPECT_VALID(result, "Valid present + absent should pass");
    }
    
    /* Test: Present but invalid */
    {
        TEST("Present wrapper with invalid value");
        PresenceTest msg = PresenceTest_init_zero;
        msg.has_optional_name = true;
        strcpy(msg.optional_name.value, "ab");  /* < 5 chars, invalid */
        msg.has_optional_count = false;
        
        pb_violations_t viol;
        pb_violations_init(&viol);
        bool result = pb_validate_PresenceTest(&msg, &viol);
        EXPECT_INVALID(result, "Present wrapper with invalid value should fail");
        EXPECT_VIOLATION(viol, "string.min_len");
    }
}

/* ========================================================================
 * MAIN
 * ======================================================================== */
int main(void)
{
    printf("Wrapper Types Validation Test Suite\n");
    printf("====================================\n");
    
    /* String wrapper tests */
    test_string_wrapper_min_len();
    test_string_wrapper_prefix();
    test_string_wrapper_email();
    
    /* Int32 wrapper tests */
    test_int32_wrapper_gt();
    test_int32_wrapper_lte();
    
    /* Bool wrapper tests */
    test_bool_wrapper();
    
    /* Bytes wrapper tests */
    test_bytes_wrapper_min_len();
    
    /* Double wrapper tests */
    test_double_wrapper_gt();
    
    /* Required wrapper tests */
    test_required_wrapper();
    
    /* Presence semantics tests */
    test_presence_semantics();
    
    /* Summary */
    printf("\n====================================\n");
    printf("Test Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("====================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
