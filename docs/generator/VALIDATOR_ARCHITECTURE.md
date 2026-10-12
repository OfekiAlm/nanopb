# Nanopb Validator Architecture

This document describes the internal architecture of `nanopb_validator.py`, which extends the nanopb generator to support declarative validation constraints based on the `protoc-gen-validate` specification.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Core Classes](#core-classes)
- [Validation Rules](#validation-rules)
- [Code Generation](#code-generation)
- [Integration with Generator](#integration-with-generator)
- [Development Guide](#development-guide)

## Overview

The validator is a ~1100-line Python module that:
1. Parses validation constraints from `.proto` files
2. Generates C validation functions
3. Integrates with nanopb's runtime validation system

**Key Features:**
- Field-level validation (numeric ranges, string patterns, etc.)
- Message-level validation (required fields, mutual exclusion)
- Comprehensive rule support (45+ rule types)
- Minimal runtime overhead

## Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────┐
│              Proto File with Validation              │
│                                                      │
│  message User {                                      │
│    string email = 1 [(validate.rules).string = {    │
│      email: true,                                    │
│      min_len: 5,                                     │
│      max_len: 100                                    │
│    }];                                               │
│  }                                                   │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────────┐
│            nanopb_validator.py                       │
│                                                      │
│  ValidatorGenerator                                  │
│    └─► MessageValidator                             │
│          └─► FieldValidator                         │
│                └─► ValidationRule objects           │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────────┐
│         Generated Validation Code                    │
│                                                      │
│  bool validate_User_email(const User *msg) {        │
│    if (!validate_email(msg->email)) return false;   │
│    if (strlen(msg->email) < 5) return false;        │
│    if (strlen(msg->email) > 100) return false;      │
│    return true;                                      │
│  }                                                   │
└─────────────────────────────────────────────────────┘
```

### Integration Flow

```
nanopb_generator.py
  │
  ├─► Parses .proto files
  │
  ├─► Creates ProtoFile, Message, Field objects
  │
  └─► Calls ValidatorGenerator
       │
       ├─► Extracts validation rules from fields
       │
       ├─► Creates ValidationRule objects
       │
       └─► Generates validation functions
           │
           └─► Output included in .pb.c/.pb.h
```

## Core Classes

### 1. ValidationRule

Immutable data class representing a single validation constraint.

```python
@dataclass(frozen=True)
class ValidationRule:
    """Represents a single validation rule."""
    rule_type: str        # e.g., 'REQUIRED', 'MIN_LEN', 'EMAIL'
    constraint_id: str    # Unique identifier for error reporting
    params: Dict[str, Any] = field(default_factory=dict)
```

**Examples:**
```python
# String minimum length
ValidationRule('MIN_LEN', 'string.min_len', {'value': 5})

# Numeric range
ValidationRule('GTE', 'int32.gte', {'value': 0})
ValidationRule('LTE', 'int32.lte', {'value': 100})

# Email validation
ValidationRule('EMAIL', 'string.email', {})
```

### 2. FieldValidator

Handles validation for a single field.

**Responsibilities:**
- Parse validation rules from field options
- Store rules for code generation
- Support all protobuf field types

```python
class FieldValidator:
    def __init__(self, field, rules_option, proto_file=None):
        self.field = field          # Field descriptor
        self.rules = []             # List of ValidationRule objects
        self.proto_file = proto_file
        self.parse_rules(rules_option)
```

**Rule Parsing Methods:**
- `_parse_string_rules()` - String validation (length, pattern, format)
- `_parse_numeric_rules()` - Numeric validation (range, in/not_in)
- `_parse_bool_rules()` - Boolean validation (const)
- `_parse_bytes_rules()` - Bytes validation (length, pattern)
- `_parse_enum_rules()` - Enum validation (defined values, in/not_in)
- `_parse_repeated_rules()` - Repeated field validation (count, unique)
- `_parse_message_rules()` - Submessage validation (required)
- `_parse_map_rules()` - Map validation (count, keys, values)

### 3. MessageValidator

Handles validation for an entire message.

**Responsibilities:**
- Collect field validators
- Handle message-level rules (mutex, at_least)
- Generate validation function

```python
class MessageValidator:
    def __init__(self, message, message_rules=None, proto_file=None):
        self.message = message
        self.field_validators = {}  # field_name -> FieldValidator
        self.message_rules = []     # Message-level ValidationRule objects
        self.proto_file = proto_file
```

**Message-Level Rules:**
- **Mutex**: Fields are mutually exclusive
  ```protobuf
  option (validate.message).mutex = {fields: ["field1", "field2"]};
  ```

- **At Least**: Require N fields from a set
  ```protobuf
  option (validate.message).at_least = {n: 2, fields: ["a", "b", "c"]};
  ```

### 4. ValidatorGenerator

Orchestrates validation code generation for a proto file.

**Responsibilities:**
- Manage MessageValidator objects
- Generate validation function declarations
- Generate validation function implementations
- Generate Doxygen documentation

```python
class ValidatorGenerator:
    def __init__(self, proto_file):
        self.proto_file = proto_file
        self.validators = OrderedDict()  # message_name -> MessageValidator
        self.validate_enabled = False    # File-level validation flag
```

**Key Methods:**
- `add_message_validator()` - Add validator for a message
- `generate_validation_header()` - Generate .pb.h declarations
- `generate_validation_source()` - Generate .pb.c implementations
- `generate_field_validation()` - Generate code for field rules
- `generate_message_validation()` - Generate code for message rules

## Validation Rules

### Rule Categories

#### 1. Numeric Rules (int32, int64, uint32, uint64, float, double, etc.)

- **EQ** - Equals exact value
  ```protobuf
  int32 age = 1 [(validate.rules).int32.eq = 18];
  ```

- **GT/GTE** - Greater than / Greater than or equal
  ```protobuf
  int32 age = 1 [(validate.rules).int32.gte = 0];
  ```

- **LT/LTE** - Less than / Less than or equal
  ```protobuf
  int32 score = 1 [(validate.rules).int32.lte = 100];
  ```

- **IN/NOT_IN** - Value in/not in set
  ```protobuf
  int32 status = 1 [(validate.rules).int32.in = [1, 2, 3]];
  ```

#### 2. String Rules

- **MIN_LEN/MAX_LEN** - String length bounds
  ```protobuf
  string name = 1 [(validate.rules).string = {
    min_len: 1,
    max_len: 50
  }];
  ```

- **PREFIX/SUFFIX/CONTAINS** - String patterns
  ```protobuf
  string filename = 1 [(validate.rules).string.prefix = "file_"];
  ```

- **ASCII** - Require ASCII-only characters
  ```protobuf
  string code = 1 [(validate.rules).string.ascii = true];
  ```

- **EMAIL** - Valid email address format
  ```protobuf
  string email = 1 [(validate.rules).string.email = true];
  ```

- **HOSTNAME** - Valid hostname format
  ```protobuf
  string host = 1 [(validate.rules).string.hostname = true];
  ```

- **IP/IPV4/IPV6** - IP address validation
  ```protobuf
  string addr = 1 [(validate.rules).string.ipv4 = true];
  ```

#### 3. Bytes Rules

- **MIN_LEN/MAX_LEN** - Bytes length bounds
  ```protobuf
  bytes data = 1 [(validate.rules).bytes.max_len = 1024];
  ```

- **PREFIX/SUFFIX/CONTAINS** - Bytes patterns
  ```protobuf
  bytes magic = 1 [(validate.rules).bytes.prefix = "\x89PNG"];
  ```

#### 4. Enum Rules

- **ENUM_DEFINED** - Must be defined enum value (not unknown)
  ```protobuf
  MyEnum status = 1 [(validate.rules).enum.defined_only = true];
  ```

- **IN/NOT_IN** - Enum value in/not in set
  ```protobuf
  MyEnum status = 1 [(validate.rules).enum.in = [STATUS_OK, STATUS_ERROR]];
  ```

#### 5. Repeated Field Rules

- **MIN_ITEMS/MAX_ITEMS** - Array size bounds
  ```protobuf
  repeated int32 values = 1 [(validate.rules).repeated.min_items = 1];
  ```

- **UNIQUE** - All items must be unique
  ```protobuf
  repeated string tags = 1 [(validate.rules).repeated.unique = true];
  ```

- **NO_SPARSE** - Array cannot have gaps (for proto3)
  ```protobuf
  repeated int32 ids = 1 [(validate.rules).repeated.no_sparse = true];
  ```

#### 6. Map Rules

- **MIN_PAIRS/MAX_PAIRS** - Map size bounds
  ```protobuf
  map<string, int32> counts = 1 [(validate.rules).map.min_pairs = 1];
  ```

- **Keys/Values** - Validation for map keys/values
  ```protobuf
  map<string, int32> scores = 1 [(validate.rules).map = {
    keys: {string: {min_len: 1}},
    values: {int32: {gte: 0}}
  }];
  ```

#### 7. Message Rules

- **REQUIRED** - Field must be present
  ```protobuf
  MyMessage msg = 1 [(validate.rules).message.required = true];
  ```

#### 8. Oneof Rules

- **ONEOF_REQUIRED** - At least one field in oneof must be set
  ```protobuf
  oneof result {
    option (validate.oneof).required = true;
    string success = 1;
    string error = 2;
  }
  ```

## Code Generation

### Generated Function Structure

For each message with validation rules, the generator creates:

#### 1. Validation Function Declaration (.pb.h)

```c
/**
 * @brief Validate User message
 * 
 * Validation rules:
 *  - email: valid email address, min length 5, max length 100
 *  - age: >= 18, <= 120
 * 
 * @param msg Message to validate
 * @return true if valid, false otherwise
 */
bool validate_User(const User *msg);
```

#### 2. Validation Function Implementation (.pb.c)

```c
bool validate_User(const User *msg) {
    if (!msg) return false;
    
    /* Validate email field */
    if (!validate_email_format(msg->email)) {
        return false;  /* email: must be valid email */
    }
    size_t email_len = strlen(msg->email);
    if (email_len < 5) {
        return false;  /* email: min length 5 */
    }
    if (email_len > 100) {
        return false;  /* email: max length 100 */
    }
    
    /* Validate age field */
    if (msg->age < 18) {
        return false;  /* age: >= 18 */
    }
    if (msg->age > 120) {
        return false;  /* age: <= 120 */
    }
    
    return true;
}
```

### Code Generation Strategies

#### 1. Inline Validation

Simple checks are inlined:
```c
if (msg->age < 0) return false;
if (msg->age > 100) return false;
```

#### 2. Helper Functions

Complex validations use helpers:
```c
if (!validate_email_format(msg->email)) return false;
if (!validate_ipv4_address(msg->ip_addr)) return false;
```

#### 3. Repeated Field Iteration

```c
for (pb_size_t i = 0; i < msg->values_count; i++) {
    if (msg->values[i] < 0) return false;
}
```

#### 4. Unique Check

```c
/* Check for unique items */
for (pb_size_t i = 0; i < msg->tags_count; i++) {
    for (pb_size_t j = i + 1; j < msg->tags_count; j++) {
        if (strcmp(msg->tags[i], msg->tags[j]) == 0) {
            return false;  /* tags: items must be unique */
        }
    }
}
```

### Documentation Generation

The validator generates comprehensive Doxygen comments:

```c
/**
 * @brief Validate User message
 * 
 * Validation rules applied:
 *  - email (string):
 *    - valid email address
 *    - min length 5
 *    - max length 100
 *  - age (int32):
 *    - >= 18
 *    - <= 120
 *  - tags (repeated string):
 *    - at least 1 items
 *    - at most 10 items
 *    - all items must be unique
 * 
 * @param msg Pointer to message to validate
 * @return true if all validation rules pass, false otherwise
 */
```

## Integration with Generator

### How Validator is Called

1. **Generator detects validation rules:**
   ```python
   if validate_pb2 and field.validate_rules:
       # Field has validation rules
   ```

2. **Creates ValidatorGenerator:**
   ```python
   validator_gen = ValidatorGenerator(proto_file)
   ```

3. **Adds validators for each message:**
   ```python
   for message in proto_file.messages:
       if has_validation_rules(message):
           validator_gen.add_message_validator(message, message_rules)
   ```

4. **Generates validation code:**
   ```python
   # In header generation
   validation_declarations = validator_gen.generate_validation_header()
   
   # In source generation
   validation_implementations = validator_gen.generate_validation_source()
   ```

### File-Level Control

Enable validation for entire file:
```protobuf
option (validate.file).validate = true;
```

Disable for specific message:
```protobuf
message MyMessage {
  option (validate.message).disabled = true;
  // No validation generated
}
```

## Development Guide

### Adding a New Validation Rule

1. **Add rule constant:**
   ```python
   RULE_MY_NEW_RULE = 'MY_NEW_RULE'
   ```

2. **Parse rule in FieldValidator:**
   ```python
   def _parse_string_rules(self, rules):
       if rules.HasField('my_new_rule'):
           self.rules.append(ValidationRule(
               RULE_MY_NEW_RULE,
               'string.my_new_rule',
               {'value': rules.my_new_rule}
           ))
   ```

3. **Generate validation code:**
   ```python
   def generate_field_validation(self, field_validator):
       for rule in field_validator.rules:
           if rule.rule_type == RULE_MY_NEW_RULE:
               # Generate C code for validation
               code.append(f"if (!check_my_rule(field)) return false;")
   ```

4. **Add helper function (if needed):**
   ```c
   static bool check_my_rule(const char *value) {
       // Validation logic
       return true;
   }
   ```

5. **Update documentation:**
   ```python
   def _rule_to_text(self, rule):
       if rt == RULE_MY_NEW_RULE:
           return f'my custom rule: {val}'
   ```

6. **Add test cases:**
   - Create test proto with new rule
   - Verify code generation
   - Test runtime validation

### Testing

#### Unit Tests

Test rule parsing:
```python
def test_parse_my_rule():
    field = create_test_field()
    rules = create_rules_with_my_rule()
    validator = FieldValidator(field, rules)
    assert any(r.rule_type == RULE_MY_NEW_RULE for r in validator.rules)
```

#### Integration Tests

1. Create test proto:
   ```protobuf
   message TestMessage {
     string field = 1 [(validate.rules).string.my_rule = true];
   }
   ```

2. Generate code:
   ```bash
   python generator/nanopb_generator.py test.proto
   ```

3. Verify generated validation function

4. Test at runtime:
   ```c
   TestMessage msg = {...};
   assert(validate_TestMessage(&msg));
   ```

### Common Patterns

#### Checking if Field Has Rules

```python
if hasattr(field, 'validate_rules') and field.validate_rules:
    validator = FieldValidator(field, field.validate_rules)
```

#### Generating Conditional Code

```python
if rule.rule_type == RULE_MIN_LEN:
    value = rule.params['value']
    code.append(f"if (strlen(field) < {value}) return false;")
```

#### Handling Repeated Fields

```python
if field.label == FieldDescriptor.LABEL_REPEATED:
    # Generate loop
    code.append(f"for (size_t i = 0; i < msg->{field_name}_count; i++) {{")
    code.append(f"    // Validate msg->{field_name}[i]")
    code.append("}")
```

## Performance Considerations

### Runtime Performance

- **Validation is optional** - Only called explicitly
- **Inline checks** - Minimal function call overhead
- **Early exit** - Fail fast on first violation
- **No allocations** - Stack-based validation

### Generated Code Size

- **Function per message** - Not per field
- **Shared helpers** - Common validation logic reused
- **Conditional compilation** - Can disable validation

### Optimization Tips

1. **Order rules by likelihood of failure**
   - Check common failures first
   - Expensive checks last (email, regex)

2. **Combine related checks**
   ```c
   // Instead of:
   if (x < 0) return false;
   if (x > 100) return false;
   
   // Use:
   if (x < 0 || x > 100) return false;
   ```

3. **Cache computed values**
   ```c
   size_t len = strlen(str);
   if (len < 5) return false;
   if (len > 100) return false;  // Reuse len
   ```

## Further Reading

- [GENERATOR_ARCHITECTURE.md](GENERATOR_ARCHITECTURE.md) - Main generator
- [../../ARCHITECTURE.md](../../ARCHITECTURE.md) - Overall architecture
- [../validation.md](../validation.md) - Validation usage guide
- [protoc-gen-validate spec](https://github.com/bufbuild/protoc-gen-validate) - Validation standard

## Contributing

When adding validation features:
1. Follow protoc-gen-validate specification
2. Generate efficient C code
3. Add comprehensive tests
4. Document new rules
5. Update examples

---

**Note:** This document describes nanopb 1.0.0-dev. Earlier versions may differ.
