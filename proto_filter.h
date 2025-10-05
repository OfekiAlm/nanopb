/* proto_filter.h: Generic filter wrapper for nanopb + validation */
#ifndef PROTO_FILTER_H_INCLUDED
#define PROTO_FILTER_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "pb.h"
#include "pb_decode.h"
#include "pb_validate.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Return codes for filter functions */
    enum
    {
        PROTO_FILTER_OK = 0,
        PROTO_FILTER_ERR_INVALID_INPUT = -1,
        PROTO_FILTER_ERR_NOT_REGISTERED = -2,
        PROTO_FILTER_ERR_DECODE = -3,
        PROTO_FILTER_ERR_NO_VALIDATOR = -4
    };

    /* Validator adapter signature: cast the msg pointer to the concrete type and call the generated pb_validate_... */
    typedef bool (*proto_filter_validate_fn)(const void *msg, pb_violations_t *violations);

    /* Optional hook to set up decode callbacks / buffers before calling pb_decode. */
    typedef void (*proto_filter_prepare_decode_fn)(void *msg, void *user_ctx, bool is_to_server);

    typedef struct proto_filter_spec_s
    {
        const pb_msgdesc_t *msg_desc;                  /* nanopb message descriptor (e.g., &test_BasicValidation_msg) */
        size_t msg_size;                               /* sizeof(message struct) */
        proto_filter_validate_fn validate;             /* adapter to generated validator: returns true if valid */
        proto_filter_prepare_decode_fn prepare_decode; /* optional */
    } proto_filter_spec_t;

    /* Register the active filter. Thread-unsafe simple global registration. */
    void proto_filter_register(const proto_filter_spec_t *spec);

    /* Wrappers: decode packet and validate using the registered filter. */
    int filter_tcp(void *ctx, char *packet, size_t packet_size, bool is_to_server);
    int filter_udp(void *ctx, char *packet, size_t packet_size, bool is_to_server);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PROTO_FILTER_H_INCLUDED */
