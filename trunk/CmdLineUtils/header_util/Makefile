APPNAME = header_util
CC = g++
DEBUG = -g
CFLAGS = `wx-config --cxxflags core`
LFLAGS = `wx-config --libs core`

TEST_FILE_LIST = foo_dir/a_dir/file1.h foo_dir/a_dir/file1.cpp \
	foo_dir/a_dir/file2.h foo_dir/a_dir/file2.cpp \
	foo_dir/a_dir/file3.h foo_dir/a_dir/file3.cpp \
	foo_dir/b_dir/file1.h foo_dir/b_dir/file1.cpp \
	foo_dir/b_dir/file2.h foo_dir/b_dir/file2.cpp \
	foo_dir/b_dir/file3.h foo_dir/b_dir/file3.cpp

all: $(APPNAME)

all64: $(APPNAME)64

$(APPNAME)64 : $(APPNAME)64.o
	$(CC) -o $(APPNAME)64 $(APPNAME)64.o -m64 $(LFLAGS)

$(APPNAME)64.o : $(APPNAME).cpp
	$(CC) -o $(APPNAME)64.o -m64 $(CFLAGS) -c $(APPNAME).cpp

$(APPNAME) : $(APPNAME).o
	$(CC) -o $(APPNAME) $(APPNAME).o -m32 $(LFLAGS)

$(APPNAME).o : $(APPNAME).cpp
	$(CC) -m32 $(CFLAGS) -c $(APPNAME).cpp

reset:
	rm -f $(TEST_FILE_LIST)
	touch $(TEST_FILE_LIST)
	
clean:
	rm -f *.o $(APPNAME) $(APPNAME)64
