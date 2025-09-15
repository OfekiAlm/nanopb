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
        
        # Try to import from proto subdirectory first
        try:
            from proto import validate_pb2 as validate_module
            validate_pb2 = validate_module
            return validate_pb2
        except ImportError:
            pass
            
        # Try direct import from proto subdirectory
        try:
            import proto.validate_pb2 as validate_module
            validate_pb2 = validate_module
            return validate_pb2
        except ImportError:
            pass
            
        # Try to build it using the proto module
        try:
            from . import proto
            proto.load_nanopb_pb2()  # Ensure nanopb_pb2 is loaded first
            
            # Try to import pre-compiled version
            try:
                from .proto import validate_pb2 as validate_module
                validate_pb2 = validate_module
                return validate_pb2
            except ImportError:
                pass
            
            # Try to build it
            mypath = os.path.dirname(__file__)
            protosrc = os.path.join(mypath, 'proto', 'validate.proto')
            if os.path.exists(protosrc) and proto.build_nanopb_proto(protosrc, mypath):
                from .proto import validate_pb2 as validate_module
                validate_pb2 = validate_module
                return validate_pb2
                
        except Exception:
            pass
            
    except Exception as e:
        sys.stderr.write("Failed to load validate.proto: %s\n" % str(e))
        sys.stderr.write("Validation support will be disabled.\n")
        return None
    
    sys.stderr.write("validate.proto not found or could not be loaded.\n")
    sys.stderr.write("Validation support will be disabled.\n")
    return None

def parse_validation_rules_from_unknown_fields(field_options):
    """Parse validation rules from unknown fields in field options.
    
    This is a fallback when the validate_pb2 extensions can't be properly loaded.
    We manually parse the wire format data from unknown fields.
    """
    rules = {}
    
    # Extension number 1011 is used for validate.rules
    validate_rules_tag = b'\x9a?'  # Wire format for field 1011
    
    if hasattr(field_options, '_unknown_fields'):
        for tag, value in field_options._unknown_fields:
            if tag == validate_rules_tag:
                # Parse the FieldRules message from the wire format
                try:
                    # The value is a length-prefixed message
                    # First byte is the length, rest is the actual message
                    if len(value) < 2:
                        continue
                        
                    # Skip the length byte and parse the message content
                    message_data = value[1:]  # Skip first byte (length)
                    
                    # Parse wire format manually
                    i = 0
                    while i < len(message_data):
                        if i >= len(message_data):
                            break
                            
                        # Read field tag and wire type
                        tag_byte = message_data[i]
                        i += 1
                        
                        field_num = tag_byte >> 3
                        wire_type = tag_byte & 0x07
                        
                        if wire_type == 2:  # Length-delimited (string/message)
                            # Read length
                            length = message_data[i]
                            i += 1
                            
                            # Read data
                            data = message_data[i:i+length]
                            i += length
                            
                            if field_num == 14:  # StringRules field
                                if 'string' not in rules:
                                    rules['string'] = {}
                                string_rules = parse_string_rules(data)
                                # Merge with existing string rules
                                rules['string'].update(string_rules)
                        elif wire_type == 0:  # Varint
                            # Read varint
                            val = 0
                            shift = 0
                            while i < len(message_data):
                                byte = message_data[i]
                                i += 1
                                val |= (byte & 0x7F) << shift
                                if (byte & 0x80) == 0:
                                    break
                                shift += 7
                            
                            # Handle simple field rules
                            if field_num == 22:  # required field
                                rules['required'] = bool(val)
                        else:
                            # Skip unknown wire types
                            break
                            
                except Exception as e:
                    # If parsing fails, just continue without rules
                    pass
    
    return rules

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
            
        # Parse rules based on field type
        if rules_option.HasField('string'):
            self._parse_string_rules(rules_option.string)
        elif rules_option.HasField('int32'):
            self._parse_numeric_rules(rules_option.int32, 'int32')
        elif rules_option.HasField('int64'):
            self._parse_numeric_rules(rules_option.int64, 'int64')
        elif rules_option.HasField('uint32'):
            self._parse_numeric_rules(rules_option.uint32, 'uint32')
        elif rules_option.HasField('uint64'):
            self._parse_numeric_rules(rules_option.uint64, 'uint64')
        elif rules_option.HasField('double'):
            self._parse_numeric_rules(rules_option.double, 'double')
        elif rules_option.HasField('float'):
            self._parse_numeric_rules(rules_option.float, 'float')
        elif rules_option.HasField('bool'):
            self._parse_bool_rules(rules_option.bool)
        elif rules_option.HasField('bytes'):
            self._parse_bytes_rules(rules_option.bytes)
        elif rules_option.HasField('enum'):
            self._parse_enum_rules(rules_option.enum)
        elif rules_option.HasField('repeated'):
            self._parse_repeated_rules(rules_option.repeated)
        elif rules_option.HasField('map'):
            self._parse_map_rules(rules_option.map)
        
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
                # Fallback: try to parse from proto descriptor unknown fields
                field_validator = FieldValidator(field, None, proto_file, message)
                
                # If field_validator has rules after parsing unknown fields, use it
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
        
        elif rule.rule_type == 'STRING_MIN_LEN':
            min_len = rule.params.get('value', 0)
            return '        if (strlen(msg->%s) < %d) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "String too short");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, min_len, rule.constraint_id)
        
        elif rule.rule_type == 'STRING_MAX_LEN':
            max_len = rule.params.get('value', 0)
            return '        if (strlen(msg->%s) > %d) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "String too long");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field_name, max_len, rule.constraint_id)
        
        # Add more rule type implementations here
        return '        /* TODO: Implement rule type %s */\n' % rule.rule_type
    
    def _generate_message_rule_check(self, message, rule):
        """Generate code for a message-level validation rule."""
        # This is a simplified version - full implementation would handle all rule types
        return '    /* TODO: Implement message rule type %s */\n' % rule.rule_type
