/* Validation implementation for test_basic_validation.proto */
#include "test_basic_validation_validate.h"
#include <string.h>

bool pb_validate_test_BasicValidation(const test_BasicValidation *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: age */
    if (!pb_validate_context_push_field(&ctx, "age")) return false;
    {
        /* Rule: int32.gte */
        if (!(msg->age >= 0)) {
            pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Value must be greater than or equal to 0");
            if (ctx.early_exit) return false;
        }
    }
    {
        /* Rule: int32.lte */
        if (!(msg->age <= 150)) {
            pb_violations_add(violations, ctx.path_buffer, "int32.lte", "Value must be less than or equal to 150");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: score */
    if (!pb_validate_context_push_field(&ctx, "score")) return false;
    {
        /* Rule: int32.gt */
        if (!(msg->score > 0)) {
            pb_violations_add(violations, ctx.path_buffer, "int32.gt", "Value must be greater than 0");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: user_id */
    if (!pb_validate_context_push_field(&ctx, "user_id")) return false;
    {
        /* Rule: int64.gt */
        if (!(msg->user_id > 0)) {
            pb_violations_add(violations, ctx.path_buffer, "int64.gt", "Value must be greater than 0");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: timestamp */
    if (!pb_validate_context_push_field(&ctx, "timestamp")) return false;
    {
        /* Rule: int64.gte */
        if (!(msg->timestamp >= 0)) {
            pb_violations_add(violations, ctx.path_buffer, "int64.gte", "Value must be greater than or equal to 0");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: count */
    if (!pb_validate_context_push_field(&ctx, "count")) return false;
    {
        /* Rule: uint32.lte */
        if (!(msg->count <= 1000)) {
            pb_violations_add(violations, ctx.path_buffer, "uint32.lte", "Value must be less than or equal to 1000");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: size */
    if (!pb_validate_context_push_field(&ctx, "size")) return false;
    {
        /* Rule: uint32.gte */
        if (!(msg->size >= 10)) {
            pb_violations_add(violations, ctx.path_buffer, "uint32.gte", "Value must be greater than or equal to 10");
            if (ctx.early_exit) return false;
        }
    }
    {
        /* Rule: uint32.lte */
        if (!(msg->size <= 100)) {
            pb_violations_add(violations, ctx.path_buffer, "uint32.lte", "Value must be less than or equal to 100");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: total_bytes */
    if (!pb_validate_context_push_field(&ctx, "total_bytes")) return false;
    {
        /* Rule: uint64.lt */
        if (msg->has_total_bytes) {
                if (!(msg->total_bytes < 1000000000)) {
                    pb_violations_add(violations, ctx.path_buffer, "uint64.lt", "Value must be less than 1000000000");
                    if (ctx.early_exit) return false;
                }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: sequence_num */
    if (!pb_validate_context_push_field(&ctx, "sequence_num")) return false;
    {
        /* Rule: uint64.gte */
        if (!(msg->sequence_num >= 1)) {
            pb_violations_add(violations, ctx.path_buffer, "uint64.gte", "Value must be greater than or equal to 1");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: username */
    if (!pb_validate_context_push_field(&ctx, "username")) return false;
    {
        /* Rule: string.min_len */
        /* TODO: String MIN_LEN validation for callback field not supported yet */
    }
    {
        /* Rule: string.max_len */
        /* TODO: String MAX_LEN validation for callback field not supported yet */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: email */
    if (!pb_validate_context_push_field(&ctx, "email")) return false;
    {
        /* Rule: string.min_len */
        /* TODO: String MIN_LEN validation for callback field not supported yet */
    }
    {
        /* Rule: string.contains */
        /* TODO: String CONTAINS validation for callback field not supported yet */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: password */
    if (!pb_validate_context_push_field(&ctx, "password")) return false;
    {
        /* Rule: string.min_len */
        /* TODO: String MIN_LEN validation for callback field not supported yet */
    }
    {
        /* Rule: string.max_len */
        /* TODO: String MAX_LEN validation for callback field not supported yet */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: prefix_field */
    if (!pb_validate_context_push_field(&ctx, "prefix_field")) return false;
    {
        /* Rule: string.prefix */
        /* TODO: String PREFIX validation for callback field not supported yet */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: suffix_field */
    if (!pb_validate_context_push_field(&ctx, "suffix_field")) return false;
    {
        /* Rule: string.suffix */
        /* TODO: String SUFFIX validation for callback field not supported yet */
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

