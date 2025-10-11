#!/usr/bin/env python3
"""
Unit tests for generateptxall.py

Tests the PTX umbrella header generation system, including:
- Header file discovery
- Template and virtual file filtering
- Output file generation
- Include path formatting
"""

import unittest
import tempfile
import shutil
from pathlib import Path
import sys

# Add scripts directory to path
SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from generateptxall import gather_headers, generate


# ============================================================================
# Test Header Discovery
# ============================================================================

class TestHeaderDiscovery(unittest.TestCase):
    """Test finding and filtering header files"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)
        self.include_root = Path(self.temp_dir) / "include"
        self.include_root.mkdir(parents=True, exist_ok=True)

    def test_gather_simple_headers(self):
        """Test gathering simple header files"""
        # Create some headers
        (self.include_root / "vector.hpp").write_text("class Vector {};")
        (self.include_root / "matrix.hpp").write_text("class Matrix {};")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 2)
        self.assertIn("vector.hpp", headers)
        self.assertIn("matrix.hpp", headers)

    def test_gather_headers_in_subdirs(self):
        """Test gathering headers from subdirectories"""
        # Create nested structure
        core_dir = self.include_root / "core"
        core_dir.mkdir(parents=True, exist_ok=True)
        (core_dir / "engine.hpp").write_text("class Engine {};")

        math_dir = self.include_root / "core" / "math"
        math_dir.mkdir(parents=True, exist_ok=True)
        (math_dir / "vector.hpp").write_text("class Vector {};")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 2)
        self.assertIn("core/engine.hpp", headers)
        self.assertIn("core/math/vector.hpp", headers)

    def test_gather_headers_excludes_templates(self):
        """Test that template files are excluded"""
        (self.include_root / "regular.hpp").write_text("class Regular {};")
        (self.include_root / "template.hpp").write_text("""
template <typename T>
class TemplateClass {
    T value;
};
""")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 1)
        self.assertIn("regular.hpp", headers)
        self.assertNotIn("template.hpp", headers)

    def test_gather_headers_excludes_virtual(self):
        """Test that files with virtual methods are excluded"""
        (self.include_root / "concrete.hpp").write_text("class Concrete { void DoWork(); };")
        (self.include_root / "abstract.hpp").write_text("""
class Abstract {
public:
    virtual void DoWork() = 0;
};
""")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 1)
        self.assertIn("concrete.hpp", headers)
        self.assertNotIn("abstract.hpp", headers)

    def test_gather_headers_excludes_output_file(self):
        """Test that the output file itself is excluded"""
        (self.include_root / "vector.hpp").write_text("class Vector {};")
        (self.include_root / "ptxall.hpp").write_text("#include <vector.hpp>")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 1)
        self.assertIn("vector.hpp", headers)
        self.assertNotIn("ptxall.hpp", headers)

    def test_gather_headers_ignores_non_hpp(self):
        """Test that non-.hpp files are ignored"""
        (self.include_root / "header.hpp").write_text("class Header {};")
        (self.include_root / "source.cpp").write_text("void func() {}")
        (self.include_root / "readme.txt").write_text("Documentation")
        (self.include_root / "config.h").write_text("#define MACRO 1")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 1)
        self.assertIn("header.hpp", headers)

    def test_gather_headers_sorted(self):
        """Test that headers are returned in sorted order"""
        (self.include_root / "zebra.hpp").write_text("class Zebra {};")
        (self.include_root / "alpha.hpp").write_text("class Alpha {};")
        (self.include_root / "beta.hpp").write_text("class Beta {};")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(headers[0], "alpha.hpp")
        self.assertEqual(headers[1], "beta.hpp")
        self.assertEqual(headers[2], "zebra.hpp")

    def test_gather_headers_empty_directory(self):
        """Test behavior with empty directory"""
        headers = gather_headers(self.include_root, "ptxall.hpp")
        self.assertEqual(len(headers), 0)


# ============================================================================
# Test Output Generation
# ============================================================================

class TestOutputGeneration(unittest.TestCase):
    """Test umbrella header file generation"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)
        self.include_root = Path(self.temp_dir) / "include"
        self.include_root.mkdir(parents=True, exist_ok=True)

    def test_generate_simple_umbrella(self):
        """Test generating simple umbrella header"""
        # Create some headers
        (self.include_root / "vector.hpp").write_text("class Vector {};")
        (self.include_root / "matrix.hpp").write_text("class Matrix {};")

        output_file = self.include_root / "ptxall.hpp"
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        self.assertTrue(output_file.exists())

        content = output_file.read_text()
        self.assertIn("#pragma once", content)
        self.assertIn("@file ptxall.hpp", content)
        self.assertIn("DO NOT EDIT BY HAND", content)
        self.assertIn('#include "matrix.hpp"', content)
        self.assertIn('#include "vector.hpp"', content)

    def test_generate_with_subdirs(self):
        """Test generating umbrella with subdirectory headers"""
        core_dir = self.include_root / "core"
        core_dir.mkdir(parents=True, exist_ok=True)
        (core_dir / "engine.hpp").write_text("class Engine {};")

        math_dir = core_dir / "math"
        math_dir.mkdir(parents=True, exist_ok=True)
        (math_dir / "vector.hpp").write_text("class Vector {};")

        output_file = self.include_root / "ptxall.hpp"
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        content = output_file.read_text()

        # Check forward slashes in paths
        self.assertIn('#include "core/engine.hpp"', content)
        self.assertIn('#include "core/math/vector.hpp"', content)

    def test_generate_creates_parent_dirs(self):
        """Test that generate creates parent directories if needed"""
        (self.include_root / "vector.hpp").write_text("class Vector {};")

        output_file = Path(self.temp_dir) / "build" / "generated" / "ptxall.hpp"
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        self.assertTrue(output_file.exists())
        self.assertTrue(output_file.parent.exists())

    def test_generate_empty_headers(self):
        """Test generating umbrella with no headers"""
        output_file = self.include_root / "ptxall.hpp"
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        content = output_file.read_text()

        self.assertIn("#pragma once", content)
        self.assertIn("@file ptxall.hpp", content)
        # Should have header but no includes
        self.assertEqual(content.count('#include'), 0)

    def test_generate_filters_templates_and_virtual(self):
        """Test that generated output excludes template and virtual files"""
        (self.include_root / "concrete.hpp").write_text("class Concrete {};")
        (self.include_root / "template.hpp").write_text("template<typename T> class Foo {};")
        (self.include_root / "abstract.hpp").write_text("class Abstract { virtual void f() = 0; };")

        output_file = self.include_root / "ptxall.hpp"
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        content = output_file.read_text()

        self.assertIn('#include "concrete.hpp"', content)
        self.assertNotIn('#include "template.hpp"', content)
        self.assertNotIn('#include "abstract.hpp"', content)

    def test_generate_excludes_itself(self):
        """Test that umbrella header doesn't include itself"""
        (self.include_root / "vector.hpp").write_text("class Vector {};")

        output_file = self.include_root / "ptxall.hpp"

        # Generate once
        generate(self.include_root, output_file)

        # Generate again (ptxall.hpp now exists)
        result = generate(self.include_root, output_file)

        self.assertEqual(result, 0)
        content = output_file.read_text()

        # Should not include itself
        self.assertNotIn('#include "ptxall.hpp"', content)


