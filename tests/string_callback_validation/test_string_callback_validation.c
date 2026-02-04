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
 * CALLBACK STRING TESTS (FT_CALLBACK fields via filter_udp)
 * 
 * Tests content-based validation rules on callback strings in the
 * StringValidationEnvelope message. The envelope has callback fields
 * for PREFIX, SUFFIX, CONTAINS, ASCII, EMAIL, IN, and NOT_IN rules.
 * 
 * Test pipeline: encode → filter_udp (decode callback + validation)
 * 
 * The callback context stores actual string content (up to 256 bytes),
 * enabling full content-based validation (PREFIX, SUFFIX, CONTAINS, etc.)
 *======================================================================*/

/* Encode callback that writes a string from arg */
static bool encode_callback_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    const char *str = (const char *)*arg;
    if (!pb_encode_tag_for_field(stream, field))
        return false;
    return pb_encode_string(stream, (const uint8_t *)str, strlen(str));
}

/* Helper to test callback string field via filter_udp */
static void test_callback_via_envelope(const char *test_name, 
                                       int field_num,  /* Which callback field */
                                       const char *test_str,
                                       bool expect_valid)
{
    uint8_t buffer[2048];
    size_t msg_len;
    pb_ostream_t ostream;
    int result;
    
    TEST(test_name);
    
    /* Initialize envelope with all valid values */
    StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
    
    /* Set valid values for all regular fields */
    strcpy(msg.regular_prefix, "PREFIX_test");
    strcpy(msg.regular_suffix, "test_SUFFIX");
    strcpy(msg.regular_contains, "test@example.com");
    strcpy(msg.regular_ascii, "Hello World");
    strcpy(msg.regular_email, "user@example.com");
    strcpy(msg.regular_hostname, "www.example.com");
    strcpy(msg.regular_ip, "192.168.1.1");
    strcpy(msg.regular_in, "red");
    strcpy(msg.regular_not_in, "allowed");
    
    /* Set repeated fields to valid values */
    strcpy(msg.repeated_prefix[0], "PREFIX_one");
    msg.repeated_prefix_count = 1;
    strcpy(msg.repeated_contains[0], "user@test.com");
    msg.repeated_contains_count = 1;
    
    /* Set all callback fields to valid defaults */
    const char *valid_prefix = "PREFIX_valid";
    const char *valid_suffix = "valid_SUFFIX";
    const char *valid_contains = "has@sign";
    const char *valid_ascii = "ascii only";
    const char *valid_email = "test@test.com";
    const char *valid_in = "red";
    const char *valid_not_in = "allowed";
    
    msg.callback_prefix.funcs.encode = &encode_callback_string;
    msg.callback_prefix.arg = (void *)valid_prefix;
    msg.callback_suffix.funcs.encode = &encode_callback_string;
    msg.callback_suffix.arg = (void *)valid_suffix;
    msg.callback_contains.funcs.encode = &encode_callback_string;
    msg.callback_contains.arg = (void *)valid_contains;
    msg.callback_ascii.funcs.encode = &encode_callback_string;
    msg.callback_ascii.arg = (void *)valid_ascii;
    msg.callback_email.funcs.encode = &encode_callback_string;
    msg.callback_email.arg = (void *)valid_email;
    msg.callback_in.funcs.encode = &encode_callback_string;
    msg.callback_in.arg = (void *)valid_in;
    msg.callback_not_in.funcs.encode = &encode_callback_string;
    msg.callback_not_in.arg = (void *)valid_not_in;
    
    /* Override the field under test with test_str */
    switch (field_num) {
        case 12: msg.callback_prefix.arg = (void *)test_str; break;
        case 13: msg.callback_suffix.arg = (void *)test_str; break;
        case 14: msg.callback_contains.arg = (void *)test_str; break;
        case 15: msg.callback_ascii.arg = (void *)test_str; break;
        case 16: msg.callback_email.arg = (void *)test_str; break;
        case 17: msg.callback_in.arg = (void *)test_str; break;
        case 18: msg.callback_not_in.arg = (void *)test_str; break;
    }
    
    /* Encode */
    ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&ostream, &StringValidationEnvelope_msg, &msg)) {
        tests_failed++;
        printf("    [FAIL] Encode failed\n");
        return;
    }
    msg_len = ostream.bytes_written;
    
    /* Test via filter_udp */
    result = filter_udp(NULL, buffer, msg_len);
    
    if (expect_valid) {
        if (result == 0) {
            tests_passed++;
            printf("    [PASS] Valid callback string accepted\n");
        } else {
            tests_failed++;
            printf("    [FAIL] Expected valid, got %d\n", result);
        }
    } else {
        if (result != 0) {
            tests_passed++;
            printf("    [PASS] Invalid callback string rejected\n");
        } else {
            tests_failed++;
            printf("    [FAIL] Expected invalid\n");
        }
    }
}

