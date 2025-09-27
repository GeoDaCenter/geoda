#!/usr/bin/env python3
"""
Script to update version information in GeoDa macOS Info.plist based on version.h
This script reads the version information from version.h and updates
CFBundleVersion and CFBundleShortVersionString in GeoDa-GDAL-Info.plist

This script is automatically run during the macOS build process in the GitHub Actions
workflow (osx_build.yml) before 'make app' to ensure the Info.plist version matches
the actual build version from version.h.

Fixes issue where Munki and other package managers couldn't detect version updates
due to hardcoded version numbers in the Info.plist file.
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


def update_plist_file(plist_file_path, full_version):
    """Update CFBundleVersion and CFBundleShortVersionString in Info.plist file"""
    try:
        with open(plist_file_path, 'r') as f:
            content = f.read()

        # Replace CFBundleVersion value
        bundle_version_pattern = r'(<key>CFBundleVersion</key>\s*<string>)[^<]*(<\/string>)'
        new_content = re.sub(bundle_version_pattern, f'\\g<1>{full_version}\\g<2>', content)
        
        # Replace CFBundleShortVersionString value
        short_version_pattern = r'(<key>CFBundleShortVersionString</key>\s*<string>)[^<]*(<\/string>)'
        new_content = re.sub(short_version_pattern, f'\\g<1>{full_version}\\g<2>', new_content)

        if new_content != content:
            with open(plist_file_path, 'w') as f:
                f.write(new_content)
            print(f"Updated {plist_file_path} with version {full_version}")
            return True
        else:
            print(f"No changes needed for {plist_file_path}")
            return False

    except Exception as e:
        print(f"Error updating {plist_file_path}: {e}")
        return False


def main():
    # Get the script directory and find the project root
    script_dir = Path(__file__).parent.absolute()
    # BuildTools/macosx -> BuildTools -> project root
    project_root = script_dir.parent.parent
    version_h_path = project_root / "version.h"

    # Extract version information
    version_info = extract_version_from_header(version_h_path)
    if not version_info:
        sys.exit(1)

    # Create full version string
    full_version = f"{version_info['major']}.{version_info['minor']}.{version_info['build']}.{version_info['subbuild']}"
    print(f"Full version: {full_version}")

    # Find the Info.plist file
    plist_file_path = script_dir / "GeoDa-GDAL-Info.plist"
    
    if not plist_file_path.exists():
        print(f"Error: GeoDa-GDAL-Info.plist not found at {plist_file_path}")
        sys.exit(1)

    # Update the plist file
    if update_plist_file(plist_file_path, full_version):
        print("Successfully updated Info.plist version!")
    else:
        print("Info.plist version was already up to date.")


if __name__ == "__main__":
    main()