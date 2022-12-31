import subprocess
import os, sys
from shutil import copyfile

framework_path = sys.argv[1] #e.g. '/Users/xun/Github/geoda/BuildTools/macosx/build/GeoDa.app/Contents/Frameworks'
id = sys.argv[2]
codesign_only = bool(sys.argv[3]) if len(sys.argv) > 3  else False

print(id, codesign_only)

def ProcessDependency(dir_path, dylib_name):
    dylib_path = dir_path + '/' + dylib_name
    dep_libs = subprocess.check_output(['otool', '-L', dylib_path]).decode('utf-8')
    items = dep_libs.split('\n')[2:-1]
    for item in items:
        item = item.strip().split(" ")[0]

        # workaround for gdal 3.3.3 that @rpath/libgeos.3.10.2.dylib was used instead of dir path
        copyitem = item
        if item == '@rpath/libgeos.3.10.2.dylib':
            copyitem = '/usr/local/opt/geos/lib/libgeos.dylib'
        if item == '@rpath/libgeos.3.11.0.dylib':
            copyitem = '/usr/local/opt/geos/lib/libgeos.dylib'
        if item == '@rpath/libgeos.3.11.1.dylib':
            copyitem = '/usr/local/opt/geos/lib/libgeos.dylib'
        if item == '@loader_path/libicuuc.70.dylib':
            copyitem = '/usr/local/opt/icu4c/lib/libicuuc.70.dylib'
        if item == '@loader_path/libicuuc.71.dylib':
            copyitem = '/usr/local/opt/icu4c/lib/libicuuc.71.dylib'
        if item == '@loader_path/libicudata.70.dylib':
            copyitem = '/usr/local/opt/icu4c/lib/libicudata.70.dylib'
        if item == '@loader_path/libicudata.71.dylib':
            copyitem = '/usr/local/opt/icu4c/lib/libicudata.71.dylib'
        if item == '@loader_path/libbrotlicommon.1.dylib':
            copyitem = '/usr/local/opt/brotli/lib/libbrotlicommon.1.dylib'
        if item == '@rpath/libIlmThread-3_1.30.dylib':
            copyitem = '/usr/local/opt/openexr/lib/libIlmThread-3_1.30.dylib'
        if item == '@rpath/libIex-3_1.30.dylib':
            copyitem = '/usr/local/opt/openexr/lib/libIex-3_1.30.dylib'
        if item == '@rpath/libOpenEXRCore-3_1.30.dylib':
            copyitem = '/usr/local/opt/openexr/lib/libOpenEXRCore-3_1.30.dylib'
        if item == '@rpath/libOpenEXRCore-3_1.30.dylib':
            copyitem = '/usr/local/opt/openexr/lib/libOpenEXRCore-3_1.30.dylib'

        if item.startswith('/usr/lib') == False and item.startswith('/System') == False and (codesign_only or item.startswith('@executable_path/')==False):
            print("Process:", item)
            file_name = os.path.basename(item)
            # Copy the dylib to Frameworks if needed
            dest = dir_path + '/' + file_name
            if os.path.exists(dest) == False and not codesign_only:
                copyfile(copyitem, dest, follow_symlinks=True)
            # install_name_tool current item
            new_path = "@executable_path/../Frameworks/{}".format(file_name)
            cmd = "install_name_tool -change \"{}\" \"{}\" {}".format(item, new_path, dylib_path)
            if not codesign_only:
                os.system(cmd)
            # process item
            ProcessDependency(dir_path, file_name)
    print("codesign {}", dylib_path)
    cmd = 'codesign --force --timestamp -o runtime -s "{}" {}'.format(id, dylib_path)
    os.system(cmd)


ProcessDependency(framework_path, "libwx_osx_cocoau_gl-3.1.dylib")
ProcessDependency(framework_path, "libwx_osx_cocoau-3.1.dylib")
ProcessDependency(framework_path, "libgdal.31.dylib")
