# Nanopb Data Generator

A Python module for generating valid and invalid protobuf test data based on validation constraints defined using `validate.proto`.

## Features

- 📝 **Parse validation rules** from `.proto` files
- ✅ **Generate valid data** that satisfies all constraints
- ❌ **Generate invalid data** that violates specific constraints
- 🔢 **Multiple output formats**: Binary, C arrays, hex strings
- 🎲 **Reproducible** with random seeds
- 🎯 **Type-safe** with full validation rule support

## Supported Validation Rules

### Numeric Types (int32, int64, uint32, uint64, sint32, sint64, float, double)
- `gt` - Greater than
- `gte` - Greater than or equal
- `lt` - Less than
- `lte` - Less than or equal
- `const` - Exact value
- `in` - Value must be in list
- `not_in` - Value must not be in list

### String/Bytes
- `min_len` - Minimum length
- `max_len` - Maximum length
- `const` - Exact value
- `prefix` - Required prefix
- `suffix` - Required suffix
- `contains` - Must contain substring
- `ascii` - ASCII-only characters
- `in` - Value must be in list
- `not_in` - Value must not be in list

### Repeated Fields
- `min_items` - Minimum number of items
- `max_items` - Maximum number of items
- `unique` - All items must be unique

### Boolean
- `const` - Exact value

## Installation

The generator is part of the nanopb project. Ensure you have the required dependencies:

```bash
pip install protobuf grpcio-tools
```

## Usage

### Command Line Interface

Basic usage:

```bash
# Generate valid data as C array for a specific message
python generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation

# Generate data for all messages in a proto file
python generator/nanopb_data_generator.py my_proto.proto --all-messages

# Generate invalid data violating a specific field
python generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --invalid --field age --rule lte

# Output as binary file
python generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --format binary -o valid_data.bin

# Output as hex string
python generator/nanopb_data_generator.py test_basic_validation.proto BasicValidation \
    --format hex

# Generate all messages and save to separate binary files
python generator/nanopb_data_generator.py my_proto.proto --all-messages \
    --format binary -o data.bin
# Creates: data_message1.bin, data_message2.bin, etc.

# With custom include paths
python generator/nanopb_data_generator.py my_proto.proto MyMessage \
    -I proto -I third_party/proto
```

#### CLI Options

- `proto_file` - Path to `.proto` file (required)
- `message` - Message name to generate data for (optional if `--all-messages` is used)
- `--all-messages` - Generate data for all messages in the proto file
- `--invalid` - Generate invalid data instead of valid
- `--field FIELD` - Field to violate (for invalid data)
- `--rule RULE` - Rule to violate (for invalid data)
- `--format {binary,c_array,hex,dict}` - Output format (default: c_array)
- `--output FILE, -o FILE` - Output file (default: stdout)
- `--seed SEED` - Random seed for reproducibility
- `-I DIR, --include DIR` - Include path for proto files

### Python API

```python
from nanopb_data_generator import DataGenerator, OutputFormat

# Create generator
generator = DataGenerator('test_basic_validation.proto', 
                         include_paths=['generator/proto'])

# List available messages
messages = generator.get_messages()
print(messages)  # ['BasicValidation']

# Generate valid data for all messages
for message_name in generator.get_messages():
    valid_data = generator.generate_valid(message_name, seed=42)
    binary_data = generator.encode_to_binary(message_name, valid_data)
    c_array = generator.format_output(binary_data, OutputFormat.C_ARRAY, 
                                     f'{message_name.lower()}_data')
    print(c_array)

# Generate valid data for a specific message
valid_data = generator.generate_valid('BasicValidation', seed=42)
print(valid_data)
# {
#   'age': 75,
#   'score': 50,
#   'username': 'john_doe',
#   'email': 'user@example.com',
#   ...
# }

# Generate invalid data (random violation)
invalid_data = generator.generate_invalid('BasicValidation')

# Generate invalid data (specific violation)
invalid_data = generator.generate_invalid('BasicValidation',
                                         violate_field='age',
                                         violate_rule='lte')
print(invalid_data['age'])  # Will be > 150 (violating the lte constraint)

# Encode to binary protobuf format
binary_data = generator.encode_to_binary('BasicValidation', valid_data)

# Format output
c_array = generator.format_output(binary_data, 
                                 OutputFormat.C_ARRAY, 
                                 'my_test_data')
print(c_array)
# const uint8_t my_test_data[] = {0x08, 0x4b, 0x10, 0x32, ...};
# const size_t my_test_data_size = 142;

hex_string = generator.format_output(binary_data, OutputFormat.HEX_STRING)
print(hex_string)  # 084b1032...
```

## Examples

### Example 1: Generate Test Vectors

```python
from nanopb_data_generator import DataGenerator, OutputFormat

generator = DataGenerator('my_message.proto')

# Generate 10 valid test cases
for i in range(10):
    data = generator.generate_valid('MyMessage', seed=i)
    binary = generator.encode_to_binary('MyMessage', data)
    c_array = generator.format_output(binary, OutputFormat.C_ARRAY, 
                                     f'test_case_{i}')
    print(c_array)
```

