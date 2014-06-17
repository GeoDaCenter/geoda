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
TARGET_VERSION="10.6"

#if [[ $OSX_VERSION != $TARGET_VERSION* ]]; then
if ! type "gcc-4.2" > /dev/null; then
    echo "If you want to build GeoDa on OSX > 10.6, please install the \"command-line tools\" package through XCode. Then it will use instaled GNU GCC-4.2 and G++-4.2."
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
GDA_CC="gcc-4.2"
GDA_CFLAGS="-Os -arch x86_64"
GDA_CXX="g++-4.2"
GDA_CXXFLAGS="-Os -arch x86_64"
GDA_LDFLAGS="-arch x86_64"
GDA_WITH_SYSROOT="/Developer/SDKs/MacOSX10.6.sdk/"

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
    if ! [ -d "$LIB_NAME" ] ; then
        curl -O $LIB_URL
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
        ./configure --with-sysroot="$GDA_WITH_SYSROOT" CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX
        $MAKER
        make install
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}
#########################################################################
# install cURL
#########################################################################
install_library curl-7.30.0 http://curl.haxx.se/download/curl-7.30.0.tar.gz libcurl.a

export PATH=$PREFIX/bin:$PATH

#########################################################################
# install Xerces
#########################################################################
XERCES_NAME="xerces-c-3.1.1"
XERCES_URL="http://mirror.metrocast.net/apache//xerces/c/3/sources/xerces-c-3.1.1.tar.gz"
install_library $XERCES_NAME $XERCES_URL libxerces-c.a

#########################################################################
# install GEOS
#########################################################################
install_library geos-3.3.8 http://download.osgeo.org/geos/geos-3.3.8.tar.bz2 libgeos.a

#########################################################################
# install PROJ.4
#########################################################################
install_library proj-4.8.0 http://download.osgeo.org/proj/proj-4.8.0.tar.gz libproj.a

#########################################################################
# install FreeXL
#########################################################################
install_library freexl-1.0.0f http://www.gaia-gis.it/gaia-sins/freexl-sources/freexl-1.0.0f.tar.gz libfreexl.a

#########################################################################
# install SQLite
#########################################################################
install_library sqlite-autoconf-3071602 http://www.sqlite.org/2013/sqlite-autoconf-3071602.tar.gz libsqlite3.a

#########################################################################
# install PostgreSQL
#########################################################################
install_library postgresql-9.2.4 http://ftp.postgresql.org/pub/source/v9.2.4/postgresql-9.2.4.tar.bz2 libpq.a

#########################################################################
# install libjpeg
#########################################################################
install_library jpeg-8 http://www.ijg.org/files/jpegsrc.v8.tar.gz libjpeg.a

