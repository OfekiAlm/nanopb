/* Validation header for test_simple.proto */
#ifndef PB_VALIDATE_TEST_SIMPLE_INCLUDED
#define PB_VALIDATE_TEST_SIMPLE_INCLUDED

#include <pb_validate.h>
#include "test_simple.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate test_SimpleRequest message */
bool pb_validate_test_SimpleRequest(const test_SimpleRequest *msg, pb_violations_t *violations);

/* Validate test_SimpleResponse message */
bool pb_validate_test_SimpleResponse(const test_SimpleResponse *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_SIMPLE_INCLUDED */