/* Test callback string rules */
static void test_callback_string_rules(void)
{
    printf("\n=== Callback String Rules Tests (End-to-End via filter_udp) ===\n");
    
    /* callback_prefix (field 12) */
    test_callback_via_envelope("callback_prefix - valid", 12, "PREFIX_test", true);
    test_callback_via_envelope("callback_prefix - invalid", 12, "WRONG_test", false);
    
    /* callback_suffix (field 13) */
    test_callback_via_envelope("callback_suffix - valid", 13, "test_SUFFIX", true);
    test_callback_via_envelope("callback_suffix - invalid", 13, "test_WRONG", false);
    
    /* callback_contains (field 14) */
    test_callback_via_envelope("callback_contains - valid", 14, "has@sign", true);
    test_callback_via_envelope("callback_contains - invalid", 14, "no_at_sign", false);
    
    /* callback_ascii (field 15) */
    test_callback_via_envelope("callback_ascii - valid", 15, "Hello World", true);
    test_callback_via_envelope("callback_ascii - invalid", 15, "Caf\xc3\xa9", false);  /* UTF-8 'é' */
    
    /* callback_email (field 16) */
    test_callback_via_envelope("callback_email - valid", 16, "user@example.com", true);
    test_callback_via_envelope("callback_email - invalid", 16, "not_an_email", false);
    
    /* callback_in (field 17) */
    test_callback_via_envelope("callback_in - valid", 17, "red", true);  /* In: red, green, blue */
    test_callback_via_envelope("callback_in - invalid", 17, "purple", false);
    
    /* callback_not_in (field 18) */
    test_callback_via_envelope("callback_not_in - valid", 18, "allowed", true);
    test_callback_via_envelope("callback_not_in - invalid", 18, "FORBIDDEN", false);  /* Forbidden set */
}

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
 * REPEATED CALLBACK STRINGS, CALLBACK SUBMESSAGES, 
 * AND REPEATED CALLBACK SUBMESSAGES TESTS
 *======================================================================*/

/* Encode callback for repeated strings - encodes array of strings */
typedef struct {
    const char **strings;
    int count;
} repeated_string_ctx_t;

static bool encode_repeated_callback_string(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg)
{
    repeated_string_ctx_t *ctx = (repeated_string_ctx_t *)*arg;
    int i;
    
    for (i = 0; i < ctx->count; i++) {
        if (!pb_encode_tag_for_field(stream, field))
            return false;
        if (!pb_encode_string(stream, (const uint8_t *)ctx->strings[i], strlen(ctx->strings[i])))
            return false;
    }
    
    return true;
}

/* Encode callback for CallbackInnerMessage submessage */
typedef struct {
    const char *inner_str;
    int32_t inner_num;
} inner_msg_ctx_t;

static bool encode_inner_string(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg)
{
    const char *str = (const char *)*arg;
    (void)field;
    return pb_encode_string(stream, (const uint8_t *)str, strlen(str));
}

static bool encode_callback_submsg(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg)
{
    inner_msg_ctx_t *ctx = (inner_msg_ctx_t *)*arg;
    CallbackInnerMessage inner = CallbackInnerMessage_init_zero;
    
    inner.inner_str.funcs.encode = &encode_inner_string;
    inner.inner_str.arg = (void *)ctx->inner_str;
    inner.inner_num = ctx->inner_num;
    
    if (!pb_encode_tag_for_field(stream, field))
        return false;
    
    return pb_encode_submessage(stream, &CallbackInnerMessage_msg, &inner);
}

/* Encode callback for repeated CallbackInnerMessage submessages */
typedef struct {
    inner_msg_ctx_t *items;
    int count;
} repeated_inner_ctx_t;

