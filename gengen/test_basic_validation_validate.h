/* Validation header for test_basic_validation.proto */
#ifndef PB_VALIDATE_TEST_BASIC_VALIDATION_INCLUDED
#define PB_VALIDATE_TEST_BASIC_VALIDATION_INCLUDED

#include <pb_validate.h>
#include "test_basic_validation.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate test_BasicValidation message */
bool pb_validate_test_BasicValidation(const test_BasicValidation *msg, pb_violations_t *violations);

/* Validate test_Address message */
bool pb_validate_test_Address(const test_Address *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_BASIC_VALIDATION_INCLUDED */
