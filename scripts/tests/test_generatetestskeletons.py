#!/usr/bin/env python3
"""
Unit tests for GenerateTestSkeletons.py

Tests the test skeleton generation and validation system, including:
- Reflected class discovery
- Header file location
- Method extraction from PTX_METHODS blocks
- Test file generation
- Test validation and synchronization
- Cache management
"""

import unittest
import tempfile
import shutil
import json
import hashlib
from pathlib import Path
from datetime import datetime
import sys

# Add scripts directory to path
SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from generatetestskeletons import (
    compute_file_hash,
    load_cache,
    save_cache,
    is_cache_valid,
    parse_class_methods,
    generate_test_method_name,
    compute_expected_test_methods,
    generate_test_header_content,
    generate_test_impl_content,
    generate_test_method_impl,
    find_header_file,
    determine_category,
    MethodInfo,
    ClassInfo
)


# ============================================================================
# Test Cache Management
# ============================================================================

class TestCacheManagement(unittest.TestCase):
    """Test cache file operations"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

    def test_compute_file_hash(self):
        """Test SHA256 hash computation"""
        test_file = Path(self.temp_dir) / "test.txt"
        test_file.write_text("Hello World")

        hash1 = compute_file_hash(test_file)
        self.assertEqual(len(hash1), 64)  # SHA256 produces 64 hex chars

        # Same content = same hash
        test_file2 = Path(self.temp_dir) / "test2.txt"
        test_file2.write_text("Hello World")
        hash2 = compute_file_hash(test_file2)
        self.assertEqual(hash1, hash2)

        # Different content = different hash
        test_file2.write_text("Different")
        hash3 = compute_file_hash(test_file2)
        self.assertNotEqual(hash1, hash3)

    def test_compute_file_hash_missing_file(self):
        """Test hash of non-existent file returns empty string"""
        missing = Path(self.temp_dir) / "missing.txt"
        hash_result = compute_file_hash(missing)
        self.assertEqual(hash_result, "")

    def test_load_cache_missing_file(self):
        """Test loading non-existent cache returns default"""
        missing_cache = Path(self.temp_dir) / "missing.json"
        cache = load_cache(missing_cache)

        self.assertEqual(cache["reflection_hash"], "")
        self.assertEqual(cache["validated_tests"], {})
        self.assertEqual(cache["version"], "2.0")

    def test_load_cache_valid(self):
        """Test loading valid cache file"""
        cache_file = Path(self.temp_dir) / "cache.json"
        cache_data = {
            "reflection_hash": "abc123",
            "validated_tests": {
                "Vector3D": {
                    "methods": ["TestAdd", "TestSubtract"],
                    "category": "core/math"
                }
            },
            "version": "2.0"
        }
        cache_file.write_text(json.dumps(cache_data))

        loaded = load_cache(cache_file)
        self.assertEqual(loaded["reflection_hash"], "abc123")
        self.assertIn("Vector3D", loaded["validated_tests"])

    def test_load_cache_corrupted(self):
        """Test loading corrupted cache returns default"""
        cache_file = Path(self.temp_dir) / "cache.json"
        cache_file.write_text("{ invalid json }")

        cache = load_cache(cache_file)
        self.assertEqual(cache["version"], "2.0")
        self.assertEqual(cache["validated_tests"], {})

    def test_save_cache(self):
        """Test saving cache to file"""
        cache_file = Path(self.temp_dir) / "cache.json"
        cache_data = {
            "reflection_hash": "test123",
            "validated_tests": {},
            "version": "2.0"
        }

        save_cache(cache_file, cache_data)

        self.assertTrue(cache_file.exists())
        loaded = json.loads(cache_file.read_text())
        self.assertEqual(loaded["reflection_hash"], "test123")

    def test_is_cache_valid(self):
        """Test cache validation based on reflection file hash"""
        cache_file = Path(self.temp_dir) / "cache.json"
        reflection_file = Path(self.temp_dir) / "reflection.cpp"
        reflection_file.write_text("void TestClass::Describe();")

        reflection_hash = compute_file_hash(reflection_file)
        cache_data = {
            "reflection_hash": reflection_hash,
            "validated_tests": {},
            "version": "2.0"
        }
        save_cache(cache_file, cache_data)

        # Cache should be valid
        self.assertTrue(is_cache_valid(cache_file, reflection_file))

        # Modify reflection file - cache should be invalid
        reflection_file.write_text("void TestClass::Describe();\nvoid NewClass::Describe();")
        self.assertFalse(is_cache_valid(cache_file, reflection_file))


# ============================================================================
# Test Method Parsing
# ============================================================================

class TestMethodParsing(unittest.TestCase):
    """Test extraction of methods from header files"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

    def test_parse_class_methods_simple(self):
        """Test parsing simple PTX_METHODS block"""
        header = Path(self.temp_dir) / "simple.hpp"
        header.write_text("""
class SimpleClass {
public:
    SimpleClass();
    int GetValue() const;

    PTX_BEGIN_METHODS(SimpleClass)
        PTX_METHOD_AUTO(SimpleClass, GetValue, "Get value")
    PTX_END_METHODS
};
""")

        methods, has_default_ctor = parse_class_methods(header, "SimpleClass")

        self.assertTrue(has_default_ctor)
        self.assertEqual(len(methods), 1)
        self.assertEqual(methods[0].name, "GetValue")
        self.assertFalse(methods[0].is_static)

    def test_parse_class_methods_multiple(self):
        """Test parsing multiple methods"""
        header = Path(self.temp_dir) / "multiple.hpp"
        header.write_text("""
class MultiClass {
public:
    void Add(int x);
    void Remove(int x);
    int Count();

    PTX_BEGIN_METHODS(MultiClass)
        PTX_METHOD_AUTO(MultiClass, Add, "Add element")
        PTX_METHOD_AUTO(MultiClass, Remove, "Remove element")
        PTX_METHOD_AUTO(MultiClass, Count, "Get count")
    PTX_END_METHODS
};
""")

        methods, _ = parse_class_methods(header, "MultiClass")

        self.assertEqual(len(methods), 3)
        method_names = [m.name for m in methods]
        self.assertIn("Add", method_names)
        self.assertIn("Remove", method_names)
        self.assertIn("Count", method_names)

    def test_parse_class_methods_static(self):
        """Test parsing static methods"""
        header = Path(self.temp_dir) / "static.hpp"
        header.write_text("""
class StaticClass {
public:
    static int GetCount();
    void DoWork();

    PTX_BEGIN_METHODS(StaticClass)
        PTX_SMETHOD_AUTO(StaticClass, GetCount, "Get count")
        PTX_METHOD_AUTO(StaticClass, DoWork, "Do work")
    PTX_END_METHODS
};
""")

        methods, _ = parse_class_methods(header, "StaticClass")

        self.assertEqual(len(methods), 2)

        # Find the static method
        get_count = next(m for m in methods if m.name == "GetCount")
        self.assertTrue(get_count.is_static)

        # Regular method should not be static
        do_work = next(m for m in methods if m.name == "DoWork")
        self.assertFalse(do_work.is_static)

    def test_parse_class_methods_overloaded(self):
        """Test parsing overloaded methods (deduplicated)"""
        header = Path(self.temp_dir) / "overload.hpp"
        header.write_text("""
class OverloadClass {
public:
    void Set(int x);
    void Set(float x);

    PTX_BEGIN_METHODS(OverloadClass)
        PTX_METHOD_OVLD(OverloadClass, Set, "Set int", int)
        PTX_METHOD_OVLD(OverloadClass, Set, "Set float", float)
    PTX_END_METHODS
};
""")

        methods, _ = parse_class_methods(header, "OverloadClass")

        # Should only have one "Set" method (deduplication)
        self.assertEqual(len(methods), 1)
        self.assertEqual(methods[0].name, "Set")

    def test_parse_class_methods_no_block(self):
        """Test class without PTX_METHODS block"""
        header = Path(self.temp_dir) / "noblock.hpp"
        header.write_text("""
class NoBlockClass {
public:
    int value;
};
""")

        methods, _ = parse_class_methods(header, "NoBlockClass")

        self.assertEqual(len(methods), 0)

    def test_parse_class_methods_no_default_ctor(self):
        """Test class without default constructor"""
        header = Path(self.temp_dir) / "nodefault.hpp"
        header.write_text("""
class NoDefaultClass {
public:
    NoDefaultClass(int x);  // Only parameterized

    PTX_BEGIN_METHODS(NoDefaultClass)
    PTX_END_METHODS
};
""")

        methods, has_default_ctor = parse_class_methods(header, "NoDefaultClass")

        self.assertFalse(has_default_ctor)


