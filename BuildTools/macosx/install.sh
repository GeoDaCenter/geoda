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
brew install boost@1.75

# Build wxWidgets 3.1.4
cd temp
curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4.tar.bz2
tar -xf wxWidgets-3.1.4.tar.bz2
cd wxWidgets-3.1.4
./configure --with-cocoa --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --enable-cxx11 --enable-webview --disable-mediactrl --enable-webviewwebkit --enable-monolithic --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --prefix=$GEODA_HOME/libraries
make -j $CPUS
make install
cd ..

# Build JSON Spirit v4.08
curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
unzip json_spirit_v4.08.zip
cd json_spirit_v4.08
cp ../../dep/json_spirit/CMakeLists.txt .
mkdir bld
cd bld
cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBOOST_ROOT:PATHNAME=/usr/local/opt ..
make -j $CPUS
cp -R ../json_spirit ../../../libraries/include/.
cp json_spirit/libjson_spirit.a ../../../libraries/lib/.
cd ../..

# Build CLAPACK
curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
tar -xf clapack.tgz
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

# Build Eigen3 and Spectra
curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip
unzip eigen3.zip
curl -L -O https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip
unzip v0.8.0.zip
mv spectra-0.8.0 spectra

# Install libgdal-dev
brew install gdal

# Build GeoDa
cp ../../GeoDamake.macosx.opt ../../GeoDamake.opt
make -j $CPUS
make app
