'''
code sign geoda dependent dylibs inplace
'''
import subprocess
import os
import re
import sys
from pathlib import Path

processed_items = {}


def ProcessDependency(dylib_path, cid, current_item=None):
    print("ProcessDependency:", dylib_path, cid, current_item)
    if dylib_path in processed_items:
        return
    else:
        processed_items[dylib_path] = True

    if dylib_path.startswith('@rpath'):
        print('@rpath: before', dylib_path)
        item_filename = os.path.basename(dylib_path)
        copy_dir = str(Path(current_item).parent)
        dylib_path = f'{copy_dir}/{item_filename}'
        print('@rpath: after', dylib_path)
    # m = re.search('@loader_path/../../../../(opt*)', dylib_path)
    # if m:
    #     dylib_path = '/usr/local/' + m.group(1)

    if dylib_path.startswith('@loader_path'):
        item_filename = os.path.basename(dylib_path)
        upper_levels = dylib_path.count('../')
        copy_dir = str(Path(current_item).parent)
        print('upper_levels:', upper_levels, 'copy_dir:',
              copy_dir, 'item_filename:', item_filename)
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
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.34.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
ProcessDependency(sys.argv[1], sys.argv[2])
