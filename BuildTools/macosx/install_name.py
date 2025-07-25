'''
Update the install name of dependent libraries of libgdal and libwx
Code sign each dependent library
'''
import subprocess
import os
import sys
import re
from pathlib import Path
from shutil import copyfile


def get_gdal_dylib_name():
    """Get the GDAL dylib name dynamically"""
    try:
        # Get GDAL version from gdal-config
        gdal_version = subprocess.check_output(['gdal-config', '--version'],
                                               stderr=subprocess.STDOUT,
                                               universal_newlines=True).strip()

        # Extract major version (e.g., "3.7.4" -> "37")
        major_version = ''.join(gdal_version.split('.')[:2])

        # Always use the standard naming pattern based on major version
        standard_name = f"libgdal.{major_version}.dylib"

        # Check if the standard dylib exists
        possible_paths = [
            f"/usr/local/opt/gdal/lib/{standard_name}",
            f"/opt/homebrew/opt/gdal/lib/{standard_name}"
        ]

        for path in possible_paths:
            if os.path.exists(path):
                return standard_name

        # If standard dylib doesn't exist, check if any libgdal.*.dylib file exists
        # but still return the standard name for consistency
        for base_path in ["/usr/local/opt/gdal/lib", "/opt/homebrew/opt/gdal/lib"]:
            if os.path.exists(base_path):
                for file in os.listdir(base_path):
                    if file.startswith("libgdal.") and file.endswith(".dylib"):
                        # Found a dylib file, but return the standard name
                        return standard_name

        # If all else fails, return the standard name
        return standard_name

    except (subprocess.CalledProcessError, FileNotFoundError):
        # Fallback to default if gdal-config is not available
        return "libgdal.37.dylib"


# e.g. '/Users/xun/Github/geoda/BuildTools/macosx/build/GeoDa.app/Contents/Frameworks'
FRAMEWORK_PATH = sys.argv[1]
CODESIGN_ID = sys.argv[2]
GDAL_DYLIB_NAME = sys.argv[3] if len(sys.argv) > 3 else get_gdal_dylib_name()
CODESIGN_ONLY = bool(sys.argv[4]) if len(sys.argv) > 4 else False

print(CODESIGN_ID, CODESIGN_ONLY)

PROCESSED_DYLIBS = {}


def process_dependency(framework_path, dylib_name):
    """_Process each dependent library_

    Args:
        framework_path (_str_): _Framework path_
        dylib_name (_str_): _dylib file name_
    """
    if dylib_name in PROCESSED_DYLIBS:
        print(f'{dylib_name} has been processed')
        return
    PROCESSED_DYLIBS[dylib_name] = True
    dylib_path = framework_path + '/' + dylib_name
    dep_libs = subprocess.check_output(
        ['otool', '-L', dylib_path]).decode('utf-8')
    all_items = dep_libs.split('\n')
    # e.g. '\t/opt/homebrew/opt/gdal/lib/libgdal.33.dylib (compatibility version 33.0.0, current version 33.3.7)'
    current_item = all_items[1].strip().split(" ")[0]
    items = all_items[2:-1]
    for item in items:
        # e.g. '@loader_path/../../../../opt/libarchive/lib/libarchive.13.dylib (compatibility version 20.0.0, current version 20.2.0)'
        item = item.strip().split(" ")[0]
        copyitem = item

        if item.startswith('@rpath'):
            item_filename = os.path.basename(item)
            copy_dir = str(Path(current_item).parent)
            copyitem = f'{copy_dir}/{item_filename}'

        if item.startswith('@loader_path'):
            item_filename = os.path.basename(item)
            upper_levels = item.count('../')
            copy_dir = str(Path(current_item).parent)
            if upper_levels - 1 >= 0:
                current_dir = Path(current_item).parents[upper_levels - 1]
                copy_dir = str(current_dir)
                item_filename = item[item.rindex('../') + 3:]
            copyitem = f'{copy_dir}/{item_filename}'

        if not item.startswith('/usr/lib') and not item.startswith('/System') \
                and not (CODESIGN_ONLY or item.startswith('@executable_path/')):
            print("Process:", item)
            file_name = os.path.basename(item)
            # Copy the dylib to Frameworks if needed
            dest = framework_path + '/' + file_name
            if not os.path.exists(dest) and not CODESIGN_ONLY:
                copyfile(copyitem, dest, follow_symlinks=True)
            # install_name_tool current item
            new_path = f"@executable_path/../Frameworks/{file_name}"
            cmd = f'install_name_tool -change "{item}" "{new_path}" {dylib_path}'
            if not CODESIGN_ONLY:
                os.system(cmd)
            # process item
            process_dependency(framework_path, file_name)
    print("codesign {}", dylib_path)
    cmd = f'codesign --force --timestamp -o runtime -s "{CODESIGN_ID}" {dylib_path}'
    os.system(cmd)


process_dependency(FRAMEWORK_PATH, "libwx_osx_cocoau_gl-3.2.dylib")
process_dependency(FRAMEWORK_PATH, "libwx_osx_cocoau-3.2.dylib")
process_dependency(FRAMEWORK_PATH, GDAL_DYLIB_NAME)
