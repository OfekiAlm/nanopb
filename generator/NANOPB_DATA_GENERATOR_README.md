# nanopb_data_generator.py - Test Data Generator

## Overview

The `nanopb_data_generator.py` script generates valid and invalid test data for protobuf messages with validation constraints. It supports all major protobuf features including oneof, Any, enums, nested messages, and comprehensive validation rules from `validate.proto`.

## Features

### Validation Constraints Supported

**Numeric Types** (int32, int64, uint32, uint64, sint32, sint64, float, double, fixed32, fixed64, sfixed32, sfixed64):
- `const_value`: Value must equal constant
- `lt`: Less than
- `lte`: Less than or equal
- `gt`: Greater than
- `gte`: Greater than or equal
- `in`: Value must be in list
- `not_in`: Value must not be in list

**String Type**:
- `const_value`: String must equal constant
- `min_len`: Minimum length in characters
- `max_len`: Maximum length in characters
- `prefix`: Must start with prefix
- `suffix`: Must end with suffix
- `contains`: Must contain substring
- `ascii`: Only ASCII characters allowed
- `email`: Must be valid email format
- `hostname`: Must be valid hostname
- `ip`: Must be valid IPv4 or IPv6
- `ipv4`: Must be valid IPv4
- `ipv6`: Must be valid IPv6
- `in`: Must be in list of strings
- `not_in`: Must not be in list of strings

**Bytes Type**:
- `const_value`: Bytes must equal constant
- `min_len`: Minimum length
- `max_len`: Maximum length

**Bool Type**:
- `const_value`: Must equal constant

**Enum Type**:
- `const_value`: Must equal specific enum value
- `defined_only`: Must be a defined enum value
- `in`: Must be in list of enum values
- `not_in`: Must not be in list of enum values

**Repeated Fields**:
- `min_items`: Minimum number of items
- `max_items`: Maximum number of items
- `unique`: All items must be unique

**Any Type** (google.protobuf.Any):
- `in`: type_url must be in list
- `not_in`: type_url must not be in list

### Special Features

**Oneof Support**:
- Enforces mutual exclusivity (exactly one field set)
- Supports deterministic field selection
- Validates constraints on selected field

**Nested Messages**:
- Recursive generation
- Full constraint validation on nested fields

**Deterministic Mode**:
- Seed parameter for reproducible output
- Same seed always produces identical results

**Binary Encoding**:
- Protobuf wire format encoding
- Supports all field types
- Deterministic encoding

## Usage

### Basic Usage

```python
from nanopb_data_generator import DataGenerator

# Create generator
gen = DataGenerator('my_proto.proto')

# Generate valid data
data = gen.generate_valid('MyMessage', seed=42)
print(data)

# Encode to binary
binary = gen.encode_to_binary('MyMessage', data)

# Format as C array
c_array = gen.format_output(binary, OutputFormat.C_ARRAY, 'test_data')
print(c_array)
```

### Generate Invalid Data

```python
# Violate random constraint on random field
invalid_data = gen.generate_invalid('MyMessage', seed=42)

# Violate specific field
invalid_data = gen.generate_invalid('MyMessage', violate_field='age')

# Violate specific rule
invalid_data = gen.generate_invalid('MyMessage', 
                                   violate_field='email', 
                                   violate_rule='email')

# Violate multiple fields
invalid_data = gen.generate_invalid('MyMessage',
                                   violate_field=['age', 'email'])
```

### Oneof with Custom Selection

```python
# Random selection (default)
data = gen.generate_valid('OneofMessage', seed=42)

# Deterministic selection
data = gen.generate_valid('OneofMessage', 
                         oneof_choice={'my_oneof': 'str_option'})
```

### Any Type with Registry

```python
# Define type registry (maps type_url to message name)
registry = {
    'type.googleapis.com/MyMessage': 'MyMessage',
    'type.googleapis.com/OtherMessage': 'OtherMessage'
}

gen = DataGenerator('proto_with_any.proto', any_type_registry=registry)
data = gen.generate_valid('ContainerMessage')
```

### Command-Line Usage

```bash
# Generate valid data
python3 nanopb_data_generator.py my.proto MyMessage --seed 42

# Generate invalid data
python3 nanopb_data_generator.py my.proto MyMessage --invalid --field age

# Output as C array
python3 nanopb_data_generator.py my.proto MyMessage --format c_array -o output.h

# Output as hex string
python3 nanopb_data_generator.py my.proto MyMessage --format hex
```

## API Reference

### DataGenerator Class

#### Constructor
```python
DataGenerator(proto_file: str, 
             include_paths: Optional[List[str]] = None,
             any_type_registry: Optional[Dict[str, str]] = None)
```