# ============================================================================
# Test Method Name Generation
# ============================================================================

class TestMethodNameGeneration(unittest.TestCase):
    """Test test method name generation"""

    def test_generate_test_method_name(self):
        """Test generating test method names"""
        self.assertEqual(generate_test_method_name("Add"), "TestAdd")
        self.assertEqual(generate_test_method_name("GetValue"), "TestGetValue")
        self.assertEqual(generate_test_method_name("IsValid"), "TestIsValid")

    def test_compute_expected_test_methods_simple(self):
        """Test computing expected test methods for simple class"""
        class_info = ClassInfo(
            name="SimpleClass",
            qualified_name="SimpleClass",
            header_path=Path("/fake/simple.hpp"),
            methods=[
                MethodInfo(name="GetValue", return_type="int", is_static=False),
                MethodInfo(name="SetValue", return_type="void", is_static=False)
            ],
            has_default_constructor=True,
            category="core"
        )

        expected = compute_expected_test_methods(class_info)

        self.assertIn("TestDefaultConstructor", expected)
        self.assertIn("TestParameterizedConstructor", expected)
        self.assertIn("TestGetValue", expected)
        self.assertIn("TestSetValue", expected)
        self.assertIn("TestEdgeCases", expected)

    def test_compute_expected_test_methods_no_methods(self):
        """Test class with no methods still gets constructor tests"""
        class_info = ClassInfo(
            name="EmptyClass",
            qualified_name="EmptyClass",
            header_path=Path("/fake/empty.hpp"),
            methods=[],
            has_default_constructor=True,
            category="core"
        )

        expected = compute_expected_test_methods(class_info)

        # Should have constructors and edge cases
        self.assertIn("TestDefaultConstructor", expected)
        self.assertIn("TestParameterizedConstructor", expected)
        self.assertIn("TestEdgeCases", expected)
        self.assertEqual(len(expected), 3)


