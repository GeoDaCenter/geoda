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
cp columbus_qn2.gal clmbs_test/columbus_qn2.gal
cp columbus_thres_xy.gwt clmbs_test/columbus_thres_xy.gwt
cp columbus.gda clmbs_test/columbus.gda

