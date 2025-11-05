/* Validation implementation for demo_multi/structures/geometry.proto */
#include "demo_multi/structures/geometry_validate.h"
#include <string.h>

bool pb_validate_demo_structures_Point(const demo_structures_Point *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: x */
    if (!pb_validate_context_push_field(&ctx, "x")) return false;
    {
        /* Rule: float.gte */
        {
            float expected = (float)-1000.0;
            if (!pb_validate_float(msg->x, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: float.lte */
        {
            float expected = (float)1000.0;
            if (!pb_validate_float(msg->x, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: y */
    if (!pb_validate_context_push_field(&ctx, "y")) return false;
    {
        /* Rule: float.gte */
        {
            float expected = (float)-1000.0;
            if (!pb_validate_float(msg->y, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: float.lte */
        {
            float expected = (float)1000.0;
            if (!pb_validate_float(msg->y, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_demo_structures_ColoredPoint(const demo_structures_ColoredPoint *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: color */
    if (!pb_validate_context_push_field(&ctx, "color")) return false;
    {
        /* Rule: enum.defined_only */
        /* TODO: Implement enum defined_only validation */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: p */
    if (!pb_validate_context_push_field(&ctx, "p")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_p) {
            bool ok_nested = pb_validate_demo_structures_Point(&msg->p, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

