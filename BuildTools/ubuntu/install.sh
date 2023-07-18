#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
chmod +w o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 

# Install libgdal
export DEBIAN_FRONTEND=noninteractive
$APT update -y
# fix curl 60 error
$APT install -y ca-certificates libgnutls30
echo '-k' > ~/.curlrc
$APT install -y libpq-dev
$APT install -y gdal-bin
$APT install -y libgdal-dev
$APT install -y unzip cmake dh-autoreconf libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev 

if  [ $OS = 'jammy' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
elif  [ $OS = 'focal' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
else
    $APT install -y libwebkitgtk-3.0-dev 
fi

# Build GeoDa
cp ../../GeoDamake.$OS.opt ../../GeoDamake.opt
make -j$(nproc)
make app

# Create deb#
./create_deb.sh $OS $VER