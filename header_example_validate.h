/* Validation header for header_example.proto */
#ifndef PB_VALIDATE_HEADER_EXAMPLE_INCLUDED
#define PB_VALIDATE_HEADER_EXAMPLE_INCLUDED

#include <pb_validate.h>
#include "header_example.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate my_pkg_Envelope message */
bool pb_validate_my_pkg_Envelope(const my_pkg_Envelope *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_HEADER_EXAMPLE_INCLUDED */
