/* Simple validation test */
#include <stdio.h>
#include <string.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb_validate.h>
#include "validation_test.pb.h"
#include "validation_test_validate.h"

int main(void)
{
    printf("===================================\n");
    printf("  Nanopb Validation Test\n");
    printf("===================================\n");
    
    /* Test 1: Valid Person */
    printf("\nTest 1: Valid Person\n");
    Person person = Person_init_zero;
    strcpy(person.name, "John Doe");
    strcpy(person.email, "john@example.com");
    person.age = 30;
    person.gender = Person_Gender_MALE;
    
    pb_violations_t violations;
    pb_violations_init(&violations);
    
    bool result = pb_validate_Person(&person, &violations);
    
    if (result && !pb_violations_has_any(&violations))
    {
        printf("  PASS - Valid person accepted\n");
    }
    else
    {
        printf("  FAIL - Valid person rejected\n");
        return 1;
    }
    
    /* Test 2: Invalid Person (name too short) */
    printf("\nTest 2: Invalid Person (empty name)\n");
    Person person2 = Person_init_zero;
    person2.name[0] = '\0';
    strcpy(person2.email, "john@example.com");
    person2.age = 30;
    person2.gender = Person_Gender_MALE;
    
    pb_violations_init(&violations);
    result = pb_validate_Person(&person2, &violations);
    
    if (!result || pb_violations_has_any(&violations))
    {
        printf("  PASS - Empty name rejected\n");
    }
    else
    {
        printf("  FAIL - Empty name accepted\n");
        return 1;
    }
    
    printf("\n===================================\n");
    printf("  All tests PASSED!\n");
    printf("===================================\n");
    
    return 0;
}
