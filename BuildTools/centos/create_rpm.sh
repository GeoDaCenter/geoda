#!/bin/bash

rm *.rpm
cp rpmmacros ~/.rpmmacros
rm -rf rpm/
cp -rf rpm_template/ rpm/
mkdir rpm/SOURCES
cp -rf build/ rpm/SOURCES/
mv rpm/SOURCES/build rpm/SOURCES/GeoDa-1.8
rm -r rpm/SOURCES/GeoDa-1.8/web_plugins/d3
cd rpm/SOURCES/
tar czf geoda.tar.gz GeoDa-1.8/
cd ..
rpmbuild -ba SPECS/GeoDa.spec
mv RPMS/x86_64/*.rpm ../
