/* Validation implementation for test_envelope.proto */
#include "test_envelope_validate.h"
#include <string.h>

bool pb_validate_test_Ping(const test_Ping *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - sequence
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: timestamp */
    PBV_FIELD("timestamp");
    {
        /* Rule: uint64.gt */
        PBV_CHECK_NUM(pb_validate_uint64, uint64_t, msg->timestamp, 0, PB_VALIDATE_RULE_GT, "uint64.gt");
    }
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Pong(const test_Pong *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - latency_ms
       - sequence
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: timestamp */
    PBV_FIELD("timestamp");
    {
        /* Rule: uint64.gt */
        PBV_CHECK_NUM(pb_validate_uint64, uint64_t, msg->timestamp, 0, PB_VALIDATE_RULE_GT, "uint64.gt");
    }
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Request(const test_Request *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - request_id
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: method */
    PBV_FIELD("method");
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->method, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    PBV_FIELD_END();
    
    /* Validate field: payload */
    PBV_FIELD("payload");
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Response(const test_Response *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - request_id
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: status_code */
    PBV_FIELD("status_code");
    {
        /* Rule: int32.gte */
        PBV_CHECK_NUM(pb_validate_int32, int32_t, msg->status_code, 0, PB_VALIDATE_RULE_GTE, "int32.gte");
    }
    {
        /* Rule: int32.lt */
        PBV_CHECK_NUM(pb_validate_int32, int32_t, msg->status_code, 600, PB_VALIDATE_RULE_LT, "int32.lt");
    }
    PBV_FIELD_END();
    
    /* Validate field: payload */
    PBV_FIELD("payload");
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Error(const test_Error *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - details
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: error_code */
    PBV_FIELD("error_code");
    {
        /* Rule: uint32.gt */
        PBV_CHECK_NUM(pb_validate_uint32, uint32_t, msg->error_code, 0, PB_VALIDATE_RULE_GT, "uint32.gt");
    }
    PBV_FIELD_END();
    
    /* Validate field: message */
    PBV_FIELD("message");
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->message, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Notification(const test_Notification *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - data
       - timestamp
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: event_type */
    PBV_FIELD("event_type");
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->event_type, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Envelope(const test_Envelope *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - correlation_id
       - message
    */

    if (!msg) return false;
    PBV_INIT();

    /* Validate field: version */
    PBV_FIELD("version");
    {
        /* Rule: uint32.gte */
        PBV_CHECK_NUM(pb_validate_uint32, uint32_t, msg->version, 1, PB_VALIDATE_RULE_GTE, "uint32.gte");
    }
    {
        /* Rule: uint32.lte */
        PBV_CHECK_NUM(pb_validate_uint32, uint32_t, msg->version, 10, PB_VALIDATE_RULE_LTE, "uint32.lte");
    }
    PBV_FIELD_END();
    
    /* Validate field: msg_type */
    PBV_FIELD("msg_type");
    {
        /* Rule: enum.defined_only */
        {
            static const int __pb_defined_vals[] = { 0, 1, 2, 3, 4, 5, 6 };
            if (!pb_validate_enum_defined_only((int)msg->msg_type, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {
                pb_violations_add(violations, ctx.path_buffer, "enum.defined_only", "Value must be a defined enum value");
                if (ctx.early_exit) return false;
            }
        }
    }
    PBV_FIELD_END();
    
    return !pb_violations_has_any(violations);
}

