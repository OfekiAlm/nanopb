"""
Pytest configuration and shared fixtures for nanopb tests.

This module provides fixtures and helper functions that enable pytest to
orchestrate nanopb's test suite while using SCons as the build engine.

Key concepts:
    - SCons is used for all C compilation and protobuf generation
    - pytest handles test orchestration, assertions, and reporting
    - Tests can run individual test binaries or validate generated code
"""

import os
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path
from typing import List, Optional, Dict, Any

import pytest


# =============================================================================
# Path Constants
# =============================================================================

# Repository root directory
REPO_ROOT = Path(__file__).parent.parent.absolute()

# Tests directory
TESTS_DIR = Path(__file__).parent.absolute()

# Default build directory (used by SCons)
DEFAULT_BUILD_DIR = TESTS_DIR / "build"

# Path to nanopb generator
GENERATOR_DIR = REPO_ROOT / "generator"

# Path to protoc wrapper
PROTOC_PATH = GENERATOR_DIR / "protoc"


# =============================================================================
# Utility Functions
# =============================================================================

def find_protoc() -> Path:
    """Find the protoc compiler to use."""
    # First check for bundled protoc
    bundled_protoc = GENERATOR_DIR / "protoc"
    if bundled_protoc.exists():
        return bundled_protoc
    
    # Fall back to system protoc
    result = shutil.which("protoc")
    if result:
        return Path(result)
    
    raise RuntimeError("Could not find protoc compiler")


def find_nanopb_generator() -> Path:
    """Find the nanopb generator script."""
    generator_script = GENERATOR_DIR / "nanopb_generator.py"
    if generator_script.exists():
        return generator_script
    
    raise RuntimeError("Could not find nanopb generator")


# =============================================================================
# Fixtures
# =============================================================================

@pytest.fixture(scope="session")
def repo_root() -> Path:
    """Return the repository root directory."""
    return REPO_ROOT


@pytest.fixture(scope="session")
def tests_dir() -> Path:
    """Return the tests directory."""
    return TESTS_DIR


@pytest.fixture(scope="session")
def build_dir() -> Path:
    """
    Return the SCons build directory.
    
    This directory contains all compiled binaries and generated .pb.c/.pb.h files
    after running SCons.
    """
    return DEFAULT_BUILD_DIR


@pytest.fixture(scope="session")
def protoc_path() -> Path:
    """Return the path to the protoc compiler."""
    return find_protoc()


@pytest.fixture(scope="session")
def nanopb_generator() -> Path:
    """Return the path to the nanopb generator script."""
    return find_nanopb_generator()


@pytest.fixture
def temp_dir(tmp_path: Path) -> Path:
    """
    Provide a temporary directory for test-specific files.
    
    This is automatically cleaned up after each test.
    """
    return tmp_path


# =============================================================================
# SCons Build Helpers
# =============================================================================

class SconsRunner:
    """Helper class to run SCons builds for tests."""
    
    def __init__(self, tests_dir: Path, build_dir: Path):
        self.tests_dir = tests_dir
        self.build_dir = build_dir
    
    def build_target(
        self,
        target: str,
        extra_args: Optional[List[str]] = None,
        env: Optional[Dict[str, str]] = None,
        timeout: int = 300
    ) -> subprocess.CompletedProcess:
        """
        Build a specific SCons target.
        
        Args:
            target: The SCons target to build (e.g., "build/alltypes/encode_alltypes")
            extra_args: Additional arguments to pass to SCons
            env: Additional environment variables
            timeout: Timeout in seconds
            
        Returns:
            CompletedProcess with the result
        """
        cmd = ["scons", "-j1", "NODEFARGS=1", target]
        if extra_args:
            cmd.extend(extra_args)
        
        run_env = os.environ.copy()
        if env:
            run_env.update(env)
        
        return subprocess.run(
            cmd,
            cwd=str(self.tests_dir),
            env=run_env,
            capture_output=True,
            text=True,
            timeout=timeout
        )
    
    def build_test_suite(
        self,
        test_name: str,
        extra_args: Optional[List[str]] = None,
        timeout: int = 300
    ) -> bool:
        """
        Build all targets for a specific test suite.
        
        Args:
            test_name: Name of the test directory (e.g., "alltypes", "basic_buffer")
            extra_args: Additional arguments to pass to SCons
            timeout: Timeout in seconds
            
        Returns:
            True if build succeeded, False otherwise
        """
        target = f"build/{test_name}"
        result = self.build_target(target, extra_args, timeout=timeout)
        return result.returncode == 0
    
    def full_build(
        self,
        extra_args: Optional[List[str]] = None,
        timeout: int = 600
    ) -> subprocess.CompletedProcess:
        """
        Run a full SCons build of all tests.
        
        Args:
            extra_args: Additional arguments to pass to SCons
            timeout: Timeout in seconds
            
        Returns:
            CompletedProcess with the result
        """
        cmd = ["scons", "-j1", "NODEFARGS=1"]
        if extra_args:
            cmd.extend(extra_args)
        
        return subprocess.run(
            cmd,
            cwd=str(self.tests_dir),
            capture_output=True,
            text=True,
            timeout=timeout
        )


