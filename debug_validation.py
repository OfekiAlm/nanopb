#!/usr/bin/env python3

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'generator'))

from generator import nanopb_generator
from generator.proto import validate_pb2

# Test if validation rules are being parsed
def test_validation_parsing():
    print("Testing validation parsing...")
    
    # Create a simple test proto file content
    proto_content = '''
syntax = "proto3";
import "generator/proto/validate.proto";
option (validate.validate) = true;

message TestMessage {
    string username = 1 [
        (validate.rules).string.min_len = 3,
        (validate.rules).string.max_len = 20,
        (validate.rules).required = true
    ];
}
'''
    
    # Write to temporary file
    with open('test_validation.proto', 'w') as f:
        f.write(proto_content)
    
    try:
        # Parse the proto file
        from google.protobuf import descriptor_pb2
        from google.protobuf import message_factory
        from google.protobuf import descriptor
        
        # This is a simplified test - in reality we'd need to use protoc
        print("Proto file created for testing")
        
    finally:
        # Clean up
        if os.path.exists('test_validation.proto'):
            os.remove('test_validation.proto')

if __name__ == '__main__':
    test_validation_parsing()
