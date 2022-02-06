#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

echo $OS
echo $VER
echo $APT

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
mkdir -p o
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
mkdir -p libraries
mkdir -p libraries/lib
mkdir -p libraries/include
mkdir -p temp

cd temp

# Install libgdal
export DEBIAN_FRONTEND=noninteractive
$APT update -y
# fix curl 60 error
$APT install -y ca-certificates libgnutls30
$APT install -y libpq-dev
$APT install -y gdal-bin
$APT install -y libgdal-dev
$APT install -y unzip cmake dh-autoreconf libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev 

if  [ $OS = 'disco' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
elif  [ $OS = 'focal' ] ; then
    $APT install -y libwebkit2gtk-4.0-dev
else
    $APT install -y libwebkitgtk-3.0-dev 
fi

# Install boost 1.75
if ! [ -f "boost_1_75_0.tar.bz2" ] ; then
    curl -L -O https://pilotfiber.dl.sourceforge.net/project/boost/boost/1.75.0/boost_1_75_0.tar.bz2
fi
if ! [ -d "boost" ] ; then 
    tar -xf boost_1_75_0.tar.bz2 
    mv boost_1_75_0 boost
fi
cd boost
./bootstrap.sh
./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi stage
cd ..

# Build JSON Spirit v4.08
if ! [ -f "json_spirit_v4.08.zip" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
fi
if ! [ -d "json_spirit_v4.08" ] ; then 
    unzip json_spirit_v4.08.zip
fi
cd json_spirit_v4.08
cp ../../dep/json_spirit/CMakeLists.txt .
mkdir -p bld
cd bld
cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBOOST_ROOT:PATHNAME=$GEODA_HOME/temp/boost  ..
make -j2
cp -R ../json_spirit ../../../libraries/include/.
cp json_spirit/libjson_spirit.a ../../../libraries/lib/.
cd ..
cd ..

# Build CLAPACK
if ! [ -f "clapack.tgz" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
fi
if ! [ -d "CLAPACK-3.2.1" ] ; then 
    tar -xf clapack.tgz
    cp -rf ../dep/CLAPACK-3.2.1 .
fi
cd CLAPACK-3.2.1
make -j2 f2clib
make -j2 blaslib
cd INSTALL
make -j2
cd ..
cd SRC
make -j2
cd ..
cp F2CLIBS/libf2c.a .
cd ..

# Build Eigen3 and Spectra
if ! [ -f "eigen3.zip" ] ; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip
    unzip eigen3.zip
fi
if ! [ -f "v0.8.0.zip" ] ; then
    curl -L -O https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip
    unzip v0.8.0.zip
    mv spectra-0.8.0 spectra
fi

# Build wxWidgets 3.1.4
if ! [ -f "wxWidgets-3.1.4.tar.bz2" ] ; then
    curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4.tar.bz2
fi
if ! [ -d "wxWidgets-3.1.4" ] ; then 
    tar -xf wxWidgets-3.1.4.tar.bz2
fi
cd wxWidgets-3.1.4
chmod +x configure
./configure --with-gtk=3 --disable-shared --enable-monolithic --with-opengl --enable-postscript --without-libtiff --disable-debug --enable-webview --prefix=$GEODA_HOME/libraries
make -j2
make install
cd ..
cd ..

# Build GeoDa
cp ../../GeoDamake.$OS.opt ../../GeoDamake.opt
make -j2
make app

# Create deb#
./create_deb.sh $OS $VER