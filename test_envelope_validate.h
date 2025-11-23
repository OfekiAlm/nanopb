/* Validation header for test_envelope.proto */
#ifndef PB_VALIDATE_TEST_ENVELOPE_INCLUDED
#define PB_VALIDATE_TEST_ENVELOPE_INCLUDED

#include <pb_validate.h>
#include "test_envelope.pb.h"

/*
 * Helper macros used by generated validation code to keep it concise and readable.
 * You can override PBV_EARLY_EXIT before including this header to tweak behavior.
 */
#ifndef PBV_EARLY_EXIT
#define PBV_EARLY_EXIT PB_VALIDATE_EARLY_EXIT
#endif

#ifndef PB_VALIDATE_HELPER_MACROS_DEFINED
#define PB_VALIDATE_HELPER_MACROS_DEFINED 1

#define PBV_INIT() do { \
    pb_validate_context_t ctx = {0}; \
    ctx.violations = violations; \
    ctx.early_exit = PBV_EARLY_EXIT; \
} while(0)
#define PBV_FIELD(name) do { if (!pb_validate_context_push_field(&ctx, (name))) return false; } while(0)
#define PBV_FIELD_END() do { pb_validate_context_pop_field(&ctx); } while(0)
#define PBV_POP_AND_FAIL() do { pb_validate_context_pop_field(&ctx); return false; } while(0)
#define PBV_FAIL(rule_id, msgtxt) do { pb_violations_add(violations, ctx.path_buffer, (rule_id), (msgtxt)); if (ctx.early_exit) return false; } while(0)
#define PBV_CHECK_NUM(validator, ctype, value_expr, expected_expr, rule_enum, rule_id) do { ctype __pbv_expected = (ctype)(expected_expr); if (!(validator((value_expr), &__pbv_expected, (rule_enum)))) { PBV_FAIL((rule_id), "Value constraint failed"); } } while(0)
#define PBV_STRING_CHECK(s,l,param,rule_enum,rule_id,msgtxt) do { if (!pb_validate_string((s),(l),(param),(rule_enum))) { PBV_FAIL((rule_id),(msgtxt)); } } while(0)
#define PBV_STRING_CB(field, BODY) do { const char *s = NULL; pb_size_t l = 0; if (pb_read_callback_string(&msg->field, &s, &l)) { BODY } } while(0)
#define PBV_STRING_INLINE(field, BODY) do { const char *s = msg->field; pb_size_t l = (pb_size_t)strlen(s); { BODY } } while(0)
#define PBV_ENUM_DEFINED_ONLY(value_expr, array, rule_id) do { if (!pb_validate_enum_defined_only((int)(value_expr), (array), (pb_size_t)(sizeof(array)/sizeof((array)[0])))) { PBV_FAIL((rule_id), "Value must be a defined enum value"); } } while(0)
#define PBV_MIN_ITEMS(count, minv, rule_id) do { if (!pb_validate_min_items((count), (minv))) { PBV_FAIL((rule_id), "Too few items"); } } while(0)
#define PBV_MAX_ITEMS(count, maxv, rule_id) do { if (!pb_validate_max_items((count), (maxv))) { PBV_FAIL((rule_id), "Too many items"); } } while(0)
#define PBV_RECURSE_PTR(field, func) do { if (msg->field) { bool ok_nested = func(msg->field, violations); if (!ok_nested && ctx.early_exit) { PBV_POP_AND_FAIL(); } } } while(0)
#define PBV_RECURSE_INLINE(field, func) do { { bool ok_nested = func(&msg->field, violations); if (!ok_nested && ctx.early_exit) { PBV_POP_AND_FAIL(); } } } while(0)
#define PBV_RECURSE_INLINE_OPT(has_field, field, func) do { if (msg->has_field) { bool ok_nested = func(&msg->field, violations); if (!ok_nested && ctx.early_exit) { PBV_POP_AND_FAIL(); } } } while(0)

#endif /* PB_VALIDATE_HELPER_MACROS_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_Ping message.
 *
 * Fields and constraints:
 * - timestamp: > 0
 * - sequence: no constraints
 *
 * @param msg [in] Pointer to test_Ping instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Ping(const test_Ping *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Pong message.
 *
 * Fields and constraints:
 * - timestamp: > 0
 * - sequence: no constraints
 * - latency_ms: no constraints
 *
 * @param msg [in] Pointer to test_Pong instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Pong(const test_Pong *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Request message.
 *
 * Fields and constraints:
 * - method: min length 1
 * - payload: no constraints
 * - request_id: no constraints
 *
 * @param msg [in] Pointer to test_Request instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Request(const test_Request *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Response message.
 *
 * Fields and constraints:
 * - status_code: >= 0; < 600
 * - payload: no constraints
 * - request_id: no constraints
 *
 * @param msg [in] Pointer to test_Response instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Response(const test_Response *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Error message.
 *
 * Fields and constraints:
 * - error_code: > 0
 * - message: min length 1
 * - details: no constraints
 *
 * @param msg [in] Pointer to test_Error instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Error(const test_Error *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Notification message.
 *
 * Fields and constraints:
 * - event_type: min length 1
 * - data: no constraints
 * - timestamp: no constraints
 *
 * @param msg [in] Pointer to test_Notification instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Notification(const test_Notification *msg, pb_violations_t *violations);

/**
 * @brief Validate test_Envelope message.
 *
 * Fields and constraints:
 * - version: >= 1; <= 10
 * - msg_type: must be a defined enum value
 * - correlation_id: no constraints
 * - message: no constraints
 *
 * @param msg [in] Pointer to test_Envelope instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_Envelope(const test_Envelope *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_ENVELOPE_INCLUDED */
