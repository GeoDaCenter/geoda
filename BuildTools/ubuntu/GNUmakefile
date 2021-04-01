ifndef GEODA_HOME
$(error You have to setup GEODA_HOME variable e.g. export GEODA_HOME=PWD, or run build.sh)
endif

include ../../GeoDamake.opt

GeoDa_OBJ = $(wildcard $(GeoDa_ROOT)/o/*.o)

default:	compile-geoda

app:  build-geoda

compile-geoda:	ogl-target io-target algorithms-target dataviewer-target dialogtools-target \
		weights-target knn-target explore-target libgdiam-target regression-target \
		shapeoperations-target resource-target varcalc-target \
		geoda-target

conf = `./libraries/bin/wx-config --libs`

test1:
	@echo test $(WX_HEADER) $(LIBS)

resource-target:
	(cd $(GeoDa_ROOT)/rc; $(MAKE) xrc; $(MAKE))

ogl-target:
	(cd $(GeoDa_ROOT)/ogl; $(MAKE))

weights-target:
	(cd $(GeoDa_ROOT)/Weights; $(MAKE))

knn-target:
	(cd $(GeoDa_ROOT)/kNN; $(MAKE))

io-target:
	(cd $(GeoDa_ROOT)/io; $(MAKE))

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

build-geoda:
	rm -rf build
	mkdir -p build
	mkdir build/Resources
	mkdir build/gdaldata
	mkdir build/proj
	mkdir build/plugins
	mkdir build/basemap_cache
	mkdir build/web_plugins
	mkdir build/lang
	$(LD) $(LDFLAGS) $(GeoDa_OBJ) $(LIBS) -o build/GeoDa
	#cp run.sh build/run.sh
	#chmod +x build/run.sh
	cp -rf $(GeoDa_ROOT)/internationalization/lang/* build/lang
	cp $(GeoDa_ROOT)/rc/data_viewer_dialogs.xrc build/Resources
	cp $(GeoDa_ROOT)/rc/dialogs.xrc build/Resources
	cp $(GeoDa_ROOT)/rc/GeoDa.icns build/Resources
	cp $(GeoDa_ROOT)/rc/menus.xrc build/Resources
	cp $(GeoDa_ROOT)/rc/toolbar.xrc build/Resources
	cp $(GEODA_HOME)/../CommonDistFiles/cache.sqlite build/cache.sqlite
	cp $(GEODA_HOME)/../CommonDistFiles/geoda_prefs.sqlite build/geoda_prefs.sqlite
	cp $(GEODA_HOME)/../CommonDistFiles/geoda_prefs.json build/geoda_prefs.json
	cp -rf $(GEODA_HOME)/../CommonDistFiles/web_plugins/* build/web_plugins
	cp -rf $(GeoDa_ROOT)/internationalization/lang build/
	if [ -d "/usr/share/gdal/2.2" ]; then cp /usr/share/gdal/2.2/* build/gdaldata; fi
	if [ -f "/usr/share/gdal/header.dxf" ]; then cp /usr/share/gdal/* build/gdaldata; fi
	#cp $(GEODA_HOME)/libraries/lib/libcares.so.2.1.0 build/plugins/libcares.so.2

clean:
	rm -f ../../o/*
	rm -rf build
