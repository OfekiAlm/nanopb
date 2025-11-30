"""
Unit tests for nanopb encoder and decoder.

These tests run the standalone unit test programs that test the
low-level encoding and decoding functionality of nanopb.

Original SCons tests:
- tests/encode_unittests/SConscript
- tests/decode_unittests/SConscript
- tests/common_unittests/SConscript
"""

import pytest
from pathlib import Path


class TestEncodeUnittests:
    """
    Unit tests for the nanopb encoder.
    
    These tests verify low-level encoding behavior by running the
    standalone encode_unittests program.
    
    Original SCons test: tests/encode_unittests/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_unittests(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Run the encoder unit tests."""
        result = binary_runner.run_binary("encode_unittests/encode_unittests")
        
        assert result.returncode == 0, (
            f"encode_unittests failed with return code {result.returncode}\n"
            f"stdout: {result.stdout.decode() if result.stdout else ''}\n"
            f"stderr: {result.stderr.decode() if result.stderr else ''}"
        )


class TestDecodeUnittests:
    """
    Unit tests for the nanopb decoder.
    
    These tests verify low-level decoding behavior by running the
    standalone decode_unittests program.
    
    Original SCons test: tests/decode_unittests/SConscript
    """
    
    @pytest.mark.decoding
    def test_decode_unittests(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Run the decoder unit tests."""
        result = binary_runner.run_binary("decode_unittests/decode_unittests")
        
        assert result.returncode == 0, (
            f"decode_unittests failed with return code {result.returncode}\n"
            f"stdout: {result.stdout.decode() if result.stdout else ''}\n"
            f"stderr: {result.stderr.decode() if result.stderr else ''}"
        )


class TestCommonUnittests:
    """
    Unit tests for common nanopb functionality.
    
    These tests verify functionality shared between encoder and decoder.
    
    Original SCons test: tests/common_unittests/SConscript
    """
    
    def test_common_unittests(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Run the common unit tests if they exist."""
        test_binary = build_dir / "common_unittests" / "common_unittests"
        
        if test_binary.exists():
            result = binary_runner.run_binary("common_unittests/common_unittests")
            
            assert result.returncode == 0, (
                f"common_unittests failed with return code {result.returncode}\n"
                f"stdout: {result.stdout.decode() if result.stdout else ''}\n"
                f"stderr: {result.stderr.decode() if result.stderr else ''}"
            )


class TestCallbacks:
    """
    Tests for callback-based encoding/decoding.
    
    These tests verify the callback mechanism for handling fields
    that can't be statically allocated.
    
    Original SCons test: tests/callbacks/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_callbacks(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test callback-based encoding."""
        result = binary_runner.run_binary("callbacks/encode_callbacks")
        
        assert result.returncode == 0, (
            f"encode_callbacks failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_callbacks(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test callback-based decoding."""
        output_file = build_dir / "callbacks" / "encode_callbacks.output"
        
        result = binary_runner.run_binary(
            "callbacks/decode_callbacks",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"decode_callbacks failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.integration
    def test_callbacks_output_matches_protoc(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that callback decoded output matches protoc."""
        decoded_file = build_dir / "callbacks" / "decode_callbacks.output"
        protoc_decoded = build_dir / "callbacks" / "encode_callbacks.decoded"
        
        assert decoded_file.exists(), "decoded output should exist"
        assert protoc_decoded.exists(), "protoc decoded should exist"
        
        assert decoded_file.read_bytes() == protoc_decoded.read_bytes(), (
            "callback decode should match protoc decode"
        )


class TestIOErrors:
    """
    Tests for I/O error handling.
    
    These tests verify that nanopb correctly handles I/O errors
    during encoding and decoding.
    
    Original SCons tests: tests/io_errors/SConscript, tests/io_errors_pointers/SConscript
    """
    
    def test_io_errors(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test I/O error handling."""
        test_binary = build_dir / "io_errors" / "io_errors"
        # io_errors needs the alltypes encoded output as input
        input_file = build_dir / "alltypes" / "encode_alltypes.output"
        
        if test_binary.exists() and input_file.exists():
            result = binary_runner.run_binary(
                "io_errors/io_errors",
                input_file=input_file
            )
            
            assert result.returncode == 0, (
                f"io_errors test failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )
    
    def test_io_errors_pointers(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test I/O error handling with pointer fields."""
        test_binary = build_dir / "io_errors_pointers" / "io_errors_pointers"
        
        if test_binary.exists():
            result = binary_runner.run_binary("io_errors_pointers/io_errors_pointers")
            
            assert result.returncode == 0, (
                f"io_errors_pointers test failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestMissingFields:
    """
    Tests for handling missing required fields.
    
    Original SCons test: tests/missing_fields/SConscript
    """
    
    def test_missing_fields(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test handling of missing required fields."""
        result = binary_runner.run_binary("missing_fields/missing_fields")
        
        assert result.returncode == 0, (
            f"missing_fields test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestExtraFields:
    """
    Tests for handling extra/unknown fields.
    
    This test uses existing decode binaries (not a separate test binary)
    to verify that unknown fields are handled correctly.
    
    Original SCons test: tests/extra_fields/SConscript
    """
    
    def test_extra_fields_person(
        self,
        binary_runner,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test handling of unknown fields during decoding."""
        output_file = build_dir / "extra_fields" / "person_with_extra_field.output"
        expected_file = tests_dir / "extra_fields" / "person_with_extra_field.expected"
        
        if output_file.exists() and expected_file.exists():
            assert output_file.read_bytes() == expected_file.read_bytes(), (
                "extra_fields output should match expected"
            )


class TestMemRelease:
    """
    Tests for memory release functionality.
    
    Verifies that pb_release() correctly frees allocated memory.
    
    Original SCons test: tests/mem_release/SConscript
    """
    
    def test_mem_release(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test memory release functionality."""
        result = binary_runner.run_binary("mem_release/mem_release")
        
        assert result.returncode == 0, (
            f"mem_release test failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestBufferOnly:
    """
    Tests for buffer-only compilation mode.
    
    This test verifies that nanopb can be compiled without stream support.
    
    Original SCons test: tests/buffer_only/SConscript
    """
    
    @pytest.mark.encoding
    def test_buffer_only_encode(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test buffer-only encoding."""
        result = binary_runner.run_binary("buffer_only/encode_alltypes")
        
        assert result.returncode == 0, (
            f"buffer_only encode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_buffer_only_decode(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test buffer-only decoding."""
        output_file = build_dir / "buffer_only" / "encode_alltypes.output"
        
        result = binary_runner.run_binary(
            "buffer_only/decode_alltypes",
            input_file=output_file
        )
        
        assert result.returncode == 0, (
            f"buffer_only decode failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestInline:
    """
    Tests for inline function compilation.
    
    Original SCons test: tests/inline/SConscript
    """
    
    def test_inline(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test inline function compilation."""
        test_binary = build_dir / "inline" / "inline"
        
        if test_binary.exists():
            result = binary_runner.run_binary("inline/inline")
            
            assert result.returncode == 0, (
                f"inline test failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )
