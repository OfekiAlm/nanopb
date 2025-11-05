/* Validation header for test_envelope.proto */
#ifndef PB_VALIDATE_TEST_ENVELOPE_INCLUDED
#define PB_VALIDATE_TEST_ENVELOPE_INCLUDED

#include <pb_validate.h>
#include "test_envelope.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate test_Ping message */
bool pb_validate_test_Ping(const test_Ping *msg, pb_violations_t *violations);

/* Validate test_Pong message */
bool pb_validate_test_Pong(const test_Pong *msg, pb_violations_t *violations);

/* Validate test_Request message */
bool pb_validate_test_Request(const test_Request *msg, pb_violations_t *violations);

/* Validate test_Response message */
bool pb_validate_test_Response(const test_Response *msg, pb_violations_t *violations);

/* Validate test_Error message */
bool pb_validate_test_Error(const test_Error *msg, pb_violations_t *violations);

/* Validate test_Notification message */
bool pb_validate_test_Notification(const test_Notification *msg, pb_violations_t *violations);

/* Validate test_Envelope message */
bool pb_validate_test_Envelope(const test_Envelope *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_ENVELOPE_INCLUDED */
