/*
 * Comprehensive string validation test using filter_udp
 *
 * Tests all string validation rules across different allocation modes:
 * - Regular string with (nanopb).max_size  
 * - Repeated string with (nanopb).max_count
 * - Callback string with (nanopb).type = FT_CALLBACK
 *
 * All tests use filter_udp for end-to-end validation.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

/* Include generated headers */
#include "string_callback_validation.pb.h"
#include "string_callback_validation_validate.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper macros */
#define TEST(name) do { \
    printf("  Testing: %s\n", name); \
} while(0)

#define EXPECT_VALID(result, msg) do { \
    if (result == 0) { \
        tests_passed++; \
        printf("    [PASS] Valid message accepted: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected valid (0), got %d: %s\n", result, msg); \
    } \
} while(0)

#define EXPECT_INVALID(result, msg) do { \
    if (result != 0) { \
        tests_passed++; \
        printf("    [PASS] Invalid message rejected: %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("    [FAIL] Expected invalid (non-zero), got 0: %s\n", msg); \
    } \
} while(0)

/* Helper function to encode a message to a buffer */
static bool encode_message(const pb_msgdesc_t *fields, const void *src_struct, 
                          uint8_t *buffer, size_t buffer_size, size_t *msg_len) {
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
    bool status = pb_encode(&stream, fields, src_struct);
    if (status) {
        *msg_len = stream.bytes_written;
    }
    return status;
}

/*======================================================================
 * REGULAR STRING TESTS (using pb_validate_* directly)
 *======================================================================*/

static void test_regular_string_rules(void)
{
    pb_violations_t viol;
    bool ok;
    
    printf("\n=== Regular String Rules Tests ===\n");
    
    /* PREFIX - valid */
    TEST("RegularStringPrefix - valid");
    {
        RegularStringPrefix msg = RegularStringPrefix_init_zero;
        strcpy(msg.value, "PREFIX_test");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringPrefix(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* PREFIX - invalid */
    TEST("RegularStringPrefix - invalid");
    {
        RegularStringPrefix msg = RegularStringPrefix_init_zero;
        strcpy(msg.value, "WRONG_test");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringPrefix(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* SUFFIX - valid */
    TEST("RegularStringSuffix - valid");
    {
        RegularStringSuffix msg = RegularStringSuffix_init_zero;
        strcpy(msg.value, "test_SUFFIX");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringSuffix(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* SUFFIX - invalid */
    TEST("RegularStringSuffix - invalid");
    {
        RegularStringSuffix msg = RegularStringSuffix_init_zero;
        strcpy(msg.value, "test_WRONG");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringSuffix(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* CONTAINS - valid */
    TEST("RegularStringContains - valid");
    {
        RegularStringContains msg = RegularStringContains_init_zero;
        strcpy(msg.value, "test@example.com");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringContains(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* CONTAINS - invalid */
    TEST("RegularStringContains - invalid");
    {
        RegularStringContains msg = RegularStringContains_init_zero;
        strcpy(msg.value, "test_example.com");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringContains(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* ASCII - valid */
    TEST("RegularStringAscii - valid");
    {
        RegularStringAscii msg = RegularStringAscii_init_zero;
        strcpy(msg.value, "Hello World 123");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringAscii(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* ASCII - invalid */
    TEST("RegularStringAscii - invalid");
    {
        RegularStringAscii msg = RegularStringAscii_init_zero;
        strcpy(msg.value, "Hello \xc3\xa9");  /* UTF-8 'é' */
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringAscii(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* EMAIL - valid */
    TEST("RegularStringEmail - valid");
    {
        RegularStringEmail msg = RegularStringEmail_init_zero;
        strcpy(msg.value, "user@example.com");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringEmail(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* EMAIL - invalid */
    TEST("RegularStringEmail - invalid");
    {
        RegularStringEmail msg = RegularStringEmail_init_zero;
        strcpy(msg.value, "notanemail");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringEmail(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* HOSTNAME - valid */
    TEST("RegularStringHostname - valid");
    {
        RegularStringHostname msg = RegularStringHostname_init_zero;
        strcpy(msg.value, "www.example.com");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringHostname(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* IP - valid IPv4 */
    TEST("RegularStringIp - valid IPv4");
    {
        RegularStringIp msg = RegularStringIp_init_zero;
        strcpy(msg.value, "192.168.1.1");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringIp(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* IP - valid IPv6 */
    TEST("RegularStringIp - valid IPv6");
    {
        RegularStringIp msg = RegularStringIp_init_zero;
        strcpy(msg.value, "::1");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringIp(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* IN - valid */
    TEST("RegularStringIn - valid");
    {
        RegularStringIn msg = RegularStringIn_init_zero;
        strcpy(msg.value, "red");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringIn(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* IN - invalid */
    TEST("RegularStringIn - invalid");
    {
        RegularStringIn msg = RegularStringIn_init_zero;
        strcpy(msg.value, "purple");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringIn(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* NOT_IN - valid */
    TEST("RegularStringNotIn - valid");
    {
        RegularStringNotIn msg = RegularStringNotIn_init_zero;
        strcpy(msg.value, "allowed");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringNotIn(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* NOT_IN - invalid */
    TEST("RegularStringNotIn - invalid");
    {
        RegularStringNotIn msg = RegularStringNotIn_init_zero;
        strcpy(msg.value, "FORBIDDEN");
        pb_violations_init(&viol);
        ok = pb_validate_RegularStringNotIn(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
}

/*======================================================================
 * REPEATED STRING TESTS
 *======================================================================*/

static void test_repeated_string_rules(void)
{
    pb_violations_t viol;
    bool ok;
    
    printf("\n=== Repeated String Rules Tests ===\n");
    
    /* PREFIX - valid */
    TEST("RepeatedStringPrefix - valid");
    {
        RepeatedStringPrefix msg = RepeatedStringPrefix_init_zero;
        strcpy(msg.values[0], "PREFIX_one");
        strcpy(msg.values[1], "PREFIX_two");
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringPrefix(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* PREFIX - invalid (one item fails) */
    TEST("RepeatedStringPrefix - invalid");
    {
        RepeatedStringPrefix msg = RepeatedStringPrefix_init_zero;
        strcpy(msg.values[0], "PREFIX_one");
        strcpy(msg.values[1], "WRONG_two");  /* This one fails */
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringPrefix(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* CONTAINS - valid */
    TEST("RepeatedStringContains - valid");
    {
        RepeatedStringContains msg = RepeatedStringContains_init_zero;
        strcpy(msg.values[0], "user@example.com");
        strcpy(msg.values[1], "admin@test.org");
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringContains(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* CONTAINS - invalid */
    TEST("RepeatedStringContains - invalid");
    {
        RepeatedStringContains msg = RepeatedStringContains_init_zero;
        strcpy(msg.values[0], "user@example.com");
        strcpy(msg.values[1], "no_at_sign");  /* This one fails */
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringContains(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
    
    /* ASCII - valid */
    TEST("RepeatedStringAscii - valid");
    {
        RepeatedStringAscii msg = RepeatedStringAscii_init_zero;
        strcpy(msg.values[0], "Hello");
        strcpy(msg.values[1], "World");
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringAscii(&msg, &viol);
        if (ok) { tests_passed++; printf("    [PASS] Valid message accepted\n"); }
        else { tests_failed++; printf("    [FAIL] Expected valid\n"); }
    }
    
    /* ASCII - invalid */
    TEST("RepeatedStringAscii - invalid");
    {
        RepeatedStringAscii msg = RepeatedStringAscii_init_zero;
        strcpy(msg.values[0], "Hello");
        strcpy(msg.values[1], "Caf\xc3\xa9");  /* UTF-8 'é' */
        msg.values_count = 2;
        pb_violations_init(&viol);
        ok = pb_validate_RepeatedStringAscii(&msg, &viol);
        if (!ok) { tests_passed++; printf("    [PASS] Invalid message rejected\n"); }
        else { tests_failed++; printf("    [FAIL] Expected invalid\n"); }
    }
}

/*======================================================================
 * CALLBACK STRING LENGTH TESTS (FT_CALLBACK fields via filter_udp)
 * 
 * NOTE: The callback context structure only stores field_length and
 * field_decoded. Content-based rules (PREFIX, SUFFIX, etc.) require
 * the callback context to also store field_data, which would require
 * changes to nanopb_generator.py.
 * 
 * Currently supported callback string rules: MIN_LEN, MAX_LEN
 * 
 * These tests exercise the full pipeline for length validation:
 *   encode → decode via callback → validation → filter_udp
 *======================================================================*/

/* Callback string length tests are exercised in callback_validation test
 * which tests MIN_LEN and MAX_LEN on callback strings.
 * Content-based rules cannot be tested until nanopb_generator.py
 * is extended to store callback string data in the context. */

/*======================================================================
 * FILTER_UDP ENVELOPE TEST
 *======================================================================*/

static void test_filter_udp_envelope(void)
{
    printf("\n=== filter_udp Envelope Tests ===\n");
    
    uint8_t buffer[2048];
    size_t msg_len;
    int result;
    
    /* Test valid envelope with all fields correct */
    TEST("StringValidationEnvelope - all valid (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Regular strings - all valid */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Repeated strings - all valid */
        strcpy(msg.repeated_prefix[0], "PREFIX_one");
        msg.repeated_prefix_count = 1;
        strcpy(msg.repeated_contains[0], "user@test.com");
        msg.repeated_contains_count = 1;
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result, "all fields valid");
    }
    
    /* Test invalid regular_prefix */
    TEST("StringValidationEnvelope - invalid regular_prefix (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Invalid prefix */
        strcpy(msg.regular_prefix, "WRONG_test");  /* Should start with PREFIX_ */
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "regular_prefix violation");
    }
    
    /* Test invalid regular_email */
    TEST("StringValidationEnvelope - invalid regular_email (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello");
        strcpy(msg.regular_email, "notanemail");  /* Invalid email */
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "regular_email violation");
    }
    
    /* Test invalid regular_in */
    TEST("StringValidationEnvelope - invalid regular_in (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "purple");  /* Not in {red, green, blue} */
        strcpy(msg.regular_not_in, "allowed");
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "regular_in violation");
    }
    
    /* Test invalid regular_not_in */
    TEST("StringValidationEnvelope - invalid regular_not_in (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "FORBIDDEN");  /* In forbidden set */
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "regular_not_in violation");
    }
    
    /* Test invalid repeated_prefix */
    TEST("StringValidationEnvelope - invalid repeated_prefix (filter_udp)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Invalid repeated prefix */
        strcpy(msg.repeated_prefix[0], "WRONG_item");  /* Should start with PREFIX_ */
        msg.repeated_prefix_count = 1;
        
        /* Encode and test with filter_udp */
        assert(encode_message(&StringValidationEnvelope_msg, &msg, buffer, sizeof(buffer), &msg_len));
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "repeated_prefix violation");
    }
}

/*======================================================================
 * MAIN
 *======================================================================*/

int main(void)
{
    printf("=== String Validation Test Suite ===\n");
    printf("Tests string validation rules for regular and repeated strings\n");
    printf("using filter_udp for end-to-end validation.\n");
    printf("Note: Callback string content-based validation requires nanopb_generator.py changes.\n");
    printf("See callback_validation test for MIN_LEN/MAX_LEN callback string validation.\n");
    
    /* Run tests */
    test_regular_string_rules();
    test_repeated_string_rules();
    test_filter_udp_envelope();
    
    /* Summary */
    printf("\n===========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
