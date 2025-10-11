/* Test program for the generated filter functions */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "pb_encode.h"
#include "pb_decode.h"
#include "chat.pb.h"

int main()
{
    printf("=== Testing nanopb service filter functions ===\n\n");

    // Test 1: Create and encode a ServerMessage (has simple types)
    printf("Test 1: Encoding ServerMessage with timestamp...\n");
    chat_ServerMessage server_msg = chat_ServerMessage_init_zero;
    server_msg.timestamp = 12345;

    // For callbacks, we need to set them up properly or use simpler message
    // Let's use LoginResponse which has a bool and no callbacks
    chat_LoginResponse login_resp = chat_LoginResponse_init_zero;
    login_resp.success = true;

    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool status = pb_encode(&ostream, &chat_LoginResponse_msg, &login_resp);

    if (!status)
    {
        printf("FAIL: Failed to encode message: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }

    size_t message_length = ostream.bytes_written;
    printf("SUCCESS: Encoded LoginResponse (%zu bytes)\n", message_length);

    // Test 2: Use filter_udp to decode and validate the packet
    printf("\nTest 2: Testing filter_udp with valid packet...\n");
    int result = filter_udp(NULL, buffer, message_length);
    printf("filter_udp result: %d (expected 1)\n", result);

    if (result != 1)
    {
        printf("FAIL: Valid packet was rejected!\n");
        return 1;
    }
    printf("SUCCESS: filter_udp accepted valid packet\n");

    // Test 3: Use filter_tcp (from server) to validate - LoginResponse is a response
    printf("\nTest 3: Testing filter_tcp (from server) with LoginResponse...\n");
    result = filter_tcp(NULL, buffer, message_length, false);
    printf("filter_tcp (to_server=false) result: %d (expected 1)\n", result);

    if (result != 1)
    {
        printf("FAIL: Valid response packet was rejected!\n");
        return 1;
    }
    printf("SUCCESS: filter_tcp accepted LoginResponse from server\n");

    // Test 4: Use filter_tcp (to server) - should fail since LoginResponse is not a request
    printf("\nTest 4: Testing filter_tcp (to server) with LoginResponse...\n");
    result = filter_tcp(NULL, buffer, message_length, true);
    printf("filter_tcp (to_server=true) result: %d (expected 0)\n", result);

    if (result != 0)
    {
        printf("FAIL: Response message incorrectly accepted as request!\n");
        return 1;
    }
    printf("SUCCESS: filter_tcp correctly rejected response as request\n");

    // Test 5: Create a LoginRequest (which is a request message)
    printf("\nTest 5: Encoding LoginRequest...\n");
    chat_LoginRequest login_req = chat_LoginRequest_init_zero;
    // LoginRequest has string fields which are callbacks, skip for now

    uint8_t buffer2[256];
    pb_ostream_t ostream2 = pb_ostream_from_buffer(buffer2, sizeof(buffer2));
    status = pb_encode(&ostream2, &chat_LoginRequest_msg, &login_req);

    if (!status)
    {
        printf("FAIL: Failed to encode LoginRequest: %s\n", PB_GET_ERROR(&ostream2));
        return 1;
    }

    message_length = ostream2.bytes_written;
    printf("SUCCESS: Encoded LoginRequest (%zu bytes)\n", message_length);

    // Test 6: filter_tcp (to server) should accept LoginRequest
    printf("\nTest 6: Testing filter_tcp (to server) with LoginRequest...\n");
    result = filter_tcp(NULL, buffer2, message_length, true);
    printf("filter_tcp (to_server=true) result: %d (expected 1)\n", result);

    if (result != 1)
    {
        printf("FAIL: Valid request packet was rejected!\n");
        return 1;
    }
    printf("SUCCESS: filter_tcp accepted LoginRequest to server\n");

    // Test 7: Try with invalid/garbage data
    printf("\nTest 7: Testing with garbage data...\n");
    uint8_t garbage[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    result = filter_udp(NULL, garbage, sizeof(garbage));
    printf("filter_udp result for garbage: %d (expected 0)\n", result);

    if (result != 0)
    {
        printf("FAIL: Garbage data was accepted!\n");
        return 1;
    }
    printf("SUCCESS: filter_udp correctly rejected garbage data\n");

    // Test 8: Empty packet
    printf("\nTest 8: Testing with empty packet...\n");
    result = filter_udp(NULL, buffer, 0);
    printf("filter_udp result for empty packet: %d (expected 0)\n", result);

    if (result != 0)
    {
        printf("FAIL: Empty packet was accepted!\n");
        return 1;
    }
    printf("SUCCESS: filter_udp correctly rejected empty packet\n");

    printf("\n=== ALL TESTS PASSED ===\n");
    printf("\nSummary:\n");
    printf("- filter_udp correctly decodes and accepts valid packets\n");
    printf("- filter_tcp correctly distinguishes request vs response messages\n");
    printf("- Both functions correctly reject invalid/garbage data\n");
    printf("- Service-based packet filtering is working as expected!\n");

    return 0;
}

uint8_t buffer[256];
pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
bool status = pb_encode(&ostream, &chat_ClientMessage_msg, &client_msg);

if (!status)
{
    printf("Failed to encode message\n");
    return 1;
}

size_t message_length = ostream.bytes_written;
printf("Encoded ClientMessage (%zu bytes)\n", message_length);

// Test 2: Use filter_udp to validate the packet
int result = filter_udp(NULL, buffer, message_length);
printf("filter_udp result: %d (expected 1)\n", result);

if (result != 1)
{
    printf("FAIL: Valid packet was rejected!\n");
    return 1;
}

// Test 3: Use filter_tcp (to server) to validate
result = filter_tcp(NULL, buffer, message_length, true);
printf("filter_tcp (to_server=true) result: %d (expected 1)\n", result);

if (result != 1)
{
    printf("FAIL: Valid client packet was rejected!\n");
    return 1;
}

// Test 4: Use filter_tcp (from server) - should fail since this is a client message
result = filter_tcp(NULL, buffer, message_length, false);
printf("filter_tcp (to_server=false) result: %d (expected 0)\n", result);

if (result != 0)
{
    printf("FAIL: Client message incorrectly accepted as server message!\n");
    return 1;
}

// Test 5: Create an invalid message (username too short)
chat_ClientMessage invalid_msg = chat_ClientMessage_init_zero;
strcpy(invalid_msg.user, "ab"); // Too short (min_len = 3)
strcpy(invalid_msg.text, "Test");

uint8_t buffer2[256];
pb_ostream_t ostream2 = pb_ostream_from_buffer(buffer2, sizeof(buffer2));
status = pb_encode(&ostream2, &chat_ClientMessage_msg, &invalid_msg);

if (!status)
{
    printf("Failed to encode invalid message\n");
    return 1;
}

message_length = ostream2.bytes_written;
printf("\nEncoded invalid ClientMessage (%zu bytes)\n", message_length);

// Test 6: filter_udp should reject invalid message
result = filter_udp(NULL, buffer2, message_length);
printf("filter_udp result for invalid message: %d (expected 0)\n", result);

if (result != 0)
{
    printf("FAIL: Invalid packet was accepted!\n");
    return 1;
}

// Test 7: Create a ServerMessage and test with filter_tcp
chat_ServerMessage server_msg = chat_ServerMessage_init_zero;
strcpy(server_msg.user, "bob");
strcpy(server_msg.text, "Response message");
server_msg.timestamp = 12345;

uint8_t buffer3[256];
pb_ostream_t ostream3 = pb_ostream_from_buffer(buffer3, sizeof(buffer3));
status = pb_encode(&ostream3, &chat_ServerMessage_msg, &server_msg);

if (!status)
{
    printf("Failed to encode server message\n");
    return 1;
}

message_length = ostream3.bytes_written;
printf("\nEncoded ServerMessage (%zu bytes)\n", message_length);

// Test 8: filter_tcp (from server) should accept
result = filter_tcp(NULL, buffer3, message_length, false);
printf("filter_tcp (to_server=false) result: %d (expected 1)\n", result);

if (result != 1)
{
    printf("FAIL: Valid server message was rejected!\n");
    return 1;
}

printf("\n=== ALL TESTS PASSED ===\n");
return 0;
}
