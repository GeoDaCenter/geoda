

include ../../../GDALmake.opt

SDE_HOME = /home/test/esri/sdeexe101
EXTRA_LIBS += -L$(SDE_HOME)/lib -lpe -lsde -lsg

LNK_FLAGS += -m64

OBJ	=	ogrsdedriver.o ogrsdedatasource.o ogrsdelayer.o

CPPFLAGS	:= -m64 -fPIC	$(GDAL_INCLUDE) -I$(SDE_HOME)/include $(CPPFLAGS)

default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_sde.h

plugin: $(OBJ)
	g++ -shared $(LNK_FLAGS) $(OBJ) $(CONFIG_LIBS_INS) $(EXTRA_LIBS) \
		-o ogr_SDE.so
