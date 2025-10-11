#!/usr/bin/env python3
"""
Generate and validate test skeleton files for PTX Engine classes.

This script:
1. Only generates test files for classes that don't have tests yet
2. Validates existing test files have methods matching the source class
3. Adds missing test methods to existing test files
4. Removes test methods that no longer exist in the source class
5. Generates TEST_ASSERT_TRUE(false) for unimplemented tests as indicators

Usage:
    python GenerateTestSkeletons.py [--class ClassName] [--all] [--dry-run] [--cache-file PATH]

Examples:
    # Validate and update all tests
    python GenerateTestSkeletons.py --all

    # Check specific class
    python GenerateTestSkeletons.py --class Vector3D
    
    # Preview changes without writing
    python GenerateTestSkeletons.py --all --dry-run
"""

import argparse
import json
import hashlib
import re
import sys
from pathlib import Path
from typing import List, Dict, Set, Optional, Tuple
from dataclasses import dataclass
from datetime import datetime

# Add scripts directory to path for console_output module
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from consoleoutput import print_progress, print_status, print_section, print_warning, print_error, print_success, Colors

# Project paths
REPO_ROOT = Path(__file__).resolve().parents[1]
ENGINE_INCLUDE = REPO_ROOT / "engine" / "include" / "ptx"
TESTS_ENGINE = REPO_ROOT / "tests" / "engine"
REFLECTION_GEN = REPO_ROOT / "build" / "generated" / "reflection_entry_gen.cpp"


@dataclass
class MethodInfo:
    """Information about a class method."""
    name: str
    return_type: str
    is_const: bool = False
    is_static: bool = False


@dataclass
class ClassInfo:
    """Information about a source class."""
    name: str
    qualified_name: str
    header_path: Path
    methods: List[MethodInfo]
    has_default_constructor: bool
    category: str


@dataclass
class TestInfo:
    """Information about a test file."""
    test_class_name: str
    source_class_name: str
    header_path: Path
    impl_path: Path
    existing_methods: List[str]
    category: str


# Console output is now handled by console_output module
# Matches CMake build style: [XX%] message in green


# ============================================================================
# Cache Management
# ============================================================================

def compute_file_hash(file_path: Path) -> str:
    """Compute SHA256 hash of a file."""
    if not file_path.exists():
        return ""
    return hashlib.sha256(file_path.read_bytes()).hexdigest()


def load_cache(cache_file: Optional[Path]) -> Dict:
    """Load cache from JSON file."""
    if not cache_file or not cache_file.exists():
        return {
            "reflection_hash": "",
            "validated_tests": {},
            "version": "2.0"
        }
    
    try:
        return json.loads(cache_file.read_text())
    except (json.JSONDecodeError, IOError):
        return {
            "reflection_hash": "",
            "validated_tests": {},
            "version": "2.0"
        }


def save_cache(cache_file: Optional[Path], cache_data: Dict):
    """Save cache to JSON file."""
    if not cache_file:
        return
    
    cache_file.parent.mkdir(parents=True, exist_ok=True)
    cache_file.write_text(json.dumps(cache_data, indent=2))


def is_cache_valid(cache_file: Optional[Path], reflection_file: Path) -> bool:
    """Check if cache is valid based on reflection file hash."""
    if not cache_file or not cache_file.exists():
        return False
    
    cache = load_cache(cache_file)
    current_hash = compute_file_hash(reflection_file)
    
    return cache.get("reflection_hash") == current_hash and current_hash != ""


# ============================================================================
# Source Class Parsing
# ============================================================================

def parse_reflected_classes() -> Dict[str, str]:
    """
    Parse reflection_entry_gen.cpp to extract all reflected class names.
    
    Returns:
        Dict mapping class name to qualified name
    """
    if not REFLECTION_GEN.exists():
        raise FileNotFoundError(
            f"[error] reflection file not found: {REFLECTION_GEN}\n"
            "Run 'cmake . && make' in build/ directory first."
        )
    
    content = REFLECTION_GEN.read_text()
    pattern = re.compile(r'\(void\)([A-Za-z_:][A-Za-z0-9_:]*?)::Describe\(\);')
    
    classes = {}
    for match in pattern.finditer(content):
        qualified_name = match.group(1)
        simple_name = qualified_name.split('::')[-1]
        classes[simple_name] = qualified_name
    
    return classes


