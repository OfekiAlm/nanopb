/* Test validation functionality with various constraints */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb_validate.h>
#include "validation_test.pb.h"
#include "validation_test_validate.h"

/* Helper to print violations */
static void print_violations(const pb_violations_t *violations)
{
    printf("  Violations: %u (truncated=%s)\n", 
           (unsigned)pb_violations_count(violations),
           violations->truncated ? "true" : "false");
    
    for (pb_size_t i = 0; i < pb_violations_count(violations); i++)
    {
        const pb_violation_t *v = &violations->violations[i];
        printf("    [%u] %s: %s (%s)\n", 
               (unsigned)i,
               v->field_path ? v->field_path : "<no-path>",
               v->message ? v->message : "<no-msg>",
               v->constraint_id ? v->constraint_id : "<no-rule>");
    }
}

/* Test valid Person message */
static int test_valid_person(void)
{
    printf("\n=== Test: Valid Person ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "John Doe";
    const char *email_str = "john@example.com";
    const char *phone_str = "1234567890";
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = Person_Gender_MALE;
    person.phone.arg = (void *)phone_str;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (!result || pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected valid person\n");
        print_violations(&violations);
        return 1;
    }
    
    printf("  PASS\n");
    return 0;
}

/* Test Person with name too short (min_len=1) */
static int test_person_name_too_short(void)
{
    printf("\n=== Test: Person name too short ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "";  /* Empty name - violates min_len */
    const char *email_str = "john@example.com";
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = Person_Gender_MALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for empty name\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test Person with name too long (max_len=50) */
static int test_person_name_too_long(void)
{
    printf("\n=== Test: Person name too long ===\n");
    
    Person person = Person_init_zero;
    /* Name longer than 50 characters */
    const char *name_str = "This is a very long name that definitely exceeds the maximum length of fifty characters";
    const char *email_str = "john@example.com";
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = Person_Gender_MALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for name too long\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test Person email without '@' (contains constraint) */
static int test_person_email_no_at(void)
{
    printf("\n=== Test: Person email without @ ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "John Doe";
    const char *email_str = "johnexample.com";  /* Missing @ */
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = Person_Gender_MALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for email without @\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test Person with age out of range */
static int test_person_age_out_of_range(void)
{
    printf("\n=== Test: Person age out of range ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "John Doe";
    const char *email_str = "john@example.com";
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 200;  /* Exceeds max of 150 */
    person.gender = Person_Gender_MALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for age > 150\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test Person with invalid enum value */
static int test_person_invalid_enum(void)
{
    printf("\n=== Test: Person with invalid enum value ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "John Doe";
    const char *email_str = "john@example.com";
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = (Person_Gender)999;  /* Invalid enum value */
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for invalid enum\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test Person with phone too short */
static int test_person_phone_too_short(void)
{
    printf("\n=== Test: Person phone too short ===\n");
    
    Person person = Person_init_zero;
    const char *name_str = "John Doe";
    const char *email_str = "john@example.com";
    const char *phone_str = "123";  /* Less than 10 characters */
    
    person.name.arg = (void *)name_str;
    person.email.arg = (void *)email_str;
    person.age = 30;
    person.gender = Person_Gender_MALE;
    person.phone.arg = (void *)phone_str;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for phone too short\n");
        return 1;
    }
    
    printf("  PASS - Violation detected\n");
    print_violations(&violations);
    return 0;
}

/* Test valid Company with nested Person */
static int test_valid_company(void)
{
    printf("\n=== Test: Valid Company with nested Person ===\n");
    
    Company company = Company_init_zero;
    const char *company_name_str = "Tech Corp";
    const char *ceo_name_str = "Jane CEO";
    const char *ceo_email_str = "jane@techcorp.com";
    
    company.name.arg = (void *)company_name_str;
    
    /* Add valid CEO */
    company.has_ceo = true;
    company.ceo.name.arg = (void *)ceo_name_str;
    company.ceo.email.arg = (void *)ceo_email_str;
    company.ceo.age = 45;
    company.ceo.gender = Person_Gender_FEMALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Company(&company, &violations);
    
    if (!result || pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected valid company\n");
        print_violations(&violations);
        return 1;
    }
    
    printf("  PASS\n");
    return 0;
}

/* Test Company with invalid nested Person */
static int test_company_invalid_ceo(void)
{
    printf("\n=== Test: Company with invalid CEO ===\n");
    
    Company company = Company_init_zero;
    const char *company_name_str = "Tech Corp";
    const char *ceo_name_str = "Jane CEO";
    const char *ceo_email_str = "jane@techcorp.com";
    
    company.name.arg = (void *)company_name_str;
    
    /* Add CEO with invalid age */
    company.has_ceo = true;
    company.ceo.name.arg = (void *)ceo_name_str;
    company.ceo.email.arg = (void *)ceo_email_str;
    company.ceo.age = 200;  /* Invalid */
    company.ceo.gender = Person_Gender_FEMALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Company(&company, &violations);
    
    if (result || !pb_violations_has_any(&violations))
    {
        printf("  FAIL: Expected violation for invalid CEO age\n");
        return 1;
    }
    
    printf("  PASS - Violation detected in nested message\n");
    print_violations(&violations);
    return 0;
}

int main(void)
{
    int failures = 0;
    
    printf("===================================\n");
    printf("  Nanopb Validation Test Suite\n");
    printf("===================================\n");
    
    failures += test_valid_person();
    failures += test_person_name_too_short();
    failures += test_person_name_too_long();
    failures += test_person_email_no_at();
    failures += test_person_age_out_of_range();
    failures += test_person_invalid_enum();
    failures += test_person_phone_too_short();
    failures += test_valid_company();
    failures += test_company_invalid_ceo();
    
    printf("\n===================================\n");
    if (failures == 0)
    {
        printf("  All tests PASSED!\n");
    }
    else
    {
        printf("  %d test(s) FAILED!\n", failures);
    }
    printf("===================================\n");
    
    return failures;
}
