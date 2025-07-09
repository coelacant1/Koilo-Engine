import os
import re
import argparse
from pathlib import Path
from collections import defaultdict

# This regex for finding class definitions, i.e., "class Triangle2D"
CLASS_DEF_REGEX = re.compile(r"class\s+([a-zA-Z0-9_]+)")

class DynamicIncludeCorrector:
    """
    A script to dynamically find and correct C++ #include paths in a project.
    """
    def __init__(self, project_root, dry_run=False):
        self.project_root = Path(project_root).resolve()
        self.dry_run = dry_run
        self.include_regex = re.compile(r'^(#include\s*["<])([^">]+)([">].*)$')
        self.files_changed = 0
        self.includes_corrected = 0
        self.file_index = self._build_file_index() # Build file index

    def _build_file_index(self):
        """
        Walks project directory to build a map of source file's basename to a list of 
        its full, absolute paths, ignoring dot-folders.
        """
        index = defaultdict(list)
        for root, dirs, files in os.walk(self.project_root): # Traverse tree
            dirs[:] = [d for d in dirs if not d.startswith('.')] # Exclude . dirs

            for file in files:
                if file.endswith(('.hpp', '.cpp', '.tpp', '.h')):
                    index[file].append(Path(root) / file)
        return index

    def _get_class_defined_in_file(self, file_path):
        """
        Find the primary class name defined in a header file.
        Returns the first class name found, e.g., "Triangle2D".
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            match = CLASS_DEF_REGEX.search(content)
            if match:
                return match.group(1)
        except Exception:
            return None
        return None

    def _resolve_ambiguous_path(self, basename, candidates, source_file_path):
        """
        Handles cases where multiple files have the same basename.
        Reads the source file content to see which class constructor is used.
        """
        try:
            with open(source_file_path, 'r', encoding='utf-8') as f:
                source_content = f.read()
        except Exception:
            return candidates[0] # Fallback on error

        for candidate_path in candidates:
            class_name = self._get_class_defined_in_file(candidate_path)
            if not class_name:
                continue
            
            usage_pattern = re.compile(fr'\b{class_name}\b') # Look for usage of class, e.g., "Triangle2D(" or "Triangle2D myVar"
            if usage_pattern.search(source_content):
                return candidate_path

        return candidates[0]

    def run(self):
        """Starts the process of scanning and correcting files."""
        print(f"Scanning project root: {self.project_root}")
        if self.dry_run:
            print("--- DRY RUN MODE: No files will be modified. ---")

        for basename, paths in self.file_index.items(): # Step through filtered file list
            for file_path in paths:
                self._process_file(file_path)

        print("\n--- Correction Complete ---")
        if self.dry_run:
            print(f"REPORT: Would correct {self.includes_corrected} include(s) in {self.files_changed} file(s).")
        else:
            print(f"Corrected {self.includes_corrected} include(s) in {self.files_changed} file(s).")

    def _process_file(self, file_path):
        """Reads a file, corrects its includes, and writes it back if changed."""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"[ERROR] Could not read file {file_path}: {e}")
            return

        new_lines = []
        has_changed = False
        file_dir = file_path.parent

        for line_num, line in enumerate(lines):
            match = self.include_regex.search(line)
            if not match:
                new_lines.append(line)
                continue

            prefix, original_include, suffix = match.groups()

            if '<' in prefix: # Skip system libraries
                new_lines.append(line)
                continue
            
            expected_path = (file_dir / original_include).resolve() # Check if path is valid
            if expected_path.exists():
                new_lines.append(line)
                continue

            basename = Path(original_include).name
            candidates = self.file_index.get(basename, [])

            if not candidates:
                new_lines.append(line)
                continue

            correct_abs_path = None
            if len(candidates) == 1:
                correct_abs_path = candidates[0]
            else:
                correct_abs_path = self._resolve_ambiguous_path(basename, candidates, file_path)

            new_relative_path = os.path.relpath(correct_abs_path, file_dir).replace('\\', '/')
            new_line = f'{prefix}{new_relative_path}{suffix}\n'
            
            if line.strip() != new_line.strip():
                new_lines.append(new_line)
                has_changed = True
                self.includes_corrected += 1
                if self.dry_run:
                    print(f"In {file_path.relative_to(self.project_root)} (Line {line_num + 1}):")
                    print(f"  - BROKEN: {line.strip()}")
                    print(f"  - FIXED:  {new_line.strip()}")
            else:
                new_lines.append(line)

        if has_changed:
            self.files_changed += 1
            if not self.dry_run:
                try:
                    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
                        f.writelines(new_lines)
                    print(f"Updated: {file_path.relative_to(self.project_root)}")
                except Exception as e:
                    print(f"[ERROR] Could not write to file {file_path}: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Dynamically find and correct C++ #include paths.")
    parser.add_argument("project_root", help="The root directory of the project to scan (e.g., 'um3d').")
    parser.add_argument("--dry-run", action="store_true", help="Print changes without modifying files.")
    
    args = parser.parse_args()

    if not Path(args.project_root).is_dir():
        print(f"Error: Provided path '{args.project_root}' is not a valid directory.")
    else:
        corrector = DynamicIncludeCorrector(args.project_root, args.dry_run)
        corrector.run()
