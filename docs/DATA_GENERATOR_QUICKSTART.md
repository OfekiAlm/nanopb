# Nanopb Data Generator - Quick Start

## Overview

The **Nanopb Data Generator** is a Python tool that generates valid and invalid protobuf test data based on validation constraints from `.proto` files. It's perfect for testing your nanopb-generated C code, validation logic, and creating test vectors.

## Installation

No additional installation needed beyond nanopb dependencies:

```bash
pip install protobuf grpcio-tools
```

## Quick Examples

### 1. Generate Valid Data (C Array)

```bash
python3 generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation -I generator/proto
```

Output:
```c
const uint8_t basicvalidation_data[] = {0x08, 0x40, 0x10, 0xad, ...};
const size_t basicvalidation_data_size = 252;
```

### 2. Generate Invalid Data (Violating a Specific Rule)

```bash
python3 generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --invalid --field age --rule lte -I generator/proto
```

This generates data where `age > 150`, violating the `lte = 150` constraint.

### 3. Generate Hex Output

```bash
python3 generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --format hex -I generator/proto
```

Output:
```
084b10321801b6a503ba...
```

### 4. Save to Binary File

```bash
python3 generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --format binary -o test_data.bin -I generator/proto
```

> **Tip:** The generator works for any proto that uses `validate.proto` style
> constraints, not just the bundled examples. When compiling standalone files
> (like `basic.proto` in the repo root), pass the path to `validate.proto`
> through `-I generator/proto` so protoc can resolve the validation options.

## Python API Usage

### Basic Generation

```python
from generator.nanopb_data_generator import DataGenerator, OutputFormat

# Create generator
gen = DataGenerator('test_basic_validation.proto', include_paths=['generator/proto'])

# Generate valid data
valid_data = gen.generate_valid('BasicValidation', seed=42)
print(valid_data)
# {'age': 75, 'username': 'john_doe', ...}

# Encode to binary
binary = gen.encode_to_binary('BasicValidation', valid_data)

# Format as C array
c_code = gen.format_output(binary, OutputFormat.C_ARRAY, 'test_data')
print(c_code)
```

### Generate Invalid Data

```python
# Random violation
invalid_data = gen.generate_invalid('BasicValidation')

# Specific violation
invalid_data = gen.generate_invalid(
    'BasicValidation',
    violate_field='age',
    violate_rule='lte'
)
print(invalid_data['age'])  # Will be > 150
```

### Systematic Testing

```python
# Test all validation rules
all_fields = gen.get_all_fields('BasicValidation')

for field_name, field_info in all_fields.items():
    for constraint in field_info.constraints:
        invalid_data = gen.generate_invalid(
            'BasicValidation',
            violate_field=field_name,
            violate_rule=constraint.rule_type
        )
        binary = gen.encode_to_binary('BasicValidation', invalid_data)
        # Use binary for testing
        test_validation_function(binary)
```

## Supported Validation Rules

### Numeric (int32, int64, uint32, uint64, float, double)
- `gt`, `gte`, `lt`, `lte` - Range constraints
- `const` - Exact value
- `in`, `not_in` - Allowed/forbidden values

### String
- `min_len`, `max_len` - Length constraints
- `prefix`, `suffix` - Required prefix/suffix
- `contains` - Required substring
- `ascii` - ASCII-only characters
- `in`, `not_in` - Allowed/forbidden values

### Repeated Fields
- `min_items`, `max_items` - Count constraints
- `unique` - All items must be unique

### Boolean
- `const` - Exact value

## Example Proto File

```protobuf
syntax = "proto3";
import "validate.proto";

message User {
  int32 age = 1 [
    (validate.rules).int32.gte = 18,
    (validate.rules).int32.lte = 120
  ];
  
  string email = 2 [
    (validate.rules).string.contains = "@",
    (validate.rules).string.min_len = 5
  ];
  
  repeated string tags = 3 [
    (validate.rules).repeated.min_items = 1,
    (validate.rules).repeated.max_items = 10
  ];
}
```

## Output Formats

| Format | Description | Use Case |
|--------|-------------|----------|
| `c_array` | C array with size | Embed in C test files |
| `binary` | Raw bytes | Save to file for testing |
| `hex` | Hex string | Debugging, documentation |
| `dict` | Python dict | Inspection, debugging |

## CLI Options Reference

