"""
Shared console output formatting for PTX Engine scripts.
Matches CMake build output style: [XX%] message in green
"""

import sys
from typing import Optional


# ANSI color codes
class Colors:
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    RED = '\033[31m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def format_progress(current: int, total: int, message: str, color: str = Colors.GREEN) -> str:
    """
    Format a progress message in CMake style: [XX%] message
    Everything after [ XX%] is colored.
    
    Args:
        current: Current item number
        total: Total items
        message: Message to display
        color: ANSI color code (default green)
    
    Returns:
        Formatted string with color codes
    """
    if total > 0:
        percentage = int((current / total) * 100)
        return f"[{percentage:3d}%] {color}{message}{Colors.RESET}"
    else:
        return f"[ --] {color}{message}{Colors.RESET}"


def print_progress(current: int, total: int, message: str, color: str = Colors.GREEN):
    """Print a progress message in CMake style."""
    print(format_progress(current, total, message, color))


def print_status(message: str, color: str = Colors.GREEN):
    """Print a status message (no percentage)."""
    print(f"{color}{message}{Colors.RESET}")


def print_section(title: str):
    """Print a section header."""
    print(f"\n{title}")


def print_warning(message: str):
    """Print a warning message."""
    print(f"{Colors.YELLOW}{message}{Colors.RESET}")


def print_error(message: str):
    """Print an error message."""
    print(f"{Colors.RED}{message}{Colors.RESET}")


def print_success(message: str):
    """Print a success message."""
    print(f"{Colors.GREEN}{message}{Colors.RESET}")
