#!/usr/bin/env python3
"""
Comprehensive test suite for nanopb_data_generator.py

Tests all supported protobuf features and validation constraints.
"""

import os
import sys
import unittest
from pathlib import Path

# Add generator directory to path
sys.path.insert(0, os.path.dirname(__file__))

from nanopb_data_generator import DataGenerator, ValidationConstraint


class TestNumericConstraints(unittest.TestCase):
    """Test numeric validation constraints."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'numeric_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_int32_lt_constraint(self):
        """Test int32 less-than constraint."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('lt_field', data)
        self.assertLess(data['lt_field'], 100, "lt_field should be < 100")
    
    def test_int32_lte_constraint(self):
        """Test int32 less-than-or-equal constraint."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('lte_field', data)
        self.assertLessEqual(data['lte_field'], 100, "lte_field should be <= 100")
    
    def test_int32_gt_constraint(self):
        """Test int32 greater-than constraint."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('gt_field', data)
        self.assertGreater(data['gt_field'], 0, "gt_field should be > 0")
    
    def test_int32_gte_constraint(self):
        """Test int32 greater-than-or-equal constraint."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('gte_field', data)
        self.assertGreaterEqual(data['gte_field'], 0, "gte_field should be >= 0")
    
    def test_int32_const_constraint(self):
        """Test int32 const constraint."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('const_field', data)
        self.assertEqual(data['const_field'], 42, "const_field should be 42")
    
    def test_int32_range_constraint(self):
        """Test int32 range constraint (gte + lte)."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        self.assertIn('range_field', data)
        self.assertGreaterEqual(data['range_field'], 0, "range_field should be >= 0")
        self.assertLessEqual(data['range_field'], 150, "range_field should be <= 150")
    
    def test_int32_invalid_lt(self):
        """Test generating invalid data for lt constraint."""
        data = self.generator.generate_invalid('Int32Rules', violate_field='lt_field', seed=42)
        self.assertIn('lt_field', data)
        self.assertGreaterEqual(data['lt_field'], 100, "lt_field should be >= 100 (invalid)")
    
    def test_int32_invalid_const(self):
        """Test generating invalid data for const constraint."""
        data = self.generator.generate_invalid('Int32Rules', violate_field='const_field', seed=42)
        self.assertIn('const_field', data)
        self.assertNotEqual(data['const_field'], 42, "const_field should not be 42 (invalid)")
    
    def test_deterministic_generation(self):
        """Test that same seed produces same output."""
        data1 = self.generator.generate_valid('Int32Rules', seed=12345)
        data2 = self.generator.generate_valid('Int32Rules', seed=12345)
        self.assertEqual(data1, data2, "Same seed should produce identical output")
    
    def test_different_seeds_different_output(self):
        """Test that different seeds produce different output."""
        data1 = self.generator.generate_valid('Int32Rules', seed=12345)
        data2 = self.generator.generate_valid('Int32Rules', seed=54321)
        # At least one field should be different
        different = any(data1.get(k) != data2.get(k) for k in data1.keys())
        self.assertTrue(different, "Different seeds should produce different output")


class TestStringConstraints(unittest.TestCase):
    """Test string validation constraints."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'string_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_string_min_len(self):
        """Test string min_len constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('min_len_field', data)
        self.assertGreaterEqual(len(data['min_len_field']), 3, "min_len_field should be >= 3 chars")
    
    def test_string_max_len(self):
        """Test string max_len constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('max_len_field', data)
        self.assertLessEqual(len(data['max_len_field']), 20, "max_len_field should be <= 20 chars")
    
    def test_string_range_len(self):
        """Test string length range constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('range_len_field', data)
        length = len(data['range_len_field'])
        self.assertGreaterEqual(length, 3, "range_len_field should be >= 3 chars")
        self.assertLessEqual(length, 20, "range_len_field should be <= 20 chars")
    
    def test_string_prefix(self):
        """Test string prefix constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('prefix_field', data)
        self.assertTrue(data['prefix_field'].startswith('PREFIX_'), 
                       f"prefix_field should start with 'PREFIX_', got: {data['prefix_field']}")
    
    def test_string_suffix(self):
        """Test string suffix constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('suffix_field', data)
        self.assertTrue(data['suffix_field'].endswith('_SUFFIX'),
                       f"suffix_field should end with '_SUFFIX', got: {data['suffix_field']}")
    
    def test_string_contains(self):
        """Test string contains constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('contains_field', data)
        self.assertIn('@', data['contains_field'], "contains_field should contain '@'")
    
    def test_string_ascii(self):
        """Test string ascii constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('ascii_field', data)
        # All characters should be ASCII (ord < 128)
        self.assertTrue(all(ord(c) < 128 for c in data['ascii_field']),
                       f"ascii_field should only contain ASCII characters, got: {data['ascii_field']}")
    
    def test_string_email(self):
        """Test string email constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('email_field', data)
        # Basic email validation
        self.assertIn('@', data['email_field'], "email_field should contain '@'")
        parts = data['email_field'].split('@')
        self.assertEqual(len(parts), 2, f"email_field should have exactly one '@', got: {data['email_field']}")
        self.assertGreater(len(parts[0]), 0, "email local part should not be empty")
        self.assertGreater(len(parts[1]), 0, "email domain should not be empty")
    
    def test_string_hostname(self):
        """Test string hostname constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('hostname_field', data)
        # Basic hostname validation
        self.assertGreater(len(data['hostname_field']), 0, "hostname should not be empty")
        # Should not start or end with hyphen
        self.assertNotEqual(data['hostname_field'][0], '-', "hostname should not start with '-'")
        self.assertNotEqual(data['hostname_field'][-1], '-', "hostname should not end with '-'")
    
    def test_string_ipv4(self):
        """Test string ipv4 constraint."""
        data = self.generator.generate_valid('StringRules', seed=42)
        self.assertIn('ipv4_field', data)
        # Basic IPv4 validation
        parts = data['ipv4_field'].split('.')
        self.assertEqual(len(parts), 4, f"IPv4 should have 4 octets, got: {data['ipv4_field']}")
        for part in parts:
            val = int(part)
            self.assertGreaterEqual(val, 0, "IPv4 octet should be >= 0")
            self.assertLessEqual(val, 255, "IPv4 octet should be <= 255")
    
    def test_string_invalid_min_len(self):
        """Test generating invalid data for min_len constraint."""
        data = self.generator.generate_invalid('StringRules', violate_field='min_len_field', seed=42)
        self.assertIn('min_len_field', data)
        self.assertLess(len(data['min_len_field']), 3, "min_len_field should be < 3 chars (invalid)")


class TestEnumConstraints(unittest.TestCase):
    """Test enum validation constraints."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'enum_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_enum_defined_only(self):
        """Test enum defined_only constraint."""
        data = self.generator.generate_valid('EnumRules', seed=42)
        self.assertIn('defined_only_field', data)
        # Should be one of: 0, 1, 2, 3 (STATUS_UNKNOWN, ACTIVE, INACTIVE, SUSPENDED)
        self.assertIn(data['defined_only_field'], [0, 1, 2, 3],
                     f"defined_only_field should be a valid Status enum value, got: {data['defined_only_field']}")
    
    def test_enum_const(self):
        """Test enum const constraint."""
        data = self.generator.generate_valid('EnumRules', seed=42)
        self.assertIn('const_field', data)
        self.assertEqual(data['const_field'], 1, "const_field should be 1 (STATUS_ACTIVE)")
    
    def test_enum_color_defined(self):
        """Test enum with different type."""
        data = self.generator.generate_valid('EnumRules', seed=42)
        self.assertIn('color_field', data)
        # Should be one of: 0, 1, 2, 3 (COLOR_UNSPECIFIED, RED, GREEN, BLUE)
        self.assertIn(data['color_field'], [0, 1, 2, 3],
                     f"color_field should be a valid Color enum value, got: {data['color_field']}")


