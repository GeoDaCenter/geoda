#!/bin/bash 
GEODA_HOME=$PWD
VER_MAJOR=$(grep version_major $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
VER_MINOR=$(grep version_minor $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
VER_BUILD=$(grep version_build $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
GEODA_VERSION=$VER_MAJOR.$VER_MINOR.$VER_BUILD
echo $GEODA_VERSION

hdiutil create GeoDa$GEODA_VERSION.dmg -volname "GEODA_$GEODA_VERSION" -fs HFS+ -srcfolder "build"
