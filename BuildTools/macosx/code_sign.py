import subprocess
import os
import sys
import re
from shutil import copyfile

processed_items = {}


def ProcessDependency(dylib_path, cid):
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
    if dylib_path == '@rpath/libgeos.3.11.2.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.11.2.dylib'
    if dylib_path == '@rpath/libgeos.3.12.2.dylib':
        dylib_path = '/opt/homebrew/opt/geos/lib/libgeos.3.12.2.dylib'
    if dylib_path == '@loader_path/libicuuc.71.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicuuc.71.dylib'
    if dylib_path == '@loader_path/libicuuc.72.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicuuc.72.dylib'
    if dylib_path == '@loader_path/libicuuc.73.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicuuc.73.dylib'
    if dylib_path == '@loader_path/libicudata.71.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicudata.71.dylib'
    if dylib_path == '@loader_path/libicudata.72.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicudata.72.dylib'
    if dylib_path == '@loader_path/libicudata.73.dylib':
        dylib_path = '/opt/homebrew/opt/icu4c/lib/libicudata.73.dylib'
    if dylib_path == '@loader_path/libbrotlicommon.1.dylib':
        dylib_path = '/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib'
    if dylib_path == '@rpath/libsharpyuv.0.dylib':
        dylib_path = '/opt/homebrew/opt/webp/lib/libsharpyuv.0.dylib'
    if dylib_path == '@loader_path/libkmlbase.1.dylib':
        dylib_path = '/opt/homebrew/opt/libkml/lib/libkmlbase.1.dylib'
    if dylib_path == '@loader_path/libkmldom.1.dylib':
        dylib_path = '/opt/homebrew/opt/libkml/lib/libkmldom.1.dylib'

    m = re.search('@rpath/libIlmThread-(*).dylib', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/openexr/lib/libIlmThread-' + \
            m.group(1) + '.dylib'
    m = re.search('@rpath/libIex-(*).dylib', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/openexr/lib/libIex-' + \
            m.group(1) + '.dylib'
    m = re.search('@rpath/libOpenEXR-(*).dylib', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/openexr/lib/libOpenEXR-' + \
            m.group(1) + '.dylib'
    m = re.search('@rpath/libOpenEXRCore-(*).dylib', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/openexr/lib/libOpenEXRCore-' + \
            m.group(1) + '.dylib'

    m = re.search('@rpath/(libabsl.*)', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/abseil/lib/' + m.group(1)

    m = re.search('@rpath/(libaws.*)', dylib_path)
    if m:
        dylib_path = '/usr/local/opt/aws-sdk-cpp/lib/' + m.group(1)

    m = re.search('@loader_path/../../../../(opt*)', dylib_path)
    if m:
        dylib_path = '/usr/local/' + m.group(1)

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
            ProcessDependency(item, cid)


# e.g.
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.29.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
# python3 code_sign.py /opt/homebrew/opt/gdal/lib/libgdal.32.dylib "Apple Development: xunli@uchicago.edu (AN5USPSZF6)"
ProcessDependency(sys.argv[1], sys.argv[2])
