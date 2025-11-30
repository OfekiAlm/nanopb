"""
Regression tests for nanopb.

These tests verify fixes for specific issues that have been reported.
Each test corresponds to a GitHub issue number.

The tests are organized to preserve historical context - when a bug
is fixed, a regression test is added to ensure it doesn't reappear.

Original SCons tests: tests/regression/*/SConscript
"""

import pytest
from pathlib import Path
from conftest import match_patterns


class TestRegressionIssue118:
    """
    Regression test for Issue 118.
    
    Original SCons test: tests/regression/issue_118/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_118(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 118."""
        test_binary = build_dir / "regression" / "issue_118" / "test_issue_118"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_118/test_issue_118")
            assert result.returncode == 0, f"Issue 118 regression: {result.stderr}"


class TestRegressionIssue125:
    """
    Regression test for Issue 125: Wrong identifier name for extension fields.
    
    Original SCons test: tests/regression/issue_125/SConscript
    """
    
    @pytest.mark.regression
    @pytest.mark.generator
    def test_issue_125(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Test fix for issue 125 - extension field identifiers."""
        generated_header = build_dir / "regression" / "issue_125" / "extensionbug.pb.h"
        expected_patterns = tests_dir / "regression" / "issue_125" / "extensionbug.expected"
        
        if generated_header.exists() and expected_patterns.exists():
            content = generated_header.read_text(encoding='utf-8')
            failures = match_patterns(content, expected_patterns)
            
            assert not failures, f"Issue 125 regression:\n" + "\n".join(failures)


class TestRegressionIssue141:
    """
    Regression test for Issue 141.
    
    Original SCons test: tests/regression/issue_141/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_141(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 141."""
        test_binary = build_dir / "regression" / "issue_141" / "test_issue_141"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_141/test_issue_141")
            assert result.returncode == 0, f"Issue 141 regression: {result.stderr}"


class TestRegressionIssue145:
    """
    Regression test for Issue 145.
    
    Original SCons test: tests/regression/issue_145/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_145(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 145."""
        test_binary = build_dir / "regression" / "issue_145" / "test_issue_145"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_145/test_issue_145")
            assert result.returncode == 0, f"Issue 145 regression: {result.stderr}"


class TestRegressionIssue166:
    """
    Regression test for Issue 166.
    
    Original SCons test: tests/regression/issue_166/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_166(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 166."""
        test_binary = build_dir / "regression" / "issue_166" / "test_issue_166"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_166/test_issue_166")
            assert result.returncode == 0, f"Issue 166 regression: {result.stderr}"


class TestRegressionIssue172:
    """
    Regression test for Issue 172.
    
    Original SCons test: tests/regression/issue_172/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_172(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 172."""
        test_binary = build_dir / "regression" / "issue_172" / "test_issue_172"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_172/test_issue_172")
            assert result.returncode == 0, f"Issue 172 regression: {result.stderr}"


class TestRegressionIssue188:
    """
    Regression test for Issue 188.
    
    Original SCons test: tests/regression/issue_188/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_188(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 188."""
        test_binary = build_dir / "regression" / "issue_188" / "test_issue_188"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_188/test_issue_188")
            assert result.returncode == 0, f"Issue 188 regression: {result.stderr}"


class TestRegressionIssue195:
    """
    Regression test for Issue 195.
    
    Original SCons test: tests/regression/issue_195/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_195(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 195."""
        test_binary = build_dir / "regression" / "issue_195" / "test_issue_195"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_195/test_issue_195")
            assert result.returncode == 0, f"Issue 195 regression: {result.stderr}"


class TestRegressionIssue203:
    """
    Regression test for Issue 203.
    
    Original SCons test: tests/regression/issue_203/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_203(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 203."""
        test_binary = build_dir / "regression" / "issue_203" / "test_issue_203"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_203/test_issue_203")
            assert result.returncode == 0, f"Issue 203 regression: {result.stderr}"


class TestRegressionIssue205:
    """
    Regression test for Issue 205.
    
    Original SCons test: tests/regression/issue_205/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_205(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 205."""
        test_binary = build_dir / "regression" / "issue_205" / "test_issue_205"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_205/test_issue_205")
            assert result.returncode == 0, f"Issue 205 regression: {result.stderr}"


class TestRegressionIssue227:
    """
    Regression test for Issue 227.
    
    Original SCons test: tests/regression/issue_227/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_227(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 227."""
        test_binary = build_dir / "regression" / "issue_227" / "test_issue_227"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_227/test_issue_227")
            assert result.returncode == 0, f"Issue 227 regression: {result.stderr}"


class TestRegressionIssue229:
    """
    Regression test for Issue 229.
    
    Original SCons test: tests/regression/issue_229/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_229(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 229."""
        test_binary = build_dir / "regression" / "issue_229" / "test_issue_229"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_229/test_issue_229")
            assert result.returncode == 0, f"Issue 229 regression: {result.stderr}"


class TestRegressionIssue242:
    """
    Regression test for Issue 242.
    
    Original SCons test: tests/regression/issue_242/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_242(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 242."""
        test_binary = build_dir / "regression" / "issue_242" / "test_issue_242"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_242/test_issue_242")
            assert result.returncode == 0, f"Issue 242 regression: {result.stderr}"


class TestRegressionIssue247:
    """
    Regression test for Issue 247.
    
    Original SCons test: tests/regression/issue_247/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_247(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 247."""
        test_binary = build_dir / "regression" / "issue_247" / "test_issue_247"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_247/test_issue_247")
            assert result.returncode == 0, f"Issue 247 regression: {result.stderr}"


class TestRegressionIssue249:
    """
    Regression test for Issue 249.
    
    Original SCons test: tests/regression/issue_249/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_249(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 249."""
        test_binary = build_dir / "regression" / "issue_249" / "test_issue_249"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_249/test_issue_249")
            assert result.returncode == 0, f"Issue 249 regression: {result.stderr}"


class TestRegressionIssue253:
    """
    Regression test for Issue 253.
    
    Original SCons test: tests/regression/issue_253/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_253(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 253."""
        test_binary = build_dir / "regression" / "issue_253" / "test_issue_253"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_253/test_issue_253")
            assert result.returncode == 0, f"Issue 253 regression: {result.stderr}"


class TestRegressionIssue256:
    """
    Regression test for Issue 256.
    
    Original SCons test: tests/regression/issue_256/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_256(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 256."""
        test_binary = build_dir / "regression" / "issue_256" / "test_issue_256"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_256/test_issue_256")
            assert result.returncode == 0, f"Issue 256 regression: {result.stderr}"


class TestRegressionIssue259:
    """
    Regression test for Issue 259.
    
    Original SCons test: tests/regression/issue_259/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_259(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 259."""
        test_binary = build_dir / "regression" / "issue_259" / "test_issue_259"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_259/test_issue_259")
            assert result.returncode == 0, f"Issue 259 regression: {result.stderr}"


class TestRegressionIssue306:
    """
    Regression test for Issue 306.
    
    Original SCons test: tests/regression/issue_306/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_306(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 306."""
        test_binary = build_dir / "regression" / "issue_306" / "test_issue_306"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_306/test_issue_306")
            assert result.returncode == 0, f"Issue 306 regression: {result.stderr}"


class TestRegressionIssue322:
    """
    Regression test for Issue 322.
    
    Original SCons test: tests/regression/issue_322/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_322(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 322."""
        test_binary = build_dir / "regression" / "issue_322" / "test_issue_322"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_322/test_issue_322")
            assert result.returncode == 0, f"Issue 322 regression: {result.stderr}"


class TestRegressionIssue338:
    """
    Regression test for Issue 338 - memory usage issue.
    
    Original SCons test: tests/regression/issue_338/SConscript
    """
    
    @pytest.mark.regression
    def test_issue_338(
        self,
        binary_runner,
        build_dir,
        ensure_build_exists
    ):
        """Test fix for issue 338 - memory usage."""
        test_binary = build_dir / "regression" / "issue_338" / "test_issue_338"
        
        if test_binary.exists():
            result = binary_runner.run_binary("regression/issue_338/test_issue_338")
            assert result.returncode == 0, f"Issue 338 regression: {result.stderr}"


# Helper function to dynamically generate regression tests
def get_regression_test_dirs(tests_dir: Path) -> list:
    """Get list of regression test directories."""
    regression_dir = tests_dir / "regression"
    if not regression_dir.exists():
        return []
    
    return [d.name for d in regression_dir.iterdir() if d.is_dir()]


# Parametrized test for all regression issues
class TestAllRegressions:
    """
    Parametrized tests for all regression test directories.
    
    This provides a way to run all regression tests without needing
    to manually create a class for each one.
    """
    
    @pytest.mark.regression
    def test_regression_builds_exist(
        self,
        build_dir,
        tests_dir,
        ensure_build_exists
    ):
        """Verify that regression test directories were built."""
        regression_build = build_dir / "regression"
        
        if regression_build.exists():
            # Count directories with built content
            built_dirs = [d for d in regression_build.iterdir() if d.is_dir()]
            assert len(built_dirs) > 0, "Some regression tests should be built"
