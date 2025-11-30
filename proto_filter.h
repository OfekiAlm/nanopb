/* proto_filter.h: Generic filter wrapper for nanopb + validation
 *
 * This header provides a simple interface for decoding and validating
 * protobuf messages from network packets.
 *
 * Example usage:
 *   static bool my_validator(const void *msg, pb_violations_t *v) {
 *       return pb_validate_MyMessage((const MyMessage *)msg, v);
 *   }
 *
 *   static const proto_filter_spec_t my_filter = {
 *       .msg_desc = &MyMessage_msg,
 *       .msg_size = sizeof(MyMessage),
 *       .validate = my_validator,
 *       .prepare_decode = NULL
 *   };
 *
 *   proto_filter_register(&my_filter);
 *   int result = filter_tcp(ctx, packet, len, true);
 */
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
    typedef enum proto_filter_result_e
    {
        PROTO_FILTER_OK = 0,
        PROTO_FILTER_ERR_INVALID_INPUT = -1,
        PROTO_FILTER_ERR_NOT_REGISTERED = -2,
        PROTO_FILTER_ERR_DECODE = -3,
        PROTO_FILTER_ERR_NO_VALIDATOR = -4
    } proto_filter_result_t;

    /* Validator adapter signature: cast the msg pointer to the concrete type and call the generated pb_validate_... */
    typedef bool (*proto_filter_validate_fn)(const void *msg, pb_violations_t *violations);

    /* Optional hook to set up decode callbacks / buffers before calling pb_decode. */
    typedef void (*proto_filter_prepare_decode_fn)(void *msg, void *user_ctx, bool is_to_server);

    /* Filter specification structure */
    typedef struct proto_filter_spec_s
    {
        const pb_msgdesc_t *msg_desc;                  /* nanopb message descriptor (e.g., &MyMessage_msg) */
        size_t msg_size;                               /* sizeof(message struct) */
        proto_filter_validate_fn validate;             /* adapter to generated validator: returns true if valid */
        proto_filter_prepare_decode_fn prepare_decode; /* optional: called before pb_decode */
    } proto_filter_spec_t;

    /* Register the active filter.
     * WARNING: This function is NOT thread-safe. Uses a global variable.
     * For thread-safe usage, consider passing filter spec directly to a
     * modified filter function.
     */
    void proto_filter_register(const proto_filter_spec_t *spec);

    /* Process a TCP packet: decode and validate using the registered filter.
     * Returns PROTO_FILTER_OK on success, negative error code on failure.
     */
    int filter_tcp(void *ctx, char *packet, size_t packet_size, bool is_to_server);

    /* Process a UDP packet: decode and validate using the registered filter.
     * Returns PROTO_FILTER_OK on success, negative error code on failure.
     */
    int filter_udp(void *ctx, char *packet, size_t packet_size, bool is_to_server);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PROTO_FILTER_H_INCLUDED */
