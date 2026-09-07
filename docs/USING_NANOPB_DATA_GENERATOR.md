# Using nanopb_data_generator

## Overview

The `nanopb_data_generator.py` tool automatically generates test data for Protocol Buffer messages based on validation constraints. It creates both valid data (satisfying all constraints) and invalid data (violating specific constraints) for comprehensive testing.

## Why Use a Data Generator?

### Manual Test Data is Error-Prone

Writing test data by hand is tedious and error-prone:

```c
// Manual test data - did we get it right?
uint8_t test_data[] = {
    0x0a, 0x08, 0x53, 0x45, 0x4e, 0x53, 0x4f, 0x52, 0x30, 0x31,
    0x15, 0x00, 0x00, 0xbc, 0x41,
    // ... many more bytes
};
```

Problems:
- Hard to read and verify
- Easy to make mistakes
- No guarantee it matches validation rules
- Tedious to create variations

### Automated Generation Benefits

The data generator:
- ✅ Creates data that's guaranteed to match your validation rules
- ✅ Generates both valid and invalid test cases automatically
- ✅ Produces multiple output formats (binary, C arrays, hex)
- ✅ Saves time and reduces errors
- ✅ Makes testing validation logic straightforward

## Basic Usage

### Generate Valid Data

```bash
# Generate valid data as C array (default format)
python generator/nanopb_data_generator.py message.proto MessageType

# Output: C array printed to stdout
```

**Example output:**
```c
const uint8_t test_data[] = {
    0x0a, 0x08, 0x53, 0x45, 0x4e, 0x53, 0x4f, 0x52, 0x30, 0x31,
    0x15, 0x00, 0x00, 0xbc, 0x41, 0x18, 0xd2, 0x85, 0xd8, 0xcc,
    0x04
};
const size_t test_data_size = 21;
```

### Generate Invalid Data

```bash
# Generate data that violates a specific constraint
python generator/nanopb_data_generator.py message.proto MessageType \
    --invalid --field field_name --rule rule_type
```

**Example:**
```bash
# Violate the "age >= 0" constraint
python generator/nanopb_data_generator.py user.proto User \
    --invalid --field age --rule gte
```

## Command-Line Options

### Basic Options

```bash
# Specify output format
python generator/nanopb_data_generator.py message.proto Type --format binary

# Save to file
python generator/nanopb_data_generator.py message.proto Type -o output.bin

# Add include paths
python generator/nanopb_data_generator.py message.proto Type -I proto -I vendor

# Set random seed for reproducible output
python generator/nanopb_data_generator.py message.proto Type --seed 42

# Verbose output
python generator/nanopb_data_generator.py message.proto Type --verbose
```

### Invalid Data Options

```bash
# Generate invalid data
--invalid              # Enable invalid data generation
--field FIELD_NAME     # Field to violate
--rule RULE_TYPE       # Which rule to violate (gte, lte, min_len, etc.)
```

### Output Format Options

```bash
--format FORMAT        # Output format:
                       #   c_array     - C array initializer (default)
                       #   binary      - Raw binary data
                       #   hex_string  - Hexadecimal string
                       #   python_dict - Python dictionary

-o FILE               # Output file (default: stdout)
```

## Output Formats

### 1. C Array (Default)

**Usage:**
```bash
python generator/nanopb_data_generator.py sensor.proto Sensor
```

**Output:**
```c
const uint8_t test_data[] = {
    0x0a, 0x08, 0x53, 0x45, 0x4e, 0x53, 0x4f, 0x52, 0x30, 0x31,
    0x15, 0x00, 0x00, 0xbc, 0x41, 0x18, 0xd2, 0x85, 0xd8, 0xcc,
    0x04
};
const size_t test_data_size = 21;
```

**Use case:** Include directly in C test files.

### 2. Binary

**Usage:**
```bash
python generator/nanopb_data_generator.py sensor.proto Sensor \
    --format binary -o test.bin
```

**Output:** Raw binary file with protobuf encoded data.

**Use case:** 
- Input files for test programs
- Network protocol testing
- File-based integration tests

### 3. Hex String

**Usage:**
```bash
python generator/nanopb_data_generator.py sensor.proto Sensor \
    --format hex_string
```

**Output:**
```
0a0853454e534f5230311500bc411885d8cc04
```

**Use case:**
- Debugging
- Documentation
- Manual inspection

### 4. Python Dictionary

**Usage:**
```bash
python generator/nanopb_data_generator.py sensor.proto Sensor \
    --format python_dict
```

**Output:**
```python
{
    'sensor_id': 'SENSOR01',
    'temperature': 23.5,
    'timestamp': 1234567890
}
```

**Use case:**
- Understanding generated values
- Debugging
- Python-based testing

## Typical Use Cases

### Use Case 1: Unit Testing Valid Data

**Scenario:** Test that your decoder handles valid messages correctly.

**Generate test data:**
```bash
python generator/nanopb_data_generator.py message.proto MyMessage \
    --format c_array > test_valid_data.h
```

