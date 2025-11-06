/* Validation header for test_simple.proto */
#ifndef PB_VALIDATE_TEST_SIMPLE_INCLUDED
#define PB_VALIDATE_TEST_SIMPLE_INCLUDED

#include <pb_validate.h>
#include "test_simple.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_SimpleRequest message.
 *
 * Fields and constraints:
 * - id: no constraints
 * - value: no constraints
 *
 * @param msg [in] Pointer to test_SimpleRequest instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_SimpleRequest(const test_SimpleRequest *msg, pb_violations_t *violations);

/**
 * @brief Validate test_SimpleResponse message.
 *
 * Fields and constraints:
 * - success: no constraints
 * - result: no constraints
 *
 * @param msg [in] Pointer to test_SimpleResponse instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_SimpleResponse(const test_SimpleResponse *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_SIMPLE_INCLUDED */
