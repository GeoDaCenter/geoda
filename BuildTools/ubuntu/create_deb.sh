#!/bin/bash


if ! [ -d build ]; then
    echo "Please run build.sh to build GeoDa executable file first."
    exit
fi

if ! [ -f build/GeoDa ]; then
    echo "Please run build.sh to build GeoDa executable file first."
    exit
fi

# 64-bit platform
MACHINE_TYPE=`uname -m`
if [[ $MACHINE_TYPE != 'x86_64' ]]; then
    echo "Machine type should be x86_64"
    exit
fi

if [[ $# -ne 2 ]]; then
    echo "create_deb.sh bionic|disco|xenial 1.18"
    exit
fi

rm -rf product
cp -rf package product

chmod +x product/DEBIAN/postinst
mkdir product/usr/local
mkdir product/usr/bin
mkdir product/usr/local/geoda
cp -rf build/* product/usr/local/geoda/
cp run_geoda.sh product/usr/bin/

cd product 
find . -name .svn |xargs rm -rf
cd ..

rm product/DEBIAN/control
mv product/DEBIAN/control_$1 product/DEBIAN/control

rm -f *.deb
dpkg -b product/ geoda_$2-1$11_amd64.deb
