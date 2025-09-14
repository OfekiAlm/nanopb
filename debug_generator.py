#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'generator'))

from generator import nanopb_generator
from generator.proto import validate_pb2

def debug_validation():
    print("Debugging validation...")
    
    # Test if validation rules are being parsed
    proto_file = 'simple_validation.proto'
    
    # Parse the proto file
    from google.protobuf import descriptor_pb2
    from google.protobuf import message_factory
    from google.protobuf import descriptor
    
    # This is a simplified test - we need to use protoc to parse the proto file
    print("Proto file:", proto_file)
    print("validate_pb2 loaded:", validate_pb2 is not None)
    
    if validate_pb2:
        print("FieldRules available:", hasattr(validate_pb2, 'FieldRules'))
        print("StringRules available:", hasattr(validate_pb2, 'StringRules'))
        
        # Test creating a FieldRules object
        try:
            field_rules = validate_pb2.FieldRules()
            string_rules = validate_pb2.StringRules()
            string_rules.min_len = 3
            string_rules.max_len = 20
            field_rules.string.CopyFrom(string_rules)
            field_rules.required = True
            
            print("FieldRules created successfully")
            print("String rules min_len:", field_rules.string.min_len)
            print("String rules max_len:", field_rules.string.max_len)
            print("Required:", field_rules.required)
            
        except Exception as e:
            print("Error creating FieldRules:", e)

if __name__ == '__main__':
    debug_validation()