# ============================================================================
# Test File Generation
# ============================================================================

class TestFileGeneration(unittest.TestCase):
    """Test test file content generation"""

    def test_generate_test_method_impl_default_ctor(self):
        """Test generating default constructor test"""
        impl = generate_test_method_impl("TestVector3D", "TestDefaultConstructor", "Vector3D")

        self.assertIn("void TestVector3D::TestDefaultConstructor()", impl)
        self.assertIn("Vector3D obj;", impl)
        self.assertIn("TEST_ASSERT_TRUE(false)", impl)
        self.assertIn("Not implemented", impl)

    def test_generate_test_method_impl_parameterized_ctor(self):
        """Test generating parameterized constructor test"""
        impl = generate_test_method_impl("TestVector3D", "TestParameterizedConstructor", "Vector3D")

        self.assertIn("void TestVector3D::TestParameterizedConstructor()", impl)
        self.assertIn("TEST_ASSERT_TRUE(false)", impl)

    def test_generate_test_method_impl_edge_cases(self):
        """Test generating edge cases test"""
        impl = generate_test_method_impl("TestVector3D", "TestEdgeCases", "Vector3D")

        self.assertIn("void TestVector3D::TestEdgeCases()", impl)
        self.assertIn("edge cases", impl.lower())
        self.assertIn("TEST_ASSERT_TRUE(false)", impl)

    def test_generate_test_method_impl_regular_method(self):
        """Test generating regular method test"""
        impl = generate_test_method_impl("TestVector3D", "TestAdd", "Vector3D")

        self.assertIn("void TestVector3D::TestAdd()", impl)
        self.assertIn("Vector3D obj;", impl)
        self.assertIn("test for Add()", impl)
        self.assertIn("TEST_ASSERT_TRUE(false)", impl)

    def test_generate_test_header_content(self):
        """Test generating complete test header file"""
        class_info = ClassInfo(
            name="Vector3D",
            qualified_name="Vector3D",
            header_path=Path("/engine/include/ptx/core/math/vector3d.hpp"),
            methods=[
                MethodInfo(name="Add", return_type="Vector3D", is_static=False),
                MethodInfo(name="Magnitude", return_type="float", is_static=False)
            ],
            has_default_constructor=True,
            category="core/math"
        )

        test_methods = [
            "TestDefaultConstructor",
            "TestParameterizedConstructor",
            "TestAdd",
            "TestMagnitude",
            "TestEdgeCases"
        ]

        content = generate_test_header_content(class_info, test_methods)

        # Check structure
        self.assertIn("#pragma once", content)
        self.assertIn("#include <unity.h>", content)
        self.assertIn("class TestVector3D", content)
        self.assertIn("static void TestDefaultConstructor();", content)
        self.assertIn("static void TestAdd();", content)
        self.assertIn("static void TestMagnitude();", content)
        self.assertIn("static void TestEdgeCases();", content)
        self.assertIn("static void RunAllTests();", content)

        # Check include path (may be relative or just filename)
        self.assertTrue("vector3d.hpp" in content)

    def test_generate_test_impl_content(self):
        """Test generating complete test implementation file"""
        class_info = ClassInfo(
            name="Vector3D",
            qualified_name="Vector3D",
            header_path=Path("/engine/include/ptx/core/math/vector3d.hpp"),
            methods=[
                MethodInfo(name="Add", return_type="Vector3D", is_static=False)
            ],
            has_default_constructor=True,
            category="core/math"
        )

        test_methods = [
            "TestDefaultConstructor",
            "TestParameterizedConstructor",
            "TestAdd",
            "TestEdgeCases"
        ]

        content = generate_test_impl_content(class_info, test_methods)

        # Check structure
        self.assertIn('#include "testvector3d.hpp"', content)
        self.assertIn("void TestVector3D::TestDefaultConstructor()", content)
        self.assertIn("void TestVector3D::TestParameterizedConstructor()", content)
        self.assertIn("void TestVector3D::TestAdd()", content)
        self.assertIn("void TestVector3D::TestEdgeCases()", content)
        self.assertIn("void TestVector3D::RunAllTests()", content)

        # Check RUN_TEST calls
        self.assertIn("RUN_TEST(TestDefaultConstructor);", content)
        self.assertIn("RUN_TEST(TestAdd);", content)
        self.assertIn("RUN_TEST(TestEdgeCases);", content)

        # Check section headers
        self.assertIn("Constructor Tests", content)
        self.assertIn("Method Tests", content)
        self.assertIn("Edge Cases", content)


