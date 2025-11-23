/* Validation header for proto_examples/simple.proto */
#ifndef PB_VALIDATE_PROTO_EXAMPLES_SIMPLE_INCLUDED
#define PB_VALIDATE_PROTO_EXAMPLES_SIMPLE_INCLUDED

#include <pb_validate.h>
#include "proto_examples/simple.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate test_SimpleMessage message.
 *
 * Fields and constraints:
 * - bounded_float: <= 100.0; >= 0.0
 *
 * @param msg [in] Pointer to test_SimpleMessage instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_PROTO_EXAMPLES_SIMPLE_INCLUDED */
