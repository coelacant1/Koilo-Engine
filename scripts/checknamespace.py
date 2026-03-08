#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Koilo Namespace Checker and Fixer

This script scans C++ header files to ensure all classes are properly
wrapped in the koilo namespace. It can detect global namespace violations
and optionally fix them automatically.

Usage:
    python checknamespace.py --check             # Check only
    python checknamespace.py --fix               # Fix violations
    python checknamespace.py --report report.txt # Generate report
"""

import re
import os
import sys
import argparse
from pathlib import Path
from typing import List, Tuple, Dict, Set
from dataclasses import dataclass
from collections import defaultdict


@dataclass
class NamespaceViolation:
    """Represents a class found in global namespace"""
    file_path: str
    line_number: int
    class_name: str
    class_type: str  # 'class' or 'struct'
    context: str  # Few lines of context


class NamespaceChecker:
    """Analyzes C++ headers for namespace violations"""
    
    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir)
        self.violations: List[NamespaceViolation] = []
        self.checked_files: List[str] = []
        self.fixed_files: List[str] = []
        
        # Patterns
        self.class_pattern = re.compile(
            r'^\s*(class|struct)\s+(\w+)(?:\s+[^\s{;:]+)*\s*(?::|{|;)',
            re.MULTILINE
        )
        self.namespace_pattern = re.compile(
            r'^\s*namespace\s+([A-Za-z_][\w:]*)\s*{',
            re.MULTILINE
        )
        self.namespace_end_pattern = re.compile(
            r'^\s*}\s*//\s*namespace\s+([\w:]+)?',
            re.MULTILINE
        )
        
        # Exceptions - types that should be global
        self.global_exceptions: Set[str] = set()
    
    def should_skip_file(self, file_path: Path) -> bool:
        """Check if file should be skipped"""
        skip_dirs = {'build', 'third_party', 'external', '.git', 'tests', 'vendor'}
        skip_files = {'stb_', 'imgui', 'glad'}
        
        # Skip if in excluded directory
        for part in file_path.parts:
            if part in skip_dirs:
                return True
        
        # Skip if filename contains excluded prefix
        for prefix in skip_files:
            if prefix in file_path.name:
                return True
        
        return False
    
    def is_in_namespace(self, lines: List[str], line_idx: int) -> Tuple[bool, str]:
        """Check if a line is inside a namespace block"""
        namespace_stack = []
        
        for i in range(line_idx + 1):
            line = lines[i].strip()
            
            # Check for namespace start
            ns_match = self.namespace_pattern.match(lines[i])
            if ns_match:
                namespace_stack.append(ns_match.group(1))
            
            # Check for namespace end
            ns_end_match = self.namespace_end_pattern.match(lines[i])
            if ns_end_match and namespace_stack:
                namespace_stack.pop()
        
        if namespace_stack:
            return True, '::'.join(namespace_stack)
        return False, ''
    
    def analyze_file(self, file_path: Path) -> List[NamespaceViolation]:
        """Analyze a single file for namespace violations"""
        violations = []
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
            
            # Find all class/struct declarations
            for match in self.class_pattern.finditer(content):
                class_type = match.group(1)  # 'class' or 'struct'
                class_name = match.group(2)
                matched_text = match.group(0)
                
                # Skip forward declarations (no body)
                if ';' in matched_text and '{' not in matched_text:
                    continue
                
                # Skip exceptions
                if class_name in self.global_exceptions:
                    continue
                
                # Find line number
                line_num = content[:match.start()].count('\n') + 1
                
                # Check if in namespace
                in_namespace, ns_name = self.is_in_namespace(lines, line_num - 1)
                
                if not in_namespace or not ns_name.startswith('koilo'):
                    # Get context (3 lines before and after)
                    start_line = max(0, line_num - 3)
                    end_line = min(len(lines), line_num + 3)
                    context = '\n'.join(lines[start_line:end_line])
                    
                    violation = NamespaceViolation(
                        file_path=str(file_path),
                        line_number=line_num,
                        class_name=class_name,
                        class_type=class_type,
                        context=context
                    )
                    violations.append(violation)
        
        except Exception as e:
            print(f"Error analyzing {file_path}: {e}", file=sys.stderr)
        
        return violations
    
    def scan_directory(self, directory: Path = None) -> None:
        """Scan directory for C++ headers"""
        if directory is None:
            directory = self.root_dir
        
        # Find all .hpp and .h files
        header_files = list(directory.rglob('*.hpp')) + list(directory.rglob('*.h'))
        
        for file_path in header_files:
            if self.should_skip_file(file_path):
                continue
            
            self.checked_files.append(str(file_path))
            violations = self.analyze_file(file_path)
            self.violations.extend(violations)
    
    def fix_file(self, file_path: str) -> bool:
        """Fix namespace violations in a file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
            
            # Check if file needs fixing
            file_violations = [v for v in self.violations if v.file_path == file_path]
            if not file_violations:
                return False
            
            # Check if already has koilo namespace
            has_koilo_namespace = 'namespace koilo' in content
            
            if not has_koilo_namespace:
                # Find where to insert namespace
                # Look for first #include, then insert after last #include
                last_include_idx = -1
                pragma_once_idx = -1
                
                for i, line in enumerate(lines):
                    if '#pragma once' in line:
                        pragma_once_idx = i
                    if line.strip().startswith('#include'):
                        last_include_idx = i
                
                # Insert namespace after includes (or after pragma once if no includes)
                insert_idx = max(last_include_idx, pragma_once_idx) + 1
                
                # Skip blank lines
                while insert_idx < len(lines) and not lines[insert_idx].strip():
                    insert_idx += 1
                
                # Insert namespace opening
                lines.insert(insert_idx, '')
                lines.insert(insert_idx + 1, 'namespace koilo {')
                lines.insert(insert_idx + 2, '')
                
                # Find where to insert closing brace
                close_idx = len(lines)
                while close_idx > 0 and not lines[close_idx - 1].strip():
                    close_idx -= 1
                # Ensure we insert before trailing #endif or similar directives
                while close_idx > 0 and lines[close_idx - 1].strip().startswith('#endif'):
                    close_idx -= 1
                while close_idx > 0 and lines[close_idx - 1].strip().startswith('#pragma'):
                    # Leave pragmas (unlikely at end) at bottom
                    break
                
                # Insert namespace closing
                lines.insert(close_idx, '')
                lines.insert(close_idx + 1, '} // namespace koilo')
                
                # Write back
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write('\n'.join(lines))
                
                self.fixed_files.append(file_path)
                return True
        
        except Exception as e:
            print(f"Error fixing {file_path}: {e}", file=sys.stderr)
            return False
    
    def generate_report(self, output_file: str = None) -> str:
        """Generate a report of findings"""
        report_lines = []
        report_lines.append("=" * 80)
        report_lines.append("Koilo Namespace Violation Report")
        report_lines.append("=" * 80)
        report_lines.append("")
        report_lines.append(f"Files checked: {len(self.checked_files)}")
        report_lines.append(f"Violations found: {len(self.violations)}")
        report_lines.append("")
        
        if not self.violations:
            report_lines.append("All classes are properly namespaced!")
            report_lines.append("")
        else:
            # Group by file
            by_file: Dict[str, List[NamespaceViolation]] = defaultdict(list)
            for v in self.violations:
                by_file[v.file_path].append(v)
            
            report_lines.append("Violations by file:")
            report_lines.append("-" * 80)
            
            for file_path, violations in sorted(by_file.items()):
                rel_path = os.path.relpath(file_path, self.root_dir)
                report_lines.append("")
                report_lines.append(f"{rel_path}")
                report_lines.append(f"   {len(violations)} violation(s)")
                
                for v in violations:
                    report_lines.append(f"   Line {v.line_number}: {v.class_type} {v.class_name}")
                    report_lines.append("")
                    # Show context with line numbers
                    context_lines = v.context.split('\n')
                    start_line = v.line_number - (len(context_lines) // 2)
                    for i, ctx_line in enumerate(context_lines):
                        line_num = start_line + i
                        marker = ">>>" if line_num == v.line_number else "   "
                        report_lines.append(f"   {marker} {line_num:4d} | {ctx_line}")
                    report_lines.append("")
        
        report_lines.append("=" * 80)
        
        report_text = '\n'.join(report_lines)
        
        if output_file:
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(report_text)
            print(f"Report written to: {output_file}")
        
        return report_text


def main():
    parser = argparse.ArgumentParser(
        description='Check and fix Koilo namespace usage in C++ headers'
    )
    parser.add_argument(
        'root_dir',
        nargs='?',
        default='.',
        help='Root directory to scan (default: current directory)'
    )
    parser.add_argument(
        '--check',
        action='store_true',
        help='Check for violations without fixing'
    )
    parser.add_argument(
        '--fix',
        action='store_true',
        help='Automatically fix violations'
    )
    parser.add_argument(
        '--report',
        metavar='FILE',
        help='Generate report to file'
    )
    parser.add_argument(
        '--include-dir',
        default='engine',
        help='Subdirectory to scan (default: engine)'
    )
    
    args = parser.parse_args()
    
    # Default to check mode if nothing specified
    if not args.check and not args.fix and not args.report:
        args.check = True
    
    root_path = Path(args.root_dir).resolve()
    if not root_path.exists():
        print(f"Error: Directory not found: {root_path}", file=sys.stderr)
        sys.exit(1)
    
    scan_path = root_path / args.include_dir
    if not scan_path.exists():
        print(f"Warning: Include directory not found: {scan_path}", file=sys.stderr)
        scan_path = root_path
    
    print(f"Scanning directory: {scan_path}")
    print()
    
    checker = NamespaceChecker(root_path)
    checker.scan_directory(scan_path)
    
    print(f"Checked {len(checker.checked_files)} files")
    print(f"Found {len(checker.violations)} violations")
    print()
    
    if args.check or not args.fix:
        # Just report
        if checker.violations:
            print("Violations found:")
            by_file = defaultdict(list)
            for v in checker.violations:
                by_file[v.file_path].append(v)
            
            for file_path, violations in sorted(by_file.items()):
                rel_path = os.path.relpath(file_path, root_path)
                print(f"  {rel_path}")
                for v in violations:
                    print(f"    Line {v.line_number}: {v.class_type} {v.class_name}")
            print()
            print("Run with --fix to automatically fix these violations")
        else:
            print("No violations found! All classes use koilo namespace.")
    
    if args.fix:
        print("Fixing violations...")
        files_to_fix = set(v.file_path for v in checker.violations)
        
        for file_path in files_to_fix:
            if checker.fix_file(file_path):
                rel_path = os.path.relpath(file_path, root_path)
                print(f"  Fixed: {rel_path}")
        
        print()
        print(f"Fixed {len(checker.fixed_files)} files")
    
    if args.report:
        report = checker.generate_report(args.report)
        print()
        print("Report generated!")
    
    # Exit code: 0 if no violations, 1 if violations found
    sys.exit(0 if not checker.violations else 1)


if __name__ == '__main__':
    main()
