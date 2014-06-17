
include GeoDamake.opt

CPPFLAGS 	:=	$(CPPFLAGS)
CXXFLAGS 	:=	$(CXXFLAGS)

CXX_SRCS := $(wildcard *.cpp)
OBJ := ${CXX_SRCS:.cpp=.o}

default: $(T_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o 

