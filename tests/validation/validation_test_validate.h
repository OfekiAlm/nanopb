/* Validation header for validation_test.proto */
#ifndef PB_VALIDATE_VALIDATION_TEST_INCLUDED
#define PB_VALIDATE_VALIDATION_TEST_INCLUDED

#include <pb_validate.h>
#include "validation_test.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate Person message.
 *
 * Fields and constraints:
 * - name: min length 1; max length 50; required
 * - email: min length 3; max length 100; contains "@"
 * - age: <= 150; >= 0
 * - gender: must be a defined enum value
 * - tags: at least 0 items; at most 10 items
 * - phone: min length 10
 *
 * @param msg [in] Pointer to Person instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_Person(const Person *msg, pb_violations_t *violations);

/**
 * @brief Validate Company message.
 *
 * Fields and constraints:
 * - name: required
 *
 * @param msg [in] Pointer to Company instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_Company(const Company *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_VALIDATION_TEST_INCLUDED */
