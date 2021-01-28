#!/bin/bash 
#############################################################################
# ./build.sh
# ./build.sh [CPU]
# ./build.sh 8 (with debug)
# ./build.sh [CPU] [NODEBUG, true=1 false=0(default)]
# ./build.sh 8 1 (no debug)
#############################################################################
# Note: mac osx 10.7 autoconf, automake gettext libtool
#
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

if ! type "ar" > /dev/null; then
    echo "You need to install Xcode development enviroment to run this script."
    exit
fi

if ! type "cmake" > /dev/null; then
    echo "You need to install cmake to run this script."
    echo "Please download and install cmake from http://www.cmake.org/files/v2.8/cmake-2.8.12-Darwin64-universal.dmg."
    exit
fi

OSX_VERSION=`sw_vers -productVersion`
TARGET_VERSION="11.1"

#if [[ $OSX_VERSION != $TARGET_VERSION* ]]; then
if ! type "gcc" > /dev/null; then
    echo "If you want to build GeoDa on OSX, please install the \"command-line tools\" package through XCode. Then it will use instaled GNU GCC- and G++."
    read -p "Do you want to continue? [y/n]" -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit
    fi
fi

unset ORACLE_HOME
export GEODA_HOME=$PWD
PREFIX=$GEODA_HOME/libraries
DOWNLOAD_HOME=$GEODA_HOME/temp
echo $PREFIX

MAKER="make -j $CPUS"
if [[ $NODEBUG -eq 0 ]] ; then
    MAKER="make -j $CPUS USER_LOG=-DDEBUG"
fi
GDA_CC="clang"
GDA_CFLAGS="-Os -arch arm64"
GDA_CXX="clang++"
GDA_CXXFLAGS="-Os -arch arm64"
GDA_LDFLAGS="-arch arm64"
GDA_WITH_SYSROOT="/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/"

if ! [ -d $DOWNLOAD_HOME ]; then
    mkdir $DOWNLOAD_HOME
fi

if ! [ -d $PREFIX ]; then
    mkdir $PREFIX
fi

#########################################################################
# install library function
#########################################################################
install_library()
{
    LIB_NAME=$1
    LIB_URL=$2
    LIB_CHECKER=$3
    LIB_FILENAME=$(basename "$LIB_URL" ".tar")
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME

    if ! [ -f "$LIB_FILENAME" ] ; then
        curl -O $LIB_URL
    fi

    if ! [ -d "$LIB_NAME" ] ; then
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
        #./configure --with-sysroot="$GDA_WITH_SYSROOT" CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
        ./configure CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
        $MAKER
        make install
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}
#########################################################################
# install c-ares -- for cURL, prevent crash on Mac oSx with threads
#########################################################################
install_library c-ares-1.10.0 https://github.com/GeoDaCenter/software/releases/download/v2000/c-ares-1.10.0.tar.gz libcares.a

#########################################################################
# install cURL
#########################################################################
LIB_NAME=curl-7.46.0
LIB_CHECKER=libcurl.a
LIB_URL=https://github.com/GeoDaCenter/software/releases/download/v2000/curl-7.46.0.zip
LIB_FILENAME=curl-7.46.0.zip
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    curl -O -L $LIB_URL
    unzip $LIB_FILENAME
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure --with-ssl=/usr/local --disable-ldap --disable-ldaps LDFLAGS="-L/usr/local/lib -L/usr/lib" CPPFLAGS="-I/usr/local/include -mmacosx-version-min=11.1" CFLAGS="-mmacosx-version-min=11.1" --without-brotli --prefix=$PREFIX
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
LIB_NAME=wxWidgets-3.1.4
LIB_URL=https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4.tar.bz2
LIB_FILENAME=$(basename "$LIB_NAME" ".tar.bz2")
LIB_CHECKER=libwx_baseu-3.1.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -f "$LIB_FILENAME.tar.bz2" ] ; then
        curl -L -k -o $LIB_FILENAME $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure CFLAGS="$GDA_CFLAGS" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" OBJCFLAGS="-arch arm64" OBJCXXFLAGS="-arch arm64" --with-cocoa --disable-shared --disable-monolithic --with-opengl --enable-postscript --enable-textfile --without-liblzma --enable-webview --prefix=$PREFIX
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
LIB_URL=https://github.com/GeoDaCenter/software/releases/download/v2000/boost_1_57_0.tar.gz
LIB_FILENAME=$LIB_NAME.tar.gz
LIB_CHECKER=libboost_thread.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -L -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