### Example 2: Systematic Invalid Data Testing

```python
from nanopb_data_generator import DataGenerator

generator = DataGenerator('test_basic_validation.proto')

# Get all fields with constraints
all_fields = generator.get_all_fields('BasicValidation')

# Generate invalid data for each constraint
for field_name, field_info in all_fields.items():
    for constraint in field_info.constraints:
        print(f"\nTesting violation: {field_name}.{constraint.rule_type}")
        
        invalid_data = generator.generate_invalid(
            'BasicValidation',
            violate_field=field_name,
            violate_rule=constraint.rule_type
        )
        
        binary = generator.encode_to_binary('BasicValidation', invalid_data)
        # Use this data to test your validation logic
        test_validation(binary)  # Your test function
```

### Example 3: Fuzzing Test Data

```python
import random
from nanopb_data_generator import DataGenerator

generator = DataGenerator('my_api.proto')

# Generate 1000 random test cases (mix of valid and invalid)
for i in range(1000):
    if random.random() < 0.7:
        # 70% valid data
        data = generator.generate_valid('ApiRequest', seed=i)
    else:
        # 30% invalid data
        data = generator.generate_invalid('ApiRequest', seed=i)
    
    binary = generator.encode_to_binary('ApiRequest', data)
    # Feed to your API for testing
    test_api_endpoint(binary)
```

## Output Formats

### Binary (`OutputFormat.BINARY`)
Raw protobuf binary data as bytes.

### C Array (`OutputFormat.C_ARRAY`)
```c
const uint8_t test_data[] = {0x08, 0x4b, 0x10, 0x32, 0x18, 0x01};
const size_t test_data_size = 6;
```

### Hex String (`OutputFormat.HEX_STRING`)
```
084b10321801
```

### Python Dict (`OutputFormat.PYTHON_DICT`)
```python
b'\x08K\x102\x18\x01'
```

## Validation Rules Reference

### Example Proto File

```protobuf
syntax = "proto3";

import "validate.proto";

message User {
  // Age between 18-120
  int32 age = 1 [
    (validate.rules).int32.gte = 18,
    (validate.rules).int32.lte = 120
  ];
  
  // Username 3-20 characters
  string username = 2 [
    (validate.rules).string.min_len = 3,
    (validate.rules).string.max_len = 20
  ];
  
  // Email must contain @
  string email = 3 [
    (validate.rules).string.contains = "@",
    (validate.rules).string.min_len = 5
  ];
  
  // Score must be positive
  int32 score = 4 [
    (validate.rules).int32.gt = 0
  ];
  
  // Tags array with 1-10 items
  repeated string tags = 5 [
    (validate.rules).repeated.min_items = 1,
    (validate.rules).repeated.max_items = 10
  ];
}
```

## Architecture

The data generator consists of several components:

1. **ProtoFieldInfo**: Parses field descriptors and validation constraints
2. **DataGenerator**: Main class for generating test data
3. **Value Generators**: Type-specific generators for valid/invalid values
4. **Encoder**: Encodes data to protobuf wire format
5. **Formatters**: Format binary data to various output formats

## Limitations

- Message and enum types are not yet fully supported
- Map fields have limited support
- Nested messages require manual handling
- Some complex validation rules may need custom handling

## Testing

Run the example script to see the generator in action:

```bash
cd examples/data_generation
python generate_test_data.py
```

## Integration with Nanopb

The generated test data can be used to:

1. **Test encoding/decoding**: Verify your nanopb-generated code correctly handles data
2. **Test validation**: Ensure validation code catches invalid data
3. **Fuzzing**: Feed random data to find edge cases
4. **Documentation**: Generate example data for documentation

Example integration:

```c
#include "test_basic_validation.pb.h"
#include "test_basic_validation_validate.h"
#include <pb_decode.h>

// Generated by nanopb_data_generator.py
const uint8_t invalid_age_data[] = {0x08, 0xff, 0x01, ...};
const size_t invalid_age_data_size = 142;

void test_validation() {
    BasicValidation msg = BasicValidation_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(invalid_age_data, 
                                                  invalid_age_data_size);
    
    // Decode should succeed
    assert(pb_decode(&stream, BasicValidation_fields, &msg));
    
    // But validation should fail
    assert(!pb_validate_BasicValidation(&msg));
}
```

## Contributing

Contributions are welcome! Areas for improvement:

- [ ] Support for nested messages
- [ ] Support for enum validation
- [ ] Support for map fields
- [ ] Custom validation rules
- [ ] Better error messages
- [ ] Performance optimizations

## License

This module is part of the nanopb project and follows the same license.

## See Also

- [nanopb documentation](https://jpa.kapsi.fi/nanopb/)
- [protoc-gen-validate](https://github.com/bufbuild/protoc-gen-validate)
- [validate.proto spec](https://github.com/bufbuild/protovalidate)
