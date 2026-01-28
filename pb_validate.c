/* pb_validate.c: Runtime support implementation for nanopb validation */

#include "pb_validate.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdio.h>

static bool is_valid_email(const char *s, size_t len);
static bool is_valid_hostname(const char *s, size_t len);
static bool is_valid_ipv4(const char *s, size_t len);
static bool is_valid_ipv6(const char *s, size_t len);

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
        const struct
        {
            const int32_t *values;
            pb_size_t count;
        } *in_data = rule_data;
        return value_in_list(&value, in_data->values, in_data->count, sizeof(int32_t));
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const int32_t *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const int64_t *values;
            pb_size_t count;
        } *in_data = rule_data;
        return value_in_list(&value, in_data->values, in_data->count, sizeof(int64_t));
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const int64_t *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const uint32_t *values;
            pb_size_t count;
        } *in_data = rule_data;
        return value_in_list(&value, in_data->values, in_data->count, sizeof(uint32_t));
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const uint32_t *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const uint64_t *values;
            pb_size_t count;
        } *in_data = rule_data;
        return value_in_list(&value, in_data->values, in_data->count, sizeof(uint64_t));
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const uint64_t *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const float *values;
            pb_size_t count;
        } *in_data = rule_data;
        for (pb_size_t i = 0; i < in_data->count; i++)
        {
            if (value == in_data->values[i])
                return true;
        }
        return false;
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const float *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const double *values;
            pb_size_t count;
        } *in_data = rule_data;
        for (pb_size_t i = 0; i < in_data->count; i++)
        {
            if (value == in_data->values[i])
                return true;
        }
        return false;
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const double *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
    case PB_VALIDATE_RULE_EMAIL:
        return is_valid_email(value, length);
    case PB_VALIDATE_RULE_HOSTNAME:
        return is_valid_hostname(value, length);
    case PB_VALIDATE_RULE_IPV4:
        return is_valid_ipv4(value, length);
    case PB_VALIDATE_RULE_IPV6:
        return is_valid_ipv6(value, length);
    case PB_VALIDATE_RULE_IP:
        return is_valid_ipv4(value, length) || is_valid_ipv6(value, length);
    case PB_VALIDATE_RULE_IN:
    {
        const struct
        {
            const char *const *values;
            pb_size_t count;
        } *in_data = rule_data;
        for (pb_size_t i = 0; i < in_data->count; i++)
        {
            if (strcmp(value, in_data->values[i]) == 0)
                return true;
        }
        return false;
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const char *const *values;
            pb_size_t count;
        } *not_in_data = rule_data;
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
        const struct
        {
            const int *values;
            pb_size_t count;
        } *in_data = rule_data;
        return value_in_list(&value, in_data->values, in_data->count, sizeof(int));
    }
    case PB_VALIDATE_RULE_NOT_IN:
    {
        const struct
        {
            const int *values;
            pb_size_t count;
        } *not_in_data = rule_data;
        return !value_in_list(&value, not_in_data->values, not_in_data->count, sizeof(int));
    }
    case PB_VALIDATE_RULE_ENUM_DEFINED:
    {
        const struct
        {
            const int *values;
            pb_size_t count;
        } *defined_values = rule_data;
        return value_in_list(&value, defined_values->values, defined_values->count, sizeof(int));
    }
    default:
        return true;
    }
}

/* Public helper for enum defined_only rule */
bool pb_validate_enum_defined_only(int value, const int *values, pb_size_t count)
{
    if (!values || count == 0)
        return true; /* If no list provided, treat as valid to avoid false negatives */
    return value_in_list(&value, values, count, sizeof(int));
}

/* Helper function for validating string length during decode callbacks.
 * Used when the full string data is not stored (callback fields).
 * min_len: minimum allowed length (0 = no minimum)
 * max_len: maximum allowed length (0 = no maximum)
 * Returns true if length is within bounds, false otherwise.
 */
bool pb_validate_string_length(pb_size_t length, pb_size_t min_len, pb_size_t max_len)
{
    if (min_len > 0 && length < min_len)
        return false;
    if (max_len > 0 && length > max_len)
        return false;
    return true;
}

/* Helper function for validating bytes length during decode callbacks.
 * Used when the full bytes data is not stored (callback fields).
 * min_len: minimum allowed length (0 = no minimum)
 * max_len: maximum allowed length (0 = no maximum)
 * Returns true if length is within bounds, false otherwise.
 */
bool pb_validate_bytes_length(pb_size_t length, pb_size_t min_len, pb_size_t max_len)
{
    if (min_len > 0 && length < min_len)
        return false;
    if (max_len > 0 && length > max_len)
        return false;
    return true;
}


/* Helper: read NUL-terminated string pointer from pb_callback_t.
 * Assumptions:
 * - For decoded messages, user provided a decode callback that stores pointer
 *   to buffer in callback->arg OR uses default callback where arg is the buffer.
 * - We do not invoke the function pointer; this is a lightweight accessor.
 * - If pointer unavailable, we return false.
 */
