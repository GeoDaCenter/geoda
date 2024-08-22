#!/bin/zsh

# stops the execution of a script if a command or pipeline has an error
set -e

export GEODA_HOME=$PWD
echo $GEODA_HOME
echo $GEODA_ARCH
# check if $GEODA_HOME and $GEODA_ARCH are set
if [ -z "$GEODA_HOME" ]; then
    echo "Please set GEODA_HOME environment variable"
    exit
fi
if [ -z "$GEODA_ARCH" ]; then
    echo "Please set GEODA_ARCH environment variable"
    exit
fi

CPUS=`sysctl -n hw.ncpu`

cd $GEODA_HOME
mkdir -p temp
mkdir -p libraries
mkdir -p libraries/lib
mkdir -p libraries/include
mkdir -p ../../o

cd temp

# Build CLAPACK
if ! [ -f "clapack.tgz" ]; then
    curl -L -O https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz
    tar -xf clapack.tgz
fi
if ! [ -f "CLAPACK-3.2.1/libf2c.a" ]; then
    cp -rf ../dep/CLAPACK-3.2.1 .
    cd CLAPACK-3.2.1
    make f2clib
    make blaslib
    cd INSTALL
    make
    cd ..
    cd SRC
    make
    cd ..
    cp F2CLIBS/libf2c.a .
    cd ..
fi

# Build Boost 1.76
if ! [ -f "boost_1_76_0.tar.bz2" ]; then
    curl -L -O https://archives.boost.io/release/1.76.0/source/boost_1_76_0.tar.gz
    tar -xf boost_1_76_0.tar.gz
fi
if ! [ -f "../libraries/lib/libboost_thread.a" ]; then
    cd boost_1_76_0
    ./bootstrap.sh
    ./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi stage
    cp -R stage/lib/* ../../libraries/lib/.
    cp -R boost ../../libraries/include/.
    cd ..
fi

# Build wxWidgets 3.2.4
if ! [ -f "wxWidgets-3.2.4.tar.bz2" ]; then
    curl -L -O https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.4/wxWidgets-3.2.4.tar.bz2
    tar -xf wxWidgets-3.2.4.tar.bz2
fi
if ! [ -f "../libraries/bin/wx-config" ]; then
    cd wxWidgets-3.2.4
    ./configure --with-cocoa --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --enable-cxx11 --disable-mediactrl --enable-webviewwebkit --enable-monolithic --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --prefix=$GEODA_HOME/libraries
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
    cmake -DBoost_NO_BOOST_CMAKE=TRUE -DBOOST_ROOT:PATHNAME=$GEODA_HOME/libraries/include ..
    make -j $CPUS
    cp -R ../json_spirit ../../../libraries/include/.
    cp json_spirit/libjson_spirit.a ../../../libraries/lib/.
    cd ../..
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

cd ..