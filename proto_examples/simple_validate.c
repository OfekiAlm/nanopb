/* Validation implementation for proto_examples/simple.proto */
#include "proto_examples/simple_validate.h"
#include <string.h>

bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations)
{
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
    
    return !pb_violations_has_any(violations);
}

