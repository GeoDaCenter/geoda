import subprocess
import os
import sys
import re
from shutil import copyfile
from pathlib import Path

processed_items = {}


def ProcessDependency(dylib_path, cid, current_item=None):
    if dylib_path in processed_items:
        return
    else:
        processed_items[dylib_path] = True

    if dylib_path == '@rpath/libgeos.3.11.2.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.11.2.dylib'
    if dylib_path == '@rpath/libgeos.3.11.1.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.11.1.dylib'
    if dylib_path == '@rpath/libgeos.3.11.0.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.11.0.dylib'
    if dylib_path == '@loader_path/libicuuc.71.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicuuc.71.dylib'
    if dylib_path == '@loader_path/libicudata.71.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicudata.71.dylib'
    if dylib_path == '@loader_path/libbrotlicommon.1.dylib':
        dylib_path = '/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib'

    if dylib_path == '@rpath/libIlmThread-3_1.30.dylib':
        dylib_path = '/usr/local/opt/openexr/lib/libIlmThread-3_1.30.dylib'
    if dylib_path == '@rpath/libIex-3_1.30.dylib':
        dylib_path = '/usr/local/opt/openexr/lib/libIex-3_1.30.dylib'
    if dylib_path == '@rpath/libOpenEXR-3_1.30.dylib':
        dylib_path = '/usr/local/opt/openexr/lib/libOpenEXR-3_1.30.dylib'
    if dylib_path == '@rpath/libOpenEXRCore-3_1.30.dylib':
        dylib_path = '/usr/local/opt/openexr/lib/libOpenEXRCore-3_1.30.dylib'
        
    m = re.search('@rpath/(libabsl.*)', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/abseil/lib/' + m.group(1)
    elif dylib_path.startswith('@rpath'):
        item_filename = os.path.basename(dylib_path)
        copy_dir = str(Path(current_item).parent)
        dylib_path = f'{copy_dir}/{item_filename}'

    m = re.search('@rpath/(libaws.*)', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/aws-sdk-cpp/lib/' + m.group(1)

    if dylib_path.startswith('@loader_path'):
        item_filename = os.path.basename(dylib_path)
        upper_levels = dylib_path.count('../')
        copy_dir = str(Path(current_item).parent)
        if upper_levels - 1 >= 0:
            current_dir = Path(current_item).parents[upper_levels - 1]
            copy_dir = str(current_dir)
            item_filename = dylib_path[dylib_path.rindex('../') + 3:]
            dylib_path = f'{copy_dir}/{item_filename}'
        elif upper_levels == 0:
            dylib_path = f'{copy_dir}/{item_filename}'
            
    print("Process:", dylib_path)
    # cmd = "codesign -f -s - "
    cmd = '/usr/bin/codesign --force --sign "{}" '.format(cid)
    os.system(cmd + dylib_path)

    dep_libs = subprocess.check_output(
        ['otool', '-L', dylib_path]).decode('utf-8')
    items = dep_libs.split('\n')[2:-1]
    for item in items:
        item = item.strip().split(" ")[0]
        if item.startswith('/usr/lib') == False and item.startswith('/System') == False:
            # process item
            ProcessDependency(item, cid, dylib_path)


# e.g.
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.29.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.32.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
#ProcessDependency(sys.argv[1], sys.argv[2])
#ProcessDependency('/opt/homebrew/Cellar/gdal/3.7.2/lib/libgdal.33.3.7.2.dylib', "Apple Development: xunli@uchicago.edu (AN5USPSZF6)")
