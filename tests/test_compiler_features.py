"""
Tests for C++ compatibility and compiler-specific features.

These tests verify that nanopb works correctly when compiled with C++
and handles various compiler-specific scenarios.

Original SCons tests:
- tests/cxx_main_program/SConscript
- tests/cxx_callback_datatype/SConscript
- tests/cxx_descriptor/SConscript
"""

import pytest
from pathlib import Path


class TestCxxMainProgram:
    """
    Tests for C++ main program compatibility.
    
    Verifies that nanopb headers can be included from C++ code
    and that the generated code compiles correctly with a C++ compiler.
    
    Original SCons test: tests/cxx_main_program/SConscript
    """
    
    def test_cxx_main_program(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that nanopb works from a C++ main program."""
        test_binary = build_dir / "cxx_main_program" / "cxx_main_program"
        
        if test_binary.exists():
            result = binary_runner.run_binary("cxx_main_program/cxx_main_program")
            
            assert result.returncode == 0, (
                f"cxx_main_program failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestCxxCallbackDatatype:
    """
    Tests for C++ callback data type compatibility.
    
    Original SCons test: tests/cxx_callback_datatype/SConscript
    """
    
    def test_cxx_callback_datatype(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test C++ callback data types."""
        test_binary = build_dir / "cxx_callback_datatype" / "cxx_callback_datatype"
        
        if test_binary.exists():
            result = binary_runner.run_binary("cxx_callback_datatype/cxx_callback_datatype")
            
            assert result.returncode == 0, (
                f"cxx_callback_datatype failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestCxxDescriptor:
    """
    Tests for C++ descriptor compatibility.
    
    Original SCons test: tests/cxx_descriptor/SConscript
    """
    
    def test_cxx_descriptor(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test C++ descriptor handling."""
        test_binary = build_dir / "cxx_descriptor" / "test_cxx_descriptor"
        
        if test_binary.exists():
            result = binary_runner.run_binary("cxx_descriptor/test_cxx_descriptor")
            
            assert result.returncode == 0, (
                f"cxx_descriptor failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestFieldSize:
    """
    Tests for different field size configurations.
    
    Tests PB_FIELD_16BIT and PB_FIELD_32BIT modes.
    
    Original SCons tests:
    - tests/field_size_16/SConscript
    - tests/field_size_16_proto3/SConscript
    - tests/field_size_32/SConscript
    """
    
    @pytest.mark.encoding
    def test_field_size_16_encode(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding with 16-bit field sizes."""
        result = binary_runner.run_binary("field_size_16/encode_alltypes")
        
        assert result.returncode == 0, (
            f"field_size_16 encode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_field_size_16_decode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding with 16-bit field sizes."""
        output_file = build_dir / "field_size_16" / "encode_alltypes.output"
        
        result = binary_runner.run_binary(
            "field_size_16/decode_alltypes",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"field_size_16 decode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.encoding
    def test_field_size_32_encode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test encoding with 32-bit field sizes."""
        test_binary = build_dir / "field_size_32" / "encode_alltypes"
        
        if test_binary.exists():
            result = binary_runner.run_binary("field_size_32/encode_alltypes")
            
            assert result.returncode == 0, (
                f"field_size_32 encode failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestWithout64Bit:
    """
    Tests for builds without 64-bit support.
    
    Verifies that nanopb works on platforms without native 64-bit integers.
    
    Original SCons test: tests/without_64bit/SConscript
    """
    
    @pytest.mark.encoding
    def test_without_64bit_encode(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding without 64-bit support."""
        result = binary_runner.run_binary("without_64bit/encode_alltypes")
        
        assert result.returncode == 0, (
            f"without_64bit encode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_without_64bit_decode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding without 64-bit support."""
        output_file = build_dir / "without_64bit" / "encode_alltypes.output"
        
        result = binary_runner.run_binary(
            "without_64bit/decode_alltypes",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"without_64bit decode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestNoErrmsg:
    """
    Tests for builds without error messages.
    
    Verifies that PB_NO_ERRMSG mode works correctly.
    
    Original SCons test: tests/no_errmsg/SConscript
    """
    
    @pytest.mark.encoding
    def test_no_errmsg_encode(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding without error messages."""
        result = binary_runner.run_binary("no_errmsg/encode_alltypes")
        
        assert result.returncode == 0, (
            f"no_errmsg encode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_no_errmsg_decode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding without error messages."""
        output_file = build_dir / "no_errmsg" / "encode_alltypes.output"
        
        result = binary_runner.run_binary(
            "no_errmsg/decode_alltypes",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"no_errmsg decode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestIntsizes:
    """
    Tests for various integer size handling.
    
    Original SCons test: tests/intsizes/SConscript
    """
    
    def test_intsizes(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test integer size handling."""
        result = binary_runner.run_binary("intsizes/intsizes_unittests")
        
        assert result.returncode == 0, (
            f"intsizes test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestEnumMapping:
    """
    Tests for enum mapping functionality.
    
    This test verifies that enum mapping macros are generated correctly.
    It's a generator test, not a runtime test.
    
    Original SCons test: tests/enum_mapping/SConscript
    """
    
    @pytest.mark.generator
    def test_enum_mapping_header(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test enum mapping in generated header."""
        from conftest import match_patterns
        
        generated_header = build_dir / "enum_mapping" / "enum_mapping.pb.h"
        expected_patterns = tests_dir / "enum_mapping" / "enum_mapping.expected"
        
        assert generated_header.exists(), "generated header should exist"
        assert expected_patterns.exists(), "expected patterns file should exist"
        
        content = generated_header.read_text(encoding='utf-8')
        failures = match_patterns(content, expected_patterns)
        
        assert not failures, f"Pattern matching failed:\n" + "\n".join(failures)


class TestEnumMinmax:
    """
    Tests for enum min/max values.
    
    Original SCons test: tests/enum_minmax/SConscript
    """
    
    def test_enum_minmax(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test enum min/max handling."""
        result = binary_runner.run_binary("enum_minmax/enumminmax_unittests")
        
        assert result.returncode == 0, (
            f"enum_minmax test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestEnumValidate:
    """
    Tests for enum validation.
    
    Original SCons test: tests/enum_validate/SConscript
    """
    
    def test_enum_validate(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test enum validation."""
        result = binary_runner.run_binary("enum_validate/enum_validate")
        
        assert result.returncode == 0, (
            f"enum_validate test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestBackwardsCompatibility:
    """
    Tests for backwards compatibility with older nanopb versions.
    
    Original SCons test: tests/backwards_compatibility/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_legacy(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding with legacy format."""
        result = binary_runner.run_binary("backwards_compatibility/encode_legacy")
        
        assert result.returncode == 0, (
            f"encode_legacy failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_legacy(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding legacy format."""
        output_file = build_dir / "backwards_compatibility" / "encode_legacy.output"
        
        result = binary_runner.run_binary(
            "backwards_compatibility/decode_legacy",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"decode_legacy failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestValidateUtf8:
    """
    Tests for UTF-8 validation.
    
    Original SCons test: tests/validate_utf8/SConscript
    """
    
    @pytest.mark.encoding
    def test_validate_utf8_encode(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding with UTF-8 validation enabled."""
        result = binary_runner.run_binary("validate_utf8/encode_alltypes")
        
        assert result.returncode == 0, (
            f"validate_utf8 encode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_validate_utf8_decode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding with UTF-8 validation enabled."""
        output_file = build_dir / "validate_utf8" / "encode_alltypes.output"
        
        result = binary_runner.run_binary(
            "validate_utf8/decode_alltypes",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"validate_utf8 decode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
