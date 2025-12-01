/* Test file for repeated.items and repeated.unique validation */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_validate.h"
#include "repeated_validation.pb.h"
#include "repeated_validation_validate.h"

static void print_violations(const pb_violations_t *viol)
{
    printf("  violations: %u (truncated=%s)\n", (unsigned)pb_violations_count(viol), viol->truncated ? "true" : "false");
    for (pb_size_t i = 0; i < pb_violations_count(viol); ++i)
    {
        const pb_violation_t *v = &viol->violations[i];
        printf("  - %s: %s (%s)\n", v->field_path ? v->field_path : "<path>", v->message ? v->message : "<msg>", v->constraint_id ? v->constraint_id : "<rule>");
    }
}

static int test_count = 0;
static int test_passed = 0;

#define TEST(name, expr) do { \
    test_count++; \
    printf("Test %d: %s ... ", test_count, name); \
    if (expr) { \
        printf("PASS\n"); \
        test_passed++; \
    } else { \
        printf("FAIL\n"); \
        return 1; \
    } \
} while(0)

int main(void)
{
    pb_violations_t viol;
    bool ok;

    printf("== Testing repeated.items validation ==\n\n");

    /* Test 1: RepeatedStringItems - happy path */
    {
        test_RepeatedStringItems msg = test_RepeatedStringItems_init_zero;
        msg.values_count = 3;
        strcpy(msg.values[0], "abc");      /* len=3, OK */
        strcpy(msg.values[1], "defghi");   /* len=6, OK */
        strcpy(msg.values[2], "xyz");      /* len=3, OK */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedStringItems(&msg, &viol);
        TEST("RepeatedStringItems: valid strings (len 3-10)", ok && !pb_violations_has_any(&viol));
    }

    /* Test 2: RepeatedStringItems - string too short */
    {
        test_RepeatedStringItems msg = test_RepeatedStringItems_init_zero;
        msg.values_count = 2;
        strcpy(msg.values[0], "ab");       /* len=2, TOO SHORT (min_len=3) */
        strcpy(msg.values[1], "xyz");      /* len=3, OK */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedStringItems(&msg, &viol);
        TEST("RepeatedStringItems: string too short", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation at values[0]: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 3: RepeatedStringItems - string too long */
    {
        test_RepeatedStringItems msg = test_RepeatedStringItems_init_zero;
        msg.values_count = 2;
        strcpy(msg.values[0], "abc");              /* len=3, OK */
        strcpy(msg.values[1], "toolongstring1");  /* len=14, TOO LONG (max_len=10) */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedStringItems(&msg, &viol);
        TEST("RepeatedStringItems: string too long", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation at values[1]: %s\n", viol.violations[0].constraint_id);
        }
    }

    printf("\n== Testing repeated.items with int32 ==\n\n");

    /* Test 4: RepeatedInt32Items - happy path */
    {
        test_RepeatedInt32Items msg = test_RepeatedInt32Items_init_zero;
        msg.values_count = 3;
        msg.values[0] = 1;    /* > 0 and < 100, OK */
        msg.values[1] = 50;   /* > 0 and < 100, OK */
        msg.values[2] = 99;   /* > 0 and < 100, OK */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedInt32Items(&msg, &viol);
        TEST("RepeatedInt32Items: valid values (0 < v < 100)", ok && !pb_violations_has_any(&viol));
    }

    /* Test 5: RepeatedInt32Items - value too small (not > 0) */
    {
        test_RepeatedInt32Items msg = test_RepeatedInt32Items_init_zero;
        msg.values_count = 2;
        msg.values[0] = 50;   /* OK */
        msg.values[1] = 0;    /* NOT > 0, FAIL */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedInt32Items(&msg, &viol);
        TEST("RepeatedInt32Items: value not > 0", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation at values[1]: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 6: RepeatedInt32Items - value too large (not < 100) */
    {
        test_RepeatedInt32Items msg = test_RepeatedInt32Items_init_zero;
        msg.values_count = 2;
        msg.values[0] = 50;   /* OK */
        msg.values[1] = 100;  /* NOT < 100, FAIL */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedInt32Items(&msg, &viol);
        TEST("RepeatedInt32Items: value not < 100", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation at values[1]: %s\n", viol.violations[0].constraint_id);
        }
    }

    printf("\n== Testing repeated.unique validation ==\n\n");

    /* Test 7: RepeatedUniqueStrings - happy path */
    {
        test_RepeatedUniqueStrings msg = test_RepeatedUniqueStrings_init_zero;
        msg.values_count = 3;
        strcpy(msg.values[0], "apple");
        strcpy(msg.values[1], "banana");
        strcpy(msg.values[2], "cherry");
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedUniqueStrings(&msg, &viol);
        TEST("RepeatedUniqueStrings: all unique strings", ok && !pb_violations_has_any(&viol));
    }

    /* Test 8: RepeatedUniqueStrings - duplicate */
    {
        test_RepeatedUniqueStrings msg = test_RepeatedUniqueStrings_init_zero;
        msg.values_count = 3;
        strcpy(msg.values[0], "apple");
        strcpy(msg.values[1], "banana");
        strcpy(msg.values[2], "apple");  /* DUPLICATE */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedUniqueStrings(&msg, &viol);
        TEST("RepeatedUniqueStrings: duplicate string", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 9: RepeatedUniqueInt32 - happy path */
    {
        test_RepeatedUniqueInt32 msg = test_RepeatedUniqueInt32_init_zero;
        msg.values_count = 4;
        msg.values[0] = 1;
        msg.values[1] = 2;
        msg.values[2] = 3;
        msg.values[3] = 4;
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedUniqueInt32(&msg, &viol);
        TEST("RepeatedUniqueInt32: all unique integers", ok && !pb_violations_has_any(&viol));
    }

    /* Test 10: RepeatedUniqueInt32 - duplicate */
    {
        test_RepeatedUniqueInt32 msg = test_RepeatedUniqueInt32_init_zero;
        msg.values_count = 4;
        msg.values[0] = 1;
        msg.values[1] = 2;
        msg.values[2] = 1;  /* DUPLICATE */
        msg.values[3] = 4;
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedUniqueInt32(&msg, &viol);
        TEST("RepeatedUniqueInt32: duplicate integer", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    printf("\n== Testing combined items + unique ==\n\n");

    /* Test 11: RepeatedBothConstraints - happy path */
    {
        test_RepeatedBothConstraints msg = test_RepeatedBothConstraints_init_zero;
        msg.values_count = 3;
        strcpy(msg.values[0], "ab");     /* len=2, >= min_len=2, OK */
        strcpy(msg.values[1], "cd");     /* len=2, >= min_len=2, OK */
        strcpy(msg.values[2], "ef");     /* len=2, >= min_len=2, OK and all unique */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedBothConstraints(&msg, &viol);
        TEST("RepeatedBothConstraints: valid and unique", ok && !pb_violations_has_any(&viol));
    }

    /* Test 12: RepeatedBothConstraints - items violation */
    {
        test_RepeatedBothConstraints msg = test_RepeatedBothConstraints_init_zero;
        msg.values_count = 2;
        strcpy(msg.values[0], "x");      /* len=1, < min_len=2, FAIL */
        strcpy(msg.values[1], "yz");     /* OK */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedBothConstraints(&msg, &viol);
        TEST("RepeatedBothConstraints: items violation (too short)", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 13: RepeatedBothConstraints - unique violation */
    {
        test_RepeatedBothConstraints msg = test_RepeatedBothConstraints_init_zero;
        msg.values_count = 3;
        strcpy(msg.values[0], "ab");
        strcpy(msg.values[1], "cd");
        strcpy(msg.values[2], "ab");     /* DUPLICATE */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedBothConstraints(&msg, &viol);
        TEST("RepeatedBothConstraints: unique violation", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    printf("\n== Testing all constraints (min_items, max_items, items, unique) ==\n\n");

    /* Test 14: RepeatedAllConstraints - happy path */
    {
        test_RepeatedAllConstraints msg = test_RepeatedAllConstraints_init_zero;
        msg.numbers_count = 5;
        msg.numbers[0] = 10;
        msg.numbers[1] = 20;
        msg.numbers[2] = 30;
        msg.numbers[3] = 40;
        msg.numbers[4] = 50;  /* All unique, in range [0,1000], count in [1,10] */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedAllConstraints(&msg, &viol);
        TEST("RepeatedAllConstraints: all valid", ok && !pb_violations_has_any(&viol));
    }

    /* Test 15: RepeatedAllConstraints - min_items violation */
    {
        test_RepeatedAllConstraints msg = test_RepeatedAllConstraints_init_zero;
        msg.numbers_count = 0;  /* < min_items=1 */
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedAllConstraints(&msg, &viol);
        TEST("RepeatedAllConstraints: min_items violation", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 16: RepeatedAllConstraints - max_items violation */
    {
        test_RepeatedAllConstraints msg = test_RepeatedAllConstraints_init_zero;
        msg.numbers_count = 11;  /* > max_items=10 */
        for (int i = 0; i < 11; i++) {
            msg.numbers[i] = i + 1;
        }
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedAllConstraints(&msg, &viol);
        TEST("RepeatedAllConstraints: max_items violation", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 17: RepeatedAllConstraints - items out of range */
    {
        test_RepeatedAllConstraints msg = test_RepeatedAllConstraints_init_zero;
        msg.numbers_count = 3;
        msg.numbers[0] = 500;
        msg.numbers[1] = -1;     /* < gte=0, FAIL */
        msg.numbers[2] = 100;
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedAllConstraints(&msg, &viol);
        TEST("RepeatedAllConstraints: items range violation", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    /* Test 18: RepeatedAllConstraints - unique violation */
    {
        test_RepeatedAllConstraints msg = test_RepeatedAllConstraints_init_zero;
        msg.numbers_count = 5;
        msg.numbers[0] = 10;
        msg.numbers[1] = 20;
        msg.numbers[2] = 10;  /* DUPLICATE */
        msg.numbers[3] = 40;
        msg.numbers[4] = 50;
        
        pb_violations_init(&viol);
        ok = pb_validate_test_RepeatedAllConstraints(&msg, &viol);
        TEST("RepeatedAllConstraints: unique violation", !ok && pb_violations_has_any(&viol));
        if (pb_violations_has_any(&viol)) {
            printf("  Expected violation: %s\n", viol.violations[0].constraint_id);
        }
    }

    printf("\n== Summary ==\n");
    printf("Passed: %d / %d tests\n", test_passed, test_count);

    return (test_passed == test_count) ? 0 : 1;
}