static bool encode_repeated_callback_submsg(pb_ostream_t *stream, const pb_field_iter_t *field, void * const *arg)
{
    repeated_inner_ctx_t *ctx = (repeated_inner_ctx_t *)*arg;
    int i;
    
    for (i = 0; i < ctx->count; i++) {
        CallbackInnerMessage inner = CallbackInnerMessage_init_zero;
        inner.inner_str.funcs.encode = &encode_inner_string;
        inner.inner_str.arg = (void *)ctx->items[i].inner_str;
        inner.inner_num = ctx->items[i].inner_num;
        
        if (!pb_encode_tag_for_field(stream, field))
            return false;
        if (!pb_encode_submessage(stream, &CallbackInnerMessage_msg, &inner))
            return false;
    }
    
    return true;
}

/* Test repeated callback strings */
static void test_repeated_callback_strings(void)
{
    uint8_t buffer[2048];
    size_t msg_len;
    pb_ostream_t ostream;
    int result;
    
    printf("\n=== Repeated Callback Strings Tests (End-to-End via filter_udp) ===\n");
    
    /* Test valid repeated callback strings */
    TEST("repeated_callback_prefix - all valid");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set repeated callback strings to valid */
        const char *rep_strs[] = {"PREFIX_one", "PREFIX_two"};
        repeated_string_ctx_t rep_ctx = {rep_strs, 2};
        msg.repeated_callback_prefix.funcs.encode = &encode_repeated_callback_string;
        msg.repeated_callback_prefix.arg = &rep_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result, "all repeated callback strings valid");
    }
    
    /* Test invalid repeated callback string (one invalid) */
    TEST("repeated_callback_prefix - one invalid");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* One invalid string - missing prefix */
        const char *rep_strs[] = {"PREFIX_one", "WRONG_two"};  /* Second one invalid */
        repeated_string_ctx_t rep_ctx = {rep_strs, 2};
        msg.repeated_callback_prefix.funcs.encode = &encode_repeated_callback_string;
        msg.repeated_callback_prefix.arg = &rep_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "one repeated callback string invalid");
    }
}

/* Test callback submessages (nested message with callback fields) */
static void test_callback_submessages(void)
{
    uint8_t buffer[2048];
    size_t msg_len;
    pb_ostream_t ostream;
    int result;
    
    printf("\n=== Callback Submessage Tests (End-to-End via filter_udp) ===\n");
    
    /* Test valid callback submessage */
    TEST("callback_submsg - valid inner_str and inner_num");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set callback submessage - inner_str must start with INNER_, inner_num > 0 */
        inner_msg_ctx_t inner_ctx = {"INNER_test", 42};
        msg.callback_submsg.funcs.encode = &encode_callback_submsg;
        msg.callback_submsg.arg = &inner_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result, "callback submessage valid");
    }
    
    /* Test invalid callback submessage - inner_str wrong prefix */
    TEST("callback_submsg - invalid inner_str prefix");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set callback submessage with invalid inner_str (wrong prefix) */
        inner_msg_ctx_t inner_ctx = {"WRONG_prefix", 42};  /* Should be INNER_ */
        msg.callback_submsg.funcs.encode = &encode_callback_submsg;
        msg.callback_submsg.arg = &inner_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "callback submessage inner_str invalid prefix");
    }
    
    /* Test invalid callback submessage - inner_num invalid */
    TEST("callback_submsg - invalid inner_num (not > 0)");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set callback submessage with invalid inner_num (must be > 0) */
        inner_msg_ctx_t inner_ctx = {"INNER_test", 0};  /* 0 is not > 0 */
        msg.callback_submsg.funcs.encode = &encode_callback_submsg;
        msg.callback_submsg.arg = &inner_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "callback submessage inner_num invalid (not > 0)");
    }
}

