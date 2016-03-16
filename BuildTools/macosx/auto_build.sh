#!/bin/sh

VERSION=$1

#launchctl load auto_build.plist
cd ~/geoda_trunk/
git checkout rc/GdaAppResources.cpp
git pull
cd ~/geoda_trunk/BuildTools/macosx
./build.sh 16 1
cd ~/Dropbox/yoursway-create-dmg
geoda.sh $VERSION
mv GeoDa$VERSION-Installer.dmg ~/Dropbox
cd ..