# ============================================================================
# Test Category Detection
# ============================================================================

class TestCategoryDetection(unittest.TestCase):
    """Test determining test category from header location"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

        # Create fake engine structure
        self.engine_include = Path(self.temp_dir) / "engine" / "include" / "ptx"
        self.engine_include.mkdir(parents=True, exist_ok=True)

    def test_determine_category_core_math(self):
        """Test category for core/math header"""
        header = self.engine_include / "core" / "math" / "vector3d.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("class Vector3D {};")

        # Temporarily override ENGINE_INCLUDE
        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            category = determine_category(header)
            self.assertEqual(category, "core/math")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include

    def test_determine_category_systems_render(self):
        """Test category for systems/render header"""
        header = self.engine_include / "systems" / "render" / "shader" / "shader.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("class Shader {};")

        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            category = determine_category(header)
            self.assertEqual(category, "systems/render/shader")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include

    def test_determine_category_root(self):
        """Test category for root-level header"""
        header = self.engine_include / "engine.hpp"
        header.write_text("class Engine {};")

        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            category = determine_category(header)
            self.assertEqual(category, "")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include


# ============================================================================
# Test Header File Discovery
# ============================================================================

class TestHeaderFileDiscovery(unittest.TestCase):
    """Test finding header files by class name"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

        # Create fake engine structure
        self.engine_include = Path(self.temp_dir) / "engine" / "include" / "ptx"
        self.engine_include.mkdir(parents=True, exist_ok=True)

    def test_find_header_file_simple_match(self):
        """Test finding header by simple filename match"""
        header = self.engine_include / "core" / "vector3d.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("class Vector3D {};")

        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            found = find_header_file("Vector3D", "Vector3D")
            self.assertIsNotNone(found)
            self.assertEqual(found.name, "vector3d.hpp")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include

    def test_find_header_file_2d_suffix(self):
        """Test finding header with 2D suffix stripped"""
        # Class is Circle2D but file is circle.hpp
        header = self.engine_include / "geometry" / "circle.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("class Circle2D : public Shape {};")

        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            found = find_header_file("Circle2D", "Circle2D")
            self.assertIsNotNone(found)
            self.assertEqual(found.name, "circle.hpp")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include

    def test_find_header_file_3d_suffix(self):
        """Test finding header with 3D suffix stripped"""
        header = self.engine_include / "geometry" / "sphere.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("class Sphere3D {};")

        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            found = find_header_file("Sphere3D", "Sphere3D")
            self.assertIsNotNone(found)
            self.assertEqual(found.name, "sphere.hpp")
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include

    def test_find_header_file_not_found(self):
        """Test that missing header returns None"""
        import generatetestskeletons
        original_include = generatetestskeletons.ENGINE_INCLUDE
        generatetestskeletons.ENGINE_INCLUDE = self.engine_include

        try:
            found = find_header_file("NonExistentClass", "NonExistentClass")
            self.assertIsNone(found)
        finally:
            generatetestskeletons.ENGINE_INCLUDE = original_include


