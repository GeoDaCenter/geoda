```
sudo yum install python-devel
```

Location of python headers: /usr/include/python2.7

File list:

-proxy.h
-proxy.cpp

First, create "proxy.i" file based on proxy.h.
```c++
%module geoda

%include "std_string.i"

%{
#include "proxy.h"
#include <wx/wx.h>
%}

bool CreateQueenWeights(std::string in_file, std::string out_file, int order);
```

Second, create python wrapper file using swig:

```
swig -python -cxx proxy.i
```

Following files will be created:

-proxy_wrap.cxx
-geoda.py

To test if the files can be compiled:
```
gcc -I/home/xun/geoda/BuildTools/centos/libraries/lib/wx/include/gtk3-unicode-static-3.1 -I/home/xun/geoda/BuildTools/centos/libraries/include/wx-3.1 -D_FILE_OFFSET_BITS=64 -DwxDEBUG_LEVEL=0 -D__WXGTK__ -pthread -I/usr/include/python2.7 -c proxy.cpp proxy_wrap.cxx
```

Run the following command to create dynamic wrapper
```
python setup.py build_ext --inplace
CFLAGS='-Wall -O0 -DDEBUG' python setup.py build_ext --inplace --force
```

Test cases
```
python -c 'import geoda;geoda.CreateQueenWeights("nat.shp","nat.gal",1,False);'

python -c 'import geoda;geoda.CreateRookWeights("nat.shp","nat_rook.gal",1,False);'

python -c 'import geoda;geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4);'

python -c 'import geoda;geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4, True, False);'

```
 



