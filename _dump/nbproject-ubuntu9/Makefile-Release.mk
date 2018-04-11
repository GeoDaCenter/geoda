#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Release
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Explore/3DPlotView.o \
	${OBJECTDIR}/DialogTools/DataMovieDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o \
	${OBJECTDIR}/DialogTools/ExportCsvDlg.o \
	${OBJECTDIR}/Regression/DenseMatrix.o \
	${OBJECTDIR}/DialogTools/MapQuantileDlg.o \
	${OBJECTDIR}/DialogTools/CreateSpTmProjectDlg.o \
	${OBJECTDIR}/Explore/LisaCoordinator.o \
	${OBJECTDIR}/Regression/DiagnosticReport.o \
	${OBJECTDIR}/DialogTools/3DControlPan.o \
	${OBJECTDIR}/Explore/LisaScatterPlotView.o \
	${OBJECTDIR}/ShapeOperations/shp2gwt.o \
	${OBJECTDIR}/Explore/Geom3D.o \
	${OBJECTDIR}/DialogTools/CatClassifDlg.o \
	${OBJECTDIR}/rc/MyAppResources.o \
	${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o \
	${OBJECTDIR}/Explore/CatClassifManager.o \
	${OBJECTDIR}/ShapeOperations/RateSmoothing.o \
	${OBJECTDIR}/ShapeOperations/GeodaWeight.o \
	${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o \
	${OBJECTDIR}/DialogTools/PCPDlg.o \
	${OBJECTDIR}/logger.o \
	${OBJECTDIR}/DialogTools/ImportCsvDlg.o \
	${OBJECTDIR}/ShapeOperations/GwtWeight.o \
	${OBJECTDIR}/Explore/MapNewView.o \
	${OBJECTDIR}/ShapeOperations/ShpFile.o \
	${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o \
	${OBJECTDIR}/Regression/PowerLag.o \
	${OBJECTDIR}/GeneralWxUtils.o \
	${OBJECTDIR}/Regression/PowerSymLag.o \
	${OBJECTDIR}/DialogTools/OpenSpaceTimeDlg.o \
	${OBJECTDIR}/DialogTools/ASC2SHPDlg.o \
	${OBJECTDIR}/Regression/SparseVector.o \
	${OBJECTDIR}/Generic/HighlightState.o \
	${OBJECTDIR}/DialogTools/TimeVariantImportDlg.o \
	${OBJECTDIR}/Explore/CatClassification.o \
	${OBJECTDIR}/Explore/GetisOrdMapNewView.o \
	${OBJECTDIR}/DialogTools/RandomizationDlg.o \
	${OBJECTDIR}/ShapeOperations/DorlingCartogram.o \
	${OBJECTDIR}/DialogTools/VariableSettingsDlg.o \
	${OBJECTDIR}/DialogTools/SelectWeightDlg.o \
	${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o \
	${OBJECTDIR}/Explore/PCPNewView.o \
	${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o \
	${OBJECTDIR}/DataViewer/MergeTableDlg.o \
	${OBJECTDIR}/Generic/TestScrollWinView.o \
	${OBJECTDIR}/ShapeOperations/WeightsManager.o \
	${OBJECTDIR}/Explore/CartogramNewView.o \
	${OBJECTDIR}/GeoDaConst.o \
	${OBJECTDIR}/Explore/HistogramView.o \
	${OBJECTDIR}/Regression/smile2.o \
	${OBJECTDIR}/DataViewer/DbfGridTableBase.o \
	${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o \
	${OBJECTDIR}/Explore/ConditionalScatterPlotView.o \
	${OBJECTDIR}/DialogTools/CreateGridDlg.o \
	${OBJECTDIR}/GeoDa.o \
	${OBJECTDIR}/ShapeOperations/ShapeFile.o \
	${OBJECTDIR}/DialogTools/HistIntervalDlg.o \
	${OBJECTDIR}/DialogTools/PermutationCounterDlg.o \
	${OBJECTDIR}/DialogTools/RangeSelectionDlg.o \
	${OBJECTDIR}/ShapeOperations/DbfFile.o \
	${OBJECTDIR}/Generic/MyShape.o \
	${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o \
	${OBJECTDIR}/Regression/mix.o \
	${OBJECTDIR}/ShapeOperations/GalWeight.o \
	${OBJECTDIR}/Explore/ScatterNewPlotView.o \
	${OBJECTDIR}/kNN/kd_search.o \
	${OBJECTDIR}/DialogTools/AddIdVariable.o \
	${OBJECTDIR}/TemplateFrame.o \
	${OBJECTDIR}/Regression/SparseRow.o \
	${OBJECTDIR}/DialogTools/RegressionReportDlg.o \
	${OBJECTDIR}/kNN/kd_split.o \
	${OBJECTDIR}/Explore/ConditionalNewView.o \
	${OBJECTDIR}/ShapeOperations/shp.o \
	${OBJECTDIR}/ShapeOperations/CsvFileUtils.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o \
	${OBJECTDIR}/Explore/ConditionalMapView.o \
	${OBJECTDIR}/Explore/GStatCoordinator.o \
	${OBJECTDIR}/Regression/ML_im.o \
	${OBJECTDIR}/Explore/BoxNewPlotView.o \
	${OBJECTDIR}/Explore/ConnectivityHistView.o \
	${OBJECTDIR}/Regression/SparseMatrix.o \
	${OBJECTDIR}/DialogTools/ProgressDlg.o \
	${OBJECTDIR}/DataViewer/TableState.o \
	${OBJECTDIR}/kNN/ANN.o \
	${OBJECTDIR}/ShapeOperations/shp2cnt.o \
	${OBJECTDIR}/DialogTools/RegressionDlg.o \
	${OBJECTDIR}/GenGeomAlgs.o \
	${OBJECTDIR}/kNN/kd_pr_search.o \
	${OBJECTDIR}/Project.o \
	${OBJECTDIR}/ShapeOperations/VoronoiUtils.o \
	${OBJECTDIR}/GenUtils.o \
	${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o \
	${OBJECTDIR}/kNN/kd_util.o \
	${OBJECTDIR}/DialogTools/TimeChooserDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o \
	${OBJECTDIR}/ShapeOperations/ShapeUtils.o \
	${OBJECTDIR}/ShapeOperations/Randik.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o \
	${OBJECTDIR}/DialogTools/Statistics.o \
	${OBJECTDIR}/NewTableViewer.o \
	${OBJECTDIR}/DialogTools/SaveToTableDlg.o \
	${OBJECTDIR}/TemplateLegend.o \
	${OBJECTDIR}/FramesManager.o \
	${OBJECTDIR}/Explore/LisaMapNewView.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o \
	${OBJECTDIR}/ShapeOperations/Box.o \
	${OBJECTDIR}/Regression/Weights.o \
	${OBJECTDIR}/DataViewer/DataViewer.o \
	${OBJECTDIR}/DialogTools/SHP2ASCDlg.o \
	${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o \
	${OBJECTDIR}/kNN/kNN.o \
	${OBJECTDIR}/Explore/CatClassifState.o \
	${OBJECTDIR}/Regression/DenseVector.o \
	${OBJECTDIR}/TemplateCanvas.o \
	${OBJECTDIR}/ShapeOperations/DBF.o \
	${OBJECTDIR}/kNN/kd_tree.o \
	${OBJECTDIR}/DialogTools/SaveSelectionDlg.o \
	${OBJECTDIR}/DialogTools/CreatingWeightDlg.o \
	${OBJECTDIR}/ShapeOperations/BasePoint.o \
	${OBJECTDIR}/ShapeOperations/AbstractShape.o \
	${OBJECTDIR}/Explore/ConditionalHistogramView.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pthread
CXXFLAGS=-pthread

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -pthread ${HOME}/CLAPACK-3.2/lapack.a ${HOME}/CLAPACK-3.2/blas.a ${HOME}/CLAPACK-3.2/F2CLIBS/libf2c.a /usr/local/lib/libwx_gtk2u_gl-2.9.a -lGL -lGLU /usr/local/lib/libwx_gtk2u_richtext-2.9.a /usr/local/lib/libwx_gtk2u_xrc-2.9.a /usr/local/lib/libwx_gtk2u_qa-2.9.a /usr/local/lib/libwx_gtk2u_html-2.9.a /usr/local/lib/libwx_gtk2u_adv-2.9.a /usr/local/lib/libwx_gtk2u_core-2.9.a /usr/local/lib/libwx_baseu_xml-2.9.a /usr/local/lib/libwx_baseu_net-2.9.a /usr/local/lib/libwx_baseu-2.9.a -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lXinerama -lSM -lwxtiff-2.9 -lwxjpeg-2.9 -lwxpng-2.9 -lexpat -lz -ldl -lm

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-Release.mk dist/Release/GNU-Linux-x86/trac_trunk

dist/Release/GNU-Linux-x86/trac_trunk: ${HOME}/CLAPACK-3.2/lapack.a

dist/Release/GNU-Linux-x86/trac_trunk: ${HOME}/CLAPACK-3.2/blas.a

dist/Release/GNU-Linux-x86/trac_trunk: ${HOME}/CLAPACK-3.2/F2CLIBS/libf2c.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_gl-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_richtext-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_xrc-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_qa-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_html-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_adv-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_gtk2u_core-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_baseu_xml-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_baseu_net-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: /usr/local/lib/libwx_baseu-2.9.a

dist/Release/GNU-Linux-x86/trac_trunk: ${OBJECTFILES}
	${MKDIR} -p dist/Release/GNU-Linux-x86
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trac_trunk ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Explore/3DPlotView.o: Explore/3DPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/3DPlotView.o Explore/3DPlotView.cpp

${OBJECTDIR}/DialogTools/DataMovieDlg.o: DialogTools/DataMovieDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/DataMovieDlg.o DialogTools/DataMovieDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o: DialogTools/FieldNewCalcLagDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o DialogTools/FieldNewCalcLagDlg.cpp

${OBJECTDIR}/DialogTools/ExportCsvDlg.o: DialogTools/ExportCsvDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ExportCsvDlg.o DialogTools/ExportCsvDlg.cpp

${OBJECTDIR}/Regression/DenseMatrix.o: Regression/DenseMatrix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DenseMatrix.o Regression/DenseMatrix.cpp

${OBJECTDIR}/DialogTools/MapQuantileDlg.o: DialogTools/MapQuantileDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/MapQuantileDlg.o DialogTools/MapQuantileDlg.cpp

${OBJECTDIR}/DialogTools/CreateSpTmProjectDlg.o: DialogTools/CreateSpTmProjectDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CreateSpTmProjectDlg.o DialogTools/CreateSpTmProjectDlg.cpp

${OBJECTDIR}/Explore/LisaCoordinator.o: Explore/LisaCoordinator.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaCoordinator.o Explore/LisaCoordinator.cpp

${OBJECTDIR}/Regression/DiagnosticReport.o: Regression/DiagnosticReport.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DiagnosticReport.o Regression/DiagnosticReport.cpp

${OBJECTDIR}/DialogTools/3DControlPan.o: DialogTools/3DControlPan.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/3DControlPan.o DialogTools/3DControlPan.cpp

${OBJECTDIR}/Explore/LisaScatterPlotView.o: Explore/LisaScatterPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaScatterPlotView.o Explore/LisaScatterPlotView.cpp

${OBJECTDIR}/ShapeOperations/shp2gwt.o: ShapeOperations/shp2gwt.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp2gwt.o ShapeOperations/shp2gwt.cpp

${OBJECTDIR}/Explore/Geom3D.o: Explore/Geom3D.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/Geom3D.o Explore/Geom3D.cpp

${OBJECTDIR}/DialogTools/CatClassifDlg.o: DialogTools/CatClassifDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CatClassifDlg.o DialogTools/CatClassifDlg.cpp

${OBJECTDIR}/rc/MyAppResources.o: rc/MyAppResources.cpp 
	${MKDIR} -p ${OBJECTDIR}/rc
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/rc/MyAppResources.o rc/MyAppResources.cpp

${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o: DataViewer/DataViewerResizeColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o DataViewer/DataViewerResizeColDlg.cpp

${OBJECTDIR}/Explore/CatClassifManager.o: Explore/CatClassifManager.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/CatClassifManager.o Explore/CatClassifManager.cpp

${OBJECTDIR}/ShapeOperations/RateSmoothing.o: ShapeOperations/RateSmoothing.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/RateSmoothing.o ShapeOperations/RateSmoothing.cpp

${OBJECTDIR}/ShapeOperations/GeodaWeight.o: ShapeOperations/GeodaWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GeodaWeight.o ShapeOperations/GeodaWeight.cpp

${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o: DialogTools/LisaWhat2OpenDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o DialogTools/LisaWhat2OpenDlg.cpp

${OBJECTDIR}/DialogTools/PCPDlg.o: DialogTools/PCPDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/PCPDlg.o DialogTools/PCPDlg.cpp

${OBJECTDIR}/logger.o: logger.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/logger.o logger.cpp

${OBJECTDIR}/DialogTools/ImportCsvDlg.o: DialogTools/ImportCsvDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ImportCsvDlg.o DialogTools/ImportCsvDlg.cpp

${OBJECTDIR}/ShapeOperations/GwtWeight.o: ShapeOperations/GwtWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GwtWeight.o ShapeOperations/GwtWeight.cpp

${OBJECTDIR}/Explore/MapNewView.o: Explore/MapNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/MapNewView.o Explore/MapNewView.cpp

${OBJECTDIR}/ShapeOperations/ShpFile.o: ShapeOperations/ShpFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShpFile.o ShapeOperations/ShpFile.cpp

${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o: ShapeOperations/ShapeFileTriplet.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o ShapeOperations/ShapeFileTriplet.cpp

${OBJECTDIR}/Regression/PowerLag.o: Regression/PowerLag.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/PowerLag.o Regression/PowerLag.cpp

${OBJECTDIR}/GeneralWxUtils.o: GeneralWxUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeneralWxUtils.o GeneralWxUtils.cpp

${OBJECTDIR}/Regression/PowerSymLag.o: Regression/PowerSymLag.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/PowerSymLag.o Regression/PowerSymLag.cpp

${OBJECTDIR}/DialogTools/OpenSpaceTimeDlg.o: DialogTools/OpenSpaceTimeDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/OpenSpaceTimeDlg.o DialogTools/OpenSpaceTimeDlg.cpp

${OBJECTDIR}/DialogTools/ASC2SHPDlg.o: DialogTools/ASC2SHPDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ASC2SHPDlg.o DialogTools/ASC2SHPDlg.cpp

${OBJECTDIR}/Regression/SparseVector.o: Regression/SparseVector.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseVector.o Regression/SparseVector.cpp

${OBJECTDIR}/Generic/HighlightState.o: Generic/HighlightState.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/HighlightState.o Generic/HighlightState.cpp

${OBJECTDIR}/DialogTools/TimeVariantImportDlg.o: DialogTools/TimeVariantImportDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/TimeVariantImportDlg.o DialogTools/TimeVariantImportDlg.cpp

${OBJECTDIR}/Explore/CatClassification.o: Explore/CatClassification.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/CatClassification.o Explore/CatClassification.cpp

${OBJECTDIR}/Explore/GetisOrdMapNewView.o: Explore/GetisOrdMapNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/GetisOrdMapNewView.o Explore/GetisOrdMapNewView.cpp

${OBJECTDIR}/DialogTools/RandomizationDlg.o: DialogTools/RandomizationDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RandomizationDlg.o DialogTools/RandomizationDlg.cpp

${OBJECTDIR}/ShapeOperations/DorlingCartogram.o: ShapeOperations/DorlingCartogram.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/DorlingCartogram.o ShapeOperations/DorlingCartogram.cpp

${OBJECTDIR}/DialogTools/VariableSettingsDlg.o: DialogTools/VariableSettingsDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/VariableSettingsDlg.o DialogTools/VariableSettingsDlg.cpp

${OBJECTDIR}/DialogTools/SelectWeightDlg.o: DialogTools/SelectWeightDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SelectWeightDlg.o DialogTools/SelectWeightDlg.cpp

${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o: DataViewer/DataViewerDeleteColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o DataViewer/DataViewerDeleteColDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o: DialogTools/FieldNewCalcUniDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o DialogTools/FieldNewCalcUniDlg.cpp

${OBJECTDIR}/Explore/PCPNewView.o: Explore/PCPNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/PCPNewView.o Explore/PCPNewView.cpp

${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o: ShapeOperations/ShapeFileHdr.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o ShapeOperations/ShapeFileHdr.cpp

${OBJECTDIR}/DataViewer/MergeTableDlg.o: DataViewer/MergeTableDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/MergeTableDlg.o DataViewer/MergeTableDlg.cpp

${OBJECTDIR}/Generic/TestScrollWinView.o: Generic/TestScrollWinView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/TestScrollWinView.o Generic/TestScrollWinView.cpp

${OBJECTDIR}/ShapeOperations/WeightsManager.o: ShapeOperations/WeightsManager.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/WeightsManager.o ShapeOperations/WeightsManager.cpp

${OBJECTDIR}/Explore/CartogramNewView.o: Explore/CartogramNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/CartogramNewView.o Explore/CartogramNewView.cpp

${OBJECTDIR}/GeoDaConst.o: GeoDaConst.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeoDaConst.o GeoDaConst.cpp

${OBJECTDIR}/Explore/HistogramView.o: Explore/HistogramView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/HistogramView.o Explore/HistogramView.cpp

${OBJECTDIR}/Regression/smile2.o: Regression/smile2.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/smile2.o Regression/smile2.cpp

${OBJECTDIR}/DataViewer/DbfGridTableBase.o: DataViewer/DbfGridTableBase.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DbfGridTableBase.o DataViewer/DbfGridTableBase.cpp

${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o: DialogTools/GetisOrdChoiceDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o DialogTools/GetisOrdChoiceDlg.cpp

${OBJECTDIR}/Explore/ConditionalScatterPlotView.o: Explore/ConditionalScatterPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConditionalScatterPlotView.o Explore/ConditionalScatterPlotView.cpp

${OBJECTDIR}/DialogTools/CreateGridDlg.o: DialogTools/CreateGridDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CreateGridDlg.o DialogTools/CreateGridDlg.cpp

${OBJECTDIR}/GeoDa.o: GeoDa.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeoDa.o GeoDa.cpp

${OBJECTDIR}/ShapeOperations/ShapeFile.o: ShapeOperations/ShapeFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFile.o ShapeOperations/ShapeFile.cpp

${OBJECTDIR}/DialogTools/HistIntervalDlg.o: DialogTools/HistIntervalDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/HistIntervalDlg.o DialogTools/HistIntervalDlg.cpp

${OBJECTDIR}/DialogTools/PermutationCounterDlg.o: DialogTools/PermutationCounterDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/PermutationCounterDlg.o DialogTools/PermutationCounterDlg.cpp

${OBJECTDIR}/DialogTools/RangeSelectionDlg.o: DialogTools/RangeSelectionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RangeSelectionDlg.o DialogTools/RangeSelectionDlg.cpp

${OBJECTDIR}/ShapeOperations/DbfFile.o: ShapeOperations/DbfFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/DbfFile.o ShapeOperations/DbfFile.cpp

${OBJECTDIR}/Generic/MyShape.o: Generic/MyShape.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/MyShape.o Generic/MyShape.cpp

${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o: DataViewer/DataViewerAddColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o DataViewer/DataViewerAddColDlg.cpp

${OBJECTDIR}/Regression/mix.o: Regression/mix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/mix.o Regression/mix.cpp

${OBJECTDIR}/ShapeOperations/GalWeight.o: ShapeOperations/GalWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GalWeight.o ShapeOperations/GalWeight.cpp

${OBJECTDIR}/Explore/ScatterNewPlotView.o: Explore/ScatterNewPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ScatterNewPlotView.o Explore/ScatterNewPlotView.cpp

${OBJECTDIR}/kNN/kd_search.o: kNN/kd_search.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_search.o kNN/kd_search.cpp

${OBJECTDIR}/DialogTools/AddIdVariable.o: DialogTools/AddIdVariable.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/AddIdVariable.o DialogTools/AddIdVariable.cpp

${OBJECTDIR}/TemplateFrame.o: TemplateFrame.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateFrame.o TemplateFrame.cpp

${OBJECTDIR}/Regression/SparseRow.o: Regression/SparseRow.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseRow.o Regression/SparseRow.cpp

${OBJECTDIR}/DialogTools/RegressionReportDlg.o: DialogTools/RegressionReportDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RegressionReportDlg.o DialogTools/RegressionReportDlg.cpp

${OBJECTDIR}/kNN/kd_split.o: kNN/kd_split.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_split.o kNN/kd_split.cpp

${OBJECTDIR}/Explore/ConditionalNewView.o: Explore/ConditionalNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConditionalNewView.o Explore/ConditionalNewView.cpp

${OBJECTDIR}/ShapeOperations/shp.o: ShapeOperations/shp.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp.o ShapeOperations/shp.cpp

${OBJECTDIR}/ShapeOperations/CsvFileUtils.o: ShapeOperations/CsvFileUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/CsvFileUtils.o ShapeOperations/CsvFileUtils.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o: DialogTools/FieldNewCalcSheetDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o DialogTools/FieldNewCalcSheetDlg.cpp

${OBJECTDIR}/Explore/ConditionalMapView.o: Explore/ConditionalMapView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConditionalMapView.o Explore/ConditionalMapView.cpp

${OBJECTDIR}/Explore/GStatCoordinator.o: Explore/GStatCoordinator.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/GStatCoordinator.o Explore/GStatCoordinator.cpp

${OBJECTDIR}/Regression/ML_im.o: Regression/ML_im.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/ML_im.o Regression/ML_im.cpp

${OBJECTDIR}/Explore/BoxNewPlotView.o: Explore/BoxNewPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/BoxNewPlotView.o Explore/BoxNewPlotView.cpp

${OBJECTDIR}/Explore/ConnectivityHistView.o: Explore/ConnectivityHistView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConnectivityHistView.o Explore/ConnectivityHistView.cpp

${OBJECTDIR}/Regression/SparseMatrix.o: Regression/SparseMatrix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseMatrix.o Regression/SparseMatrix.cpp

${OBJECTDIR}/DialogTools/ProgressDlg.o: DialogTools/ProgressDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ProgressDlg.o DialogTools/ProgressDlg.cpp

${OBJECTDIR}/DataViewer/TableState.o: DataViewer/TableState.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/TableState.o DataViewer/TableState.cpp

${OBJECTDIR}/kNN/ANN.o: kNN/ANN.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/ANN.o kNN/ANN.cpp

${OBJECTDIR}/ShapeOperations/shp2cnt.o: ShapeOperations/shp2cnt.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp2cnt.o ShapeOperations/shp2cnt.cpp

${OBJECTDIR}/DialogTools/RegressionDlg.o: DialogTools/RegressionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RegressionDlg.o DialogTools/RegressionDlg.cpp

${OBJECTDIR}/GenGeomAlgs.o: GenGeomAlgs.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GenGeomAlgs.o GenGeomAlgs.cpp

${OBJECTDIR}/kNN/kd_pr_search.o: kNN/kd_pr_search.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_pr_search.o kNN/kd_pr_search.cpp

${OBJECTDIR}/Project.o: Project.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Project.o Project.cpp

${OBJECTDIR}/ShapeOperations/VoronoiUtils.o: ShapeOperations/VoronoiUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/VoronoiUtils.o ShapeOperations/VoronoiUtils.cpp

${OBJECTDIR}/GenUtils.o: GenUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GenUtils.o GenUtils.cpp

${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o: DataViewer/DataViewerEditFieldPropertiesDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o DataViewer/DataViewerEditFieldPropertiesDlg.cpp

${OBJECTDIR}/kNN/kd_util.o: kNN/kd_util.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_util.o kNN/kd_util.cpp

${OBJECTDIR}/DialogTools/TimeChooserDlg.o: DialogTools/TimeChooserDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/TimeChooserDlg.o DialogTools/TimeChooserDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o: DialogTools/FieldNewCalcSpecialDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o DialogTools/FieldNewCalcSpecialDlg.cpp

${OBJECTDIR}/ShapeOperations/ShapeUtils.o: ShapeOperations/ShapeUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeUtils.o ShapeOperations/ShapeUtils.cpp

${OBJECTDIR}/ShapeOperations/Randik.o: ShapeOperations/Randik.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/Randik.o ShapeOperations/Randik.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o: DialogTools/FieldNewCalcRateDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o DialogTools/FieldNewCalcRateDlg.cpp

${OBJECTDIR}/DialogTools/Statistics.o: DialogTools/Statistics.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/Statistics.o DialogTools/Statistics.cpp

${OBJECTDIR}/NewTableViewer.o: NewTableViewer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/NewTableViewer.o NewTableViewer.cpp

${OBJECTDIR}/DialogTools/SaveToTableDlg.o: DialogTools/SaveToTableDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SaveToTableDlg.o DialogTools/SaveToTableDlg.cpp

${OBJECTDIR}/TemplateLegend.o: TemplateLegend.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateLegend.o TemplateLegend.cpp

${OBJECTDIR}/FramesManager.o: FramesManager.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/FramesManager.o FramesManager.cpp

${OBJECTDIR}/Explore/LisaMapNewView.o: Explore/LisaMapNewView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaMapNewView.o Explore/LisaMapNewView.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o: DialogTools/FieldNewCalcBinDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o DialogTools/FieldNewCalcBinDlg.cpp

${OBJECTDIR}/ShapeOperations/Box.o: ShapeOperations/Box.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/Box.o ShapeOperations/Box.cpp

${OBJECTDIR}/Regression/Weights.o: Regression/Weights.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/Weights.o Regression/Weights.cpp

${OBJECTDIR}/DataViewer/DataViewer.o: DataViewer/DataViewer.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewer.o DataViewer/DataViewer.cpp

${OBJECTDIR}/DialogTools/SHP2ASCDlg.o: DialogTools/SHP2ASCDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SHP2ASCDlg.o DialogTools/SHP2ASCDlg.cpp

${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o: DialogTools/Bnd2ShpDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o DialogTools/Bnd2ShpDlg.cpp

${OBJECTDIR}/kNN/kNN.o: kNN/kNN.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kNN.o kNN/kNN.cpp

${OBJECTDIR}/Explore/CatClassifState.o: Explore/CatClassifState.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/CatClassifState.o Explore/CatClassifState.cpp

${OBJECTDIR}/Regression/DenseVector.o: Regression/DenseVector.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DenseVector.o Regression/DenseVector.cpp

${OBJECTDIR}/TemplateCanvas.o: TemplateCanvas.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateCanvas.o TemplateCanvas.cpp

${OBJECTDIR}/ShapeOperations/DBF.o: ShapeOperations/DBF.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/DBF.o ShapeOperations/DBF.cpp

${OBJECTDIR}/kNN/kd_tree.o: kNN/kd_tree.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_tree.o kNN/kd_tree.cpp

${OBJECTDIR}/DialogTools/SaveSelectionDlg.o: DialogTools/SaveSelectionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SaveSelectionDlg.o DialogTools/SaveSelectionDlg.cpp

${OBJECTDIR}/DialogTools/CreatingWeightDlg.o: DialogTools/CreatingWeightDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CreatingWeightDlg.o DialogTools/CreatingWeightDlg.cpp

${OBJECTDIR}/ShapeOperations/BasePoint.o: ShapeOperations/BasePoint.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/BasePoint.o ShapeOperations/BasePoint.cpp

${OBJECTDIR}/ShapeOperations/AbstractShape.o: ShapeOperations/AbstractShape.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/AbstractShape.o ShapeOperations/AbstractShape.cpp

${OBJECTDIR}/Explore/ConditionalHistogramView.o: Explore/ConditionalHistogramView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConditionalHistogramView.o Explore/ConditionalHistogramView.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Release
	${RM} dist/Release/GNU-Linux-x86/trac_trunk

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