**Parameters**:
- `proto_file`: Path to .proto file
- `include_paths`: Additional include paths for protoc
- `any_type_registry`: Dict mapping type_url to message name for Any fields

#### Methods

**generate_valid()**
```python
generate_valid(message_name: str, 
              seed: Optional[int] = None,
              oneof_choice: Optional[Dict[str, str]] = None) -> Dict[str, Any]
```

Generate valid test data for a message.

**Parameters**:
- `message_name`: Name of the message type
- `seed`: Random seed for reproducibility
- `oneof_choice`: Dict mapping oneof name to chosen field name

**Returns**: Dictionary of field values

**generate_invalid()**
```python
generate_invalid(message_name: str,
                violate_field: Optional[Union[str, List[str]]] = None,
                violate_rule: Optional[Union[str, List[str]]] = None,
                seed: Optional[int] = None) -> Dict[str, Any]
```

Generate invalid test data for a message.

**Parameters**:
- `message_name`: Name of the message type
- `violate_field`: Field(s) to make invalid (comma-separated or list)
- `violate_rule`: Rule(s) to violate (comma-separated or list)
- `seed`: Random seed for reproducibility

**Returns**: Dictionary of field values with violations

**encode_to_binary()**
```python
encode_to_binary(message_name: str, data: Dict[str, Any]) -> bytes
```

Encode data dictionary to protobuf binary format.

**format_output()**
```python
format_output(data: bytes,
             format_type: OutputFormat = OutputFormat.C_ARRAY,
             name: str = "test_data") -> str
```

Format binary data for output.

**Parameters**:
- `data`: Binary protobuf data
- `format_type`: Desired output format (BINARY, C_ARRAY, HEX_STRING, PYTHON_DICT)
- `name`: Variable name for C arrays

**Returns**: Formatted string

### OutputFormat Enum

- `BINARY`: Raw bytes
- `C_ARRAY`: C array initializer
- `HEX_STRING`: Hexadecimal string
- `PYTHON_DICT`: Python dictionary representation

## Testing

### Running Tests

```bash
cd /path/to/nanopb
python3 generator/test_nanopb_data_generator.py
```

### Test Coverage

The test suite includes 33 tests covering:
- Numeric constraints (10 tests)
- String constraints (11 tests)
- Enum constraints (3 tests)
- Oneof behavior (4 tests)
- Constraint parsing (3 tests)
- Binary encoding (2 tests)

All tests pass with 100% success rate.

## Troubleshooting

### Constraint Not Working

**Problem**: Generated data doesn't respect constraints.

**Solution**: Ensure `validate.proto` is accessible and constraints are properly defined in your .proto file. Check that field uses correct syntax:

```protobuf
int32 age = 1 [(validate.rules).int32.gte = 0, (validate.rules).int32.lte = 120];
```

### Oneof Not Mutually Exclusive

**Problem**: Multiple oneof fields are set.

**Solution**: Ensure proto file correctly defines oneof:

```protobuf
message MyMessage {
  oneof choice {
    string str_option = 1;
    int32 int_option = 2;
  }
}
```

### Any Type Not Working

**Problem**: Any field generates empty data.

**Solution**: Provide type registry to DataGenerator:

```python
registry = {'type.googleapis.com/MyMessage': 'MyMessage'}
gen = DataGenerator('my.proto', any_type_registry=registry)
```

### Import Error: validate_pb2 not found

**Problem**: Cannot import validate_pb2.

**Solution**: The generator automatically builds validate_pb2.py on first use. Ensure protoc is installed and accessible in PATH:

```bash
pip install grpcio-tools
```

## Architecture

The generator follows a clean architecture:

1. **Proto Loading**: Compiles .proto file and extracts descriptors
2. **Constraint Parsing**: Reads validate.proto rules from field options
3. **Value Generation**: Type-specific generators for each field type
4. **Binary Encoding**: Protobuf wire format encoder
5. **Output Formatting**: Multiple output format support

## Limitations

- Map fields are treated as repeated fields (if supported by nanopb)
- Recursion depth is not limited (can cause issues with circular references)
- Float NaN/Inf handling follows Python defaults
- Timestamp/Duration well-known types not specially handled
- Custom validation functions not supported

## Contributing

When adding new features:
1. Add constraint parsing in `ProtoFieldInfo._parse_*_rules()`
2. Add value generation in `DataGenerator._generate_valid_*()`
3. Add invalid value generation in `DataGenerator._generate_invalid_value()`
4. Add wire encoding if needed in `DataGenerator._encode_single_field()`
5. Add tests in `test_nanopb_data_generator.py`

## License

This script is part of the nanopb project and follows the same license (Zlib).
