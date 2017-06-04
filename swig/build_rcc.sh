
cp OGRLayerProxy.cpp ../ShapeOperations/OGRLayerProxy.cpp

CFLAGS='-Wall -O0 -DDEBUG' python setup_rcc.py build_ext --inplace --force