@pytest.fixture(scope="session")
def scons_runner(tests_dir: Path, build_dir: Path) -> SconsRunner:
    """Provide a SCons runner for building test targets."""
    return SconsRunner(tests_dir, build_dir)


@pytest.fixture(scope="session")
def ensure_build_exists(scons_runner: SconsRunner, build_dir: Path):
    """
    Ensure that the SCons build directory exists with all compiled tests.
    
    This fixture runs a full SCons build if the build directory doesn't exist
    or is empty. It's session-scoped to avoid rebuilding for every test.
    """
    if not build_dir.exists() or not any(build_dir.iterdir()):
        result = scons_runner.full_build()
        if result.returncode != 0:
            pytest.fail(f"SCons build failed:\n{result.stderr}")
    
    return build_dir


# =============================================================================
# Test Binary Runner
# =============================================================================

class TestBinaryRunner:
    """Helper class to run compiled test binaries."""
    
    def __init__(self, build_dir: Path):
        self.build_dir = build_dir
    
    def run_binary(
        self,
        binary_path: str,
        args: Optional[List[str]] = None,
        input_file: Optional[Path] = None,
        timeout: int = 60
    ) -> subprocess.CompletedProcess:
        """
        Run a compiled test binary.
        
        Args:
            binary_path: Path to binary relative to build directory
            args: Command line arguments
            input_file: File to use as stdin
            timeout: Timeout in seconds
            
        Returns:
            CompletedProcess with stdout, stderr, and return code
        """
        full_path = self.build_dir / binary_path
        
        if not full_path.exists():
            raise FileNotFoundError(f"Binary not found: {full_path}")
        
        cmd = [str(full_path)]
        if args:
            cmd.extend(args)
        
        stdin_data = None
        if input_file and input_file.exists():
            stdin_data = input_file.read_bytes()
        
        return subprocess.run(
            cmd,
            input=stdin_data,
            capture_output=True,
            timeout=timeout
        )
    
    def run_encoder_decoder_pair(
        self,
        encoder_path: str,
        decoder_path: str,
        encoder_args: Optional[List[str]] = None,
        decoder_args: Optional[List[str]] = None,
        timeout: int = 60
    ) -> tuple:
        """
        Run an encoder/decoder pair where decoder receives encoder's output.
        
        Args:
            encoder_path: Path to encoder binary relative to build dir
            decoder_path: Path to decoder binary relative to build dir
            encoder_args: Arguments for encoder
            decoder_args: Arguments for decoder
            timeout: Timeout in seconds
            
        Returns:
            Tuple of (encoder_result, decoder_result)
        """
        # Run encoder
        encoder_result = self.run_binary(encoder_path, encoder_args, timeout=timeout)
        
        if encoder_result.returncode != 0:
            return encoder_result, None
        
        # Create temp file with encoder output
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(encoder_result.stdout)
            temp_path = Path(f.name)
        
        try:
            # Run decoder with encoder output as input
            decoder_result = self.run_binary(
                decoder_path, 
                decoder_args, 
                input_file=temp_path,
                timeout=timeout
            )
        finally:
            temp_path.unlink()
        
        return encoder_result, decoder_result


@pytest.fixture(scope="session")
def binary_runner(build_dir: Path) -> TestBinaryRunner:
    """Provide a test binary runner."""
    return TestBinaryRunner(build_dir)


# =============================================================================
# Protobuf Generation Helpers
# =============================================================================

