/* Validation header for header_example.proto */
#ifndef PB_VALIDATE_HEADER_EXAMPLE_INCLUDED
#define PB_VALIDATE_HEADER_EXAMPLE_INCLUDED

#include <pb_validate.h>
#include "header_example.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate my_pkg_Envelope message.
 *
 * Fields and constraints:
 * - version: > 0; < 100
 * - opcode: must be a defined enum value
 * - payload: no constraints
 *
 * @param msg [in] Pointer to my_pkg_Envelope instance to validate.
 * @param violations [out] Optional violations accumulator (can be NULL).
 * @return true if valid, false otherwise.
 */
bool pb_validate_my_pkg_Envelope(const my_pkg_Envelope *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_HEADER_EXAMPLE_INCLUDED */