**Use in test:**
```c
#include "test_valid_data.h"
#include "message.pb.h"

void test_decode_valid() {
    MyMessage msg = MyMessage_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(test_data, test_data_size);
    
    bool status = pb_decode(&stream, MyMessage_msg, &msg);
    assert(status == true);
    
    // Verify fields are correct
    assert(validate_MyMessage(&msg, NULL));
}
```

### Use Case 2: Testing Validation Failures

**Scenario:** Verify that validation correctly rejects invalid data.

**message.proto:**
```protobuf
import "validate.proto";

message User {
    string username = 1 [(validate.rules).string.min_len = 3];
    int32 age = 2 [(validate.rules).int32.gte = 0,
                   (validate.rules).int32.lte = 150];
}
```

**Generate invalid data (username too short):**
```bash
python generator/nanopb_data_generator.py message.proto User \
    --invalid --field username --rule min_len \
    --format c_array > test_invalid_username.h
```

**Generate invalid data (age negative):**
```bash
python generator/nanopb_data_generator.py message.proto User \
    --invalid --field age --rule gte \
    --format c_array > test_invalid_age.h
```

**Test validation:**
```c
#include "test_invalid_username.h"
#include "message.val.h"

void test_validation_username() {
    User msg = User_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(test_data, test_data_size);
    pb_decode(&stream, User_msg, &msg);
    
    pb_validation_result_t result = PB_VALIDATION_RESULT_INIT;
    bool valid = validate_User(&msg, &result);
    
    assert(valid == false);  // Should fail validation
    assert(strstr(result.error_message, "username") != NULL);
}
```

### Use Case 3: Fuzz Testing

**Scenario:** Generate many test cases for fuzzing.

**Generate multiple test files:**
```bash
#!/bin/bash
for i in {1..100}; do
    python generator/nanopb_data_generator.py message.proto MyMessage \
        --seed $i --format binary -o fuzz_test_$i.bin
done
```

**Fuzz test:**
```c
void fuzz_test(const char *filename) {
    FILE *f = fopen(filename, "rb");
    uint8_t buffer[1024];
    size_t size = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    
    MyMessage msg = MyMessage_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(buffer, size);
    
    // Should not crash, even with random data
    pb_decode(&stream, MyMessage_msg, &msg);
}
```

### Use Case 4: Integration Testing

**Scenario:** Test end-to-end communication between devices.

**Generate test messages:**
```bash
# Generate request message
python generator/nanopb_data_generator.py api.proto Request \
    --format binary -o request.bin

# Generate expected response
python generator/nanopb_data_generator.py api.proto Response \
    --format binary -o response.bin
```

**Test:**
```python
# Test script
def test_api():
    request_data = open('request.bin', 'rb').read()
    response = send_to_device(request_data)
    expected = open('response.bin', 'rb').read()
    assert response == expected
```

### Use Case 5: Documentation and Examples

**Scenario:** Create example data for documentation.

**Generate human-readable examples:**
```bash
# Show structure as Python dict
python generator/nanopb_data_generator.py message.proto Example \
    --format python_dict

# Show wire format as hex
python generator/nanopb_data_generator.py message.proto Example \
    --format hex_string
```

Include in documentation:
```markdown
## Example Message

Structure:
```python
{
    'device_id': 'DEV-001',
    'value': 42.0
}
```

Wire format: `0a07444556...`
```

## Supported Validation Rules

The data generator understands all validation rules from `validate.proto`:

### Numeric Rules

```protobuf
int32 age = 1 [(validate.rules).int32 = {
    gte: 0,      // Greater than or equal
    lte: 150,    // Less than or equal
    gt: 0,       // Greater than (strict)
    lt: 200,     // Less than (strict)
    in: [1, 2, 3],        // Must be in list
    not_in: [99, 100]     // Must not be in list
}];
```

**Generated valid data:** Satisfies all constraints (e.g., age = 25)

**Generated invalid data:** Violates specified constraint (e.g., age = -1 for `--rule gte`)

### String Rules

```protobuf
string username = 1 [(validate.rules).string = {
    min_len: 3,
    max_len: 20,
    prefix: "user_",
    suffix: "_active",
    contains: "@"
}];
```

**Generated valid data:** Meets all constraints (e.g., "user_john_active")

**Generated invalid data:** Violates specified rule (e.g., "ab" for `--rule min_len`)

### Repeated Field Rules

```protobuf
repeated int32 values = 1 [(validate.rules).repeated = {
    min_items: 1,
    max_items: 10,
    unique: true
}];
```

**Generated valid data:** Array with 1-10 unique elements

**Generated invalid data:** 
- `--rule min_items`: Empty array
- `--rule max_items`: 11+ elements
- `--rule unique`: Duplicate elements

### Boolean Rules

```protobuf
bool enabled = 1 [(validate.rules).bool.const = true];
```

**Generated valid data:** `true`

**Generated invalid data:** `false`

## Advanced Usage

### Reproducible Data Generation

Use `--seed` for reproducible random data:

```bash
# Same seed = same output
python generator/nanopb_data_generator.py msg.proto Type --seed 42
python generator/nanopb_data_generator.py msg.proto Type --seed 42  # Identical
```

