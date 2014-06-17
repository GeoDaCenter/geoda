

include ../../../GDALmake.opt

FGDB_INC=-I/home/test/Downloads/FileGDB_API/include
EXTRA_LIBS+= -L/home/test/Downloads/FileGDB_API/lib -lFileGDBAPI
OBJ	=	FGdbDatasource.o FGdbDriver.o FGdbLayer.o FGdbUtils.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(FGDB_INC) $(CPPFLAGS)

default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_fgdb.h

plugin: $(OBJ)
	g++ -shared $(LNK_FLAGS) $(OBJ) $(CONFIG_LIBS_INS) $(EXTRA_LIBS) \
                -o ogr_FileGDB.so
