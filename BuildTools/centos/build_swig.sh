!/bin/bash
#############################################################################
# ./build.sh
# ./build.sh [CPU]
# ./build.sh 8 (with debug)
# ./build.sh [CPU] [NODEBUG, true=1 false=0(default)]
# ./build.sh 8 1 (no debug)
#############################################################################
CPUS=4
NODEBUG=1
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

if ! type "g++" > /dev/null; then
    echo "You need to install g++ to run this script."
    sudo yum install gcc-c++
fi

if ! type "cmake" > /dev/null; then
    echo "You need to install cmake to run this script."
    sudo yum install cmake
fi

if ! type "curl" > /dev/null; then
    echo "You need to install curl to run this script."
    sudo yum install curl
fi

read -p "Do you want to install pre-requisites (e.g. libreadline, zlib, libexpat, libcurl ...)?[y/n]" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo yum install readline-devel zlib-devel expat-devel curl-devel autoreconf libtool openssl-devel libidn-devel openldap-devel mesa-libGL-devel mesa-libGLU-devel gtk3 freeglut-devel webkitgtk3-devel
fi

unset ORACLE_HOME
export GEODA_HOME=$PWD
PREFIX=$GEODA_HOME/libraries
DOWNLOAD_HOME=$GEODA_HOME/temp
echo $PREFIX

MAKER="make -j $CPUS"
export GDA_CC='/usr/bin/gcc'
export GDA_CXX='/usr/bin/g++'
export CFLAGS="-m64 -fPIC"
export CXXFLAGS="-m64 -fPIC"
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
    LIB_FILENAME=$(basename "$LIB_URL")
    CONFIGURE_FLAGS=$4
    echo ""
    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    echo "% Building: $LIB_FILENAME"
    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME

    if ! [ -f "$LIB_FILENAME" ] ; then
	echo "$LIB_FILENAME not found. Downloading..."
        curl -O $LIB_URL
    else
	echo "$LIB_FILENAME found.  Download skipped."
    fi

    if ! [ -d "$LIB_NAME" ] ; then
	echo "Directory $LIB_NAME not found.  Expanding..."
        tar -xf $LIB_FILENAME
    else
	echo "Directory $LIB_NAME found.  File expansion skipped."
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
	chmod +x configure
        ./configure CFLAGS="-m64" CXXFLAGS="-m64" LDFLAGS="-m64 -L/usr/lib64" --prefix=$PREFIX $CONFIGURE_FLAGS
        #./configure --prefix=$PREFIX
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
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: libiConv %"
echo "%%%%%%%%%%%%%%%%%%%%%%"
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
# install c-ares -- for cURL, prevent crash on Mac oSx with threads
#########################################################################
install_library c-ares-1.10.0 https://s3.us-east-2.amazonaws.com/geodabuild/c-ares-1.10.0.tar.gz libcares.a


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
    ./configure --enable-ares=$PREFIX CC="$GDA_CC" CFLAGS="$GDA_CFLAGS" CXX="$GDA_CXX" CXXFLAGS="$GDA_CXXFLAGS" LDFLAGS="$GDA_LDFLAGS" --prefix=$PREFIX --without-librtmp
    $MAKER
    make install
fi

if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
    echo "Error! Exit"
    exit
fi

