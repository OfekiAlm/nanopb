#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb validation support module.

This module extends the nanopb generator to support declarative validation
constraints via custom options defined in validate.proto.
"""

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
        from . import proto
        proto.load_nanopb_pb2()  # Ensure nanopb_pb2 is loaded first
        
        # Try to import pre-compiled version
        try:
            from .proto import validate_pb2
            return validate_pb2
        except ImportError:
            pass
        
        # Try to build it
        import os.path
        mypath = os.path.dirname(__file__)
        protosrc = os.path.join(mypath, 'proto', 'validate.proto')
        if proto.build_nanopb_proto(protosrc, mypath):
            from .proto import validate_pb2
            return validate_pb2
    except Exception as e:
        sys.stderr.write("Failed to load validate.proto: %s\n" % str(e))
        sys.stderr.write("Validation support will be disabled.\n")
        return None

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
    def __init__(self, field, rules_option):
        self.field = field
        self.rules = []
        self.parse_rules(rules_option)
    
    def parse_rules(self, rules_option):
        """Parse validation rules from the FieldRules option."""
        if not rules_option:
            return
        
        # Check for required constraint
        if rules_option.HasField('required') and rules_option.required:
            self.rules.append(ValidationRule('REQUIRED', 'required'))
        
        # Check for oneof_required constraint
        if rules_option.HasField('oneof_required') and rules_option.oneof_required:
            self.rules.append(ValidationRule('ONEOF_REQUIRED', 'oneof_required'))
        
        # Parse type-specific rules
        if rules_option.HasField('int32'):
            self._parse_numeric_rules(rules_option.int32, 'int32')
        elif rules_option.HasField('int64'):
            self._parse_numeric_rules(rules_option.int64, 'int64')
        elif rules_option.HasField('uint32'):
            self._parse_numeric_rules(rules_option.uint32, 'uint32')
        elif rules_option.HasField('uint64'):
            self._parse_numeric_rules(rules_option.uint64, 'uint64')
        elif rules_option.HasField('sint32'):
            self._parse_numeric_rules(rules_option.sint32, 'sint32')
        elif rules_option.HasField('sint64'):
            self._parse_numeric_rules(rules_option.sint64, 'sint64')
        elif rules_option.HasField('fixed32'):
            self._parse_numeric_rules(rules_option.fixed32, 'fixed32')
        elif rules_option.HasField('fixed64'):
            self._parse_numeric_rules(rules_option.fixed64, 'fixed64')
        elif rules_option.HasField('sfixed32'):
            self._parse_numeric_rules(rules_option.sfixed32, 'sfixed32')
        elif rules_option.HasField('sfixed64'):
            self._parse_numeric_rules(rules_option.sfixed64, 'sfixed64')
        elif rules_option.HasField('float'):
            self._parse_numeric_rules(rules_option.float, 'float')
        elif rules_option.HasField('double'):
            self._parse_numeric_rules(rules_option.double, 'double')
        elif rules_option.HasField('bool'):
            self._parse_bool_rules(rules_option.bool)
        elif rules_option.HasField('string'):
            self._parse_string_rules(rules_option.string)
        elif rules_option.HasField('bytes'):
            self._parse_bytes_rules(rules_option.bytes)
        elif rules_option.HasField('enum'):
            self._parse_enum_rules(rules_option.enum)
        elif rules_option.HasField('repeated'):
            self._parse_repeated_rules(rules_option.repeated)
        elif rules_option.HasField('map'):
            self._parse_map_rules(rules_option.map)
    
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
    def __init__(self, message, message_rules=None):
        self.message = message
        self.field_validators = OrderedDict()
        self.message_rules = []
        
        # Parse field-level rules
        for field in message.fields:
            if hasattr(field, 'validate_rules') and field.validate_rules:
                self.field_validators[field.name] = FieldValidator(field, field.validate_rules)
        
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
        validator = MessageValidator(message, message_rules)
        if validator.field_validators or validator.message_rules:
            self.validators[message.name] = validator
    
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
        # This is a simplified version - full implementation would handle all field types
        if rule.rule_type == 'REQUIRED':
            if field.rules == 'OPTIONAL':
                return '        if (!msg->has_%s) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Field is required");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field.name, rule.constraint_id)
        
        # Add more rule type implementations here
        return '        /* TODO: Implement rule type %s */\n' % rule.rule_type
    
    def _generate_message_rule_check(self, message, rule):
        """Generate code for a message-level validation rule."""
        # This is a simplified version - full implementation would handle all rule types
        return '    /* TODO: Implement message rule type %s */\n' % rule.rule_type
