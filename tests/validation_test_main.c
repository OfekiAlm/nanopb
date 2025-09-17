#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

#include "gengen/test_basic_validation.pb.h"
#include "gengen/test_basic_validation_validate.h"

static void print_violations(const pb_violations_t *viol)
{
    printf("violations: %u (truncated=%s)\n", (unsigned)pb_violations_count(viol), viol->truncated ? "true" : "false");
    for (pb_size_t i = 0; i < pb_violations_count(viol); ++i)
    {
        const pb_violation_t *v = &viol->violations[i];
        printf("- %s: %s (%s)\n", v->field_path ? v->field_path : "<path>", v->message ? v->message : "<msg>", v->constraint_id ? v->constraint_id : "<rule>");
    }
}

int main(void)
{
    /* Happy path: valid message */
    test_BasicValidation msg = test_BasicValidation_init_zero;
    msg.age = 42;                /* 0 <= age <= 150 */
    msg.score = 1;               /* > 0 */
    msg.user_id = 123;           /* > 0 */
    msg.timestamp = 0;           /* >= 0 */
    msg.count = 7;               /* <= 1000 */
    msg.size = 50;               /* 10 <= size <= 100 */
    msg.has_total_bytes = false; /* optional, unset => skip */
    msg.sequence_num = 1;        /* >= 1 */

    pb_violations_t viol;
    pb_violations_init(&viol);
    bool ok = pb_validate_test_BasicValidation(&msg, &viol);
    if (!ok)
    {
        printf("Validation (happy) returned false unexpectedly.\n");
        print_violations(&viol);
        return 1;
    }
    if (pb_violations_has_any(&viol))
    {
        printf("Validation (happy) produced unexpected violations.\n");
        print_violations(&viol);
        return 1;
    }

    /* Negative tests: trigger a few numeric violations */
    test_BasicValidation bad = test_BasicValidation_init_zero;
    bad.age = -1;       /* violates gte 0 */
    bad.score = 0;      /* violates gt 0 */
    bad.user_id = 0;    /* violates gt 0 */
    bad.timestamp = -5; /* violates gte 0 */
    bad.count = 5000;   /* violates lte 1000 */
    bad.size = 5;       /* violates gte 10 */
    bad.has_total_bytes = true;
    bad.total_bytes = 1000000000; /* violates lt 1000000000 */
    bad.sequence_num = 0;         /* violates gte 1 */

    pb_violations_init(&viol);
    ok = pb_validate_test_BasicValidation(&bad, &viol);
    if (ok || !pb_violations_has_any(&viol))
    {
        printf("Expected violations for 'bad' message, but validation passed.\n");
        return 1;
    }
    print_violations(&viol);

    /* Encode and decode the valid message to test pb functionality */
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&ostream, &test_BasicValidation_msg, &msg))
    {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    size_t encoded_size = ostream.bytes_written;
    printf("Encoded size: %zu bytes\n", encoded_size);

    test_BasicValidation round = test_BasicValidation_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, encoded_size);
    if (!pb_decode(&istream, &test_BasicValidation_msg, &round))
    {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }

    /* Re-validate decoded */
    pb_violations_init(&viol);
    ok = pb_validate_test_BasicValidation(&round, &viol);
    if (!ok || pb_violations_has_any(&viol))
    {
        printf("Validation after decode failed.\n");
        print_violations(&viol);
        return 1;
    }

    printf("All tests passed.\n");
    return 0;
}
