/* pb_validate.c: Runtime support implementation for nanopb validation */

#include "pb_validate.h"
#include <string.h>

/* Initialize a violations structure */
void pb_violations_init(pb_violations_t *violations)
{
    if (violations)
    {
        violations->count = 0;
        violations->truncated = false;
        memset(violations->violations, 0, sizeof(violations->violations));
    }
}

/* Add a violation to the collection */
bool pb_violations_add(pb_violations_t *violations,
                      const char *field_path,
                      const char *constraint_id,
                      const char *message)
{
    if (!violations)
        return false;
    
    if (violations->count >= PB_VALIDATE_MAX_VIOLATIONS)
    {
        violations->truncated = true;
        return false;
    }
    
    pb_violation_t *v = &violations->violations[violations->count];
    v->field_path = field_path;
    v->constraint_id = constraint_id;
    v->message = message;
    violations->count++;
    
    return true;
}

/* Push a field name to the path */
bool pb_validate_context_push_field(pb_validate_context_t *ctx, const char *field_name)
{
    size_t name_len = strlen(field_name);
    size_t needed = name_len + (ctx->path_length > 0 ? 1 : 0); /* +1 for dot separator */
    
    if (ctx->path_length + needed >= PB_VALIDATE_MAX_PATH_LENGTH)
        return false;
    
    if (ctx->path_length > 0)
    {
        ctx->path_buffer[ctx->path_length++] = '.';
    }
    
    memcpy(&ctx->path_buffer[ctx->path_length], field_name, name_len);
    ctx->path_length += name_len;
    ctx->path_buffer[ctx->path_length] = '\0';
    
    return true;
}

/* Pop a field name from the path */
void pb_validate_context_pop_field(pb_validate_context_t *ctx)
{
    /* Find the last dot and truncate there */
    char *last_dot = strrchr(ctx->path_buffer, '.');
    if (last_dot)
    {
        *last_dot = '\0';
        ctx->path_length = last_dot - ctx->path_buffer;
    }
    else
    {
        /* No dot found, clear the entire path */
        ctx->path_buffer[0] = '\0';
        ctx->path_length = 0;
    }
}

/* Push an array index to the path */
bool pb_validate_context_push_index(pb_validate_context_t *ctx, pb_size_t index)
{
    char index_str[16];
    int len = snprintf(index_str, sizeof(index_str), "[%u]", (unsigned int)index);
    
    if (len < 0 || (size_t)len >= sizeof(index_str))
        return false;
    
    if (ctx->path_length + len >= PB_VALIDATE_MAX_PATH_LENGTH)
        return false;
    
    memcpy(&ctx->path_buffer[ctx->path_length], index_str, len);
    ctx->path_length += len;
    ctx->path_buffer[ctx->path_length] = '\0';
    
    return true;
}

/* Pop an array index from the path */
void pb_validate_context_pop_index(pb_validate_context_t *ctx)
{
    /* Find the last '[' and truncate there */
    char *last_bracket = strrchr(ctx->path_buffer, '[');
    if (last_bracket)
    {
        *last_bracket = '\0';
        ctx->path_length = last_bracket - ctx->path_buffer;
    }
}

/* Helper function to check if a value is in a list */
static bool value_in_list(const void *value, const void *list, pb_size_t count, size_t elem_size)
{
    const uint8_t *list_bytes = (const uint8_t *)list;
    for (pb_size_t i = 0; i < count; i++)
    {
        if (memcmp(value, list_bytes + i * elem_size, elem_size) == 0)
            return true;
    }
    return false;
}

/* Validation functions for int32 */
bool pb_validate_int32(int32_t value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const int32_t *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const int32_t *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const int32_t *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const int32_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const int32_t *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const int32_t *values; pb_size_t count; } *in_data = rule_data;
            return value_in_list(&value, in_data->values, in_data->count, sizeof(int32_t));
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const int32_t *values; pb_size_t count; } *not_in_data = rule_data;
            return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(int32_t));
        }
        default:
            return true;
    }
}

/* Validation functions for int64 */
bool pb_validate_int64(int64_t value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const int64_t *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const int64_t *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const int64_t *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const int64_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const int64_t *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const int64_t *values; pb_size_t count; } *in_data = rule_data;
            return value_in_list(&value, in_data->values, in_data->count, sizeof(int64_t));
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const int64_t *values; pb_size_t count; } *not_in_data = rule_data;
            return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(int64_t));
        }
        default:
            return true;
    }
}

/* Validation functions for uint32 */
bool pb_validate_uint32(uint32_t value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const uint32_t *values; pb_size_t count; } *in_data = rule_data;
            return value_in_list(&value, in_data->values, in_data->count, sizeof(uint32_t));
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const uint32_t *values; pb_size_t count; } *not_in_data = rule_data;
            return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(uint32_t));
        }
        default:
            return true;
    }
}

/* Validation functions for uint64 */
bool pb_validate_uint64(uint64_t value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const uint64_t *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const uint64_t *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const uint64_t *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const uint64_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const uint64_t *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const uint64_t *values; pb_size_t count; } *in_data = rule_data;
            return value_in_list(&value, in_data->values, in_data->count, sizeof(uint64_t));
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const uint64_t *values; pb_size_t count; } *not_in_data = rule_data;
            return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(uint64_t));
        }
        default:
            return true;
    }
}

/* Validation functions for float */
bool pb_validate_float(float value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const float *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const float *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const float *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const float *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const float *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const float *values; pb_size_t count; } *in_data = rule_data;
            for (pb_size_t i = 0; i < in_data->count; i++)
            {
                if (value == in_data->values[i])
                    return true;
            }
            return false;
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const float *values; pb_size_t count; } *not_in_data = rule_data;
            for (pb_size_t i = 0; i < not_in_data->count; i++)
            {
                if (value == not_in_data->values[i])
                    return false;
            }
            return true;
        }
        default:
            return true;
    }
}

