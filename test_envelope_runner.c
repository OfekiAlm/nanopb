/* Test program for envelope-based decoding */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "test_envelope.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#define BUFFER_SIZE 1024

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            printf("PASS: %s\n", msg); \
            tests_passed++; \
        } else { \
            printf("FAIL: %s\n", msg); \
            tests_failed++; \
        } \
    } while(0)

// Helper function to encode an envelope
bool encode_envelope(test_Envelope *envelope, uint8_t *buffer, size_t *size) {
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, BUFFER_SIZE);
    bool status = pb_encode(&stream, &test_Envelope_msg, envelope);
    if (status) {
        *size = stream.bytes_written;
    }
    return status;
}

// Test 1: Encode and filter a Ping message
void test_ping_message() {
    printf("\n=== Test 1: Ping Message ===\n");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t size;
    
    // Create envelope with Ping
    test_Envelope envelope = test_Envelope_init_zero;
    envelope.version = 1;
    envelope.msg_type = test_MessageType_MSG_PING;
    envelope.correlation_id = 12345;
    envelope.which_message = test_Envelope_message_ping_tag;
    envelope.message.ping.timestamp = 1000000;
    envelope.message.ping.sequence = 1;
    
    // Encode
    bool encode_status = encode_envelope(&envelope, buffer, &size);
    TEST_ASSERT(encode_status, "Ping message encoded successfully");
    printf("  Encoded size: %zu bytes\n", size);
    
    // Filter using UDP filter (should accept)
    int filter_result = filter_udp(NULL, buffer, size);
    TEST_ASSERT(filter_result == 1, "UDP filter accepts valid Ping message");
    
    // Filter using TCP filter (should accept)
    filter_result = filter_tcp(NULL, buffer, size, true);
    TEST_ASSERT(filter_result == 1, "TCP filter accepts valid Ping message");
}

// Test 2: Encode and filter a Request message
void test_request_message() {
    printf("\n=== Test 2: Request Message ===\n");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t size;
    
    // Create envelope with Request
    test_Envelope envelope = test_Envelope_init_zero;
    envelope.version = 1;
    envelope.msg_type = test_MessageType_MSG_REQUEST;
    envelope.correlation_id = 12347;
    envelope.which_message = test_Envelope_message_request_tag;
    envelope.message.request.request_id = 100;
    
    // Note: method and payload are callbacks, so we'll skip setting them for this test
    // In a real scenario, you'd set up callback functions
    
    // Encode
    bool encode_status = encode_envelope(&envelope, buffer, &size);
    TEST_ASSERT(encode_status, "Request message encoded successfully");
    printf("  Encoded size: %zu bytes\n", size);
    
    // Filter
    int filter_result = filter_udp(NULL, buffer, size);
    TEST_ASSERT(filter_result == 1, "Filter accepts valid Request message");
}

// Test 3: Test with wrong opcode (opcode doesn't match which_message)
void test_opcode_mismatch() {
    printf("\n=== Test 3: Opcode Mismatch ===\n");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t size;
    
    // Create envelope with mismatched opcode and message
    test_Envelope envelope = test_Envelope_init_zero;
    envelope.version = 1;
    envelope.msg_type = test_MessageType_MSG_PING;  // Says it's a Ping
    envelope.correlation_id = 99999;
    envelope.which_message = test_Envelope_message_pong_tag;  // But actually a Pong
    envelope.message.pong.timestamp = 4000000;
    envelope.message.pong.sequence = 999;
    
    // Encode
    bool encode_status = encode_envelope(&envelope, buffer, &size);
    TEST_ASSERT(encode_status, "Mismatched message encoded successfully");
    
    // Filter should reject (opcode doesn't match which_message)
    int filter_result = filter_udp(NULL, buffer, size);
    TEST_ASSERT(filter_result == 0, "Filter rejects message with opcode mismatch");
}

// Test 4: Test with invalid/corrupted data
void test_invalid_data() {
    printf("\n=== Test 4: Invalid Data ===\n");
    
    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0xFF, 50);  // Fill with garbage
    
    // Filter should reject
    int filter_result = filter_udp(NULL, buffer, 50);
    TEST_ASSERT(filter_result == 0, "Filter rejects corrupted data");
}

// Test 5: Performance test
void test_performance() {
    printf("\n=== Test 5: Performance Test ===\n");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t size;
    
    // Create a sample envelope
    test_Envelope envelope = test_Envelope_init_zero;
    envelope.version = 1;
    envelope.msg_type = test_MessageType_MSG_PING;
    envelope.correlation_id = 99999;
    envelope.which_message = test_Envelope_message_ping_tag;
    envelope.message.ping.timestamp = 1000000;
    envelope.message.ping.sequence = 1;
    
    encode_envelope(&envelope, buffer, &size);
    
    // Run filter many times
    const int iterations = 10000;
    clock_t start = clock();
    
    int success_count = 0;
    for (int i = 0; i < iterations; i++) {
        if (filter_udp(NULL, buffer, size) == 1) {
            success_count++;
        }
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("  Processed %d messages in %.3f seconds\n", iterations, cpu_time);
    printf("  Average time per message: %.3f microseconds\n", (cpu_time * 1000000) / iterations);
    
    TEST_ASSERT(success_count == iterations, "All performance test iterations succeeded");
}

int main() {
    printf("=== Envelope-Based Decoding Comprehensive Test ===\n");
    
    // Run all tests
    test_ping_message();
    test_request_message();
    test_opcode_mismatch();
    test_invalid_data();
    test_performance();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Total Tests:  %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED! Envelope-based decoding works correctly!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED. Please review the output above.\n");
        return 1;
    }
}