def find_header_file(class_name: str, qualified_name: str = None) -> Optional[Path]:
    """Find the header file for a given class name.

    Args:
        class_name: Simple class name (e.g., "Triangle2D")
        qualified_name: Fully qualified name (e.g., "ptx::Triangle2D")

    Returns:
        Path to header file containing the class, or None if not found
    """
    # First try filename matching (fast path for simple cases)
    filename = class_name.lower() + ".hpp"

    for header in ENGINE_INCLUDE.rglob("*.hpp"):
        if header.name.lower() == filename:
            return header

    # If qualified name provided, try extracting common patterns
    # For example: Triangle2D might be in triangle.hpp or geometry.hpp
    if qualified_name:
        # Try stripping numeric suffixes: Triangle2D -> triangle.hpp
        base_name = re.sub(r'\d+D$', '', class_name)
        if base_name != class_name:
            filename = base_name.lower() + ".hpp"
            for header in ENGINE_INCLUDE.rglob("*.hpp"):
                if header.name.lower() == filename:
                    # Verify the class is actually in this file
                    try:
                        content = header.read_text(encoding='utf-8')
                        class_pattern = re.compile(
                            rf'^\s*(class|struct)\s+{re.escape(class_name)}\b',
                            re.MULTILINE
                        )
                        if class_pattern.search(content):
                            return header
                    except Exception:
                        continue

    # Last resort: search for class declaration within files
    # This is slower but handles edge cases
    class_decl_pattern = re.compile(
        rf'^\s*(class|struct)\s+{re.escape(class_name)}\b',
        re.MULTILINE
    )

    for header in ENGINE_INCLUDE.rglob("*.hpp"):
        try:
            content = header.read_text(encoding='utf-8')
            if class_decl_pattern.search(content):
                return header
        except Exception:
            continue

    return None


def determine_category(header_path: Path) -> str:
    """Determine the test category based on header file location."""
    try:
        rel_path = header_path.relative_to(ENGINE_INCLUDE)
        category = str(rel_path.parent).replace('\\', '/')
        return category if category != '.' else ''
    except ValueError:
        return ''


def parse_class_methods(header_path: Path, class_name: str) -> Tuple[List[MethodInfo], bool]:
    """
    Parse header file to extract public method names from PTX_METHODS block.
    
    Returns:
        Tuple of (methods list, has_default_constructor)
    """
    content = header_path.read_text()
    
    # Check for default constructor
    has_default_ctor = bool(re.search(rf'\b{class_name}\s*\(\s*\)', content))
    
    # Find PTX_BEGIN_METHODS block
    methods_pattern = re.compile(
        rf'PTX_BEGIN_METHODS\({class_name}\)(.*?)PTX_END_METHODS',
        re.DOTALL
    )
    match = methods_pattern.search(content)
    
    if not match:
        return [], has_default_ctor
    
    methods_block = match.group(1)
    methods = []
    
    # Extract method names from various PTX_METHOD macros
    # PTX_METHOD_AUTO, PTX_SMETHOD_AUTO, PTX_METHOD_OVLD, etc.
    method_pattern = re.compile(
        r'PTX_(?:S)?METHOD_(?:AUTO|OVLD|MANUAL)\s*\(\s*' +
        re.escape(class_name) + r'\s*,\s*([A-Za-z_][A-Za-z0-9_]*)'
    )
    
    seen = set()
    for m in method_pattern.finditer(methods_block):
        method_name = m.group(1)
        if method_name not in seen:
            seen.add(method_name)
            # Determine if static (PTX_SMETHOD)
            is_static = 'SMETHOD' in m.group(0)
            methods.append(MethodInfo(
                name=method_name,
                return_type="auto",  # Not parsed for now
                is_static=is_static
            ))
    
    return methods, has_default_ctor


def get_class_info(class_name: str, qualified_name: str) -> Optional[ClassInfo]:
    """Get complete class information."""
    header_path = find_header_file(class_name, qualified_name)
    if not header_path:
        return None

    methods, has_default_ctor = parse_class_methods(header_path, class_name)
    category = determine_category(header_path)

    return ClassInfo(
        name=class_name,
        qualified_name=qualified_name,
        header_path=header_path,
        methods=methods,
        has_default_constructor=has_default_ctor,
        category=category
    )


# ============================================================================
# Test File Parsing
# ============================================================================