/* Test repeated callback submessages */
static void test_repeated_callback_submessages(void)
{
    uint8_t buffer[2048];
    size_t msg_len;
    pb_ostream_t ostream;
    int result;
    
    printf("\n=== Repeated Callback Submessages Tests (End-to-End via filter_udp) ===\n");
    
    /* Test valid repeated callback submessages */
    TEST("repeated_callback_submsg - all valid");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set repeated callback submessages - all valid */
        inner_msg_ctx_t items[] = {
            {"INNER_one", 10},
            {"INNER_two", 20},
            {"INNER_three", 30}
        };
        repeated_inner_ctx_t rep_ctx = {items, 3};
        msg.repeated_callback_submsg.funcs.encode = &encode_repeated_callback_submsg;
        msg.repeated_callback_submsg.arg = &rep_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_VALID(result, "all repeated callback submessages valid");
    }
    
    /* Test invalid repeated callback submessages - second item invalid */
    TEST("repeated_callback_submsg - second item invalid inner_str");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set repeated callback submessages - second item has wrong prefix */
        inner_msg_ctx_t items[] = {
            {"INNER_one", 10},
            {"WRONG_two", 20},   /* Wrong prefix */
            {"INNER_three", 30}
        };
        repeated_inner_ctx_t rep_ctx = {items, 3};
        msg.repeated_callback_submsg.funcs.encode = &encode_repeated_callback_submsg;
        msg.repeated_callback_submsg.arg = &rep_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "second repeated callback submessage invalid");
    }
    
    /* Test invalid repeated callback submessages - third item inner_num invalid */
    TEST("repeated_callback_submsg - third item invalid inner_num");
    {
        StringValidationEnvelope msg = StringValidationEnvelope_init_zero;
        
        /* Set required regular fields to valid values */
        strcpy(msg.regular_prefix, "PREFIX_test");
        strcpy(msg.regular_suffix, "test_SUFFIX");
        strcpy(msg.regular_contains, "test@example.com");
        strcpy(msg.regular_ascii, "Hello World");
        strcpy(msg.regular_email, "user@example.com");
        strcpy(msg.regular_hostname, "www.example.com");
        strcpy(msg.regular_ip, "192.168.1.1");
        strcpy(msg.regular_in, "red");
        strcpy(msg.regular_not_in, "allowed");
        
        /* Set callback strings to valid */
        const char *cb_valid_prefix = "PREFIX_valid";
        const char *cb_valid_suffix = "valid_SUFFIX";
        const char *cb_valid_contains = "has@sign";
        const char *cb_valid_ascii = "ascii only";
        const char *cb_valid_email = "test@test.com";
        const char *cb_valid_in = "red";
        const char *cb_valid_not_in = "allowed";
        
        msg.callback_prefix.funcs.encode = &encode_callback_string;
        msg.callback_prefix.arg = (void *)cb_valid_prefix;
        msg.callback_suffix.funcs.encode = &encode_callback_string;
        msg.callback_suffix.arg = (void *)cb_valid_suffix;
        msg.callback_contains.funcs.encode = &encode_callback_string;
        msg.callback_contains.arg = (void *)cb_valid_contains;
        msg.callback_ascii.funcs.encode = &encode_callback_string;
        msg.callback_ascii.arg = (void *)cb_valid_ascii;
        msg.callback_email.funcs.encode = &encode_callback_string;
        msg.callback_email.arg = (void *)cb_valid_email;
        msg.callback_in.funcs.encode = &encode_callback_string;
        msg.callback_in.arg = (void *)cb_valid_in;
        msg.callback_not_in.funcs.encode = &encode_callback_string;
        msg.callback_not_in.arg = (void *)cb_valid_not_in;
        
        /* Set repeated callback submessages - third item has invalid inner_num */
        inner_msg_ctx_t items[] = {
            {"INNER_one", 10},
            {"INNER_two", 20},
            {"INNER_three", -5}  /* Invalid: not > 0 */
        };
        repeated_inner_ctx_t rep_ctx = {items, 3};
        msg.repeated_callback_submsg.funcs.encode = &encode_repeated_callback_submsg;
        msg.repeated_callback_submsg.arg = &rep_ctx;
        
        /* Encode and test */
        ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        assert(pb_encode(&ostream, &StringValidationEnvelope_msg, &msg));
        msg_len = ostream.bytes_written;
        result = filter_udp(NULL, buffer, msg_len);
        EXPECT_INVALID(result, "third repeated callback submessage invalid inner_num");
    }
}

/*======================================================================
 * MAIN
 *======================================================================*/

int main(void)
{
    printf("=== String Validation Test Suite ===\n");
    printf("Tests string validation rules for regular, repeated, and callback strings\n");
    printf("using filter_udp for end-to-end validation.\n");
    printf("Callback strings now fully support content-based validation (PREFIX, SUFFIX, etc.)\n");
    
    /* Run tests */
    test_regular_string_rules();
    test_repeated_string_rules();
    test_callback_string_rules();
    test_filter_udp_envelope();
    test_repeated_callback_strings();
    test_callback_submessages();
    test_repeated_callback_submessages();
    
    /* Summary */
    printf("\n===========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
