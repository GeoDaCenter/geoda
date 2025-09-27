#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
echo $GEODA_HOME

# Build GeoDa
cp ../../GeoDamake.ubuntu.opt ../../GeoDamake.opt

# Add AppImage-specific static linking flags if needed
if [ "$APPIMAGE_BUILD" = "true" ] ; then
    echo "Building GeoDa for AppImage with static linking"
    # The EXTRA_GEODA_LD_FLAGS should already be set by the calling workflow
    echo "Using EXTRA_GEODA_LD_FLAGS: $EXTRA_GEODA_LD_FLAGS"
fi

make -j$(nproc)
make app

# Create deb (skip for AppImage builds)
if [ "$APPIMAGE_BUILD" != "true" ] ; then
    ./create_deb.sh $OS $VER
fi