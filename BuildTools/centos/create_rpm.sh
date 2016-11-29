#!/bin/bash

rm -rf rpm/
cp -rf rpm_template/ rpm/
cp -rf build/ rpm/SOURCES/
mv rpm/SOURCES/build rpm/SOURCES/GeoDa-1.8
cd rpm/SOURCES/
tar czf geoda.tar.gz GeoDa-1.8/
cd ..
rpmbuild -ba SPECS/GeoDa.spec

