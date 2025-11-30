/* Validation header for proto_examples/advanced.proto */
#ifndef PB_VALIDATE_PROTO_EXAMPLES_ADVANCED_INCLUDED
#define PB_VALIDATE_PROTO_EXAMPLES_ADVANCED_INCLUDED

#include <pb_validate.h>
#include "proto_examples/advanced.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_AdvancedMessage message.
 *
 * Fields and constraints:
 * - values: at least 1 items; at most 5 items; items must be unique; per-item validation rules
 * - email: valid email address
 * - test_oneof: no constraints
 *
 * @param msg [in] Pointer to test_AdvancedMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_AdvancedMessage(const test_AdvancedMessage *msg, pb_violations_t *violations);

/**
 * @brief Validate test_SimpleMessage message.
 *
 * Fields and constraints:
 * - bounded_float: <= 100.0; >= 0.0
 * - optional_string: min length 5; max length 20
 * - advanced_message: no constraints
 *
 * @param msg [in] Pointer to test_SimpleMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_PROTO_EXAMPLES_ADVANCED_INCLUDED */
