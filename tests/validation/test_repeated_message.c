/* Test for repeated submessage validation */

#include <stdio.h>
#include <string.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <unittests.h>
#include "repeated_message_rules.pb.h"
#include "repeated_message_rules_validate.h"

int main()
{
    int status = 0;

    /* Test 1: Valid Team with valid members */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Engineering");
        team.members_count = 2;
        
        strcpy(team.members[0].name, "Alice");
        team.members[0].age = 30;
        
        strcpy(team.members[1].name, "Bob");
        team.members[1].age = 25;
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == true);
        TEST(violations.count == 0);
        
        printf("Test 1: Valid team - PASSED\n");
    }

    /* Test 2: Team with invalid member (age out of range) */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Research");
        team.members_count = 1;
        
        strcpy(team.members[0].name, "Charlie");
        team.members[0].age = 200; /* Invalid: > 150 */
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == false);
        TEST(violations.count > 0);
        
        printf("Test 2: Invalid member age - PASSED (found %d violations)\n", violations.count);
    }

    /* Test 3: Team with empty member name */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Marketing");
        team.members_count = 1;
        
        team.members[0].name[0] = '\0'; /* Invalid: empty string */
        team.members[0].age = 28;
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == false);
        TEST(violations.count > 0);
        
        printf("Test 3: Empty member name - PASSED (found %d violations)\n", violations.count);
    }

    /* Test 4: Team with too few members (min_items = 1) */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Sales");
        team.members_count = 0; /* Invalid: < min_items */
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == false);
        TEST(violations.count > 0);
        
        printf("Test 4: Too few members - PASSED (found %d violations)\n", violations.count);
    }

    /* Test 5: Team with too many members (max_items = 5) */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Support");
        team.members_count = 6; /* Invalid: > max_items */
        
        for (int i = 0; i < 6; i++) {
            sprintf(team.members[i].name, "Member%d", i);
            team.members[i].age = 25 + i;
        }
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == false);
        TEST(violations.count > 0);
        
        printf("Test 5: Too many members - PASSED (found %d violations)\n", violations.count);
    }

    /* Test 6: Valid team at boundary (exactly 5 members) */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "Development");
        team.members_count = 5; /* Valid: exactly max_items */
        
        for (int i = 0; i < 5; i++) {
            sprintf(team.members[i].name, "Dev%d", i);
            team.members[i].age = 20 + i * 10;
        }
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == true);
        TEST(violations.count == 0);
        
        printf("Test 6: Valid team at boundary - PASSED\n");
    }

    /* Test 7: Team with multiple invalid members */
    {
        Team team = Team_init_zero;
        strcpy(team.team_name, "QA");
        team.members_count = 3;
        
        strcpy(team.members[0].name, "Valid");
        team.members[0].age = 30;
        
        team.members[1].name[0] = '\0'; /* Invalid: empty name */
        team.members[1].age = 25;
        
        strcpy(team.members[2].name, "Another");
        team.members[2].age = -5; /* Invalid: negative age */
        
        pb_violations_t violations = {0};
        pb_violations_init(&violations);
        
        bool valid = pb_validate_Team(&team, &violations);
        TEST(valid == false);
        TEST(violations.count >= 2); /* At least 2 violations */
        
        printf("Test 7: Multiple invalid members - PASSED (found %d violations)\n", violations.count);
    }

    if (status != 0)
        fprintf(stdout, "\n\nSome tests FAILED!\n");

    return status;
}
