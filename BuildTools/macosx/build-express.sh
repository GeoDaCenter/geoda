#!/bin/bash 
#############################################################################
# ./build.sh
# ./build.sh [CPU]
# ./build.sh 8 (with debug)
# ./build.sh [CPU] [NODEBUG, true=1 false=0(default)]
# ./build.sh 8 1 (no debug)
#############################################################################
CPUS=$1
NODEBUG=$2
if [[ $CPUS == "" ]] ; then
    CPUS=8
fi
if [[ $NODEBUG == "" ]] ; then
    NODEBUG=0
else
    if ! [[ $NODEBUG -eq 1 ]] ; then
        NODEBUG=0
    fi
fi


if ! type "gcc" > /dev/null; then
    echo "If you want to build GeoDa on OSX > 10.6, please install the \"command-line tools\" package through XCode. Then it will use instaled GNU GCC and G++"
    read -p "Do you want to continue? [y/n]" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit
    fi
    xcode-select --install
fi

unset ORACLE_HOME
export GEODA_HOME=$PWD
export MC_HOME="/Users/xun/geoda_trunk/BuildTools/macosx"
PREFIX=$GEODA_HOME/libraries
DOWNLOAD_HOME=$GEODA_HOME/temp
echo $PREFIX

MAKER="make -j $CPUS"
GDA_CC="gcc"
GDA_CFLAGS="-Os -arch x86_64 -arch i386"
GDA_CXX="g++"
GDA_CXXFLAGS="-Os -arch x86_64 -arch i386"
GDA_LDFLAGS="-arch x86_64 -arch i386"
GDA_WITH_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/"

if ! [ -d $DOWNLOAD_HOME ]; then
    mkdir $DOWNLOAD_HOME
fi

if ! [ -d $PREFIX ]; then
    mkdir $PREFIX
fi

#########################################################################
# copy library/*
#########################################################################
if ! [ -f "$PREFIX/lib/libjson_spirit.a" ] ; then
    LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/libraries.zip
    curl -O $LIB_URL
    unzip libraries.zip
    rm libraries.zip
    cd libraries/lib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libgdal.20.dylib" libgdal.20.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlengine.0.dylib" libkmlengine.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlregionator.0.dylib" libkmlregionator.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlxsd.0.dylib" libkmlxsd.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmldom.0.dylib" libkmldom.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlbase.0.dylib" libkmlbase.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlconvenience.0.dylib" libkmlconvenience.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libkmlengine.0.dylib" libkmlengine.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libminizip.0.dylib" libminizip.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/liburiparser.1.dylib" liburiparser.1.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libxerces-c-3.1.dylib" libxerces-c-3.1.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libpq.5.dylib" libpq.5.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libspatialite.5.dylib" libspatialite.5.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libfreexl.1.dylib" libfreexl.1.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libproj.0.dylib" libproj.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libsqlite3.0.dylib" libsqlite3.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libgeos_c.1.dylib" libgeos_c.1.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libgeos-3.3.8.dylib" libgeos-3.3.8.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libcurl.4.dylib" libcurl.4.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libproj.0.dylib" libproj.0.dylib
    install_name_tool -id "$GEODA_HOME/libraries/lib/libcares.2.dylib" libcares.2.dylib
    #
    install_name_tool -change "$MC_HOME/libraries/lib/libgdal.20.dylib" "$GEODA_HOME/libraries/lib/libgdal.20.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libcares.2.dylib" "$GEODA_HOME/libraries/lib/libcares.2.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlengine.0.dylib" "$GEODA_HOME/libraries/lib/libkmlengine.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlregionator.0.dylib" "$GEODA_HOME/libraries/lib/libkmlregionator.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlxsd.0.dylib" "$GEODA_HOME/libraries/lib/libkmlxsd.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmldom.0.dylib" "$GEODA_HOME/libraries/lib/libkmldom.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlbase.0.dylib" "$GEODA_HOME/libraries/lib/libkmlbase.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlconvenience.0.dylib" "$GEODA_HOME/libraries/lib/libkmlconvenience.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libminizip.0.dylib" "$GEODA_HOME/libraries/lib/libminizip.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/liburiparser.1.dylib" "$GEODA_HOME/libraries/lib/liburiparser.1.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libxerces-c-3.1.dylib" "$GEODA_HOME/libraries/lib/libxerces-c-3.1.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libpq.5.dylib" "$GEODA_HOME/libraries/lib/libpq.5.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libspatialite.5.dylib" "$GEODA_HOME/libraries/lib/libspatialite.5.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libfreexl.1.dylib" "$GEODA_HOME/libraries/lib/libfreexl.1.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libproj.0.dylib" "$GEODA_HOME/libraries/lib/libproj.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libsqlite3.0.dylib" "$GEODA_HOME/libraries/lib/libsqlite3.0.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libgeos_c.1.dylib" "$GEODA_HOME/libraries/lib/libgeos_c.1.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libgeos-3.3.8.dylib" "$GEODA_HOME/libraries/lib/libgeos-3.3.8.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libcurl.4.dylib" "$GEODA_HOME/libraries/lib/libcurl.4.dylib" libgdal.20.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlengine.0.dylib" "$GEODA_HOME/libraries/lib/libkmlengine.0.dylib" libkmlengine.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libkmlbase.0.dylib" "$GEODA_HOME/libraries/lib/libkmlbase.0.dylib" libkmlengine.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libminizip.0.dylib" "$GEODA_HOME/libraries/lib/libminizip.0.dylib" libkmlengine.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/liburiparser.1.dylib" "$GEODA_HOME/libraries/lib/liburiparser.1.dylib" libkmlengine.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libminizip.0.dylib" "$GEODA_HOME/libraries/lib/libminizip.0.dylib" libkmlbase.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/liburiparser.1.dylib" "$GEODA_HOME/libraries/lib/liburiparser.1.dylib" libkmlbase.0.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libfreexl.1.dylib" "$GEODA_HOME/libraries/lib/libfreexl.1.dylib" libspatialite.5.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libproj.0.dylib" "$GEODA_HOME/libraries/lib/libproj.0.dylib" libspatialite.5.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libsqlite3.0.dylib" "$GEODA_HOME/libraries/lib/libsqlite3.0.dylib" libspatialite.5.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libgeos_c.1.dylib" "$GEODA_HOME/libraries/lib/libgeos_c.1.dylib" libspatialite.5.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libgeos-3.3.8.dylib" "$GEODA_HOME/libraries/lib/libgeos-3.3.8.dylib" libgeos_c.1.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libgeos-3.3.8.dylib" "$GEODA_HOME/libraries/lib/libgeos-3.3.8.dylib" libspatialite.5.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libcurl.4.dylib" "$GEODA_HOME/libraries/lib/libcurl.4.dylib" libxerces-c-3.1.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libcares.2.dylib" "$GEODA_HOME/libraries/lib/libcares.2.dylib" libxerces-c-3.1.dylib
    install_name_tool -change "$MC_HOME/libraries/lib/libcares.2.dylib" "$GEODA_HOME/libraries/lib/libcares.2.dylib" libcurl.4.dylib
    cd ../..
