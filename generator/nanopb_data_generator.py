#!/usr/bin/env python3
# kate: replace-tabs on; indent-width 4;

"""
nanopb test data generator.

This module generates valid and invalid protobuf test data based on
validation constraints defined in .proto files using validate.proto.

The generator can output:
- Binary protobuf data
- C array initializers
- Python dictionaries for inspection

Example usage:
    generator = DataGenerator('test.proto')
    valid_data = generator.generate_valid('BasicValidation')
    invalid_data = generator.generate_invalid('BasicValidation', 'age')
"""

import os
import sys
import struct
import random
import string
from typing import Any, Dict, List, Optional, Tuple, Union
from dataclasses import dataclass
from enum import Enum

# Prefer pure-Python protobuf runtime to support loading generated *_pb2
# files built with older protoc versions (avoids descriptor runtime errors).
os.environ.setdefault('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION', 'python')

# Import protobuf libraries
try:
    from google.protobuf import descriptor_pb2
    from google.protobuf.message import Message
except ImportError:
    sys.stderr.write("Error: protobuf library required. Install with: pip install protobuf\n")
    sys.exit(1)

# Import nanopb modules
try:
    from . import nanopb_validator
    from .proto._utils import invoke_protoc
    from .proto import TemporaryDirectory
    from .proto import validate_pb2
except ImportError:
    import nanopb_validator
    from proto._utils import invoke_protoc
    from proto import TemporaryDirectory
    try:
        from proto import validate_pb2
    except ImportError:
        # validate_pb2 may not exist yet; will be built on demand
        validate_pb2 = None


class OutputFormat(Enum):
    """Output format for generated data."""
    BINARY = "binary"
    C_ARRAY = "c_array"
    PYTHON_DICT = "python_dict"
    HEX_STRING = "hex_string"


@dataclass
class ValidationConstraint:
    """Represents a validation constraint for a field."""
    field_name: str
    field_type: str
    rule_type: str
    value: Any
    
    def __repr__(self):
        return f"ValidationConstraint({self.field_name}, {self.rule_type}={self.value})"


