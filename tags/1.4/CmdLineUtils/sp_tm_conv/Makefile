APPNAME = sp_tm_conv
CC = g++
DEBUG = -g
CFLAGS = `wx-config --cxxflags core` $(DEBUG) -I/usr/local/include/boost
LFLAGS = `wx-config --libs core`

all: $(APPNAME)

all64: $(APPNAME)64

$(APPNAME)64 : $(APPNAME)64.o DbfFile64.o
	$(CC) -o $(APPNAME)64 $(APPNAME)64.o DbfFile64.o -m64 $(LFLAGS)

$(APPNAME)64.o : $(APPNAME).cpp
	$(CC) -o $(APPNAME)64.o -m64 $(CFLAGS) -c $(APPNAME).cpp

DbfFile64.o : ../../ShapeOperations/DbfFile.h ../../ShapeOperations/DbfFile.cpp
	$(CC) -o DbfFile64.o -m64 $(CFLAGS) -c ../../ShapeOperations/DbfFile.cpp

$(APPNAME) : $(APPNAME).o DbfFile.o
	$(CC) -o $(APPNAME) $(APPNAME).o DbfFile.o -m32 $(LFLAGS)

$(APPNAME).o : $(APPNAME).cpp
	$(CC) -m32 $(CFLAGS) -c $(APPNAME).cpp

DbfFile.o : ../../ShapeOperations/DbfFile.h ../../ShapeOperations/DbfFile.cpp
	$(CC) -o DbfFile.o -m32 $(CFLAGS) -c ../../ShapeOperations/DbfFile.cpp

clean:
	rm -f *.o $(APPNAME) $(APPNAME)64
