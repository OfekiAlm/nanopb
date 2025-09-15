#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb validation support module.

This module extends the nanopb generator to support declarative validation
constraints via custom options defined in validate.proto.
"""
import os
import sys
import re
from collections import OrderedDict

# Import the validate_pb2 module (generated from validate.proto)
# This will be loaded dynamically like nanopb_pb2
validate_pb2 = None

def load_validate_pb2():
    """Load the compiled validate.proto definitions."""
    global validate_pb2
    if validate_pb2 is not None:
        return validate_pb2
    
    try:
        # Add current directory to path for imports
        current_dir = os.path.dirname(__file__)
        if current_dir not in sys.path:
            sys.path.insert(0, current_dir)
        
        # Try different import paths for validate_pb2
        try:
            # Try from generator/proto/ directory  
            sys.path.insert(0, os.path.join(current_dir, 'proto'))
            import validate_pb2 as validate_module
            validate_pb2 = validate_module
            return validate_pb2
        except ImportError:
            pass
            
        try:
            # Try from proto subdirectory
            from proto import validate_pb2 as validate_module
            validate_pb2 = validate_module
            return validate_pb2
        except ImportError:
            pass
            
        # If all imports fail, return None
        sys.stderr.write("Could not import validate_pb2 module.\n")
        return None
            
    except Exception as e:
        sys.stderr.write("Failed to load validate.proto: %s\n" % str(e))
        sys.stderr.write("Validation support will be disabled.\n")
        return None

def parse_validation_rules_from_unknown_fields(field_options):
    """Parse validation rules from unknown fields in field options.
    
    This is a fallback when the validate_pb2 extensions can't be properly loaded.
    We manually parse the wire format data from unknown fields.
    """
    rules = {}
    
    # Extension number 1011 is used for validate.rules
    # In wire format, field 1011 would be encoded as: (1011 << 3) | 2 = 8090 (for length-delimited)
    # But protobuf stores unknown fields differently, let's check for the tag
    
    if hasattr(field_options, '_unknown_fields'):
        for unknown_field in field_options._unknown_fields:
            # unknown_field is typically a tuple (field_number, value)
            if len(unknown_field) >= 2:
                field_number = unknown_field[0]
                value = unknown_field[1]
                
                # Check if this is the validate.rules extension (field 1011)
                if field_number == 1011:
                    # Try to parse the FieldRules message from the wire format value
                    try:
                        parsed_rules = parse_field_rules_from_bytes(value)
                        rules.update(parsed_rules)
                    except Exception as e:
                        # If parsing fails, try to extract basic info
                        pass
    
    # Also check ListFields() which might contain extension data
    if hasattr(field_options, 'ListFields'):
        try:
            for field_desc, value in field_options.ListFields():
                if hasattr(field_desc, 'number') and field_desc.number == 1011:
                    # This is our validation rules extension
                    parsed_rules = parse_field_rules_from_protobuf_message(value)
                    rules.update(parsed_rules)
        except Exception:
            pass
    
    return rules

def parse_field_rules_from_protobuf_message(field_rules_msg):
    """Parse FieldRules from a protobuf message object."""
    rules = {}
    
    if not field_rules_msg:
        return rules
    
    # Try to extract rules from the message
    try:
        # Check for string rules
        if hasattr(field_rules_msg, 'string') and field_rules_msg.HasField('string'):
            string_rules = field_rules_msg.string
            rules['string'] = {}
            if string_rules.HasField('min_len'):
                rules['string']['min_len'] = string_rules.min_len
            if string_rules.HasField('max_len'):
                rules['string']['max_len'] = string_rules.max_len
            if string_rules.HasField('const'):
                rules['string']['const'] = string_rules.const
            if string_rules.HasField('prefix'):
                rules['string']['prefix'] = string_rules.prefix
            if string_rules.HasField('suffix'):
                rules['string']['suffix'] = string_rules.suffix
            if string_rules.HasField('contains'):
                rules['string']['contains'] = string_rules.contains
            if string_rules.HasField('ascii'):
                rules['string']['ascii'] = string_rules.ascii
        
        # Check for int32 rules
        if hasattr(field_rules_msg, 'int32') and field_rules_msg.HasField('int32'):
            int32_rules = field_rules_msg.int32
            rules['int32'] = {}
            if int32_rules.HasField('gt'):
                rules['int32']['gt'] = int32_rules.gt
            if int32_rules.HasField('gte'):
                rules['int32']['gte'] = int32_rules.gte
            if int32_rules.HasField('lt'):
                rules['int32']['lt'] = int32_rules.lt
            if int32_rules.HasField('lte'):
                rules['int32']['lte'] = int32_rules.lte
            if int32_rules.HasField('const'):
                rules['int32']['const'] = int32_rules.const
        
        # Check for other numeric types (similar pattern)
        for numeric_type in ['int64', 'uint32', 'uint64', 'sint32', 'sint64', 
                           'fixed32', 'fixed64', 'sfixed32', 'sfixed64', 'float', 'double']:
            if hasattr(field_rules_msg, numeric_type) and field_rules_msg.HasField(numeric_type):
                numeric_rules = getattr(field_rules_msg, numeric_type)
                rules[numeric_type] = {}
                for constraint in ['gt', 'gte', 'lt', 'lte', 'const']:
                    if numeric_rules.HasField(constraint):
                        rules[numeric_type][constraint] = getattr(numeric_rules, constraint)
        
        # Check for bool rules
        if hasattr(field_rules_msg, 'bool') and field_rules_msg.HasField('bool'):
            bool_rules = field_rules_msg.bool
            rules['bool'] = {}
            if bool_rules.HasField('const'):
                rules['bool']['const'] = bool_rules.const
        
        # Check for bytes rules
        if hasattr(field_rules_msg, 'bytes') and field_rules_msg.HasField('bytes'):
            bytes_rules = field_rules_msg.bytes
            rules['bytes'] = {}
            if bytes_rules.HasField('min_len'):
                rules['bytes']['min_len'] = bytes_rules.min_len
            if bytes_rules.HasField('max_len'):
                rules['bytes']['max_len'] = bytes_rules.max_len
            if bytes_rules.HasField('const'):
                rules['bytes']['const'] = bytes_rules.const
            if bytes_rules.HasField('prefix'):
                rules['bytes']['prefix'] = bytes_rules.prefix
            if bytes_rules.HasField('suffix'):
                rules['bytes']['suffix'] = bytes_rules.suffix
            if bytes_rules.HasField('contains'):
                rules['bytes']['contains'] = bytes_rules.contains
        
        # Check for required field
        if hasattr(field_rules_msg, 'required') and field_rules_msg.HasField('required'):
            rules['required'] = field_rules_msg.required
            
        # Check for oneof_required field
        if hasattr(field_rules_msg, 'oneof_required') and field_rules_msg.HasField('oneof_required'):
            rules['oneof_required'] = field_rules_msg.oneof_required
        
    except Exception as e:
        # If parsing fails, return what we have
        pass
    
    return rules

def parse_field_rules_from_bytes(data):
    """Parse FieldRules from raw bytes (wire format)."""
    # This is more complex and would require implementing protobuf wire format parsing
    # For now, return empty rules
    return {}

def parse_string_rules(data):
    """Parse StringRules from wire format data."""
    rules = {}
    i = 0
    
    while i < len(data):
        if i >= len(data):
            break
            
        tag_byte = data[i]
        i += 1
        
        field_num = tag_byte >> 3
        wire_type = tag_byte & 0x07
        
        if wire_type == 0:  # Varint
            val = 0
            shift = 0
            while i < len(data):
                byte = data[i]
                i += 1
                val |= (byte & 0x7F) << shift
                if (byte & 0x80) == 0:
                    break
                shift += 7
            
            if field_num == 2:  # min_len
                rules['min_len'] = val
            elif field_num == 3:  # max_len
                rules['max_len'] = val
        else:
            # Skip other wire types for now
            break
    
    return rules

class ValidationRule:
    """Represents a single validation rule."""
    def __init__(self, rule_type, constraint_id, params=None):
        self.rule_type = rule_type
        self.constraint_id = constraint_id
        self.params = params or {}
    
    def __repr__(self):
        return f"ValidationRule({self.rule_type}, {self.constraint_id}, {self.params})"

class FieldValidator:
    """Handles validation for a single field."""
    def __init__(self, field, rules_option, proto_file=None, message_desc=None):
        self.field = field
        self.rules = []
        self.proto_file = proto_file
        self.message_desc = message_desc
        self.parse_rules(rules_option)
    
    def parse_rules(self, rules_option):
        """Parse validation rules from the FieldRules option."""
        if not rules_option:
            # Try to parse from unknown fields as fallback
            if self.proto_file and self.message_desc and hasattr(self.message_desc, 'desc'):
                # Find the field descriptor for this field
                for field_desc in self.message_desc.desc.field:
                    if field_desc.name == self.field.name:
                        parsed_rules = parse_validation_rules_from_unknown_fields(field_desc.options)
                        self._add_parsed_rules(parsed_rules)
                        break
            return
            
        # Handle proper FieldRules when extensions are working
        validate_pb2 = load_validate_pb2()
        if not validate_pb2:
            return
            
        # Debug: Print what fields are available
        # print(f"Debug: Parsing rules for field {self.field.name}, rules_option type: {type(rules_option)}")
        # print(f"Debug: Available rule fields: {[f.name for f in rules_option.DESCRIPTOR.fields]}")
        
        # Parse rules based on field type - check all possible types
        parsed_any = False
        if rules_option.HasField('string'):
            self._parse_string_rules(rules_option.string)
            parsed_any = True
        if rules_option.HasField('int32'):
            self._parse_numeric_rules(rules_option.int32, 'int32')
            parsed_any = True
        if rules_option.HasField('int64'):
            self._parse_numeric_rules(rules_option.int64, 'int64')
            parsed_any = True
        if rules_option.HasField('uint32'):
            self._parse_numeric_rules(rules_option.uint32, 'uint32')
            parsed_any = True
        if rules_option.HasField('uint64'):
            self._parse_numeric_rules(rules_option.uint64, 'uint64')
            parsed_any = True
        if rules_option.HasField('sint32'):
            self._parse_numeric_rules(rules_option.sint32, 'sint32')
            parsed_any = True
        if rules_option.HasField('sint64'):
            self._parse_numeric_rules(rules_option.sint64, 'sint64')
            parsed_any = True
        if rules_option.HasField('fixed32'):
            self._parse_numeric_rules(rules_option.fixed32, 'fixed32')
            parsed_any = True
        if rules_option.HasField('fixed64'):
            self._parse_numeric_rules(rules_option.fixed64, 'fixed64')
            parsed_any = True
        if rules_option.HasField('sfixed32'):
            self._parse_numeric_rules(rules_option.sfixed32, 'sfixed32')
            parsed_any = True
        if rules_option.HasField('sfixed64'):
            self._parse_numeric_rules(rules_option.sfixed64, 'sfixed64')
            parsed_any = True
        if rules_option.HasField('double'):
            self._parse_numeric_rules(rules_option.double, 'double')
            parsed_any = True
        if rules_option.HasField('float'):
            self._parse_numeric_rules(rules_option.float, 'float')
            parsed_any = True
        if rules_option.HasField('bool'):
            self._parse_bool_rules(rules_option.bool)
            parsed_any = True
        if rules_option.HasField('bytes'):
            self._parse_bytes_rules(rules_option.bytes)
            parsed_any = True
        if rules_option.HasField('enum'):
            self._parse_enum_rules(rules_option.enum)
            parsed_any = True
        if rules_option.HasField('repeated'):
            self._parse_repeated_rules(rules_option.repeated)
            parsed_any = True
        if rules_option.HasField('map'):
            self._parse_map_rules(rules_option.map)
            parsed_any = True
        
        # Debug output if no specific type rules were found
        if not parsed_any:
            print(f"Warning: No type-specific rules found for field {self.field.name}")
            for field in rules_option.DESCRIPTOR.fields:
                if rules_option.HasField(field.name):
                    print(f"  Found rule type: {field.name}")
        
        # Check for required constraint
        if rules_option.HasField('required') and rules_option.required:
            self.rules.append(ValidationRule('REQUIRED', 'required'))
        
        # Check for oneof_required constraint
        if rules_option.HasField('oneof_required') and rules_option.oneof_required:
            self.rules.append(ValidationRule('ONEOF_REQUIRED', 'oneof_required'))
    
    def _add_parsed_rules(self, parsed_rules):
        """Add parsed rules to the validator."""
        for rule_type, rule_data in parsed_rules.items():
            if rule_type == 'string' and isinstance(rule_data, dict):
                for constraint, value in rule_data.items():
                    self.rules.append(ValidationRule('STRING_' + constraint.upper(), 
                                                   f'string.{constraint}', 
                                                   {'value': value}))
            elif rule_type == 'required':
                self.rules.append(ValidationRule('REQUIRED', 'required', {'required': rule_data}))
    
    def _parse_numeric_rules(self, rules, type_name):
        """Parse numeric validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule('EQ', f'{type_name}.const', {'value': rules.const}))
        if rules.HasField('lt'):
            self.rules.append(ValidationRule('LT', f'{type_name}.lt', {'value': rules.lt}))
        if rules.HasField('lte'):
            self.rules.append(ValidationRule('LTE', f'{type_name}.lte', {'value': rules.lte}))
        if rules.HasField('gt'):
            self.rules.append(ValidationRule('GT', f'{type_name}.gt', {'value': rules.gt}))
        if rules.HasField('gte'):
            self.rules.append(ValidationRule('GTE', f'{type_name}.gte', {'value': rules.gte}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule('IN', f'{type_name}.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule('NOT_IN', f'{type_name}.not_in', {'values': list(rules.not_in)}))
    
    def _parse_bool_rules(self, rules):
        """Parse bool validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule('EQ', 'bool.const', {'value': rules.const}))
    
    def _parse_string_rules(self, rules):
        """Parse string validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule('EQ', 'string.const', {'value': rules.const}))
        if rules.HasField('min_len'):
            self.rules.append(ValidationRule('MIN_LEN', 'string.min_len', {'value': rules.min_len}))
        if rules.HasField('max_len'):
            self.rules.append(ValidationRule('MAX_LEN', 'string.max_len', {'value': rules.max_len}))
        if rules.HasField('prefix'):
            self.rules.append(ValidationRule('PREFIX', 'string.prefix', {'value': rules.prefix}))
        if rules.HasField('suffix'):
            self.rules.append(ValidationRule('SUFFIX', 'string.suffix', {'value': rules.suffix}))
        if rules.HasField('contains'):
            self.rules.append(ValidationRule('CONTAINS', 'string.contains', {'value': rules.contains}))
        if rules.HasField('ascii') and rules.ascii:
            self.rules.append(ValidationRule('ASCII', 'string.ascii'))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule('IN', 'string.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule('NOT_IN', 'string.not_in', {'values': list(rules.not_in)}))
    
    def _parse_bytes_rules(self, rules):
        """Parse bytes validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule('EQ', 'bytes.const', {'value': rules.const}))
        if rules.HasField('min_len'):
            self.rules.append(ValidationRule('MIN_LEN', 'bytes.min_len', {'value': rules.min_len}))
        if rules.HasField('max_len'):
            self.rules.append(ValidationRule('MAX_LEN', 'bytes.max_len', {'value': rules.max_len}))
        if rules.HasField('prefix'):
            self.rules.append(ValidationRule('PREFIX', 'bytes.prefix', {'value': rules.prefix}))
        if rules.HasField('suffix'):
            self.rules.append(ValidationRule('SUFFIX', 'bytes.suffix', {'value': rules.suffix}))
        if rules.HasField('contains'):
            self.rules.append(ValidationRule('CONTAINS', 'bytes.contains', {'value': rules.contains}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule('IN', 'bytes.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule('NOT_IN', 'bytes.not_in', {'values': list(rules.not_in)}))
    
    def _parse_enum_rules(self, rules):
        """Parse enum validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule('EQ', 'enum.const', {'value': rules.const}))
        if rules.HasField('defined_only'):
            self.rules.append(ValidationRule('ENUM_DEFINED', 'enum.defined_only', {'value': rules.defined_only}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule('IN', 'enum.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule('NOT_IN', 'enum.not_in', {'values': list(rules.not_in)}))
    
    def _parse_repeated_rules(self, rules):
        """Parse repeated field validation rules."""
        if rules.HasField('min_items'):
            self.rules.append(ValidationRule('MIN_ITEMS', 'repeated.min_items', {'value': rules.min_items}))
        if rules.HasField('max_items'):
            self.rules.append(ValidationRule('MAX_ITEMS', 'repeated.max_items', {'value': rules.max_items}))
        if rules.HasField('unique') and rules.unique:
            self.rules.append(ValidationRule('UNIQUE', 'repeated.unique'))
        if rules.HasField('items'):
            # TODO: Handle per-item validation rules
            pass
    
    def _parse_map_rules(self, rules):
        """Parse map field validation rules."""
        if rules.HasField('min_pairs'):
            self.rules.append(ValidationRule('MIN_ITEMS', 'map.min_pairs', {'value': rules.min_pairs}))
        if rules.HasField('max_pairs'):
            self.rules.append(ValidationRule('MAX_ITEMS', 'map.max_pairs', {'value': rules.max_pairs}))
        if rules.HasField('no_sparse') and rules.no_sparse:
            self.rules.append(ValidationRule('NO_SPARSE', 'map.no_sparse'))
        # TODO: Handle key/value validation rules

class MessageValidator:
    """Handles validation for a message."""
    def __init__(self, message, message_rules=None, proto_file=None):
        self.message = message
        self.field_validators = OrderedDict()
        self.message_rules = []
        self.proto_file = proto_file
        
        # Parse field-level rules
        for field in message.fields:
            field_validator = None
            
            # First try to use existing validate_rules from extensions
            if hasattr(field, 'validate_rules') and field.validate_rules:
                field_validator = FieldValidator(field, field.validate_rules, proto_file, message)
            else:
                # Fallback: try to parse from proto descriptor 
                # This approach works even when extensions aren't properly registered
                field_validator = FieldValidator(field, None, proto_file, message)
                
                # If field_validator has rules after parsing, use it
                if not field_validator.rules:
                    field_validator = None
            
            if field_validator:
                self.field_validators[field.name] = field_validator
        
        # Parse message-level rules
        if message_rules:
            self.parse_message_rules(message_rules)
    
    def parse_message_rules(self, rules):
        """Parse message-level validation rules."""
        if rules.requires:
            for field_name in rules.requires:
                self.message_rules.append(ValidationRule('REQUIRES', 'message.requires', {'field': field_name}))
        
        if rules.mutex:
            for group in rules.mutex:
                self.message_rules.append(ValidationRule('MUTEX', 'message.mutex', {'fields': list(group.fields)}))
        
        if rules.at_least:
            for rule in rules.at_least:
                self.message_rules.append(ValidationRule('AT_LEAST', 'message.at_least', 
                                                       {'n': rule.n, 'fields': list(rule.fields)}))

class ValidatorGenerator:
    """Generates validation code for messages."""
    def __init__(self, proto_file):
        self.proto_file = proto_file
        self.validators = OrderedDict()
        
        # Check if validation is enabled
        self.validate_enabled = False
        if hasattr(proto_file, 'file_options') and hasattr(proto_file.file_options, 'validate'):
            self.validate_enabled = proto_file.file_options.validate
    
    def add_message_validator(self, message, message_rules=None):
        """Add a validator for a message."""
        validator = MessageValidator(message, message_rules, self.proto_file)
        if validator.field_validators or validator.message_rules:
            self.validators[str(message.name)] = validator
    
    def force_add_message_validator(self, message, message_rules=None):
        """Force add a validator for a message, even if no rules are present."""
        validator = MessageValidator(message, message_rules, self.proto_file)
        self.validators[str(message.name)] = validator
    
    def generate_header(self):
        """Generate validation header file content."""
        if not self.validators:
            return
        
        # Extract base name without .proto extension
        base_name = self.proto_file.fdesc.name
        if base_name.endswith('.proto'):
            base_name = base_name[:-6]
        
        guard = 'PB_VALIDATE_%s_INCLUDED' % base_name.upper().replace('.', '_').replace('/', '_').replace('-', '_')
        
        yield '/* Validation header for %s */\n' % self.proto_file.fdesc.name
        yield '#ifndef %s\n' % guard
        yield '#define %s\n' % guard
        yield '\n'
        yield '#include <pb_validate.h>\n'
        yield '#include "%s.pb.h"\n' % base_name
        yield '\n'
        yield '#ifdef __cplusplus\n'
        yield 'extern "C" {\n'
        yield '#endif\n'
        yield '\n'
        
        # Generate validation function declarations
        for msg_name, validator in self.validators.items():
            # Use the C struct name from the message
            struct_name = str(validator.message.name)
            func_name = 'pb_validate_' + struct_name.replace('.', '_')
            yield '/* Validate %s message */\n' % struct_name
            yield 'bool %s(const %s *msg, pb_violations_t *violations);\n' % (func_name, struct_name)
            yield '\n'
        
        yield '#ifdef __cplusplus\n'
        yield '} /* extern "C" */\n'
        yield '#endif\n'
        yield '\n'
        yield '#endif /* %s */\n' % guard
    
    def generate_source(self):
        """Generate validation source file content."""
        if not self.validators:
            return
        
        # Extract base name without .proto extension
        base_name = self.proto_file.fdesc.name
        if base_name.endswith('.proto'):
            base_name = base_name[:-6]
        
        yield '/* Validation implementation for %s */\n' % self.proto_file.fdesc.name
        yield '#include "%s_validate.h"\n' % base_name
        yield '#include <string.h>\n'
        yield '\n'
        
        # Generate static rule data
        for msg_name, validator in self.validators.items():
            # Use the C struct name from the message
            struct_name = str(validator.message.name)
            func_name = 'pb_validate_' + struct_name.replace('.', '_')
            
            # Generate validation function
            yield 'bool %s(const %s *msg, pb_violations_t *violations)\n' % (func_name, struct_name)
            yield '{\n'
            yield '    if (!msg) return false;\n'
            yield '    \n'
            yield '    pb_validate_context_t ctx = {0};\n'
            yield '    ctx.violations = violations;\n'
            yield '    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;\n'
            yield '    \n'
            
            # Generate field validations
            for field_name, field_validator in validator.field_validators.items():
                field = field_validator.field
                yield '    /* Validate field: %s */\n' % field_name
                yield '    if (!pb_validate_context_push_field(&ctx, "%s")) return false;\n' % field_name
                
                for rule in field_validator.rules:
                    yield '    {\n'
                    yield '        /* Rule: %s */\n' % rule.constraint_id
                    yield self._generate_rule_check(field, rule)
                    yield '    }\n'
                
                yield '    pb_validate_context_pop_field(&ctx);\n'
                yield '    \n'
            
            # Generate message-level validations
            for rule in validator.message_rules:
                yield '    /* Message rule: %s */\n' % rule.constraint_id
                yield self._generate_message_rule_check(validator.message, rule)
                yield '    \n'
            
            yield '    return !pb_violations_has_any(violations);\n'
            yield '}\n'
            yield '\n'
    
    def _generate_rule_check(self, field, rule):
        """Generate code for a single field validation rule."""
        field_name = field.name
        
        if rule.rule_type == 'REQUIRED':
            if field.rules == 'OPTIONAL':
                return '        if (!msg->has_%s) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Field is required");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, rule.constraint_id)
        
        # Numeric constraint implementations (GT, LT, GTE, LTE, EQ)
        elif rule.rule_type == 'GT':
            value = rule.params.get('value', 0)
            return '        if (!(msg->%s > %s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be greater than %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, value, rule.constraint_id, value)
        
        elif rule.rule_type == 'LT':
            value = rule.params.get('value', 0)
            return '        if (!(msg->%s < %s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be less than %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, value, rule.constraint_id, value)
        
        elif rule.rule_type == 'GTE':
            value = rule.params.get('value', 0)
            return '        if (!(msg->%s >= %s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be greater than or equal to %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, value, rule.constraint_id, value)
        
        elif rule.rule_type == 'LTE':
            value = rule.params.get('value', 0)
            return '        if (!(msg->%s <= %s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be less than or equal to %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, value, rule.constraint_id, value)
        
        elif rule.rule_type == 'EQ':
            value = rule.params.get('value', 0)
            if 'string' in rule.constraint_id:
                return '        if (strcmp(msg->%s, "%s") != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must equal \'%s\'");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, value, rule.constraint_id, value)
            elif 'bool' in rule.constraint_id:
                bool_val = 'true' if value else 'false'
                return '        if (msg->%s != %s) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, bool_val, rule.constraint_id, bool_val)
            else:
                return '        if (msg->%s != %s) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must equal %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, value, rule.constraint_id, value)
        
        elif rule.rule_type == 'IN':
            values = rule.params.get('values', [])
            if 'string' in rule.constraint_id:
                conditions = ['strcmp(msg->%s, "%s") == 0' % (field_name, v) for v in values]
                condition_str = ' || '.join(conditions)
                values_str = ', '.join('"%s"' % v for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
            else:
                conditions = ['msg->%s == %s' % (field_name, v) for v in values]
                condition_str = ' || '.join(conditions)
                values_str = ', '.join(str(v) for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
        
        elif rule.rule_type == 'NOT_IN':
            values = rule.params.get('values', [])
            if 'string' in rule.constraint_id:
                conditions = ['strcmp(msg->%s, "%s") != 0' % (field_name, v) for v in values]
                condition_str = ' && '.join(conditions)
                values_str = ', '.join('"%s"' % v for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
            else:
                conditions = ['msg->%s != %s' % (field_name, v) for v in values]
                condition_str = ' && '.join(conditions)
                values_str = ', '.join(str(v) for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
        
        # String-specific constraints
        elif rule.rule_type == 'MIN_LEN':
            min_len = rule.params.get('value', 0)
            if 'string' in rule.constraint_id:
                return '        if (strlen(msg->%s) < %d) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "String too short");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, min_len, rule.constraint_id)
            else:  # bytes
                return '        if (msg->%s.size < %d) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too short");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, min_len, rule.constraint_id)
        
        elif rule.rule_type == 'MAX_LEN':
            max_len = rule.params.get('value', 0)
            if 'string' in rule.constraint_id:
                return '        if (strlen(msg->%s) > %d) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "String too long");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, max_len, rule.constraint_id)
            else:  # bytes
                return '        if (msg->%s.size > %d) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too long");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, max_len, rule.constraint_id)
        
        elif rule.rule_type == 'PREFIX':
            prefix = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                return '        if (strncmp(msg->%s, "%s", %d) != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "String must start with \'%s\'");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, prefix, len(prefix), rule.constraint_id, prefix)
            else:  # bytes
                return '        if (msg->%s.size < %d || memcmp(msg->%s.bytes, "%s", %d) != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must start with specified prefix");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, len(prefix), field_name, prefix, len(prefix), rule.constraint_id)
        
        elif rule.rule_type == 'SUFFIX':
            suffix = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                return '        {\n' \
                       '            size_t len = strlen(msg->%s);\n' \
                       '            if (len < %d || strcmp(msg->%s + len - %d, "%s") != 0) {\n' \
                       '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must end with \'%s\'");\n' \
                       '                if (ctx.early_exit) return false;\n' \
                       '            }\n' \
                       '        }\n' % (field_name, len(suffix), field_name, len(suffix), suffix, rule.constraint_id, suffix)
            else:  # bytes
                return '        if (msg->%s.size < %d || memcmp(msg->%s.bytes + msg->%s.size - %d, "%s", %d) != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must end with specified suffix");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, len(suffix), field_name, field_name, len(suffix), suffix, len(suffix), rule.constraint_id)
        
        elif rule.rule_type == 'CONTAINS':
            contains = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                return '        if (strstr(msg->%s, "%s") == NULL) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain \'%s\'");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, contains, rule.constraint_id, contains)
            else:  # bytes  
                return '        {\n' \
                       '            bool found = false;\n' \
                       '            for (size_t i = 0; i <= msg->%s.size - %d; i++) {\n' \
                       '                if (memcmp(msg->%s.bytes + i, "%s", %d) == 0) {\n' \
                       '                    found = true;\n' \
                       '                    break;\n' \
                       '                }\n' \
                       '            }\n' \
                       '            if (!found) {\n' \
                       '                pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must contain specified sequence");\n' \
                       '                if (ctx.early_exit) return false;\n' \
                       '            }\n' \
                       '        }\n' % (field_name, len(contains), field_name, contains, len(contains), rule.constraint_id)
        
        elif rule.rule_type == 'ASCII':
            return '        {\n' \
                   '            for (size_t i = 0; msg->%s[i] != \'\\0\'; i++) {\n' \
                   '                if ((unsigned char)msg->%s[i] > 127) {\n' \
                   '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain only ASCII characters");\n' \
                   '                    if (ctx.early_exit) return false;\n' \
                   '                    break;\n' \
                   '                }\n' \
                   '            }\n' \
                   '        }\n' % (field_name, field_name, rule.constraint_id)
        
        # Enum constraints
        elif rule.rule_type == 'ENUM_DEFINED':
            # This would require enum definition lookup, simplified for now
            return '        /* TODO: Implement enum defined_only validation */\n'
        
        # Repeated field constraints
        elif rule.rule_type == 'MIN_ITEMS':
            min_items = rule.params.get('value', 0)
            return '        if (msg->%s_count < %d) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Too few items");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, min_items, rule.constraint_id)
        
        elif rule.rule_type == 'MAX_ITEMS':
            max_items = rule.params.get('value', 0)
            return '        if (msg->%s_count > %d) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Too many items");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, max_items, rule.constraint_id)
        
        elif rule.rule_type == 'UNIQUE':
            # Simplified unique check - would need type-specific implementation
            return '        /* TODO: Implement unique constraint for repeated field */\n'
        
        elif rule.rule_type == 'NO_SPARSE':
            return '        /* TODO: Implement no_sparse constraint for map field */\n'
        
        # Default fallback
        return '        /* TODO: Implement rule type %s */\n' % rule.rule_type
    
    def _generate_message_rule_check(self, message, rule):
        """Generate code for a message-level validation rule."""
        # This is a simplified version - full implementation would handle all rule types
        return '    /* TODO: Implement message rule type %s */\n' % rule.rule_type
