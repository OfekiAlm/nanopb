/* Validation header for test_any_validation.proto */
#ifndef PB_VALIDATE_TEST_ANY_VALIDATION_INCLUDED
#define PB_VALIDATE_TEST_ANY_VALIDATION_INCLUDED

#include <pb_validate.h>
#include "test_any_validation.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_BaseMessage message.
 *
 * Fields and constraints:
 * - payload: any in
 * - message_id: no constraints
 * - timestamp: no constraints
 *
 * @param msg [in] Pointer to test_BaseMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_BaseMessage(const test_BaseMessage *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RestrictedMessage message.
 *
 * Fields and constraints:
 * - payload: any not in
 * - message_id: no constraints
 *
 * @param msg [in] Pointer to test_RestrictedMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RestrictedMessage(const test_RestrictedMessage *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_ANY_VALIDATION_INCLUDED */
