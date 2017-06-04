#!/bin/sh

VERSION=$1
CPU=$2
NODEBUG=$3

#launchctl load auto_build.plist
cd ~/geoda_trunk/
git checkout rc/GdaAppResources.cpp
git pull
rm ~/geoda_trunk/o/*
cd ~/geoda_trunk/BuildTools/macosx
cp dep/3D* ~/geoda_trunk/Explore/
./build.sh $CPU $NODEBUG
cd ~/geoda_trunk/BuildTools/macosx/create-dmg
./geoda.sh $VERSION
mv GeoDa$VERSION-Installer.dmg /Volumes/xun/Dropbox
cd ..
