/* Validation implementation for validation_test.proto */
#include "validation_test_validate.h"
#include <string.h>

bool pb_validate_Person(const Person *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: name */
    if (!pb_validate_context_push_field(&ctx, "name")) return false;
    {
        /* Rule: string.min_len */
        {
            uint32_t min_len_v = 1;
            if (!pb_validate_string(msg->name, (pb_size_t)strlen(msg->name), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            uint32_t max_len_v = 50;
            if (!pb_validate_string(msg->name, (pb_size_t)strlen(msg->name), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: required */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: email */
    if (!pb_validate_context_push_field(&ctx, "email")) return false;
    {
        /* Rule: string.min_len */
        {
            uint32_t min_len_v = 3;
            if (!pb_validate_string(msg->email, (pb_size_t)strlen(msg->email), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            uint32_t max_len_v = 100;
            if (!pb_validate_string(msg->email, (pb_size_t)strlen(msg->email), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.contains */
        {
            const char *needle = "@";
            if (!pb_validate_string(msg->email, (pb_size_t)strlen(msg->email), needle, PB_VALIDATE_RULE_CONTAINS)) {
                pb_violations_add(violations, ctx.path_buffer, "string.contains", "String must contain '@'");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: age */
    if (!pb_validate_context_push_field(&ctx, "age")) return false;
    {
        /* Rule: int32.lte */
        {
            int32_t expected = (int32_t)150;
            if (!pb_validate_int32(msg->age, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: int32.gte */
        {
            int32_t expected = (int32_t)0;
            if (!pb_validate_int32(msg->age, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: gender */
    if (!pb_validate_context_push_field(&ctx, "gender")) return false;
    {
        /* Rule: enum.defined_only */
        {
            static const int __pb_defined_vals[] = { 0, 1, 2, 3 };
            if (!pb_validate_enum_defined_only((int)msg->gender, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {
                pb_violations_add(violations, ctx.path_buffer, "enum.defined_only", "Value must be a defined enum value");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: tags */
    if (!pb_validate_context_push_field(&ctx, "tags")) return false;
    {
        /* Rule: repeated.min_items */
        if (!pb_validate_min_items(msg->tags_count, 0)) {
            pb_violations_add(violations, ctx.path_buffer, "repeated.min_items", "Too few items");
            if (ctx.early_exit) return false;
        }
    }
    {
        /* Rule: repeated.max_items */
        if (!pb_validate_max_items(msg->tags_count, 10)) {
            pb_violations_add(violations, ctx.path_buffer, "repeated.max_items", "Too many items");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: phone */
    if (!pb_validate_context_push_field(&ctx, "phone")) return false;
    {
        /* Rule: string.min_len */
        if (msg->has_phone) {
            {
                uint32_t min_len_v = 10;
                if (!pb_validate_string(msg->phone, (pb_size_t)strlen(msg->phone), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_Company(const Company *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: name */
    if (!pb_validate_context_push_field(&ctx, "name")) return false;
    {
        /* Rule: required */
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