class TestOneofConstraints(unittest.TestCase):
    """Test oneof field behavior."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'oneof_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_oneof_mutual_exclusivity(self):
        """Test that only one oneof field is set."""
        data = self.generator.generate_valid('OneofRules', seed=42)
        
        # Check that common_field (not in oneof) is present
        self.assertIn('common_field', data)
        
        # Count how many oneof fields are set
        oneof_fields = ['str_option', 'int_option', 'float_option']
        set_count = sum(1 for f in oneof_fields if f in data)
        
        self.assertEqual(set_count, 1, 
                        f"Exactly one oneof field should be set, got {set_count}: {[f for f in oneof_fields if f in data]}")
    
    def test_multiple_oneofs(self):
        """Test message with multiple oneof groups."""
        data = self.generator.generate_valid('MultipleOneofs', seed=42)
        
        # First oneof: first_str or first_int
        first_oneof = ['first_str', 'first_int']
        first_count = sum(1 for f in first_oneof if f in data)
        self.assertEqual(first_count, 1, 
                        f"Exactly one field from first_choice should be set, got {first_count}")
        
        # Second oneof: second_str or second_double
        second_oneof = ['second_str', 'second_double']
        second_count = sum(1 for f in second_oneof if f in data)
        self.assertEqual(second_count, 1,
                        f"Exactly one field from second_choice should be set, got {second_count}")
    
    def test_oneof_with_constraints(self):
        """Test that oneof fields respect their constraints."""
        data = self.generator.generate_valid('OneofRules', seed=42)
        
        # If str_option is set, it should respect min_len=3
        if 'str_option' in data:
            self.assertGreaterEqual(len(data['str_option']), 3,
                                   "str_option should have min_len >= 3")
        
        # If int_option is set, it should be in range [0, 1000]
        if 'int_option' in data:
            self.assertGreaterEqual(data['int_option'], 0, "int_option should be >= 0")
            self.assertLessEqual(data['int_option'], 1000, "int_option should be <= 1000")
        
        # If float_option is set, it should be > 0.0
        if 'float_option' in data:
            self.assertGreater(data['float_option'], 0.0, "float_option should be > 0.0")
    
    def test_oneof_deterministic_selection(self):
        """Test custom oneof field selection."""
        # Select specific field
        data = self.generator.generate_valid('OneofRules', seed=42, 
                                            oneof_choice={'choice': 'str_option'})
        self.assertIn('str_option', data, "str_option should be selected")
        self.assertNotIn('int_option', data, "int_option should not be selected")
        self.assertNotIn('float_option', data, "float_option should not be selected")
        
        # Select different field
        data2 = self.generator.generate_valid('OneofRules', seed=42,
                                             oneof_choice={'choice': 'int_option'})
        self.assertNotIn('str_option', data2, "str_option should not be selected")
        self.assertIn('int_option', data2, "int_option should be selected")
        self.assertNotIn('float_option', data2, "float_option should not be selected")


class TestConstraintParsing(unittest.TestCase):
    """Test that validation constraints are correctly parsed."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'numeric_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_constraints_parsed(self):
        """Test that constraints are parsed from proto file."""
        fields = self.generator.get_all_fields('Int32Rules')
        
        # Check lt_field
        lt_field = fields.get('lt_field')
        self.assertIsNotNone(lt_field, "lt_field should exist")
        self.assertGreater(len(lt_field.constraints), 0, "lt_field should have constraints")
        
        # Find lt constraint
        lt_constraints = [c for c in lt_field.constraints if c.rule_type == 'lt']
        self.assertEqual(len(lt_constraints), 1, "Should have exactly one lt constraint")
        self.assertEqual(lt_constraints[0].value, 100, "lt constraint should be 100")
    
    def test_const_constraint_parsed(self):
        """Test that const constraint is parsed correctly."""
        fields = self.generator.get_all_fields('Int32Rules')
        const_field = fields.get('const_field')
        self.assertIsNotNone(const_field, "const_field should exist")
        
        const_constraints = [c for c in const_field.constraints if c.rule_type == 'const']
        self.assertEqual(len(const_constraints), 1, "Should have exactly one const constraint")
        self.assertEqual(const_constraints[0].value, 42, "const constraint should be 42")
    
    def test_range_constraints_parsed(self):
        """Test that range constraints (gte + lte) are parsed."""
        fields = self.generator.get_all_fields('Int32Rules')
        range_field = fields.get('range_field')
        self.assertIsNotNone(range_field, "range_field should exist")
        
        gte_constraints = [c for c in range_field.constraints if c.rule_type == 'gte']
        lte_constraints = [c for c in range_field.constraints if c.rule_type == 'lte']
        
        self.assertEqual(len(gte_constraints), 1, "Should have exactly one gte constraint")
        self.assertEqual(len(lte_constraints), 1, "Should have exactly one lte constraint")
        self.assertEqual(gte_constraints[0].value, 0, "gte constraint should be 0")
        self.assertEqual(lte_constraints[0].value, 150, "lte constraint should be 150")