#########################################################################
# install Xerces
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%"
echo "% Building: Xerces %"
echo "%%%%%%%%%%%%%%%%%%%%"
{
    LIB_NAME="xerces-c-3.1.1"
    LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/xerces-c-3.1.1.tar.gz"
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
install_library geos-3.3.8 https://s3.us-east-2.amazonaws.com/geodabuild/geos-3.3.8.tar.bz2  libgeos.a

#########################################################################
# install PROJ.4
#########################################################################
install_library proj-4.8.0 https://s3.us-east-2.amazonaws.com/geodabuild/proj-4.8.0.tar.gz libproj.a

#########################################################################
# install FreeXL
#########################################################################
install_library freexl-1.0.0f https://s3.us-east-2.amazonaws.com/geodabuild/freexl-1.0.0f.tar.gz libfreexl.a

#########################################################################
# install SQLite
#########################################################################
install_library sqlite-autoconf-3071602 https://s3.us-east-2.amazonaws.com/geodabuild/sqlite-autoconf-3071602.tar.gz libsqlite3.a

#########################################################################
# install PostgreSQL
#########################################################################
# libreadline, zlib
echo "install libreadline, zlib"
install_library postgresql-9.2.4 https://s3.us-east-2.amazonaws.com/geodabuild/postgresql-9.2.4.tar.bz2 libpq.a

#########################################################################
# install libjpeg
#########################################################################
install_library jpeg-8 https://s3.us-east-2.amazonaws.com/geodabuild/jpegsrc.v8.tar.gz libjpeg.a

#########################################################################
# install libkml requires 1.3
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%"
echo "% Building: libkml %"
echo "%%%%%%%%%%%%%%%%%%%%"
# libexpat,libcurl4-gnutls-dev 
#install_library libkml-1.2.0 https://libkml.googlecode.com/files/libkml-1.2.0.tar.gz libkmlbase.a
{
    LIB_NAME="libkml"
    LIB_CHECKER="libkmlbase.a"
    LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/libkml-r680.tar.gz
    LIB_FILENAME=libkml-r680.tar.gz
    echo $LIB_NAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        curl -O $LIB_URL
    fi

    if ! [ -d "$LIB_NAME" ]; then
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cp -rf ../dep/"$LIB_NAME" .
        cd $LIB_NAME
        ./autogen.sh
    	chmod +x configure
        ./configure CXXFLAGS="-Wno-long-long" --prefix=$PREFIX
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
        #$MAKER
        sed -i.old "s/examples//g" Makefile
        make third_party
        make src
        make testdata
        make install
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}

#########################################################################
# install SpatiaLite
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: Spatialite %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%"
{
    LIB_NAME=libspatialite-4.0.0
    LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/libspatialite-4.0.0.tar.gz
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
	echo $PREFIX
        ./configure CFLAGS="-I$PREFIX/include" CXXFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib" --prefix=$PREFIX --enable-geos --enable-iconv --enable-proj --with-geosconfig=$PREFIX/bin/geos-config
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
echo ""
echo "%%%%%%%%%%%%%%%%%%%"
echo "% Building: MySQL %"
echo "%%%%%%%%%%%%%%%%%%%"
{
    LIB_NAME=mysql-5.6.14
    LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/mysql-5.6.14.tar.gz
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
# install boost library
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: Boost 1.57 %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%"
{
    LIB_NAME=boost_1_57_0
    LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/boost_1_57_0.tar.gz
    LIB_FILENAME=boost_1_57_0.tar.gz
    LIB_CHECKER=libboost_thread.a
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -f "$LIB_FILENAME" ]; then
	echo "$LIB_FILENAME not found. Downloading..."
        curl -O $LIB_URL
    else
	echo "$LIB_FILENAME found. Skipping download."
    fi

    if ! [ -d "$LIB_NAME" ]; then
	echo "Directory $LIB_NAME not found. Expanding archive."
        tar -xf $LIB_FILENAME
    else
	echo "Directory $LIB_NAME found. Skipping expansion."
    fi

    if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/stage/lib/$LIB_CHECKER" ] ; then
	echo "$LIB_CHECKER not found.  Building Boost..."
        cd $PREFIX/include
        rm boost
        ln -s $DOWNLOAD_HOME/boost_1_57_0 boost
        cd $DOWNLOAD_HOME/boost_1_57_0
        chmod +x bootstrap.sh
        #chmod +x tools/build/v2/engine/build.sh
        ./bootstrap.sh
        chmod +x b2
        ./b2 --with-thread --with-date_time --with-chrono --with-system link=static,shared threading=multi stage
    fi

    if ! [ -f "$GEODA_HOME/temp/$LIB_NAME/stage/lib/$LIB_CHECKER" ] ; then
        echo "Error: Target library $LIB_CHECKER not found. Exiting build."
        exit
    fi
}

#########################################################################
# install JSON Spirit
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: JSON Spirit %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%"
LIB_NAME="json_spirit_v4.08"
LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/json_spirit_v4.08.zip"
LIB_CHECKER="libjson_spirit.a"
LIB_FILENAME="json_spirit_v4.08.zip"
echo $LIB_FILENAME


cd $DOWNLOAD_HOME

if ! [ -d "$LIB_NAME" ]; then
    curl -O https://s3.us-east-2.amazonaws.com/geodabuild/json_spirit_v4.08.zip
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
# install CLAPACK
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: CLAPACK 3.2.1 %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%"
{
    CLAPACK_NAME="CLAPACK-3.2.1"
    LIB_CHECKER="libf2c.a"
    echo $CLAPACK_NAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$CLAPACK_NAME" ]; then
        curl -O https://s3.us-east-2.amazonaws.com/geodabuild/clapack.tgz
        tar -xvf clapack.tgz
    fi

    cp -rf $GEODA_HOME/dep/$CLAPACK_NAME .
    cd $CLAPACK_NAME
    if ! [ -f "libf2c.a" ] || ! [ -f "blas.a" ] || ! [ -f "lapack.a" ]; then
        cp make.inc.example make.inc
        $MAKER f2clib
        cp F2CLIBS/libf2c.a .
        $MAKER blaslib
        cd INSTALL
        $MAKER
        cd ..
        cd SRC
        $MAKER
        cd ..
        $MAKER
        mv -f blas_LINUX.a blas.a
        mv -f lapack_LINUX.a lapack.a
        mv -f tmglib_LINUX.a tmglib.a
        cd ..
    fi

    if ! [ -f "$GEODA_HOME/temp/$CLAPACK_NAME/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}

#########################################################################
# install json-c
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: json-c                 "
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
{

    LIB_NAME=json-c
    LIB_CHECKER=libjson-c.a

    if ! [ -d "$LIB_NAME" ] ; then
	git clone https://github.com/json-c/json-c.git
    fi


    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
	sh autogen.sh
        ./configure --prefix=$PREFIX
	make
	make install
    fi

    #if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        #echo "Error! Exit"
        #exit
    #fi
}

#########################################################################
# install GDAL/OGR
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building: Custom GDAL/OGR 1.9.2 %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
{
    LIB_NAME=gdal
    LIB_URL=https://codeload.github.com/lixun910/gdal/zip/GeoDa17Merge
    LIB_FILENAME=GeoDa17Merge
    LIB_CHECKER=libgdal.a
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -d "$LIB_NAME" ] ; then
        #curl -k -O $LIB_URL
        #unzip $LIB_FILENAME
        git clone https://github.com/lixun910/gdal.git gdal-GeoDa17Merge
        mv gdal-GeoDa17Merge/gdal gdal
    fi

    if ! [ -f "$PREFIX/lib/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
        chmod +x configure
        chmod +x install-sh
        ./configure
        cp -rf $GEODA_HOME/dep/gdal-1.9.2/* .
        cp GDALmake64.opt GDALmake.opt
        #make clean
        $MAKER
        touch .libs/libgdal.lai
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

#########################################################################
# install wxWidgets library
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "% Building wxWidgets 3.0.2 %"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
# sudo apt-get install libgtk2.0-dev libglu1-mesa-dev libgl1-mesa-dev
{
    LIB_NAME=wxWidgets-3.1.0
    LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/wxWidgets-3.1.0.tar.bz2"

    LIB_FILENAME=$(basename "$LIB_URL" ".tar")
    LIB_CHECKER=wx-config
    echo $LIB_FILENAME

    cd $DOWNLOAD_HOME
    if ! [ -f "$LIB_FILENAME" ] ; then
        curl -k -o $LIB_FILENAME $LIB_URL
    fi

    if ! [ -d "$LIB_NAME" ]; then
        tar -xf $LIB_FILENAME
    fi

    if ! [ -f "$PREFIX/bin/$LIB_CHECKER" ] ; then
        cd $LIB_NAME
        cp -rf $GEODA_HOME/dep/$LIB_NAME/* .
        chmod +x configure
        chmod +x src/stc/gen_iface.py
        ./configure --with-gtk=3 --enable-ascii --disable-monolithic --with-opengl --enable-postscript --without-libtiff --disable-debug --enable-webview --prefix=$PREFIX
        #make clean
        $MAKER
        make install
        cd ..
    fi

    if ! [ -f "$PREFIX/bin/$LIB_CHECKER" ] ; then
        echo "Error! Exit"
        exit
    fi
}

#########################################################################
# Eigen3
#########################################################################
LIB_NAME=eigen3
LIB_URL=https://s3.us-east-2.amazonaws.com/geodabuild/eigen3.zip
LIB_CHECKER=Dense
LIB_FILENAME=$LIB_NAME.zip
echo $LIB_FILENAME
cd $DOWNLOAD_HOME

if ! [ -f "$LIB_FILENAME" ] ; then
        curl -O $LIB_URL
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
# build GeoDa
#########################################################################
echo ""
echo "%%%%%%%%%%%%%%%%%%%"
echo "% Building: GeoDa %"
echo "%%%%%%%%%%%%%%%%%%%"
{
    cd $GEODA_HOME
    cp ../../GeoDamake.centos.opt ../../GeoDamake.opt
    mkdir ../../o
    make clean
    $MAKER
    make app
    #cp plugins/x64/*.so build/plugins/
    cp ../CommonDistFiles/web_plugins/no_map.png build/web_plugins/no_map.png
}
