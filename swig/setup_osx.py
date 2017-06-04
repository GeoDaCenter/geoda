#!/usr/bin/env python

from distutils.core import setup, Extension

GEODA_HOME = '/Users/xun/geoda_trunk/BuildTools/macosx'

PYTHON_HEADER = '-I/usr/include/python2.7'

INCLUDE_DIRS = [GEODA_HOME + '/libraries/lib/wx/include/osx_cocoa-unicode-static-3.1', 
                GEODA_HOME + '/libraries/include/wx-3.1',
                GEODA_HOME + '/libraries/include/boost',
                GEODA_HOME + '/libraries/include/eigen3',
                GEODA_HOME + '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include',
                GEODA_HOME + '/libraries/include',
                ]

LIBRARY_DIRS = [GEODA_HOME + '/libraries/lib',GEODA_HOME + '/temp/boost_1_57_0/stage/lib']

LIBRARIES = ['boost_thread', 'boost_system', 'gdal', 'curl', 'cares',
            'expat',  'wxregexu-3.1', 'wxtiff-3.1', 'wxjpeg-3.1', 'wxpng-3.1', 'z', 'pthread', 'iconv']

SWIG_OPTS = ['-c++']

EXTRA_COMPILE_ARGS =  [ '-D_FILE_OFFSET_BITS=64', '-D__WXMAC__', '-D__WXOSX__', '-D__WXOSX_COCOA__']

EXTRA_OBJECTS = [GEODA_HOME + '/libraries/lib/libjson_spirit.a',
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_xrc-3.1.a', 
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_webview-3.1.a', 
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_qa-3.1.a', 
                 GEODA_HOME + '/libraries/lib/libwx_baseu_net-3.1.a',
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_html-3.1.a',
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_adv-3.1.a',
                 GEODA_HOME + '/libraries/lib/libwx_osx_cocoau_core-3.1.a',
                 GEODA_HOME + '/libraries/lib/libwx_baseu_xml-3.1.a',
                 GEODA_HOME + '/libraries/lib/libwx_baseu-3.1.a',
                 GEODA_HOME + '/libraries/lib/libcurl.a',
                 GEODA_HOME + '/temp/CLAPACK-3.2.1/lapack.a',
                 GEODA_HOME + '/temp/CLAPACK-3.2.1/libf2c.a',
                 GEODA_HOME + '/temp/CLAPACK-3.2.1/blas.a',
                 GEODA_HOME + '/temp/CLAPACK-3.2.1/tmglib.a']

GEODA_SOURCES = [
        '../DataViewer/DataSource.cpp',
        '../DialogTools/FieldNameCorrectionDlg.cpp',
        '../Explore/Basemap.cpp',
        '../ShapeOperations/AbstractShape.cpp', 
        '../ShapeOperations/BasePoint.cpp', 
        '../ShapeOperations/Box.cpp', 
        '../ShapeOperations/GwtWeight.cpp', 
        '../ShapeOperations/GalWeight.cpp', 
        '../ShapeOperations/GeodaWeight.cpp', 
        '../ShapeOperations/GdaCache.cpp', 
        '../ShapeOperations/OGRDataAdapter.cpp', 
        '../ShapeOperations/OGRDatasourceProxy.cpp', 
        '../ShapeOperations/OGRLayerProxy.cpp', 
        '../ShapeOperations/OGRFieldProxy.cpp', 
        '../ShapeOperations/PolysToContigWeights.cpp', 
        '../ShapeOperations/VoronoiUtils.cpp',
        '../VarCalc/NumericTests.cpp',
        '../GenGeomAlgs.cpp', 
        '../GdaConst.cpp', 
        '../GdaCartoDB.cpp', 
        '../GdaJson.cpp', 
        '../GdaShape.cpp', 
        '../GenUtils.cpp', 
        '../GeneralWxUtils.cpp', 
        '../ShpFile.cpp', 
        '../SpatialIndAlgs.cpp', 
        '../logger.cpp', 
    ]

SOURCE_FILES  = ['proxy_wrap.cxx', 'proxy.cpp'] + GEODA_SOURCES
 
extensions = [Extension('_geoda',
                        sources=SOURCE_FILES,
                        include_dirs=INCLUDE_DIRS,
                        swig_opts=[],
                        extra_compile_args=EXTRA_COMPILE_ARGS,
                        library_dirs=LIBRARY_DIRS,
                        runtime_library_dirs=LIBRARY_DIRS,
                        libraries=LIBRARIES,
                        extra_objects=EXTRA_OBJECTS),]

setup (name = 'GeoDa', version = '0.1', author = "Xun Li", description = """Python wrapper for GeoDa""",
       ext_modules = extensions, py_modules = ["geoda"],
      )