cd $PREFIX/include
#ln -s $DOWNLOAD_HOME/$LIB_NAME ./boost

if ! [ -f $DOWNLOAD_HOME/$LIB_NAME/stage/lib/$LIB_CHECKER ]; then
    cd $DOWNLOAD_HOME/$LIB_NAME
    #./bootstrap.sh
    #./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi toolset=darwin cxxflags="-arch x86_64" stage
    #./b2 --with-thread --with-date_time --with-chrono --with-system link=static threading=multi toolset=darwin cxxflags="-arch x86_64 -mmacosx-version-min=10.5 -isysroot $GDA_WITH_SYSROOT" macosx-version=10.5 stage
    #bjam toolset=darwin address-model=32

    # 10.5 against 1_50_0
    #./bjam --toolset=darwin --toolset-root=/usr/bin/gcc-4.2 address-model=32 macosx-version=10.5.5
fi

#if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/stage/lib/$LIB_CHECKER" ] ; then
#    echo "Error! Exit"
#    exit
#fi
 
#########################################################################
# install JSON Spirit
#########################################################################
LIB_NAME="json_spirit_v4.08"
LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip"
LIB_CHECKER="libjson_spirit.a"
LIB_FILENAME="json_spirit_v4.08.zip"
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ]; then
    curl -L -O $LIB_URL
    unzip $LIB_FILENAME
fi

cd $DOWNLOAD_HOME/$LIB_NAME

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cp $GEODA_HOME/dep/json_spirit/CMakeLists.txt .
    mkdir bld
    cd bld
    CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake ..
    make
    rm -rf "$PREFIX/include/json_spirit"
    rm -f "$PREFIX/lib/$LIB_CHECKER"
    mkdir "$PREFIX/include/json_spirit"
    echo "Copying JSON Sprit includes..."
    cp -R "../json_spirit" "$PREFIX/include/."
    echo "Copying libjson_spirit.a"
    cp json_spirit/libjson_spirit.a "$PREFIX/lib/."
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

#########################################################################
# MySQL 
#########################################################################
LIB_NAME=mysql-5.6.14
LIB_URL=https://github.com/GeoDaCenter/software/releases/download/v2000/mysql-5.6.14.tar.gz
LIB_CHECKER=libmysqlclient.a
LIB_FILENAME=$LIB_NAME.tar.gz
echo $LIB_FILENAME
cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -L -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

cd $DOWNLOAD_HOME/$LIB_NAME
#if ! [ -f "bld/libmysql/$LIB_CHECKER" ] ; then
    #mkdir bld
    #cd bld
    #CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake .. 
    #make
#fi

#if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/bld/libmysql/$LIB_CHECKER" ] ; then
    #echo "Error! Exit"
    #exit
#fi

#########################################################################
# Eigen3
#########################################################################
LIB_NAME=eigen3
LIB_URL=https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip
LIB_CHECKER=Dense
LIB_FILENAME=$LIB_NAME.zip
echo $LIB_FILENAME
cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -L -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    unzip $LIB_FILENAME
fi

cd $DOWNLOAD_HOME/$LIB_NAME
if ! [ -f "$PREFIX/include/eigen3/Eigen/$LIB_CHECKER" ] ; then
    mkdir bld
    cd bld
    cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX  
    make install
fi

