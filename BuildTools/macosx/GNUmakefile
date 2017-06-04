ifndef GEODA_HOME
A = $(shell ls -l)
$(error You have to setup GEODA_HOME variable e.g. export GEODA_HOME=\$PWD, or run build.sh)
endif

include ../../GeoDamake.opt

GeoDa_OBJ = $(wildcard $(GeoDa_ROOT)/o/*.o)

default:	compile-geoda 

app:  build-geoda-mac

compile-geoda:	algorithms-target dataviewer-target dialogtools-target \
		explore-target libgdiam-target regression-target \
		shapeoperations-target resource-target varcalc-target \
		geoda-target

test:
	echo $(WX_HEADER)

resource-target:
	(cd $(GeoDa_ROOT)/rc; $(MAKE) xrc; $(MAKE))

algorithms-target:
	(cd $(GeoDa_ROOT)/Algorithms; $(MAKE))

dataviewer-target:
	(cd $(GeoDa_ROOT)/DataViewer; $(MAKE))

dialogtools-target:
	(cd $(GeoDa_ROOT)/DialogTools; $(MAKE))

explore-target:
	(cd $(GeoDa_ROOT)/Explore; $(MAKE))

libgdiam-target:
	(cd $(GeoDa_ROOT)/libgdiam; $(MAKE))

regression-target:
	(cd $(GeoDa_ROOT)/Regression; $(MAKE))

shapeoperations-target:
	(cd $(GeoDa_ROOT)/ShapeOperations; $(MAKE))

varcalc-target:
	(cd $(GeoDa_ROOT)/VarCalc; $(MAKE))

geoda-target:
	(cd $(GeoDa_ROOT); $(MAKE))

build-geoda-mac:
	rm -rf build/GeoDa.app
	mkdir -p build
	mkdir -p build/GeoDa.app
	mkdir -p build/GeoDa.app/Contents
	mkdir -p build/GeoDa.app/Contents/MacOS
	mkdir -p build/GeoDa.app/Contents/MacOS/basemap_cache
	mkdir -p build/GeoDa.app/Contents/MacOS/web_plugins
	mkdir -p build/GeoDa.app/Contents/Resources
	mkdir -p build/GeoDa.app/Contents/Resources/gdaldata
	mkdir -p build/GeoDa.app/Contents/Resources/plugins
	cp $(GeoDa_ROOT)/BuildTools/CommonDistFiles/web_plugins/*.* build/GeoDa.app/Contents/Resources/
	cp $(GeoDa_ROOT)/BuildTools/CommonDistFiles/cache.sqlite build/GeoDa.app/Contents/MacOS
	$(LD) $(LDFLAGS) $(GeoDa_OBJ) $(LIBS) -o build/GeoDa.app/Contents/MacOS/GeoDa
	cp run.sh build/GeoDa.app/Contents/MacOS/run.sh
	cp GeoDa-GDAL-Info.plist build/GeoDa.app/Contents/Info.plist
	#sed -i '' 's/ORACLE_HOME_PATH/$(subst /,\/,$(ORACLE_HOME))/g' build/GeoDa.app/Contents/Info.plist
	#sed -i '' 's/ORACLE_LIBRARY_PATH/$(subst /,\/,$(DYLD_LIBRARY_PATH))/g' build/GeoDa.app/Contents/Info.plist
	cp $(GeoDa_ROOT)/rc/data_viewer_dialogs.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/dialogs.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/GeoDa.icns build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/menus.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/toolbar.xrc build/GeoDa.app/Contents/Resources
	cp libraries/share/gdal/* build/GeoDa.app/Contents/Resources/gdaldata
	cp libraries/lib/libgdal.20.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmlengine.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmldom.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmlbase.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmlconvenience.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmlregionator.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libkmlxsd.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libminizip.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/liburiparser.1.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libxerces-c-3.1.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libpq.5.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libspatialite.5.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libfreexl.1.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libproj.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libsqlite3.0.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libgeos_c.1.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libgeos-3.3.8.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libcurl.4.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp libraries/lib/libcares.2.dylib build/GeoDa.app/Contents/Resources/plugins/
	#cp libraries/lib/libmysqlclient.18.dylib build/GeoDa.app/Contents/Resources/plugins/
	cp plugins/ogr_OCI.so build/GeoDa.app/Contents/Resources/plugins/ogr_OCI.so
	cp plugins/ogr_FileGDB.so build/GeoDa.app/Contents/Resources/plugins/ogr_FileGDB.so
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgdal.20.dylib" "@executable_path/../Resources/plugins/libgdal.20.dylib" build/GeoDa.app/Contents/MacOS/GeoDa
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcurl.4.dylib" "@executable_path/../Resources/plugins/libcurl.4.dylib" build/GeoDa.app/Contents/MacOS/GeoDa
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgdal.20.dylib" "@executable_path/../Resources/plugins/libgdal.20.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	#install_name_tool -change "libmysqlclient.18.dylib" "@executable_path/../Resources/plugins/libmysqlclient.18.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlengine.0.dylib" "@executable_path/../Resources/plugins/libkmlengine.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlregionator.0.dylib" "@executable_path/../Resources/plugins/libkmlregionator.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlxsd.0.dylib" "@executable_path/../Resources/plugins/libkmlxsd.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmldom.0.dylib" "@executable_path/../Resources/plugins/libkmldom.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlbase.0.dylib" "@executable_path/../Resources/plugins/libkmlbase.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlconvenience.0.dylib" "@executable_path/../Resources/plugins/libkmlconvenience.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libminizip.0.dylib" "@executable_path/../Resources/plugins/libminizip.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/liburiparser.1.dylib" "@executable_path/../Resources/plugins/liburiparser.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libxerces-c-3.1.dylib" "@executable_path/../Resources/plugins/libxerces-c-3.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libpq.5.dylib" "@executable_path/../Resources/plugins/libpq.5.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libspatialite.5.dylib" "@executable_path/../Resources/plugins/libspatialite.5.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libfreexl.1.dylib" "@executable_path/../Resources/plugins/libfreexl.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libproj.0.dylib" "@executable_path/../Resources/plugins/libproj.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libsqlite3.0.dylib" "@executable_path/../Resources/plugins/libsqlite3.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgeos_c.1.dylib" "@executable_path/../Resources/plugins/libgeos_c.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgeos-3.3.8.dylib" "@executable_path/../Resources/plugins/libgeos-3.3.8.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcurl.4.dylib" "@executable_path/../Resources/plugins/libcurl.4.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcares.2.dylib" "@executable_path/../Resources/plugins/libcares.2.dylib" build/GeoDa.app/Contents/Resources/plugins/libgdal.20.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcares.2.dylib" "@executable_path/../Resources/plugins/libcares.2.dylib" build/GeoDa.app/Contents/Resources/plugins/libcurl.4.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcares.2.dylib" "@executable_path/../Resources/plugins/libcares.2.dylib" build/GeoDa.app/Contents/Resources/plugins/libxerces-c-3.1.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlengine.0.dylib" "@executable_path/../Resources/plugins/libkmlengine.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlengine.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libkmlbase.0.dylib" "@executable_path/../Resources/plugins/libkmlbase.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlengine.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libminizip.0.dylib" "@executable_path/../Resources/plugins/libminizip.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlengine.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/liburiparser.1.dylib" "@executable_path/../Resources/plugins/liburiparser.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlengine.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libminizip.0.dylib" "@executable_path/../Resources/plugins/libminizip.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlbase.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/liburiparser.1.dylib" "@executable_path/../Resources/plugins/liburiparser.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libkmlbase.0.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libfreexl.1.dylib" "@executable_path/../Resources/plugins/libfreexl.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libspatialite.5.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libproj.0.dylib" "@executable_path/../Resources/plugins/libproj.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libspatialite.5.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libsqlite3.0.dylib" "@executable_path/../Resources/plugins/libsqlite3.0.dylib" build/GeoDa.app/Contents/Resources/plugins/libspatialite.5.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgeos_c.1.dylib" "@executable_path/../Resources/plugins/libgeos_c.1.dylib" build/GeoDa.app/Contents/Resources/plugins/libspatialite.5.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgeos-3.3.8.dylib" "@executable_path/../Resources/plugins/libgeos-3.3.8.dylib" build/GeoDa.app/Contents/Resources/plugins/libspatialite.5.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libgeos-3.3.8.dylib" "@executable_path/../Resources/plugins/libgeos-3.3.8.dylib" build/GeoDa.app/Contents/Resources/plugins/libgeos_c.1.dylib 
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libcurl.4.dylib" "@executable_path/../Resources/plugins/libcurl.4.dylib" build/GeoDa.app/Contents/Resources/plugins/libxerces-c-3.1.dylib 

clean:
	rm -f ../../o/*
	rm -rf build/GeoDa.app