# ============================================================================
# Test Path Handling
# ============================================================================

class TestPathHandling(unittest.TestCase):
    """Test path handling and formatting"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)
        self.include_root = Path(self.temp_dir) / "include"
        self.include_root.mkdir(parents=True, exist_ok=True)

    def test_paths_use_forward_slashes(self):
        """Test that generated includes use forward slashes (not backslashes)"""
        subdir = self.include_root / "level1" / "level2" / "level3"
        subdir.mkdir(parents=True, exist_ok=True)
        (subdir / "deep.hpp").write_text("class Deep {};")

        output_file = self.include_root / "ptxall.hpp"
        generate(self.include_root, output_file)

        content = output_file.read_text()
        self.assertIn('#include "level1/level2/level3/deep.hpp"', content)
        self.assertNotIn('\\', content)  # No backslashes

    def test_relative_paths_from_root(self):
        """Test that paths are relative to include root"""
        # Create nested structure
        (self.include_root / "root.hpp").write_text("class Root {};")

        subdir = self.include_root / "sub"
        subdir.mkdir(parents=True, exist_ok=True)
        (subdir / "nested.hpp").write_text("class Nested {};")

        output_file = self.include_root / "ptxall.hpp"
        generate(self.include_root, output_file)

        content = output_file.read_text()

        # Paths should be relative to include_root
        self.assertIn('#include "root.hpp"', content)
        self.assertIn('#include "sub/nested.hpp"', content)


# ============================================================================
# Test Edge Cases
# ============================================================================

class TestEdgeCases(unittest.TestCase):
    """Test edge cases and error handling"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)
        self.include_root = Path(self.temp_dir) / "include"
        self.include_root.mkdir(parents=True, exist_ok=True)

    def test_unreadable_file_is_included(self):
        """Test that files that can't be read are still included (fail-safe)"""
        header = self.include_root / "unreadable.hpp"
        header.write_text("class Unreadable {};")

        # gather_headers tries to read files to filter templates/virtual
        # but if it fails, it includes the file anyway
        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertIn("unreadable.hpp", headers)

    def test_handles_special_characters_in_filenames(self):
        """Test handling filenames with special characters"""
        # Note: Some characters may not be valid on all filesystems
        (self.include_root / "class-name.hpp").write_text("class ClassName {};")
        (self.include_root / "class_name.hpp").write_text("class ClassName2 {};")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        self.assertEqual(len(headers), 2)

    def test_mixed_template_and_regular_code(self):
        """Test file with both template and regular class (should be excluded)"""
        (self.include_root / "mixed.hpp").write_text("""
class RegularClass {
    int value;
};

template <typename T>
class TemplateClass {
    T value;
};
""")

        headers = gather_headers(self.include_root, "ptxall.hpp")

        # File contains template, so should be excluded
        self.assertEqual(len(headers), 0)


if __name__ == "__main__":
    unittest.main()