**Use case:** Reproducible test suites, debugging.

### Multiple Invalid Variations

Generate different invalid cases for the same field:

```bash
# Violate minimum constraint
python generator/nanopb_data_generator.py msg.proto Type \
    --invalid --field age --rule gte -o invalid_min.bin

# Violate maximum constraint
python generator/nanopb_data_generator.py msg.proto Type \
    --invalid --field age --rule lte -o invalid_max.bin
```

### Batch Generation Script

```bash
#!/bin/bash
# generate_test_data.sh

PROTO="message.proto"
MSG="MyMessage"

# Generate valid data
python generator/nanopb_data_generator.py $PROTO $MSG \
    --format c_array > test_data_valid.h

# Generate invalid variations
for field in age temperature pressure; do
    for rule in gte lte; do
        python generator/nanopb_data_generator.py $PROTO $MSG \
            --invalid --field $field --rule $rule \
            --format c_array > test_data_invalid_${field}_${rule}.h
    done
done
```

### Integration with Test Framework

**Example with Unity testing framework:**

```c
#include "unity.h"
#include "message.pb.h"
#include "message.val.h"

// Generated test data
#include "test_valid.h"
#include "test_invalid_age_gte.h"

void test_decode_valid_message(void) {
    MyMessage msg = MyMessage_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(test_data_valid, 
                                                  test_data_valid_size);
    
    TEST_ASSERT_TRUE(pb_decode(&stream, MyMessage_msg, &msg));
    TEST_ASSERT_TRUE(validate_MyMessage(&msg, NULL));
}

void test_reject_invalid_age(void) {
    MyMessage msg = MyMessage_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(test_data_invalid_age, 
                                                  test_data_invalid_age_size);
    
    pb_decode(&stream, MyMessage_msg, &msg);
    
    pb_validation_result_t result = PB_VALIDATION_RESULT_INIT;
    TEST_ASSERT_FALSE(validate_MyMessage(&msg, &result));
}
```

## Limitations and Considerations

### What the Generator Supports

✅ All basic validation rules (numeric, string, repeated)
✅ Multiple fields with constraints
✅ Nested messages
✅ Optional fields
✅ Enumerations

### What the Generator Doesn't Support

❌ Custom validation logic beyond validate.proto rules
❌ Cross-field validation (e.g., field1 > field2)
❌ Complex regex patterns
❌ Message-level custom validators

### Data Generation Strategy

**For valid data:**
- Numeric: Random value within bounds
- String: Random string matching length/pattern constraints
- Repeated: Random count within bounds, random elements
- Boolean: Random true/false (or const value if specified)

**For invalid data:**
- Numeric: Value just outside bounds (e.g., min-1, max+1)
- String: Too short, too long, or missing required pattern
- Repeated: Too few, too many, or duplicate elements

## Troubleshooting

### No Validation Rules

**Problem:** Generated data doesn't respect constraints.

**Cause:** `.proto` file doesn't import `validate.proto` or has no rules.

**Solution:**
```protobuf
import "validate.proto";  // Add this

message MyMessage {
    int32 value = 1 [(validate.rules).int32.gte = 0];  // Add rules
}
```

### Unknown Field

**Problem:** `Error: Field 'xyz' not found in message`

**Solution:** Check field name spelling and message type.

### Unknown Rule

**Problem:** `Error: Unknown rule type 'xyz'`

**Solution:** Use valid rule names: `gte`, `lte`, `min_len`, `max_len`, etc.

### Import Errors

**Problem:** `Proto file not found`

**Solution:** Add include paths:
```bash
python generator/nanopb_data_generator.py -I proto -I vendor message.proto Type
```

## Best Practices

### 1. Generate Test Data in Build Process

Include in your build system:

```makefile
test_data.h: message.proto
	python generator/nanopb_data_generator.py message.proto Type \
		--format c_array > test_data.h
```

### 2. Use Descriptive Output Names

```bash
# Good: Clear what's being tested
python generator/nanopb_data_generator.py msg.proto Type \
    --invalid --field age --rule gte -o test_age_negative.h

# Bad: Unclear
python generator/nanopb_data_generator.py msg.proto Type -o test1.h
```

### 3. Test Both Valid and Invalid Cases

```bash
# Generate comprehensive test suite
make test_data_valid.h
make test_data_invalid_field1.h
make test_data_invalid_field2.h
```

### 4. Use Seeds for Stable Tests

```bash
# Reproducible tests
python generator/nanopb_data_generator.py msg.proto Type --seed 12345
```

### 5. Document Generated Data

```c
// test_data.h
// Generated from: sensor.proto, SensorReading message
// Valid data: temperature=23.5°C, humidity=45%
const uint8_t test_data[] = { /* ... */ };
```

## Next Steps

- **[Validation Guide](validation.md)** - Learn about validation rules
- **[Using nanopb_generator](USING_NANOPB_GENERATOR.md)** - Generate C code with validation
- **[Developer Guide](DEVELOPER_GUIDE.md)** - Extend the data generator