if ! [ -f "$PREFIX/include/eigen3/Eigen/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi


#########################################################################
# install CLAPACK
#########################################################################
LIB_NAME="CLAPACK-3.2.1"
LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/clapack.tgz"
LIB_CHECKER="lapack.a"
LIB_FILENAME=clapack.tgz
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
    curl -L -O $LIB_URL
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

#########################################################################
# install uriparser
#########################################################################
LIB_NAME=uriparser-0.9.4
LIB_CHECKER=liburiparser.a
LIB_URL=https://github.com/uriparser/uriparser/releases/download/uriparser-0.9.4/uriparser-0.9.4.tar.bz2
LIB_FILENAME=uriparser-0.9.4.tar.bz2
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    curl -L -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    mkdir bld
    cd bld
    CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release -DURIPARSER_BUILD_TESTS=OFF -DURIPARSER_BUILD_DOCS=OFF ..
    make
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

#########################################################################
# install minizip/zlib
#########################################################################

LIB_NAME=zlib
LIB_CHECKER=libz.a
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    git clone https://github.com/madler/zlib.git
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    mkdir bld
    cd bld
    CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release ..
    make
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
#########################################################################
# install libkml requires 1.3
#########################################################################
LIB_NAME=libkml-1.3.0
LIB_CHECKER=libkmlbase.a
LIB_URL=https://github.com/libkml/libkml/archive/1.3.0.tar.gz
LIB_FILENAME=1.3.0.tar.gz
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    curl -L -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

#if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
#    cd $LIB_NAME
#    mkdir bld
#    cd bld
#    CC=$GDA_CC CXX=$GDA_CXX CFLAGS="$GDA_CFLAGS -I/Users/xun/github/geoda/BuildTools/macosx/libraries/include" CXXFLAGS="$GDA_CXXFLAGS -I/Users/xun/github/geoda/BuildTools/macosx/libraries/include" LDFLAGS=$GDA_LDFLAGS cmake -DCMAKE_CXX_FLAGS=-I/Users/xun/github/geoda/BuildTools/macosx/libraries/include -DMINIZIP_INCLUDE_DIR=$PREFIX/include -DBOOST_INCLUDEDIR=$DOWNLOAD_HOME/boost_1_57_0 -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release ..
#    make
#fi

#if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
#    echo "Error! Exit"
#    exit
#fi

#########################################################################
# install PostgreSQL
#########################################################################
install_library postgresql-13.1 https://ftp.postgresql.org/pub/source/v13.1/postgresql-13.1.tar.bz2 libpq.a

#########################################################################
# install FreeXL
#########################################################################
install_library freexl-1.0.6 http://www.gaia-gis.it/gaia-sins/freexl-1.0.6.tar.gz libfreexl.a

#########################################################################
# install SQLite
#########################################################################
install_library sqlite-autoconf-3340000 https://www.sqlite.org/2020/sqlite-autoconf-3340000.tar.gz libsqlite3.a

#########################################################################
# install GEOS
#########################################################################
install_library geos-3.9.0 http://download.osgeo.org/geos/geos-3.9.0.tar.bz2 libgeos.a

# see https://github.com/libgeos/geos/pull/365

#########################################################################
# install libTIFF for proj
#########################################################################
install_library tiff-4.2.0 http://download.osgeo.org/libtiff/tiff-4.2.0.tar.gz libtiff.a

#########################################################################
# install PROJ.4
#########################################################################
export SQLITE3_CFLAGS=-I$PREFIX/include
export SQLITE3_LIBS=$PREFIX/lib/libsqlite3.a
export TIFF_CFLAGS=-I$PREFIX/include
export TIFF_LIBS="$PREFIX/lib/libtiff.a -lz"
export CXXFLAGS="-arch arm64"
export CPPFLAGS="-arch arm64"
export CFLAGS="-arch arm64"
export LDFLAGS="-arch arm64"
install_library proj-7.2.1 https://download.osgeo.org/proj/proj-7.2.1.tar.gz libproj.a


#########################################################################
# install libXML2
#########################################################################

LIB_NAME=liblzma
LIB_CHECKER=liblzma.a
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    git clone https://github.com/kobolabs/liblzma.git
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
    $MAKER
    make install
fi


LIB_NAME=libxml2
LIB_CHECKER=libxml2.dylib
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    git clone https://gitlab.gnome.org/GNOME/libxml2.git
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    mkdir bld
    cd bld
    CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake -DBUILD_SHARED_LIBS=OFF -DLIBXML2_WITH_DEBUG=OFF -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release ..
    make
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

#########################################################################
# install libzstd
#########################################################################

LIB_NAME=zstd
LIB_CHECKER=libzstd.a
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    git clone https://github.com/facebook/zstd.git
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
    $MAKER
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi


#########################################################################
# install SpatiaLite
#########################################################################
LIB_NAME=libspatialite-5.0.0
LIB_URL=http://www.gaia-gis.it/gaia-sins/libspatialite-4.2.1.tar.gz
LIB_FILENAME=$LIB_NAME.tar.gz
LIB_CHECKER=libspatialite.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -O $LIB_URL
fi

if ! [ -d "$LIB_NAME" ]; then
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    export LIBS="-lz -lxml2"
    export LIBXML2_CFLAGS="-I$PREFIX/include"
    export LIBXML2_LIBS="-L$PREFIX/lib -lxml2"
    ./configure CFLAGS="$GDA_CFLAGS -I$PREFIX/include" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS -I$PREFIX/include" LDFLAGS="$GDA_LDFLAGS -L$PREFIX/lib -L/usr/lib -liconv" --prefix=$PREFIX --enable-geos --with-geosconfig=$PREFIX/bin/geos-config --enable-static=no
    $MAKER
    #touch src/.libs/libspatialite.lai
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
exit

#########################################################################
# install libjpeg libpng
#########################################################################
install_library jpeg-8 https://github.com/GeoDaCenter/software/releases/download/v2000/jpegsrc.v8.tar.gz libjpeg.a

LIB_NAME=libpng-1.6.37
LIB_CHECKER=libpng.a
echo $LIB_NAME

cd $DOWNLOAD_HOME

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX --enable-arm-neon
    $MAKER
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
#########################################################################
# install GDAL/OGR
#########################################################################
LIB_NAME=gdal-3.2.1
LIB_URL=https://github.com/OSGeo/gdal/releases/download/v3.2.1/gdal-3.2.1.tar.gz
LIB_FILENAME=gdal
LIB_CHECKER=libgdal.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ]; then
    curl -k -O $LIB_URL
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    export PQ_CFLAGS=-I$PREFIX/include
    export PQ_LIBS=$PREFIX/lib/libpq.a
    ./configure CC="$GDA_CC" CXX="$GDA_CXX" CFLAGS="$GDA_CFLAGS" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --with-jpeg=no --prefix=$PREFIX --with-freexl=$PREFIX \
    --with-libiconv-prefix="-L/usr/lib" --with-sqlite3=$PREFIX \
    --with-spatialite=$PREFIX --with-spatialite-soname=libspatialite.dylib \
    --with-proj=$PREFIX --with-curl=$PREFIX/bin/curl-config --with-geos=$PREFIX/bin/geos-config \
    --with-xerces=$PREFIX --with-xerces-inc="$PREFIX/include" \
    --with-xerces-lib="-L$PREFIX/lib -lxerces-c -framework CoreServices" \
    --with-libtiff=$PREFIX --with-geotiff=internal \
    --disable-driver-gpsbabel --disable-driver-edigeo --disable-driver-flatgeobuf \
    --disable-driver-aeronavfaa --disable-driver-ntf --without-geotiff --disable-driver-pds \
    --without-lerc --disable-driver-mrf --with-png=no --disable-driver-grib --disable-driver-mrf
    $MAKER
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    echo "You need to modify the libgdal.la and remove the extra '=' symobls."
    exit
fi

exit

#########################################################################
# build GeoDa
#########################################################################
cd $GEODA_HOME
cp ../../GeoDamake.macosx.opt ../../GeoDamake.opt
mkdir ../../o
$MAKER
if [ -d "build" ] ; then
    rm -rf build
fi
make app
