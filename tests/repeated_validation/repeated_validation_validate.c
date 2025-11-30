/* Validation implementation for tests/repeated_validation/repeated_validation.proto */
#include "tests/repeated_validation/repeated_validation_validate.h"
#include <string.h>

bool pb_validate_test_RepeatedStringItems(const test_RepeatedStringItems *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedStringItems, msg, violations);

    /* Validate field: values */
    PB_VALIDATE_FIELD_BEGIN(ctx, "values");
    {
        /* Rule: repeated.items */
        /* Validate repeated.items: per-element validation */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            pb_validate_context_push_index(&ctx, __pb_i);
            {
                uint32_t __pb_min_len_v = 3;
                if (!pb_validate_string(msg->values[__pb_i], (pb_size_t)strlen(msg->values[__pb_i]), &__pb_min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            {
                uint32_t __pb_max_len_v = 10;
                if (!pb_validate_string(msg->values[__pb_i], (pb_size_t)strlen(msg->values[__pb_i]), &__pb_max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            pb_validate_context_pop_index(&ctx);
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_RepeatedInt32Items(const test_RepeatedInt32Items *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedInt32Items, msg, violations);

    /* Validate field: values */
    PB_VALIDATE_FIELD_BEGIN(ctx, "values");
    {
        /* Rule: repeated.items */
        /* Validate repeated.items: per-element validation */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            pb_validate_context_push_index(&ctx, __pb_i);
            {
                int32_t __pb_expected = (int32_t)100;
                if (!pb_validate_int32(msg->values[__pb_i], &__pb_expected, PB_VALIDATE_RULE_LT)) {
                    pb_violations_add(violations, ctx.path_buffer, "int32.lt", "Value constraint failed");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            {
                int32_t __pb_expected = (int32_t)0;
                if (!pb_validate_int32(msg->values[__pb_i], &__pb_expected, PB_VALIDATE_RULE_GT)) {
                    pb_violations_add(violations, ctx.path_buffer, "int32.gt", "Value constraint failed");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            pb_validate_context_pop_index(&ctx);
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_RepeatedUniqueStrings(const test_RepeatedUniqueStrings *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedUniqueStrings, msg, violations);

    /* Validate field: values */
    PB_VALIDATE_FIELD_BEGIN(ctx, "values");
    {
        /* Rule: repeated.unique */
        /* Validate repeated.unique: check for duplicate elements */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            for (pb_size_t __pb_j = __pb_i + 1; __pb_j < msg->values_count; ++__pb_j) {
                if (strcmp(msg->values[__pb_i], msg->values[__pb_j]) == 0) {
                    pb_violations_add(violations, ctx.path_buffer, "repeated.unique", "Repeated field elements must be unique");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_RepeatedUniqueInt32(const test_RepeatedUniqueInt32 *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedUniqueInt32, msg, violations);

    /* Validate field: values */
    PB_VALIDATE_FIELD_BEGIN(ctx, "values");
    {
        /* Rule: repeated.unique */
        /* Validate repeated.unique: check for duplicate elements */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            for (pb_size_t __pb_j = __pb_i + 1; __pb_j < msg->values_count; ++__pb_j) {
                if (msg->values[__pb_i] == msg->values[__pb_j]) {
                    pb_violations_add(violations, ctx.path_buffer, "repeated.unique", "Repeated field elements must be unique");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_RepeatedBothConstraints(const test_RepeatedBothConstraints *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedBothConstraints, msg, violations);

    /* Validate field: values */
    PB_VALIDATE_FIELD_BEGIN(ctx, "values");
    {
        /* Rule: repeated.unique */
        /* Validate repeated.unique: check for duplicate elements */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            for (pb_size_t __pb_j = __pb_i + 1; __pb_j < msg->values_count; ++__pb_j) {
                if (strcmp(msg->values[__pb_i], msg->values[__pb_j]) == 0) {
                    pb_violations_add(violations, ctx.path_buffer, "repeated.unique", "Repeated field elements must be unique");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: repeated.items */
        /* Validate repeated.items: per-element validation */
        for (pb_size_t __pb_i = 0; __pb_i < msg->values_count; ++__pb_i) {
            pb_validate_context_push_index(&ctx, __pb_i);
            {
                uint32_t __pb_min_len_v = 2;
                if (!pb_validate_string(msg->values[__pb_i], (pb_size_t)strlen(msg->values[__pb_i]), &__pb_min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            pb_validate_context_pop_index(&ctx);
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

bool pb_validate_test_RepeatedAllConstraints(const test_RepeatedAllConstraints *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_RepeatedAllConstraints, msg, violations);

    /* Validate field: numbers */
    PB_VALIDATE_FIELD_BEGIN(ctx, "numbers");
    {
        /* Rule: repeated.min_items */
        PB_VALIDATE_MIN_ITEMS(ctx, msg, numbers, 1, "repeated.min_items");
    }
    {
        /* Rule: repeated.max_items */
        PB_VALIDATE_MAX_ITEMS(ctx, msg, numbers, 10, "repeated.max_items");
    }
    {
        /* Rule: repeated.unique */
        /* Validate repeated.unique: check for duplicate elements */
        for (pb_size_t __pb_i = 0; __pb_i < msg->numbers_count; ++__pb_i) {
            for (pb_size_t __pb_j = __pb_i + 1; __pb_j < msg->numbers_count; ++__pb_j) {
                if (msg->numbers[__pb_i] == msg->numbers[__pb_j]) {
                    pb_violations_add(violations, ctx.path_buffer, "repeated.unique", "Repeated field elements must be unique");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: repeated.items */
        /* Validate repeated.items: per-element validation */
        for (pb_size_t __pb_i = 0; __pb_i < msg->numbers_count; ++__pb_i) {
            pb_validate_context_push_index(&ctx, __pb_i);
            {
                int32_t __pb_expected = (int32_t)1000;
                if (!pb_validate_int32(msg->numbers[__pb_i], &__pb_expected, PB_VALIDATE_RULE_LTE)) {
                    pb_violations_add(violations, ctx.path_buffer, "int32.lte", "Value constraint failed");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            {
                int32_t __pb_expected = (int32_t)0;
                if (!pb_validate_int32(msg->numbers[__pb_i], &__pb_expected, PB_VALIDATE_RULE_GTE)) {
                    pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Value constraint failed");
                    if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }
                }
            }
            pb_validate_context_pop_index(&ctx);
        }
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}

