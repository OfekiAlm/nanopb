/* Validation implementation for proto_examples/advanced.proto */
#include "proto_examples/advanced_validate.h"
#include <string.h>

bool pb_validate_test_AdvancedMessage(const test_AdvancedMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - test_oneof
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: email */
    if (!pb_validate_context_push_field(&ctx, "email")) return false;
    {
        /* Rule: string.email */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->email, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_EMAIL)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.email", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - advanced_message
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: bounded_float */
    if (!pb_validate_context_push_field(&ctx, "bounded_float")) return false;
    {
        /* Rule: float.lte */
        {
            float expected = (float)100.0;
            if (!pb_validate_float(msg->bounded_float, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: float.gte */
        {
            float expected = (float)0.0;
            if (!pb_validate_float(msg->bounded_float, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: optional_string */
    if (!pb_validate_context_push_field(&ctx, "optional_string")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->optional_string, &s, &l)) {
                uint32_t min_len = 5;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->optional_string, &s, &l)) {
                uint32_t max_len_v = 20;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: advanced_message */
    if (!pb_validate_context_push_field(&ctx, "advanced_message")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_advanced_message) {
            bool ok_nested = pb_validate_test_AdvancedMessage(&msg->advanced_message, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