def find_existing_test_files(class_name: str) -> Optional[TestInfo]:
    """Find existing test files for a class."""
    filename_lower = class_name.lower()
    test_pattern = f"test{filename_lower}.hpp"
    
    for test_header in TESTS_ENGINE.rglob(test_pattern):
        test_impl = test_header.with_suffix('.cpp')
        
        # Parse test class name and existing methods
        try:
            content = test_header.read_text()
            test_class_match = re.search(r'class\s+(Test[A-Za-z0-9_]+)', content)
            if not test_class_match:
                continue
            
            test_class_name = test_class_match.group(1)
            
            # Find all test method declarations
            method_pattern = re.compile(r'static\s+void\s+(Test[A-Za-z0-9_]+)\s*\(\s*\);')
            existing_methods = []
            for m in method_pattern.finditer(content):
                method_name = m.group(1)
                if method_name not in ['RunAllTests']:
                    existing_methods.append(method_name)
            
            category = str(test_header.parent.relative_to(TESTS_ENGINE)).replace('\\', '/')
            
            return TestInfo(
                test_class_name=test_class_name,
                source_class_name=class_name,
                header_path=test_header,
                impl_path=test_impl,
                existing_methods=existing_methods,
                category=category if category != '.' else ''
            )
        except Exception as e:
            print_warning(f"Error parsing {test_header}: {e}")
            continue
    
    return None


# ============================================================================
# Test Generation/Validation
# ============================================================================

def generate_test_method_name(method_name: str) -> str:
    """Generate test method name from source method name."""
    return f"Test{method_name}"


def compute_expected_test_methods(class_info: ClassInfo) -> List[str]:
    """Compute what test methods should exist for a class."""
    expected = [
        "TestDefaultConstructor",
        "TestParameterizedConstructor"
    ]
    
    # Add test for each method
    for method in class_info.methods:
        test_name = generate_test_method_name(method.name)
        if test_name not in expected:
            expected.append(test_name)
    
    # Always include edge cases
    expected.append("TestEdgeCases")
    
    return expected


def generate_test_header_content(class_info: ClassInfo, test_methods: List[str]) -> str:
    """Generate complete test header file content."""
    test_class_name = f"Test{class_info.name}"
    filename_lower = class_info.name.lower()
    
    # Get header path relative to engine/include/ptx
    try:
        header_rel = class_info.header_path.relative_to(ENGINE_INCLUDE)
    except ValueError:
        header_rel = class_info.header_path.name
    
    # Group methods
    constructor_tests = [m for m in test_methods if 'Constructor' in m]
    method_tests = [m for m in test_methods if 'Constructor' not in m and m != 'TestEdgeCases']
    
    # Build method declarations
    decls = []
    
    if constructor_tests:
        decls.append("    // Constructor & lifecycle tests")
        for method in constructor_tests:
            decls.append(f"    static void {method}();")
        decls.append("")
    
    if method_tests:
        decls.append("    // Method tests")
        for method in method_tests:
            decls.append(f"    static void {method}();")
        decls.append("")
    
    decls.append("    // Edge case & integration tests")
    decls.append("    static void TestEdgeCases();")
    
    today = datetime.now().strftime("%d/%m/%Y")
    
    return f'''/**
 * @file test{filename_lower}.hpp
 * @brief Unit tests for the {class_info.name} class.
 *
 * @date {today}
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/{header_rel}>
#include <utils/testhelpers.hpp>

/**
 * @class {test_class_name}
 * @brief Contains static test methods for the {class_info.name} class.
 */
class {test_class_name} {{
public:
{chr(10).join(decls)}

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
}};
'''


def generate_test_method_impl(test_class_name: str, method_name: str, 
                              class_name: str, is_constructor: bool = False) -> str:
    """Generate a single test method implementation with TEST_ASSERT_TRUE(false)."""
    if method_name == "TestDefaultConstructor":
        return f'''void {test_class_name}::TestDefaultConstructor() {{
    // TODO: Implement test for default constructor
    {class_name} obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}}'''
    elif method_name == "TestParameterizedConstructor":
        return f'''void {test_class_name}::TestParameterizedConstructor() {{
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}}'''
    elif method_name == "TestEdgeCases":
        return f'''void {test_class_name}::TestEdgeCases() {{
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}}'''
    else:
        # Regular method test
        source_method = method_name.replace('Test', '', 1)
        return f'''void {test_class_name}::{method_name}() {{
    // TODO: Implement test for {source_method}()
    {class_name} obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}}'''


