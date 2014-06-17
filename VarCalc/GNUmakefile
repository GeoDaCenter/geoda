
include ../GeoDamake.opt

#include ./file.lst

CPPFLAGS 	:=	$(CPPFLAGS)
CXXFLAGS 	:=	$(CXXFLAGS)

CXX_SRCS := $(wildcard *.cpp)
OBJ := ${CXX_SRCS:.cpp=.o}

default: $(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o 

