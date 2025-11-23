/* Validation implementation for header_example.proto */
#include "header_example_validate.h"
#include <string.h>

bool pb_validate_my_pkg_Envelope(const my_pkg_Envelope *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - payload
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: version */
    if (!pb_validate_context_push_field(&ctx, "version")) return false;
    {
        /* Rule: int32.gt */
        {
            int32_t expected = (int32_t)0;
            if (!pb_validate_int32(msg->version, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: int32.lt */
        {
            int32_t expected = (int32_t)100;
            if (!pb_validate_int32(msg->version, &expected, PB_VALIDATE_RULE_LT)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.lt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: opcode */
    if (!pb_validate_context_push_field(&ctx, "opcode")) return false;
    {
        /* Rule: enum.defined_only */
        {
            static const int __pb_defined_vals[] = { 0, 1, 2, 3 };
            if (!pb_validate_enum_defined_only((int)msg->opcode, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {
                pb_violations_add(violations, ctx.path_buffer, "enum.defined_only", "Value must be a defined enum value");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

