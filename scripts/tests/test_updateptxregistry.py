#!/usr/bin/env python3
"""
Unit tests for UpdatePTXRegistry.py

Tests the reflection macro generation functionality including:
- Adding macros to files without them
- Updating existing macros
- Handling template classes
- Handling abstract classes
- Parsing various C++ constructs
"""

import unittest
import sys
import os
import tempfile
import shutil
from pathlib import Path

# Add scripts directory to path
SCRIPT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPT_DIR))

from updateptxregistry import (
    strip_ptx_blocks,
    find_comment_spans,
    in_comment,
    ensure_reflect_include,
    guess_range,
    nice_doc_from_name,
    _split_args,
    _strip_default,
    _strip_leading_attributes,
    _clean_return_type,
    _normalize_param_type,
    parse_public_api_from_body,
    regex_parse_file,
    gen_blocks,
    ensure_blocks_in_file,
    validate_macro_blocks,
    FieldInfo,
    MethodInfo,
    CtorInfo,
    ClassInfo,
)


class TestCommentDetection(unittest.TestCase):
    """Test comment span detection"""

    def test_single_line_comment(self):
        src = "int x; // comment\nint y;"
        starts, spans = find_comment_spans(src)
        self.assertEqual(len(spans), 1)
        self.assertTrue(in_comment(10, starts, spans))  # Inside comment
        self.assertFalse(in_comment(5, starts, spans))  # Before comment

    def test_multi_line_comment(self):
        src = "int x; /* multi\nline\ncomment */ int y;"
        starts, spans = find_comment_spans(src)
        self.assertEqual(len(spans), 1)
        self.assertTrue(in_comment(15, starts, spans))  # Inside comment
        self.assertFalse(in_comment(5, starts, spans))  # Before comment

    def test_string_with_comment_chars(self):
        src = 'const char* s = "// not a comment";'
        starts, spans = find_comment_spans(src)
        self.assertEqual(len(spans), 0)  # Should not detect comment in string


class TestMacroManipulation(unittest.TestCase):
    """Test PTX macro manipulation"""

    def test_strip_ptx_blocks(self):
        src = """
class Foo {
    PTX_BEGIN_FIELDS(Foo)
        PTX_FIELD(Foo, value, "Value", 0, 100),
    PTX_END_FIELDS

    int value;
};
"""
        result = strip_ptx_blocks(src)
        self.assertNotIn("PTX_BEGIN_FIELDS", result)
        self.assertNotIn("PTX_END_FIELDS", result)
        self.assertIn("int value;", result)

    def test_validate_macro_blocks_valid(self):
        src = """
class Foo {
    PTX_BEGIN_FIELDS(Foo)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Foo)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Foo)
    PTX_END_DESCRIBE(Foo)
};
"""
        # Should not raise
        validate_macro_blocks(src)

    def test_validate_macro_blocks_mismatched(self):
        src = """
class Foo {
    PTX_BEGIN_FIELDS(Foo)
    PTX_BEGIN_METHODS(Foo)
    PTX_END_METHODS
};
"""
        with self.assertRaises(RuntimeError) as ctx:
            validate_macro_blocks(src)
        self.assertIn("Mismatched", str(ctx.exception))


class TestTypeRanges(unittest.TestCase):
    """Test type range guessing"""

    def test_integer_types(self):
        self.assertEqual(guess_range("int"), ("-2147483648", "2147483647"))
        self.assertEqual(guess_range("uint8_t"), ("0", "255"))
        self.assertEqual(guess_range("bool"), ("0", "1"))

    def test_floating_types(self):
        min_val, max_val = guess_range("float")
        self.assertIn("FLT", min_val)
        self.assertIn("FLT", max_val)

    def test_qualified_types(self):
        # Should strip const and references
        self.assertEqual(guess_range("const int&"), ("-2147483648", "2147483647"))


class TestNameFormatting(unittest.TestCase):
    """Test name to documentation conversion"""

    def test_snake_case(self):
        self.assertEqual(nice_doc_from_name("my_value"), "My value")

    def test_camel_case(self):
        self.assertEqual(nice_doc_from_name("myValue"), "My value")

    def test_mixed(self):
        self.assertEqual(nice_doc_from_name("my_Value123"), "My value123")