class ProtobufGenerator:
    """Helper class to generate nanopb files from .proto definitions."""
    
    def __init__(self, protoc_path: Path, generator_dir: Path):
        self.protoc_path = protoc_path
        self.generator_dir = generator_dir
    
    def generate(
        self,
        proto_file: Path,
        output_dir: Path,
        extra_flags: Optional[List[str]] = None,
        options_file: Optional[Path] = None
    ) -> subprocess.CompletedProcess:
        """
        Generate .pb.c and .pb.h files from a .proto file.
        
        Args:
            proto_file: Path to the .proto file
            output_dir: Directory for output files
            extra_flags: Additional flags for nanopb generator
            options_file: Path to .options file
            
        Returns:
            CompletedProcess with the result
        """
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Build protoc command
        cmd = [
            str(self.protoc_path),
            f"-I{proto_file.parent}",
            f"-I{self.generator_dir / 'proto'}",
            f"--nanopb_out={output_dir}",
            str(proto_file)
        ]
        
        if extra_flags:
            # Modify the nanopb_out to include flags
            flags_str = ",".join(extra_flags)
            cmd[-2] = f"--nanopb_out={flags_str}:{output_dir}"
        
        return subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )
    
    def decode_message(
        self,
        proto_file: Path,
        message_type: str,
        encoded_data: bytes
    ) -> subprocess.CompletedProcess:
        """
        Decode a protobuf message using protoc.
        
        Args:
            proto_file: Path to the .proto file
            message_type: Name of the message type
            encoded_data: The encoded protobuf data
            
        Returns:
            CompletedProcess with decoded text representation
        """
        cmd = [
            str(self.protoc_path),
            f"-I{proto_file.parent}",
            f"-I{self.generator_dir / 'proto'}",
            f"--decode={message_type}",
            str(proto_file)
        ]
        
        return subprocess.run(
            cmd,
            input=encoded_data,
            capture_output=True
        )
    
    def encode_message(
        self,
        proto_file: Path,
        message_type: str,
        text_data: str
    ) -> subprocess.CompletedProcess:
        """
        Encode a protobuf message using protoc.
        
        Args:
            proto_file: Path to the .proto file
            message_type: Name of the message type
            text_data: The text format representation
            
        Returns:
            CompletedProcess with encoded binary data in stdout
        """
        cmd = [
            str(self.protoc_path),
            f"-I{proto_file.parent}",
            f"-I{self.generator_dir / 'proto'}",
            f"--encode={message_type}",
            str(proto_file)
        ]
        
        return subprocess.run(
            cmd,
            input=text_data.encode(),
            capture_output=True
        )


@pytest.fixture(scope="session")
def protobuf_generator(protoc_path: Path, nanopb_generator: Path) -> ProtobufGenerator:
    """Provide a protobuf generator helper."""
    return ProtobufGenerator(protoc_path, GENERATOR_DIR)


# =============================================================================
# File Comparison Helpers
# =============================================================================

def compare_files(file1: Path, file2: Path) -> bool:
    """
    Compare two files for equality.
    
    Args:
        file1: First file path
        file2: Second file path
        
    Returns:
        True if files are identical, False otherwise
    """
    if not file1.exists() or not file2.exists():
        return False
    
    return file1.read_bytes() == file2.read_bytes()


def match_patterns(content: str, pattern_file: Path) -> List[str]:
    """
    Check if content matches all patterns in a pattern file.
    
    Pattern file format:
        - Each line is a regex pattern
        - Lines starting with '! ' are inverted (pattern should NOT match)
        - Empty lines are ignored
    
    Args:
        content: The content to check
        pattern_file: Path to file containing patterns
        
    Returns:
        List of failed pattern descriptions (empty if all pass)
    """
    import re
    
    failures = []
    patterns = pattern_file.read_text(encoding='utf-8').splitlines()
    
    for pattern in patterns:
        pattern = pattern.strip()
        if not pattern:
            continue
        
        invert = False
        if pattern.startswith('! '):
            invert = True
            pattern = pattern[2:]
        
        match = re.search(pattern, content, re.MULTILINE)
        
        if not match and not invert:
            failures.append(f"Pattern not found: {pattern}")
        elif match and invert:
            failures.append(f"Pattern should not exist: {pattern}")
    
    return failures


# =============================================================================
# Pytest Markers
# =============================================================================

def pytest_configure(config):
    """Configure custom pytest markers."""
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers", "integration: marks tests as integration tests"
    )
    config.addinivalue_line(
        "markers", "generator: marks tests related to code generation"
    )
    config.addinivalue_line(
        "markers", "encoding: marks tests related to encoding"
    )
    config.addinivalue_line(
        "markers", "decoding: marks tests related to decoding"
    )
    config.addinivalue_line(
        "markers", "regression: marks regression tests"
    )


# =============================================================================
# Pytest Hooks
# =============================================================================

def pytest_collection_modifyitems(config, items):
    """Modify test collection based on markers."""
    # Add slow marker to tests in certain directories
    slow_dirs = ["fuzztest", "stackusage"]
    
    for item in items:
        # Get the test's directory relative to tests/
        test_path = Path(item.fspath)
        if test_path.is_relative_to(TESTS_DIR):
            rel_path = test_path.relative_to(TESTS_DIR)
            parts = rel_path.parts
            
            if parts and parts[0] in slow_dirs:
                item.add_marker(pytest.mark.slow)
