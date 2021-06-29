#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

export GEODA_HOME=$PWD
echo $GEODA_HOME
CPUS=2

mkdir -p temp
mkdir -p libraries
mkdir -p libraries/lib
mkdir -p libraries/include
mkdir -p ../../o


# Install boost 1.75
brew install boost

# Install libgdal-dev
brew install gdal

# Build wxWidgets 3.1.4
cd temp
if ! [ -f "wxWidgets-3.1.4.tar.bz2" ]; then
    curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4.tar.bz2
    tar -xf wxWidgets-3.1.4.tar.bz2
fi
if ! [ -f "../libraries/bin/wx-config" ]; then
    cd wxWidgets-3.1.4
    ./configure --with-cocoa --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --enable-cxx11 --enable-webview --disable-mediactrl --enable-webviewwebkit --enable-monolithic --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --prefix=$GEODA_HOME/libraries
    make -j $CPUS
    make install
    cd ..
fi

# Build JSON Spirit v4.08
if ! [ -f "json_spirit_v4.08.zip" ]; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
    unzip json_spirit_v4.08.zip
fi
if ! [ -f "../libraries/lib/libjson_spirit.a" ]; then
    cd json_spirit_v4.08
    cp ../../dep/json_spirit/CMakeLists.txt .
    mkdir -p bld
    cd bld
    cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBOOST_ROOT:PATHNAME=/usr/local/opt ..
    make -j $CPUS
    cp -R ../json_spirit ../../../libraries/include/.
    cp json_spirit/libjson_spirit.a ../../../libraries/lib/.
    cd ../..
fi

# Build CLAPACK
if ! [ -f "clapack.tgz" ]; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
    tar -xf clapack.tgz
fi
if ! [ -f "CLAPACK-3.2.1/libf2c.a" ]; then
    cp -rf ../dep/CLAPACK-3.2.1 .
    cd CLAPACK-3.2.1
    make -j $CPUS f2clib
    make -j $CPUS blaslib
    cd INSTALL
    make -j $CPUS
    cd ..
    cd SRC
    make -j $CPUS
    cd ..
    cp F2CLIBS/libf2c.a .
    cd ..
fi

# Build Eigen3 and Spectra
if ! [ -f "eigen3.zip" ]; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip
    unzip eigen3.zip
fi
if ! [ -f "v0.8.0.zip" ]; then
    curl -L -O https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip
    unzip v0.8.0.zip
    mv spectra-0.8.0 spectra
fi

# Build GeoDa
cd ..
make -j $CPUS
make app

# Create dmg
VER_MAJOR=$(grep version_major $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
VER_MINOR=$(grep version_minor $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
VER_BUILD=$(grep version_build $GEODA_HOME/../../version.h | sed -e 's/^[[:space:]][[:alpha:]|[:space:]|_|=]*//g' | sed -e 's/;//g')
GEODA_VERSION=$VER_MAJOR.$VER_MINOR.$VER_BUILD
echo $GEODA_VERSION

cd create-dmg
./geoda.sh $GEODA_VERSION
codesign --timestamp -s "Developer ID Application: Geodapress LLC (26M5NG43GP)" -i edu.uchicago.spatial GeoDa$GEODA_VERSION-Installer.dmg