class TestBinaryEncoding(unittest.TestCase):
    """Test protobuf binary encoding."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        test_proto = Path(__file__).parent.parent / 'tests' / 'validation' / 'numeric_rules.proto'
        cls.generator = DataGenerator(str(test_proto))
    
    def test_encode_to_binary(self):
        """Test encoding data to binary protobuf format."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        binary = self.generator.encode_to_binary('Int32Rules', data)
        
        self.assertIsInstance(binary, bytes, "Encoded data should be bytes")
        self.assertGreater(len(binary), 0, "Encoded data should not be empty")
    
    def test_binary_encoding_deterministic(self):
        """Test that encoding is deterministic."""
        data = self.generator.generate_valid('Int32Rules', seed=42)
        binary1 = self.generator.encode_to_binary('Int32Rules', data)
        binary2 = self.generator.encode_to_binary('Int32Rules', data)
        
        self.assertEqual(binary1, binary2, "Encoding same data should produce identical bytes")


def run_tests():
    """Run all tests."""
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestNumericConstraints))
    suite.addTests(loader.loadTestsFromTestCase(TestStringConstraints))
    suite.addTests(loader.loadTestsFromTestCase(TestEnumConstraints))
    suite.addTests(loader.loadTestsFromTestCase(TestOneofConstraints))
    suite.addTests(loader.loadTestsFromTestCase(TestConstraintParsing))
    suite.addTests(loader.loadTestsFromTestCase(TestBinaryEncoding))
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_tests())