def generate_test_impl_content(class_info: ClassInfo, test_methods: List[str]) -> str:
    """Generate complete test implementation file content."""
    test_class_name = f"Test{class_info.name}"
    filename_lower = class_info.name.lower()
    
    # Generate method implementations
    impls = []
    
    # Constructors
    constructor_tests = [m for m in test_methods if 'Constructor' in m]
    if constructor_tests:
        impls.append("// ========== Constructor Tests ==========\n")
        for method in constructor_tests:
            impls.append(generate_test_method_impl(
                test_class_name, method, class_info.name, is_constructor=True
            ))
            impls.append("")
    
    # Methods
    method_tests = [m for m in test_methods if 'Constructor' not in m and m != 'TestEdgeCases']
    if method_tests:
        impls.append("// ========== Method Tests ==========\n")
        for method in method_tests:
            impls.append(generate_test_method_impl(
                test_class_name, method, class_info.name
            ))
            impls.append("")
    
    # Edge cases
    impls.append("// ========== Edge Cases ==========\n")
    impls.append(generate_test_method_impl(
        test_class_name, "TestEdgeCases", class_info.name
    ))
    impls.append("")
    
    # RunAllTests
    impls.append("// ========== Test Runner ==========\n")
    run_calls = [f"    RUN_TEST({m});" for m in test_methods]
    impls.append(f'''void {test_class_name}::RunAllTests() {{
{chr(10).join(run_calls)}
}}''')
    
    return f'''/**
 * @file test{filename_lower}.cpp
 * @brief Implementation of {class_info.name} unit tests.
 */

#include "test{filename_lower}.hpp"

{chr(10).join(impls)}
'''


def update_existing_test_files(test_info: TestInfo, expected_methods: List[str], 
                               class_info: ClassInfo, dry_run: bool) -> Dict[str, List[str]]:
    """
    Update existing test files to match expected methods.
    
    Returns:
        Dict with 'added' and 'removed' method lists
    """
    changes = {'added': [], 'removed': []}
    
    existing_set = set(test_info.existing_methods)
    expected_set = set(expected_methods)
    
    # Find methods to add/remove
    to_add = expected_set - existing_set
    to_remove = existing_set - expected_set
    
    if not to_add and not to_remove:
        return changes
    
    # Read current files
    header_content = test_info.header_path.read_text()
    impl_content = test_info.impl_path.read_text() if test_info.impl_path.exists() else ""
    
    # Add missing methods to header
    if to_add:
        for method in sorted(to_add):
            # Add declaration before RunAllTests
            decl = f"    static void {method}();"
            header_content = header_content.replace(
                "    static void RunAllTests();",
                f"{decl}\n    static void RunAllTests();"
            )
            changes['added'].append(method)
            
            # Add implementation
            impl = generate_test_method_impl(
                test_info.test_class_name, method, class_info.name
            )
            # Insert before RunAllTests implementation
            impl_content = impl_content.replace(
                f"void {test_info.test_class_name}::RunAllTests()",
                f"{impl}\n\nvoid {test_info.test_class_name}::RunAllTests()"
            )
            
            # Add to RunAllTests call list
            run_test_line = f"    RUN_TEST({method});"
            if f"void {test_info.test_class_name}::RunAllTests()" in impl_content:
                # Find the RunAllTests implementation and add the call
                pattern = rf'(void {test_info.test_class_name}::RunAllTests\(\) {{[^}}]*)(}})'
                impl_content = re.sub(
                    pattern,
                    rf'\1    RUN_TEST({method});\n\2',
                    impl_content
                )
    
    # Remove obsolete methods
    if to_remove:
        for method in to_remove:
            # Remove from header
            header_content = re.sub(
                rf'^\s*static\s+void\s+{method}\s*\(\s*\);\s*$',
                '',
                header_content,
                flags=re.MULTILINE
            )
            
            # Remove from implementation
            impl_content = re.sub(
                rf'void\s+{test_info.test_class_name}::{method}\s*\([^{{]*{{[^}}]*}}[\s\n]*',
                '',
                impl_content,
                flags=re.DOTALL
            )
            
            # Remove from RunAllTests
            impl_content = re.sub(
                rf'^\s*RUN_TEST\({method}\);\s*$',
                '',
                impl_content,
                flags=re.MULTILINE
            )
            
            changes['removed'].append(method)
    
    # Write back if not dry-run
    if not dry_run:
        test_info.header_path.write_text(header_content)
        if test_info.impl_path.exists():
            test_info.impl_path.write_text(impl_content)
    
    return changes