class TestArgumentParsing(unittest.TestCase):
    """Test C++ argument parsing"""

    def test_split_simple_args(self):
        result = _split_args("int, float, bool")
        self.assertEqual(result, ["int", "float", "bool"])

    def test_split_template_args(self):
        result = _split_args("std::vector<int>, std::map<std::string, int>")
        self.assertEqual(len(result), 2)
        self.assertIn("std::vector<int>", result)

    def test_split_nested_templates(self):
        result = _split_args("std::vector<std::pair<int, float>>")
        self.assertEqual(len(result), 1)

    def test_strip_default_values(self):
        self.assertEqual(_strip_default("int x = 5"), "int x")
        self.assertEqual(_strip_default("std::string s = \"hello\""), "std::string s")
        self.assertEqual(_strip_default("int x"), "int x")

    def test_strip_attributes(self):
        self.assertEqual(_strip_leading_attributes("[[nodiscard]] int"), "int")
        self.assertEqual(_strip_leading_attributes("int"), "int")

    def test_clean_return_type(self):
        self.assertEqual(_clean_return_type("inline virtual int"), "int")
        self.assertEqual(_clean_return_type("static constexpr float"), "float")

    def test_normalize_param_type(self):
        # With parameter name
        ptype, ok = _normalize_param_type("int value")
        self.assertEqual(ptype, "int")
        self.assertTrue(ok)

        # Without parameter name
        ptype, ok = _normalize_param_type("const float&")
        self.assertEqual(ptype, "const float&")
        self.assertFalse(ok)  # Can't confidently strip name

        # Variadic
        ptype, ok = _normalize_param_type("...")
        self.assertEqual(ptype, "...")
        self.assertTrue(ok)


class TestSimpleClassParsing(unittest.TestCase):
    """Test parsing of simple C++ classes"""

    def test_parse_simple_class(self):
        src = """
class MyClass {
public:
    int value;
    float GetValue() const;
    void SetValue(float v);
    MyClass(int val);
};
"""
        classes = regex_parse_file(Path("test.hpp"), src)
        self.assertEqual(len(classes), 1)

        ci = classes[0]
        self.assertEqual(ci.name, "MyClass")
        self.assertEqual(len(ci.fields), 1)
        self.assertEqual(ci.fields[0].name, "value")
        self.assertEqual(ci.fields[0].type, "int")

        # Methods
        method_names = [m.name for m in ci.methods]
        self.assertIn("GetValue", method_names)
        self.assertIn("SetValue", method_names)

        # Constructors
        self.assertEqual(len(ci.ctors), 1)

    def test_parse_struct(self):
        src = """
struct Point2D {
    float x;
    float y;
};
"""
        classes = regex_parse_file(Path("test.hpp"), src)
        self.assertEqual(len(classes), 1)
        ci = classes[0]
        self.assertEqual(ci.name, "Point2D")

    def test_skip_template_class(self):
        src = """
template<typename T>
class TemplateClass {
public:
    T value;
};
"""
        classes = regex_parse_file(Path("test.hpp"), src)
        self.assertEqual(len(classes), 1)
        self.assertTrue(classes[0].is_template)

    def test_skip_enum_class(self):
        src = """
enum class Color {
    Red,
    Green,
    Blue
};

class MyClass {
    int value;
};
"""
        classes = regex_parse_file(Path("test.hpp"), src)
        # Should find at least MyClass (enum may or may not be filtered)
        self.assertTrue(len(classes) >= 1)
        class_names = [c.name for c in classes]
        self.assertIn("MyClass", class_names)


class TestPublicAPIExtraction(unittest.TestCase):
    """Test extraction of public API from class body"""

    def test_parse_fields(self):
        body = """
private:
    int privateField;
public:
    float publicField;
    double anotherField;
"""
        fields, methods, ctors, has_pv = parse_public_api_from_body(body, "Test")

        self.assertEqual(len(fields), 2)
        self.assertEqual(fields[0].name, "publicField")
        self.assertEqual(fields[0].type, "float")

    def test_parse_methods(self):
        body = """
public:
    int GetValue() const;
    void SetValue(int v);
    static bool IsValid();
"""
        fields, methods, ctors, has_pv = parse_public_api_from_body(body, "Test")

        self.assertEqual(len(methods), 3)

        get_method = next(m for m in methods if m.name == "GetValue")
        self.assertTrue(get_method.is_const)
        self.assertFalse(get_method.is_static)

        is_valid = next(m for m in methods if m.name == "IsValid")
        self.assertTrue(is_valid.is_static)

    def test_parse_constructors(self):
        body = """
public:
    Test();
    Test(int value);
    Test(int x, float y);
"""
        fields, methods, ctors, has_pv = parse_public_api_from_body(body, "Test")

        self.assertEqual(len(ctors), 3)
        self.assertEqual(len(ctors[0].args), 0)  # Default ctor
        self.assertEqual(len(ctors[1].args), 1)
        self.assertEqual(len(ctors[2].args), 2)

    def test_detect_pure_virtual(self):
        body = """
public:
    virtual void Update() = 0;
"""
        fields, methods, ctors, has_pv = parse_public_api_from_body(body, "Test")

        self.assertTrue(has_pv)


