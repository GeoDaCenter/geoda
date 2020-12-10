#!/bin/bash


if ! [ -d build ]; then
    echo "Please run build.sh to build GeoDa executable file first."
    exit
fi

if ! [ -f build/GeoDa ]; then
    echo "Please run build.sh to build GeoDa executable file first."
    exit
fi


rm -rf product
if [ $# -ne 1 ]; then
    cp -rf package product
else
    cp -rf package_$1 product
fi

chmod +x product/DEBIAN/postinst
mkdir product/usr/local
mkdir product/usr/bin
mkdir product/usr/local/geoda
cp -rf build/* product/usr/local/geoda/
cp run_geoda.sh product/usr/bin/

cd product 
find . -name .svn |xargs rm -rf
cd ..

MACHINE_TYPE=`uname -m`
if [ $MACHINE_TYPE == 'x86_64' ]; then
    # 64-bit platform
    rm product/DEBIAN/control
    mv product/DEBIAN/control_disco product/DEBIAN/control
fi

rm -f *.deb
if [ $# -ne 1 ]; then
    dpkg -b product/ geoda_1.18-1disco1_amd64.deb
else
    dpkg -b product/ geoda_1.18-1$1.deb
fi
