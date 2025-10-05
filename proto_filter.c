/* proto_filter.c: Implementation for generic filter wrapper */

#include "proto_filter.h"
#include <string.h>
#include <stdlib.h>

static const proto_filter_spec_t *g_active_filter = NULL;

void proto_filter_register(const proto_filter_spec_t *spec)
{
    g_active_filter = spec;
}

static int proto_filter_process(void *user_ctx, char *packet, size_t packet_size, bool is_to_server)
{
    if (!packet || packet_size == 0)
    {
        return PROTO_FILTER_ERR_INVALID_INPUT;
    }
    if (!g_active_filter || !g_active_filter->msg_desc || !g_active_filter->msg_size)
    {
        return PROTO_FILTER_ERR_NOT_REGISTERED;
    }

    /* Allocate message on stack and zero-init */
    uint8_t msgbuf[1024]; /* fallback if message size is modest */
    void *msg = NULL;
    if (g_active_filter->msg_size <= sizeof(msgbuf))
    {
        memset(msgbuf, 0, sizeof(msgbuf));
        msg = msgbuf;
    }
    else
    {
        /* If the struct is larger than our small buffer, allocate dynamically */
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

    /* Decode */
    pb_istream_t istream = pb_istream_from_buffer((const pb_byte_t *)packet, (pb_size_t)packet_size);
    if (!pb_decode(&istream, g_active_filter->msg_desc, msg))
    {
        if (msg != msgbuf)
            free(msg);
        return PROTO_FILTER_ERR_DECODE;
    }

    if (!g_active_filter->validate)
    {
        if (msg != msgbuf)
            free(msg);
        return PROTO_FILTER_ERR_NO_VALIDATOR;
    }

    /* Validate */
    pb_violations_t viol;
    pb_violations_init(&viol);
    bool ok = g_active_filter->validate(msg, &viol);

    if (msg != msgbuf)
        free(msg);

    return ok ? PROTO_FILTER_OK : PROTO_FILTER_ERR_DECODE; /* use decode error code for invalid */
}

int filter_tcp(void *ctx, char *packet, size_t packet_size, bool is_to_server)
{
    return proto_filter_process(ctx, packet, packet_size, is_to_server);
}

int filter_udp(void *ctx, char *packet, size_t packet_size, bool is_to_server)
{
    return proto_filter_process(ctx, packet, packet_size, is_to_server);
}