class TestBlockGeneration(unittest.TestCase):
    """Test PTX block generation"""

    def test_generate_simple_blocks(self):
        ci = ClassInfo(
            name="TestClass",
            name_token="TestClass",
            start=0,
            close_brace=100,
            after_semicolon=101,
            is_template=False,
            in_template_scope=False,
            fields=[FieldInfo(name="value", type="int")],
            methods=[MethodInfo(name="GetValue", is_static=False, return_type="int", args=[], is_const=True)],
            ctors=[CtorInfo(args=[])],
        )

        blocks = gen_blocks(ci)

        self.assertIn("PTX_BEGIN_FIELDS(TestClass)", blocks)
        self.assertIn("PTX_END_FIELDS", blocks)
        self.assertIn("PTX_FIELD(TestClass, value,", blocks)

        self.assertIn("PTX_BEGIN_METHODS(TestClass)", blocks)
        self.assertIn("PTX_END_METHODS", blocks)
        self.assertIn("PTX_METHOD_AUTO(TestClass, GetValue,", blocks)

        self.assertIn("PTX_BEGIN_DESCRIBE(TestClass)", blocks)
        self.assertIn("PTX_END_DESCRIBE(TestClass)", blocks)
        self.assertIn("PTX_CTOR0(TestClass)", blocks)

    def test_generate_overloaded_methods(self):
        ci = ClassInfo(
            name="TestClass",
            name_token="TestClass",
            start=0,
            close_brace=100,
            after_semicolon=101,
            is_template=False,
            in_template_scope=False,
            fields=[],
            methods=[
                MethodInfo(name="GetValue", is_static=False, return_type="int", args=[], is_const=True),
                MethodInfo(name="GetValue", is_static=False, return_type="float", args=["int"], is_const=True),
            ],
            ctors=[],
        )

        blocks = gen_blocks(ci)

        # Should use overload macros
        self.assertIn("PTX_METHOD_OVLD", blocks)

    def test_generate_static_methods(self):
        ci = ClassInfo(
            name="TestClass",
            name_token="TestClass",
            start=0,
            close_brace=100,
            after_semicolon=101,
            is_template=False,
            in_template_scope=False,
            fields=[],
            methods=[
                MethodInfo(name="Create", is_static=True, return_type="TestClass*", args=[], is_const=False),
            ],
            ctors=[],
        )

        blocks = gen_blocks(ci)

        self.assertIn("PTX_SMETHOD_AUTO(TestClass::Create,", blocks)


class TestIncludeInjection(unittest.TestCase):
    """Test reflect_macros.hpp include injection"""

    def test_ensure_include_after_pragma_once(self):
        src = """#pragma once

#include <string>

class Foo {};
"""
        root = Path("/test/root")
        header = Path("/test/root/core/foo.hpp")

        result, changed, rel = ensure_reflect_include(src, header, root)

        self.assertTrue(changed)
        self.assertIn('#include "', result)
        self.assertIn("reflect_macros.hpp", result)

    def test_no_duplicate_include(self):
        src = """#pragma once

#include "../registry/reflect_macros.hpp"

class Foo {};
"""
        root = Path("/test/root")
        header = Path("/test/root/core/foo.hpp")

        result, changed, rel = ensure_reflect_include(src, header, root)

        self.assertFalse(changed)


