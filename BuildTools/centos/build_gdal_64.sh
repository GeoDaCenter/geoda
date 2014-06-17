#!/bin/bash
#############################################################################
# ./build.sh
# ./build.sh [CPU]
# ./build.sh 8 (with debug)
# ./build.sh [CPU] [NODEBUG, true=1 false=0(default)]
# ./build.sh 8 1 (no debug)
#############################################################################
CPUS=2
NODEBUG=1

if ! type "cmake" > /dev/null; then
    echo "You need to install cmake to run this script."
    #sudo apt-get install cmake
fi

if ! type "g++" > /dev/null; then
    echo "You need to install g++ to run this script."
    yum install gcc-c++
fi

if ! type "svn" > /dev/null; then
    echo "You need to install SVN to run this script."
    #sudo apt-get install subversion
fi

unset ORACLE_HOME
export GEODA_HOME=$PWD
PREFIX=/usr/local
DOWNLOAD_HOME=$GEODA_HOME/temp
echo $PREFIX

MAKER="make -j $CPUS"
export CFLAGS="-m64"
export CXXFLAGS="-m64"
#export LDFLAGS="-m64 -L/usr/lib/x86_64-linux-gnu"

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
        wget $LIB_URL
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
	chmod +x configure
        #./configure CFLAGS="-m64" CXXFLAGS="-m64" LDFLAGS="-m64 -L/usr/lib/x86_64-linux-gnu" --prefix=$PREFIX
        ./configure --prefix=$PREFIX
        $MAKER
        make install
    fi
    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}
#########################################################################
# install libiConv
#########################################################################
{
    LIB_NAME="libiconv-1.13"
    LIB_URL="http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.13.tar.gz"
    LIB_CHECKER="libiconv.so"
    LIB_FILENAME="libiconv-1.13.tar.gz"
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        curl -O $LIB_URL
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
	chmod +x configure
        ./configure --enable-static --prefix=$PREFIX
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

#########################################################################
# install Xerces
#########################################################################
{
    LIB_NAME="xerces-c-3.1.1"
    LIB_URL="http://mirror.metrocast.net/apache//xerces/c/3/sources/xerces-c-3.1.1.tar.gz"
    LIB_CHECKER="libxerces-c.a"
    LIB_FILENAME=$(basename "$LIB_URL" ".tar")
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        curl -O $LIB_URL
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
	chmod +x configure
        ./configure --prefix=$PREFIX
        chmod +x config/pretty-make
        $MAKER
        make install
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}

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
# libreadline, zlib
echo "install libreadline, zlib"
install_library postgresql-9.2.4 http://ftp.postgresql.org/pub/source/v9.2.4/postgresql-9.2.4.tar.bz2 libpq.a

#########################################################################
# install libjpeg
#########################################################################
install_library jpeg-8 http://www.ijg.org/files/jpegsrc.v8.tar.gz libjpeg.a

#########################################################################
# install libkml requires 1.3
#########################################################################
# libexpat,libcurl4-gnutls-dev 
#########################################################################
# install SpatiaLite
#########################################################################
{
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
        chmod +x configure
        ./configure --host=i686-pc-linux-gnu CFLAGS="-I$PREFIX/include" CXXFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib -liconv -lsqlite3" --prefix=$PREFIX --enable-geos --enable-iconv --enable-proj --with-geosconfig=$PREFIX/bin/geos-config
        $MAKER
        make install
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}
#########################################################################
# MySQL 
#########################################################################
#cmake, curse
{
    LIB_NAME=mysql-5.6.14
    LIB_URL=http://softlayer-dal.dl.sourceforge.net/project/mysql.mirror/MySQL%205.6.14/mysql-5.6.14.tar.gz
    LIB_CHECKER=libmysqlclient.a

    echo $LIB_NAME
    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        curl -O $LIB_URL
        tar -xf $LIB_NAME.tar.gz
        #cp $GEODA_HOME/dep/mysql/my_default.h $GEODA_HOME/temp/mysql-5.6.14/include/my_default.h
    fi

    if ! [ -f "$GEODA_HOME/temp/mysql-5.6.14/bld/libmysql/$LIB_CHECKER" ] ; then
        echo "HERE"
        cd $LIB_NAME
        mkdir -p bld
        cd bld
        cmake -DCURSES_LIBRARY=/usr/lib/libncurses.so -DCURSES_INCLUDE_PATH=/usr/include ..
        chmod +x bld/extra/comp_err
        chmod +x bld/sql/gen_lex_hash
        chmod +x storage/perfschema/gen_pfs_lex_token
        $MAKER
    fi

    if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/bld/libmysql/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}

#########################################################################
# install GDAL/OGR
#########################################################################
{
    LIB_NAME=gdal-1.9.2
    LIB_URL=https://codeload.github.com/lixun910/gdal-1.9.2-work/zip/master
    LIB_FILENAME=master
    LIB_CHECKER=libgdal.a
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        #svn co https://github.com/lixun910/gdal-1.9.2-work/trunk gdal-1.9.2
        curl -k -O $LIB_URL
        unzip $LIB_FILENAME
        mv gdal-1.9.2-work-master gdal-1.9.2
    fi

    cp -rf $GEODA_HOME/dep/$LIB_NAME .
    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
        chmod +x configure
        chmod +x install-sh
        ./configure
        cp -rf $GEODA_HOME/dep/gdal-1.9.2/* .
        cp GDALmake.opt.web GDALmake.opt
        #make clean
        $MAKER
        make install
        #cd ogr/ogrsf_frmts/oci
        #make plugin
        #mv ogr_OCI.so ogr_OCI.dylib
        #install_name_tool -change "/scratch/plebld/208/network/lib/libnnz10.dylib" "/Users/xun/Downloads/Oracle_10204Client_MAC_X86/ohome/lib/libnnz10.dylib" ogr_OCI.so
    fi
    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}
