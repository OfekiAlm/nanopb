/*
 * Test suite for verifying that generated validation code uses PB_CHECK_* macros
 * from pb_filter_macros.h
 *
 * This test exercises the new validation macros:
 * - PB_VALIDATE_NUMERIC_GTE (uses PB_CHECK_MIN)
 * - PB_VALIDATE_NUMERIC_LTE (uses PB_CHECK_MAX)
 * - PB_VALIDATE_NUMERIC_GT (uses PB_CHECK_GT)
 * - PB_VALIDATE_NUMERIC_LT (uses PB_CHECK_LT)
 * - PB_VALIDATE_NUMERIC_EQ (uses PB_CHECK_EQ)
 * - PB_VALIDATE_ONEOF_NUMERIC_* variants
 * - PB_VALIDATE_REPEATED_ITEMS_* variants
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_validate.h"
#include "pb_filter_macros.h"

/* Include generated headers */
#include "filter_macros.pb.h"
#include "filter_macros_validate.h"

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
    if (!(result)) { \
        tests_passed++; \
        printf("    [PASS] Invalid message rejected: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected invalid, got valid: %s\n", msg); \
    } \
} while(0)

/*
 * Test PB_VALIDATE_NUMERIC_GTE (uses PB_CHECK_MIN internally)
 */
