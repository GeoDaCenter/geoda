#!/bin/bash 

APP_PATH=$1

show_tip()
{
    echo "============================================================"
    echo "                GeoDa Tool"
    echo " This tool will setup GeoDa software to enable plugins of "
    echo " Oracle database and ESRI File Geodatabase. Users need to "
    echo " download, configure and setup Oracle Instance Client and "
    echo " ESRI File Geodatabase API first. This tool will check    "
    echo ' $ORACLE_HOME for Oracle Instant Client and $FileGDB_HOME '
    echo " for ESRI File Geodatabase automatically for GeoDa plugins"
    echo " configuration."
    echo "============================================================"
    echo "Usage: ./install_plugins.sh GeoDaAppPath"
    echo ""
    echo "Options: "
    echo "        GeoDaAppPath e.g. ./GeoDa.app"
    echo "============================================================"
}

if [[ $APP_PATH == "" ]] ; then
    show_tip 
    exit
fi

if ! [ -d $APP_PATH ] && ! [ -d "$APP_PATH.app" ] ; then
    show_tip
    echo " GeoDaAppPath is not exist."
    echo "============================================================"
    exit
fi

if [[ $ORACLE_HOME == "" ]] && [[ $FileGDB_HOME == "" ]]; then
    show_tip
    echo ' Both $ORACLE_HOME and $FileGDB_HOME are not setup.'
    echo " Can't setup GeoDa plugins."
    echo "============================================================"
    exit
fi

if ! [ -d $APP_PATH ] ; then
    APP_PATH="$APP_PATH.app"
fi

RUN_FILE="$APP_PATH/Contents/MacOS/run.sh"
mv $RUN_FILE "$RUN_FILE.bak"

echo '#!/bin/sh' >> $RUN_FILE
echo '' >> $RUN_FILE
echo 'GEODA_HOME=$(cd "$(dirname "$0")"; pwd)' >> $RUN_FILE
echo '' >> $RUN_FILE
echo 'export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$GEODA_HOME/plugins' >> $RUN_FILE
echo 'export GDAL_DATA=$GEODA_HOME/../Resources/gdaldata' >> $RUN_FILE
echo 'export OGR_DRIVER_PATH=$GEODA_HOME/../Resources/plugins' >> $RUN_FILE

HAS_ORACLE=0
if ! [[ $ORACLE_HOME == "" ]] ; then
    HAS_ORACLE=1
    echo "export ORACLE_HOME=$ORACLE_HOME" >> $RUN_FILE
    echo 'export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$ORACLE_HOME' >> $RUN_FILE
fi

HAS_FILEGDB=0
if ! [[ $FileGDB_HOME == "" ]] ; then
    HAS_FILEGDB=1
    echo "export FileGDB_HOME=$FileGDB_HOME" >> $RUN_FILE
    echo 'export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$FileGDB_HOME' >> $RUN_FILE
fi

echo 'exec "$GEODA_HOME/GeoDa"' >> $RUN_FILE

chmod +x $RUN_FILE
rm "$RUN_FILE.bak"

if [[ $HAS_ORACLE+$HAS_FILEGDB > 0 ]] ; then
    echo "The following plugins have been configured in GeoDa:"

    if [[ $HAS_ORACLE == 1 ]] ; then
        echo "-->Oracle Instance Client"
    fi

    if [[ $HAS_FILEGDB == 1 ]] ; then
        echo "-->ESRI File Geodatabase"
    fi

    echo "Please restart GeoDa to use these plugins. Thank you!"
fi