bool pb_read_callback_string(const struct pb_callback_s *callback, const char **out_str, pb_size_t *out_len)
{
    if (!callback || !out_str || !out_len)
        return false;
    /* pb_callback_t defined in pb.h as struct pb_callback_s { bool (*func)(pb_istream_t*, pb_ostream_t*, const pb_field_t*, void**); void *arg; } (typical).
       We rely only on arg. */
    const char *p = (const char *)callback->arg;
    if (!p)
        return false;
    /* Ensure NUL-termination within reasonable bounds: scan up to PB_VALIDATE_MAX_MESSAGE_LENGTH */
    size_t len = 0;
    while (p[len] != '\0')
    {
        if (len >= PB_VALIDATE_MAX_MESSAGE_LENGTH)
        {
            /* Treat overly long as failure */
            return false;
        }
        len++;
    }
    *out_str = p;
    *out_len = (pb_size_t)len;
    return true;
}

/* --- Additional string constraint helpers --------------------------------- */

static bool is_valid_hostname_label(const char *s, size_t len)
{
    if (len == 0 || len > 63)
        return false;
    /* First and last cannot be '-' */
    if (s[0] == '-' || s[len - 1] == '-')
        return false;
    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)s[i];
        if (c > 127)
            return false; /* ASCII only */
        if (!(isalnum(c) || c == '-'))
            return false;
    }
    return true;
}

static bool is_valid_hostname(const char *s, size_t len)
{
    if (!s || len == 0 || len > 253)
        return false;
    /* Cannot start or end with '.' and no consecutive dots */
    if (s[0] == '.' || s[len - 1] == '.')
        return false;
    size_t start = 0;
    for (size_t i = 0; i <= len; i++)
    {
        if (i == len || s[i] == '.')
        {
            if (i == start) /* empty label => consecutive dots */
                return false;
            if (!is_valid_hostname_label(&s[start], i - start))
                return false;
            start = i + 1;
        }
        else if ((unsigned char)s[i] <= 32 || (unsigned char)s[i] == 127)
        {
            return false; /* no spaces or control chars */
        }
    }
    return true;
}

static bool is_valid_email(const char *s, size_t len)
{
    if (!s || len < 3) /* a@b minimal */
        return false;
    /* Basic checks: single '@', non-empty local & domain, no spaces */
    size_t at_pos = (size_t)-1;
    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)s[i];
        if (c <= 32 || c == 127)
            return false; /* space/control not allowed */
        if (s[i] == '@')
        {
            if (at_pos != (size_t)-1)
                return false; /* multiple @ */
            at_pos = i;
        }
    }
    if (at_pos == (size_t)-1 || at_pos == 0 || at_pos == len - 1)
        return false;
    /* Local part basic validity: cannot start/end with '.', no consecutive '..' */
    size_t lstart = 0, lend = at_pos;
    if (s[lstart] == '.' || s[lend - 1] == '.')
        return false;
    for (size_t i = lstart + 1; i < lend; i++)
    {
        if (s[i] == '.' && s[i - 1] == '.')
            return false;
    }
    /* Domain part must be a valid hostname */
    return is_valid_hostname(&s[at_pos + 1], len - at_pos - 1);
}

static bool parse_uint_dotted_segment(const char *s, size_t len, int *out)
{
    if (len == 0)
        return false;
    int value = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (!isdigit((unsigned char)s[i]))
            return false;
        value = value * 10 + (s[i] - '0');
        if (value > 255)
            return false;
    }
    *out = value;
    return true;
}

static bool is_valid_ipv4(const char *s, size_t len)
{
    if (!s || len < 7 || len > 15)
        return false;
    size_t start = 0;
    int parts = 0;
    for (size_t i = 0; i <= len; i++)
    {
        if (i == len || s[i] == '.')
        {
            if (parts > 3)
                return false;
            if (i == start)
                return false; /* empty */
            int v;
            if (!parse_uint_dotted_segment(&s[start], i - start, &v))
                return false;
            parts++;
            start = i + 1;
        }
        else if (!isdigit((unsigned char)s[i]))
        {
            return false;
        }
    }
    return parts == 4;
}

static bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool is_valid_ipv6(const char *s, size_t len)
{
    if (!s || len < 2)
        return false;
    size_t i = 0, start = 0;
    int hextets = 0;
    bool compress = false;

    if (s[i] == ':')
    {
        if (i + 1 >= len || s[i + 1] != ':')
            return false;
        i += 2;
        start = i;
        compress = true; /* leading :: */
        if (i == len)
            return true; /* "::" only (all zeros) */
    }

    while (i <= len)
    {
        if (i == len || s[i] == ':')
        {
            if (i == start)
            {
                /* empty, must be '::' */
                if (compress)
                    return false; /* second '::' not allowed */
                compress = true;
                i++;
                start = i; /* skip second ':' */
                continue;
            }

            /* Check if this segment is IPv4 tail */
            bool has_dot = false;
            for (size_t k = start; k < i; k++)
            {
                if (s[k] == '.')
                {
                    has_dot = true;
                    break;
                }
            }
            if (has_dot)
            {
                if (!is_valid_ipv4(&s[start], i - start))
                    return false;
                hextets += 2; /* IPv4 part equals two hextets */
                start = i + 1;
                break; /* IPv4 must be the last part */
            }

            size_t seglen = i - start;
            if (seglen == 0 || seglen > 4)
                return false;
            for (size_t k = start; k < i; k++)
            {
                if (!is_hex_digit(s[k]))
                    return false;
            }
            hextets++;
            start = i + 1;
        }
        i++;
    }

    if (start <= len)
    {
        /* If there is trailing part after last ':' already processed above */
    }

    if (compress)
        return hextets < 8; /* at least one compressed */
    return hextets == 8;
}
