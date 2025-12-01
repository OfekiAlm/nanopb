#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb validation support module.

This module extends the nanopb generator to support declarative validation
constraints via custom options defined in validate.proto.
"""
from collections import OrderedDict

from dataclasses import dataclass, field
from typing import Any, Dict, List

# --- Module constants -----------------------------------------------------

# Rule identifiers used throughout codegen
RULE_REQUIRED = 'REQUIRED'
RULE_ONEOF_REQUIRED = 'ONEOF_REQUIRED'
RULE_EQ = 'EQ'
RULE_GT = 'GT'
RULE_GTE = 'GTE'
RULE_LT = 'LT'
RULE_LTE = 'LTE'
RULE_IN = 'IN'
RULE_NOT_IN = 'NOT_IN'
RULE_MIN_LEN = 'MIN_LEN'
RULE_MAX_LEN = 'MAX_LEN'
RULE_PREFIX = 'PREFIX'
RULE_SUFFIX = 'SUFFIX'
RULE_CONTAINS = 'CONTAINS'
RULE_ASCII = 'ASCII'
RULE_EMAIL = 'EMAIL'
RULE_HOSTNAME = 'HOSTNAME'
RULE_IP = 'IP'
RULE_IPV4 = 'IPV4'
RULE_IPV6 = 'IPV6'
RULE_ENUM_DEFINED = 'ENUM_DEFINED'
RULE_MIN_ITEMS = 'MIN_ITEMS'
RULE_MAX_ITEMS = 'MAX_ITEMS'
RULE_UNIQUE = 'UNIQUE'
RULE_ITEMS = 'ITEMS'
RULE_NO_SPARSE = 'NO_SPARSE'
RULE_ANY_IN = 'ANY_IN'
RULE_ANY_NOT_IN = 'ANY_NOT_IN'



@dataclass(frozen=True)
class ValidationRule:
    """Represents a single validation rule."""
    rule_type: str
    constraint_id: str
    params: Dict[str, Any] = field(default_factory=dict)

class FieldValidator:
    """Handles validation for a single field."""
    def __init__(self, field, rules_option, proto_file=None, _message_desc=None):
        # _message_desc retained in signature for compatibility; no longer used.
        self.field = field
        self.rules = []
        self.proto_file = proto_file
        self.parse_rules(rules_option)
    
    def parse_rules(self, rules_option):
        """Parse validation rules from the FieldRules option.

        Simplified: rely only on the provided rules_option (field.validate_rules).
        No descriptor scanning or binary parsing fallback.
        """
        if not rules_option:
            return
        if rules_option.HasField('string'):
            self._parse_string_rules(rules_option.string)
        if rules_option.HasField('int32'):
            self._parse_numeric_rules(rules_option.int32, 'int32')
        if rules_option.HasField('int64'):
            self._parse_numeric_rules(rules_option.int64, 'int64')
        if rules_option.HasField('uint32'):
            self._parse_numeric_rules(rules_option.uint32, 'uint32')
        if rules_option.HasField('uint64'):
            self._parse_numeric_rules(rules_option.uint64, 'uint64')
        if rules_option.HasField('sint32'):
            self._parse_numeric_rules(rules_option.sint32, 'sint32')
        if rules_option.HasField('sint64'):
            self._parse_numeric_rules(rules_option.sint64, 'sint64')
        if rules_option.HasField('fixed32'):
            self._parse_numeric_rules(rules_option.fixed32, 'fixed32')
        if rules_option.HasField('fixed64'):
            self._parse_numeric_rules(rules_option.fixed64, 'fixed64')
        if rules_option.HasField('sfixed32'):
            self._parse_numeric_rules(rules_option.sfixed32, 'sfixed32')
        if rules_option.HasField('sfixed64'):
            self._parse_numeric_rules(rules_option.sfixed64, 'sfixed64')
        if rules_option.HasField('double'):
            self._parse_numeric_rules(rules_option.double, 'double')
        if rules_option.HasField('float'):
            self._parse_numeric_rules(rules_option.float, 'float')
        if rules_option.HasField('bool'):
            self._parse_bool_rules(rules_option.bool)
        if rules_option.HasField('bytes'):
            self._parse_bytes_rules(rules_option.bytes)
        if rules_option.HasField('enum'):
            self._parse_enum_rules(rules_option.enum)
        if rules_option.HasField('repeated'):
            self._parse_repeated_rules(rules_option.repeated)
        if rules_option.HasField('map'):
            self._parse_map_rules(rules_option.map)
        if rules_option.HasField('any'):
            self._parse_any_rules(rules_option.any)
        if rules_option.HasField('required') and rules_option.required:
            self.rules.append(ValidationRule(RULE_REQUIRED, 'required'))
        if rules_option.HasField('oneof_required') and rules_option.oneof_required:
            self.rules.append(ValidationRule(RULE_ONEOF_REQUIRED, 'oneof_required'))
    
    
    def _parse_numeric_rules(self, rules, type_name):
        """Parse numeric validation rules."""
        if rules.HasField('const_value'):
            self.rules.append(ValidationRule(RULE_EQ, f'{type_name}.const', {'value': rules.const_value}))
        if rules.HasField('lt'):
            self.rules.append(ValidationRule(RULE_LT, f'{type_name}.lt', {'value': rules.lt}))
        if rules.HasField('lte'):
            self.rules.append(ValidationRule(RULE_LTE, f'{type_name}.lte', {'value': rules.lte}))
        if rules.HasField('gt'):
            self.rules.append(ValidationRule(RULE_GT, f'{type_name}.gt', {'value': rules.gt}))
        if rules.HasField('gte'):
            self.rules.append(ValidationRule(RULE_GTE, f'{type_name}.gte', {'value': rules.gte}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule(RULE_IN, f'{type_name}.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule(RULE_NOT_IN, f'{type_name}.not_in', {'values': list(rules.not_in)}))
    
    def _parse_bool_rules(self, rules: Any):
        """Parse bool validation rules."""
        if rules.HasField('const_value'):
            self.rules.append(ValidationRule(RULE_EQ, 'bool.const', {'value': rules.const_value}))
    
    def _parse_string_rules(self, rules: Any):
        """Parse string validation rules."""
        if rules.HasField('const_value'):
            self.rules.append(ValidationRule(RULE_EQ, 'string.const', {'value': rules.const_value}))
        if rules.HasField('min_len'):
            self.rules.append(ValidationRule(RULE_MIN_LEN, 'string.min_len', {'value': rules.min_len}))
        if rules.HasField('max_len'):
            self.rules.append(ValidationRule(RULE_MAX_LEN, 'string.max_len', {'value': rules.max_len}))
        if rules.HasField('prefix'):
            self.rules.append(ValidationRule(RULE_PREFIX, 'string.prefix', {'value': rules.prefix}))
        if rules.HasField('suffix'):
            self.rules.append(ValidationRule(RULE_SUFFIX, 'string.suffix', {'value': rules.suffix}))
        if rules.HasField('contains'):
            self.rules.append(ValidationRule(RULE_CONTAINS, 'string.contains', {'value': rules.contains}))
        if rules.HasField('ascii') and rules.ascii:
            self.rules.append(ValidationRule(RULE_ASCII, 'string.ascii'))
        if getattr(rules, 'email', False):
            self.rules.append(ValidationRule(RULE_EMAIL, 'string.email'))
        if getattr(rules, 'hostname', False):
            self.rules.append(ValidationRule(RULE_HOSTNAME, 'string.hostname'))
        if getattr(rules, 'ip', False):
            self.rules.append(ValidationRule(RULE_IP, 'string.ip'))
        if getattr(rules, 'ipv4', False):
            self.rules.append(ValidationRule(RULE_IPV4, 'string.ipv4'))
        if getattr(rules, 'ipv6', False):
            self.rules.append(ValidationRule(RULE_IPV6, 'string.ipv6'))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule(RULE_IN, 'string.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule(RULE_NOT_IN, 'string.not_in', {'values': list(rules.not_in)}))
    
    def _parse_bytes_rules(self, rules: Any):
        """Parse bytes validation rules."""
        if rules.HasField('const_value'):
            self.rules.append(ValidationRule(RULE_EQ, 'bytes.const', {'value': rules.const_value}))
        if rules.HasField('min_len'):
            self.rules.append(ValidationRule(RULE_MIN_LEN, 'bytes.min_len', {'value': rules.min_len}))
        if rules.HasField('max_len'):
            self.rules.append(ValidationRule(RULE_MAX_LEN, 'bytes.max_len', {'value': rules.max_len}))
        if rules.HasField('prefix'):
            self.rules.append(ValidationRule(RULE_PREFIX, 'bytes.prefix', {'value': rules.prefix}))
        if rules.HasField('suffix'):
            self.rules.append(ValidationRule(RULE_SUFFIX, 'bytes.suffix', {'value': rules.suffix}))
        if rules.HasField('contains'):
            self.rules.append(ValidationRule(RULE_CONTAINS, 'bytes.contains', {'value': rules.contains}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule(RULE_IN, 'bytes.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule(RULE_NOT_IN, 'bytes.not_in', {'values': list(rules.not_in)}))
    
    def _parse_enum_rules(self, rules: Any):
        """Parse enum validation rules."""
        if rules.HasField('const_value'):
            self.rules.append(ValidationRule(RULE_EQ, 'enum.const', {'value': rules.const_value}))
        if rules.HasField('defined_only'):
            self.rules.append(ValidationRule(RULE_ENUM_DEFINED, 'enum.defined_only', {'value': rules.defined_only}))
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule(RULE_IN, 'enum.in', {'values': list(getattr(rules, 'in'))}))
        if rules.not_in:
            self.rules.append(ValidationRule(RULE_NOT_IN, 'enum.not_in', {'values': list(rules.not_in)}))
    
    def _parse_repeated_rules(self, rules: Any):
        """Parse repeated field validation rules."""
        if rules.HasField('min_items'):
            self.rules.append(ValidationRule(RULE_MIN_ITEMS, 'repeated.min_items', {'value': rules.min_items}))
        if rules.HasField('max_items'):
            self.rules.append(ValidationRule(RULE_MAX_ITEMS, 'repeated.max_items', {'value': rules.max_items}))
        if rules.HasField('unique') and rules.unique:
            self.rules.append(ValidationRule(RULE_UNIQUE, 'repeated.unique'))
        if rules.HasField('items'):
            # Parse per-item validation rules
            items_rules = rules.items
            item_rules_list = []
            # Extract type-specific rules from items
            if items_rules.HasField('string'):
                item_rules_list.extend(self._extract_string_item_rules(items_rules.string))
            if items_rules.HasField('int32'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.int32, 'int32'))
            if items_rules.HasField('int64'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.int64, 'int64'))
            if items_rules.HasField('uint32'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.uint32, 'uint32'))
            if items_rules.HasField('uint64'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.uint64, 'uint64'))
            if items_rules.HasField('sint32'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.sint32, 'sint32'))
            if items_rules.HasField('sint64'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.sint64, 'sint64'))
            if items_rules.HasField('fixed32'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.fixed32, 'fixed32'))
            if items_rules.HasField('fixed64'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.fixed64, 'fixed64'))
            if items_rules.HasField('sfixed32'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.sfixed32, 'sfixed32'))
            if items_rules.HasField('sfixed64'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.sfixed64, 'sfixed64'))
            if items_rules.HasField('float'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.float, 'float'))
            if items_rules.HasField('double'):
                item_rules_list.extend(self._extract_numeric_item_rules(items_rules.double, 'double'))
            if items_rules.HasField('bool'):
                item_rules_list.extend(self._extract_bool_item_rules(items_rules.bool))
            if items_rules.HasField('enum'):
                item_rules_list.extend(self._extract_enum_item_rules(items_rules.enum))
            if item_rules_list:
                self.rules.append(ValidationRule(RULE_ITEMS, 'repeated.items', {'item_rules': item_rules_list}))
    
    def _extract_string_item_rules(self, rules: Any) -> List[Dict[str, Any]]:
        """Extract string validation rules for repeated items."""
        result = []
        if rules.HasField('const_value'):
            result.append({'rule': RULE_EQ, 'constraint_id': 'string.const', 'value': rules.const_value})
        if rules.HasField('min_len'):
            result.append({'rule': RULE_MIN_LEN, 'constraint_id': 'string.min_len', 'value': rules.min_len})
        if rules.HasField('max_len'):
            result.append({'rule': RULE_MAX_LEN, 'constraint_id': 'string.max_len', 'value': rules.max_len})
        if rules.HasField('prefix'):
            result.append({'rule': RULE_PREFIX, 'constraint_id': 'string.prefix', 'value': rules.prefix})
        if rules.HasField('suffix'):
            result.append({'rule': RULE_SUFFIX, 'constraint_id': 'string.suffix', 'value': rules.suffix})
        if rules.HasField('contains'):
            result.append({'rule': RULE_CONTAINS, 'constraint_id': 'string.contains', 'value': rules.contains})
        if rules.HasField('ascii') and rules.ascii:
            result.append({'rule': RULE_ASCII, 'constraint_id': 'string.ascii'})
        if getattr(rules, 'email', False):
            result.append({'rule': RULE_EMAIL, 'constraint_id': 'string.email'})
        if getattr(rules, 'hostname', False):
            result.append({'rule': RULE_HOSTNAME, 'constraint_id': 'string.hostname'})
        if getattr(rules, 'ip', False):
            result.append({'rule': RULE_IP, 'constraint_id': 'string.ip'})
        if getattr(rules, 'ipv4', False):
            result.append({'rule': RULE_IPV4, 'constraint_id': 'string.ipv4'})
        if getattr(rules, 'ipv6', False):
            result.append({'rule': RULE_IPV6, 'constraint_id': 'string.ipv6'})
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            result.append({'rule': RULE_IN, 'constraint_id': 'string.in', 'values': list(getattr(rules, 'in'))})
        if rules.not_in:
            result.append({'rule': RULE_NOT_IN, 'constraint_id': 'string.not_in', 'values': list(rules.not_in)})
        return result
    
    def _extract_numeric_item_rules(self, rules: Any, type_name: str) -> List[Dict[str, Any]]:
        """Extract numeric validation rules for repeated items."""
        result = []
        if rules.HasField('const_value'):
            result.append({'rule': RULE_EQ, 'constraint_id': f'{type_name}.const', 'value': rules.const_value})
        if rules.HasField('lt'):
            result.append({'rule': RULE_LT, 'constraint_id': f'{type_name}.lt', 'value': rules.lt})
        if rules.HasField('lte'):
            result.append({'rule': RULE_LTE, 'constraint_id': f'{type_name}.lte', 'value': rules.lte})
        if rules.HasField('gt'):
            result.append({'rule': RULE_GT, 'constraint_id': f'{type_name}.gt', 'value': rules.gt})
        if rules.HasField('gte'):
            result.append({'rule': RULE_GTE, 'constraint_id': f'{type_name}.gte', 'value': rules.gte})
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            result.append({'rule': RULE_IN, 'constraint_id': f'{type_name}.in', 'values': list(getattr(rules, 'in'))})
        if rules.not_in:
            result.append({'rule': RULE_NOT_IN, 'constraint_id': f'{type_name}.not_in', 'values': list(rules.not_in)})
        return result
    
    def _extract_bool_item_rules(self, rules: Any) -> List[Dict[str, Any]]:
        """Extract bool validation rules for repeated items."""
        result = []
        if rules.HasField('const_value'):
            result.append({'rule': RULE_EQ, 'constraint_id': 'bool.const', 'value': rules.const_value})
        return result
    
    def _extract_enum_item_rules(self, rules: Any) -> List[Dict[str, Any]]:
        """Extract enum validation rules for repeated items."""
        result = []
        if rules.HasField('const_value'):
            result.append({'rule': RULE_EQ, 'constraint_id': 'enum.const', 'value': rules.const_value})
        if rules.HasField('defined_only') and rules.defined_only:
            result.append({'rule': RULE_ENUM_DEFINED, 'constraint_id': 'enum.defined_only', 'value': rules.defined_only})
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            result.append({'rule': RULE_IN, 'constraint_id': 'enum.in', 'values': list(getattr(rules, 'in'))})
        if rules.not_in:
            result.append({'rule': RULE_NOT_IN, 'constraint_id': 'enum.not_in', 'values': list(rules.not_in)})
        return result
    
    def _parse_map_rules(self, rules: Any):
        """Parse map field validation rules."""
        if rules.HasField('min_pairs'):
            self.rules.append(ValidationRule(RULE_MIN_ITEMS, 'map.min_pairs', {'value': rules.min_pairs}))
        if rules.HasField('max_pairs'):
            self.rules.append(ValidationRule(RULE_MAX_ITEMS, 'map.max_pairs', {'value': rules.max_pairs}))
        if rules.HasField('no_sparse') and rules.no_sparse:
            self.rules.append(ValidationRule(RULE_NO_SPARSE, 'map.no_sparse'))
        # TODO: Handle key/value validation rules
    
    def _parse_any_rules(self, rules: Any):
        """Parse google.protobuf.Any field validation rules."""
        if hasattr(rules, 'in') and getattr(rules, 'in'):
            self.rules.append(ValidationRule(RULE_ANY_IN, 'any.in', {'values': list(getattr(rules, 'in'))}))
        if hasattr(rules, 'not_in') and getattr(rules, 'not_in'):
            self.rules.append(ValidationRule(RULE_ANY_NOT_IN, 'any.not_in', {'values': list(getattr(rules, 'not_in'))}))

class MessageValidator:
    """Handles validation for a message."""
    def __init__(self, message, message_rules=None, proto_file=None):
        self.message = message
        self.field_validators = OrderedDict()
        self.message_rules = []
        self.proto_file = proto_file
        # Track oneof groups with fields that have validation rules
        # Maps oneof name -> list of (field, FieldValidator) tuples
        self.oneof_validators = OrderedDict()
        
        # Parse field-level rules (simplified: only direct validate_rules usage)
        for field in getattr(message, 'fields', []) or []:
            # Check if this is a OneOf container
            if hasattr(field, 'pbtype') and field.pbtype == 'oneof':
                # Process fields inside the oneof
                oneof_name = field.name
                oneof_fields_with_rules = []
                for oneof_member in getattr(field, 'fields', []) or []:
                    if hasattr(oneof_member, 'validate_rules') and oneof_member.validate_rules:
                        fv = FieldValidator(oneof_member, oneof_member.validate_rules, proto_file, message)
                        oneof_fields_with_rules.append((oneof_member, fv))
                if oneof_fields_with_rules:
                    self.oneof_validators[oneof_name] = {
                        'oneof': field,
                        'members': oneof_fields_with_rules
                    }
            elif hasattr(field, 'validate_rules') and field.validate_rules:
                self.field_validators[field.name] = FieldValidator(field, field.validate_rules, proto_file, message)
        
        # Parse message-level rules
        if message_rules:
            self.parse_message_rules(message_rules)
    
    def parse_message_rules(self, rules: Any):
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
    def __init__(self, proto_file, bypass=False):
        self.proto_file = proto_file
        self.validators = OrderedDict()
        self.bypass = bypass  # When True, generated code collects all violations without early exit
        
        # Check if validation is enabled
        self.validate_enabled = False
        if hasattr(proto_file, 'file_options') and hasattr(proto_file.file_options, 'validate'):
            self.validate_enabled = proto_file.file_options.validate
    
    def add_message_validator(self, message, message_rules=None):
        """Add a validator for a message."""
        validator = MessageValidator(message, message_rules, self.proto_file)
        if validator.field_validators or validator.message_rules or validator.oneof_validators:
            self.validators[str(message.name)] = validator
    
    def force_add_message_validator(self, message, message_rules=None):
        """Force add a validator for a message, even if no rules are present."""
        validator = MessageValidator(message, message_rules, self.proto_file)
        self.validators[str(message.name)] = validator
    
    # ---------------------- Doxygen helpers -------------------------------
    def _escape_for_comment(self, s: str) -> str:
        """Escape strings so they are safe inside C comment/Doxygen blocks."""
        try:
            if s is None:
                return ''
            # Avoid closing comment accidentally and normalize newlines
            return str(s).replace('*/', '* /').replace('\r\n', '\n').replace('\r', '\n')
        except Exception:
            return str(s)

    def _escape_c_string(self, s: str) -> str:
        """Escape a string to be safe for use in C string literals."""
        try:
            if s is None:
                return ''
            # Escape backslashes first, then special characters
            s = str(s)
            s = s.replace('\\', '\\\\')
            s = s.replace('"', '\\"')
            s = s.replace('\n', '\\n')
            s = s.replace('\r', '\\r')
            s = s.replace('\t', '\\t')
            return s
        except Exception:
            return str(s)

    def _rule_to_text(self, rule: ValidationRule) -> str:
        """Convert a single ValidationRule into a human-friendly sentence fragment."""
        rt = rule.rule_type
        p = rule.params or {}
        val = p.get('value')
        values = p.get('values')
        def list_render(vals):
            try:
                return '{' + ', '.join(str(v) for v in vals) + '}'
            except Exception:
                return '{...}'
        if rt == RULE_REQUIRED:
            return 'required'
        if rt == RULE_ONEOF_REQUIRED:
            return 'one-of group requires a value'
        if rt == RULE_EQ:
            return f'== {val}'
        if rt == RULE_GT:
            return f'> {val}'
        if rt == RULE_GTE:
            return f'>= {val}'
        if rt == RULE_LT:
            return f'< {val}'
        if rt == RULE_LTE:
            return f'<= {val}'
        if rt == RULE_IN:
            return f'in {list_render(values)}'
        if rt == RULE_NOT_IN:
            return f'not in {list_render(values)}'
        if rt == RULE_MIN_LEN:
            return f'min length {val}'
        if rt == RULE_MAX_LEN:
            return f'max length {val}'
        if rt == RULE_PREFIX:
            return f'prefix "{self._escape_for_comment(val)}"'
        if rt == RULE_SUFFIX:
            return f'suffix "{self._escape_for_comment(val)}"'
        if rt == RULE_CONTAINS:
            return f'contains "{self._escape_for_comment(val)}"'
        if rt == RULE_ASCII:
            return 'ASCII only'
        if rt == RULE_EMAIL:
            return 'valid email address'
        if rt == RULE_HOSTNAME:
            return 'valid hostname'
        if rt == RULE_IP:
            return 'valid IP address'
        if rt == RULE_IPV4:
            return 'valid IPv4 address'
        if rt == RULE_IPV6:
            return 'valid IPv6 address'
        if rt == RULE_ENUM_DEFINED:
            return 'must be a defined enum value'
        if rt == RULE_MIN_ITEMS:
            return f'at least {val} items'
        if rt == RULE_MAX_ITEMS:
            return f'at most {val} items'
        if rt == RULE_UNIQUE:
            return 'items must be unique'
        if rt == RULE_ITEMS:
            return 'per-item validation rules'
        if rt == RULE_NO_SPARSE:
            return 'no sparse map entries'
        # Fallback
        return rt.replace('_', ' ').lower()

    def _format_field_constraints(self, field_name: str, fv: 'FieldValidator') -> str:
        """Produce a concise, pretty description line for a field and its constraints."""
        try:
            # Group multiple fragments with semicolons
            parts = [self._rule_to_text(r) for r in fv.rules]
            # Deduplicate while preserving order
            seen = set()
            uniq = []
            for t in parts:
                if t not in seen:
                    seen.add(t)
                    uniq.append(t)
            if not uniq:
                return f"- {field_name}: no constraints"
            return f"- {field_name}: " + '; '.join(uniq)
        except Exception:
            return f"- {field_name}: (constraints unavailable)"
    # ----------------------------------------------------------------------

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
        
        # Generate validation function declarations with rich Doxygen
        for msg_name, validator in self.validators.items():
            # Use the C struct name from the message
            struct_name = str(validator.message.name)
            func_name = 'pb_validate_' + struct_name.replace('.', '_')
            # Build field description list
            try:
                all_field_names = []
                for f in getattr(validator.message, 'fields', []) or []:
                    fname = getattr(f, 'name', None)
                    if fname:
                        all_field_names.append(fname)
                validated_field_names = list(validator.field_validators.keys())
                fields_without = [fn for fn in all_field_names if fn not in validated_field_names]
            except Exception:
                all_field_names, fields_without = [], []

            # Doxygen header
            yield '/**\n'
            yield ' * @brief Validate %s message.\n' % struct_name
            yield ' *\n'
            yield ' * Fields and constraints:\n'
            # With constraints
            for fname, fv in validator.field_validators.items():
                desc = self._escape_for_comment(self._format_field_constraints(fname, fv))
                yield ' * %s\n' % desc
            # Oneof validators
            for oneof_name, oneof_data in validator.oneof_validators.items():
                yield ' * - oneof %s:\n' % self._escape_for_comment(oneof_name)
                for member_field, member_fv in oneof_data['members']:
                    desc = self._escape_for_comment(self._format_field_constraints(member_field.name, member_fv))
                    yield ' *   %s\n' % desc
            # Without constraints
            for fname in fields_without:
                # Skip oneof names as they are already documented above
                if fname in validator.oneof_validators:
                    continue
                yield ' * - %s: no constraints\n' % self._escape_for_comment(fname)
            # Message-level rules summary
            if validator.message_rules:
                yield ' *\n'
                yield ' * Message-level rules:\n'
                for mr in validator.message_rules:
                    try:
                        if mr.rule_type == 'REQUIRES':
                            yield ' * - requires field "%s"\n' % self._escape_for_comment(mr.params.get('field', ''))
                        elif mr.rule_type == 'MUTEX':
                            fields = mr.params.get('fields', [])
                            yield ' * - mutual exclusion among %s\n' % self._escape_for_comment('{'+', '.join(fields)+'}')
                        elif mr.rule_type == 'AT_LEAST':
                            n = mr.params.get('n', 1)
                            fields = mr.params.get('fields', [])
                            yield ' * - at least %s of %s must be set\n' % (str(n), self._escape_for_comment('{'+', '.join(fields)+'}'))
                        else:
                            yield ' * - %s\n' % self._escape_for_comment(mr.rule_type.lower())
                    except Exception:
                        yield ' * - (message rule)\n'
            yield ' *\n'
            yield ' * @param msg [in] Pointer to %s instance to validate.\n' % struct_name
            yield ' * @param violations [out] Optional violations accumulator (can be NULL).\n'
            yield ' * @return true if valid, false otherwise.\n'
            yield ' */\n'
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
            
            # Generate comment listing fields without constraints
            fields_without_constraints = []
            all_field_names = set()
            for f in getattr(validator.message, 'fields', []):
                try:
                    field_name = getattr(f, 'name', None)
                    if field_name:
                        all_field_names.add(field_name)
                except Exception:
                    pass
            
            validated_field_names = set(validator.field_validators.keys())
            fields_without_constraints = sorted(all_field_names - validated_field_names)

            # Generate validation function
            yield 'bool %s(const %s *msg, pb_violations_t *violations)\n' % (func_name, struct_name)
            yield '{\n'
            if fields_without_constraints:
                    yield '    /* Fields without constraints:\n'
                    for field in fields_without_constraints:
                        yield '       - %s\n' % field
                    yield '    */\n'
                    yield '\n'
            # Use bypass macro if bypass mode is enabled
            if self.bypass:
                yield '    PB_VALIDATE_BEGIN_BYPASS(ctx, %s, msg, violations);\n' % struct_name
            else:
                yield '    PB_VALIDATE_BEGIN(ctx, %s, msg, violations);\n' % struct_name
            yield '\n'
            
            # Generate field validations
            for field_name, field_validator in validator.field_validators.items():
                field = field_validator.field
                yield '    /* Validate field: %s */\n' % field_name
                yield '    PB_VALIDATE_FIELD_BEGIN(ctx, "%s");\n' % field_name
                
                for rule in field_validator.rules:
                    yield self._generate_rule_check(field, rule)

                # Automatic recursion for submessages
                try:
                    is_message_field = getattr(field, 'pbtype', None) in ('MESSAGE', 'MSG_W_CB')
                    submsg_ctype = getattr(field, 'ctype', None)
                    if is_message_field and submsg_ctype:
                        # Skip google.protobuf.Any - no validation function exists for it
                        submsg_ctype_str = str(submsg_ctype).lower()
                        if not ('google' in submsg_ctype_str and 'protobuf' in submsg_ctype_str and 'any' in submsg_ctype_str):
                            sub_func = 'pb_validate_' + str(submsg_ctype).replace('.', '_')
                            allocation = getattr(field, 'allocation', None)
                            rules = getattr(field, 'rules', None)
                            if allocation == 'POINTER':
                                # Pointer to submessage
                                yield '    if (msg->%s) {\n' % field_name
                                yield '        bool ok_nested = %s(msg->%s, violations);\n' % (sub_func, field_name)
                                yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                yield '    }\n'
                            else:
                                # Inline submessage struct
                                if rules == 'OPTIONAL':
                                    yield '    if (msg->has_%s) {\n' % field_name
                                    yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, field_name)
                                    yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                    yield '    }\n'
                                else:
                                    yield '    {\n'
                                    yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, field_name)
                                    yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                    yield '    }\n'
                except Exception:
                    # If field shape is unexpected, skip recursion silently
                    pass
                
                yield '    PB_VALIDATE_FIELD_END(ctx);\n'
                yield '\n'
            
            # Also recurse into message-typed fields that have no field-level rules
            try:
                field_names_with_rules = set(validator.field_validators.keys())
            except Exception:
                field_names_with_rules = set()
            for f in getattr(validator.message, 'fields', []):
                try:
                    if getattr(f, 'name', None) in field_names_with_rules:
                        continue
                    if getattr(f, 'pbtype', None) not in ('MESSAGE', 'MSG_W_CB'):
                        continue
                    fname = getattr(f, 'name')
                    submsg_ctype = getattr(f, 'ctype', None)
                    if not submsg_ctype:
                        continue
                    # Skip google.protobuf.Any - no validation function exists for it
                    submsg_ctype_str = str(submsg_ctype).lower()
                    if 'google' in submsg_ctype_str and 'protobuf' in submsg_ctype_str and 'any' in submsg_ctype_str:
                        continue
                    sub_func = 'pb_validate_' + str(submsg_ctype).replace('.', '_')
                    allocation = getattr(f, 'allocation', None)
                    rules = getattr(f, 'rules', None)
                    # Open path context
                    yield '    /* Validate field: %s */\n' % fname
                    yield '    PB_VALIDATE_FIELD_BEGIN(ctx, "%s");\n' % fname
                    if allocation == 'POINTER':
                        yield '    if (msg->%s) {\n' % fname
                        yield '        bool ok_nested = %s(msg->%s, violations);\n' % (sub_func, fname)
                        yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                        yield '    }\n'
                    else:
                        if rules == 'OPTIONAL':
                            yield '    if (msg->has_%s) {\n' % fname
                            yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, fname)
                            yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                            yield '    }\n'
                        else:
                            yield '    {\n'
                            yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, fname)
                            yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                            yield '    }\n'
                    yield '    PB_VALIDATE_FIELD_END(ctx);\n'
                    yield '\n'
                except Exception:
                    # Skip if field metadata is unexpected
                    pass

            # Generate oneof member validations (switch-based)
            for oneof_name, oneof_data in validator.oneof_validators.items():
                oneof_obj = oneof_data['oneof']
                members = oneof_data['members']  # List of (field, FieldValidator) tuples
                
                yield '    /* Validate oneof: %s */\n' % oneof_name
                yield '    switch (msg->which_%s) {\n' % oneof_name
                
                for member_field, member_fv in members:
                    field_name = member_field.name
                    # Generate case for this oneof member
                    # Tag constant format: <MESSAGE_NAME>_<field_name>_tag
                    tag_name = '%s_%s_tag' % (struct_name, field_name)
                    yield '        case %s:\n' % tag_name
                    yield '            PB_VALIDATE_FIELD_BEGIN(ctx, "%s");\n' % field_name
                    
                    for rule in member_fv.rules:
                        yield self._generate_oneof_rule_check(member_field, rule, oneof_name, oneof_obj)
                    
                    yield '            PB_VALIDATE_FIELD_END(ctx);\n'
                    yield '            break;\n'
                
                yield '        default:\n'
                yield '            break;\n'
                yield '    }\n'
                yield '\n'

            # Generate message-level validations
            for rule in validator.message_rules:
                yield '    /* Message rule: %s */\n' % rule.constraint_id
                yield self._generate_message_rule_check(validator.message, rule)
                yield '\n'
            
            yield '    PB_VALIDATE_END(ctx, violations);\n'
            yield '}\n'
            yield '\n'
    
    def _wrap_optional(self, field: Any, code: str) -> str:
        """If the field is OPTIONAL, guard checks with has_ flag."""
        try:
            is_optional = (getattr(field, 'rules', None) == 'OPTIONAL')
        except Exception:
            is_optional = False
        if not is_optional:
            return code
        field_name = field.name
        indented = ''.join(('    ' + line) if line.strip() else line for line in code.splitlines(True))
        return '        if (msg->has_%s) {\n%s        }\n' % (field_name, indented)

    def _gen_required(self, field: Any, rule: ValidationRule) -> str:
        if getattr(field, 'rules', None) == 'OPTIONAL':
            return '        if (!msg->has_%s) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Field is required");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (field.name, rule.constraint_id)
        return ''

    def _gen_numeric_comparison(self, field: Any, rule: ValidationRule) -> str:
        type_map_func = {
            'int32': ('pb_validate_int32', 'int32_t'),
            'sint32': ('pb_validate_int32', 'int32_t'),
            'sfixed32': ('pb_validate_int32', 'int32_t'),
            'int64': ('pb_validate_int64', 'int64_t'),
            'sint64': ('pb_validate_int64', 'int64_t'),
            'sfixed64': ('pb_validate_int64', 'int64_t'),
            'uint32': ('pb_validate_uint32', 'uint32_t'),
            'fixed32': ('pb_validate_uint32', 'uint32_t'),
            'uint64': ('pb_validate_uint64', 'uint64_t'),
            'fixed64': ('pb_validate_uint64', 'uint64_t'),
            'float': ('pb_validate_float', 'float'),
            'double': ('pb_validate_double', 'double'),
            'bool': ('pb_validate_bool', 'bool'),
            'enum': ('pb_validate_enum', 'int')
        }
        constraint_prefix = rule.constraint_id.split('.', 1)[0]
        validator_func, ctype = type_map_func.get(constraint_prefix, (None, None))

        if not validator_func:
            return ''

        value = rule.params.get('value', 0)
        enum_map = {
            RULE_GT: 'PB_VALIDATE_RULE_GT',
            RULE_GTE: 'PB_VALIDATE_RULE_GTE',
            RULE_LT: 'PB_VALIDATE_RULE_LT',
            RULE_LTE: 'PB_VALIDATE_RULE_LTE',
            RULE_EQ: 'PB_VALIDATE_RULE_EQ'
        }
        rule_enum = enum_map.get(rule.rule_type)
        if not rule_enum:
            return ''

        # Use PB_VALIDATE_NUMERIC_GENERIC macro instead of inlining logic
        body = (
            '        PB_VALIDATE_NUMERIC_GENERIC(ctx, msg, %s, %s, %s, %s, %s, "%s");\n'
        ) % (field.name, ctype, validator_func, rule_enum, value, rule.constraint_id)
        return self._wrap_optional(field, body)

    def _gen_in(self, field: Any, rule: ValidationRule) -> str:
        values = rule.params.get('values', [])
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                values_array = ', '.join('"%s"' % v for v in values)
                return ('        {\n'
                        '            const char *s = NULL; pb_size_t l = 0; (void)l;\n'
                        '            const char *allowed[] = { %s };\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                bool match = false;\n'
                        '                for (size_t i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++) {\n'
                        '                    if (strcmp(s, allowed[i]) == 0) { match = true; break; }\n'
                        '                }\n'
                        '                if (!match) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n') % (values_array, field_name, rule.constraint_id)
            conditions = ['strcmp(msg->%s, "%s") == 0' % (field_name, v) for v in values]
            condition_str = ' || '.join(conditions)
            # Use simple message to avoid escaping issues with quoted values in C string literals
            return '        if (!(%s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (condition_str, rule.constraint_id)
        else:
            conditions = ['msg->%s == %s' % (field_name, v) for v in values]
            condition_str = ' || '.join(conditions)
            values_str = ', '.join(str(v) for v in values)
            return self._wrap_optional(field, '        if (!(%s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of: %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (condition_str, rule.constraint_id, values_str))

    def _gen_not_in(self, field: Any, rule: ValidationRule) -> str:
        values = rule.params.get('values', [])
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                values_array = ', '.join('"%s"' % v for v in values)
                return ('        {\n'
                        '            const char *s = NULL; pb_size_t l = 0; (void)l;\n'
                        '            const char *blocked[] = { %s };\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                bool forbidden = false;\n'
                        '                for (size_t i = 0; i < sizeof(blocked)/sizeof(blocked[0]); i++) {\n'
                        '                    if (strcmp(s, blocked[i]) == 0) { forbidden = true; break; }\n'
                        '                }\n'
                        '                if (forbidden) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "Value is in forbidden set");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n') % (values_array, field_name, rule.constraint_id)
            conditions = ['strcmp(msg->%s, "%s") != 0' % (field_name, v) for v in values]
            condition_str = ' && '.join(conditions)
            # Use simple message to avoid escaping issues with quoted values in C string literals
            return '        if (!(%s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of forbidden set");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (condition_str, rule.constraint_id)
        else:
            conditions = ['msg->%s != %s' % (field_name, v) for v in values]
            condition_str = ' && '.join(conditions)
            values_str = ', '.join(str(v) for v in values)
            return self._wrap_optional(field, '        if (!(%s)) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of: %s");\n' \
                   '            if (ctx.early_exit) return false;\n' \
                   '        }\n' % (condition_str, rule.constraint_id, values_str))

    def _gen_min_len(self, field: Any, rule: ValidationRule) -> str:
        min_len = rule.params.get('value', 0)
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_MIN_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, min_len, rule.constraint_id)
                )
            # Use macro helper for normal string fields
            return self._wrap_optional(field,
                '        PB_VALIDATE_STR_MIN_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, min_len, rule.constraint_id)
            )
        else:  # bytes
            return self._wrap_optional(field,
                '        if (msg->%s.size < %d) {\n'
                '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too short");\n'
                '            if (ctx.early_exit) return false;\n'
                '        }\n' % (field_name, min_len, rule.constraint_id)
            )

    def _gen_max_len(self, field: Any, rule: ValidationRule) -> str:
        max_len = rule.params.get('value', 0)
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_MAX_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, max_len, rule.constraint_id)
                )
            # Use macro helper for normal string fields
            return self._wrap_optional(field,
                '        PB_VALIDATE_STR_MAX_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, max_len, rule.constraint_id)
            )
        else:  # bytes
            return self._wrap_optional(field,
                '        if (msg->%s.size > %d) {\n'
                '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too long");\n'
                '            if (ctx.early_exit) return false;\n'
                '        }\n' % (field_name, max_len, rule.constraint_id)
            )

    def _gen_prefix(self, field: Any, rule: ValidationRule) -> str:
        prefix = rule.params.get('value', '')
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_PREFIX(ctx, msg, %s, "%s", "%s");\n' % (field_name, prefix, rule.constraint_id)
                )
            return self._wrap_optional(field, 
                '        PB_VALIDATE_STR_PREFIX(ctx, msg, %s, "%s", "%s");\n' % (field_name, prefix, rule.constraint_id)
            )
        else:  # bytes
            return self._wrap_optional(field, '        if (msg->%s.size < %d || memcmp(msg->%s.bytes, "%s", %d) != 0) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must start with specified prefix");\n' \
                   '            if (ctx.early_exit) return false;\n' \
          '        }\n' % (field_name, len(prefix), field_name, prefix, len(prefix), rule.constraint_id))

    def _gen_suffix(self, field: Any, rule: ValidationRule) -> str:
        suffix = rule.params.get('value', '')
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_SUFFIX(ctx, msg, %s, "%s", "%s");\n' % (field_name, suffix, rule.constraint_id)
                )
            return self._wrap_optional(field, 
                '        PB_VALIDATE_STR_SUFFIX(ctx, msg, %s, "%s", "%s");\n' % (field_name, suffix, rule.constraint_id)
            )
        else:  # bytes
            return self._wrap_optional(field, '        if (msg->%s.size < %d || memcmp(msg->%s.bytes + msg->%s.size - %d, "%s", %d) != 0) {\n' \
                   '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must end with specified suffix");\n' \
                   '            if (ctx.early_exit) return false;\n' \
          '        }\n' % (field_name, len(suffix), field_name, field_name, len(suffix), suffix, len(suffix), rule.constraint_id))

    def _gen_contains(self, field: Any, rule: ValidationRule) -> str:
        contains = rule.params.get('value', '')
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_CONTAINS(ctx, msg, %s, "%s", "%s");\n' % (field_name, contains, rule.constraint_id)
                )
            return self._wrap_optional(field, 
                '        PB_VALIDATE_STR_CONTAINS(ctx, msg, %s, "%s", "%s");\n' % (field_name, contains, rule.constraint_id)
            )
        else:  # bytes  
            return self._wrap_optional(field, '        {\n' \
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
                    '        }\n' % (field_name, len(contains), field_name, contains, len(contains), rule.constraint_id))

    def _gen_ascii(self, field: Any, rule: ValidationRule) -> str:
        field_name = field.name
        if 'string' in rule.constraint_id:
            if getattr(field, 'allocation', None) == 'CALLBACK':
                # Use macro for callback string fields
                return self._wrap_optional(field,
                    '        PB_VALIDATE_STRING_ASCII(ctx, msg, %s, "%s");\n' % (field_name, rule.constraint_id)
                )
            # Use macro for normal string fields
            return self._wrap_optional(field,
                '        PB_VALIDATE_STR_ASCII(ctx, msg, %s, "%s");\n' % (field_name, rule.constraint_id)
            )
        return ''

    def _gen_string_format(self, field: Any, rule: ValidationRule) -> str:
        field_name = field.name
        if 'string' in rule.constraint_id:
            # Map rule types to macro names for callback and normal string fields
            callback_macro_map = {
                RULE_EMAIL: 'PB_VALIDATE_STRING_EMAIL',
                RULE_HOSTNAME: 'PB_VALIDATE_STRING_HOSTNAME',
                RULE_IP: 'PB_VALIDATE_STRING_IP',
                RULE_IPV4: 'PB_VALIDATE_STRING_IPV4',
                RULE_IPV6: 'PB_VALIDATE_STRING_IPV6',
            }
            normal_macro_map = {
                RULE_EMAIL: 'PB_VALIDATE_STR_EMAIL',
                RULE_HOSTNAME: 'PB_VALIDATE_STR_HOSTNAME',
                RULE_IP: 'PB_VALIDATE_STR_IP',
                RULE_IPV4: 'PB_VALIDATE_STR_IPV4',
                RULE_IPV6: 'PB_VALIDATE_STR_IPV6',
            }
            if getattr(field, 'allocation', None) == 'CALLBACK':
                macro_name = callback_macro_map.get(rule.rule_type)
            else:
                macro_name = normal_macro_map.get(rule.rule_type)
            if not macro_name:
                return ''
            return self._wrap_optional(field,
                '        %s(ctx, msg, %s, "%s");\n' % (macro_name, field_name, rule.constraint_id)
            )
        return ''

    def _gen_enum_defined(self, field: Any, rule: ValidationRule) -> str:
        # Only enforce when defined_only is true
        if not rule.params.get('value', True):
            return ''
        # Collect enum numeric values for this field's enum type
        try:
            enum_vals = []
            ctype = getattr(field, 'ctype', None)
            for e in getattr(self.proto_file, 'enums', []) or []:
                try:
                    if e.names == ctype or str(e.names) == str(ctype):
                        enum_vals = [int(v) for (_, v) in getattr(e, 'values', [])]
                        break
                except Exception:
                    continue
            if enum_vals:
                arr_values = ', '.join(str(v) for v in enum_vals)
                field_name = field.name
                body = (
                    '        {\n'
                    '            static const int __pb_defined_vals[] = { %s };\n'
                    '            if (!pb_validate_enum_defined_only((int)msg->%s, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be a defined enum value");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n'
                ) % (arr_values, field_name, rule.constraint_id)
                return self._wrap_optional(field, body)
        except Exception:
            # Fallback: no-op if enum definition not found
            pass
        return ''

    def _gen_min_items(self, field: Any, rule: ValidationRule) -> str:
        min_items = rule.params.get('value', 0)
        return (
            '        PB_VALIDATE_MIN_ITEMS(ctx, msg, %s, %d, "%s");\n'
        ) % (field.name, min_items, rule.constraint_id)

    def _gen_max_items(self, field: Any, rule: ValidationRule) -> str:
        max_items = rule.params.get('value', 0)
        return (
            '        PB_VALIDATE_MAX_ITEMS(ctx, msg, %s, %d, "%s");\n'
        ) % (field.name, max_items, rule.constraint_id)

    def _gen_unique(self, field: Any, rule: ValidationRule) -> str:
        """Generate code for unique constraint on repeated fields."""
        field_name = field.name
        # Get the field type to determine comparison method
        pbtype = getattr(field, 'pbtype', None)
        
        # Skip unique validation for message types
        if pbtype in ('MESSAGE', 'MSG_W_CB'):
            return '        /* NOTE: repeated.unique is not supported for message types */\n'
        
        # Use appropriate macro based on type
        if pbtype == 'STRING':
            return '        PB_VALIDATE_REPEATED_UNIQUE_STRING(ctx, msg, %s, "%s");\n' % (field_name, rule.constraint_id)
        elif pbtype == 'BYTES':
            return '        PB_VALIDATE_REPEATED_UNIQUE_BYTES(ctx, msg, %s, "%s");\n' % (field_name, rule.constraint_id)
        else:
            # Scalar types and enums use direct comparison
            return '        PB_VALIDATE_REPEATED_UNIQUE_SCALAR(ctx, msg, %s, "%s");\n' % (field_name, rule.constraint_id)
    
    def _gen_items(self, field: Any, rule: ValidationRule) -> str:
        """Generate code for per-item validation rules on repeated fields."""
        field_name = field.name
        item_rules = rule.params.get('item_rules', [])
        if not item_rules:
            return ''
        
        pbtype = getattr(field, 'pbtype', None)
        code = ''
        
        for item_rule in item_rules:
            rule_type = item_rule.get('rule')
            constraint_id = item_rule.get('constraint_id', '')
            
            if rule_type == RULE_MIN_LEN and pbtype == 'STRING':
                value = item_rule.get('value', 0)
                code += '        PB_VALIDATE_REPEATED_ITEMS_STR_MIN_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, value, constraint_id)
            
            elif rule_type == RULE_MAX_LEN and pbtype == 'STRING':
                value = item_rule.get('value', 0)
                code += '        PB_VALIDATE_REPEATED_ITEMS_STR_MAX_LEN(ctx, msg, %s, %d, "%s");\n' % (field_name, value, constraint_id)
            
            elif rule_type in (RULE_GT, RULE_GTE, RULE_LT, RULE_LTE, RULE_EQ):
                value = item_rule.get('value', 0)
                # Determine C type and validator function based on constraint_id
                ctype, func = self._get_numeric_type_info(constraint_id)
                if ctype and func:
                    rule_enum = {
                        RULE_GT: 'PB_VALIDATE_RULE_GT',
                        RULE_GTE: 'PB_VALIDATE_RULE_GTE',
                        RULE_LT: 'PB_VALIDATE_RULE_LT',
                        RULE_LTE: 'PB_VALIDATE_RULE_LTE',
                        RULE_EQ: 'PB_VALIDATE_RULE_EQ'
                    }.get(rule_type)
                    if rule_enum:
                        code += '        PB_VALIDATE_REPEATED_ITEMS_NUMERIC(ctx, msg, %s, %s, %s, %s, %s, "%s");\n' % (
                            field_name, ctype, func, rule_enum, value, constraint_id)
            
            elif rule_type == RULE_PREFIX:
                prefix = item_rule.get('value', '')
                if pbtype == 'STRING':
                    code += '        /* repeated.items string.prefix validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            const char *__pb_prefix = "%s";\n' % prefix
                    code += '            if (!pb_validate_string(msg->%s[__pb_i], (pb_size_t)strlen(msg->%s[__pb_i]), __pb_prefix, PB_VALIDATE_RULE_PREFIX)) {\n' % (field_name, field_name)
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must start with specified prefix");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type == RULE_SUFFIX:
                suffix = item_rule.get('value', '')
                if pbtype == 'STRING':
                    code += '        /* repeated.items string.suffix validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            const char *__pb_suffix = "%s";\n' % suffix
                    code += '            if (!pb_validate_string(msg->%s[__pb_i], (pb_size_t)strlen(msg->%s[__pb_i]), __pb_suffix, PB_VALIDATE_RULE_SUFFIX)) {\n' % (field_name, field_name)
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must end with specified suffix");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type == RULE_CONTAINS:
                needle = item_rule.get('value', '')
                if pbtype == 'STRING':
                    code += '        /* repeated.items string.contains validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            const char *__pb_needle = "%s";\n' % needle
                    code += '            if (!pb_validate_string(msg->%s[__pb_i], (pb_size_t)strlen(msg->%s[__pb_i]), __pb_needle, PB_VALIDATE_RULE_CONTAINS)) {\n' % (field_name, field_name)
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain specified substring");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type == RULE_ASCII:
                if pbtype == 'STRING':
                    code += '        /* repeated.items string.ascii validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            if (!pb_validate_string(msg->%s[__pb_i], (pb_size_t)strlen(msg->%s[__pb_i]), NULL, PB_VALIDATE_RULE_ASCII)) {\n' % (field_name, field_name)
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain only ASCII characters");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type in (RULE_EMAIL, RULE_HOSTNAME, RULE_IP, RULE_IPV4, RULE_IPV6):
                if pbtype == 'STRING':
                    rule_enum = {
                        RULE_EMAIL: 'PB_VALIDATE_RULE_EMAIL',
                        RULE_HOSTNAME: 'PB_VALIDATE_RULE_HOSTNAME',
                        RULE_IP: 'PB_VALIDATE_RULE_IP',
                        RULE_IPV4: 'PB_VALIDATE_RULE_IPV4',
                        RULE_IPV6: 'PB_VALIDATE_RULE_IPV6',
                    }.get(rule_type)
                    code += '        /* repeated.items %s validation */\n' % constraint_id
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            if (!pb_validate_string(msg->%s[__pb_i], (pb_size_t)strlen(msg->%s[__pb_i]), NULL, %s)) {\n' % (field_name, field_name, rule_enum)
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "String format validation failed");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type == RULE_IN:
                values = item_rule.get('values', [])
                if pbtype == 'STRING':
                    values_array = ', '.join('"%s"' % v for v in values)
                    code += '        /* repeated.items string.in validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            const char *__pb_allowed[] = { %s };\n' % values_array
                    code += '            bool __pb_match = false;\n'
                    code += '            for (size_t __pb_k = 0; __pb_k < sizeof(__pb_allowed)/sizeof(__pb_allowed[0]); __pb_k++) {\n'
                    code += '                if (strcmp(msg->%s[__pb_i], __pb_allowed[__pb_k]) == 0) { __pb_match = true; break; }\n' % field_name
                    code += '            }\n'
                    code += '            if (!__pb_match) {\n'
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
                else:
                    conditions = ['msg->%s[__pb_i] == %s' % (field_name, v) for v in values]
                    condition_str = ' || '.join(conditions)
                    code += '        /* repeated.items %s.in validation */\n' % constraint_id.split('.')[0]
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            if (!(%s)) {\n' % condition_str
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
            
            elif rule_type == RULE_NOT_IN:
                values = item_rule.get('values', [])
                if pbtype == 'STRING':
                    values_array = ', '.join('"%s"' % v for v in values)
                    code += '        /* repeated.items string.not_in validation */\n'
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            const char *__pb_blocked[] = { %s };\n' % values_array
                    code += '            bool __pb_forbidden = false;\n'
                    code += '            for (size_t __pb_k = 0; __pb_k < sizeof(__pb_blocked)/sizeof(__pb_blocked[0]); __pb_k++) {\n'
                    code += '                if (strcmp(msg->%s[__pb_i], __pb_blocked[__pb_k]) == 0) { __pb_forbidden = true; break; }\n' % field_name
                    code += '            }\n'
                    code += '            if (__pb_forbidden) {\n'
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value is in forbidden set");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
                else:
                    conditions = ['msg->%s[__pb_i] != %s' % (field_name, v) for v in values]
                    condition_str = ' && '.join(conditions)
                    code += '        /* repeated.items %s.not_in validation */\n' % constraint_id.split('.')[0]
                    code += '        for (pb_size_t __pb_i = 0; __pb_i < msg->%s_count; ++__pb_i) {\n' % field_name
                    code += '            pb_validate_context_push_index(&ctx, __pb_i);\n'
                    code += '            if (!(%s)) {\n' % condition_str
                    code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value is in forbidden set");\n' % constraint_id
                    code += '                if (ctx.early_exit) { pb_validate_context_pop_index(&ctx); return false; }\n'
                    code += '            }\n'
                    code += '            pb_validate_context_pop_index(&ctx);\n'
                    code += '        }\n'
        
        return code
    
    def _get_numeric_type_info(self, constraint_id: str) -> tuple:
        """Get C type and validator function name from constraint_id."""
        type_map = {
            'int32': ('int32_t', 'pb_validate_int32'),
            'sint32': ('int32_t', 'pb_validate_int32'),
            'sfixed32': ('int32_t', 'pb_validate_int32'),
            'int64': ('int64_t', 'pb_validate_int64'),
            'sint64': ('int64_t', 'pb_validate_int64'),
            'sfixed64': ('int64_t', 'pb_validate_int64'),
            'uint32': ('uint32_t', 'pb_validate_uint32'),
            'fixed32': ('uint32_t', 'pb_validate_uint32'),
            'uint64': ('uint64_t', 'pb_validate_uint64'),
            'fixed64': ('uint64_t', 'pb_validate_uint64'),
            'float': ('float', 'pb_validate_float'),
            'double': ('double', 'pb_validate_double'),
            'bool': ('bool', 'pb_validate_bool'),
            'enum': ('int', 'pb_validate_enum'),
        }
        type_prefix = constraint_id.split('.', 1)[0]
        return type_map.get(type_prefix, (None, None))

    def _gen_no_sparse(self, field: Any, rule: ValidationRule) -> str:
        return '        /* TODO: Implement no_sparse constraint for map field */\n'

    def _gen_any_in(self, field: Any, rule: ValidationRule) -> str:
        values = rule.params.get('values', [])
        if not values:
            return ''
        
        field_name = field.name
        code = '        /* Validate Any type_url is in allowed list */\n'
        code += '        if (msg->has_%s) {\n' % field_name
        code += '            const char *type_url = (const char *)msg->%s.type_url;\n' % field_name
        code += '            bool type_url_valid = false;\n'
        for url in values:
            code += '            if (type_url && strcmp(type_url, "%s") == 0) type_url_valid = true;\n' % url
        code += '            if (!type_url_valid) {\n'
        code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "type_url not in allowed list");\n' % rule.constraint_id
        code += '                if (ctx.early_exit) return false;\n'
        code += '            }\n'
        code += '        }\n'
        return code

    def _gen_any_not_in(self, field: Any, rule: ValidationRule) -> str:
        values = rule.params.get('values', [])
        if not values:
            return ''
        
        field_name = field.name
        code = '        /* Validate Any type_url is not in disallowed list */\n'
        code += '        if (msg->has_%s) {\n' % field_name
        code += '            const char *type_url = (const char *)msg->%s.type_url;\n' % field_name
        for url in values:
            code += '            if (type_url && strcmp(type_url, "%s") == 0) {\n' % url
            code += '                pb_violations_add(violations, ctx.path_buffer, "%s", "type_url in disallowed list");\n' % rule.constraint_id
            code += '                if (ctx.early_exit) return false;\n'
            code += '            }\n'
        code += '        }\n'
        return code

    def _generate_rule_check(self, field: Any, rule: ValidationRule):
        """Generate code for a single field validation rule."""
        dispatch = {
            RULE_REQUIRED: self._gen_required,
            RULE_GT: self._gen_numeric_comparison,
            RULE_GTE: self._gen_numeric_comparison,
            RULE_LT: self._gen_numeric_comparison,
            RULE_LTE: self._gen_numeric_comparison,
            RULE_EQ: self._gen_numeric_comparison,
            RULE_IN: self._gen_in,
            RULE_NOT_IN: self._gen_not_in,
            RULE_MIN_LEN: self._gen_min_len,
            RULE_MAX_LEN: self._gen_max_len,
            RULE_PREFIX: self._gen_prefix,
            RULE_SUFFIX: self._gen_suffix,
            RULE_CONTAINS: self._gen_contains,
            RULE_ASCII: self._gen_ascii,
            RULE_EMAIL: self._gen_string_format,
            RULE_HOSTNAME: self._gen_string_format,
            RULE_IP: self._gen_string_format,
            RULE_IPV4: self._gen_string_format,
            RULE_IPV6: self._gen_string_format,
            RULE_ENUM_DEFINED: self._gen_enum_defined,
            RULE_MIN_ITEMS: self._gen_min_items,
            RULE_MAX_ITEMS: self._gen_max_items,
            RULE_UNIQUE: self._gen_unique,
            RULE_NO_SPARSE: self._gen_no_sparse,
            RULE_ANY_IN: self._gen_any_in,
            RULE_ANY_NOT_IN: self._gen_any_not_in,
            RULE_ITEMS: self._gen_items,
        }
        
        handler = dispatch.get(rule.rule_type)
        if handler:
            return handler(field, rule)
        
        return '        /* TODO: Implement rule type %s */\n' % rule.rule_type

    def _generate_oneof_rule_check(self, field: Any, rule: ValidationRule, oneof_name: str, oneof_obj: Any):
        """Generate code for a single oneof member validation rule.
        
        Oneof members are accessed via msg->oneof_name.field_name (or msg->field_name for anonymous oneofs).
        
        Anonymous oneofs (defined with nanopb option 'anonymous_oneof = true') generate C unions
        without a named union field, so their members are accessed directly at the message struct level.
        Non-anonymous oneofs generate a named union field (e.g. msg->values), and their members
        are accessed through this union (e.g. msg->values.field_name).
        """
        field_name = field.name
        # Check if oneof is anonymous (nanopb-specific option that generates C11 anonymous unions)
        anonymous = getattr(oneof_obj, 'anonymous', False)
        
        # Build field access expression based on whether the oneof is anonymous
        if anonymous:
            # Anonymous oneof: fields accessed directly as msg->field_name
            field_access = field_name
        else:
            # Named oneof: fields accessed as msg->oneof_name.field_name
            field_access = '%s.%s' % (oneof_name, field_name)
        
        # Dispatch to appropriate rule handler with oneof context
        rt = rule.rule_type
        
        if rt in (RULE_GT, RULE_GTE, RULE_LT, RULE_LTE, RULE_EQ):
            return self._gen_oneof_numeric_comparison(field, rule, field_access)
        elif rt == RULE_MIN_LEN:
            return self._gen_oneof_min_len(field, rule, field_access)
        elif rt == RULE_MAX_LEN:
            return self._gen_oneof_max_len(field, rule, field_access)
        elif rt == RULE_PREFIX:
            return self._gen_oneof_prefix(field, rule, field_access)
        elif rt == RULE_SUFFIX:
            return self._gen_oneof_suffix(field, rule, field_access)
        elif rt == RULE_CONTAINS:
            return self._gen_oneof_contains(field, rule, field_access)
        elif rt == RULE_ASCII:
            return self._gen_oneof_ascii(field, rule, field_access)
        elif rt in (RULE_EMAIL, RULE_HOSTNAME, RULE_IP, RULE_IPV4, RULE_IPV6):
            return self._gen_oneof_string_format(field, rule, field_access)
        elif rt == RULE_IN:
            return self._gen_oneof_in(field, rule, field_access)
        elif rt == RULE_NOT_IN:
            return self._gen_oneof_not_in(field, rule, field_access)
        elif rt == RULE_ENUM_DEFINED:
            return self._gen_oneof_enum_defined(field, rule, field_access)
        
        return '            /* TODO: Implement oneof rule type %s */\n' % rule.rule_type

    def _gen_oneof_numeric_comparison(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate numeric comparison validation for oneof member."""
        type_map_func = {
            'int32': ('pb_validate_int32', 'int32_t'),
            'sint32': ('pb_validate_int32', 'int32_t'),
            'sfixed32': ('pb_validate_int32', 'int32_t'),
            'int64': ('pb_validate_int64', 'int64_t'),
            'sint64': ('pb_validate_int64', 'int64_t'),
            'sfixed64': ('pb_validate_int64', 'int64_t'),
            'uint32': ('pb_validate_uint32', 'uint32_t'),
            'fixed32': ('pb_validate_uint32', 'uint32_t'),
            'uint64': ('pb_validate_uint64', 'uint64_t'),
            'fixed64': ('pb_validate_uint64', 'uint64_t'),
            'float': ('pb_validate_float', 'float'),
            'double': ('pb_validate_double', 'double'),
            'bool': ('pb_validate_bool', 'bool'),
            'enum': ('pb_validate_enum', 'int')
        }
        constraint_prefix = rule.constraint_id.split('.', 1)[0]
        validator_func, ctype = type_map_func.get(constraint_prefix, (None, None))

        if not validator_func:
            return ''

        value = rule.params.get('value', 0)
        enum_map = {
            RULE_GT: 'PB_VALIDATE_RULE_GT',
            RULE_GTE: 'PB_VALIDATE_RULE_GTE',
            RULE_LT: 'PB_VALIDATE_RULE_LT',
            RULE_LTE: 'PB_VALIDATE_RULE_LTE',
            RULE_EQ: 'PB_VALIDATE_RULE_EQ'
        }
        rule_enum = enum_map.get(rule.rule_type)
        if not rule_enum:
            return ''

        return (
            '            {\n'
            '                %s expected = (%s)%s;\n'
            '                if (!%s(msg->%s, &expected, %s)) {\n'
            '                    pb_violations_add(violations, ctx.path_buffer, "%s", "Value constraint failed");\n'
            '                    if (ctx.early_exit) return false;\n'
            '                }\n'
            '            }\n'
        ) % (ctype, ctype, value, validator_func, field_access, rule_enum, rule.constraint_id)

    def _gen_oneof_min_len(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate min_len validation for oneof string member."""
        min_len = rule.params.get('value', 0)
        if 'string' in rule.constraint_id:
            return (
                '            {\n'
                '                uint32_t min_len = %d;\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), &min_len, PB_VALIDATE_RULE_MIN_LEN)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String too short");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (min_len, field_access, field_access, rule.constraint_id)
        else:  # bytes
            return (
                '            if (msg->%s.size < %d) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too short");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (field_access, min_len, rule.constraint_id)

    def _gen_oneof_max_len(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate max_len validation for oneof string member."""
        max_len = rule.params.get('value', 0)
        if 'string' in rule.constraint_id:
            return (
                '            {\n'
                '                uint32_t max_len = %d;\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), &max_len, PB_VALIDATE_RULE_MAX_LEN)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String too long");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (max_len, field_access, field_access, rule.constraint_id)
        else:  # bytes
            return (
                '            if (msg->%s.size > %d) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too long");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (field_access, max_len, rule.constraint_id)

    def _gen_oneof_prefix(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate prefix validation for oneof string member."""
        prefix = rule.params.get('value', '')
        if 'string' in rule.constraint_id:
            escaped_prefix = self._escape_c_string(prefix)
            return (
                '            {\n'
                '                const char *prefix = "%s";\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), prefix, PB_VALIDATE_RULE_PREFIX)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must start with specified prefix");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (escaped_prefix, field_access, field_access, rule.constraint_id)
        return ''

    def _gen_oneof_suffix(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate suffix validation for oneof string member."""
        suffix = rule.params.get('value', '')
        if 'string' in rule.constraint_id:
            escaped_suffix = self._escape_c_string(suffix)
            return (
                '            {\n'
                '                const char *suffix = "%s";\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), suffix, PB_VALIDATE_RULE_SUFFIX)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must end with specified suffix");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (escaped_suffix, field_access, field_access, rule.constraint_id)
        return ''

    def _gen_oneof_contains(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate contains validation for oneof string member."""
        contains = rule.params.get('value', '')
        if 'string' in rule.constraint_id:
            escaped_contains = self._escape_c_string(contains)
            return (
                '            {\n'
                '                const char *needle = "%s";\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), needle, PB_VALIDATE_RULE_CONTAINS)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain specified substring");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (escaped_contains, field_access, field_access, rule.constraint_id)
        return ''

    def _gen_oneof_ascii(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate ascii validation for oneof string member."""
        if 'string' in rule.constraint_id:
            return (
                '            {\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), NULL, PB_VALIDATE_RULE_ASCII)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain only ASCII characters");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (field_access, field_access, rule.constraint_id)
        return ''

    def _gen_oneof_string_format(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate format validation (email, hostname, ip) for oneof string member."""
        if 'string' in rule.constraint_id:
            enum_map = {
                RULE_EMAIL: 'PB_VALIDATE_RULE_EMAIL',
                RULE_HOSTNAME: 'PB_VALIDATE_RULE_HOSTNAME',
                RULE_IP: 'PB_VALIDATE_RULE_IP',
                RULE_IPV4: 'PB_VALIDATE_RULE_IPV4',
                RULE_IPV6: 'PB_VALIDATE_RULE_IPV6',
            }
            rule_enum = enum_map.get(rule.rule_type)
            if not rule_enum:
                return ''
            return (
                '            {\n'
                '                if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), NULL, %s)) {\n'
                '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String format validation failed");\n'
                '                    if (ctx.early_exit) return false;\n'
                '                }\n'
                '            }\n'
            ) % (field_access, field_access, rule_enum, rule.constraint_id)
        return ''

    def _gen_oneof_in(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate 'in' validation for oneof member."""
        values = rule.params.get('values', [])
        if 'string' in rule.constraint_id:
            escaped_values = [self._escape_c_string(v) for v in values]
            conditions = ['strcmp(msg->%s, "%s") == 0' % (field_access, v) for v in escaped_values]
            condition_str = ' || '.join(conditions)
            return (
                '            if (!(%s)) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (condition_str, rule.constraint_id)
        else:
            conditions = ['msg->%s == %s' % (field_access, v) for v in values]
            condition_str = ' || '.join(conditions)
            return (
                '            if (!(%s)) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of allowed set");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (condition_str, rule.constraint_id)

    def _gen_oneof_not_in(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate 'not_in' validation for oneof member."""
        values = rule.params.get('values', [])
        if 'string' in rule.constraint_id:
            escaped_values = [self._escape_c_string(v) for v in values]
            conditions = ['strcmp(msg->%s, "%s") != 0' % (field_access, v) for v in escaped_values]
            condition_str = ' && '.join(conditions)
            return (
                '            if (!(%s)) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value in forbidden set");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (condition_str, rule.constraint_id)
        else:
            conditions = ['msg->%s != %s' % (field_access, v) for v in values]
            condition_str = ' && '.join(conditions)
            return (
                '            if (!(%s)) {\n'
                '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value in forbidden set");\n'
                '                if (ctx.early_exit) return false;\n'
                '            }\n'
            ) % (condition_str, rule.constraint_id)

    def _gen_oneof_enum_defined(self, field: Any, rule: ValidationRule, field_access: str) -> str:
        """Generate enum defined_only validation for oneof member."""
        if not rule.params.get('value', True):
            return ''
        try:
            enum_vals = []
            ctype = getattr(field, 'ctype', None)
            for e in getattr(self.proto_file, 'enums', []) or []:
                try:
                    if e.names == ctype or str(e.names) == str(ctype):
                        enum_vals = [int(v) for (_, v) in getattr(e, 'values', [])]
                        break
                except Exception:
                    continue
            if enum_vals:
                arr_values = ', '.join(str(v) for v in enum_vals)
                return (
                    '            {\n'
                    '                static const int __pb_defined_vals[] = { %s };\n'
                    '                if (!pb_validate_enum_defined_only((int)msg->%s, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {\n'
                    '                    pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be a defined enum value");\n'
                    '                    if (ctx.early_exit) return false;\n'
                    '                }\n'
                    '            }\n'
                ) % (arr_values, field_access, rule.constraint_id)
        except Exception:
            pass
        return ''


    
    def _generate_message_rule_check(self, message: Any, rule: ValidationRule):
        """Generate code for a message-level validation rule."""
        # This is a simplified version - full implementation would handle all rule types
        return '    /* TODO: Implement message rule type %s */\n' % rule.rule_type