class ProtoFieldInfo:
    """Information about a protobuf field."""
    
    def __init__(self, field_desc, file_descriptor=None):
        self.name = field_desc.name
        self.number = field_desc.number
        self.type = field_desc.type
        self.type_name = field_desc.type_name
        self.label = field_desc.label
        self.descriptor = field_desc
        self.file_descriptor = file_descriptor
        
        # Parse validation rules
        self.constraints = []
        if hasattr(field_desc, 'options') and field_desc.options:
            self._parse_validation_rules(field_desc.options)
    
    def _parse_validation_rules(self, field_options):
        """Parse validation rules from field options using validate_pb2."""
        try:
            # First, try to import validate_pb2 if not already imported
            global validate_pb2
            if validate_pb2 is None:
                try:
                    from proto import validate_pb2 as vpb2
                    validate_pb2 = vpb2
                except ImportError:
                    try:
                        import validate_pb2 as vpb2
                        validate_pb2 = vpb2
                    except ImportError:
                        # Can't import, skip validation parsing
                        return
            
            # Check if the validate.rules extension exists
            if not hasattr(validate_pb2, 'rules'):
                return
                
            # Check if field options has the extension
            if not field_options.HasExtension(validate_pb2.rules):
                return
            
            # Get the validation rules
            rules = field_options.Extensions[validate_pb2.rules]
            
            # Parse rules based on type
            type_name = self.get_type_name()
            
            # Numeric types
            if type_name in ['int32', 'int64', 'uint32', 'uint64', 'sint32', 'sint64', 
                            'fixed32', 'fixed64', 'sfixed32', 'sfixed64', 'float', 'double']:
                self._parse_numeric_rules(rules, type_name)
            # String type
            elif type_name == 'string':
                self._parse_string_rules(rules)
            # Bytes type
            elif type_name == 'bytes':
                self._parse_bytes_rules(rules)
            # Bool type
            elif type_name == 'bool':
                self._parse_bool_rules(rules)
            # Enum type
            elif type_name == 'enum':
                self._parse_enum_rules(rules)
            # Any type (google.protobuf.Any)
            elif type_name == 'message' and '.google.protobuf.Any' in self.type_name:
                self._parse_any_rules(rules)
            
            # Repeated field rules (applies to any repeated field)
            if self.is_repeated() and rules.HasField('repeated'):
                self._parse_repeated_rules(rules.repeated)
                
        except Exception as e:
            # Validation parsing is optional, fail silently
            # But for debugging, uncomment this:
            # import traceback
            # print(f"Warning: Failed to parse validation rules for {self.name}: {e}", file=sys.stderr)
            # traceback.print_exc()
            pass
    
    def _parse_numeric_rules(self, rules, type_name):
        """Parse numeric validation rules."""
        # Get the rules for this specific numeric type
        if not rules.HasField(type_name):
            return
        numeric_rules = getattr(rules, type_name)
        
        # Parse const (use const_value field name)
        if numeric_rules.HasField('const_value'):
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'const', numeric_rules.const_value)
            )
        
        # Parse less than
        if numeric_rules.HasField('lt'):
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'lt', numeric_rules.lt)
            )
        
        # Parse less than or equal
        if numeric_rules.HasField('lte'):
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'lte', numeric_rules.lte)
            )
        
        # Parse greater than
        if numeric_rules.HasField('gt'):
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'gt', numeric_rules.gt)
            )
        
        # Parse greater than or equal
        if numeric_rules.HasField('gte'):
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'gte', numeric_rules.gte)
            )
        
        # Parse in list (use getattr since 'in' is a Python keyword)
        in_list = getattr(numeric_rules, 'in', [])
        if len(in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'in', list(in_list))
            )
        
        # Parse not_in list
        not_in_list = getattr(numeric_rules, 'not_in', [])
        if len(not_in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, type_name, 'not_in', list(not_in_list))
            )
    
    def _parse_string_rules(self, rules):
        """Parse string validation rules."""
        if not rules.HasField('string'):
            return
        string_rules = rules.string
        
        # Length constraints
        if string_rules.HasField('min_len'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'min_len', string_rules.min_len)
            )
        
        if string_rules.HasField('max_len'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'max_len', string_rules.max_len)
            )
        
        # Pattern constraints
        if string_rules.HasField('prefix'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'prefix', string_rules.prefix)
            )
        
        if string_rules.HasField('suffix'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'suffix', string_rules.suffix)
            )
        
        if string_rules.HasField('contains'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'contains', string_rules.contains)
            )
        
        # Const
        if string_rules.HasField('const_value'):
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'const', string_rules.const_value)
            )
        
        # Character set
        if string_rules.HasField('ascii') and string_rules.ascii:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'ascii', True)
            )
        
        # Format validation
        if string_rules.HasField('email') and string_rules.email:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'email', True)
            )
        
        if string_rules.HasField('hostname') and string_rules.hostname:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'hostname', True)
            )
        
        if string_rules.HasField('ip') and string_rules.ip:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'ip', True)
            )
        
        if string_rules.HasField('ipv4') and string_rules.ipv4:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'ipv4', True)
            )
        
        if string_rules.HasField('ipv6') and string_rules.ipv6:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'ipv6', True)
            )
        
        # Set membership (use getattr since 'in' is a Python keyword)
        in_list = getattr(string_rules, 'in', [])
        if len(in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'in', list(in_list))
            )
        
        not_in_list = getattr(string_rules, 'not_in', [])
        if len(not_in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'string', 'not_in', list(not_in_list))
            )
    
    def _parse_bytes_rules(self, rules):
        """Parse bytes validation rules."""
        if not rules.HasField('bytes'):
            return
        bytes_rules = rules.bytes
        
        if bytes_rules.HasField('min_len'):
            self.constraints.append(
                ValidationConstraint(self.name, 'bytes', 'min_len', bytes_rules.min_len)
            )
        
        if bytes_rules.HasField('max_len'):
            self.constraints.append(
                ValidationConstraint(self.name, 'bytes', 'max_len', bytes_rules.max_len)
            )
        
        if bytes_rules.HasField('const_value'):
            self.constraints.append(
                ValidationConstraint(self.name, 'bytes', 'const', bytes_rules.const_value)
            )
    
    def _parse_bool_rules(self, rules):
        """Parse bool validation rules."""
        if not rules.HasField('bool'):
            return
        bool_rules = rules.bool
        
        if bool_rules.HasField('const_value'):
            self.constraints.append(
                ValidationConstraint(self.name, 'bool', 'const', bool_rules.const_value)
            )
    
    def _parse_enum_rules(self, rules):
        """Parse enum validation rules."""
        if not rules.HasField('enum'):
            return
        enum_rules = rules.enum
        
        if enum_rules.HasField('const_value'):
            self.constraints.append(
                ValidationConstraint(self.name, 'enum', 'const', enum_rules.const_value)
            )
        
        if enum_rules.HasField('defined_only') and enum_rules.defined_only:
            self.constraints.append(
                ValidationConstraint(self.name, 'enum', 'defined_only', True)
            )
        
        in_list = getattr(enum_rules, 'in', [])
        if len(in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'enum', 'in', list(in_list))
            )
        
        not_in_list = getattr(enum_rules, 'not_in', [])
        if len(not_in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'enum', 'not_in', list(not_in_list))
            )
    
    def _parse_any_rules(self, rules):
        """Parse google.protobuf.Any validation rules."""
        if not rules.HasField('any'):
            return
        any_rules = rules.any
        
        in_list = getattr(any_rules, 'in', [])
        if len(in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'any', 'in', list(in_list))
            )
        
        not_in_list = getattr(any_rules, 'not_in', [])
        if len(not_in_list) > 0:
            self.constraints.append(
                ValidationConstraint(self.name, 'any', 'not_in', list(not_in_list))
            )
    
    def _parse_repeated_rules(self, repeated_rules):
        """Parse repeated field validation rules."""
        if repeated_rules.HasField('min_items'):
            self.constraints.append(
                ValidationConstraint(self.name, 'repeated', 'min_items', repeated_rules.min_items)
            )
        
        if repeated_rules.HasField('max_items'):
            self.constraints.append(
                ValidationConstraint(self.name, 'repeated', 'max_items', repeated_rules.max_items)
            )
        
        if repeated_rules.HasField('unique') and repeated_rules.unique:
            self.constraints.append(
                ValidationConstraint(self.name, 'repeated', 'unique', True)
            )
    
    def get_type_name(self) -> str:
        """Get human-readable type name."""
        type_map = {
            1: 'double', 2: 'float', 3: 'int64', 4: 'uint64',
            5: 'int32', 6: 'fixed64', 7: 'fixed32', 8: 'bool',
            9: 'string', 10: 'group', 11: 'message', 12: 'bytes',
            13: 'uint32', 14: 'enum', 15: 'sfixed32', 16: 'sfixed64',
            17: 'sint32', 18: 'sint64'
        }
        return type_map.get(self.type, 'unknown')
    
    def is_repeated(self) -> bool:
        """Check if field is repeated."""
        return self.label == 3  # LABEL_REPEATED