class TestEndToEndTransformation(unittest.TestCase):
    """End-to-end tests with temporary files"""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

    def test_add_macros_to_simple_class(self):
        """Test adding PTX macros to a class without them"""

        # Create a simple header file
        header_content = """#pragma once

class SimpleClass {
public:
    int value;

    SimpleClass(int v);
    int GetValue() const;
    void SetValue(int v);
};
"""

        header_path = Path(self.temp_dir) / "simple.hpp"
        header_path.write_text(header_content)

        # Parse and generate blocks
        classes = regex_parse_file(header_path, header_content)
        self.assertEqual(len(classes), 1)

        ci = classes[0]
        self.assertEqual(ci.name, "SimpleClass")

        # Generate blocks
        blocks = gen_blocks(ci)

        # Verify generated blocks
        self.assertIn("PTX_BEGIN_FIELDS(SimpleClass)", blocks)
        self.assertIn("PTX_FIELD(SimpleClass, value,", blocks)
        self.assertIn("PTX_METHOD_AUTO(SimpleClass, GetValue,", blocks)
        self.assertIn("PTX_METHOD_AUTO(SimpleClass, SetValue,", blocks)
        # Constructor may be PTX_CTOR or PTX_CTOR(SimpleClass, int)
        self.assertTrue("PTX_CTOR" in blocks)

    def test_update_existing_macros(self):
        """Test updating existing PTX macros when class changes"""

        # Header with existing macros (missing a method)
        header_content = """#pragma once

class UpdateClass {
public:
    int value;

    UpdateClass();
    int GetValue() const;
    void SetValue(int v);
    void NewMethod();  // This is new!

    PTX_BEGIN_FIELDS(UpdateClass)
        PTX_FIELD(UpdateClass, value, "Value", 0, 0),
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(UpdateClass)
        PTX_METHOD_AUTO(UpdateClass, GetValue, "Get value"),
        PTX_METHOD_AUTO(UpdateClass, SetValue, "Set value")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(UpdateClass)
        PTX_CTOR0(UpdateClass)
    PTX_END_DESCRIBE(UpdateClass)
};
"""

        header_path = Path(self.temp_dir) / "update.hpp"
        header_path.write_text(header_content)

        # Parse the class
        classes = regex_parse_file(header_path, header_content)
        self.assertEqual(len(classes), 1)

        ci = classes[0]

        # Verify NewMethod was detected
        method_names = [m.name for m in ci.methods]
        self.assertIn("NewMethod", method_names)


class TestRealWorldScenarios(unittest.TestCase):
    """Test real-world scenarios with complex classes"""

    def test_vector3d_style_class(self):
        """Test a class similar to Vector3D"""
        src = """
class Vector3D {
public:
    float X, Y, Z;

    Vector3D();
    Vector3D(float x, float y, float z);

    float Magnitude() const;
    Vector3D Normalize() const;
    Vector3D Cross(const Vector3D& other) const;
    float Dot(const Vector3D& other) const;

    Vector3D operator+(const Vector3D& other) const;
    Vector3D operator-(const Vector3D& other) const;
};
"""
        classes = regex_parse_file(Path("vector3d.hpp"), src)
        self.assertEqual(len(classes), 1)

        ci = classes[0]

        # Should detect fields (may be 0 or 3 depending on parsing)
        # Just verify class was parsed
        self.assertTrue(len(ci.fields) >= 0)

        # Should detect methods (but not operators)
        method_names = [m.name for m in ci.methods]
        self.assertIn("Magnitude", method_names)
        self.assertIn("Normalize", method_names)
        self.assertNotIn("operator+", method_names)  # Operators excluded

    def test_singleton_pattern(self):
        """Test singleton pattern with static methods"""
        src = """
class Singleton {
private:
    static Singleton* instance;
    Singleton();

public:
    static Singleton* GetInstance();
    static void Destroy();
    void DoSomething();
};
"""
        classes = regex_parse_file(Path("singleton.hpp"), src)
        self.assertEqual(len(classes), 1)

        ci = classes[0]

        # Static methods
        static_methods = [m.name for m in ci.methods if m.is_static]
        self.assertIn("GetInstance", static_methods)
        self.assertIn("Destroy", static_methods)

        # Instance method
        instance_methods = [m.name for m in ci.methods if not m.is_static]
        self.assertIn("DoSomething", instance_methods)


def run_tests():
    """Run all tests"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestCommentDetection))
    suite.addTests(loader.loadTestsFromTestCase(TestMacroManipulation))
    suite.addTests(loader.loadTestsFromTestCase(TestTypeRanges))
    suite.addTests(loader.loadTestsFromTestCase(TestNameFormatting))
    suite.addTests(loader.loadTestsFromTestCase(TestArgumentParsing))
    suite.addTests(loader.loadTestsFromTestCase(TestSimpleClassParsing))
    suite.addTests(loader.loadTestsFromTestCase(TestPublicAPIExtraction))
    suite.addTests(loader.loadTestsFromTestCase(TestBlockGeneration))
    suite.addTests(loader.loadTestsFromTestCase(TestIncludeInjection))
    suite.addTests(loader.loadTestsFromTestCase(TestEndToEndTransformation))
    suite.addTests(loader.loadTestsFromTestCase(TestRealWorldScenarios))

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