/* Validation functions for double */
bool pb_validate_double(double value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_LT:
            return value < *(const double *)rule_data;
        case PB_VALIDATE_RULE_LTE:
            return value <= *(const double *)rule_data;
        case PB_VALIDATE_RULE_GT:
            return value > *(const double *)rule_data;
        case PB_VALIDATE_RULE_GTE:
            return value >= *(const double *)rule_data;
        case PB_VALIDATE_RULE_EQ:
            return value == *(const double *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const double *values; pb_size_t count; } *in_data = rule_data;
            for (pb_size_t i = 0; i < in_data->count; i++)
            {
                if (value == in_data->values[i])
                    return true;
            }
            return false;
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const double *values; pb_size_t count; } *not_in_data = rule_data;
            for (pb_size_t i = 0; i < not_in_data->count; i++)
            {
                if (value == not_in_data->values[i])
                    return false;
            }
            return true;
        }
        default:
            return true;
    }
}

/* Validation function for bool */
bool pb_validate_bool(bool value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_EQ:
            return value == *(const bool *)rule_data;
        default:
            return true;
    }
}

/* Validation function for strings */
bool pb_validate_string(const char *value, pb_size_t length, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    if (!value)
        return rule_type != PB_VALIDATE_RULE_REQUIRED;
    
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_MIN_LEN:
            return length >= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_MAX_LEN:
            return length <= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
        {
            const char *expected = (const char *)rule_data;
            return strcmp(value, expected) == 0;
        }
        case PB_VALIDATE_RULE_PREFIX:
        {
            const char *prefix = (const char *)rule_data;
            size_t prefix_len = strlen(prefix);
            return length >= prefix_len && strncmp(value, prefix, prefix_len) == 0;
        }
        case PB_VALIDATE_RULE_SUFFIX:
        {
            const char *suffix = (const char *)rule_data;
            size_t suffix_len = strlen(suffix);
            return length >= suffix_len && strcmp(value + length - suffix_len, suffix) == 0;
        }
        case PB_VALIDATE_RULE_CONTAINS:
        {
            const char *substring = (const char *)rule_data;
            return strstr(value, substring) != NULL;
        }
        case PB_VALIDATE_RULE_ASCII:
        {
            for (pb_size_t i = 0; i < length; i++)
            {
                if ((unsigned char)value[i] > 127)
                    return false;
            }
            return true;
        }
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const char * const *values; pb_size_t count; } *in_data = rule_data;
            for (pb_size_t i = 0; i < in_data->count; i++)
            {
                if (strcmp(value, in_data->values[i]) == 0)
                    return true;
            }
            return false;
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const char * const *values; pb_size_t count; } *not_in_data = rule_data;
            for (pb_size_t i = 0; i < not_in_data->count; i++)
            {
                if (strcmp(value, not_in_data->values[i]) == 0)
                    return false;
            }
            return true;
        }
        default:
            return true;
    }
}

/* Validation function for bytes */
bool pb_validate_bytes(const pb_bytes_array_t *value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    if (!value)
        return rule_type != PB_VALIDATE_RULE_REQUIRED;
    
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_MIN_LEN:
            return value->size >= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_MAX_LEN:
            return value->size <= *(const uint32_t *)rule_data;
        case PB_VALIDATE_RULE_EQ:
        {
            const pb_bytes_array_t *expected = (const pb_bytes_array_t *)rule_data;
            return value->size == expected->size && 
                   memcmp(value->bytes, expected->bytes, value->size) == 0;
        }
        case PB_VALIDATE_RULE_PREFIX:
        {
            const pb_bytes_array_t *prefix = (const pb_bytes_array_t *)rule_data;
            return value->size >= prefix->size && 
                   memcmp(value->bytes, prefix->bytes, prefix->size) == 0;
        }
        case PB_VALIDATE_RULE_SUFFIX:
        {
            const pb_bytes_array_t *suffix = (const pb_bytes_array_t *)rule_data;
            return value->size >= suffix->size && 
                   memcmp(value->bytes + value->size - suffix->size, suffix->bytes, suffix->size) == 0;
        }
        case PB_VALIDATE_RULE_CONTAINS:
        {
            const pb_bytes_array_t *pattern = (const pb_bytes_array_t *)rule_data;
            if (pattern->size > value->size)
                return false;
            
            for (pb_size_t i = 0; i <= value->size - pattern->size; i++)
            {
                if (memcmp(value->bytes + i, pattern->bytes, pattern->size) == 0)
                    return true;
            }
            return false;
        }
        default:
            return true;
    }
}

/* Validation function for enums */
bool pb_validate_enum(int value, const void *rule_data, pb_validate_rule_type_t rule_type)
{
    switch (rule_type)
    {
        case PB_VALIDATE_RULE_EQ:
            return value == *(const int *)rule_data;
        case PB_VALIDATE_RULE_IN:
        {
            const struct { const int *values; pb_size_t count; } *in_data = rule_data;
            return value_in_list(&value, in_data->values, in_data->count, sizeof(int));
        }
        case PB_VALIDATE_RULE_NOT_IN:
        {
            const struct { const int *values; pb_size_t count; } *not_in_data = rule_data;
            return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(int));
        }
        case PB_VALIDATE_RULE_ENUM_DEFINED:
        {
            const struct { const int *values; pb_size_t count; } *defined_values = rule_data;
            return value_in_list(&value, defined_values->values, defined_values->count, sizeof(int));
        }
        default:
            return true;
    }
}
