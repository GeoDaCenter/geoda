
include ../GeoDamake.opt

#include ./file.lst

CPPFLAGS 	:=	$(CPPFLAGS)
CXXFLAGS 	:=	$(CXXFLAGS)

CXX_SRCS := $(wildcard *.cpp)
OBJ := ${CXX_SRCS:.cpp=.o}

default: $(O_OBJ:.o=.$(OBJ_EXT))

xrc: GeoDaApp

GeoDaApp: dialogs.xrc data_viewer_dialogs.xrc menus.xrc toolbar.xrc panel.xrc
	$(GEODA_HOME)/libraries/bin/wxrc dialogs.xrc data_viewer_dialogs.xrc menus.xrc toolbar.xrc panel.xrc --cpp-code --output=GdaAppResources.cpp --function=GdaInitXmlResource

clean:
	rm -f *.o 

