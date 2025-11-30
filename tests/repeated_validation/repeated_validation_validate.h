/* Validation header for tests/repeated_validation/repeated_validation.proto */
#ifndef PB_VALIDATE_TESTS_REPEATED_VALIDATION_REPEATED_VALIDATION_INCLUDED
#define PB_VALIDATE_TESTS_REPEATED_VALIDATION_REPEATED_VALIDATION_INCLUDED

#include <pb_validate.h>
#include "tests/repeated_validation/repeated_validation.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_RepeatedStringItems message.
 *
 * Fields and constraints:
 * - values: per-item validation rules
 *
 * @param msg [in] Pointer to test_RepeatedStringItems instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedStringItems(const test_RepeatedStringItems *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RepeatedInt32Items message.
 *
 * Fields and constraints:
 * - values: per-item validation rules
 *
 * @param msg [in] Pointer to test_RepeatedInt32Items instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedInt32Items(const test_RepeatedInt32Items *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RepeatedUniqueStrings message.
 *
 * Fields and constraints:
 * - values: items must be unique
 *
 * @param msg [in] Pointer to test_RepeatedUniqueStrings instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedUniqueStrings(const test_RepeatedUniqueStrings *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RepeatedUniqueInt32 message.
 *
 * Fields and constraints:
 * - values: items must be unique
 *
 * @param msg [in] Pointer to test_RepeatedUniqueInt32 instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedUniqueInt32(const test_RepeatedUniqueInt32 *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RepeatedBothConstraints message.
 *
 * Fields and constraints:
 * - values: items must be unique; per-item validation rules
 *
 * @param msg [in] Pointer to test_RepeatedBothConstraints instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedBothConstraints(const test_RepeatedBothConstraints *msg, pb_violations_t *violations);

/**
 * @brief Validate test_RepeatedAllConstraints message.
 *
 * Fields and constraints:
 * - numbers: at least 1 items; at most 10 items; items must be unique; per-item validation rules
 *
 * @param msg [in] Pointer to test_RepeatedAllConstraints instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_RepeatedAllConstraints(const test_RepeatedAllConstraints *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TESTS_REPEATED_VALIDATION_REPEATED_VALIDATION_INCLUDED */