# ============================================================================
# Test Reflected Class Parsing
# ============================================================================

class TestReflectedClassParsing(unittest.TestCase):
    """Test parsing reflection_entry_gen.cpp"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

    def test_parse_reflected_classes(self):
        """Test parsing reflected classes from reflection file"""
        reflection_file = Path(self.temp_dir) / "reflection_entry_gen.cpp"
        reflection_file.write_text("""
void register_all_classes() {
    (void)Vector3D::Describe();
    (void)Circle2D::Describe();
    (void)namespace::QualifiedClass::Describe();
}
""")

        import generatetestskeletons
        original_reflection = generatetestskeletons.REFLECTION_GEN
        generatetestskeletons.REFLECTION_GEN = reflection_file

        try:
            from generatetestskeletons import parse_reflected_classes
            classes = parse_reflected_classes()

            self.assertIn("Vector3D", classes)
            self.assertEqual(classes["Vector3D"], "Vector3D")

            self.assertIn("Circle2D", classes)
            self.assertEqual(classes["Circle2D"], "Circle2D")

            self.assertIn("QualifiedClass", classes)
            self.assertEqual(classes["QualifiedClass"], "namespace::QualifiedClass")
        finally:
            generatetestskeletons.REFLECTION_GEN = original_reflection


# ============================================================================
# Integration Tests
# ============================================================================

class TestEndToEndGeneration(unittest.TestCase):
    """End-to-end test file generation"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

        # Create fake project structure
        self.engine_include = Path(self.temp_dir) / "engine" / "include" / "ptx"
        self.engine_include.mkdir(parents=True, exist_ok=True)

        self.tests_engine = Path(self.temp_dir) / "tests" / "engine"
        self.tests_engine.mkdir(parents=True, exist_ok=True)

    def test_generate_test_files_for_simple_class(self):
        """Test generating test files from scratch"""
        # Create a simple header
        header = self.engine_include / "core" / "simple.hpp"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("""
#pragma once

class SimpleClass {
public:
    SimpleClass();
    int GetValue() const;
    void SetValue(int v);

    PTX_BEGIN_METHODS(SimpleClass)
        PTX_METHOD_AUTO(SimpleClass, GetValue, "Get value")
        PTX_METHOD_AUTO(SimpleClass, SetValue, "Set value")
    PTX_END_METHODS
};
""")

        # Create ClassInfo
        methods = [
            MethodInfo(name="GetValue", return_type="int", is_static=False),
            MethodInfo(name="SetValue", return_type="void", is_static=False)
        ]

        class_info = ClassInfo(
            name="SimpleClass",
            qualified_name="SimpleClass",
            header_path=header,
            methods=methods,
            has_default_constructor=True,
            category="core"
        )

        # Generate test files
        from generatetestskeletons import create_new_test_files

        import generatetestskeletons
        original_tests = generatetestskeletons.TESTS_ENGINE
        generatetestskeletons.TESTS_ENGINE = self.tests_engine

        try:
            test_methods = compute_expected_test_methods(class_info)
            header_path, impl_path = create_new_test_files(
                class_info, test_methods, dry_run=False
            )

            # Verify files were created
            self.assertTrue(header_path.exists())
            self.assertTrue(impl_path.exists())

            # Verify header content
            header_content = header_path.read_text()
            self.assertIn("class TestSimpleClass", header_content)
            self.assertIn("static void TestGetValue();", header_content)
            self.assertIn("static void TestSetValue();", header_content)
            self.assertIn("static void RunAllTests();", header_content)

            # Verify impl content
            impl_content = impl_path.read_text()
            self.assertIn("void TestSimpleClass::TestGetValue()", impl_content)
            self.assertIn("void TestSimpleClass::TestSetValue()", impl_content)
            self.assertIn("void TestSimpleClass::RunAllTests()", impl_content)
            self.assertIn("RUN_TEST(TestGetValue);", impl_content)
            self.assertIn("TEST_ASSERT_TRUE(false)", impl_content)
        finally:
            generatetestskeletons.TESTS_ENGINE = original_tests


if __name__ == "__main__":
    unittest.main()
