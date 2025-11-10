#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb validation support module.

This module extends the nanopb generator to support declarative validation
constraints via custom options defined in validate.proto.
"""
from collections import OrderedDict

"""
Import the validate_pb2 module (generated from validate.proto) lazily.
We try a few reasonable locations without polluting sys.path excessively.
"""
_validate_pb2_module = None

from dataclasses import dataclass, field
from typing import Any, Dict, List, Tuple

# --- Module constants -----------------------------------------------------

# validate.rules field number in FieldOptions (extension) as used by protoc-gen-validate
VALIDATE_EXTENSION_FIELD: int = 1011

# Supported numeric rule types in FieldRules
NUMERIC_TYPES: Tuple[str, ...] = (
    'int64', 'uint32', 'uint64', 'sint32', 'sint64',
    'fixed32', 'fixed64', 'sfixed32', 'sfixed64', 'float', 'double'
)

# Mapping of string constraints to generic rule identifiers
STRING_CONSTRAINT_MAP: Dict[str, str] = {
    'const': 'EQ',
    'min_len': 'MIN_LEN',
    'max_len': 'MAX_LEN',
    'prefix': 'PREFIX',
    'suffix': 'SUFFIX',
    'contains': 'CONTAINS',
    'ascii': 'ASCII',
    'in': 'IN',
    'not_in': 'NOT_IN',
    'email': 'EMAIL', 
    'hostname': 'HOSTNAME', 
    'ip': 'IP', 
    'ipv4': 'IPV4', 
    'ipv6': 'IPV6', 
}

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
RULE_NO_SPARSE = 'NO_SPARSE'
RULE_ANY_IN = 'ANY_IN'
RULE_ANY_NOT_IN = 'ANY_NOT_IN'


# --- Small helpers --------------------------------------------------------

def _has_attr_and_truthy(obj: Any, name: str) -> bool:
    """Return True if object has attribute and its value is truthy (e.g., non-empty list)."""
    try:
        return hasattr(obj, name) and bool(getattr(obj, name))
    except Exception:
        return False

def _get_repeated_values(obj: Any, name: str) -> List[Any]:
    """Return list of values for a repeated field, or empty list if not present."""
    try:
        vals = getattr(obj, name)
        return list(vals) if vals else []
    except Exception:
        return []


def load_validate_pb2():
    """Load the compiled validate.proto definitions or return None if unavailable.

    Resolution order:
    1. Relative import from generator/proto (".proto.validate_pb2")
    2. Absolute-ish import when run from generator/ ("proto.validate_pb2")
    3. Top-level module ("validate_pb2")
    
    If imports fail due to protobuf version mismatch, tries setting
    PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python as a workaround.
    """
    global _validate_pb2_module
    if _validate_pb2_module is not None:
        return _validate_pb2_module

    import os
    
    # Helper to try import with optional protobuf workaround
    def try_import_with_workaround(import_func):
        try:
            return import_func()
        except (TypeError, AttributeError) as e:
            error_msg = str(e)
            if "Descriptors cannot be created directly" in error_msg or "module has no attribute" in error_msg:
                # Protobuf version mismatch - try pure Python implementation
                old_impl = os.environ.get('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION')
                try:
                    os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'python'
                    return import_func()
                finally:
                    if old_impl is None:
                        os.environ.pop('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION', None)
                    else:
                        os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = old_impl
            raise
        except Exception:
            return None
    
    # Try relative import from generator/proto first
    def try_relative():
        from .proto import validate_pb2 as _vp
        return _vp
    
    result = try_import_with_workaround(try_relative)
    if result:
        _validate_pb2_module = result
        return _validate_pb2_module

    # Try plain "proto.validate_pb2" (when run as script from generator/)
    def try_proto():
        from proto import validate_pb2 as _vp
        return _vp
    
    result = try_import_with_workaround(try_proto)
    if result:
        _validate_pb2_module = result
        return _validate_pb2_module

    # As a last resort, try importing a top-level validate_pb2
    def try_toplevel():
        import validate_pb2 as _vp
        return _vp
    
    result = try_import_with_workaround(try_toplevel)
    if result:
        _validate_pb2_module = result
        return _validate_pb2_module
    
    # Keep quiet: validation is optional and we have a fallback parser
    return None

# Note: We used to inspect private unknown_fields to extract rules. That is brittle
# across protobuf versions. Instead, we now rely on scanning serialized options to
# find the validate.rules payload (extension field number 1011).

def _read_varint(data: bytes, i: int) -> Tuple[int, int]:
    """Read a varint from data starting at index i.

    Returns a tuple (value, new_index).
    """
    result = 0
    shift = 0
    while i < len(data):
        b = data[i]
        i += 1
        result |= (b & 0x7F) << shift
        if not (b & 0x80):
            break
        shift += 7
    return result, i

def parse_validation_rules_from_serialized_options(field_options: Any) -> Dict[str, Dict[str, Any]]:
    """Extract validate.rules (field 1011) from serialized FieldOptions bytes.

    Returns a dict of parsed rules using parse_field_rules_from_protobuf_message.
    """
    try:
        data = field_options.SerializeToString()
    except Exception:
        return {}

    i = 0
    target_field = VALIDATE_EXTENSION_FIELD
    validate_rules_bytes = None

    while i < len(data):
        # key
        key, i = _read_varint(data, i)
        field_number = key >> 3
        wire_type = key & 0x07

        if wire_type == 0:  # varint
            _, i = _read_varint(data, i)
        elif wire_type == 1:  # 64-bit
            i += 8
        elif wire_type == 2:  # length-delimited
            length, i = _read_varint(data, i)
            if i + length > len(data):
                break
            value = data[i:i+length]
            i += length
            if field_number == target_field:
                validate_rules_bytes = value
                break
        elif wire_type == 5:  # 32-bit
            i += 4
        else:
            # groups and other types not expected
            break

    if not validate_rules_bytes:
        return {}

    # Use validate_pb2.FieldRules to parse payload, then convert to dict
    try:
        vp = load_validate_pb2()
        if not vp:
            return {}
        msg = vp.FieldRules()
        msg.MergeFromString(validate_rules_bytes)
        return parse_field_rules_from_protobuf_message(msg)
    except Exception:
        return {}

def parse_field_rules_from_protobuf_message(field_rules_msg: Any) -> Dict[str, Dict[str, Any]]:
    """Parse FieldRules from a protobuf message object into a nested dict.

    Result format example:
        { 'int32': {'gte': 18, 'lt': 100}, 'required': True }
    """
    rules: Dict[str, Dict[str, Any]] = {}
    if not field_rules_msg:
        return rules

    try:
        # string rules
        if hasattr(field_rules_msg, 'string') and field_rules_msg.HasField('string'):
            string_rules = field_rules_msg.string
            s: Dict[str, Any] = {}
            # For non-boolean fields, use HasField
            for attr in ('min_len', 'max_len', 'const', 'prefix', 'suffix', 'contains'):
                if string_rules.HasField(attr):
                    s[attr] = getattr(string_rules, attr)
            # For boolean fields, check the value directly (HasField doesn't work for primitives)
            for attr in ('ascii', 'email', 'hostname', 'ip', 'ipv4', 'ipv6'):
                if hasattr(string_rules, attr):
                    val = getattr(string_rules, attr)
                    if val:  # Only include if True
                        s[attr] = val
            # repeated constraints
            if _has_attr_and_truthy(string_rules, 'in'):
                s['in'] = _get_repeated_values(string_rules, 'in')
            if _has_attr_and_truthy(string_rules, 'not_in'):
                s['not_in'] = _get_repeated_values(string_rules, 'not_in')
            if s:
                rules['string'] = s

        # int32 rules
        if hasattr(field_rules_msg, 'int32') and field_rules_msg.HasField('int32'):
            int32_rules = field_rules_msg.int32
            i32: Dict[str, Any] = {}
            for attr in ('gt', 'gte', 'lt', 'lte', 'const'):
                if int32_rules.HasField(attr):
                    i32[attr] = getattr(int32_rules, attr)
            if i32:
                rules['int32'] = i32

        # other numeric types
        for numeric_type in NUMERIC_TYPES:
            if hasattr(field_rules_msg, numeric_type) and field_rules_msg.HasField(numeric_type):
                numeric_rules = getattr(field_rules_msg, numeric_type)
                nr: Dict[str, Any] = {}
                for constraint in ('gt', 'gte', 'lt', 'lte', 'const'):
                    if numeric_rules.HasField(constraint):
                        nr[constraint] = getattr(numeric_rules, constraint)
                if _has_attr_and_truthy(numeric_rules, 'in'):
                    nr['in'] = _get_repeated_values(numeric_rules, 'in')
                if _has_attr_and_truthy(numeric_rules, 'not_in'):
                    nr['not_in'] = _get_repeated_values(numeric_rules, 'not_in')
                if nr:
                    rules[numeric_type] = nr

        # bool rules
        if hasattr(field_rules_msg, 'bool') and field_rules_msg.HasField('bool'):
            bool_rules = field_rules_msg.bool
            br: Dict[str, Any] = {}
            if bool_rules.HasField('const'):
                br['const'] = bool_rules.const
            if br:
                rules['bool'] = br

        # bytes rules
        if hasattr(field_rules_msg, 'bytes') and field_rules_msg.HasField('bytes'):
            bytes_rules = field_rules_msg.bytes
            b: Dict[str, Any] = {}
            for attr in ('min_len', 'max_len', 'const', 'prefix', 'suffix', 'contains'):
                if bytes_rules.HasField(attr):
                    b[attr] = getattr(bytes_rules, attr)
            if _has_attr_and_truthy(bytes_rules, 'in'):
                b['in'] = _get_repeated_values(bytes_rules, 'in')
            if _has_attr_and_truthy(bytes_rules, 'not_in'):
                b['not_in'] = _get_repeated_values(bytes_rules, 'not_in')
            if b:
                rules['bytes'] = b

        # repeated rules
        if hasattr(field_rules_msg, 'repeated') and field_rules_msg.HasField('repeated'):
            rep_rules = field_rules_msg.repeated
            r: Dict[str, Any] = {}
            if rep_rules.HasField('min_items'):
                r['min_items'] = rep_rules.min_items
            if rep_rules.HasField('max_items'):
                r['max_items'] = rep_rules.max_items
            if rep_rules.HasField('unique'):
                r['unique'] = rep_rules.unique
            if r:
                rules['repeated'] = r

        # map rules
        if hasattr(field_rules_msg, 'map') and field_rules_msg.HasField('map'):
            map_rules = field_rules_msg.map
            m: Dict[str, Any] = {}
            if map_rules.HasField('min_pairs'):
                m['min_pairs'] = map_rules.min_pairs
            if map_rules.HasField('max_pairs'):
                m['max_pairs'] = map_rules.max_pairs
            if map_rules.HasField('no_sparse'):
                m['no_sparse'] = map_rules.no_sparse
            if m:
                rules['map'] = m

        # any rules (google.protobuf.Any)
        if hasattr(field_rules_msg, 'any') and field_rules_msg.HasField('any'):
            any_rules = field_rules_msg.any
            a: Dict[str, Any] = {}
            if _has_attr_and_truthy(any_rules, 'in'):
                a['in'] = _get_repeated_values(any_rules, 'in')
            if _has_attr_and_truthy(any_rules, 'not_in'):
                a['not_in'] = _get_repeated_values(any_rules, 'not_in')
            if a:
                rules['any'] = a

        # flags
        if hasattr(field_rules_msg, 'required') and field_rules_msg.HasField('required'):
            rules['required'] = field_rules_msg.required
        if hasattr(field_rules_msg, 'oneof_required') and field_rules_msg.HasField('oneof_required'):
            rules['oneof_required'] = field_rules_msg.oneof_required

    except Exception:
        # If parsing fails, return what we have
        pass

    return rules

# Removed deprecated unknown-field parsing helpers; serialized-options scanning is used instead.

@dataclass(frozen=True)
class ValidationRule:
    """Represents a single validation rule."""
    rule_type: str
    constraint_id: str
    params: Dict[str, Any] = field(default_factory=dict)

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
        # Strategy: Prefer canonical parsing from serialized descriptor, because
        # extension presence and python-protobuf version differences can mask
        # submessage presence. Only if descriptor path unavailable do we fall
        # back to direct rules_option inspection.

        parsed_from_serialized = False
        if self.proto_file and self.message_desc and hasattr(self.message_desc, 'desc'):
            for field_desc in self.message_desc.desc.field:
                if field_desc.name == self.field.name:
                    parsed_rules = parse_validation_rules_from_serialized_options(field_desc.options)
                    if parsed_rules:
                        self._add_parsed_rules(parsed_rules)
                        parsed_from_serialized = True
                    break

        if not parsed_from_serialized and rules_option:
            _ = load_validate_pb2()
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
    
    def _add_parsed_rules(self, parsed_rules: Dict[str, Dict[str, Any]]):
        """Add parsed rules to the validator."""
        for rule_type, rule_data in parsed_rules.items():
            if rule_type == 'string' and isinstance(rule_data, dict):
                for constraint, value in rule_data.items():
                    # Map to generic rule types handled by _generate_rule_check
                    rtype = STRING_CONSTRAINT_MAP.get(constraint)
                    if not rtype:
                        continue
                    params: Dict[str, Any] = {}
                    if constraint in ('const','min_len','max_len','prefix','suffix','contains'):
                        params['value'] = value
                    elif constraint in ('in','not_in'):
                        params['values'] = list(value)
                    # Boolean flags: email, hostname, ip, ipv4, ipv6, ascii - only add if True
                    elif constraint in ('email', 'hostname', 'ip', 'ipv4', 'ipv6', 'ascii'):
                        if not value:  # Skip if False
                            continue
                    self.rules.append(ValidationRule(rtype, f'string.{constraint}', params))
            elif rule_type in (('int32',) + NUMERIC_TYPES) and isinstance(rule_data, dict):
                for constraint, value in rule_data.items():
                    cname = constraint.upper()
                    if constraint in ('gt','gte','lt','lte'):
                        self.rules.append(ValidationRule(cname, f'{rule_type}.{constraint}', {'value': value}))
                    elif constraint == 'const':
                        self.rules.append(ValidationRule(RULE_EQ, f'{rule_type}.const', {'value': value}))
            elif rule_type == 'repeated' and isinstance(rule_data, dict):
                if 'min_items' in rule_data:
                    self.rules.append(ValidationRule(RULE_MIN_ITEMS, 'repeated.min_items', {'value': rule_data['min_items']}))
                if 'max_items' in rule_data:
                    self.rules.append(ValidationRule(RULE_MAX_ITEMS, 'repeated.max_items', {'value': rule_data['max_items']}))
                if rule_data.get('unique'):
                    self.rules.append(ValidationRule(RULE_UNIQUE, 'repeated.unique', {}))
            elif rule_type == 'map' and isinstance(rule_data, dict):
                if 'min_pairs' in rule_data:
                    self.rules.append(ValidationRule(RULE_MIN_ITEMS, 'map.min_pairs', {'value': rule_data['min_pairs']}))
                if 'max_pairs' in rule_data:
                    self.rules.append(ValidationRule(RULE_MAX_ITEMS, 'map.max_pairs', {'value': rule_data['max_pairs']}))
                if rule_data.get('no_sparse'):
                    self.rules.append(ValidationRule(RULE_NO_SPARSE, 'map.no_sparse', {}))
            elif rule_type == 'any' and isinstance(rule_data, dict):
                if 'in' in rule_data:
                    self.rules.append(ValidationRule(RULE_ANY_IN, 'any.in', {'values': list(rule_data['in'])}))
                if 'not_in' in rule_data:
                    self.rules.append(ValidationRule(RULE_ANY_NOT_IN, 'any.not_in', {'values': list(rule_data['not_in'])}))
            elif rule_type == 'required':
                self.rules.append(ValidationRule(RULE_REQUIRED, 'required', {'required': rule_data}))
            elif rule_type == 'oneof_required':
                # Record oneof-required; enforcement emitted similarly to REQUIRED but semantic differs.
                if rule_data:
                    self.rules.append(ValidationRule(RULE_ONEOF_REQUIRED, 'oneof_required', {'required': rule_data}))
    
    def _parse_numeric_rules(self, rules, type_name):
        """Parse numeric validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule(RULE_EQ, f'{type_name}.const', {'value': rules.const}))
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
        if rules.HasField('const'):
            self.rules.append(ValidationRule(RULE_EQ, 'bool.const', {'value': rules.const}))
    
    def _parse_string_rules(self, rules: Any):
        """Parse string validation rules."""
        if rules.HasField('const'):
            self.rules.append(ValidationRule(RULE_EQ, 'string.const', {'value': rules.const}))
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
        if rules.HasField('const'):
            self.rules.append(ValidationRule(RULE_EQ, 'bytes.const', {'value': rules.const}))
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
        if rules.HasField('const'):
            self.rules.append(ValidationRule(RULE_EQ, 'enum.const', {'value': rules.const}))
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
            # TODO: Handle per-item validation rules
            pass
    
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
            # Without constraints
            for fname in fields_without:
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
                                yield '    {\n'
                                yield '        /* Recurse into submessage */\n'
                                yield '        if (msg->%s) {\n' % field_name
                                yield '            bool ok_nested = %s(msg->%s, violations);\n' % (sub_func, field_name)
                                yield '            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                yield '        }\n'
                                yield '    }\n'
                            else:
                                # Inline submessage struct
                                if rules == 'OPTIONAL':
                                    yield '    {\n'
                                    yield '        /* Recurse into submessage (optional) */\n'
                                    yield '        if (msg->has_%s) {\n' % field_name
                                    yield '            bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, field_name)
                                    yield '            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                    yield '        }\n'
                                    yield '    }\n'
                                else:
                                    yield '    {\n'
                                    yield '        /* Recurse into submessage (singular) */\n'
                                    yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, field_name)
                                    yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                                    yield '    }\n'
                except Exception:
                    # If field shape is unexpected, skip recursion silently
                    pass
                
                yield '    pb_validate_context_pop_field(&ctx);\n'
                yield '    \n'
            
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
                    yield '    if (!pb_validate_context_push_field(&ctx, "%s")) return false;\n' % fname
                    if allocation == 'POINTER':
                        yield '    {\n'
                        yield '        /* Recurse into submessage */\n'
                        yield '        if (msg->%s) {\n' % fname
                        yield '            bool ok_nested = %s(msg->%s, violations);\n' % (sub_func, fname)
                        yield '            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                        yield '        }\n'
                        yield '    }\n'
                    else:
                        if rules == 'OPTIONAL':
                            yield '    {\n'
                            yield '        /* Recurse into submessage (optional) */\n'
                            yield '        if (msg->has_%s) {\n' % fname
                            yield '            bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, fname)
                            yield '            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                            yield '        }\n'
                            yield '    }\n'
                        else:
                            yield '    {\n'
                            yield '        /* Recurse into submessage (singular) */\n'
                            yield '        bool ok_nested = %s(&msg->%s, violations);\n' % (sub_func, fname)
                            yield '        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }\n'
                            yield '    }\n'
                    yield '    pb_validate_context_pop_field(&ctx);\n'
                    yield '    \n'
                except Exception:
                    # Skip if field metadata is unexpected
                    pass

            # Generate message-level validations
            for rule in validator.message_rules:
                yield '    /* Message rule: %s */\n' % rule.constraint_id
                yield self._generate_message_rule_check(validator.message, rule)
                yield '    \n'
            
            yield '    return !pb_violations_has_any(violations);\n'
            yield '}\n'
            yield '\n'
    
    def _generate_rule_check(self, field: Any, rule: ValidationRule):
        """Generate code for a single field validation rule."""
        field_name = field.name
        
        def wrap_optional(code: str) -> str:
            """If the field is OPTIONAL, guard checks with has_ flag.
            Keeps indentation consistent by indenting the payload one level.
            """
            try:
                is_optional = (getattr(field, 'rules', None) == 'OPTIONAL')
            except Exception:
                is_optional = False
            if not is_optional:
                return code
            # Indent code by 4 spaces for readability
            indented = ''.join(('    ' + line) if line.strip() else line for line in code.splitlines(True))
            return '        if (msg->has_%s) {\n%s        }\n' % (field_name, indented)

        # Helper: map rule constraint prefix to C validator function & C type
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
        
        if rule.rule_type == RULE_REQUIRED:
            if field.rules == 'OPTIONAL':
                return '        if (!msg->has_%s) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Field is required");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (field_name, rule.constraint_id)
        
        # Numeric / enum primitive constraints via runtime helpers
        elif rule.rule_type in (RULE_GT, RULE_GTE, RULE_LT, RULE_LTE, RULE_EQ) and validator_func:
            value = rule.params.get('value', 0)
            enum_map = {
                RULE_GT: 'PB_VALIDATE_RULE_GT',
                RULE_GTE: 'PB_VALIDATE_RULE_GTE',
                RULE_LT: 'PB_VALIDATE_RULE_LT',
                RULE_LTE: 'PB_VALIDATE_RULE_LTE',
                RULE_EQ: 'PB_VALIDATE_RULE_EQ'
            }
            rule_enum = enum_map[rule.rule_type]
            # Special handling for string & bytes delegated later
            if constraint_prefix not in ('string', 'bytes'):
                # bool types: use bool literal; others: typed variable
                if constraint_prefix == 'bool':
                    init_line = '%s expected = %s;' % (ctype, 'true' if bool(value) else 'false')
                else:
                    init_line = '%s expected = (%s)%s;' % (ctype, ctype, value)
                body = (
                    '        {\n'
                    '            %s\n'
                    '            if (!%s(msg->%s, &expected, %s)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value constraint failed");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n'
                ) % (init_line, validator_func, field_name, rule_enum, rule.constraint_id)
                return wrap_optional(body)
        
        elif rule.rule_type == RULE_IN:
            values = rule.params.get('values', [])
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
                values_str = ', '.join('"%s"' % v for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
            else:
                conditions = ['msg->%s == %s' % (field_name, v) for v in values]
                condition_str = ' || '.join(conditions)
                values_str = ', '.join(str(v) for v in values)
                return wrap_optional('        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str))
        
        elif rule.rule_type == RULE_NOT_IN:
            values = rule.params.get('values', [])
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
                values_str = ', '.join('"%s"' % v for v in values)
                return '        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str)
            else:
                conditions = ['msg->%s != %s' % (field_name, v) for v in values]
                condition_str = ' && '.join(conditions)
                values_str = ', '.join(str(v) for v in values)
                return wrap_optional('        if (!(%s)) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Value must not be one of: %s");\n' \
                       '            if (ctx.early_exit) return false;\n' \
                       '        }\n' % (condition_str, rule.constraint_id, values_str))
        
        # String-specific constraints
        elif rule.rule_type == RULE_MIN_LEN:
            min_len = rule.params.get('value', 0)
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                uint32_t min_len = %d;\n'
                        '                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String too short");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, min_len, rule.constraint_id)
                return wrap_optional(
                    '        {\n'
                    '            uint32_t min_len_v = %d;\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String too short");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (min_len, field_name, field_name, rule.constraint_id)
                )
            else:  # bytes (still inline for now – future: pb_validate_bytes)
                return wrap_optional(
                    '        if (msg->%s.size < %d) {\n'
                    '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too short");\n'
                    '            if (ctx.early_exit) return false;\n'
                    '        }\n' % (field_name, min_len, rule.constraint_id)
                )
        
        elif rule.rule_type == RULE_MAX_LEN:
            max_len = rule.params.get('value', 0)
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                uint32_t max_len_v = %d;\n'
                        '                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String too long");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, max_len, rule.constraint_id)
                return wrap_optional(
                    '        {\n'
                    '            uint32_t max_len_v = %d;\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String too long");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (max_len, field_name, field_name, rule.constraint_id)
                )
            else:  # bytes
                return wrap_optional(
                    '        if (msg->%s.size > %d) {\n'
                    '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes too long");\n'
                    '            if (ctx.early_exit) return false;\n'
                    '        }\n' % (field_name, max_len, rule.constraint_id)
                )
        
        elif rule.rule_type == RULE_PREFIX:
            prefix = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                const char *prefix = "%s";\n'
                        '                if (!pb_validate_string(s, l, prefix, PB_VALIDATE_RULE_PREFIX)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must start with \'%s\'");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, prefix, rule.constraint_id, prefix)
                return wrap_optional(
                    '        {\n'
                    '            const char *prefix = "%s";\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), prefix, PB_VALIDATE_RULE_PREFIX)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must start with \'%s\'");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (prefix, field_name, field_name, rule.constraint_id, prefix)
                )
            else:  # bytes
                return wrap_optional('        if (msg->%s.size < %d || memcmp(msg->%s.bytes, "%s", %d) != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must start with specified prefix");\n' \
                       '            if (ctx.early_exit) return false;\n' \
              '        }\n' % (field_name, len(prefix), field_name, prefix, len(prefix), rule.constraint_id))
        
        elif rule.rule_type == RULE_SUFFIX:
            suffix = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                const char *suffix = "%s";\n'
                        '                if (!pb_validate_string(s, l, suffix, PB_VALIDATE_RULE_SUFFIX)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must end with \'%s\'");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, suffix, rule.constraint_id, suffix)
                return wrap_optional(
                    '        {\n'
                    '            const char *suffix = "%s";\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), suffix, PB_VALIDATE_RULE_SUFFIX)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must end with \'%s\'");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (suffix, field_name, field_name, rule.constraint_id, suffix)
                )
            else:  # bytes
                return wrap_optional('        if (msg->%s.size < %d || memcmp(msg->%s.bytes + msg->%s.size - %d, "%s", %d) != 0) {\n' \
                       '            pb_violations_add(violations, ctx.path_buffer, "%s", "Bytes must end with specified suffix");\n' \
                       '            if (ctx.early_exit) return false;\n' \
              '        }\n' % (field_name, len(suffix), field_name, field_name, len(suffix), suffix, len(suffix), rule.constraint_id))
        
        elif rule.rule_type == RULE_CONTAINS:
            contains = rule.params.get('value', '')
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                const char *needle = "%s";\n'
                        '                if (!pb_validate_string(s, l, needle, PB_VALIDATE_RULE_CONTAINS)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain \'%s\'");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, contains, rule.constraint_id, contains)
                return wrap_optional(
                    '        {\n'
                    '            const char *needle = "%s";\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), needle, PB_VALIDATE_RULE_CONTAINS)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain \'%s\'");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (contains, field_name, field_name, rule.constraint_id, contains)
                )
            else:  # bytes  
                return wrap_optional('        {\n' \
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
        
        elif rule.rule_type == RULE_ASCII:
            if 'string' in rule.constraint_id:
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_ASCII)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain only ASCII characters");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, rule.constraint_id)
                return wrap_optional(
                    '        {\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), NULL, PB_VALIDATE_RULE_ASCII)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String must contain only ASCII characters");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (field_name, field_name, rule.constraint_id)
                )
        elif rule.rule_type in (RULE_EMAIL, RULE_HOSTNAME, RULE_IP, RULE_IPV4, RULE_IPV6):
            if 'string' in rule.constraint_id:
                enum_map = {
                    RULE_EMAIL: 'PB_VALIDATE_RULE_EMAIL',
                    RULE_HOSTNAME: 'PB_VALIDATE_RULE_HOSTNAME',
                    RULE_IP: 'PB_VALIDATE_RULE_IP',
                    RULE_IPV4: 'PB_VALIDATE_RULE_IPV4',
                    RULE_IPV6: 'PB_VALIDATE_RULE_IPV6',
                }
                rule_enum = enum_map[rule.rule_type]
                if getattr(field, 'allocation', None) == 'CALLBACK':
                    return (
                        '        {\n'
                        '            const char *s = NULL; pb_size_t l = 0;\n'
                        '            if (pb_read_callback_string(&msg->%s, &s, &l)) {\n'
                        '                if (!pb_validate_string(s, l, NULL, %s)) {\n'
                        '                    pb_violations_add(violations, ctx.path_buffer, "%s", "String format validation failed");\n'
                        '                    if (ctx.early_exit) return false;\n'
                        '                }\n'
                        '            }\n'
                        '        }\n'
                    ) % (field_name, rule_enum, rule.constraint_id)
                return wrap_optional(
                    '        {\n'
                    '            if (!pb_validate_string(msg->%s, (pb_size_t)strlen(msg->%s), NULL, %s)) {\n'
                    '                pb_violations_add(violations, ctx.path_buffer, "%s", "String format validation failed");\n'
                    '                if (ctx.early_exit) return false;\n'
                    '            }\n'
                    '        }\n' % (field_name, field_name, rule_enum, rule.constraint_id)
                )
        
        # Enum constraints
        elif rule.rule_type == RULE_ENUM_DEFINED:
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
                    body = (
                        '        {\n'
                        '            static const int __pb_defined_vals[] = { %s };\n'
                        '            if (!pb_validate_enum_defined_only((int)msg->%s, __pb_defined_vals, (pb_size_t)(sizeof(__pb_defined_vals)/sizeof(__pb_defined_vals[0])))) {\n'
                        '                pb_violations_add(violations, ctx.path_buffer, "%s", "Value must be a defined enum value");\n'
                        '                if (ctx.early_exit) return false;\n'
                        '            }\n'
                        '        }\n'
                    ) % (arr_values, field_name, rule.constraint_id)
                    return wrap_optional(body)
            except Exception:
                # Fallback: no-op if enum definition not found
                pass
            return ''
        
        # Repeated field constraints
        elif rule.rule_type == RULE_MIN_ITEMS:
            min_items = rule.params.get('value', 0)
            return (
                '        if (!pb_validate_min_items(msg->%s_count, %d)) {\n'
                '            pb_violations_add(violations, ctx.path_buffer, "%s", "Too few items");\n'
                '            if (ctx.early_exit) return false;\n'
                '        }\n'
            ) % (field_name, min_items, rule.constraint_id)
        
        elif rule.rule_type == RULE_MAX_ITEMS:
            max_items = rule.params.get('value', 0)
            return (
                '        if (!pb_validate_max_items(msg->%s_count, %d)) {\n'
                '            pb_violations_add(violations, ctx.path_buffer, "%s", "Too many items");\n'
                '            if (ctx.early_exit) return false;\n'
                '        }\n'
            ) % (field_name, max_items, rule.constraint_id)
        
        elif rule.rule_type == RULE_UNIQUE:
            # Simplified unique check - would need type-specific implementation
            return '        /* TODO: Implement unique constraint for repeated field */\n'
        
        elif rule.rule_type == RULE_NO_SPARSE:
            return '        /* TODO: Implement no_sparse constraint for map field */\n'
        
        # google.protobuf.Any type_url constraints
        elif rule.rule_type == RULE_ANY_IN:
            values = rule.params.get('values', [])
            if not values:
                return ''
            
            # Generate code to check if type_url is in the allowed list
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
        
        elif rule.rule_type == RULE_ANY_NOT_IN:
            values = rule.params.get('values', [])
            if not values:
                return ''
            
            # Generate code to check if type_url is NOT in the disallowed list
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
        
        # Default fallback
        return '        /* TODO: Implement rule type %s */\n' % rule.rule_type
    
    def _generate_message_rule_check(self, message: Any, rule: ValidationRule):
        """Generate code for a message-level validation rule."""
        # This is a simplified version - full implementation would handle all rule types
        return '    /* TODO: Implement message rule type %s */\n' % rule.rule_type
