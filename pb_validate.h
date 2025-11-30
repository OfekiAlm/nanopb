/* pb_validate.h: Runtime support for nanopb validation
 *
 * This file provides the public API for validating protobuf messages
 * against declarative constraints defined using custom options.
 */

#ifndef PB_VALIDATE_H_INCLUDED
#define PB_VALIDATE_H_INCLUDED

#include "pb.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Configuration options */
#ifndef PB_VALIDATE_MAX_VIOLATIONS
#define PB_VALIDATE_MAX_VIOLATIONS 16
#endif

#ifndef PB_VALIDATE_EARLY_EXIT
#define PB_VALIDATE_EARLY_EXIT 1
#endif

#ifndef PB_VALIDATE_MAX_PATH_LENGTH
#define PB_VALIDATE_MAX_PATH_LENGTH 128
#endif

#ifndef PB_VALIDATE_MAX_MESSAGE_LENGTH
#define PB_VALIDATE_MAX_MESSAGE_LENGTH 256
#endif

    /* Violation structure representing a single validation error */
    typedef struct pb_violation_s
    {
        const char *field_path;    /* Dotted path to the field, e.g., "user.email" */
        const char *constraint_id; /* Constraint identifier, e.g., "string.max_len" */
        const char *message;       /* Human-readable error message */
    } pb_violation_t;

    /* Violations collection structure */
    typedef struct pb_violations_s
    {
        pb_violation_t violations[PB_VALIDATE_MAX_VIOLATIONS];
        pb_size_t count; /* Number of violations recorded */
        bool truncated;  /* True if more violations were found than could be stored */
    } pb_violations_t;

    /* Initialize a violations structure */
    void pb_violations_init(pb_violations_t *violations);

    /* Add a violation to the collection */
    bool pb_violations_add(pb_violations_t *violations,
                           const char *field_path,
                           const char *constraint_id,
                           const char *message);

    /* Get the number of violations */
    static inline pb_size_t pb_violations_count(const pb_violations_t *violations)
    {
        return violations ? violations->count : 0;
    }

    /* Check if there are any violations */
    static inline bool pb_violations_has_any(const pb_violations_t *violations)
    {
        return pb_violations_count(violations) > 0;
    }

    /* Validation context structure (internal use) */
    typedef struct pb_validate_context_s
    {
        pb_violations_t *violations;
        char path_buffer[PB_VALIDATE_MAX_PATH_LENGTH];
        pb_size_t path_length;
        bool early_exit;
    } pb_validate_context_t;

        /* Convenience macros for generated validators
         *
         * These allow the code generator to emit compact, macro-based
         * validation code instead of repeating the same boilerplate in
         * every generated *_validate.c file, similar in spirit to the
         * PB_BIND macros used for field descriptors.
         */

    /* Begin / end of a validate function body.
     * Usage:
     *   bool pb_validate_Foo(const Foo *msg, pb_violations_t *violations)
     *   {
     *       PB_VALIDATE_BEGIN(ctx, Foo, msg, violations);
     *       ... field checks ...
     *       PB_VALIDATE_END(ctx, violations);
     *   }
     */
    #define PB_VALIDATE_BEGIN(ctx_var, MsgType, msg_ptr, violations_ptr) \
        pb_validate_context_t ctx_var = {0};                             \
        ctx_var.violations = (violations_ptr);                           \
        ctx_var.early_exit = PB_VALIDATE_EARLY_EXIT;                     \
        if (!(msg_ptr)) return false

    /* Bypass mode: collect all violations without early exit.
     * Usage (with --bypass flag):
     *   bool pb_validate_Foo(const Foo *msg, pb_violations_t *violations)
     *   {
     *       PB_VALIDATE_BEGIN_BYPASS(ctx, Foo, msg, violations);
     *       ... field checks (will collect all violations) ...
     *       PB_VALIDATE_END(ctx, violations);
     *   }
     */
    #define PB_VALIDATE_BEGIN_BYPASS(ctx_var, MsgType, msg_ptr, violations_ptr) \
        pb_validate_context_t ctx_var = {0};                                    \
        ctx_var.violations = (violations_ptr);                                  \
        ctx_var.early_exit = false;                                             \
        if (!(msg_ptr)) return false

    #define PB_VALIDATE_END(ctx_var, violations_ptr) \
        (void)(ctx_var);                                \
        return !pb_violations_has_any(violations_ptr)

    /* Per-field context helpers. These wrap push/pop of the field name
     * into the validation context path buffer.
     */
    #define PB_VALIDATE_FIELD_BEGIN(ctx_var, field_name_str)                 \
        if (!pb_validate_context_push_field(&(ctx_var), (field_name_str)))   \
            return false

    #define PB_VALIDATE_FIELD_END(ctx_var) \
        pb_validate_context_pop_field(&(ctx_var))

    /* Helper for guarding checks behind has_XXX flag for OPTIONAL fields.
     * The generator expands FIELD_RULES_ENUM to the field.rules symbol when
     * it knows the field is optional, so that checks are skipped when the
     * field is unset.
     */
    #define PB_VALIDATE_IF_OPTIONAL(has_flag_expr, CODE) do { \
            if (has_flag_expr) {                              \
                CODE                                          \
            }                                                 \
        } while (0)

    /* Generic numeric comparison helper. The generator supplies the
     * concrete C type, validator function and rule enum.
     */
    #define PB_VALIDATE_NUMERIC_GENERIC(ctx_var, msg_ptr, field_name, CTYPE, FUNC, RULE_ENUM, VALUE_EXPR, CONSTRAINT_ID) \
        do {                                                                                                            \
            CTYPE __pb_expected = (CTYPE)(VALUE_EXPR);                                                                  \
            if (!FUNC((msg_ptr)->field_name, &__pb_expected, (RULE_ENUM))) {                                            \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), "Value constraint failed"); \
                if ((ctx_var).early_exit) return false;                                                                 \
            }                                                                                                           \
        } while (0)

    /* String length helpers for normal (non-callback) string fields. */
    #define PB_VALIDATE_STR_MIN_LEN(ctx_var, msg_ptr, field_name, MIN_LEN, CONSTRAINT_ID)                      \
        do {                                                                                                   \
            uint32_t __pb_min_len_v = (uint32_t)(MIN_LEN);                                                     \
            if (!pb_validate_string((msg_ptr)->field_name, (pb_size_t)strlen((msg_ptr)->field_name),          \
                                    &__pb_min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {                              \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), "String too short"); \
                if ((ctx_var).early_exit) return false;                                                        \
            }                                                                                                  \
        } while (0)

    #define PB_VALIDATE_STR_MAX_LEN(ctx_var, msg_ptr, field_name, MAX_LEN, CONSTRAINT_ID)                      \
        do {                                                                                                   \
            uint32_t __pb_max_len_v = (uint32_t)(MAX_LEN);                                                     \
            if (!pb_validate_string((msg_ptr)->field_name, (pb_size_t)strlen((msg_ptr)->field_name),          \
                                    &__pb_max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {                              \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), "String too long"); \
                if ((ctx_var).early_exit) return false;                                                        \
            }                                                                                                  \
        } while (0)

    /* String pattern helpers for normal (non-callback) string fields. */
    #define PB_VALIDATE_STR_PREFIX(ctx_var, msg_ptr, field_name, PREFIX_STR, CONSTRAINT_ID)                     \
        do {                                                                                                    \
            const char *__pb_prefix = (PREFIX_STR);                                                             \
            if (!pb_validate_string((msg_ptr)->field_name, (pb_size_t)strlen((msg_ptr)->field_name),           \
                                    __pb_prefix, PB_VALIDATE_RULE_PREFIX)) {                                   \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID),                 \
                                  "String must start with specified prefix");                                 \
                if ((ctx_var).early_exit) return false;                                                         \
            }                                                                                                   \
        } while (0)

    #define PB_VALIDATE_STR_SUFFIX(ctx_var, msg_ptr, field_name, SUFFIX_STR, CONSTRAINT_ID)                     \
        do {                                                                                                    \
            const char *__pb_suffix = (SUFFIX_STR);                                                             \
            if (!pb_validate_string((msg_ptr)->field_name, (pb_size_t)strlen((msg_ptr)->field_name),           \
                                    __pb_suffix, PB_VALIDATE_RULE_SUFFIX)) {                                   \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID),                 \
                                  "String must end with specified suffix");                                   \
                if ((ctx_var).early_exit) return false;                                                         \
            }                                                                                                   \
        } while (0)

    #define PB_VALIDATE_STR_CONTAINS(ctx_var, msg_ptr, field_name, NEEDLE_STR, CONSTRAINT_ID)                   \
        do {                                                                                                    \
            const char *__pb_needle = (NEEDLE_STR);                                                             \
            if (!pb_validate_string((msg_ptr)->field_name, (pb_size_t)strlen((msg_ptr)->field_name),           \
                                    __pb_needle, PB_VALIDATE_RULE_CONTAINS)) {                                 \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID),                 \
                                  "String must contain specified substring");                                  \
                if ((ctx_var).early_exit) return false;                                                         \
            }                                                                                                   \
        } while (0)

    /* Generic string validation macro for format rules (email, hostname, ip, etc.)
     * that don't require a parameter value. For callback-based string fields.
     */
    #define PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, RULE_ENUM, CONSTRAINT_ID, ERR_MSG) \
        do {                                                                                                    \
            const char *__pb_s = NULL; pb_size_t __pb_l = 0;                                                    \
            if (pb_read_callback_string(&(msg_ptr)->field_name, &__pb_s, &__pb_l)) {                            \
                if (!pb_validate_string(__pb_s, __pb_l, NULL, (RULE_ENUM))) {                                   \
                    pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), (ERR_MSG)); \
                    if ((ctx_var).early_exit) return false;                                                     \
                }                                                                                               \
            }                                                                                                   \
        } while (0)

    /* Generic string validation macro for length rules (min_len, max_len).
     * For callback-based string fields.
     */
    #define PB_VALIDATE_STRING_CALLBACK_LEN(ctx_var, msg_ptr, field_name, RULE_ENUM, LEN_VALUE, CONSTRAINT_ID, ERR_MSG) \
        do {                                                                                                    \
            const char *__pb_s = NULL; pb_size_t __pb_l = 0;                                                    \
            if (pb_read_callback_string(&(msg_ptr)->field_name, &__pb_s, &__pb_l)) {                            \
                uint32_t __pb_len_v = (uint32_t)(LEN_VALUE);                                                    \
                if (!pb_validate_string(__pb_s, __pb_l, &__pb_len_v, (RULE_ENUM))) {                            \
                    pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), (ERR_MSG)); \
                    if ((ctx_var).early_exit) return false;                                                     \
                }                                                                                               \
            }                                                                                                   \
        } while (0)

    /* Generic string validation macro for pattern rules (prefix, suffix, contains).
     * For callback-based string fields.
     */
    #define PB_VALIDATE_STRING_CALLBACK_PATTERN(ctx_var, msg_ptr, field_name, RULE_ENUM, PATTERN_STR, CONSTRAINT_ID, ERR_MSG) \
        do {                                                                                                    \
            const char *__pb_s = NULL; pb_size_t __pb_l = 0;                                                    \
            if (pb_read_callback_string(&(msg_ptr)->field_name, &__pb_s, &__pb_l)) {                            \
                const char *__pb_pattern = (PATTERN_STR);                                                       \
                if (!pb_validate_string(__pb_s, __pb_l, __pb_pattern, (RULE_ENUM))) {                           \
                    pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), (ERR_MSG)); \
                    if ((ctx_var).early_exit) return false;                                                     \
                }                                                                                               \
            }                                                                                                   \
        } while (0)

    /* Unified generic string macro for any rule type with optional parameter.
     * RULE_DATA_EXPR should be a pointer to rule data or NULL for format rules.
     * For callback-based string fields.
     */
    #define PB_VALIDATE_STRING_GENERIC(ctx_var, msg_ptr, field_name, RULE_ENUM, RULE_DATA_EXPR, CONSTRAINT_ID, ERR_MSG) \
        do {                                                                                                    \
            const char *__pb_s = NULL; pb_size_t __pb_l = 0;                                                    \
            if (pb_read_callback_string(&(msg_ptr)->field_name, &__pb_s, &__pb_l)) {                            \
                if (!pb_validate_string(__pb_s, __pb_l, (RULE_DATA_EXPR), (RULE_ENUM))) {                       \
                    pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), (ERR_MSG)); \
                    if ((ctx_var).early_exit) return false;                                                     \
                }                                                                                               \
            }                                                                                                   \
        } while (0)

    /* Format validation macros for callback-based string fields */
    #define PB_VALIDATE_STRING_EMAIL(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_EMAIL, CONSTRAINT_ID, "String format validation failed")

    #define PB_VALIDATE_STRING_HOSTNAME(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_HOSTNAME, CONSTRAINT_ID, "String format validation failed")

    #define PB_VALIDATE_STRING_IP(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_IP, CONSTRAINT_ID, "String format validation failed")

    #define PB_VALIDATE_STRING_IPV4(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_IPV4, CONSTRAINT_ID, "String format validation failed")

    #define PB_VALIDATE_STRING_IPV6(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_IPV6, CONSTRAINT_ID, "String format validation failed")

    #define PB_VALIDATE_STRING_ASCII(ctx_var, msg_ptr, field_name, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_FORMAT(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_ASCII, CONSTRAINT_ID, "String must contain only ASCII characters")

    /* Length validation macros for callback-based string fields */
    #define PB_VALIDATE_STRING_MIN_LEN(ctx_var, msg_ptr, field_name, MIN_LEN, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_LEN(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_MIN_LEN, MIN_LEN, CONSTRAINT_ID, "String too short")

    #define PB_VALIDATE_STRING_MAX_LEN(ctx_var, msg_ptr, field_name, MAX_LEN, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_LEN(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_MAX_LEN, MAX_LEN, CONSTRAINT_ID, "String too long")

    /* Pattern validation macros for callback-based string fields */
    #define PB_VALIDATE_STRING_PREFIX(ctx_var, msg_ptr, field_name, PREFIX_STR, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_PATTERN(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_PREFIX, PREFIX_STR, CONSTRAINT_ID, "String must start with specified prefix")

    #define PB_VALIDATE_STRING_SUFFIX(ctx_var, msg_ptr, field_name, SUFFIX_STR, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_PATTERN(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_SUFFIX, SUFFIX_STR, CONSTRAINT_ID, "String must end with specified suffix")

    #define PB_VALIDATE_STRING_CONTAINS(ctx_var, msg_ptr, field_name, NEEDLE_STR, CONSTRAINT_ID) \
        PB_VALIDATE_STRING_CALLBACK_PATTERN(ctx_var, msg_ptr, field_name, PB_VALIDATE_RULE_CONTAINS, NEEDLE_STR, CONSTRAINT_ID, "String must contain specified substring")

    /* Repeated field size helpers working on *_count naming convention. */
    #define PB_VALIDATE_MIN_ITEMS(ctx_var, msg_ptr, field_name, MIN_ITEMS, CONSTRAINT_ID)                      \
        do {                                                                                                   \
            if (!pb_validate_min_items((msg_ptr)->field_name##_count, (MIN_ITEMS))) {                          \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), "Too few items"); \
                if ((ctx_var).early_exit) return false;                                                        \
            }                                                                                                  \
        } while (0)

    #define PB_VALIDATE_MAX_ITEMS(ctx_var, msg_ptr, field_name, MAX_ITEMS, CONSTRAINT_ID)                      \
        do {                                                                                                   \
            if (!pb_validate_max_items((msg_ptr)->field_name##_count, (MAX_ITEMS))) {                          \
                pb_violations_add((ctx_var).violations, (ctx_var).path_buffer, (CONSTRAINT_ID), "Too many items"); \
                if ((ctx_var).early_exit) return false;                                                        \
            }                                                                                                  \
        } while (0)

    /* Rule types for internal use */
    typedef enum
    {
        PB_VALIDATE_RULE_REQUIRED,
        PB_VALIDATE_RULE_LT,
        PB_VALIDATE_RULE_LTE,
        PB_VALIDATE_RULE_GT,
        PB_VALIDATE_RULE_GTE,
        PB_VALIDATE_RULE_EQ,
        PB_VALIDATE_RULE_IN,
        PB_VALIDATE_RULE_NOT_IN,
        PB_VALIDATE_RULE_MIN_LEN,
        PB_VALIDATE_RULE_MAX_LEN,
        PB_VALIDATE_RULE_ASCII,
        PB_VALIDATE_RULE_PREFIX,
        PB_VALIDATE_RULE_SUFFIX,
        PB_VALIDATE_RULE_CONTAINS,
        PB_VALIDATE_RULE_EMAIL,
        PB_VALIDATE_RULE_HOSTNAME,
        PB_VALIDATE_RULE_IP,
        PB_VALIDATE_RULE_IPV4,
        PB_VALIDATE_RULE_IPV6,
        PB_VALIDATE_RULE_MIN_ITEMS,
        PB_VALIDATE_RULE_MAX_ITEMS,
        PB_VALIDATE_RULE_UNIQUE,
        PB_VALIDATE_RULE_ENUM_DEFINED,
        PB_VALIDATE_RULE_ONEOF_REQUIRED,
        PB_VALIDATE_RULE_REQUIRES,
        PB_VALIDATE_RULE_MUTEX,
        PB_VALIDATE_RULE_AT_LEAST
    } pb_validate_rule_type_t;

    /* Validation functions for primitive types */
    bool pb_validate_int32(int32_t value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_int64(int64_t value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_uint32(uint32_t value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_uint64(uint64_t value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_float(float value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_double(double value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_bool(bool value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_string(const char *value, pb_size_t length, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_bytes(const pb_bytes_array_t *value, const void *rule_data, pb_validate_rule_type_t rule_type);
    bool pb_validate_enum(int value, const void *rule_data, pb_validate_rule_type_t rule_type);

    /* Convenience helper for enum defined_only validation.
     * Returns true if 'value' is one of the provided enumeration values.
     */
    bool pb_validate_enum_defined_only(int value, const int *values, pb_size_t count);

    /* Helper to obtain const char* and length from a pb_callback_t string field for validation.
     * Usage model: The application should set the callback's arg pointer to a buffer that
     * contains a NUL-terminated string after decoding. This helper simply exposes that
     * pointer for validation (no copying). Returns true if a valid pointer was obtained.
     * If the callback is NULL or arg is NULL, returns false (treated as empty / missing).
     */
    struct pb_callback_s; /* forward decl from pb.h */
    bool pb_read_callback_string(const struct pb_callback_s *callback, const char **out_str, pb_size_t *out_len);

    /* Helper functions for validation context */
    bool pb_validate_context_push_field(pb_validate_context_t *ctx, const char *field_name);
    void pb_validate_context_pop_field(pb_validate_context_t *ctx);
    bool pb_validate_context_push_index(pb_validate_context_t *ctx, pb_size_t index);
    void pb_validate_context_pop_index(pb_validate_context_t *ctx);

    /* Repeated field count helpers */
    static inline bool pb_validate_min_items(pb_size_t count, pb_size_t min_required)
    {
        return count >= min_required;
    }

    static inline bool pb_validate_max_items(pb_size_t count, pb_size_t max_allowed)
    {
        return count <= max_allowed;
    }

/* Encode/decode hook macros (optional) */
#ifdef PB_VALIDATE_BEFORE_ENCODE
#define pb_validate_encode(stream, msg_type, msg) \
    (pb_validate_##msg_type(msg, NULL) ? pb_encode(stream, msg_type##_fields, msg) : false)
#endif

#ifdef PB_VALIDATE_AFTER_DECODE
#define pb_validate_decode(stream, msg_type, msg) \
    (pb_decode(stream, msg_type##_fields, msg) && pb_validate_##msg_type(msg, NULL))
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_H_INCLUDED */
