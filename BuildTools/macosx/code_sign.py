import subprocess
import os, sys
from shutil import copyfile

processed_items = {}

def ProcessDependency(dylib_path):
    if dylib_path in processed_items:
        return
    else:
        processed_items[dylib_path] = True

    print("Process:", dylib_path)
    #cmd = "codesign -f -s - "
    cmd = '/usr/bin/codesign --force --sign 2C86718CD200161736254D6451732A05183661DE -o runtime --entitlements /Users/xun/Library/Developer/Xcode/DerivedData/GeoDa.m1-bzlwzpadckllnretkivbujjhepho/Build/Intermediates.noindex/GeoDa.m1.build/Debug/GeoDa.build/GeoDa.app.xcent --timestamp\=none '
    os.system(cmd + dylib_path)

    dep_libs = subprocess.check_output(['otool', '-L', dylib_path]).decode('utf-8')
    items = dep_libs.split('\n')[2:-1]
    for item in items:
        item = item.strip().split(" ")[0]
        if item.startswith('/usr/lib') == False and item.startswith('/System') == False:
            # process item
            ProcessDependency(item)


ProcessDependency(sys.argv[1])