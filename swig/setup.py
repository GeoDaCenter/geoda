#!/usr/bin/env python

from distutils.core import setup, Extension

GEODA_HOME = '/Users/xun/geoda_trunk/BuildTools/macosx'

PYTHON_HEADER = '-I/usr/include/python2.7'

INCLUDE_DIRS = [GEODA_HOME + '/libraries/lib/wx/include/osx_cocoa-unicode-static-3.1', 
                GEODA_HOME + '/libraries/include/wx-3.1',
                GEODA_HOME + '/libraries/include/boost',
                GEODA_HOME + '/libraries/include/eigen3',
                GEODA_HOME + '/libraries/include',
                ]

LIBRARY_DIRS = [GEODA_HOME + '/libraries/lib',GEODA_HOME + '/temp/boost_1_57_0/stage/lib']

LIBRARIES = ['wx_gtk3u_xrc-3.1', 'wx_gtk3u_stc-3.1','wx_gtk3u_richtext-3.1', 'wx_gtk3u_ribbon-3.1', 
            'wx_gtk3u_propgrid-3.1', 'wx_gtk3u_aui-3.1', 'wx_gtk3u_gl-3.1','wx_gtk3u_webview-3.1',
            'wx_gtk3u_qa-3.1','wx_baseu_net-3.1','wx_gtk3u_html-3.1', 'wx_gtk3u_adv-3.1', 'wx_gtk3u_core-3.1',
            'wx_baseu_xml-3.1', 'wx_baseu-3.1', 'wxscintilla-3.1', 'boost_thread', 'boost_system',
            'gdal', 'curl', 'cares',
            'pthread', 'GL', 'GLU', 'gthread-2.0', 'X11', 'Xxf86vm', 'gtk-3', 'gdk-3', 'atk-1.0',
            'gio-2.0', 'pangocairo-1.0', 'gdk_pixbuf-2.0', 'cairo-gobject', 'pango-1.0', 'cairo', 'gobject-2.0', 
            'glib-2.0', 'png', 'expat', 'wxregexu-3.1', 'wxjpeg-3.1', 'z', 'dl', 'm', 'idn', 'rt', 'ssl', 'webkitgtk-3.0']

SWIG_OPTS = ['-c++']

EXTRA_COMPILE_ARGS =  [ '-D_FILE_OFFSET_BITS=64', '-DwxDEBUG_LEVEL=0', '-D__WXGTK__' ]

EXTRA_OBJECTS = [GEODA_HOME + '/libraries/lib/libjson_spirit.a',
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

