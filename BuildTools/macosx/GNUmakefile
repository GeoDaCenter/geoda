ifndef GEODA_HOME
A = $(shell ls -l)
$(error You have to setup GEODA_HOME variable e.g. export GEODA_HOME=\$PWD, or run build.sh)
endif

include ../../GeoDamake.opt

GeoDa_OBJ = $(wildcard $(GeoDa_ROOT)/o/*.o)

default:	compile-geoda 

app:  build-geoda-mac

compile-geoda:	weights-target ogl-target io-target algorithms-target dataviewer-target dialogtools-target \
		knn-target explore-target libgdiam-target regression-target \
		shapeoperations-target resource-target varcalc-target \
		geoda-target

test:
	echo $(WX_HEADER)

resource-target:
	(cd $(GeoDa_ROOT)/rc; $(MAKE) xrc; $(MAKE))

weights-target:
	(cd $(GeoDa_ROOT)/Weights; $(MAKE))

ogl-target:
	(cd $(GeoDa_ROOT)/ogl; $(MAKE))

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

build-geoda-mac:
	rm -rf build/GeoDa.app
	mkdir -p build
	mkdir -p build/GeoDa.app
	mkdir -p build/GeoDa.app/Contents
	mkdir -p build/GeoDa.app/Contents/MacOS
	mkdir -p build/GeoDa.app/Contents/MacOS/basemap_cache
	mkdir -p build/GeoDa.app/Contents/MacOS/web_plugins
	mkdir -p build/GeoDa.app/Contents/Frameworks
	mkdir -p build/GeoDa.app/Contents/Resources
	mkdir -p build/GeoDa.app/Contents/Resources/gdaldata
	mkdir -p build/GeoDa.app/Contents/Resources/plugins
	cp $(GeoDa_ROOT)/BuildTools/CommonDistFiles/web_plugins/*.* build/GeoDa.app/Contents/Resources/
	cp $(GeoDa_ROOT)/BuildTools/CommonDistFiles/cache.sqlite build/GeoDa.app/Contents/MacOS
	cp $(GeoDa_ROOT)/Algorithms/lisa_kernel.cl build/GeoDa.app/Contents/MacOS
	cp -rf $(GeoDa_ROOT)/BuildTools/CommonDistFiles/osx/proj build/GeoDa.app/Contents/Resources
	cp -rf $(GeoDa_ROOT)/internationalization/lang build/GeoDa.app/Contents/MacOS
	$(LD) $(LDFLAGS) $(GeoDa_OBJ) $(LIBS) -o build/GeoDa.app/Contents/MacOS/GeoDa
	cp run.sh build/GeoDa.app/Contents/MacOS/run.sh
	cp GeoDa-GDAL-Info.plist build/GeoDa.app/Contents/Info.plist
	cp $(GeoDa_ROOT)/rc/data_viewer_dialogs.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/dialogs.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/GeoDa.icns build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/menus.xrc build/GeoDa.app/Contents/Resources
	cp $(GeoDa_ROOT)/rc/toolbar.xrc build/GeoDa.app/Contents/Resources
	cp /usr/local/opt/gdal/share/gdal/* build/GeoDa.app/Contents/Resources/gdaldata
	cp libraries/lib/libwx_osx_cocoau-3.1.4.0.0.dylib build/GeoDa.app/Contents/Frameworks/libwx_osx_cocoau-3.1.dylib
	cp libraries/lib/libwx_osx_cocoau_gl-3.1.4.0.0.dylib build/GeoDa.app/Contents/Frameworks/libwx_osx_cocoau_gl-3.1.dylib
	cp /usr/local/opt/gdal/lib/libgdal.28.dylib build/GeoDa.app/Contents/Frameworks
	install_name_tool -id "GeoDa" build/GeoDa.app/Contents/MacOS/GeoDa
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libwx_osx_cocoau_gl-3.1.dylib" "@executable_path/../Frameworks/libwx_osx_cocoau_gl-3.1.dylib" build/GeoDa.app/Contents/MacOS/GeoDa
	install_name_tool -change "$(GEODA_HOME)/libraries/lib/libwx_osx_cocoau-3.1.dylib" "@executable_path/../Frameworks/libwx_osx_cocoau-3.1.dylib" build/GeoDa.app/Contents/MacOS/GeoDa
	install_name_tool -change "/usr/local/opt/gdal/lib/libgdal.28.dylib" "@executable_path/../Frameworks/libgdal.28.dylib" build/GeoDa.app/Contents/MacOS/GeoDa
	python3 install_name.py $(GEODA_HOME)/build/GeoDa.app/Contents/Frameworks
	
clean:
	rm -f ../../o/*
	rm -rf build/GeoDa.app