#########################################################################
# install libkml requires 1.3
#########################################################################
LIB_NAME=libkml
LIB_CHECKER=libkmlbase.a
echo $LIB_NAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ] ; then
    svn checkout http://libkml.googlecode.com/svn/trunk/ libkml
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    cp $GEODA_HOME/dep/libkml/configure.ac .
    ./autogen.sh
    ./configure CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX    
    #if [[ $OSX_VERSION != $TARGET_VERSION* ]]; then
        sed -i.old "/gtest-death-test.Plo/d" third_party/Makefile
        sed -i.old "/gtest-filepath.Plo/d" third_party/Makefile
        sed -i.old "/gtest-port.Plo/d" third_party/Makefile
        sed -i.old "/gtest-test-part.Plo/d" third_party/Makefile
        sed -i.old "/gtest-typed-test.Plo/d" third_party/Makefile
        sed -i.old "/gtest-test.Plo/d" third_party/Makefile
        sed -i.old "/gtest.Plo/d" third_party/Makefile
        sed -i.old "/gtest_main.Plo/d" third_party/Makefile
        sed -i.old "/UriCommon.Plo/d" third_party/Makefile
        sed -i.old "/UriCompare.Plo/d" third_party/Makefile
        sed -i.old "/UriEscape.Plo/d" third_party/Makefile
        sed -i.old "/UriFile.Plo/d" third_party/Makefile
        sed -i.old "/UriIp4.Plo/d" third_party/Makefile
        sed -i.old "/UriIp4Base.Plo/d" third_party/Makefile
        sed -i.old "/UriNormalize.Plo/d" third_party/Makefile
        sed -i.old "/UriNormalizeBase.Plo/d" third_party/Makefile
        sed -i.old "/UriParse.Plo/d" third_party/Makefile
        sed -i.old "/UriParseBase.Plo/d" third_party/Makefile
        sed -i.old "/UriQuery.Plo/d" third_party/Makefile
        sed -i.old "/UriRecompose.Plo/d" third_party/Makefile
        sed -i.old "/UriResolve.Plo/d" third_party/Makefile
        sed -i.old "/UriShorten.Plo/d" third_party/Makefile
        sed -i.old "/ioapi.Plo/d" third_party/Makefile
        sed -i.old "/iomem_simple.Plo/d" third_party/Makefile
        sed -i.old "/zip.Plo/d" third_party/Makefile
    #fi
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
LIB_NAME=libspatialite-4.0.0
LIB_URL=http://www.gaia-gis.it/gaia-sins/libspatialite-sources/libspatialite-4.0.0.tar.gz
LIB_FILENAME=$(basename "$LIB_URL" ".tar")
LIB_CHECKER=libspatialite.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ] ; then
    curl -O $LIB_URL
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    ./configure --enable-iconv CC="$GDA_CC" CFLAGS="$GDA_CFLAGS -I$PREFIX/include" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS -I$PREFIX/include" LDFLAGS="$GDA_LDFLAGS -L$PREFIX/lib -L/usr/lib -liconv" --prefix=$PREFIX --enable-geos --with-geosconfig=$PREFIX/bin/geos-config
    $MAKER
    touch src/.libs/libspatialite.lai
    make install
fi
# in some case, the make install doens't work because of .la file content error,
# so copy the compiled files manually
cd $LIB_NAME
cp src/.libs/libspatialite.* $PREFIX/lib/

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
#########################################################################
# MySQL 
#########################################################################
LIB_NAME=mysql-5.6.14
LIB_URL=http://softlayer-dal.dl.sourceforge.net/project/mysql.mirror/MySQL%205.6.14/mysql-5.6.14.tar.gz
# LIB_URL=http://cdn.mysql.com/Downloads/MySQL-5.6/mysql-5.6.14.tar.gz
LIB_CHECKER=libmysqlclient.a
echo $LIB_NAME
cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ] ; then
    curl -O $LIB_URL
    tar -xf $LIB_NAME.tar.gz
fi

cd $DOWNLOAD_HOME/$LIB_NAME
if ! [ -f "bld/libmysql/$LIB_CHECKER" ] ; then
    mkdir bld
    cd bld
    CC=$GDA_CC CXX=$GDA_CXX CFLAGS=$GDA_CFLAGS CXXFLAGS=$GDA_CXXFLAGS LDFLAGS=$GDA_LDFLAGS cmake .. 
    make
fi

if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/bld/libmysql/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
#########################################################################
# install boost library
#########################################################################
LIB_NAME=boost_1_54_0
#LIB_URL=http://superb-dca2.dl.sourceforge.net/project/boost/boost/1.54.0/boost_1_54_0.tar.bz2
LIB_URL=http://softlayer-dal.dl.sourceforge.net/project/boost/boost/1.54.0/boost_1_54_0.tar.bz2
LIB_FILENAME=$(basename "$LIB_URL" ".tar")
LIB_CHECKER=libboost_thread.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ]; then
    curl -O $LIB_URL
    tar -xf $LIB_FILENAME
fi

cd $PREFIX/include
if ! [ -d "boost" ]; then
    ln -s $DOWNLOAD_HOME/$LIB_NAME ./boost
