#!/bin/sh

# stops the execution of a script if a command or pipeline has an error
set -e

echo "Creating dependencies for AppImage build"
echo $OS
echo $VER
echo $APT

# prepare: BuildTools/ubuntu
cd "$WORK_DIR"
cd BuildTools
cd ubuntu
export GEODA_HOME=$PWD 
mkdir -p libraries
mkdir -p libraries/lib
mkdir -p libraries/include
mkdir -p temp

cd temp

# Install development tools and headers (but not runtime packages for AppImage)
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
elif  [ $OS = 'noble' ] ; then
    $APT install -y libgtk-4-dev libwebkit2gtk-4.1-dev
else
    $APT install -y libwebkitgtk-3.0-dev 
fi

# Install boost 1.75 - build statically
if ! [ -f "boost_1_75_0.tar.bz2" ] ; then
    curl -L -O https://pilotfiber.dl.sourceforge.net/project/boost/boost/1.75.0/boost_1_75_0.tar.bz2
fi
if ! [ -d "boost" ] ; then 
    tar -xf boost_1_75_0.tar.bz2 
    mv boost_1_75_0 boost
fi
cd boost
./bootstrap.sh
# Build boost statically
./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi stage
cd ..

# Install Eigen3
if ! [ -f "eigen-3.3.7.tar.bz2" ] ; then
    curl -L -O https://gitlab.com/libeigen/eigen/-/archive/3.3.7/eigen-3.3.7.tar.bz2
fi
if ! [ -d "eigen3" ] ; then 
    tar -xf eigen-3.3.7.tar.bz2
    mv eigen-3.3.7 eigen3
fi

# Install Spectra
if ! [ -f "v0.8.0.zip" ] ; then
    curl -L -O https://github.com/yixuan/spectra/archive/v0.8.0.zip
fi
if ! [ -d "spectra" ] ; then 
    unzip v0.8.0.zip
    mv spectra-0.8.0 spectra
fi

# Build wxWidgets 3.2.4 - statically linked
if ! [ -f "wxWidgets-3.2.4.tar.bz2" ] ; then
    curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.4/wxWidgets-3.2.4.tar.bz2
fi
if ! [ -d "wxWidgets-3.2.4" ] ; then 
    tar -xf wxWidgets-3.2.4.tar.bz2
fi
cd wxWidgets-3.2.4
chmod +x configure
# Configure for static build
./configure --with-gtk=3 --disable-shared --enable-monolithic --with-opengl --enable-postscript --without-libtiff --disable-debug --enable-webview --prefix=$GEODA_HOME/libraries
make -j$(nproc)
make install
cd ..

# Build CLAPACK for static linking
if ! [ -f "clapack.tgz" ] ; then
    curl -L -O https://s3.us-east-2.amazonaws.com/geodabuild/clapack.tgz || \
    curl -L -O http://www.netlib.org/clapack/clapack-3.2.1-CMAKE.tgz && mv clapack-3.2.1-CMAKE.tgz clapack.tgz
fi
if ! [ -d "CLAPACK-3.2.1" ] ; then 
    tar -xf clapack.tgz
    # Copy the patch files from the dep directory
    cp -rf $GEODA_HOME/dep/CLAPACK-3.2.1/* CLAPACK-3.2.1/ 2>/dev/null || true
fi
cd CLAPACK-3.2.1
if ! [ -f "libf2c.a" ] || ! [ -f "blas.a" ] || ! [ -f "lapack.a" ]; then
    cp make.inc.example make.inc
    make f2clib
    cp F2CLIBS/libf2c.a .
    make blaslib
    cd INSTALL
    make
    cd ..
    cd SRC
    make
    mv -f lapack_LINUX.a lapack.a
    mv -f tmglib_LINUX.a tmglib.a
    cd ..
fi
# The libraries are already in the temp directory and will be used by the makefile
cd ..

# Build JSON Spirit
if ! [ -f "json_spirit_v4.08.zip" ] ; then
    curl -L -O https://s3.us-east-2.amazonaws.com/geodabuild/json_spirit_v4.08.zip || \
    curl -L -O https://github.com/codeproject/json_spirit/archive/refs/heads/master.zip && mv master.zip json_spirit_v4.08.zip
fi
if ! [ -d "json_spirit_v4.08" ] ; then 
    unzip json_spirit_v4.08.zip
    # Handle different directory names based on download source
    if [ -d "json_spirit-master" ] ; then
        mv json_spirit-master json_spirit_v4.08
    fi
fi
cd json_spirit_v4.08
# Use the CMakeLists.txt from GeoDa's dep directory if available
if [ -f "$GEODA_HOME/dep/json_spirit/CMakeLists.txt" ] ; then
    cp $GEODA_HOME/dep/json_spirit/CMakeLists.txt .
fi
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$GEODA_HOME/libraries -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
# Manual install for json_spirit since it may not have proper install target
mkdir -p $GEODA_HOME/libraries/include/json_spirit
mkdir -p $GEODA_HOME/libraries/lib
cp -R ../json_spirit $GEODA_HOME/libraries/include/
cp json_spirit/libjson_spirit.a $GEODA_HOME/libraries/lib/ 2>/dev/null || cp *.a $GEODA_HOME/libraries/lib/ 2>/dev/null || true
cd ../..

cd ..

echo "AppImage dependencies created successfully"