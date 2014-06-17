#!/bin/bash
if [ ! -e ./clmbs_test ]
then
  mkdir clmbs_test
fi

cp columbus.shx clmbs_test/columbus.shx
cp columbus.shp clmbs_test/columbus.shp
cp columbus.dbf clmbs_test/columbus.dbf
cp columbus.gal clmbs_test/columbus.gal
cp columbus_6nn.gwt clmbs_test/columbus_6nn.gwt
cp proj_columbus.gda clmbs_test/columbus.gda

