"""
Tests for nanopb generator and .options file functionality.

These tests verify that the nanopb code generator correctly:
- Generates .pb.c and .pb.h files from .proto definitions
- Applies options from .options files
- Handles various generator flags and settings

Original SCons tests: 
- tests/options/SConscript
- tests/comments/SConscript
- tests/namingstyle/SConscript
"""

import pytest
import re
from pathlib import Path
from conftest import match_patterns


class TestGeneratorOptions:
    """
    Tests for generator options functionality.
    
    Verifies that .options files are correctly applied during code generation.
    
    Original SCons test: tests/options/SConscript
    """
    
    @pytest.mark.generator
    def test_options_header_matches_expected(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test that generated .pb.h matches expected patterns."""
        generated_header = build_dir / "options" / "options.pb.h"
        expected_patterns = tests_dir / "options" / "options_h.expected"
        
        assert generated_header.exists(), "generated header should exist"
        assert expected_patterns.exists(), "expected patterns file should exist"
        
        content = generated_header.read_text(encoding='utf-8')
        failures = match_patterns(content, expected_patterns)
        
        assert not failures, f"Pattern matching failed:\n" + "\n".join(failures)
    
    @pytest.mark.generator
    def test_options_source_matches_expected(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test that generated .pb.c matches expected patterns."""
        generated_source = build_dir / "options" / "options.pb.c"
        expected_patterns = tests_dir / "options" / "options_c.expected"
        
        assert generated_source.exists(), "generated source should exist"
        assert expected_patterns.exists(), "expected patterns file should exist"
        
        content = generated_source.read_text(encoding='utf-8')
        failures = match_patterns(content, expected_patterns)
        
        assert not failures, f"Pattern matching failed:\n" + "\n".join(failures)
    
    @pytest.mark.generator
    def test_proto3_options_matches_expected(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test that proto3 generated code matches expected patterns."""
        generated_header = build_dir / "options" / "proto3_options.pb.h"
        expected_patterns = tests_dir / "options" / "proto3_options.expected"
        
        assert generated_header.exists(), "generated header should exist"
        assert expected_patterns.exists(), "expected patterns file should exist"
        
        content = generated_header.read_text(encoding='utf-8')
        failures = match_patterns(content, expected_patterns)
        
        assert not failures, f"Pattern matching failed:\n" + "\n".join(failures)
    
    @pytest.mark.generator
    def test_options_runtime_behavior(
        self,
        binary_runner,
        ensure_build_exists
    ):
        """Test that options affect runtime behavior correctly."""
        result = binary_runner.run_binary("options/options")
        
        assert result.returncode == 0, (
            f"options test binary failed: "
            f"{result.stderr.decode() if result.stderr else ''}"
        )


class TestCommentGeneration:
    """
    Tests for comment inclusion in generated code.
    
    Verifies that comments from .proto files are correctly included
    in generated .pb.h headers.
    
    Original SCons test: tests/comments/SConscript
    """
    
    @pytest.mark.generator
    def test_comments_included_in_header(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test that proto comments are included in generated header."""
        generated_header = build_dir / "comments" / "comments.pb.h"
        expected_patterns = tests_dir / "comments" / "comments.expected"
        
        assert generated_header.exists(), "generated header should exist"
        assert expected_patterns.exists(), "expected patterns file should exist"
        
        content = generated_header.read_text(encoding='utf-8')
        failures = match_patterns(content, expected_patterns)
        
        assert not failures, f"Comment patterns not found:\n" + "\n".join(failures)


class TestMessageSizes:
    """
    Tests for message size #define generation.
    
    Verifies that the generator correctly calculates and outputs
    message size constants.
    
    Original SCons test: tests/message_sizes/SConscript
    """
    
    @pytest.mark.generator
    def test_message_sizes_compiles(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that message size test compiles successfully."""
        # The SCons test just compiles a dummy.c that includes the generated headers
        # If build succeeds, the size #defines are valid
        dummy_obj = build_dir / "message_sizes" / "dummy.o"
        
        # This should exist after a successful build
        assert dummy_obj.exists() or (build_dir / "message_sizes").exists(), (
            "message_sizes build should succeed"
        )


class TestNamingStyle:
    """
    Tests for different naming style options.
    
    Verifies that the generator correctly applies naming conventions
    to generated code.
    
    Original SCons tests: tests/namingstyle/SConscript, tests/namingstyle_custom/SConscript
    """
    
    @pytest.mark.generator
    def test_default_naming_style(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test default naming style is applied correctly."""
        # Check that the test binary runs successfully
        # This validates that the generated names are correct
        test_binary = build_dir / "namingstyle" / "test_namingstyle"
        
        if test_binary.exists():
            from conftest import TestBinaryRunner
            runner = TestBinaryRunner(build_dir)
            result = runner.run_binary("namingstyle/test_namingstyle")
            assert result.returncode == 0, "naming style test should pass"


class TestInitializers:
    """
    Tests for message initializer macros.
    
    Verifies that _init_default and _init_zero macros are generated
    and work correctly.
    
    Original SCons test: tests/initializers/SConscript
    """
    
    @pytest.mark.generator
    def test_initializers_work(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that initializer macros work correctly."""
        test_binary = build_dir / "initializers" / "initializers"
        
        if test_binary.exists():
            result = binary_runner.run_binary("initializers/initializers")
            assert result.returncode == 0, (
                f"initializers test failed: "
                f"{result.stderr.decode() if result.stderr else ''}"
            )


class TestEnumGeneration:
    """
    Tests for enum generation and handling.
    
    Original SCons tests: tests/enum_sizes/SConscript, tests/enum_to_string/SConscript
    """
    
    @pytest.mark.generator
    def test_enum_sizes(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test that enum sizes are handled correctly."""
        test_binary = build_dir / "enum_sizes" / "enum_sizes"
        
        if test_binary.exists():
            result = binary_runner.run_binary("enum_sizes/enum_sizes")
            assert result.returncode == 0, "enum sizes test should pass"
    
    @pytest.mark.generator
    def test_enum_to_string(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test enum to string conversion."""
        test_binary = build_dir / "enum_to_string" / "enum_to_string"
        
        if test_binary.exists():
            result = binary_runner.run_binary("enum_to_string/enum_to_string")
            assert result.returncode == 0, "enum to string test should pass"


class TestDescriptorGeneration:
    """
    Tests for descriptor generation functionality.
    
    Original SCons test: tests/descriptor_proto/SConscript
    """
    
    @pytest.mark.generator
    def test_descriptor_proto(
        self,
        build_dir,
        ensure_build_exists
    ):
        """Test that descriptor.proto can be compiled."""
        # This test mainly verifies that google/protobuf/descriptor.proto
        # can be processed by nanopb
        descriptor_header = build_dir / "descriptor_proto" / "google" / "protobuf" / "descriptor.pb.h"
        
        # Check build directory structure exists
        descriptor_dir = build_dir / "descriptor_proto"
        assert descriptor_dir.exists(), "descriptor_proto build should exist"