```
usage: nanopb_data_generator.py [-h] [--invalid] [--field FIELD] [--rule RULE]
                                [--format {binary,c_array,hex,dict}]
                                [--output OUTPUT] [--seed SEED] [-I INCLUDE]
                                proto_file message

positional arguments:
  proto_file            Path to .proto file
  message               Message name to generate data for

optional arguments:
  -h, --help            Show help message
  --invalid             Generate invalid data
  --field FIELD         Field to violate (for invalid data)
  --rule RULE           Rule to violate (for invalid data)
  --format FORMAT       Output format (default: c_array)
  --output FILE, -o     Output file (default: stdout)
  --seed SEED           Random seed for reproducibility
  -I DIR, --include     Include path for proto files
```

## Integration with Nanopb

### Testing Validation Code

```c
#include "test_basic_validation.pb.h"
#include "test_basic_validation_validate.h"
#include <pb_decode.h>

// Generated by data generator
const uint8_t invalid_age_data[] = {0x08, 0xb9, 0x01, ...};
const size_t invalid_age_data_size = 270;

void test_age_validation() {
    BasicValidation msg = BasicValidation_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(
        invalid_age_data,
        invalid_age_data_size
    );
    
    // Decode should succeed
    assert(pb_decode(&stream, BasicValidation_fields, &msg));
    
    // But validation should fail (age is 185, limit is 150)
    assert(!pb_validate_BasicValidation(&msg));
}
```

### Fuzzing

```python
import random
from generator.nanopb_data_generator import DataGenerator

gen = DataGenerator('api.proto')

# Generate 1000 test cases (70% valid, 30% invalid)
for i in range(1000):
    if random.random() < 0.7:
        data = gen.generate_valid('ApiRequest', seed=i)
    else:
        data = gen.generate_invalid('ApiRequest', seed=i)
    
    binary = gen.encode_to_binary('ApiRequest', data)
    test_your_api(binary)
```

## Architecture

```
DataGenerator
├── ProtoFieldInfo        # Parses field descriptors & constraints
├── Value Generators      # Type-specific valid/invalid value generation
│   ├── _generate_valid_int32()
│   ├── _generate_valid_string()
│   └── _generate_invalid_value()
├── Encoder              # Protobuf wire format encoding
│   └── encode_to_binary()
└── Formatters           # Output formatting
    └── format_output()
```

## Common Use Cases

### 1. Unit Test Data Generation

```bash
# Generate test vectors for all validation rules
for rule in gte lte min_len max_len; do
    python3 generator/nanopb_data_generator.py my.proto MyMessage \
        --invalid --field my_field --rule $rule \
        -o tests/invalid_${rule}.bin --format binary
done
```

### 2. Documentation Examples

```bash
# Generate example data for documentation
python3 generator/nanopb_data_generator.py api.proto UserRequest \
    --format hex --seed 42 > docs/example_request.hex
```

### 3. Regression Testing

```python
# Generate and save test data with fixed seed
gen = DataGenerator('schema.proto')
test_data = gen.generate_valid('Message', seed=12345)
binary = gen.encode_to_binary('Message', test_data)

# Save for regression tests
with open('tests/fixtures/message_v1.bin', 'wb') as f:
    f.write(binary)
```

## Tips & Best Practices

1. **Use Seeds**: Always use `--seed` for reproducible test data
2. **Test Boundaries**: Focus on edge cases (min/max values)
3. **Systematic Coverage**: Test all validation rules for each field
4. **Version Control**: Check in generated test data for regression testing
5. **Document Violations**: Add comments explaining what each invalid test case violates

## Limitations

- Nested messages not fully supported yet
- Enum validation limited
- Map fields have basic support only
- Some complex rules may need custom handling

## Example Script

See `examples/data_generation/generate_test_data.py` for a complete example:

```bash
cd examples/data_generation
python3 generate_test_data.py
```

This will generate multiple valid and invalid test cases demonstrating all features.

## Contributing

Found a bug or want to add a feature? Contributions welcome!

Areas for improvement:
- Nested message support
- Enum validation
- Map field support
- Performance optimizations

## See Also

- [Full Documentation](examples/data_generation/README.md)
- [Nanopb Project](https://github.com/nanopb/nanopb)
- [protoc-gen-validate](https://github.com/bufbuild/protoc-gen-validate)