static void test_numeric_gte(void) {
    printf("\n=== Testing PB_VALIDATE_NUMERIC_GTE (PB_CHECK_MIN) ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid value: value_gte = 10 (gte = 10) */
    TEST("value_gte = 10 (equal to limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_VALID(result, "value_gte at limit");
    }
    
    /* Test valid value: value_gte = 100 (gte = 10) */
    TEST("value_gte = 100 (above limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 100;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_VALID(result, "value_gte above limit");
    }
    
    /* Test invalid value: value_gte = 9 (gte = 10) */
    TEST("value_gte = 9 (below limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 9;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_INVALID(result, "value_gte below limit");
    }
}

/*
 * Test PB_VALIDATE_NUMERIC_LTE (uses PB_CHECK_MAX internally)
 */
static void test_numeric_lte(void) {
    printf("\n=== Testing PB_VALIDATE_NUMERIC_LTE (PB_CHECK_MAX) ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid value: value_lte = 100 (lte = 100) */
    TEST("value_lte = 100 (equal to limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 100;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_VALID(result, "value_lte at limit");
    }
    
    /* Test invalid value: value_lte = 101 (lte = 100) */
    TEST("value_lte = 101 (above limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 101;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_INVALID(result, "value_lte above limit");
    }
}

/*
 * Test PB_VALIDATE_NUMERIC_GT (uses PB_CHECK_GT)
 */
static void test_numeric_gt(void) {
    printf("\n=== Testing PB_VALIDATE_NUMERIC_GT (PB_CHECK_GT) ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid value: value_gt = 1 (gt = 0) */
    TEST("value_gt = 1 (above limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_VALID(result, "value_gt above limit");
    }
    
    /* Test invalid value: value_gt = 0 (gt = 0) */
    TEST("value_gt = 0 (at limit - should fail)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = 0;  /* Invalid: must be > 0 */
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_INVALID(result, "value_gt at limit");
    }
    
    /* Test invalid value: value_gt = -1 (gt = 0) */
    TEST("value_gt = -1 (below limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = -1;  /* Invalid: must be > 0 */
        msg.value_lt = 25;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_INVALID(result, "value_gt below limit");
    }
}

/*
 * Test PB_VALIDATE_NUMERIC_LT (uses PB_CHECK_LT)
 */
static void test_numeric_lt(void) {
    printf("\n=== Testing PB_VALIDATE_NUMERIC_LT (PB_CHECK_LT) ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid value: value_lt = 49 (lt = 50) */
    TEST("value_lt = 49 (below limit)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 49;
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_VALID(result, "value_lt below limit");
    }
    
    /* Test invalid value: value_lt = 50 (lt = 50) */
    TEST("value_lt = 50 (at limit - should fail)");
    {
        FilterMacrosTest msg = FilterMacrosTest_init_zero;
        msg.value_gte = 10;
        msg.value_lte = 50;
        msg.value_gt = 1;
        msg.value_lt = 50;  /* Invalid: must be < 50 */
        msg.value_eq = 42;
        msg.value_range = 128;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosTest(&msg, &violations);
        EXPECT_INVALID(result, "value_lt at limit");
    }
}

/*
 * Test PB_VALIDATE_ONEOF_NUMERIC_GTE (uses PB_CHECK_MIN)
 */
static void test_oneof_numeric(void) {
    printf("\n=== Testing PB_VALIDATE_ONEOF_NUMERIC_* macros ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid oneof with gte */
    TEST("oneof int_value = 0 (gte = 0)");
    {
        FilterMacrosOneofTest msg = FilterMacrosOneofTest_init_zero;
        msg.which_data = FilterMacrosOneofTest_int_value_tag;
        msg.data.int_value = 0;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosOneofTest(&msg, &violations);
        EXPECT_VALID(result, "oneof gte at limit");
    }
    
    /* Test valid oneof with gte above limit */
    TEST("oneof int_value = 100 (gte = 0)");
    {
        FilterMacrosOneofTest msg = FilterMacrosOneofTest_init_zero;
        msg.which_data = FilterMacrosOneofTest_int_value_tag;
        msg.data.int_value = 100;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosOneofTest(&msg, &violations);
        EXPECT_VALID(result, "oneof gte above limit");
    }
    
    /* Test invalid oneof with gte */
    TEST("oneof int_value = -1 (gte = 0)");
    {
        FilterMacrosOneofTest msg = FilterMacrosOneofTest_init_zero;
        msg.which_data = FilterMacrosOneofTest_int_value_tag;
        msg.data.int_value = -1;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosOneofTest(&msg, &violations);
        EXPECT_INVALID(result, "oneof gte below limit");
    }
    
    /* Test valid oneof with lte */
    TEST("oneof max_value = 1000 (lte = 1000)");
    {
        FilterMacrosOneofTest msg = FilterMacrosOneofTest_init_zero;
        msg.which_data = FilterMacrosOneofTest_max_value_tag;
        msg.data.max_value = 1000;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosOneofTest(&msg, &violations);
        EXPECT_VALID(result, "oneof lte at limit");
    }
    
    /* Test invalid oneof with lte */
    TEST("oneof max_value = 1001 (lte = 1000)");
    {
        FilterMacrosOneofTest msg = FilterMacrosOneofTest_init_zero;
        msg.which_data = FilterMacrosOneofTest_max_value_tag;
        msg.data.max_value = 1001;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosOneofTest(&msg, &violations);
        EXPECT_INVALID(result, "oneof lte above limit");
    }
}

/*
 * Test PB_VALIDATE_REPEATED_ITEMS_* macros
 */
static void test_repeated_items(void) {
    printf("\n=== Testing PB_VALIDATE_REPEATED_ITEMS_* macros ===\n");
    pb_violations_t violations;
    bool result;
    
    /* Test valid repeated items with gte */
    TEST("repeated scores all >= 0");
    {
        FilterMacrosRepeatedTest msg = FilterMacrosRepeatedTest_init_zero;
        msg.scores[0] = 0;
        msg.scores[1] = 50;
        msg.scores[2] = 100;
        msg.scores_count = 3;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosRepeatedTest(&msg, &violations);
        EXPECT_VALID(result, "repeated gte all valid");
    }
    
    /* Test invalid repeated items with gte */
    TEST("repeated scores with negative value");
    {
        FilterMacrosRepeatedTest msg = FilterMacrosRepeatedTest_init_zero;
        msg.scores[0] = 10;
        msg.scores[1] = -5;  /* Invalid: must be >= 0 */
        msg.scores[2] = 20;
        msg.scores_count = 3;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosRepeatedTest(&msg, &violations);
        EXPECT_INVALID(result, "repeated gte with invalid item");
    }
    
    /* Test valid repeated items with lte */
    TEST("repeated percentages all <= 100");
    {
        FilterMacrosRepeatedTest msg = FilterMacrosRepeatedTest_init_zero;
        msg.percentages[0] = 0;
        msg.percentages[1] = 50;
        msg.percentages[2] = 100;
        msg.percentages_count = 3;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosRepeatedTest(&msg, &violations);
        EXPECT_VALID(result, "repeated lte all valid");
    }
    
    /* Test invalid repeated items with lte */
    TEST("repeated percentages with value > 100");
    {
        FilterMacrosRepeatedTest msg = FilterMacrosRepeatedTest_init_zero;
        msg.percentages[0] = 25;
        msg.percentages[1] = 150;  /* Invalid: must be <= 100 */
        msg.percentages[2] = 75;
        msg.percentages_count = 3;
        
        pb_violations_init(&violations);
        result = pb_validate_FilterMacrosRepeatedTest(&msg, &violations);
        EXPECT_INVALID(result, "repeated lte with invalid item");
    }
}

/*
 * Test that PB_CHECK_* macros are properly used
 * This verifies that pb_filter_macros.h is properly included
 */
static void test_filter_macros_basic(void) {
    printf("\n=== Testing PB_CHECK_* macros directly ===\n");
    
    /* Test PB_CHECK_MIN */
    TEST("PB_CHECK_MIN(NULL, 10, 5) should be true");
    {
        bool result = PB_CHECK_MIN(NULL, 10, 5);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_MIN works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_MIN failed\n");
        }
    }
    
    TEST("PB_CHECK_MIN(NULL, 3, 5) should be false");
    {
        bool result = PB_CHECK_MIN(NULL, 3, 5);
        if (!result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_MIN correctly rejected\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_MIN should have failed\n");
        }
    }
    
    /* Test PB_CHECK_MAX */
    TEST("PB_CHECK_MAX(NULL, 10, 100) should be true");
    {
        bool result = PB_CHECK_MAX(NULL, 10, 100);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_MAX works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_MAX failed\n");
        }
    }
    
    TEST("PB_CHECK_MAX(NULL, 150, 100) should be false");
    {
        bool result = PB_CHECK_MAX(NULL, 150, 100);
        if (!result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_MAX correctly rejected\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_MAX should have failed\n");
        }
    }
    
    /* Test PB_CHECK_EQ */
    TEST("PB_CHECK_EQ(NULL, 42, 42) should be true");
    {
        bool result = PB_CHECK_EQ(NULL, 42, 42);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_EQ works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_EQ failed\n");
        }
    }
    
    /* Test PB_CHECK_RANGE */
    TEST("PB_CHECK_RANGE(NULL, 50, 0, 100) should be true");
    {
        bool result = PB_CHECK_RANGE(NULL, 50, 0, 100);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_RANGE works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_RANGE failed\n");
        }
    }
    
    TEST("PB_CHECK_RANGE(NULL, 150, 0, 100) should be false");
    {
        bool result = PB_CHECK_RANGE(NULL, 150, 0, 100);
        if (!result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_RANGE correctly rejected\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_RANGE should have failed\n");
        }
    }
    
    /* Test PB_CHECK_GT (strict greater-than) */
    TEST("PB_CHECK_GT(NULL, 10, 5) should be true");
    {
        bool result = PB_CHECK_GT(NULL, 10, 5);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_GT works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_GT failed\n");
        }
    }
    
    TEST("PB_CHECK_GT(NULL, 5, 5) should be false");
    {
        bool result = PB_CHECK_GT(NULL, 5, 5);
        if (!result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_GT correctly rejected equal value\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_GT should have failed for equal value\n");
        }
    }
    
    /* Test PB_CHECK_LT (strict less-than) */
    TEST("PB_CHECK_LT(NULL, 5, 10) should be true");
    {
        bool result = PB_CHECK_LT(NULL, 5, 10);
        if (result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_LT works correctly\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_LT failed\n");
        }
    }
    
    TEST("PB_CHECK_LT(NULL, 10, 10) should be false");
    {
        bool result = PB_CHECK_LT(NULL, 10, 10);
        if (!result) {
            tests_passed++;
            printf("    [PASS] PB_CHECK_LT correctly rejected equal value\n");
        } else {
            tests_failed++;
            printf("    [FAIL] PB_CHECK_LT should have failed for equal value\n");
        }
    }
}

int main(void) {
    printf("Filter Macros Validation Tests\n");
    printf("================================\n");
    printf("Testing that generated validation uses PB_CHECK_* macros from pb_filter_macros.h\n");
    
    test_filter_macros_basic();
    test_numeric_gte();
    test_numeric_lte();
    test_numeric_gt();
    test_numeric_lt();
    test_oneof_numeric();
    test_repeated_items();
    
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
