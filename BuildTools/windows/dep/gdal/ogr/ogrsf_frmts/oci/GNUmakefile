

include ../../../GDALmake.opt
GDAL_INCLUDE += -I$(ORACLE_HOME)/sdk/include
LNK_FLAGS += -arch x86_64
EXTRA_LIBS += -L$(ORACLE_HOME)/lib -lclntsh -lnnz11

OBJ	=	oci_utils.o ogrocisession.o ogrocistatement.o \
		ogrocidriver.o ogrocidatasource.o ogrocilayer.o \
		ogrocitablelayer.o ogrociselectlayer.o ogrocistringbuf.o \
		ogrociwritablelayer.o ogrociloaderlayer.o ogrocistroke.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(OCI_INCLUDE) $(CPPFLAGS) -fPIC

PLUGIN_SO	=	ogr_OCI.so

default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_oci.h 

fastload:	fastload.cpp
	$(CXX) $(CPPFLAGS) $(CFLAGS) fastload.cpp -L../../.. -lgdal.1.1 \
		-o fastload

plugin: $(PLUGIN_SO)

$(PLUGIN_SO):	$(OBJ)
	g++ -shared $(LNK_FLAGS) $(OBJ) $(CONFIG_LIBS_INS) $(EXTRA_LIBS) \
		-o $(PLUGIN_SO)
