import subprocess
import os
from shutil import copyfile

def ProcessDependency(dir_path, dylib_name):
    dylib_path = dir_path + '/' + dylib_name
    dep_libs = subprocess.check_output(['otool', '-L', dylib_path]).decode('utf-8')
    items = dep_libs.split('\n')[2:-1]
    for item in items:
        item = item.strip().split(" ")[0]
        if item.startswith('/usr/lib') == False and item.startswith('/System') == False and item.startswith('@executable_path/')==False:
            print("Process:", item)
            file_name = os.path.basename(item)
            # Copy the dylib to Frameworks if needed
            dest = dir_path + '/' + file_name
            if os.path.exists(dest) == False:
                copyfile(item, dest)
            # install_name_tool current item
            new_path = "@executable_path/../Frameworks/{}".format(file_name)
            cmd = "install_name_tool -change \"{}\" \"{}\" {}".format(item, new_path, dylib_path)
            os.system(cmd)
            # process item
            ProcessDependency(dir_path, file_name)


framework_path = '/Users/xun/Github/geoda/BuildTools/macosx/build/GeoDa.app/Contents/Frameworks'

ProcessDependency(framework_path, "libwx_osx_cocoau_gl-3.1.dylib")
ProcessDependency(framework_path, "libwx_osx_cocoau-3.1.dylib")
ProcessDependency(framework_path, "libgdal.28.dylib")