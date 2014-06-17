#!/bin/sh

GEODA_HOME=$(cd "$(dirname "$0")"; pwd)

export ORACLE_HOME=OHOME
export FileGDB_HOME=FHOME
export DYLD_LIBRARY_PATH=$GEODA_HOME/plugins:$ORACLE_HOME:$FileGDB_HOME:$DYLD_LIBRARY_PATH
export GDAL_DATA=$GEODA_HOME/../Resources/gdaldata
export OGR_DRIVER_PATH=$GEODA_HOME/../Resources/plugins
exec "$GEODA_HOME/GeoDa"
