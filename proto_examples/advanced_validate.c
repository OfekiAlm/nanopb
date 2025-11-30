/* Validation implementation for proto_examples/advanced.proto */
#include "proto_examples/advanced_validate.h"
#include <string.h>

bool pb_validate_test_AdvancedMessage(const test_AdvancedMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - test_oneof
    */

    PB_VALIDATE_BEGIN(ctx, test_AdvancedMessage, msg, violations);

    /* Validate field: email */
    PB_VALIDATE_FIELD_BEGIN(ctx, "email");
    {
        /* Rule: string.email */
        PB_VALIDATE_STRING_EMAIL(ctx, msg, email, "string.email");
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - advanced_message
    */

    PB_VALIDATE_BEGIN(ctx, test_SimpleMessage, msg, violations);

    /* Validate field: bounded_float */
    PB_VALIDATE_FIELD_BEGIN(ctx, "bounded_float");
    {
        /* Rule: float.lte */
        PB_VALIDATE_NUMERIC_GENERIC(ctx, msg, bounded_float, float, pb_validate_float, PB_VALIDATE_RULE_LTE, 100.0, "float.lte");
    }
    {
        /* Rule: float.gte */
        PB_VALIDATE_NUMERIC_GENERIC(ctx, msg, bounded_float, float, pb_validate_float, PB_VALIDATE_RULE_GTE, 0.0, "float.gte");
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    /* Validate field: optional_string */
    PB_VALIDATE_FIELD_BEGIN(ctx, "optional_string");
    {
        /* Rule: string.min_len */
        PB_VALIDATE_STRING_MIN_LEN(ctx, msg, optional_string, 5, "string.min_len");
    }
    {
        /* Rule: string.max_len */
        PB_VALIDATE_STRING_MAX_LEN(ctx, msg, optional_string, 20, "string.max_len");
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    /* Validate field: advanced_message */
    PB_VALIDATE_FIELD_BEGIN(ctx, "advanced_message");
    {
        /* Recurse into submessage (optional) */
        if (msg->has_advanced_message) {
            bool ok_nested = pb_validate_test_AdvancedMessage(&msg->advanced_message, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}
