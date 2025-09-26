#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

echo "Building GeoDa for AppImage"

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
echo $GEODA_HOME

# Build GeoDa with static linking for AppImage
cp ../../GeoDamake.ubuntu.opt ../../GeoDamake.opt

# Add static linking flags to ensure dependencies are embedded
export EXTRA_GEODA_LD_FLAGS="$EXTRA_GEODA_LD_FLAGS -static-libgcc -static-libstdc++ -Wl,--disable-new-dtags"

echo "Building with EXTRA_GEODA_LD_FLAGS: $EXTRA_GEODA_LD_FLAGS"

make -j$(nproc)
make app

echo "GeoDa build for AppImage completed successfully"

# Verify the executable was created
if [ -f "build/GeoDa" ] ; then
    echo "GeoDa executable created successfully at build/GeoDa"
    # Show what libraries the executable depends on
    echo "Library dependencies:"
    ldd build/GeoDa || echo "Static executable (no dynamic dependencies)"
else
    echo "ERROR: GeoDa executable not found!"
    exit 1
fi