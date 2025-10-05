# Proto Filter Wrapper

This adds a small, generic wrapper that decodes packets with nanopb and invokes the generated validation code.

Public API (see `proto_filter.h`):

- `void proto_filter_register(const proto_filter_spec_t *spec)` – register the active filter.
- `int filter_tcp(void *ctx, char *packet, size_t packet_size, bool is_to_server)`
- `int filter_udp(void *ctx, char *packet, size_t packet_size, bool is_to_server)`

Where `proto_filter_spec_t` contains:
- `const pb_msgdesc_t *msg_desc` – e.g. `&test_BasicValidation_msg`.
- `size_t msg_size` – e.g. `sizeof(test_BasicValidation)`.
- `proto_filter_validate_fn validate` – adapter to the generated validator function.
- `proto_filter_prepare_decode_fn prepare_decode` – optional hook to set callback pointers, etc.

Example registration (pseudo-code):

```c
#include "gengen/test_basic_validation.pb.h"
#include "gengen/test_basic_validation_validate.h"
#include "proto_filter.h"

static bool validate_adapter(const void *msg, pb_violations_t *viol) {
    const test_BasicValidation *m = (const test_BasicValidation *)msg;
    return pb_validate_test_BasicValidation(m, viol);
}

static void prepare_decode(void *msg, void *user_ctx, bool is_to_server) {
    (void)user_ctx; (void)is_to_server;
    // Optionally set callback .arg pointers before decode if needed
}

void init_filter(void) {
    static const proto_filter_spec_t spec = {
        .msg_desc = &test_BasicValidation_msg,
        .msg_size = sizeof(test_BasicValidation),
        .validate = validate_adapter,
        .prepare_decode = prepare_decode
    };
    proto_filter_register(&spec);
}
```

Then call `filter_tcp(...)` or `filter_udp(...)` with raw packet bytes.

Return codes:
- `0` on success (valid), negative error code on failure (invalid/undecodable/unregistered).
