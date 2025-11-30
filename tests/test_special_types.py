"""
Tests for special Protocol Buffer message features.

These tests verify nanopb's handling of:
- Oneof fields
- Extensions
- Map fields
- Any type
- Cyclic/recursive messages
- Proto3 optional fields

Original SCons tests:
- tests/oneof/SConscript
- tests/extensions/SConscript
- tests/map/SConscript
- tests/any_type/SConscript
- tests/cyclic_messages/SConscript
- tests/proto3_optional/SConscript
"""

import pytest
from pathlib import Path


class TestOneof:
    """
    Tests for oneof field handling.
    
    Original SCons test: tests/oneof/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_oneof(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding oneof fields."""
        # Test encoding each variant
        for variant in ["1", "2", "3"]:
            result = binary_runner.run_binary(
                "oneof/encode_oneof",
                args=[variant]
            )
            
            assert result.returncode == 0, (
                f"encode_oneof variant {variant} failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )
    
    @pytest.mark.decoding
    def test_decode_oneof(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding oneof fields."""
        for variant in ["1", "2", "3"]:
            input_file = build_dir / "oneof" / f"message{variant}.pb"
            
            if input_file.exists():
                result = binary_runner.run_binary(
                    "oneof/decode_oneof",
                    args=[variant],
                    input_file=input_file
                )
                
                assert result.returncode == 0, (
                    f"decode_oneof variant {variant} failed: "
                    f"{result.stderr.decode() if result.stderr else ''}"
                )


class TestAnonymousOneof:
    """
    Tests for anonymous oneof (C11 anonymous union support).
    
    Original SCons test: tests/anonymous_oneof/SConscript
    """
    
    @pytest.mark.decoding
    def test_decode_anonymous_oneof(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding with anonymous oneof unions."""
        for variant in ["1", "2", "3"]:
            # Uses encoder output from oneof test
            input_file = build_dir / "oneof" / f"message{variant}.pb"
            
            if input_file.exists():
                result = binary_runner.run_binary(
                    "anonymous_oneof/decode_oneof",
                    args=[variant],
                    input_file=input_file
                )
                
                assert result.returncode == 0, (
                    f"anonymous_oneof decode variant {variant} failed: "
                    f"{result.stderr.decode() if result.stderr else ''}"
                )


class TestExtensions:
    """
    Tests for protocol buffer extensions.
    
    Original SCons test: tests/extensions/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_extensions(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding messages with extensions."""
        result = binary_runner.run_binary("extensions/encode_extensions")
        
        assert result.returncode == 0, (
            f"encode_extensions failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_extensions(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding messages with extensions."""
        output_file = build_dir / "extensions" / "encode_extensions.output"
        
        result = binary_runner.run_binary(
            "extensions/decode_extensions",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"decode_extensions failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestMap:
    """
    Tests for map field handling.
    
    Tests the backwards-compatible representation of map types.
    
    Original SCons test: tests/map/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_map(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding map fields."""
        result = binary_runner.run_binary("map/encode_map")
        
        assert result.returncode == 0, (
            f"encode_map failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_map(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding map fields."""
        input_file = build_dir / "map" / "message.pb"
        
        result = binary_runner.run_binary(
            "map/decode_map",
            input_file=input_file
        )
        
        assert result.returncode == 0, (
            f"decode_map failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestAnyType:
    """
    Tests for google.protobuf.Any type handling.
    
    Original SCons test: tests/any_type/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_any(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding Any type messages."""
        result = binary_runner.run_binary("any_type/encode_any")
        
        assert result.returncode == 0, (
            f"encode_any failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_any(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding Any type messages."""
        output_file = build_dir / "any_type" / "encode_any.output"
        
        result = binary_runner.run_binary(
            "any_type/decode_any",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"decode_any failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestCyclicMessages:
    """
    Tests for cyclic/recursive message definitions.
    
    Verifies that nanopb handles messages that reference themselves
    or form reference cycles.
    
    Original SCons test: tests/cyclic_messages/SConscript
    
    Note: This test only builds the encoder binary, there is no test to run.
    The purpose is to verify that cyclic messages with callbacks compile.
    """
    
    @pytest.mark.generator
    def test_cyclic_messages_builds(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test cyclic message build succeeds."""
        test_binary = build_dir / "cyclic_messages" / "encode_cyclic_callback"
        
        # This test verifies the build succeeds - the binary needs JSON input
        # to actually run, so we just check it was built
        assert test_binary.exists(), (
            "cyclic_messages binary should be built"
        )


class TestProto3Optional:
    """
    Tests for proto3 optional field support.
    
    Original SCons test: tests/proto3_optional/SConscript
    """
    
    def test_proto3_optional(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test proto3 optional fields."""
        result = binary_runner.run_binary("proto3_optional/optional")
        
        assert result.returncode == 0, (
            f"proto3_optional test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestFixedCount:
    """
    Tests for fixed-count repeated fields.
    
    Original SCons test: tests/fixed_count/SConscript
    """
    
    def test_fixed_count(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test fixed-count repeated field handling."""
        result = binary_runner.run_binary("fixed_count/fixed_count_unittests")
        
        assert result.returncode == 0, (
            f"fixed_count test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestRecursiveProto:
    """
    Tests for recursive message definitions.
    
    Original SCons test: tests/recursive_proto/SConscript
    """
    
    def test_recursive_proto(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test recursive message handling."""
        test_binary = build_dir / "recursive_proto" / "recursive_proto"
        
        if test_binary.exists():
            result = binary_runner.run_binary("recursive_proto/recursive_proto")
            
            assert result.returncode == 0, (
                f"recursive_proto test failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestMsgId:
    """
    Tests for message ID functionality.
    
    Original SCons test: tests/msgid/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_msgid(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test message ID encoding for all message types."""
        # Test all three message types
        for msg_type in ["1", "2", "3"]:
            result = binary_runner.run_binary(
                "msgid/encode_msgid",
                args=[msg_type]
            )
            
            assert result.returncode == 0, (
                f"encode_msgid type {msg_type} failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )
    
    @pytest.mark.decoding
    def test_decode_msgid(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test message ID decoding."""
        for msg_type in ["1", "2", "3"]:
            input_file = build_dir / "msgid" / f"message{msg_type}.pb"
            
            if input_file.exists():
                result = binary_runner.run_binary(
                    "msgid/decode_msgid",
                    input_file=input_file
                )
                
                assert result.returncode == 0, (
                    f"decode_msgid type {msg_type} failed: "
                    f"{result.stderr.decode() if result.stderr else ''}"
                )


class TestMultipleFiles:
    """
    Tests for multiple file proto definitions.
    
    Original SCons test: tests/multiple_files/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_multiple_files(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding with multiple proto files."""
        result = binary_runner.run_binary("multiple_files/test_multiple_files")
        
        assert result.returncode == 0, (
            f"multiple_files test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestFloatDoubleConversion:
    """
    Tests for float/double conversion handling.
    
    Original SCons test: tests/float_double_conversion/SConscript
    """
    
    def test_float_double_conversion(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test float/double conversion."""
        result = binary_runner.run_binary("float_double_conversion/float_double_conversion")
        
        assert result.returncode == 0, (
            f"float_double_conversion test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestInfinityNan:
    """
    Tests for infinity and NaN handling.
    
    Original SCons test: tests/infinity_nan/SConscript
    """
    
    def test_infinity_nan(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test infinity and NaN encoding/decoding."""
        result = binary_runner.run_binary("infinity_nan/infinity_nan_test")
        
        assert result.returncode == 0, (
            f"infinity_nan test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
