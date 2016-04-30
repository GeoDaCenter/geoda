#!/bin/sh

#GEODA_HOME=$(cd "$(dirname "$0")"; pwd)
GEODA_HOME=/usr/local/geoda

export LD_LIBRARY_PATH=$GEODA_HOME/plugins:$ORACLE_HOME:$LD_LIBRARY_PATH
export GDAL_DATA=$GEODA_HOME/gdaldata
export OGR_DRIVER_PATH=$GEODA_HOME/plugins
chmod +x "$GEODA_HOME/plugins/*.so"
chmod +x "$GEODA_HOME/GeoDa"
exec "$GEODA_HOME/GeoDa"
