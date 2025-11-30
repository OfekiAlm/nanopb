/* proto_filter.c: Implementation for generic filter wrapper
 *
 * This module provides a simple filter wrapper that decodes and validates
 * protobuf messages. It is primarily intended for packet inspection/filtering
 * use cases where messages need to be validated before further processing.
 *
 * Thread Safety: This implementation uses a global filter registration and
 * is NOT thread-safe. For multi-threaded use, consider passing the filter
 * specification as a parameter instead.
 */

#include "proto_filter.h"
#include <string.h>
#include <stdlib.h>

/* Maximum size for stack-allocated message buffer.
 * Messages larger than this will use heap allocation. */
#define PROTO_FILTER_STACK_BUFFER_SIZE 1024U

/* Global filter registration (NOT thread-safe) */
static const proto_filter_spec_t *g_active_filter = NULL;

void proto_filter_register(const proto_filter_spec_t *spec)
{
    g_active_filter = spec;
}

static int proto_filter_process(void *user_ctx, char *packet, size_t packet_size, bool is_to_server)
{
    uint8_t msgbuf[PROTO_FILTER_STACK_BUFFER_SIZE];
    void *msg = NULL;
    pb_istream_t istream;
    pb_violations_t viol;
    bool ok;
    int result;

    if (!packet || packet_size == 0)
    {
        return PROTO_FILTER_ERR_INVALID_INPUT;
    }
    if (!g_active_filter || !g_active_filter->msg_desc || g_active_filter->msg_size == 0)
    {
        return PROTO_FILTER_ERR_NOT_REGISTERED;
    }

    /* Allocate message buffer: stack if small enough, heap otherwise */
    if (g_active_filter->msg_size <= sizeof(msgbuf))
    {
        memset(msgbuf, 0, g_active_filter->msg_size);
        msg = msgbuf;
    }
    else
    {
        /* Heap allocation for large message structs */
        msg = malloc(g_active_filter->msg_size);
        if (!msg)
            return PROTO_FILTER_ERR_DECODE;
        memset(msg, 0, g_active_filter->msg_size);
    }

    /* Optional: set up callback fields prior to decode */
    if (g_active_filter->prepare_decode)
    {
        g_active_filter->prepare_decode(msg, user_ctx, is_to_server);
    }

    /* Decode the protobuf message */
    istream = pb_istream_from_buffer((const pb_byte_t *)packet, (pb_size_t)packet_size);
    if (!pb_decode(&istream, g_active_filter->msg_desc, msg))
    {
        if (msg != (void *)msgbuf)
            free(msg);
        return PROTO_FILTER_ERR_DECODE;
    }

    /* Validate if validator is registered */
    if (!g_active_filter->validate)
    {
        if (msg != (void *)msgbuf)
            free(msg);
        return PROTO_FILTER_ERR_NO_VALIDATOR;
    }

    pb_violations_init(&viol);
    ok = g_active_filter->validate(msg, &viol);
    result = ok ? PROTO_FILTER_OK : PROTO_FILTER_ERR_DECODE;

    if (msg != (void *)msgbuf)
        free(msg);

    return result;
}

int filter_tcp(void *ctx, char *packet, size_t packet_size, bool is_to_server)
{
    return proto_filter_process(ctx, packet, packet_size, is_to_server);
}

int filter_udp(void *ctx, char *packet, size_t packet_size, bool is_to_server)
{
    return proto_filter_process(ctx, packet, packet_size, is_to_server);
}
