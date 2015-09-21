#!/bin/sh

#launchctl load auto_build.plist
svn update ~/geoda_trunk/
cd ~/geoda_trunk/BuildTools/macosx
./build.sh 16 1
cd build/
zip -r GeoDa.zip GeoDa.app/
mv GeoDa.zip ~/Dropbox
cd ..
