# Filter TCP/UDP Validation Tests

This directory contains SCons-based tests for the `filter_tcp` and `filter_udp` validation flow using both oneof fields and google.protobuf.Any.

## Test Directories

### filter_oneof

Tests validation of messages containing oneof fields through the filter_tcp/filter_udp flow.

**Key Features:**
- Tests header opcode field with range validation (1-3)
- Tests oneof payload with three variants:
  - `auth_username`: String field with min_len validation (>= 3 chars)
  - `data_value`: Int32 field with non-negative validation (>= 0)
  - `status`: Nested message type (demonstrates structure)
- Exercises both `filter_tcp()` and `filter_udp()` entry points
- Validates correct rejection of invalid messages
- Validates correct acceptance of valid messages

**Test Coverage:**
- Valid and invalid scalar oneof members (string, int32)
- Boundary condition testing (empty strings, zero values)
- Out-of-range opcode validation
- Nested message types in oneofs (with manual validation example)

**Build and Run:**
```bash
cd tests
scons filter_oneof
```

### filter_any

Tests validation of messages containing google.protobuf.Any fields through the filter_tcp/filter_udp flow.

**Key Features:**
- Tests `any.in` validation (whitelist of allowed type_urls)
  - Allows: `type.googleapis.com/UserInfo` and `type.googleapis.com/ProductInfo`
  - Rejects: any other type_url
- Tests `any.not_in` validation (blacklist of disallowed type_urls)
  - Disallows: `type.googleapis.com/OrderInfo`
  - Allows: all other type_urls
- Nested message validation within Any payloads:
  - UserInfo: user_id > 0, valid email format, age 0-150
  - ProductInfo: product_id > 0, name min_len 1, price >= 0
  - OrderInfo: order_id > 0, total > 0

**Test Coverage:**
- Valid Any with allowed type_url
- Invalid Any with disallowed type_url
- Invalid Any with unknown type_url
- Any.in rule validation (whitelist)
- Any.not_in rule validation (blacklist)
- Proper Any packing (type_url + serialized bytes)

**Build and Run:**
```bash
cd tests
scons filter_any
```

## Implementation Details

### Proto File Generation

Both tests use the nanopb generator with the `--validate` flag to generate validation code:
- `*.pb.c` / `*.pb.h`: Standard nanopb message structures
- `*_validate.c` / `*_validate.h`: Generated validation functions

### Filter Integration

The tests use the `proto_filter` infrastructure:
1. Register a `proto_filter_spec_t` with:
   - Message descriptor
   - Message size
   - Validator function adapter
2. Encode test messages to binary
3. Call `filter_tcp()` or `filter_udp()` to decode and validate
4. Assert expected results (OK or validation failure)

### Validation Flow

```
Message Construction → Encoding → filter_tcp/udp() → Decode → Validate → Result
```

The validation happens automatically through the filter infrastructure, mimicking real-world usage where network packets are received and validated.

## Known Limitations

### Nested Messages in Oneofs

The nanopb validator generator currently does not automatically generate validation for nested message types within oneof fields. For example, in `filter_oneof.proto`, the `StatusPayload` message within the oneof requires manual validation if needed:

```c
// Manual validation of nested message
StatusPayload status = StatusPayload_init_zero;
// ... set fields ...
pb_violations_t viol;
pb_violations_init(&viol);
bool ok = pb_validate_StatusPayload(&status, &viol);
```

This is a known limitation of the generator and is documented in the test code.

## Test Structure

Both tests follow a similar pattern:
1. **Setup**: Initialize message structures and filter specs
2. **Register**: Register the filter spec with `proto_filter_register()`
3. **Test Cases**:
   - Construct valid/invalid messages
   - Encode to binary format
   - Call filter_tcp/udp
   - Assert expected behavior
4. **Summary**: Report pass/fail counts

Each test is self-contained and reports clear pass/fail status for debugging.
