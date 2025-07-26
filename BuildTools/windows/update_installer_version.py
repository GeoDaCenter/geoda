#!/usr/bin/env python3
"""
Script to update AppVersion in GeoDa installer files based on version.h
This script reads the version information from version.h and updates all
installer .iss files to use the full version number instead of just major.minor
"""

import os
import re
import sys
from pathlib import Path


def extract_version_from_header(version_h_path):
    """Extract version components from version.h file"""
    version_info = {}

    try:
        with open(version_h_path, 'r') as f:
            content = f.read()

        # Extract version components using regex
        patterns = {
            'major': r'const int version_major = (\d+);',
            'minor': r'const int version_minor = (\d+);',
            'build': r'const int version_build = (\d+);',
            'subbuild': r'const int version_subbuild = (\d+);'
        }

        for component, pattern in patterns.items():
            match = re.search(pattern, content)
            if match:
                version_info[component] = int(match.group(1))
            else:
                print(
                    f"Warning: Could not find {component} version in {version_h_path}")
                return None

        return version_info

    except FileNotFoundError:
        print(f"Error: version.h file not found at {version_h_path}")
        return None
    except Exception as e:
        print(f"Error reading version.h: {e}")
        return None


def update_installer_file(iss_file_path, full_version):
    """Update AppVersion in a single installer file"""
    try:
        with open(iss_file_path, 'r') as f:
            content = f.read()

        # Replace AppVersion line with full version
        old_pattern = r'(AppVersion=)1\.\d+(\.\d+)*'
        new_content = re.sub(old_pattern, f'\\g<1>{full_version}', content)

        # Also update OutputBaseFilename to use full version for internal versioning
        old_filename_pattern = r'(OutputBaseFilename=GeoDa_)1\.\d+(\.\d+)*'
        new_content = re.sub(old_filename_pattern,
                             f'\\g<1>{full_version}', new_content)

        if new_content != content:
            with open(iss_file_path, 'w') as f:
                f.write(new_content)
            print(f"Updated {iss_file_path} with version {full_version}")
            return True
        else:
            print(f"No changes needed for {iss_file_path}")
            return False

    except Exception as e:
        print(f"Error updating {iss_file_path}: {e}")
        return False


def main():
    # Get the script directory and find the project root
    script_dir = Path(__file__).parent.absolute()
    # BuildTools/windows -> BuildTools -> project root
    project_root = script_dir.parent.parent
    version_h_path = project_root / "version.h"

    # Extract version information
    version_info = extract_version_from_header(version_h_path)
    if not version_info:
        sys.exit(1)

    # Create full version string
    full_version = f"{version_info['major']}.{version_info['minor']}.{version_info['build']}.{version_info['subbuild']}"
    print(f"Full version: {full_version}")

    # Find all installer files
    installer_dir = script_dir / "installer"
    iss_files = []

    for arch_dir in ["32bit", "64bit"]:
        arch_path = installer_dir / arch_dir
        if arch_path.exists():
            for iss_file in arch_path.glob("*.iss"):
                iss_files.append(iss_file)

    if not iss_files:
        print("No installer files found!")
        sys.exit(1)

    # Update all installer files
    updated_count = 0
    for iss_file in iss_files:
        if update_installer_file(iss_file, full_version):
            updated_count += 1

    print(f"\nUpdated {updated_count} out of {len(iss_files)} installer files")

    if updated_count == 0:
        print("No files were updated. All installer files may already have the correct version.")
    elif updated_count == len(iss_files):
        print("All installer files updated successfully!")
    else:
        print("Some files could not be updated. Please check the output above.")


if __name__ == "__main__":
    main()