fi

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
LIB_URL="http://www.netlib.org/clapack/clapack.tgz"
LIB_CHECKER="lapack.a"
echo $LIB_NAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ]; then
    curl -O $LIB_URL
    tar -xvf clapack.tgz
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
# install GDAL/OGR
#########################################################################
LIB_NAME=gdal-1.9.2
LIB_URL=https://codeload.github.com/lixun910/gdal-1.9.2-work/zip/master
LIB_FILENAME=master
#LIB_NAME=gdal-1.10.1
#LIB_URL=http://download.osgeo.org/gdal/1.10.1/gdal-1.10.1.tar.gz
#LIB_FILENAME=gdal-1.10.1.tar.gz
LIB_CHECKER=libgdal.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ] ; then
    #svn co https://github.com/lixun910/gdal-1.9.2-work/trunk gdal-1.9.2
    curl -O $LIB_URL
    tar -xvf $LIB_FILENAME
    mv gdal-1.9.2-work-master gdal-1.9.2
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    if [[ $NODEBUG -eq 1 ]] ; then
        # no debug
    	./configure CC="$GDA_CC" CXX="$GDA_CXX" CFLAGS="$GDA_CFLAGS" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --with-jpeg=internal --prefix=$PREFIX --with-freexl=$PREFIX --with-libiconv-prefix="-L/usr/lib" --with-sqlite3=$PREFIX --with-spatialite=$PREFIX --with-static-proj4=$PREFIX --with-curl=$PREFIX/bin/curl-config --with-geos=$PREFIX/bin/geos-config --with-libkml=$PREFIX --with-xerces=$PREFIX --with-xerces-inc="$PREFIX/include" --with-xerces-lib="-L$PREFIX/lib -lxerces-c -framework CoreServices" --with-pg=$PREFIX/bin/pg_config
    else
        # with debug
    	./configure CC="$GDA_CC" CXX="$GDA_CXX" CFLAGS="$GDA_CFLAGS" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --with-jpeg=internal --prefix=$PREFIX --with-freexl=$PREFIX --with-libiconv-prefix="-L/usr/lib" --with-sqlite3=$PREFIX --with-spatialite=$PREFIX --with-static-proj4=$PREFIX --with-curl=$PREFIX/bin/curl-config --with-geos=$PREFIX/bin/geos-config --with-libkml=$PREFIX --with-xerces=$PREFIX --with-xerces-inc="$PREFIX/include" --with-xerces-lib="-L$PREFIX/lib -lxerces-c -framework CoreServices" --with-pg=$PREFIX/bin/pg_config --enable-debug
    fi
    echo "$GEODA_HOME/dep/$LIB_NAME"
    cp -rf $GEODA_HOME/dep/$LIB_NAME/* .
    #make clean
    $MAKER
    touch .libs/libgdal.lai
    make install
    cp .libs/* ../../libraries/lib
    #cd ogr/ogrsf_frmts/oci
    #make plugin
    #mv ogr_OCI.so ogr_OCI.dylib
    #install_name_tool -change "/scratch/plebld/208/network/lib/libnnz10.dylib" "/Users/xun/Downloads/Oracle_10204Client_MAC_X86/ohome/lib/libnnz10.dylib" ogr_OCI.so
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    echo "You need to modify the libgdal.la and remove the extra '=' symobls."
    exit
fi
#########################################################################
# install wxWidgets library
#########################################################################
LIB_NAME=wxWidgets-3.0.0
LIB_URL=http://softlayer-dal.dl.sourceforge.net/project/wxwindows/3.0.0/wxWidgets-3.0.0.tar.bz2
#LIB_URL=http://iweb.dl.sourceforge.net/project/wxwindows/2.9.5/wxWidgets-2.9.5.tar.bz2
LIB_FILENAME=$(basename "$LIB_URL" ".tar")
#LIB_CHECKER=libwx_baseu-2.9.a
LIB_CHECKER=libwx_baseu-3.0.a
echo $LIB_FILENAME

cd $DOWNLOAD_HOME
if ! [ -d "$LIB_NAME" ] ; then
    curl -O $LIB_URL
    tar -xf $LIB_FILENAME
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    cd $LIB_NAME
    make clean
    ./configure CFLAGS="$GDA_CFLAGS -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.6" CXXFLAGS="$GDA_CXXFLAGS -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.6" LDFLAGS="$GDA_LDFLAGS -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.6" OBJCFLAGS="-arch x86_64 -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.6" OBJCXXFLAGS="-arch x86_64 -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.6" --with-cocoa --disable-shared --disable-monolithic --with-opengl --enable-postscript --with-macosx-version-min=10.6 --enable-textfile --prefix=$PREFIX
    $MAKER 
    make install
    cd ..
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi
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