fi

#########################################################################
# install cURL
#########################################################################
LIB_NAME=curl-7.46.0
LIB_CHECKER=libcurl.a
LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/curl-7.46.0.zip
LIB_FILENAME=curl-7.46.0.zip
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    curl -O $LIB_URL
    unzip $LIB_FILENAME
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure --enable-ares=$PREFIX CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
    $MAKER
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
export PATH=$PREFIX/bin:$PATH


#########################################################################
# install wxWidgets library
#########################################################################
LIB_NAME=wxWidgets-3.1.0
LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/wxWidgets-3.1.0.tar.bz2
LIB_FILENAME=$(basename "$LIB_URL" ".tar")
LIB_CHECKER=libwx_baseu-3.1.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -f "$LIB_FILENAME" ] ; then
        curl -k -o $LIB_FILENAME $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    cp -rf $GEODA_HOME/dep/$LIB_NAME/* .
    make clean
    ./configure --with-cocoa --disable-shared --enable-mediactrl=no --disable-monolithic --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --enable-compat28 --disable-mediactrl -prefix=$PREFIX
    #./configure CFLAGS="$GDA_CFLAGS" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" OBJCFLAGS="-arch x86_64" OBJCXXFLAGS="-arch x86_64" --with-cocoa --disable-shared --disable-monolithic --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --enable-compat28 --prefix=$PREFIX
    $MAKER 
    make install
    cd ..
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

#########################################################################
# install boost library
#########################################################################
LIB_NAME=boost_1_57_0
LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/boost_1_57_0.tar.gz
LIB_FILENAME=$LIB_NAME.tar.gz
LIB_CHECKER=libboost_thread.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

cd $PREFIX/include
rm boost
ln -s $DOWNLOAD_HOME/$LIB_NAME ./boost

if ! [ -f $DOWNLOAD_HOME/$LIB_NAME/stage/lib/$LIB_CHECKER ]; then
    cd $DOWNLOAD_HOME/$LIB_NAME
    ./bootstrap.sh
    ./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi toolset=darwin cxxflags="-arch x86_64" stage
    #./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi toolset=darwin cxxflags="-arch x86_64 -mmacosx-version-min=10.5 -isysroot /Developer/SDKs/MacOSX10.5.sdk" macosx-version=10.5 stage
    #bjam toolset=darwin address-model=32

    # 10.5 against 1_50_0
    #./bjam --toolset=darwin --toolset-root=/usr/bin/gcc-4.2 address-model=32 macosx-version=10.5.5
fi

if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/stage/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi


#########################################################################
# install CLAPACK
#########################################################################
LIB_NAME="CLAPACK-3.2.1"
LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/clapack.tgz"
LIB_CHECKER="lapack.a"
LIB_FILENAME=clapack.tgz
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
    curl -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

cp -rf $GEODA_HOME/dep/$LIB_NAME .
cd $LIB_NAME
if ! [ -f "libf2c.a" ] || ! [ -f "blas.a" ] || ! [ -f "lapack.a" ]; then
    $MAKER f2clib
    $MAKER blaslib
    cd INSTALL
    $MAKER 
    cd ..
    cd SRC
    $MAKER 
    cd ..
    cp F2CLIBS/libf2c.a .
    #make  
    cd ..
fi

if ! [ -f "$DOWNLOAD_HOME/$LIB_NAME/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

cp ../../GeoDamake.macosx.opt ../../GeoDamake.opt
mkdir ../../o
echo "You can use Xcode to do dev work."
echo "In Xcode, add libc++.dylib to your project:"
echo "Go to Build Phases"
echo "Expand: Link Binary With Libraries"
echo "Click the +"
echo "Select the libc++.dylib file and press Add"