def create_new_test_files(class_info: ClassInfo, test_methods: List[str], 
                         dry_run: bool) -> Tuple[Path, Path]:
    """Create new test files for a class."""
    category = class_info.category
    test_dir = TESTS_ENGINE / category if category else TESTS_ENGINE
    
    filename_lower = class_info.name.lower()
    header_path = test_dir / f"test{filename_lower}.hpp"
    impl_path = test_dir / f"test{filename_lower}.cpp"
    
    if not dry_run:
        test_dir.mkdir(parents=True, exist_ok=True)
        
        header_content = generate_test_header_content(class_info, test_methods)
        impl_content = generate_test_impl_content(class_info, test_methods)
        
        header_path.write_text(header_content)
        impl_path.write_text(impl_content)
    
    return header_path, impl_path


# ============================================================================
# Main Processing
# ============================================================================

def process_class(class_name: str, qualified_name: str, dry_run: bool, 
                 cache_data: Optional[Dict]) -> Dict[str, any]:
    """
    Process a single class - generate or validate its tests.
    
    Returns:
        Dict with processing results
    """
    result = {
        'status': 'skipped',
        'action': None,
        'added': [],
        'removed': [],
        'total_methods': 0
    }
    
    # Get class info
    class_info = get_class_info(class_name, qualified_name)
    if not class_info:
        result['status'] = 'error'
        result['message'] = "Header not found"
        return result
    
    # Compute expected test methods
    expected_methods = compute_expected_test_methods(class_info)
    result['total_methods'] = len(expected_methods)
    
    # Check if test files exist
    test_info = find_existing_test_files(class_name)
    
    if test_info:
        # Validate and update existing tests
        changes = update_existing_test_files(test_info, expected_methods, class_info, dry_run)
        
        if changes['added'] or changes['removed']:
            result['status'] = 'updated'
            result['action'] = 'validated'
            result['added'] = changes['added']
            result['removed'] = changes['removed']
        else:
            result['status'] = 'valid'
            result['action'] = 'validated'
    else:
        # Generate new test files
        header_path, impl_path = create_new_test_files(class_info, expected_methods, dry_run)
        result['status'] = 'generated'
        result['action'] = 'created'
        result['header'] = header_path
        result['impl'] = impl_path
        result['added'] = expected_methods
    
    # Update cache
    if cache_data is not None and not dry_run:
        cache_data["validated_tests"][class_name] = {
            "methods": expected_methods,
            "category": class_info.category,
            "last_validated": datetime.now().isoformat()
        }
    
    return result


