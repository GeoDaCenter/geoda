import subprocess
import os, sys
from shutil import copyfile

processed_items = {}

def ProcessDependency(dylib_path, cid):
    if dylib_path in processed_items:
        return
    else:
        processed_items[dylib_path] = True

    if dylib_path == '@rpath/libgeos.3.10.2.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.10.2.dylib'

    print("Process:", dylib_path)
    #cmd = "codesign -f -s - "
    cmd = '/usr/bin/codesign --force --sign "{}" '.format(cid)
    os.system(cmd + dylib_path)

    dep_libs = subprocess.check_output(['otool', '-L', dylib_path]).decode('utf-8')
    items = dep_libs.split('\n')[2:-1]
    for item in items:
        item = item.strip().split(" ")[0]
        if item.startswith('/usr/lib') == False and item.startswith('/System') == False:
            # process item
            ProcessDependency(item, cid)

# e.g.
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.29.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
ProcessDependency(sys.argv[1], sys.argv[2])