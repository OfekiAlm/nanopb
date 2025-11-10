/* Validation header for test_any_envelope.proto */
#ifndef PB_VALIDATE_TEST_ANY_ENVELOPE_INCLUDED
#define PB_VALIDATE_TEST_ANY_ENVELOPE_INCLUDED

#include <pb_validate.h>
#include "test_any_envelope.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_LoginRequest message.
 *
 * Fields and constraints:
 * - username: no constraints
 * - password: no constraints
 *
 * @param msg [in] Pointer to test_LoginRequest instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_LoginRequest(const test_LoginRequest *msg, pb_violations_t *violations);

/**
 * @brief Validate test_LogoutRequest message.
 *
 * Fields and constraints:
 * - session_id: no constraints
 *
 * @param msg [in] Pointer to test_LogoutRequest instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_LogoutRequest(const test_LogoutRequest *msg, pb_violations_t *violations);

/**
 * @brief Validate test_ChatMessage message.
 *
 * Fields and constraints:
 * - content: no constraints
 * - timestamp: no constraints
 *
 * @param msg [in] Pointer to test_ChatMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_ChatMessage(const test_ChatMessage *msg, pb_violations_t *violations);

/**
 * @brief Validate test_BaseMessage message.
 *
 * Fields and constraints:
 * - message_id: no constraints
 * - payload: no constraints
 * - timestamp: no constraints
 *
 * @param msg [in] Pointer to test_BaseMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_BaseMessage(const test_BaseMessage *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_ANY_ENVELOPE_INCLUDED */
