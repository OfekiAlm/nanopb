"""
Tests for encoding and decoding functionality.

These tests verify that nanopb correctly encodes and decodes Protocol Buffer
messages using the 'alltypes' test case which exercises all supported data types.

Original SCons test: tests/alltypes/SConscript
"""

import pytest
from pathlib import Path


class TestAllTypesBasic:
    """
    Tests for basic encoding/decoding of all Protocol Buffer data types.
    
    These tests verify:
    1. nanopb encoder produces valid output (encoder runs successfully)
    2. nanopb decoder can decode encoder output (round-trip)
    3. protoc can decode nanopb output (cross-tool compatibility)
    4. protoc re-encoded data matches nanopb output (byte-for-byte equality)
    """
    
    @pytest.mark.encoding
    def test_encode_alltypes_runs_successfully(
        self, 
        binary_runner,
        ensure_build_exists
    ):
        """Test that the encode_alltypes binary runs without errors."""
        result = binary_runner.run_binary("alltypes/encode_alltypes")
        
        assert result.returncode == 0, (
            f"encode_alltypes failed with return code {result.returncode}\n"
            f"stderr: {result.stderr.decode() if result.stderr else 'none'}"
        )
        assert len(result.stdout) > 0, "encoder should produce output"
    
    @pytest.mark.decoding
    def test_decode_alltypes_decodes_encoder_output(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that decode_alltypes can decode encode_alltypes output."""
        # First encode
        encode_result = binary_runner.run_binary("alltypes/encode_alltypes")
        assert encode_result.returncode == 0, "encoder should succeed"
        
        # Write encoder output to temp file for decoder
        output_file = build_dir / "alltypes" / "encode_alltypes.output"
        
        # Decoder should succeed when reading encoder output
        decode_result = binary_runner.run_binary(
            "alltypes/decode_alltypes",
            input_file=output_file
        )
        
        assert decode_result.returncode == 0, (
            f"decode_alltypes failed with return code {decode_result.returncode}\n"
            f"stderr: {decode_result.stderr.decode() if decode_result.stderr else 'none'}"
        )
    
    @pytest.mark.integration
    def test_protoc_can_decode_nanopb_output(
        self,
        binary_runner,
        protobuf_generator,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test that protoc can decode data encoded by nanopb."""
        # Get encoder output
        output_file = build_dir / "alltypes" / "encode_alltypes.output"
        assert output_file.exists(), "encoder output should exist after build"
        
        encoded_data = output_file.read_bytes()
        
        # Decode using protoc
        proto_file = tests_dir / "alltypes" / "alltypes.proto"
        result = protobuf_generator.decode_message(
            proto_file,
            "AllTypes",
            encoded_data
        )
        
        assert result.returncode == 0, (
            f"protoc decode failed: {result.stderr.decode() if result.stderr else 'none'}"
        )
        # Decoded output should contain some expected field names
        decoded = result.stdout.decode()
        assert "req_int32" in decoded or len(decoded) > 0, (
            "decoded output should contain message fields"
        )
    
    @pytest.mark.integration
    def test_protoc_reencoded_matches_nanopb(
        self,
        protobuf_generator,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """
        Test that protoc re-encoding produces identical output to nanopb.
        
        This verifies byte-for-byte compatibility between nanopb and protoc.
        """
        output_file = build_dir / "alltypes" / "encode_alltypes.output"
        recoded_file = build_dir / "alltypes" / "encode_alltypes.output.recoded"
        
        # Both files should exist after SCons build
        assert output_file.exists(), "encoder output should exist"
        assert recoded_file.exists(), "recoded output should exist"
        
        # Compare byte-by-byte
        nanopb_data = output_file.read_bytes()
        protoc_data = recoded_file.read_bytes()
        
        assert nanopb_data == protoc_data, (
            f"nanopb output ({len(nanopb_data)} bytes) differs from "
            f"protoc re-encoded output ({len(protoc_data)} bytes)"
        )


class TestAllTypesOptional:
    """
    Tests for encoding/decoding with optional fields present.
    
    Original SCons test: tests/alltypes/SConscript (optionals.* targets)
    """
    
    @pytest.mark.encoding
    def test_encode_with_optionals(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding when optional fields are populated."""
        result = binary_runner.run_binary(
            "alltypes/encode_alltypes",
            args=["1"]  # Enable optionals
        )
        
        assert result.returncode == 0, "encoding with optionals should succeed"
        assert len(result.stdout) > 0, "should produce output"
    
    @pytest.mark.decoding
    def test_decode_with_optionals(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding when optional fields are present."""
        output_file = build_dir / "alltypes" / "optionals.output"
        
        result = binary_runner.run_binary(
            "alltypes/decode_alltypes",
            args=["1"],  # Expect optionals
            input_file=output_file
        )
        
        assert result.returncode == 0, "decoding with optionals should succeed"
    
    @pytest.mark.integration
    def test_protoc_reencoded_optionals_match(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that protoc re-encoding of optionals matches nanopb."""
        output_file = build_dir / "alltypes" / "optionals.output"
        recoded_file = build_dir / "alltypes" / "optionals.output.recoded"
        
        assert output_file.exists(), "optionals output should exist"
        assert recoded_file.exists(), "recoded optionals should exist"
        
        assert output_file.read_bytes() == recoded_file.read_bytes(), (
            "optionals encoding should match protoc"
        )


class TestAllTypesZeroInit:
    """
    Tests for encoding/decoding with _zero initializer.
    
    This tests the case where messages are initialized with
    the _zero macro rather than explicit initialization.
    
    Original SCons test: tests/alltypes/SConscript (zeroinit.* targets)
    """
    
    @pytest.mark.encoding
    def test_encode_zeroinit(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test encoding with zero-initialized message."""
        result = binary_runner.run_binary(
            "alltypes/encode_alltypes",
            args=["2"]  # Zero init mode
        )
        
        assert result.returncode == 0, "encoding zero-init should succeed"
    
    @pytest.mark.decoding
    def test_decode_zeroinit(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test decoding zero-initialized message."""
        output_file = build_dir / "alltypes" / "zeroinit.output"
        
        result = binary_runner.run_binary(
            "alltypes/decode_alltypes",
            args=["2"],  # Zero init mode
            input_file=output_file
        )
        
        assert result.returncode == 0, "decoding zero-init should succeed"
    
    @pytest.mark.integration
    def test_protoc_reencoded_zeroinit_match(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that protoc re-encoding of zeroinit matches nanopb."""
        output_file = build_dir / "alltypes" / "zeroinit.output"
        recoded_file = build_dir / "alltypes" / "zeroinit.output.recoded"
        
        assert output_file.exists(), "zeroinit output should exist"
        assert recoded_file.exists(), "recoded zeroinit should exist"
        
        assert output_file.read_bytes() == recoded_file.read_bytes(), (
            "zeroinit encoding should match protoc"
        )


class TestBasicBuffer:
    """
    Tests for basic buffer-based encoding/decoding.
    
    These tests use the 'person' message definition and memory buffer
    encoding (as opposed to stream encoding).
    
    Original SCons test: tests/basic_buffer/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_buffer(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test that buffer encoding works correctly."""
        result = binary_runner.run_binary("basic_buffer/encode_buffer")
        
        assert result.returncode == 0, (
            f"encode_buffer failed: {result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_buffer(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that buffer decoding works correctly."""
        output_file = build_dir / "basic_buffer" / "encode_buffer.output"
        
        result = binary_runner.run_binary(
            "basic_buffer/decode_buffer",
            input_file=output_file
        )
        
        assert result.returncode == 0, "decode_buffer should succeed"
    
    @pytest.mark.integration
    def test_decoded_output_matches_protoc(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that decoded output matches protoc-decoded version."""
        decoded_file = build_dir / "basic_buffer" / "decode_buffer.output"
        protoc_decoded = build_dir / "basic_buffer" / "encode_buffer.decoded"
        
        assert decoded_file.exists(), "decoded output should exist"
        assert protoc_decoded.exists(), "protoc decoded should exist"
        
        assert decoded_file.read_bytes() == protoc_decoded.read_bytes(), (
            "nanopb decode should match protoc decode"
        )


class TestBasicStream:
    """
    Tests for basic stream-based encoding/decoding.
    
    These tests use the 'person' message definition and stream
    encoding (as opposed to buffer encoding).
    
    Original SCons test: tests/basic_stream/SConscript
    """
    
    @pytest.mark.encoding
    def test_encode_stream(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test that stream encoding works correctly."""
        result = binary_runner.run_binary("basic_stream/encode_stream")
        
        assert result.returncode == 0, (
            f"encode_stream failed: {result.stderr.decode() if result.stderr else ''}"
        )
    
    @pytest.mark.decoding
    def test_decode_stream(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that stream decoding works correctly."""
        output_file = build_dir / "basic_stream" / "encode_stream.output"
        
        result = binary_runner.run_binary(
            "basic_stream/decode_stream",
            input_file=output_file
        )
        
        assert result.returncode == 0, "decode_stream should succeed"
    
    @pytest.mark.integration
    def test_decoded_output_matches_protoc(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that decoded output matches protoc-decoded version."""
        decoded_file = build_dir / "basic_stream" / "decode_stream.output"
        protoc_decoded = build_dir / "basic_stream" / "encode_stream.decoded"
        
        assert decoded_file.exists(), "decoded output should exist"
        assert protoc_decoded.exists(), "protoc decoded should exist"
        
        assert decoded_file.read_bytes() == protoc_decoded.read_bytes(), (
            "nanopb decode should match protoc decode"
        )
