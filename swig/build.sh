
cp OGRLayerProxy.cpp ../ShapeOperations/OGRLayerProxy.cpp

swig -python -c++ -threads proxy.i

CFLAGS='-Wall -O0 -DDEBUG' python setup_centos.py build_ext --inplace --force

