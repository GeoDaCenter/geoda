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
    echo "create_deb.sh focal|jammy|noble 1.22"
    exit
fi

rm -rf product
cp -rf package product

chmod +x product/DEBIAN/postinst
mkdir product/usr/local
mkdir product/usr/bin
mkdir product/usr/local/geoda
cp -rf build/* product/usr/local/geoda/
# Launcher (the legacy run_geoda.sh was removed with the GNUmake toolchain).
cat > product/usr/bin/geoda << 'EOF'
#!/bin/sh
GEODA_HOME=/usr/local/geoda
export LD_LIBRARY_PATH=$GEODA_HOME/plugins:$ORACLE_HOME:$LD_LIBRARY_PATH
export GDAL_DATA=$GEODA_HOME/gdaldata
export OGR_DRIVER_PATH=$GEODA_HOME/plugins
exec "$GEODA_HOME/GeoDa"
EOF
chmod 755 product/usr/bin/geoda

cd product 
find . -name .svn |xargs rm -rf
cd ..

rm product/DEBIAN/control
mv product/DEBIAN/control_$1 product/DEBIAN/control

rm -f *.deb
dpkg -b product/ geoda_$2-1$11_amd64.deb