def update_test_main_file(dry_run: bool = False) -> bool:
    """Update tests/engine/test.cpp with all test includes and RunAllTests calls."""
    test_main = TESTS_ENGINE / "test.cpp"
    
    if not test_main.exists():
        print_error(f"test.cpp not found at {test_main}")
        return False
    
    # Find all test header files
    test_headers = []
    for header in sorted(TESTS_ENGINE.rglob("test*.hpp")):
        if "utils" not in header.parts:
            rel_path = header.relative_to(TESTS_ENGINE)
            test_headers.append(str(rel_path).replace('\\', '/'))
    
    # Generate includes and calls
    includes = []
    run_calls = []
    
    for header_path in test_headers:
        includes.append(f'#include "{header_path}"')
        
        header_file = TESTS_ENGINE / header_path
        try:
            content = header_file.read_text()
            class_match = re.search(r'class\s+(Test[A-Za-z0-9_]+)\s*\{', content)
            if class_match:
                test_class_name = class_match.group(1)
                run_calls.append(f"    {test_class_name}::RunAllTests();")
        except Exception as e:
            print_warning(f"Warning: Could not parse {header_path}: {e}")
            continue
    
    new_content = f'''#include <unity.h>
{chr(10).join(includes)}

void setUp() {{}}
void tearDown() {{}}

int main(int /*argc*/, char ** /*argv*/) {{
    UNITY_BEGIN();

{chr(10).join(run_calls)}

    UNITY_END();
}}
'''
    
    if not dry_run:
        test_main.write_text(new_content)
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Generate and validate test skeleton files for PTX classes"
    )
    parser.add_argument("--class", dest="class_name", 
                       help="Process specific class")
    parser.add_argument("--all", action="store_true", 
                       help="Process all classes")
    parser.add_argument("--dry-run", action="store_true", 
                       help="Preview without writing files")
    parser.add_argument("--cache-file", type=Path, 
                       help="Cache file path")
    parser.add_argument("--force", action="store_true", 
                       help="Force revalidation")
    parser.add_argument("--update-main", action="store_true", 
                       help="Update test.cpp")
    
    args = parser.parse_args()
    
    if not any([args.class_name, args.all, args.update_main]):
        parser.print_help()
        return
    
    # Handle --update-main
    if args.update_main:
        print_section("Updating test.cpp")
        if update_test_main_file(args.dry_run):
            print_success("test.cpp updated successfully")
        else:
            print_error("Failed to update test.cpp")
        return
    
    # Check cache validity
    cache_data = None
    if args.cache_file:
        if not args.force and is_cache_valid(args.cache_file, REFLECTION_GEN):
            print_success("Cache is valid - test skeletons are up to date")
            print(f"   Cache file: {args.cache_file}")
            print("   Use --force to regenerate anyway")
            return
        cache_data = load_cache(args.cache_file)
        if args.force:
            print_warning("[cache] force regeneration requested")
    
    # Parse reflected classes
    print_section("Discovering reflected classes")
    classes = parse_reflected_classes()
    print(f"   Found {len(classes)} reflected classes")
    
    # Process classes
    stats = {
        'generated': 0,
        'updated': 0,
        'valid': 0,
        'errors': 0,
        'skipped': 0
    }
    
    classes_to_process = {}
    if args.class_name:
        if args.class_name in classes:
            classes_to_process[args.class_name] = classes[args.class_name]
        else:
            print_error(f"Class '{args.class_name}' not found in reflection system")
            return
    elif args.all:
        classes_to_process = classes
    
    print_section(f"Validating {len(classes_to_process)} test classes")
    
    for i, (class_name, qualified_name) in enumerate(sorted(classes_to_process.items()), 1):
        result = process_class(class_name, qualified_name, args.dry_run, cache_data)
        
        stats[result['status']] = stats.get(result['status'], 0) + 1
        
        # Print result in CMake style
        if result['status'] == 'generated':
            msg = f"Generated {result['total_methods']} test methods for {class_name}"
            print_progress(i, len(classes_to_process), msg, Colors.GREEN)
        elif result['status'] == 'updated':
            changes = []
            if result['added']:
                changes.append(f"+{len(result['added'])}")
            if result['removed']:
                changes.append(f"-{len(result['removed'])}")
            msg = f"Updated {class_name} ({', '.join(changes)} methods)"
            print_progress(i, len(classes_to_process), msg, Colors.GREEN)
            if result['added']:
                print(f"        Added: {', '.join(result['added'][:5])}" + 
                      (f"... (+{len(result['added'])-5} more)" if len(result['added']) > 5 else ""))
            if result['removed']:
                print(f"        Removed: {', '.join(result['removed'][:5])}" +
                      (f"... (+{len(result['removed'])-5} more)" if len(result['removed']) > 5 else ""))
        elif result['status'] == 'valid':
            msg = f"Validated {class_name} ({result['total_methods']} methods)"
            print_progress(i, len(classes_to_process), msg, Colors.GREEN)
        elif result['status'] == 'error':
            msg = f"Error processing {class_name}: {result.get('message', 'unknown error')}"
            print_progress(i, len(classes_to_process), msg, Colors.RED)
    
    # Save cache
    if cache_data is not None and not args.dry_run:
        cache_data["reflection_hash"] = compute_file_hash(REFLECTION_GEN)
        save_cache(args.cache_file, cache_data)
        print_section("Cache updated")
        print(f"   Validated: {len(cache_data['validated_tests'])} tests")
    
    # Summary
    print_section("Summary")
    print(f"   Generated: {stats.get('generated', 0)}")
    print(f"   Updated: {stats.get('updated', 0)}")
    print(f"   Valid: {stats.get('valid', 0)}")
    if stats.get('errors', 0) > 0:
        print(f"   Errors: {stats['errors']}")
    
    # Auto-update test.cpp if anything changed
    if not args.dry_run and (stats.get('generated', 0) > 0 or stats.get('updated', 0) > 0):
        print_section("Updating test.cpp")
        update_test_main_file(args.dry_run)
        print_success("test.cpp regenerated successfully")


if __name__ == "__main__":
    main()
