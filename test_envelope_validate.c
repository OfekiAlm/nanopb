/* Validation implementation for test_envelope.proto */
#include "test_envelope_validate.h"
#include <string.h>

bool pb_validate_test_Ping(const test_Ping *msg, pb_violations_t *violations)
{
    /* Fields without constraints: sequence */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: timestamp */
    if (!pb_validate_context_push_field(&ctx, "timestamp")) return false;
    {
        /* Rule: uint64.gt */
        {
            uint64_t expected = (uint64_t)0;
            if (!pb_validate_uint64(msg->timestamp, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "uint64.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Pong(const test_Pong *msg, pb_violations_t *violations)
{
    /* Fields without constraints: latency_ms, sequence */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: timestamp */
    if (!pb_validate_context_push_field(&ctx, "timestamp")) return false;
    {
        /* Rule: uint64.gt */
        {
            uint64_t expected = (uint64_t)0;
            if (!pb_validate_uint64(msg->timestamp, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "uint64.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Request(const test_Request *msg, pb_violations_t *violations)
{
    /* Fields without constraints: request_id */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: method */
    if (!pb_validate_context_push_field(&ctx, "method")) return false;
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
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: payload */
    if (!pb_validate_context_push_field(&ctx, "payload")) return false;
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Response(const test_Response *msg, pb_violations_t *violations)
{
    /* Fields without constraints: request_id */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: status_code */
    if (!pb_validate_context_push_field(&ctx, "status_code")) return false;
    {
        /* Rule: int32.gte */
        {
            int32_t expected = (int32_t)0;
            if (!pb_validate_int32(msg->status_code, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: int32.lt */
        {
            int32_t expected = (int32_t)600;
            if (!pb_validate_int32(msg->status_code, &expected, PB_VALIDATE_RULE_LT)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.lt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: payload */
    if (!pb_validate_context_push_field(&ctx, "payload")) return false;
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Error(const test_Error *msg, pb_violations_t *violations)
{
    /* Fields without constraints: details */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: error_code */
    if (!pb_validate_context_push_field(&ctx, "error_code")) return false;
    {
        /* Rule: uint32.gt */
        {
            uint32_t expected = (uint32_t)0;
            if (!pb_validate_uint32(msg->error_code, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: message */
    if (!pb_validate_context_push_field(&ctx, "message")) return false;
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
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Notification(const test_Notification *msg, pb_violations_t *violations)
{
    /* Fields without constraints: data, timestamp */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: event_type */
    if (!pb_validate_context_push_field(&ctx, "event_type")) return false;
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
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Envelope(const test_Envelope *msg, pb_violations_t *violations)
{
    /* Fields without constraints: correlation_id, message */
    
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: version */
    if (!pb_validate_context_push_field(&ctx, "version")) return false;
    {
        /* Rule: uint32.gte */
        {
            uint32_t expected = (uint32_t)1;
            if (!pb_validate_uint32(msg->version, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: uint32.lte */
        {
            uint32_t expected = (uint32_t)10;
            if (!pb_validate_uint32(msg->version, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: msg_type */
    if (!pb_validate_context_push_field(&ctx, "msg_type")) return false;
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
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

