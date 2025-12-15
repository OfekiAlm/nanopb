/* Fuzz testing for nanopb validation feature.
 * Tests that validation constraints work correctly with random/corrupted data.
 * 
 * This program can run in two modes:
 * - Standalone fuzzer, generating its own inputs
 * - Fuzzing target, reading input on stdin
 */

#include <pb_decode.h>
#include <pb_encode.h>
#include <pb_validate.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fuzz_validation.pb.h"
#include "fuzz_validation_validate.h"
#include "test_helpers.h"

#ifndef FUZZTEST_BUFSIZE
#define FUZZTEST_BUFSIZE 4096
#endif

static size_t g_bufsize = FUZZTEST_BUFSIZE;

/* Statistics counters */
static unsigned long g_valid_messages = 0;
static unsigned long g_invalid_messages = 0;
static unsigned long g_validation_detected = 0;
static unsigned long g_decode_failed = 0;

/* Helper to generate a valid message for testing */
static bool generate_valid_message(FuzzMessage *msg)
{
    memset(msg, 0, sizeof(FuzzMessage));
    
    /* Set valid values */
    msg->age = 25;
    msg->count = 100;
    msg->value = 5000;
    msg->score = 75.5f;
    msg->rating = 4.2;
    msg->status = FuzzMessage_Status_ACTIVE;
    msg->enabled = true;
    
    return true;
}

/* Test validation on a message */
static void test_validation(const uint8_t *buffer, size_t msglen)
{
    pb_istream_t stream;
    FuzzMessage msg = FuzzMessage_init_zero;
    pb_violations_t violations;
    bool decode_status;
    bool validate_status;
    
    /* First try to decode the message */
    stream = pb_istream_from_buffer(buffer, msglen);
    decode_status = pb_decode(&stream, FuzzMessage_fields, &msg);
    
    if (!decode_status)
    {
        /* Decode failed - this is expected for corrupted data */
        g_decode_failed++;
        return;
    }
    
    /* Message decoded successfully, now validate it */
    pb_violations_init(&violations);
    validate_status = pb_validate_FuzzMessage(&msg, &violations);
    
    if (validate_status)
    {
        /* Message is valid according to validation rules */
        g_valid_messages++;
    }
    else
    {
        /* Validation detected constraint violations */
        g_validation_detected++;
        
        /* Optionally print violation details for debugging */
        if (violations.count > 0 && 0) /* Set to 1 to enable verbose output */
        {
            printf("Validation failed with %u violations:\n", (unsigned)violations.count);
            for (pb_size_t i = 0; i < violations.count; i++)
            {
                printf("  [%u] %s: %s (%s)\n", 
                       (unsigned)i,
                       violations.violations[i].field_path ? violations.violations[i].field_path : "?",
                       violations.violations[i].message ? violations.violations[i].message : "?",
                       violations.violations[i].constraint_id ? violations.violations[i].constraint_id : "?");
            }
        }
    }
    
    /* Invalid messages should be caught by validation, not cause crashes */
    pb_release(FuzzMessage_fields, &msg);
}

/* Fuzz entry point for libFuzzer */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > g_bufsize)
        return 0;
    
    test_validation(data, size);
    return 0;
}

#ifndef LLVMFUZZER

/* Standalone fuzzer mode - generates its own test data */
static void run_iteration(unsigned long seed)
{
    uint8_t *buffer = malloc(g_bufsize);
    FuzzMessage msg;
    pb_ostream_t ostream;
    size_t msglen;
    
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate buffer\n");
        return;
    }
    
    /* Generate a valid message */
    generate_valid_message(&msg);
    
    /* Encode it */
    ostream = pb_ostream_from_buffer(buffer, g_bufsize);
    if (pb_encode(&ostream, FuzzMessage_fields, &msg))
    {
        msglen = ostream.bytes_written;
        
        /* Test the valid message */
        test_validation(buffer, msglen);
        
        /* Now corrupt the data by flipping random bytes */
        for (size_t i = 0; i < (seed % 10) + 1; i++)
        {
            if (msglen > 0)
            {
                size_t pos = (seed * (i + 1)) % msglen;
                buffer[pos] ^= (seed >> (i * 3)) & 0xFF;
            }
        }
        
        /* Test the corrupted message */
        test_validation(buffer, msglen);
        
        /* Test with random length */
        if (msglen > 1)
        {
            size_t random_len = (seed % msglen);
            test_validation(buffer, random_len);
        }
        
        /* Test with extended length (may read past valid data) */
        if (msglen < g_bufsize / 2)
        {
            size_t extended_len = msglen + (seed % 100);
            if (extended_len <= g_bufsize)
            {
                test_validation(buffer, extended_len);
            }
        }
    }
    
    free(buffer);
}

int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        /* Standalone mode with seed and optional iteration count */
        unsigned long seed = strtoul(argv[1], NULL, 0);
        int iterations = (argc >= 3) ? atoi(argv[2]) : 100;
        
        printf("Running validation fuzz test with seed %lu for %d iterations\n", 
               seed, iterations);
        
        for (int i = 0; i < iterations; i++)
        {
            run_iteration(seed + i);
            
            if ((i + 1) % 10 == 0)
            {
                printf("Iteration %d/%d: valid=%lu invalid=%lu detected=%lu decode_failed=%lu\n",
                       i + 1, iterations, 
                       g_valid_messages, g_invalid_messages,
                       g_validation_detected, g_decode_failed);
            }
        }
        
        printf("\nFinal statistics:\n");
        printf("  Valid messages:       %lu\n", g_valid_messages);
        printf("  Invalid messages:     %lu\n", g_invalid_messages);
        printf("  Validation detected:  %lu\n", g_validation_detected);
        printf("  Decode failed:        %lu\n", g_decode_failed);
        printf("\nTest completed successfully!\n");
    }
    else
    {
        /* AFL/stdin fuzzer mode */
        uint8_t *buffer = malloc(g_bufsize);
        size_t msglen;
        
        if (!buffer)
        {
            fprintf(stderr, "Failed to allocate buffer\n");
            return 1;
        }
        
        SET_BINARY_MODE(stdin);
        msglen = fread(buffer, 1, g_bufsize, stdin);
        
        test_validation(buffer, msglen);
        
        free(buffer);
    }
    
    return 0;
}

#endif