class DataGenerator:
    """Generates test data for protobuf messages with validation rules."""
    
    def __init__(self, proto_file: str, include_paths: Optional[List[str]] = None,
                 any_type_registry: Optional[Dict[str, str]] = None):
        """
        Initialize data generator.
        
        Args:
            proto_file: Path to .proto file
            include_paths: Additional include paths for protoc
            any_type_registry: Dict mapping type_url to message name for Any fields.
                              Example: {'type.googleapis.com/MyMessage': 'MyMessage'}
        """
        self.proto_file = proto_file
        self.include_paths = include_paths or []
        self.file_descriptor = None
        self.message_descriptors = {}
        self.proto_module = None
        self.any_type_registry = any_type_registry or {}
        
        self._load_proto()
    
    def _load_proto(self):
        """Load and compile the proto file."""
        # Ensure validate.proto is compiled for Python so we can parse rules
        self._ensure_validate_pb2()

        # Get absolute path of proto file
        proto_abs_path = os.path.abspath(self.proto_file)
        proto_dir = os.path.dirname(proto_abs_path)
        proto_basename = os.path.basename(proto_abs_path)
        
        # Add common paths
        search_paths = [
            proto_dir,
            os.path.join(os.path.dirname(__file__), 'proto'),
        ] + [os.path.abspath(p) for p in self.include_paths]
        
        # Create FileDescriptorSet
        with TemporaryDirectory() as tmpdir:
            desc_file = os.path.join(tmpdir, 'descriptor.pb')
            
            # Build protoc command
            protoc_args = [
                'protoc',
                '--descriptor_set_out=' + desc_file,
                '--include_imports',
            ]
            
            for path in search_paths:
                if os.path.exists(path):
                    protoc_args.append('-I' + path)
            
            # Use relative path from proto_dir
            protoc_args.append(proto_basename)
            
            # Invoke protoc from the proto directory
            old_cwd = os.getcwd()
            try:
                os.chdir(proto_dir)
                status = invoke_protoc(protoc_args)
            finally:
                os.chdir(old_cwd)
            
            if status != 0:
                raise RuntimeError(f"protoc failed with status {status}")
            
            # Load descriptor
            with open(desc_file, 'rb') as f:
                descriptor_data = f.read()
        
        # Parse FileDescriptorSet
        file_set = descriptor_pb2.FileDescriptorSet()
        file_set.ParseFromString(descriptor_data)
        
        # Find our file descriptor
        proto_name = os.path.basename(self.proto_file)
        for fdesc in file_set.file:
            if fdesc.name == proto_name or fdesc.name.endswith('/' + proto_name):
                self.file_descriptor = fdesc
                break
        
        if not self.file_descriptor:
            raise ValueError(f"Could not find descriptor for {proto_name}")
        
        # Parse message descriptors
        self._parse_messages()

    def _ensure_validate_pb2(self) -> None:
        """Ensure generator/proto/validate_pb2.py exists by building it if missing.

        This allows nanopb_validator to import validate_pb2 and parse
        (validate.rules) options from FieldOptions.
        """
        try:
            # Resolve generator/proto directory and target file
            gen_dir = os.path.dirname(os.path.abspath(__file__))
            proto_dir = os.path.join(gen_dir, 'proto')
            target_py = os.path.join(proto_dir, 'validate_pb2.py')

            # Fast path: already built
            if os.path.isfile(target_py):
                return

            # Build validate.proto into Python module in-place
            validate_proto = os.path.join(proto_dir, 'validate.proto')
            if not os.path.isfile(validate_proto):
                return  # Nothing we can do; parsing will gracefully degrade

            protoc_args = [
                'protoc',
                f'--python_out={proto_dir}',
                f'-I{proto_dir}',
                os.path.basename(validate_proto),
            ]

            old_cwd = os.getcwd()
            try:
                os.chdir(proto_dir)
                # Use the same invoke_protoc helper used elsewhere to get builtin includes
                invoke_protoc(protoc_args)
            finally:
                os.chdir(old_cwd)
        except Exception:
            # Soft-fail: if building fails, rule parsing will be skipped
            pass
    
    def _parse_messages(self):
        """Parse message descriptors from file descriptor."""
        for msg_desc in self.file_descriptor.message_type:
            msg_name = msg_desc.name
            fields = {}
            oneofs = {}
            
            # Parse oneof groups
            for oneof_idx, oneof_desc in enumerate(msg_desc.oneof_decl):
                oneofs[oneof_idx] = {
                    'name': oneof_desc.name,
                    'fields': []
                }
            
            # Parse fields
            for field_desc in msg_desc.field:
                field_info = ProtoFieldInfo(field_desc, self.file_descriptor)
                fields[field_desc.name] = field_info
                
                # Track which oneof this field belongs to (if any)
                if field_desc.HasField('oneof_index'):
                    oneof_idx = field_desc.oneof_index
                    if oneof_idx in oneofs:
                        oneofs[oneof_idx]['fields'].append(field_desc.name)
            
            self.message_descriptors[msg_name] = {
                'descriptor': msg_desc,
                'fields': fields,
                'oneofs': oneofs
            }
    
    def get_messages(self) -> List[str]:
        """Get list of message names in the proto file."""
        return list(self.message_descriptors.keys())
    
    def get_field_info(self, message_name: str, field_name: str) -> Optional[ProtoFieldInfo]:
        """Get field information for a specific message field."""
        if message_name not in self.message_descriptors:
            return None
        return self.message_descriptors[message_name]['fields'].get(field_name)
    
    def get_all_fields(self, message_name: str) -> Dict[str, ProtoFieldInfo]:
        """Get all fields for a message."""
        if message_name not in self.message_descriptors:
            return {}
        return self.message_descriptors[message_name]['fields']
    
    def generate_valid(self, message_name: str, seed: Optional[int] = None,
                       oneof_choice: Optional[Dict[str, str]] = None) -> Dict[str, Any]:
        """
        Generate valid test data for a message.
        
        Args:
            message_name: Name of the message type
            seed: Random seed for reproducibility
            oneof_choice: Dict mapping oneof name to chosen field name.
                         If None, a random field is selected from each oneof.
        
        Returns:
            Dictionary of field values
        """
        if seed is not None:
            random.seed(seed)
        
        if message_name not in self.message_descriptors:
            raise ValueError(f"Message {message_name} not found")
        
        msg_desc = self.message_descriptors[message_name]
        fields = msg_desc['fields']
        oneofs = msg_desc.get('oneofs', {})
        data = {}
        
        # Determine which fields to skip (for oneofs)
        fields_to_skip = set()
        oneof_selections = {}
        
        for oneof_idx, oneof_info in oneofs.items():
            oneof_name = oneof_info['name']
            oneof_fields = oneof_info['fields']
            
            if not oneof_fields:
                continue
            
            # Determine which field to select
            if oneof_choice and oneof_name in oneof_choice:
                selected_field = oneof_choice[oneof_name]
                if selected_field not in oneof_fields:
                    raise ValueError(
                        f"Invalid oneof choice: {selected_field} not in {oneof_name} "
                        f"(valid choices: {oneof_fields})"
                    )
            else:
                # Random selection
                selected_field = random.choice(oneof_fields)
            
            oneof_selections[oneof_name] = selected_field
            
            # Skip all other fields in this oneof
            for field in oneof_fields:
                if field != selected_field:
                    fields_to_skip.add(field)
        
        # Generate data for all fields except skipped ones
        for field_name, field_info in fields.items():
            if field_name in fields_to_skip:
                continue
                
            value = self._generate_valid_value(field_info)
            if value is not None:
                data[field_name] = value
        
        return data
    
    def generate_invalid(
        self,
        message_name: str,
        violate_field: Optional[Union[str, List[str]]] = None,
        violate_rule: Optional[Union[str, List[str]]] = None,
        seed: Optional[int] = None
    ) -> Dict[str, Any]:
        """
        Generate invalid test data for a message.
        
        Args:
            message_name: Name of the message type
            violate_field: Specific field(s) to make invalid. Can be a string,
                a comma-separated string, or a list of field names. If None,
                a random constrained field will be selected.
            violate_rule: Specific rule(s) to violate. Can be a string, a
                comma-separated string, or a list of rule names. If multiple
                are provided, one matching rule will be selected. If None,
                a random rule for the field will be selected.
            seed: Random seed for reproducibility
        
        Returns:
            Dictionary of field values with one or more invalid values
        """
        if seed is not None:
            random.seed(seed)
        
        # Start with valid data
        data = self.generate_valid(message_name, seed=None)  # Don't reuse seed
        
        fields = self.message_descriptors[message_name]['fields']

        # Collect all fields with constraints
        constrained_fields = {
            name: info for name, info in fields.items() if info.constraints
        }
        if not constrained_fields:
            raise ValueError(f"No validation constraints found for {message_name}")

        # Normalize violate_field into a list of field names
        if violate_field is None:
            # Default behavior: pick one random constrained field
            selected_fields = [random.choice(list(constrained_fields.keys()))]
        elif isinstance(violate_field, str):
            selected_fields = [f.strip() for f in violate_field.split(',') if f.strip()]
        else:
            selected_fields = []
            for f in violate_field:
                if isinstance(f, str):
                    selected_fields.extend([p.strip() for p in f.split(',') if p.strip()])

        # Validate fields exist and have constraints
        for fname in selected_fields:
            if fname not in constrained_fields:
                if fname not in fields:
                    raise ValueError(f"Field {fname} does not exist in message {message_name}")
                else:
                    raise ValueError(f"Field {fname} has no constraints to violate")

        # Normalize violate_rule into a list of rule names (may be empty)
        if violate_rule:
            if isinstance(violate_rule, str):
                candidate_rules = [r.strip() for r in violate_rule.split(',') if r.strip()]
            else:
                candidate_rules = []
                for r in violate_rule:
                    if isinstance(r, str):
                        candidate_rules.extend([p.strip() for p in r.split(',') if p.strip()])
        else:
            candidate_rules = []

        # For each selected field, choose a constraint and apply invalid value
        for fname in selected_fields:
            finfo = constrained_fields[fname]

            if candidate_rules:
                matching_constraints = [c for c in finfo.constraints if c.rule_type in set(candidate_rules)]
                if matching_constraints:
                    chosen = random.choice(matching_constraints)
                else:
                    # Fallback: choose any available constraint for this field
                    chosen = random.choice(finfo.constraints)
            else:
                chosen = random.choice(finfo.constraints)

            invalid_value = self._generate_invalid_value(finfo, chosen)
            data[fname] = invalid_value

        return data
    
    def _generate_valid_value(self, field_info: ProtoFieldInfo) -> Any:
        """Generate a valid value for a field."""
        type_name = field_info.get_type_name()
        
        if field_info.is_repeated():
            return self._generate_valid_repeated(field_info)
        
        # Find constraints
        constraints = {c.rule_type: c.value for c in field_info.constraints}
        
        # Generate based on type
        if type_name == 'int32':
            return self._generate_valid_int32(constraints)
        elif type_name == 'int64':
            return self._generate_valid_int64(constraints)
        elif type_name == 'uint32':
            return self._generate_valid_uint32(constraints)
        elif type_name == 'uint64':
            return self._generate_valid_uint64(constraints)
        elif type_name == 'sint32':
            return self._generate_valid_int32(constraints)
        elif type_name == 'sint64':
            return self._generate_valid_int64(constraints)
        elif type_name == 'float':
            return self._generate_valid_float(constraints)
        elif type_name == 'double':
            return self._generate_valid_double(constraints)
        elif type_name == 'bool':
            return self._generate_valid_bool(constraints)
        elif type_name == 'string':
            return self._generate_valid_string(constraints)
        elif type_name == 'bytes':
            return self._generate_valid_bytes(constraints)
        elif type_name == 'enum':
            return self._generate_valid_enum(field_info, constraints)
        elif type_name == 'message':
            return self._generate_valid_message(field_info, constraints)
        else:
            # Default values for unsupported types
            return None
    
    def _generate_valid_int32(self, constraints: Dict[str, Any]) -> int:
        """Generate valid int32 value."""
        min_val = -(2**31)
        max_val = 2**31 - 1
        
        if 'gte' in constraints:
            min_val = max(min_val, int(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, int(constraints['gt']) + 1)
        
        if 'lte' in constraints:
            max_val = min(max_val, int(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, int(constraints['lt']) - 1)
        
        if 'const' in constraints:
            return int(constraints['const'])
        
        if 'in' in constraints:
            return random.choice(constraints['in'])
        
        return random.randint(min_val, max_val)
    
    def _generate_valid_int64(self, constraints: Dict[str, Any]) -> int:
        """Generate valid int64 value."""
        min_val = -(2**63)
        max_val = 2**63 - 1
        
        if 'gte' in constraints:
            min_val = max(min_val, int(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, int(constraints['gt']) + 1)
        
        if 'lte' in constraints:
            max_val = min(max_val, int(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, int(constraints['lt']) - 1)
        
        if 'const' in constraints:
            return int(constraints['const'])
        
        # For large ranges, generate reasonable values
        if max_val - min_val > 10**9:
            return random.randint(min_val, min(min_val + 10**9, max_val))
        
        return random.randint(min_val, max_val)
    
    def _generate_valid_uint32(self, constraints: Dict[str, Any]) -> int:
        """Generate valid uint32 value."""
        min_val = 0
        max_val = 2**32 - 1
        
        if 'gte' in constraints:
            min_val = max(min_val, int(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, int(constraints['gt']) + 1)
        
        if 'lte' in constraints:
            max_val = min(max_val, int(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, int(constraints['lt']) - 1)
        
        if 'const' in constraints:
            return int(constraints['const'])
        
        return random.randint(min_val, max_val)
    
    def _generate_valid_uint64(self, constraints: Dict[str, Any]) -> int:
        """Generate valid uint64 value."""
        min_val = 0
        max_val = 2**64 - 1
        
        if 'gte' in constraints:
            min_val = max(min_val, int(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, int(constraints['gt']) + 1)
        
        if 'lte' in constraints:
            max_val = min(max_val, int(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, int(constraints['lt']) - 1)
        
        if 'const' in constraints:
            return int(constraints['const'])
        
        # For large ranges, generate reasonable values
        if max_val - min_val > 10**9:
            return random.randint(min_val, min(min_val + 10**9, max_val))
        
        return random.randint(min_val, max_val)
    
    def _generate_valid_float(self, constraints: Dict[str, Any]) -> float:
        """Generate valid float value."""
        min_val = -3.4e38
        max_val = 3.4e38
        
        if 'gte' in constraints:
            min_val = max(min_val, float(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, float(constraints['gt']) + 0.01)
        
        if 'lte' in constraints:
            max_val = min(max_val, float(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, float(constraints['lt']) - 0.01)
        
        if 'const' in constraints:
            return float(constraints['const'])
        
        return random.uniform(min_val, max_val)
    
    def _generate_valid_double(self, constraints: Dict[str, Any]) -> float:
        """Generate valid double value."""
        min_val = -1.7e308
        max_val = 1.7e308
        
        if 'gte' in constraints:
            min_val = max(min_val, float(constraints['gte']))
        elif 'gt' in constraints:
            min_val = max(min_val, float(constraints['gt']) + 0.01)
        
        if 'lte' in constraints:
            max_val = min(max_val, float(constraints['lte']))
        elif 'lt' in constraints:
            max_val = min(max_val, float(constraints['lt']) - 0.01)
        
        if 'const' in constraints:
            return float(constraints['const'])
        
        return random.uniform(min_val, max_val)
    
    def _generate_valid_bool(self, constraints: Dict[str, Any]) -> bool:
        """Generate valid bool value."""
        if 'const' in constraints:
            return bool(constraints['const'])
        return random.choice([True, False])
    
    def _generate_valid_string(self, constraints: Dict[str, Any]) -> str:
        """Generate valid string value."""
        min_len = constraints.get('min_len', 1)
        max_len = constraints.get('max_len', 20)

        # Constants and enumerations take precedence
        if 'const' in constraints:
            return constraints['const']

        if 'in' in constraints:
            return random.choice(constraints['in'])

        # Specialized string constraints (PGV-style)
        # When these are present, prefer generating a compliant value directly
        try:
            if constraints.get('email'):
                return self._generate_valid_email()
            if constraints.get('hostname'):
                return self._generate_valid_hostname()
            if constraints.get('ipv4'):
                return self._generate_valid_ipv4()
            if constraints.get('ipv6'):
                return self._generate_valid_ipv6()
            if constraints.get('ip'):
                # Randomly choose either IPv4 or IPv6
                return self._generate_valid_ipv4() if random.choice([True, False]) else self._generate_valid_ipv6()
        except Exception:
            # Fallback to generic generation if specialized fails for any reason
            pass

        # Generic string generation path
        length = max(min_len, min(max_len, random.randint(min_len, max_len)))

        # Check for ASCII constraint
        if constraints.get('ascii', False):
            chars = string.ascii_letters + string.digits
        else:
            chars = string.ascii_letters + string.digits + string.punctuation

        base_str = ''.join(random.choice(chars) for _ in range(max(1, length)))

        # Apply prefix
        if 'prefix' in constraints:
            prefix = constraints['prefix']
            remaining = max(0, max_len - len(prefix))
            base_str = (prefix + base_str[:remaining]) if remaining > 0 else prefix

        # Apply suffix
        if 'suffix' in constraints:
            suffix = constraints['suffix']
            available = max(0, max_len - len(suffix))
            base_str = (base_str[:available] + suffix) if available > 0 else suffix

        # Apply contains
        if 'contains' in constraints:
            contains = constraints['contains']
            if contains not in base_str:
                if len(base_str) + len(contains) <= max_len:
                    pos = random.randint(0, len(base_str))
                    base_str = base_str[:pos] + contains + base_str[pos:]
                else:
                    pos = random.randint(0, max(0, len(base_str) - len(contains)))
                    base_str = base_str[:pos] + contains + base_str[pos + len(contains):]

        # Check not_in constraint
        if 'not_in' in constraints:
            forbidden = set(constraints['not_in']) if isinstance(constraints['not_in'], list) else {constraints['not_in']}
            attempts = 0
            while base_str in forbidden and attempts < 10:
                base_str = ''.join(random.choice(chars) for _ in range(max(1, length)))
                attempts += 1

        # Ensure length constraints
        if len(base_str) < min_len:
            base_str += ''.join(random.choice(chars) for _ in range(min_len - len(base_str)))
        if len(base_str) > max_len:
            base_str = base_str[:max_len]

        return base_str

    # ----- Specialized string generators -----
    def _generate_valid_email(self) -> str:
        """Generate a simple valid email address."""
        # Local part: letters/digits/dot/underscore/hyphen, not starting/ending with dot
        lp_len = random.randint(1, 16)
        lp_chars = string.ascii_letters + string.digits + '._-'
        local = ''.join(random.choice(lp_chars) for _ in range(lp_len))
        local = local.strip('.')
        if not local:
            local = 'u'
        domain = self._generate_valid_hostname()
        return f"{local}@{domain}"

    def _generate_valid_hostname(self) -> str:
        """Generate a valid hostname (RFC 1123-ish)."""
        # 1-3 labels, each 1-63 chars, a-z0-9-, no leading/trailing hyphen
        labels = []
        num_labels = random.randint(2, 4)
        for _ in range(num_labels):
            length = random.randint(1, min(12, 63))
            chars = string.ascii_lowercase + string.digits + '-'
            label = ''.join(random.choice(chars) for _ in range(length))
            # Fix leading/trailing hyphen
            if label[0] == '-':
                label = 'a' + label[1:]
            if label[-1] == '-':
                label = label[:-1] + 'z'
            labels.append(label)
        hostname = '.'.join(labels)
        # Ensure total length <= 253
        return hostname[:253]

    def _generate_valid_ipv4(self) -> str:
        """Generate a valid IPv4 address in dotted-decimal form."""
        return '.'.join(str(random.randint(0, 255)) for _ in range(4))

    def _generate_valid_ipv6(self) -> str:
        """Generate a simple valid IPv6 address (no compression)."""
        hextet = lambda: ''.join(random.choice('0123456789abcdef') for _ in range(random.randint(1, 4)))
        return ':'.join(hextet() for _ in range(8))
    
    def _generate_valid_bytes(self, constraints: Dict[str, Any]) -> bytes:
        """Generate valid bytes value."""
        min_len = constraints.get('min_len', 1)
        max_len = constraints.get('max_len', 20)
        
        if 'const' in constraints:
            return constraints['const']
        
        length = random.randint(min_len, max_len)
        return bytes(random.randint(0, 255) for _ in range(length))
    
    def _generate_valid_enum(self, field_info: ProtoFieldInfo, constraints: Dict[str, Any]) -> int:
        """Generate valid enum value."""
        # Get enum values from the enum type
        enum_values = self._get_enum_values(field_info.type_name)
        
        if not enum_values:
            # No enum definition found, return 0 (default)
            return 0
        
        # Check for const constraint
        if 'const' in constraints:
            return constraints['const']
        
        # Check for in constraint
        if 'in' in constraints:
            valid_values = [v for v in constraints['in'] if v in enum_values]
            if valid_values:
                return random.choice(valid_values)
        
        # Check for not_in constraint
        if 'not_in' in constraints:
            valid_values = [v for v in enum_values if v not in constraints['not_in']]
            if valid_values:
                return random.choice(valid_values)
        
        # Default: pick random enum value
        return random.choice(enum_values)
    
    def _get_enum_values(self, type_name: str) -> List[int]:
        """Get list of valid enum values for an enum type."""
        if not type_name or not self.file_descriptor:
            return []
        
        # Extract enum name from type_name (format: .package.EnumName)
        enum_name = type_name.split('.')[-1]
        
        # Search for enum in file descriptor
        for enum_desc in self.file_descriptor.enum_type:
            if enum_desc.name == enum_name:
                return [v.number for v in enum_desc.value]
        
        # Search in nested enums within messages
        for msg_desc in self.file_descriptor.message_type:
            for enum_desc in msg_desc.enum_type:
                if enum_desc.name == enum_name:
                    return [v.number for v in enum_desc.value]
        
        return []
    
    def _generate_valid_message(self, field_info: ProtoFieldInfo, constraints: Dict[str, Any]) -> Dict[str, Any]:
        """Generate valid nested message value."""
        # Check if this is a google.protobuf.Any type
        if '.google.protobuf.Any' in field_info.type_name:
            return self._generate_valid_any(field_info, constraints)
        
        # Extract message name from type_name (format: .package.MessageName)
        message_name = field_info.type_name.split('.')[-1]
        
        # Check if we have this message in our descriptors
        if message_name in self.message_descriptors:
            # Recursively generate data for nested message
            return self.generate_valid(message_name, seed=None)
        
        # If message not found, return empty dict
        return {}
    
    def _generate_valid_any(self, field_info: ProtoFieldInfo, constraints: Dict[str, Any]) -> Dict[str, Any]:
        """Generate valid google.protobuf.Any value."""
        # Determine which type_url to use
        available_types = list(self.any_type_registry.keys())
        
        # Apply in constraint
        if 'in' in constraints:
            available_types = [t for t in available_types if t in constraints['in']]
        
        # Apply not_in constraint
        if 'not_in' in constraints:
            available_types = [t for t in available_types if t not in constraints['not_in']]
        
        if not available_types:
            # No valid types available, generate empty Any
            return {
                'type_url': '',
                'value': b''
            }
        
        # Select a random type
        type_url = random.choice(available_types)
        message_name = self.any_type_registry[type_url]
        
        # Generate data for the nested message
        if message_name in self.message_descriptors:
            nested_data = self.generate_valid(message_name, seed=None)
            # Encode the nested message to bytes
            value_bytes = self.encode_to_binary(message_name, nested_data)
        else:
            value_bytes = b''
        
        return {
            'type_url': type_url,
            'value': value_bytes
        }
    
    def _generate_valid_repeated(self, field_info: ProtoFieldInfo) -> List[Any]:
        """Generate valid repeated field value."""
        constraints = {c.rule_type: c.value for c in field_info.constraints}
        
        min_items = constraints.get('min_items', 1)
        max_items = constraints.get('max_items', 5)
        
        count = random.randint(min_items, max_items)
        
        # Generate items based on the field's scalar type
        # We need to treat this as a non-repeated field for generation
        type_name = field_info.get_type_name()
        item_constraints = {
            c.rule_type: c.value for c in field_info.constraints
            if c.rule_type not in ('min_items', 'max_items', 'unique')
        }
        
        items = []
        for _ in range(count):
            # Generate single item based on type
            if type_name == 'int32':
                items.append(self._generate_valid_int32(item_constraints))
            elif type_name == 'int64':
                items.append(self._generate_valid_int64(item_constraints))
            elif type_name == 'uint32':
                items.append(self._generate_valid_uint32(item_constraints))
            elif type_name == 'uint64':
                items.append(self._generate_valid_uint64(item_constraints))
            elif type_name == 'sint32':
                items.append(self._generate_valid_int32(item_constraints))
            elif type_name == 'sint64':
                items.append(self._generate_valid_int64(item_constraints))
            elif type_name == 'float':
                items.append(self._generate_valid_float(item_constraints))
            elif type_name == 'double':
                items.append(self._generate_valid_double(item_constraints))
            elif type_name == 'bool':
                items.append(self._generate_valid_bool(item_constraints))
            elif type_name == 'string':
                items.append(self._generate_valid_string(item_constraints))
            elif type_name == 'bytes':
                items.append(self._generate_valid_bytes(item_constraints))
            elif type_name == 'enum':
                items.append(self._generate_valid_enum(field_info, item_constraints))
            elif type_name == 'message':
                items.append(self._generate_valid_message(field_info, item_constraints))
            else:
                items.append(None)
        
        # Handle unique constraint
        if constraints.get('unique', False):
            items = list(set(items))
            # Generate more items if needed
            while len(items) < min_items:
                if type_name == 'int32':
                    items.append(self._generate_valid_int32(item_constraints))
                elif type_name == 'int64':
                    items.append(self._generate_valid_int64(item_constraints))
                elif type_name == 'uint32':
                    items.append(self._generate_valid_uint32(item_constraints))
                elif type_name == 'uint64':
                    items.append(self._generate_valid_uint64(item_constraints))
                elif type_name == 'string':
                    # For strings, append a unique suffix
                    items.append(self._generate_valid_string(item_constraints) + f"_{len(items)}")
        
        return items
    
    def _generate_invalid_value(
        self,
        field_info: ProtoFieldInfo,
        constraint: ValidationConstraint
    ) -> Any:
        """Generate an invalid value that violates the given constraint."""
        rule_type = constraint.rule_type
        rule_value = constraint.value
        type_name = field_info.get_type_name()
        
        # Handle different constraint types
        if rule_type in ('gt', 'gte'):
            # Violate by going below threshold
            if rule_type == 'gt':
                return rule_value - (random.randint(1, 100) if type_name.startswith('int') else random.uniform(0.1, 10))
            else:  # gte
                return rule_value - (random.randint(1, 100) if type_name.startswith('int') else random.uniform(0.1, 10))
        
        elif rule_type in ('lt', 'lte'):
            # Violate by going above threshold
            if rule_type == 'lt':
                return rule_value + (random.randint(1, 100) if type_name.startswith('int') else random.uniform(0.1, 10))
            else:  # lte
                return rule_value + (random.randint(1, 100) if type_name.startswith('int') else random.uniform(0.1, 10))
        
        elif rule_type == 'const':
            # Violate by using different value
            if type_name in ('int32', 'int64', 'uint32', 'uint64', 'sint32', 'sint64'):
                return rule_value + random.randint(1, 100)
            elif type_name in ('float', 'double'):
                return rule_value + random.uniform(1.0, 10.0)
            elif type_name == 'bool':
                return not rule_value
            elif type_name == 'string':
                return rule_value + "_invalid"
            else:
                return None
        
        elif rule_type == 'min_len':
            # String/bytes too short
            return 'x' * (rule_value - 1) if rule_value > 0 else ''
        
        elif rule_type == 'max_len':
            # String/bytes too long
            return 'x' * (rule_value + 10)
        
        elif rule_type == 'prefix':
            # String without required prefix
            return 'WRONG_' + rule_value
        
        elif rule_type == 'suffix':
            # String without required suffix
            return rule_value + '_WRONG'
        
        elif rule_type == 'contains':
            # String without required substring
            return 'DOES_NOT_CONTAIN_REQUIRED'
        
        elif rule_type == 'ascii':
            # Non-ASCII string
            return 'test_中文_invalid'
        
        elif rule_type == 'email':
            # Not a valid email
            return 'invalid-email-without-at'
        
        elif rule_type == 'hostname':
            # Invalid hostname (illegal chars / formatting)
            return '-bad_host_name-'
        
        elif rule_type == 'ip':
            # Not an IP format
            return '999.999.999.999'
        
        elif rule_type == 'ipv4':
            # Invalid IPv4
            return '256.300.1.2'
        
        elif rule_type == 'ipv6':
            # Invalid IPv6 (non-hex)
            return 'gggg:gggg:gggg:gggg:gggg:gggg:gggg:gggg'
        
        elif rule_type == 'in':
            # Value not in allowed list
            if type_name == 'string':
                return 'not_in_list_' + str(random.randint(1, 1000))
            else:
                return 999999
        
        elif rule_type == 'not_in':
            # Value in forbidden list
            if isinstance(rule_value, list) and rule_value:
                return rule_value[0]
            return rule_value
        
        elif rule_type == 'min_items':
            # Too few items
            return [] if rule_value > 0 else []
        
        elif rule_type == 'max_items':
            # Too many items - generate items based on type
            items = []
            type_name = field_info.get_type_name()
            item_constraints = {
                c.rule_type: c.value for c in field_info.constraints
                if c.rule_type not in ('min_items', 'max_items', 'unique')
            }
            
            for _ in range(rule_value + 5):
                if type_name == 'int32':
                    items.append(self._generate_valid_int32(item_constraints))
                elif type_name == 'int64':
                    items.append(self._generate_valid_int64(item_constraints))
                elif type_name == 'uint32':
                    items.append(self._generate_valid_uint32(item_constraints))
                elif type_name == 'string':
                    items.append(self._generate_valid_string(item_constraints))
                else:
                    items.append(0)
            return items
        
        elif rule_type == 'unique':
            # Duplicate items
            type_name = field_info.get_type_name()
            item_constraints = {
                c.rule_type: c.value for c in field_info.constraints
                if c.rule_type not in ('min_items', 'max_items', 'unique')
            }
            
            if type_name == 'int32':
                item = self._generate_valid_int32(item_constraints)
            elif type_name == 'string':
                item = self._generate_valid_string(item_constraints)
            else:
                item = 42
            
            return [item, item, item]
        
        # Default: return a clearly invalid value
        return None
    
    def encode_to_binary(self, message_name: str, data: Dict[str, Any]) -> bytes:
        """
        Encode data dictionary to protobuf binary format.
        
        Args:
            message_name: Name of the message type
            data: Dictionary of field values
        
        Returns:
            Binary protobuf data
        """
        output = bytearray()
        
        if message_name not in self.message_descriptors:
            raise ValueError(f"Message {message_name} not found")
        
        fields = self.message_descriptors[message_name]['fields']
        
        # Encode each field
        for field_name, value in data.items():
            if field_name not in fields:
                continue
            
            field_info = fields[field_name]
            field_bytes = self._encode_field(field_info, value)
            output.extend(field_bytes)
        
        return bytes(output)
    
    def _encode_field(self, field_info: ProtoFieldInfo, value: Any) -> bytes:
        """Encode a single field to protobuf wire format."""
        field_number = field_info.number
        field_type = field_info.type
        
        if field_info.is_repeated():
            result = bytearray()
            for item in value:
                result.extend(self._encode_single_field(field_number, field_type, item, field_info))
            return bytes(result)
        else:
            return self._encode_single_field(field_number, field_type, value, field_info)
    
    def _encode_single_field(self, field_number: int, field_type: int, value: Any, 
                            field_info: Optional[ProtoFieldInfo] = None) -> bytes:
        """Encode a single field value to protobuf wire format."""
        # Wire types: 0=varint, 1=64bit, 2=length-delimited, 5=32bit
        
        if field_type in (1, 2):  # double, float
            wire_type = 1 if field_type == 1 else 5
            key = (field_number << 3) | wire_type
            key_bytes = self._encode_varint(key)
            
            if field_type == 1:  # double
                value_bytes = struct.pack('<d', value)
            else:  # float
                value_bytes = struct.pack('<f', value)
            
            return key_bytes + value_bytes
        
        elif field_type in (3, 4, 5, 13):  # int64, uint64, int32, uint32
            wire_type = 0
            key = (field_number << 3) | wire_type
            return self._encode_varint(key) + self._encode_varint(value)
        
        elif field_type in (17, 18):  # sint32, sint64
            wire_type = 0
            key = (field_number << 3) | wire_type
            # ZigZag encoding
            if field_type == 17:
                encoded = (value << 1) ^ (value >> 31)
            else:
                encoded = (value << 1) ^ (value >> 63)
            return self._encode_varint(key) + self._encode_varint(encoded)
        
        elif field_type == 8:  # bool
            wire_type = 0
            key = (field_number << 3) | wire_type
            return self._encode_varint(key) + self._encode_varint(1 if value else 0)
        
        elif field_type in (9, 12):  # string, bytes
            wire_type = 2
            key = (field_number << 3) | wire_type
            
            if field_type == 9:  # string
                value_bytes = value.encode('utf-8')
            else:  # bytes
                value_bytes = value
            
            return (self._encode_varint(key) +
                    self._encode_varint(len(value_bytes)) +
                    value_bytes)
        
        elif field_type in (7, 15):  # fixed32, sfixed32
            wire_type = 5
            key = (field_number << 3) | wire_type
            
            if field_type == 7:  # fixed32
                value_bytes = struct.pack('<I', value)
            else:  # sfixed32
                value_bytes = struct.pack('<i', value)
            
            return self._encode_varint(key) + value_bytes
        
        elif field_type in (6, 16):  # fixed64, sfixed64
            wire_type = 1
            key = (field_number << 3) | wire_type
            
            if field_type == 6:  # fixed64
                value_bytes = struct.pack('<Q', value)
            else:  # sfixed64
                value_bytes = struct.pack('<q', value)
            
            return self._encode_varint(key) + value_bytes
        
        elif field_type == 14:  # enum
            # Enums are encoded as varints
            wire_type = 0
            key = (field_number << 3) | wire_type
            return self._encode_varint(key) + self._encode_varint(value)
        
        elif field_type == 11:  # message
            # Messages are length-delimited
            wire_type = 2
            key = (field_number << 3) | wire_type
            
            # value should be a dict representing the nested message
            if isinstance(value, dict) and field_info:
                # Extract message name from type_name (format: .package.MessageName)
                message_name = field_info.type_name.split('.')[-1]
                
                # Recursively encode the nested message
                if message_name in self.message_descriptors:
                    value_bytes = self.encode_to_binary(message_name, value)
                else:
                    value_bytes = b''
            else:
                value_bytes = b''
            
            return (self._encode_varint(key) +
                    self._encode_varint(len(value_bytes)) +
                    value_bytes)
        
        return b''
    
    def _encode_varint(self, value: int) -> bytes:
        """Encode an integer as a varint."""
        if value < 0:
            value += (1 << 64)
        
        result = bytearray()
        while value > 0x7f:
            result.append((value & 0x7f) | 0x80)
            value >>= 7
        result.append(value & 0x7f)
        return bytes(result)
    
    def format_output(
        self,
        data: bytes,
        format_type: OutputFormat = OutputFormat.C_ARRAY,
        name: str = "test_data"
    ) -> str:
        """
        Format binary data for output.
        
        Args:
            data: Binary protobuf data
            format_type: Desired output format
            name: Variable name for C arrays
        
        Returns:
            Formatted string
        """
        if format_type == OutputFormat.BINARY:
            return data
        
        elif format_type == OutputFormat.HEX_STRING:
            return data.hex()
        
        elif format_type == OutputFormat.C_ARRAY:
            hex_values = ', '.join(f'0x{b:02x}' for b in data)
            return f'const uint8_t {name}[] = {{{hex_values}}};\nconst size_t {name}_size = {len(data)};'
        
        elif format_type == OutputFormat.PYTHON_DICT:
            return str(data)
        
        return str(data)


def main():
    """Example usage of the data generator."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Generate test data for protobuf messages with validation'
    )
    parser.add_argument('proto_file', help='Path to .proto file')
    parser.add_argument('message', help='Message name to generate data for')
    parser.add_argument('--invalid', action='store_true',
                        help='Generate invalid data instead of valid')
    # Allow multiple --field entries or comma-separated values
    parser.add_argument(
        '--field', dest='fields', action='append', default=None,
        help='Field(s) to violate (can be given multiple times or comma-separated)'
    )
    # Allow multiple --rule entries or comma-separated values
    parser.add_argument(
        '--rule', dest='rules', action='append', default=None,
        help='Rule(s) to violate (can be given multiple times or comma-separated)'
    )
    parser.add_argument('--format', choices=['binary', 'c_array', 'hex', 'dict'],
                        default='c_array', help='Output format')
    parser.add_argument('--output', '-o', help='Output file (default: stdout)')
    parser.add_argument('--seed', type=int, help='Random seed for reproducibility')
    parser.add_argument('-I', '--include', action='append', default=[],
                        help='Include path for proto files')
    
    args = parser.parse_args()
    
    # Create generator
    try:
        generator = DataGenerator(args.proto_file, args.include)
    except Exception as e:
        print(f"Error loading proto file: {e}", file=sys.stderr)
        return 1
    
    # Generate data
    try:
        if args.invalid:
            data_dict = generator.generate_invalid(
                args.message,
                violate_field=args.fields,
                violate_rule=args.rules,
                seed=args.seed
            )
        else:
            data_dict = generator.generate_valid(args.message, seed=args.seed)
        
        # Encode to binary
        binary_data = generator.encode_to_binary(args.message, data_dict)
        
        # Format output
        format_map = {
            'binary': OutputFormat.BINARY,
            'c_array': OutputFormat.C_ARRAY,
            'hex': OutputFormat.HEX_STRING,
            'dict': OutputFormat.PYTHON_DICT,
        }
        
        output = generator.format_output(
            binary_data,
            format_map[args.format],
            f"{args.message.lower()}_data"
        )
        
        # Write output
        if args.output:
            if args.format == 'binary':
                with open(args.output, 'wb') as f:
                    f.write(output)
            else:
                with open(args.output, 'w') as f:
                    f.write(output)
                    f.write('\n')
        else:
            if args.format == 'binary':
                sys.stdout.buffer.write(output)
            else:
                print(output)
        
        # Print data dict to stderr for debugging
        print(f"\n// Generated data: {data_dict}", file=sys.stderr)
        
    except Exception as e:
        print(f"Error generating data: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
