
include ../GeoDamake.opt

#include ./file.lst

CPPFLAGS 	:=	$(CPPFLAGS)
CXXFLAGS 	:=	$(CXXFLAGS)

C_SRCS := $(wildcard *.c)
CXX_SRCS := $(wildcard *.cpp)

OBJ := ${CXX_SRCS:.cpp=.o} ${C_SRCS:.c=.o}

default: $(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o 

