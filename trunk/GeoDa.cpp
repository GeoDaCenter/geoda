/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#undef check // undefine needed for Xcode compilation and Boost.Geometry
#include <iostream>
#include <sstream>
#include <string>

#include <boost/foreach.hpp>

#include <wx/wxprec.h>
#include <wx/valtext.h>
#include <wx/image.h>
#include <wx/dcsvg.h>		// SVG DC
#include <wx/dcps.h>		// PostScript DC
#include <wx/dcmemory.h>	// Memory DC
#include <wx/xrc/xmlres.h>	// XRC XML resouces
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/app.h>
#include <wx/sysopt.h>
#include <wx/position.h>
#include <wx/progdlg.h>
#include <wx/filefn.h> // for wxCopyFile and wxFileExists
#include <wx/msgdlg.h>

#include "DataViewer/DbfGridTableBase.h"
#include "DataViewer/DataViewerAddColDlg.h"
#include "DataViewer/DataViewerDeleteColDlg.h"
#include "DataViewer/DataViewerEditFieldPropertiesDlg.h"
#include "DataViewer/MergeTableDlg.h"
#include "DataViewer/TableState.h"

#include "DialogTools/CatClassifDlg.h"
#include "DialogTools/PCPDlg.h"
#include "DialogTools/FieldNewCalcSheetDlg.h"
#include "DialogTools/CreateSpTmProjectDlg.h"
#include "DialogTools/DataMovieDlg.h"
#include "DialogTools/ExportCsvDlg.h"
#include "DialogTools/ImportCsvDlg.h"
#include "DialogTools/RangeSelectionDlg.h"
#include "DialogTools/OpenSpaceTimeDlg.h"
#include "DialogTools/TimeChooserDlg.h"
#include "DialogTools/TimeVariantImportDlg.h"
#include "DialogTools/VariableSettingsDlg.h"
#include "DialogTools/Statistics.h"
#include "DialogTools/SaveSelectionDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/LisaWhat2OpenDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/ProgressDlg.h"
#include "DialogTools/GetisOrdChoiceDlg.h"

#include "Explore/CatClassification.h"
#include "Explore/GetisOrdMapNewView.h"
#include "Explore/LisaMapNewView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/ConditionalMapView.h"
#include "Explore/ConditionalNewView.h"
#include "Explore/ConditionalScatterPlotView.h"
#include "Explore/ConnectivityHistView.h"
#include "Explore/ConditionalHistogramView.h"
#include "Explore/CartogramNewView.h"
#include "Explore/GStatCoordinator.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/MapNewView.h"
#include "Explore/PCPNewView.h"
#include "Explore/HistogramView.h"
#include "Explore/BoxNewPlotView.h"
#include "Explore/3DPlotView.h"

//#include "Generic/TestMapView.h"
//#include "Generic/TestTableView.h"
//#include "Generic/TestScrollWinView.h"

#include "Regression/DiagnosticReport.h"

#include "ShapeOperations/CsvFileUtils.h"
#include "ShapeOperations/GalWeight.h"
#include "ShapeOperations/ShpFile.h"
#include "ShapeOperations/ShapeUtils.h"
#include "ShapeOperations/shp2cnt.h" // only needed for IsLineShapeFile
#include "ShapeOperations/WeightsManager.h"

#include "FramesManager.h"
#include "GeoDaConst.h"
#include "GeneralWxUtils.h"
#include "GenUtils.h"
#include "logger.h"
#include "NewTableViewer.h"
#include "Project.h"
#include "TemplateFrame.h"

#include "GeoDa.h"

// The following is defined in rc/MyAppResouces.cpp.  This file was
// compiled with:
/*
 wxrc dialogs.xrc menus.xrc toolbar.xrc \
   --cpp-code --output=MyAppResources.cpp --function=MyInitXmlResource
*/
// and combines all resouces file into single source file that is linked into
// the application binary.
extern void MyInitXmlResource();

MyFrame* MyFrame::theFrame = 0;
bool MyFrame::projectOpen = false;

const int ID_TEST_MAP_FRAME = wxID_HIGHEST + 10;
const int ID_TEST_TABLE_FRAME = wxID_HIGHEST + 11;

IMPLEMENT_APP(MyApp)

//MyApp::MyApp(void)
//{
//	LOG_MSG("Entering MyApp::MyApp");
	//Don't call wxHandleFatalExceptions so that a core dump file will be
	//produced for debugging.
	//wxHandleFatalExceptions();
//	LOG_MSG("Exiting MyApp::MyApp");
//}

//wxTimer* MyApp::timer = NULL;

//void MyApp::OnTimer(wxTimerEvent& event)
//{
	// This method is called once every second
	//ExitMainLoop(); // This will force the program to exit immediately
//}

#include "rc/GeoDaIcon-16x16.xpm"

bool MyApp::OnInit(void)
{
	LOG_MSG("Entering MyApp::OnInit");
	if (!wxApp::OnInit()) return false;

	GeoDaConst::init();
	
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
	wxXmlResource::Get()->InitAllHandlers();
	
    MyInitXmlResource();  // call the init function in MyAppResources.cpp	
	
	//int majorVsn = 0;
	//int minorVsn = 0;
	//wxGetOsVersion(&majorVsn, &minorVsn);
	//LOG_MSG(wxString::Format("OS Version: %d.%d", majorVsn, minorVsn));
	//LOG_MSG(wxString::Format("XP? %d, Vista? %d", GeneralWxUtils::isXP(),
	//        GeneralWxUtils::isVista()));
	
	int frameWidth = 770;
	int frameHeight = 80;
	if (GeneralWxUtils::isMac()) {
		frameWidth = 648;
		frameHeight = 46;
	}
	if (GeneralWxUtils::isWindows()) {
		// The default is assumed to be Vista / Win 7 family, but can check
		//   with GeneralWxUtils::isVista()
		frameWidth = 590;
		frameHeight = 76;
		// Override default in case XP family of OSes is detected
		if (GeneralWxUtils::isXP()) {
			frameWidth = 590;
			frameHeight = 76;
		}
	}
	if (GeneralWxUtils::isUnix()) {  // assumes GTK
		frameWidth = 760;
 		frameHeight = 84;
	}

	wxPoint appFramePos = wxDefaultPosition;
	if (GeneralWxUtils::isUnix() || GeneralWxUtils::isMac()) {
		appFramePos = wxPoint(80,60);
	}

	MyFrame::theFrame = new MyFrame("GeoDa",
									appFramePos,
									wxSize(frameWidth, frameHeight),
									wxDEFAULT_FRAME_STYLE &
									~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
	
	MyFrame::theFrame->Show(true);
	SetTopWindow(MyFrame::theFrame);
	
	if (GeneralWxUtils::isWindows()) {
		// For XP / Vista / Win 7, the user can select to use font sizes
		// of %100, %125 or %150.
		// Therefore, we might need to slighly increase the window size
		// when sizes > %100 are used in the Display options.
		LOG(MyFrame::theFrame->GetSize().GetHeight());
		LOG(MyFrame::theFrame->GetClientSize().GetHeight());
		if (MyFrame::theFrame->GetClientSize().GetHeight() < 22) {
			MyFrame::theFrame->SetSize(MyFrame::theFrame->GetSize().GetWidth(),
				MyFrame::theFrame->GetSize().GetHeight() +
						(22 - MyFrame::theFrame->GetClientSize().GetHeight()));
			LOG(MyFrame::theFrame->GetClientSize().GetHeight());
		}
	}

	// The following code will insert a "Test Map Frame" menu item
	// under the file menu.  Comment this section out when not
	// testing.
	// BEGIN TestMapFrame TEST
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	int fm_index = mb->FindMenu("File");
	wxMenu* fm = mb->GetMenu(fm_index);
	//fm->Append(ID_TEST_MAP_FRAME, "Test Map Frame",
	//		   "Open a test frame");
	//GeneralWxUtils::EnableMenuItem(mb, "File",
	//							   ID_TEST_MAP_FRAME, true);
	//fm->Append(ID_TEST_TABLE_FRAME, "Test Table",
	//		   "Open a test table");
	//GeneralWxUtils::EnableMenuItem(mb, "File",
	//							   ID_TEST_TABLE_FRAME, false);
	// END TestMapFrame TEST
	
	//TestScrollWinFrame *subframe =
	//new TestScrollWinFrame(NULL, "Test Scrolled Window Frame",
	//					   wxDefaultPosition, wxSize(500,600),
	//					   wxDEFAULT_FRAME_STYLE);
    //subframe->Show(true);
	
	
	//MyFrame::theFrame->OpenColumbusTest();

	LOG_MSG("Exiting MyApp::OnInit");
	return true;
}

int MyApp::OnExit(void)
{
	LOG_MSG("In MyApp::OnExit");
	return 0;
}

void MyApp::OnFatalException()
{
	LOG_MSG("In MyApp::OnFatalException");
	wxMessageBox("Fatal Excption.  Program will likely close now.");
}


/*
 * This is the top-level window of the application.
 */

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_CHAR_HOOK(MyFrame::OnKeyEvent)
	EVT_MENU(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_TOOL(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_BUTTON(XRCID("ID_OPEN_SHAPE_FILE"), MyFrame::OnOpenShapefile)
	EVT_MENU(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_TOOL(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_BUTTON(XRCID("ID_OPEN_TABLE_ONLY"), MyFrame::OnOpenTableOnly)
	EVT_MENU(XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),
			 MyFrame::OnOpenSpTmShapefile)
	EVT_MENU(XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),
			 MyFrame::OnOpenSpTmTableOnly)
	EVT_MENU(XRCID("wxID_CLOSE"), MyFrame::OnMenuClose)
	EVT_MENU(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_TOOL(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_BUTTON(XRCID("ID_CLOSE_ALL"), MyFrame::OnCloseAll)
	EVT_CLOSE(MyFrame::OnClose)
	EVT_MENU(XRCID("wxID_EXIT"), MyFrame::OnQuit)

	EVT_MENU(XRCID("ID_SELECT_WITH_RECT"), MyFrame::OnSelectWithRect)
	EVT_MENU(XRCID("ID_SELECT_WITH_CIRCLE"), MyFrame::OnSelectWithCircle)
	EVT_MENU(XRCID("ID_SELECT_WITH_LINE"), MyFrame::OnSelectWithLine)
	EVT_MENU(XRCID("ID_SELECTION_MODE"), MyFrame::OnSelectionMode)
	EVT_MENU(XRCID("ID_FIT_TO_WINDOW_MODE"), MyFrame::OnFitToWindowMode)
	// Fit-To-Window Mode
	EVT_MENU(XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
						MyFrame::OnFixedAspectRatioMode)
	EVT_MENU(XRCID("ID_ZOOM_MODE"), MyFrame::OnZoomMode)
	EVT_MENU(XRCID("ID_ZOOM_IN"), MyFrame::OnZoomIn)
	EVT_MENU(XRCID("ID_ZOOM_OUT"), MyFrame::OnZoomOut)
	EVT_MENU(XRCID("ID_PAN_MODE"), MyFrame::OnPanMode)
	// Print Canvas State to Log File.  Used for debugging.
	EVT_MENU(XRCID("ID_PRINT_CANVAS_STATE"), MyFrame::OnPrintCanvasState)

	EVT_MENU(XRCID("ID_SAVE_CANVAS_IMAGE_AS"), MyFrame::OnSaveCanvasImageAs)
	EVT_MENU(XRCID("ID_SAVE_SELECTED_TO_COLUMN"),
						MyFrame::OnSaveSelectedToColumn)
	EVT_MENU(XRCID("ID_CANVAS_BACKGROUND_COLOR"),
			 MyFrame::OnCanvasBackgroundColor)
	EVT_MENU(XRCID("ID_LEGEND_BACKGROUND_COLOR"),
		 MyFrame::OnLegendBackgroundColor)
	EVT_MENU(XRCID("ID_SELECTABLE_FILL_COLOR"),
		 MyFrame::OnSelectableFillColor)
	EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_COLOR"),
			 MyFrame::OnSelectableOutlineColor)
	EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
		 MyFrame::OnSelectableOutlineVisible)
	EVT_MENU(XRCID("ID_HIGHLIGHT_COLOR"), MyFrame::OnHighlightColor)

	EVT_MENU(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
	EVT_TOOL(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
	EVT_BUTTON(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
			 MyFrame::OnSetDefaultVariableSettings)
	EVT_MENU(XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
			 MyFrame::OnCopyImageToClipboard)
	EVT_MENU(XRCID("ID_COPY_LEGEND_TO_CLIPBOARD"),
			 MyFrame::OnCopyLegendToClipboard)

	EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_OPEN"), MyFrame::OnToolsWeightsOpen)
	EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)
	EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)
	EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_CREATE"), MyFrame::OnToolsWeightsCreate)
	
	EVT_MENU(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
			 MyFrame::OnConnectivityHistView)
	EVT_TOOL(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
			 MyFrame::OnConnectivityHistView)
	EVT_BUTTON(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
			   MyFrame::OnConnectivityHistView)

	EVT_MENU(XRCID("ID_SHOW_AXES"), MyFrame::OnShowAxes)

	EVT_TOOL(XRCID("ID_MAP_CHOICES"), MyFrame::OnMapChoices)
	EVT_TOOL(XRCID("ID_OPEN_CHOICES"), MyFrame::OnOpenChoices)

	EVT_MENU(XRCID("ID_SHAPE_POINTS_FROM_ASCII"),
			 MyFrame::OnShapePointsFromASCII)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_GRID"),
			 MyFrame::OnShapePolygonsFromGrid)
	EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_BOUNDARY"),
			 MyFrame::OnShapePolygonsFromBoundary)
	EVT_MENU(XRCID("ID_SHAPE_TO_BOUNDARY"), MyFrame::OnShapeToBoundary)
	EVT_MENU(XRCID("ID_POINTS_FROM_TABLE"), MyFrame::OnGeneratePointShpFile)
 
	// Table menu items
	EVT_MENU(XRCID("ID_SHOW_TIME_CHOOSER"), MyFrame::OnShowTimeChooser)

	EVT_MENU(XRCID("ID_SHOW_DATA_MOVIE"), MyFrame::OnShowDataMovie)
	EVT_TOOL(XRCID("ID_SHOW_DATA_MOVIE"), MyFrame::OnShowDataMovie)
	EVT_BUTTON(XRCID("ID_SHOW_DATA_MOVIE"), MyFrame::OnShowDataMovie)

	EVT_MENU(XRCID("ID_SHOW_CAT_CLASSIF"), MyFrame::OnShowCatClassif)
	EVT_TOOL(XRCID("ID_SHOW_CAT_CLASSIF"), MyFrame::OnShowCatClassif)
	EVT_BUTTON(XRCID("ID_SHOW_CAT_CLASSIF"), MyFrame::OnShowCatClassif)

	EVT_MENU(XRCID("ID_SPACE_TIME_TOOL"), MyFrame::OnSpaceTimeTool)
	EVT_MENU(XRCID("ID_NEW_TABLE_MOVE_SELECTED_TO_TOP"),
			 MyFrame::OnMoveSelectedToTop)
	EVT_MENU(XRCID("ID_NEW_TABLE_CLEAR_SELECTION"),
			 MyFrame::OnClearSelection)
	EVT_MENU(XRCID("ID_NEW_TABLE_RANGE_SELECTION"),
			 MyFrame::OnRangeSelection)
	EVT_MENU(XRCID("ID_NEW_TABLE_FIELD_CALCULATION"),
			 MyFrame::OnFieldCalculation)
	EVT_MENU(XRCID("ID_NEW_TABLE_ADD_COLUMN"), MyFrame::OnAddCol)
	EVT_MENU(XRCID("ID_NEW_TABLE_DELETE_COLUMN"), MyFrame::OnDeleteCol)
	EVT_MENU(XRCID("ID_NEW_TABLE_EDIT_FIELD_PROP"),
			 MyFrame::OnEditFieldProperties)
	EVT_MENU(XRCID("ID_NEW_TABLE_MERGE_TABLE_DATA"),
			 MyFrame::OnMergeTableData)
	EVT_MENU(XRCID("ID_SAVE_PROJECT"),
			 MyFrame::OnSaveProject)
	EVT_MENU(XRCID("ID_SAVE_AS_PROJECT"),
			 MyFrame::OnSaveAsProject)
	EVT_MENU(XRCID("ID_EXPORT_TO_CSV_FILE"),
			 MyFrame::OnExportToCsvFile)
	EVT_MENU(XRCID("ID_ADD_NEIGHBORS_TO_SELECTION"),
			 MyFrame::OnAddNeighborsToSelection)

	EVT_MENU(XRCID("ID_REGRESSION_CLASSIC"), MyFrame::OnRegressionClassic)
	EVT_TOOL(XRCID("ID_REGRESSION_CLASSIC"), MyFrame::OnRegressionClassic)

	EVT_TOOL(XRCID("ID_COND_PLOT_CHOICES"), MyFrame::OnCondPlotChoices)
	// The following duplicate entries are needed as a workaround to
	// make menu enable/disable work for the menu bar when the same menu
	// item appears twice.
	EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW_MAP_MENU"),
			 MyFrame::OnShowConditionalMapView)	
	EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
			 MyFrame::OnShowConditionalMapView)
	EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
			   MyFrame::OnShowConditionalMapView)
	EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
			 MyFrame::OnShowConditionalScatterView)
	EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
			   MyFrame::OnShowConditionalScatterView)
	EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
			 MyFrame::OnShowConditionalHistView)
	EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
			   MyFrame::OnShowConditionalHistView)

	EVT_MENU(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
			 MyFrame::OnShowCartogramNewView)
	EVT_TOOL(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
			 MyFrame::OnShowCartogramNewView)
	EVT_BUTTON(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
			   MyFrame::OnShowCartogramNewView)

	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_1"), MyFrame::OnCartogramImprove1)
	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_2"), MyFrame::OnCartogramImprove2)
	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_3"), MyFrame::OnCartogramImprove3)
	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_4"), MyFrame::OnCartogramImprove4)
	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_5"), MyFrame::OnCartogramImprove5)
	EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_6"), MyFrame::OnCartogramImprove6)

	EVT_MENU(XRCID("ID_OPTIONS_HINGE_15"), MyFrame::OnHinge15)
	EVT_MENU(XRCID("ID_OPTIONS_HINGE_30"), MyFrame::OnHinge30)

	EVT_MENU(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_TOOL(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_BUTTON(XRCID("IDM_HIST"), MyFrame::OnExploreHist)
	EVT_MENU(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_MENU(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_MENU(XRCID("IDM_SCATTER_NEW_PLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_TOOL(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_TOOL(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_BUTTON(XRCID("IDM_SCATTERPLOT"), MyFrame::OnExploreScatterNewPlot)
	EVT_BUTTON(XRCID("IDM_BUBBLECHART"), MyFrame::OnExploreBubbleChart)
	EVT_MENU(ID_TEST_MAP_FRAME, MyFrame::OnExploreTestMap)
	EVT_MENU(ID_TEST_TABLE_FRAME, MyFrame::OnExploreTestTable)
	EVT_MENU(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
	EVT_TOOL(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
	EVT_BUTTON(XRCID("IDM_BOX"), MyFrame::OnExploreNewBox)
	EVT_MENU(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_TOOL(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_BUTTON(XRCID("IDM_PCP"), MyFrame::OnExplorePCP)
	EVT_MENU(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	EVT_TOOL(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	EVT_BUTTON(XRCID("IDM_3DP"), MyFrame::OnExplore3DP)
	
	EVT_TOOL(XRCID("IDM_NEW_TABLE"), MyFrame::OnToolOpenNewTable)
	EVT_BUTTON(XRCID("IDM_NEW_TABLE"), MyFrame::OnToolOpenNewTable)

	EVT_TOOL(XRCID("ID_MORAN_MENU"), MyFrame::OnMoranMenuChoices)
	EVT_MENU(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_TOOL(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_BUTTON(XRCID("IDM_MSPL"), MyFrame::OnOpenMSPL)
	EVT_MENU(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_TOOL(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_BUTTON(XRCID("IDM_GMORAN"), MyFrame::OnOpenGMoran)
	EVT_MENU(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_TOOL(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_BUTTON(XRCID("IDM_MORAN_EBRATE"), MyFrame::OnOpenMoranEB)
	EVT_TOOL(XRCID("ID_LISA_MENU"), MyFrame::OnLisaMenuChoices)
	EVT_MENU(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_TOOL(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_BUTTON(XRCID("IDM_UNI_LISA"), MyFrame::OnOpenUniLisa)
	EVT_MENU(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_TOOL(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_BUTTON(XRCID("IDM_MULTI_LISA"), MyFrame::OnOpenMultiLisa)
	EVT_MENU(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_TOOL(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_BUTTON(XRCID("IDM_LISA_EBRATE"), MyFrame::OnOpenLisaEB)
	EVT_TOOL(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)
	EVT_BUTTON(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)
	EVT_MENU(XRCID("IDM_GETIS_ORD"), MyFrame::OnOpenGetisOrd)

	EVT_MENU(XRCID("ID_HISTOGRAM_INTERVALS"), MyFrame::OnHistogramIntervals)
	EVT_MENU(XRCID("ID_SAVE_CONNECTIVITY_TO_TABLE"),
			 MyFrame::OnSaveConnectivityToTable)
	EVT_MENU(XRCID("ID_SELECT_ISOLATES"), MyFrame::OnSelectIsolates)

	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_99PERMUTATION"),
			 MyFrame::OnRan99Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_199PERMUTATION"),
			 MyFrame::OnRan199Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_499PERMUTATION"),
			 MyFrame::OnRan499Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_999PERMUTATION"),
			 MyFrame::OnRan999Per)
	EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_OTHER"),
			 MyFrame::OnRanOtherPer)

	EVT_MENU(XRCID("ID_SAVE_MORANI"), MyFrame::OnSaveMoranI)

	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_05"), MyFrame::OnSigFilter05)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_01"), MyFrame::OnSigFilter01)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_001"), MyFrame::OnSigFilter001)
	EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_0001"), MyFrame::OnSigFilter0001)

	EVT_MENU(XRCID("ID_SAVE_GETIS_ORD"), MyFrame::OnSaveGetisOrd)
	EVT_MENU(XRCID("ID_SAVE_LISA"), MyFrame::OnSaveLisa)
	EVT_MENU(XRCID("ID_SELECT_CORES"), MyFrame::OnSelectCores)
	EVT_MENU(XRCID("ID_SELECT_NEIGHBORS_OF_CORES"),
			 MyFrame::OnSelectNeighborsOfCores)
	EVT_MENU(XRCID("ID_SELECT_CORES_AND_NEIGHBORS"),
			 MyFrame::OnSelectCoresAndNeighbors)

	EVT_MENU(XRCID("ID_MAP_ADDMEANCENTERS"), MyFrame::OnAddMeanCenters)
	EVT_MENU(XRCID("ID_MAP_ADDCENTROIDS"), MyFrame::OnAddCentroids)
	EVT_MENU(XRCID("ID_DISPLAY_MEAN_CENTERS"), MyFrame::OnDisplayMeanCenters)
	EVT_MENU(XRCID("ID_DISPLAY_CENTROIDS"), MyFrame::OnDisplayCentroids)
	EVT_MENU(XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
			 MyFrame::OnDisplayVoronoiDiagram)
	EVT_MENU(XRCID("ID_SAVE_VORONOI_TO_SHAPEFILE"),
			 MyFrame::OnSaveVoronoiToShapefile)
	EVT_MENU(XRCID("ID_SAVE_MEAN_CNTRS_TO_SHAPEFILE"),
			 MyFrame::OnSaveMeanCntrsToShapefile)
	EVT_MENU(XRCID("ID_SAVE_CENTROIDS_TO_SHAPEFILE"),
			 MyFrame::OnSaveCentroidsToShapefile)
	EVT_MENU(XRCID("ID_SAVE_VORONOI_DUPS_TO_TABLE"),
			 MyFrame::OnSaveVoronoiDupsToTable)

	EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"),
			 MyFrame::OnNewCustomCatClassifA)
	EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_B"),
			 MyFrame::OnNewCustomCatClassifB)
	EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_C"),
			 MyFrame::OnNewCustomCatClassifC)

EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0, MyFrame::OnCCClassifA0)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A1, MyFrame::OnCCClassifA1)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A2, MyFrame::OnCCClassifA2)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A3, MyFrame::OnCCClassifA3)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A4, MyFrame::OnCCClassifA4)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A5, MyFrame::OnCCClassifA5)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A6, MyFrame::OnCCClassifA6)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A7, MyFrame::OnCCClassifA7)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A8, MyFrame::OnCCClassifA8)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A9, MyFrame::OnCCClassifA9)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A10, MyFrame::OnCCClassifA10)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A11, MyFrame::OnCCClassifA11)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A12, MyFrame::OnCCClassifA12)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A13, MyFrame::OnCCClassifA13)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A14, MyFrame::OnCCClassifA14)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A15, MyFrame::OnCCClassifA15)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A16, MyFrame::OnCCClassifA16)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A17, MyFrame::OnCCClassifA17)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A18, MyFrame::OnCCClassifA18)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A19, MyFrame::OnCCClassifA19)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A20, MyFrame::OnCCClassifA20)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A21, MyFrame::OnCCClassifA21)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A22, MyFrame::OnCCClassifA22)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A23, MyFrame::OnCCClassifA23)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A24, MyFrame::OnCCClassifA24)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A25, MyFrame::OnCCClassifA25)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A26, MyFrame::OnCCClassifA26)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A27, MyFrame::OnCCClassifA27)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A28, MyFrame::OnCCClassifA28)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A29, MyFrame::OnCCClassifA29)

EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0, MyFrame::OnCCClassifB0)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B1, MyFrame::OnCCClassifB1)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B2, MyFrame::OnCCClassifB2)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B3, MyFrame::OnCCClassifB3)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B4, MyFrame::OnCCClassifB4)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B5, MyFrame::OnCCClassifB5)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B6, MyFrame::OnCCClassifB6)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B7, MyFrame::OnCCClassifB7)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B8, MyFrame::OnCCClassifB8)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B9, MyFrame::OnCCClassifB9)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B10, MyFrame::OnCCClassifB10)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B11, MyFrame::OnCCClassifB11)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B12, MyFrame::OnCCClassifB12)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B13, MyFrame::OnCCClassifB13)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B14, MyFrame::OnCCClassifB14)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B15, MyFrame::OnCCClassifB15)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B16, MyFrame::OnCCClassifB16)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B17, MyFrame::OnCCClassifB17)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B18, MyFrame::OnCCClassifB18)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B19, MyFrame::OnCCClassifB19)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B20, MyFrame::OnCCClassifB20)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B21, MyFrame::OnCCClassifB21)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B22, MyFrame::OnCCClassifB22)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B23, MyFrame::OnCCClassifB23)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B24, MyFrame::OnCCClassifB24)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B25, MyFrame::OnCCClassifB25)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B26, MyFrame::OnCCClassifB26)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B27, MyFrame::OnCCClassifB27)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B28, MyFrame::OnCCClassifB28)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B29, MyFrame::OnCCClassifB29)

EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0, MyFrame::OnCCClassifC0)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C1, MyFrame::OnCCClassifC1)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C2, MyFrame::OnCCClassifC2)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C3, MyFrame::OnCCClassifC3)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C4, MyFrame::OnCCClassifC4)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C5, MyFrame::OnCCClassifC5)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C6, MyFrame::OnCCClassifC6)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C7, MyFrame::OnCCClassifC7)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C8, MyFrame::OnCCClassifC8)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C9, MyFrame::OnCCClassifC9)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C10, MyFrame::OnCCClassifC10)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C11, MyFrame::OnCCClassifC11)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C12, MyFrame::OnCCClassifC12)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C13, MyFrame::OnCCClassifC13)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C14, MyFrame::OnCCClassifC14)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C15, MyFrame::OnCCClassifC15)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C16, MyFrame::OnCCClassifC16)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C17, MyFrame::OnCCClassifC17)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C18, MyFrame::OnCCClassifC18)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C19, MyFrame::OnCCClassifC19)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C20, MyFrame::OnCCClassifC20)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C21, MyFrame::OnCCClassifC21)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C22, MyFrame::OnCCClassifC22)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C23, MyFrame::OnCCClassifC23)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C24, MyFrame::OnCCClassifC24)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C25, MyFrame::OnCCClassifC25)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C26, MyFrame::OnCCClassifC26)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C27, MyFrame::OnCCClassifC27)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C28, MyFrame::OnCCClassifC28)
EVT_MENU(GeoDaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C29, MyFrame::OnCCClassifC29)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnOpenThemelessMap)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnOpenThemelessMap)
	EVT_MENU(XRCID("ID_MAPANALYSIS_THEMELESS"),
			 MyFrame::OnThemelessMap)
	EVT_MENU(XRCID("ID_COND_VERT_THEMELESS"),
			 MyFrame::OnCondVertThemelessMap)
	EVT_MENU(XRCID("ID_COND_HORIZ_THEMELESS"),
			 MyFrame::OnCondHorizThemelessMap)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnOpenQuantile)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnOpenQuantile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_QUANTILE"),
			 MyFrame::OnQuantile)
	EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_QUANTILE"),
			 MyFrame::OnCondVertQuantile)
	EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_QUANTILE"),
			 MyFrame::OnCondHorizQuantile)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnOpenPercentile)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnOpenPercentile)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnPercentile)
	EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnCondVertPercentile)
	EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_PERCENTILE"),
			 MyFrame::OnCondHorizPercentile)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), MyFrame::OnOpenHinge15)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), MyFrame::OnOpenHinge15)
	EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_15"), MyFrame::OnHinge15)
	EVT_MENU(XRCID("ID_COND_VERT_HINGE_15"), MyFrame::OnCondVertHinge15)
	EVT_MENU(XRCID("ID_COND_HORIZ_HINGE_15"), MyFrame::OnCondHorizHinge15)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), MyFrame::OnOpenHinge30)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), MyFrame::OnOpenHinge30)
	EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_30"), MyFrame::OnHinge30)
	EVT_MENU(XRCID("ID_COND_VERT_HINGE_30"), MyFrame::OnCondVertHinge30)
	EVT_MENU(XRCID("ID_COND_HORIZ_HINGE_30"), MyFrame::OnCondHorizHinge30)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnOpenStddev)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnOpenStddev)
	EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
			 MyFrame::OnStddev)
	EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_STDDEV"),
			 MyFrame::OnCondVertStddev)
	EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_STDDEV"),
			 MyFrame::OnCondHorizStddev)
	
	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnOpenNaturalBreaks)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnOpenNaturalBreaks)
	EVT_MENU(XRCID("ID_MAPANALYSIS_NATURAL_BREAKS"),
			 MyFrame::OnNaturalBreaks)
	EVT_MENU(XRCID("ID_COND_VERT_NATURAL_BREAKS"),
			 MyFrame::OnCondVertNaturalBreaks)
	EVT_MENU(XRCID("ID_COND_HORIZ_NATURAL_BREAKS"),
			 MyFrame::OnCondHorizNaturalBreaks)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnOpenUniqueValues)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnOpenUniqueValues)
	EVT_MENU(XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
			 MyFrame::OnUniqueValues)
	EVT_MENU(XRCID("ID_COND_VERT_UNIQUE_VALUES"),
			 MyFrame::OnCondVertUniqueValues)
	EVT_MENU(XRCID("ID_COND_HORIZ_UNIQUE_VALUES"),
			 MyFrame::OnCondHorizUniqueValues)

	EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnOpenEqualIntervals)
	EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnOpenEqualIntervals)
	EVT_MENU(XRCID("ID_MAPANALYSIS_EQUAL_INTERVALS"),
			 MyFrame::OnEqualIntervals)
	EVT_MENU(XRCID("ID_COND_VERT_EQUAL_INTERVALS"),
			 MyFrame::OnCondVertEqualIntervals)
	EVT_MENU(XRCID("ID_COND_HORIZ_EQUAL_INTERVALS"),
			 MyFrame::OnCondHorizEqualIntervals)

	EVT_MENU(XRCID("ID_SAVE_CATEGORIES"), MyFrame::OnSaveCategories)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), MyFrame::OnOpenRawrate)
	EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), MyFrame::OnOpenRawrate)
	EVT_MENU(XRCID("ID_RATES_SMOOTH_RAWRATE"), MyFrame::OnRawrate)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnOpenExcessrisk)
	EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnOpenExcessrisk)
	EVT_MENU(XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
			 MyFrame::OnExcessrisk)

	EVT_TOOL(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnOpenEmpiricalBayes)
	EVT_MENU(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnOpenEmpiricalBayes)
	EVT_MENU(XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
			 MyFrame::OnEmpiricalBayes)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnOpenSpatialRate)
	EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnOpenSpatialRate)
	EVT_MENU(XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
			 MyFrame::OnSpatialRate)

	EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnOpenSpatialEmpiricalBayes)
	EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnOpenSpatialEmpiricalBayes)
	EVT_MENU(XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
			 MyFrame::OnSpatialEmpiricalBayes)

	EVT_MENU(XRCID("ID_MAPANALYSIS_SAVERESULTS"), MyFrame::OnSaveResults)

	EVT_MENU(XRCID("ID_VIEW_STANDARDIZED_DATA"),
			 MyFrame::OnViewStandardizedData)
	EVT_MENU(XRCID("ID_VIEW_ORIGINAL_DATA"), MyFrame::OnViewOriginalData)
	EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED"),
			 MyFrame::OnViewRegressionSelected)
	EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
			 MyFrame::OnViewRegressionSelectedExcluded)
	EVT_MENU(XRCID("ID_DISPLAY_STATISTICS"), MyFrame::OnDisplayStatistics)
	EVT_MENU(XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
			 MyFrame::OnShowAxesThroughOrigin)
	EVT_MENU(XRCID("ID_DISPLAY_AXES_SCALE_VALUES"),
			 MyFrame::OnDisplayAxesScaleValues)
	EVT_MENU(XRCID("ID_DISPLAY_SLOPE_VALUES"),
			 MyFrame::OnDisplaySlopeValues)

	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR1, MyFrame::OnTimeSyncVariable1)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR2, MyFrame::OnTimeSyncVariable2)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR3, MyFrame::OnTimeSyncVariable3)
	EVT_MENU(GeoDaConst::ID_TIME_SYNC_VAR4, MyFrame::OnTimeSyncVariable4)

	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
			 MyFrame::OnFixedScaleVariable1)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR2,
			 MyFrame::OnFixedScaleVariable2)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR3,
			 MyFrame::OnFixedScaleVariable3)
	EVT_MENU(GeoDaConst::ID_FIX_SCALE_OVER_TIME_VAR4,
			 MyFrame::OnFixedScaleVariable4)

	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_1, MyFrame::OnPlotsPerView1)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_2, MyFrame::OnPlotsPerView2)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_3, MyFrame::OnPlotsPerView3)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_4, MyFrame::OnPlotsPerView4)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_5, MyFrame::OnPlotsPerView5)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_6, MyFrame::OnPlotsPerView6)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_7, MyFrame::OnPlotsPerView7)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_8, MyFrame::OnPlotsPerView8)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_9, MyFrame::OnPlotsPerView9)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_10, MyFrame::OnPlotsPerView10)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_OTHER, MyFrame::OnPlotsPerViewOther)
	EVT_MENU(GeoDaConst::ID_PLOTS_PER_VIEW_ALL, MyFrame::OnPlotsPerViewAll)

	EVT_MENU(XRCID("ID_DISPLAY_STATUS_BAR"), MyFrame::OnDisplayStatusBar)

	EVT_MENU(XRCID("wxID_ABOUT"), MyFrame::OnHelpAbout)
END_EVENT_TABLE()


void MyFrame::UpdateToolbarAndMenus()
{
	// This method is called when no particular window is currently active.
	// In this case, the close menu item should be disabled.

	LOG_MSG("In MyFrame::UpdateToolbarAndMenus");
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), "File", wxID_CLOSE,
								   false);

	Project* p = GetProject();
	bool proj_open = (p != 0);
	bool shp_proj = proj_open && !p->IsTableOnlyProject();
	bool table_proj = proj_open && p->IsTableOnlyProject();
	bool time_variant = (proj_open && p->GetGridBase()->IsTimeVariant());

	wxMenuBar* mb = GetMenuBar();

	// Reset the toolbar frame title to default.
	SetTitle("GeoDa");
	
	//MMM: the following two item states are set elsewhere.  This should be
	// unified.
	EnableTool(XRCID("ID_OPEN_CHOICES"), !proj_open);
	EnableTool(XRCID("ID_CLOSE_ALL"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_SHAPE_FILE"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_TABLE_ONLY"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),
								   !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_CLOSE_ALL"),
								   proj_open);

	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), true);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), false);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), false);

	EnableTool(XRCID("ID_TOOLS_WEIGHTS_OPEN"), proj_open);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	EnableTool(XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_TOOLS_WEIGHTS_OPEN"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools",
								   XRCID("ID_POINTS_FROM_TABLE"), table_proj);
	
	EnableTool(XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"), proj_open);	
	GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_SET_DEFAULT_VARIABLE_SETTINGS"),
								   proj_open);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
								   false);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_PAN_MODE"), proj_open);	
	
	EnableTool(XRCID("IDM_BOX"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BOX"), proj_open);

	EnableTool(XRCID("IDM_HIST"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_HIST"), proj_open);

	EnableTool(XRCID("IDM_SCATTERPLOT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTERPLOT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTER_NEW_PLOT"),
								   proj_open);
	EnableTool(XRCID("IDM_BUBBLECHART"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BUBBLECHART"), proj_open);
	
	EnableTool(XRCID("IDM_PCP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_PCP"), proj_open);

	EnableTool(XRCID("ID_COND_PLOT_CHOICES"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COND_MENU"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
								   shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
								   proj_open);
	GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
								   proj_open);

	EnableTool(XRCID("IDM_3DP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_3DP"), proj_open);

	EnableTool(XRCID("IDM_NEW_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, "Table", proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_TIME_CHOOSER"),
								   time_variant);
	EnableTool(XRCID("ID_SHOW_TIME_CHOOSER"), time_variant);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_DATA_MOVIE"),
								   proj_open);
	EnableTool(XRCID("ID_SHOW_DATA_MOVIE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CAT_CLASSIF"),
								   proj_open);
	EnableTool(XRCID("ID_SHOW_CAT_CLASSIF"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SPACE_TIME_TOOL"),
								   proj_open);
	mb->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
				 (time_variant ? "Space-Time Variable Creation Tool" :
				  "Convert to Space-Time Project"));
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_PROJECT"),
			proj_open && project_p->GetGridBase()->ChangedSinceLastSave() &&
			project_p->IsAllowEnableSave());
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_AS_PROJECT"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_TO_CSV_FILE"),
								   proj_open);
	
	EnableTool(XRCID("ID_MORAN_MENU"), proj_open);
	EnableTool(XRCID("ID_LISA_MENU"), shp_proj);
	EnableTool(XRCID("IDM_MSPL"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MSPL"), proj_open);
	EnableTool(XRCID("IDM_GMORAN"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_GMORAN"), proj_open);
	EnableTool(XRCID("IDM_MORAN_EBRATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MORAN_EBRATE"), proj_open);
	EnableTool(XRCID("IDM_UNI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_UNI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_MULTI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MULTI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_LISA_EBRATE"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LISA_EBRATE"), shp_proj);
	EnableTool(XRCID("IDM_GETIS_ORD"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_GETIS_ORD"), shp_proj);
	
	GeneralWxUtils::EnableMenuAll(mb, "Map", shp_proj);
	EnableTool(XRCID("ID_MAP_CHOICES"), shp_proj);
	EnableTool(XRCID("ID_DATA_MOVIE"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), shp_proj);
	
	EnableTool(XRCID("ID_REGRESSION_CLASSIC"), true);
	
	//Empty out the Options menu:
	wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
}

void MyFrame::SetMenusToDefault()
{
	LOG_MSG("Entering MyFrame::SetMenusToDefault");
	// This method disables all menu items that are not
	// in one of File, Tools, Methods, or Help menus.
	wxMenuBar* mb = GetMenuBar();
	if (!mb) return;
	wxMenu* menu = NULL;
	wxString menuText = wxEmptyString;
	int menuCnt = mb->GetMenuCount();
	for (int i=0; i<menuCnt; i++) {
		menu = mb->GetMenu(i);
		menuText = mb->GetMenuLabelText(i);
		if ( (menuText != "File") &&
			 (menuText != "Tools") &&
			 (menuText != "Methods") &&
			 (menuText != "Help") ) {
			GeneralWxUtils::EnableMenuAll(mb, menuText, false);
		}
	}

	LOG_MSG("Entering MyFrame::SetMenusToDefault");
}


Project* MyFrame::project_p = 0;
std::list<wxToolBar*> MyFrame::toolbar_list(0);

MyFrame::MyFrame(const wxString& title,
				 const wxPoint& pos, const wxSize& size, long style):
wxFrame(NULL, -1, title, pos, size, style)
{
	LOG_MSG("Entering MyFrame::MyFrame");
	
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
	SetMenuBar(wxXmlResource::Get()->LoadMenuBar("ID_SHARED_MAIN_MENU"));

	wxToolBar* tb1;
	if (GeneralWxUtils::isMac()) {
		// for some reason the toolbar icons become disabled when
		// MyFrame loses focus on Mac.  So, for now we are not
		// making the toolbar a child of the Panel.
		tb1 = wxXmlResource::Get()->LoadToolBar(this, "ToolBar");
		SetToolBar(tb1);
	} else {
		wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
									 size, wxNO_BORDER);
		wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
		
		tb1 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar");
		//wxToolBar* tb2 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar2");
		//wxToolBar* tb3 = wxXmlResource::Get()->LoadToolBar(panel, "ToolBar3");
		wxASSERT(tb1);
		////wxASSERT(tb2);
		if (GeneralWxUtils::isUnix()) {
			// unfortunately, just GTK needs the toolbar to be added to the
			// topSizer rather than the panel itself.
			topSizer->Add(tb1, 0, 0, 0, 0);
			////topSizer->Add(tb2, 0, 0, 0, 0);
			////topSizer->Add(tb3, 0, 0, 0, 0);
		} else {
		    topSizer->Add(panel, 0, 0, 0, 0);
		}
		SetSizer(topSizer);
		topSizer->Fit(panel);
	}

	toolbar_list.push_front(tb1);
	////toolbar_list.push_front(tb2);
	////toolbar_list.push_front(tb3);
	
	//wxXmlResource::Get()->LoadPanel(appFrame, "ID_CONTROL_PANEL");
	
	SetMenusToDefault();
 	UpdateToolbarAndMenus();
		
	LOG_MSG("Exiting MyFrame::MyFrame");
}

MyFrame::~MyFrame()
{
	LOG_MSG("Entering MyFrame::~MyFrame()");
	MyFrame::theFrame = 0;
	LOG_MSG("Exiting MyFrame::~MyFrame()");
}

void MyFrame::EnableTool(int xrc_id, bool enable)
{
	BOOST_FOREACH( wxToolBar* tb, toolbar_list ) {
		if (tb)	tb->EnableTool(xrc_id, enable);
	}
}

void MyFrame::EnableTool(const wxString& id_str, bool enable)
{
	BOOST_FOREACH( wxToolBar* tb, toolbar_list ) {
		if (tb)	tb->EnableTool(wxXmlResource::GetXRCID(id_str), enable);
	}
}

bool MyFrame::IsProjectOpen()
{
	return projectOpen;
}

void MyFrame::SetProjectOpen(bool open)
{
	projectOpen = open;
}

#include "DialogTools/SelectWeightDlg.h"

GalWeight* MyFrame::GetGal()
{
	if (!project_p->GetWManager()->IsDefaultWeight()) {
		SelectWeightDlg dlg(project_p, this);
		if (dlg.ShowModal()!= wxID_OK) return 0;
	}
	GeoDaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeoDaWeight::gal_type) {
		wxMessageBox("Error: Only GAL type weights are currently supported. "
					 "Other weight types are internally converted to GAL.");
		return 0;
	}
	return (GalWeight*) w;
}

void MyFrame::OnOpenNewTable()
{
	LOG_MSG("Entering MyFrame::OnOpenNewTable");
	NewTableViewerFrame* tvf = 0;
	wxGrid* g = project_p->GetGridBase()->GetView();
	if (g) tvf = (NewTableViewerFrame*) g->GetParent();
	if (!tvf) {
		wxString msg("The Table should always be open, although somtimes it "
					 "is hidden while the project is open.  This condition "
					 "has been violated.  Please report this to "
					 "the program developers.");
		wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING);
		dlg.ShowModal();
		tvf = new NewTableViewerFrame(theFrame, project_p,
									  GeoDaConst::table_frame_title,
									  wxDefaultPosition,
									  GeoDaConst::table_default_size,
									  wxDEFAULT_FRAME_STYLE);
	}
	tvf->Show(true);
	tvf->Maximize(false);
	tvf->Raise();
	
	LOG_MSG("Exiting MyFrame::OnOpenNewTable");
}

/** returns false if user wants to abort the operation */
bool MyFrame::OnCloseMap(bool ignore_unsaved_changes)
{
	LOG_MSG("Entering MyFrame::OnCloseMap");
	
	wxString msg;
	if (IsProjectOpen() && !ignore_unsaved_changes &&
		project_p->GetGridBase()->ChangedSinceLastSave()) {
		msg = "Ok to close current project?  There have been changes to the "
			"Table since it was last saved. To save your work, "
			"go to File > Save";
		wxMessageDialog msgDlg(this, msg,
							   "Close with unsaved Table changes?",
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
		if (msgDlg.ShowModal() != wxID_YES) return false;
	}
	
	SetProjectOpen(false);
	if (project_p) {
		project_p->GetTableState()->closeAndDeleteWhenEmpty();
		project_p->GetFramesManager()->closeAndDeleteWhenEmpty();
		std::list<FramesManagerObserver*> observers(
						project_p->GetFramesManager()->getCopyObservers());
		std::list<FramesManagerObserver*>::iterator it;
		for (it=observers.begin(); it != observers.end(); it++) {
			if (wxTopLevelWindow* w = dynamic_cast<wxTopLevelWindow*>(*it)) {
				wxString msg = "Calling Close(true) for window: "+w->GetTitle();
				LOG_MSG(msg);
				w->Close(true);
			}
		}
	}
	
	if (project_p && project_p->regression_dlg) {
		((wxDialog*) project_p->regression_dlg)->Close(true);
		project_p->regression_dlg = 0;
	}
	if (project_p) delete project_p; project_p = 0;
	
	UpdateToolbarAndMenus();
	EnableTool(XRCID("ID_OPEN_CHOICES"), true);
	EnableTool(XRCID("ID_CLOSE_ALL"), false);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"), true);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),true);
	
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), false);
	
	return true;
}

void MyFrame::OnOpenShapefile(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(false, false);
}

/** open columbus shapefile for testing */
void MyFrame::OpenColumbusTest()
{
	LOG_MSG("Entering MyFrame::OpenColumbusTest");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseMap()");
		if (!OnCloseMap()) return;
	}
	
	DbfGridTableBase* grid_base = 0;
	bool is_csv_file = false;
	{
		wxFileName ifn;
		{
			ifn = wxFileName("/Users/mmccann/trac_trunk/SampleData/"
							 "columbus.shp");
			// ifn.GetFullPath returns the filename with path and extension.
			
			if (IsLineShapeFile(ifn.GetFullPath())) {
				wxMessageBox("Error: GeoDa does not support Shapefiles "
							 "with line data at this time.  Please choose a "
							 "Shapefile with either point or polygon data.");
				return;
			}
			
			bool shx_found;
			bool dbf_found;
			if (!GenUtils::ExistsShpShxDbf(ifn, 0, &shx_found, &dbf_found)) {
				wxString msg;
				msg << "Error: " <<  ifn.GetName() << ".shp, ";
				msg << ifn.GetName() << ".shx, and " << ifn.GetName();
				msg << ".dbf were not found together in the same file ";
				msg << "directory. Could not find ";
				if (!shx_found && dbf_found) {
					msg << ifn.GetName() << ".shx.";
				} else if (shx_found && !dbf_found) {
					msg << ifn.GetName() << ".dbf.";
				} else {
					msg << ifn.GetName() << ".shx and " << ifn.GetName();
					msg << ".dbf.";
				}
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}		
		
		{
			DbfFileReader dbf_reader(ifn.GetPathWithSep()+ifn.GetName()+".dbf");
			if (!dbf_reader.isDbfReadSuccess()) {
				wxString msg("There was a problem reading in the DBF file.");
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			
			project_p = new Project(dbf_reader.getNumRecords());
			grid_base = new DbfGridTableBase(dbf_reader,
											 project_p->GetHighlightState(),
											 project_p->GetTableState());
			
			project_p->Init(grid_base, ifn);
		}
	}
	
	if (!project_p->IsValid()) {
		wxString msg("Could not open new project: ");
		msg << project_p->GetErrorMessage();
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		delete grid_base;
		delete project_p; project_p = 0;
		return;
	}
	
	// By this point, we know that project has created as
	// TopFrameManager object with delete_if_empty = false
	
	// This call is very improtant because we need the wxGrid to
	// take ownership of grid_base
	NewTableViewerFrame* tvf = 0;
	tvf = new NewTableViewerFrame(this, project_p,
								  GeoDaConst::table_frame_title,
								  wxDefaultPosition,
								  GeoDaConst::table_default_size,
								  wxDEFAULT_FRAME_STYLE);
	
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
	
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
	
	EnableTool(XRCID("ID_OPEN_CHOICES"), false);
	EnableTool(XRCID("ID_CLOSE_ALL"), true);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"),
								   false);	
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), true);
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_SHOW_TIME_CHOOSER"), false);
	mb->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
				 "Convert to Space-Time Project");
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_NEW_TABLE_MERGE_TABLE_DATA"),
								   true);
	
	LOG_MSG("Exiting MyFrame::OpenColumbusTest");
	
}

void MyFrame::OnOpenTableOnly(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(true, false);
}

void MyFrame::OnOpenSpTmShapefile(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(false, true);
}

void MyFrame::OnOpenSpTmTableOnly(wxCommandEvent& WXUNUSED(event) )
{
	OpenProject(true, true);
}

void MyFrame::OpenProject(bool table_only, bool space_time)
{
	LOG_MSG("Entering MyFrame::OpenProject");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseMap()");
		if (!OnCloseMap()) return;
	}
	
	DbfGridTableBase* grid_base = 0;
	bool is_csv_file = false;
	if (!space_time) {
		wxFileName ifn;
		if (!table_only) {
			wxFileDialog dlg(this, "Choose a Shapefile to open", "", "",
							 "Shapefiles (*.shp)|*.shp");
	
			if (dlg.ShowModal() != wxID_OK) return;
		
			// dlg.GetPath returns the selected filename with complete path.
			ifn = wxFileName(dlg.GetPath());
			// ifn.GetFullPath returns the filename with path and extension.
		
			if (IsLineShapeFile(ifn.GetFullPath())) {
				wxMessageBox("Error: GeoDa does not support Shapefiles "
							 "with line data at this time.  Please choose a "
							 "Shapefile with either point or polygon data.");
				return;
			}
		
			bool shx_found;
			bool dbf_found;
			if (!GenUtils::ExistsShpShxDbf(ifn, 0, &shx_found, &dbf_found)) {
				wxString msg;
				msg << "Error: " <<  ifn.GetName() << ".shp, ";
				msg << ifn.GetName() << ".shx, and " << ifn.GetName();
				msg << ".dbf were not found together in the same file ";
				msg << "directory. Could not find ";
				if (!shx_found && dbf_found) {
					msg << ifn.GetName() << ".shx.";
				} else if (shx_found && !dbf_found) {
					msg << ifn.GetName() << ".dbf.";
				} else {
					msg << ifn.GetName() << ".shx and " << ifn.GetName();
					msg << ".dbf.";
				}
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		} else {
			//wxFileDialog dlg(this, "Choose a Table DBF file to open", "", "",
			//				 "DBF files (*.dbf)|*.dbf");
			wxFileDialog dlg(this, "Choose a Table file to open", "", "",
							 "DBF or CSV files (*.dbf;*.csv)|*.dbf;*.csv");			
			if (dlg.ShowModal() != wxID_OK) return;
			
			// dlg.GetPath returns the selected filename with complete path.
			ifn = wxFileName(dlg.GetPath());
			is_csv_file = (ifn.GetExt().CmpNoCase("dbf") != 0);
		}		
		
		if (!is_csv_file) {
			DbfFileReader dbf_reader(ifn.GetPathWithSep()+ifn.GetName()+".dbf");
			if (!dbf_reader.isDbfReadSuccess()) {
				wxString msg("There was a problem reading in the DBF file.");
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		
			project_p = new Project(dbf_reader.getNumRecords());
			grid_base = new DbfGridTableBase(dbf_reader,
											 project_p->GetHighlightState(),
											 project_p->GetTableState());
			if (!table_only) {
				project_p->Init(grid_base, ifn);
			} else {
				project_p->Init(grid_base);
			}
		} else {
			std::string csv_fname(ifn.GetFullPath().ToStdString());
			std_str_array_type string_table;
			wxString err_msg;
			bool r;
			int num_rows;
			int num_cols;
			std::vector<std::string> first_row;
			r = GeoDa::GetCsvStats(csv_fname, num_rows, num_cols, first_row,
								   err_msg);
			if (!r) {
				wxString msg("There was a problem reading in the CSV: ");
				msg << err_msg;
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			
			// need to ask user if first_row contains valid field names
			
			ImportCsvDlg dlg(this, first_row);
			if (dlg.ShowModal() != wxID_OK) return;
			
			bool first_row_field_names = dlg.contains_var_names;
			if (first_row_field_names) num_rows--;
			if (!first_row_field_names) first_row.clear();
			
			r = GeoDa::FillStringTableFromCsv(csv_fname, string_table,
											  first_row_field_names, err_msg);
			if (!r) {
				wxString msg("There was a problem reading in the CSV: ");
				msg << err_msg;
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
			
			project_p = new Project(num_rows);
			grid_base = new DbfGridTableBase(string_table,
											 first_row,
											 csv_fname,
											 project_p->GetHighlightState(),
											 project_p->GetTableState());
			project_p->Init(grid_base);
		}
	} else {
		OpenSpaceTimeDlg dlg(table_only);
		if (dlg.ShowModal() != wxID_OK) return;
		
		DbfFileReader dbf_sp(dlg.time_invariant_dbf_name.GetFullPath());
		DbfFileReader dbf_tm(dlg.time_variant_dbf_name.GetFullPath());
		
		project_p = new Project(dbf_sp.getNumRecords());
		grid_base = new DbfGridTableBase(dbf_sp, dbf_tm,
										 dlg.sp_table_space_col,
										 dlg.tm_table_space_col,
										 dlg.tm_table_time_col,
										 project_p->GetHighlightState(),
										 project_p->GetTableState());
		if (!table_only) {
			wxFileName shp_fname = dlg.time_invariant_dbf_name;
			shp_fname.SetExt("shp");
			project_p->Init(grid_base, shp_fname);
		} else {
			project_p->Init(grid_base);
		}
	}
		
	if (!project_p->IsValid()) {
		wxString msg("Could not open new project: ");
		msg << project_p->GetErrorMessage();
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		delete grid_base;
		delete project_p; project_p = 0;
		return;
	}
	
	// By this point, we know that project has created as
	// TopFrameManager object with delete_if_empty = false
	
	// This call is very improtant because we need the wxGrid to
	// take ownership of grid_base
	NewTableViewerFrame* tvf = 0;
	tvf = new NewTableViewerFrame(this, project_p,
								  GeoDaConst::table_frame_title,
								  wxDefaultPosition,
								  GeoDaConst::table_default_size,
								  wxDEFAULT_FRAME_STYLE);
	if (table_only) tvf->Show(true);
		
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
	
	if (!table_only) {
		MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
										  CatClassification::no_theme,
										  MapNewCanvas::no_smoothing,
										  wxDefaultPosition,
										  GeoDaConst::map_default_size);
		nf->UpdateTitle();
	}
	
	EnableTool(XRCID("ID_OPEN_CHOICES"), false);
	EnableTool(XRCID("ID_CLOSE_ALL"), true);
	wxMenuBar* mb = GetMenuBar();
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SHAPE_FILE"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_TABLE_ONLY"), false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_OPEN_SPACE_TIME_SHAPEFILE"),false);
	GeneralWxUtils::EnableMenuItem(mb, "File",
								XRCID("ID_OPEN_SPACE_TIME_TABLE_ONLY"), false);	
	GeneralWxUtils::EnableMenuItem(mb, "File",
								   XRCID("ID_CLOSE_ALL"), true);
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_SHOW_TIME_CHOOSER"), space_time);
	mb->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
				 (space_time ? "Space-Time Variable Creation Tool" :
				  "Convert to Space-Time Project"));
	GeneralWxUtils::EnableMenuItem(mb, "Table",
								   XRCID("ID_NEW_TABLE_MERGE_TABLE_DATA"),
								   !space_time);
	
	LOG_MSG("Exiting MyFrame::OpenProject");
}

void MyFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering MyFrame::OnClose");
	
	wxString msg;
	wxString title;
	if (IsProjectOpen() && project_p->GetGridBase()->ChangedSinceLastSave()) {
		msg = "Ok to Exit GeoDa?  The current Table has unsaved changes."
		" To save your work, go to File > Save";
		title = "Exit with unsaved Table changes?";
	} else {
		msg = "Ok to Exit?";
		title = "Exit?";
	}
	
	if (IsProjectOpen()) {
		wxMessageDialog msgDlg(this, msg, title,
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	
		// Show the message dialog, and if it returns wxID_YES...
		if (msgDlg.ShowModal() != wxID_YES) return;
	}
	OnCloseMap(true);
	Destroy();

	LOG_MSG("Exiting MyFrame::OnClose");
}

void MyFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnMenuClose");
	Close(); // This will result in a call to OnClose
    LOG_MSG("Exiting MyFrame::OnMenuClose");
}

void MyFrame::OnCloseAll(wxCommandEvent& event)
{
	LOG_MSG("In MyFrame::OnCloseAll");
	OnCloseMap();
}

void MyFrame::OnSelectWithRect(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithRect(event);
}

void MyFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithCircle(event);
}

void MyFrame::OnSelectWithLine(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithLine(event);
}

void MyFrame::OnSelectionMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectionMode(event);
}

void MyFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFitToWindowMode(event);
}

void MyFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnFixedAspectRatioMode(event);
}

void MyFrame::OnZoomMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnZoomMode(event);
}

void MyFrame::OnZoomIn(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	// MMM: Not implemented in new style framework
}

void MyFrame::OnZoomOut(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	// MMM: Not implemented in new style framework
}


void MyFrame::OnPanMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPanMode(event);
}

void MyFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called MyFrame::OnPrintCanvasState");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnPrintCanvasState(event);
	
	// Add this menu item to the XRC file to see this debugging option:
	//<object class="wxMenuItem" name="ID_PRINT_CANVAS_STATE">
	//  <label>Print Canvas State to Log File</label>
    //</object>
}

void MyFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnSaveCanvasImageAs(event);
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnQuit");
	// Generate a wxCloseEvent for MyFrame.  MyFrame::OnClose will
	// be called and will give the user a chance to not exit program.
	Close();
	LOG_MSG("Exiting MyFrame::OnQuit");
}

void MyFrame::OnSaveSelectedToColumn(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnSaveSelectedToColumn");
	SaveSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
	LOG_MSG("Exiting MyFrame::OnSaveSelectedToColumn");
}

void MyFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCanvasBackgroundColor(event);
}

void MyFrame::OnLegendBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
}

void MyFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableFillColor(event);	
}

void MyFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineColor(event);
}

void MyFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineVisible(event);
}

void MyFrame::OnHighlightColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnHighlightColor(event);
}

void MyFrame::OnSetDefaultVariableSettings(wxCommandEvent& WXUNUSED(event) )
{	
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::quadvariate, true);
	VS.ShowModal();
}

void MyFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnCopyImageToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCopyImageToClipboard(event);
	LOG_MSG("Exiting MyFrame::OnCopyImageToClipboard");
}


void MyFrame::OnCopyLegendToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnCopyLegendToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	LOG_MSG("Exiting MyFrame::OnCopyLegendToClipboard");
}

void MyFrame::OnKeyEvent(wxKeyEvent& event)
{
	Project* project = GetProject();
	if (event.GetModifiers() == wxMOD_CMD &&
		project && project->GetGridBase() &&
		project->GetGridBase()->IsTimeVariant() &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		DbfGridTableBase* grid_base = project->GetGridBase();
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		LOG(del);
		grid_base->curr_time_step = grid_base->curr_time_step + del;
		if (grid_base->curr_time_step < 0) {
			grid_base->curr_time_step = grid_base->time_steps-1;
		} else if (grid_base->curr_time_step >= grid_base->time_steps) {
			grid_base->curr_time_step = 0;
		}
		if (project->GetFramesManager()) {
			project->GetFramesManager()->notifyObservers();
		}
		return;
	}
	event.Skip();
}

void MyFrame::OnToolsWeightsOpen(wxCommandEvent& WXUNUSED(event) )
{
	SelectWeightDlg dlg(project_p, this);
	if (dlg.ShowModal()!= wxID_OK) return;
	GeoDaWeight* w = project_p->GetWManager()->GetCurrWeight();
	if (!w->weight_type == GeoDaWeight::gal_type) {
		wxMessageBox("Error: Only GAL format supported internally.");
	}
}

#include "DialogTools/CreatingWeightDlg.h"
void MyFrame::OnToolsWeightsCreate(wxCommandEvent& WXUNUSED(event) )
{
	CreatingWeightDlg dlg(NULL, project_p);
	dlg.ShowModal();
}

void MyFrame::OnConnectivityHistView(wxCommandEvent& event )
{
	LOG_MSG("Entering MyFrame::OnConnectivityHistView");
	GalWeight* gal = GetGal();
	if (!gal) return;
	ConnectivityHistFrame* f;
	f = new ConnectivityHistFrame(this, project_p, gal);
	
	LOG_MSG("Exiting MyFrame::OnConnectivityHistView");
}

void MyFrame::OnMapChoices(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnMapChoices");
	wxMenu* popupMenu = 0;
	if (GeneralWxUtils::isMac()) {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES");
	} else {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES_NO_ICONS");
	}
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnMapChoices");
}

void MyFrame::OnOpenChoices(wxCommandEvent& event)
{
	LOG_MSG("Entering MyFrame::OnOpenChoices");
	wxMenu* popupMenu = 0;
	popupMenu = wxXmlResource::Get()->LoadMenu("ID_OPEN_CHOICES_MENU");
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnOpenChoices");
}

#include "DialogTools/ASC2SHPDlg.h"
void MyFrame::OnShapePointsFromASCII(wxCommandEvent& WXUNUSED(event) )
{
	ASC2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/CreateGridDlg.h"
void MyFrame::OnShapePolygonsFromGrid(wxCommandEvent& WXUNUSED(event) )
{
	CreateGridDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/Bnd2ShpDlg.h"
void MyFrame::OnShapePolygonsFromBoundary(wxCommandEvent& WXUNUSED(event) )
{
	Bnd2ShpDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/SHP2ASCDlg.h" 
void MyFrame::OnShapeToBoundary(wxCommandEvent& WXUNUSED(event) )
{
	SHP2ASCDlg dlg(this);
	dlg.ShowModal();
}

void MyFrame::OnShowTimeChooser(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetGridBase()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		if (TimeChooserDlg* w = dynamic_cast<TimeChooserDlg*>(*it)) {
			LOG_MSG("TimeChooserDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new TimeChooserDlg");
	TimeChooserDlg* dlg = new TimeChooserDlg(0, project_p->GetFramesManager(),
											 project_p->GetGridBase());
	dlg->Show(true);
}

void MyFrame::OnShowDataMovie(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetGridBase()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		if (DataMovieDlg* w = dynamic_cast<DataMovieDlg*>(*it)) {
			LOG_MSG("DataMovieDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new DataMovieDlg");
	DataMovieDlg* dlg = new DataMovieDlg(0, project_p->GetFramesManager(),
										 project_p->GetTableState(),
										 project_p->GetGridBase(),
										 project_p->GetHighlightState());
	dlg->Show(true);
}

void MyFrame::OnShowCatClassif(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetGridBase()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		if (CatClassifFrame* w = dynamic_cast<CatClassifFrame*>(*it)) {
			LOG_MSG("Cateogry Editor already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new Cateogry Editor");
	CatClassifFrame* dlg = new CatClassifFrame(this, project_p);
}

CatClassifFrame* MyFrame::GetCatClassifFrame()
{
	Project* p = GetProject();
	if (!p || !p->GetGridBase()) return 0;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		if (CatClassifFrame* w = dynamic_cast<CatClassifFrame*>(*it)) {
			LOG_MSG("Cateogry Editor already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return w;
		}
	}
	
	LOG_MSG("Opening a new Cateogry Editor");
	CatClassifFrame* dlg = new CatClassifFrame(this, project_p);
	return dlg;
}

void MyFrame::OnSpaceTimeTool(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	if (project_p->GetGridBase()->IsTimeVariant()) {
		if (!project_p || !project_p->GetGridBase()) return;
		TimeVariantImportDlg dlg(GetProject()->GetGridBase(), this);
		if (dlg.ShowModal() != wxID_OK) return;	
	} else {
		wxString dup_name;
		if (GetProject()->GetGridBase()->IsDuplicateColNames(dup_name)) {
			wxString msg;
			msg << "Two or more fields share the name " << dup_name;
			msg << " in the Table. This is technically not permitted in the";
			msg << " DBF standard. Please use Table > Edit ";
			msg << " Variable Properties to remove all duplicate";
			msg << " field names then use File > Save to";
			msg << " save changes before proceeding.";
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		CreateSpTmProjectDlg dlg(this, GetProject());
		if (dlg.ShowModal() == wxID_OK) {
			OnSpaceTimeTool(event);
		}
		UpdateToolbarAndMenus();
	}
}

void MyFrame::OnMoveSelectedToTop(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	grid_base->MoveSelectedToTop();
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}

void MyFrame::OnClearSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	grid_base->DeselectAll();
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}

void MyFrame::OnRangeSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	RangeSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
						project_p->GetGridBase()->ChangedSinceLastSave() &&
						project_p->IsAllowEnableSave());
}

void MyFrame::OnFieldCalculation(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	FieldNewCalcSheetDlg dlg(project_p, this,
							 wxID_ANY, "Variable Calculation",
							 wxDefaultPosition,
							 wxSize(700, 500) );
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnAddCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerAddColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnDeleteCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerDeleteColDlg dlg(project_p->GetGridBase(), this);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave());
}

void MyFrame::OnEditFieldProperties(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	DataViewerEditFieldPropertiesDlg dlg(project_p->GetGridBase(),
										 wxDefaultPosition,
										 wxSize(600, 400));
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnMergeTableData(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	
	MergeTableDlg dlg(project_p->GetGridBase(), wxDefaultPosition);
	dlg.ShowModal();
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
							project_p->GetGridBase()->ChangedSinceLastSave() &&
							project_p->IsAllowEnableSave());
}

void MyFrame::OnSaveProject(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	if (project_p->GetGridBase()->IsTimeVariant()) {
		SaveTableSpaceTime();
	} else {
		SaveTableSpace();
	}
}

void MyFrame::OnSaveAsProject(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	// if Table Only, then will offer to save as DBF
	// if also Shapefile, then will ask for shapefile
	// name, but we will have to see if we can create
	// .shx, .dbf and .shp and ask for overwrite permission
	// for each if needed.
	
	DbfGridTableBase* grid_base = project_p->GetGridBase();
	bool table_only = project_p->IsTableOnlyProject();
	bool space_time = grid_base->IsTimeVariant();
	
	wxString file_dlg_title = (table_only ? "DBF Name to Save As" :
							   "Shapefile Name to Save As");
	wxString file_dlg_type = (table_only ? "DBF files (*.dbf)|*.dbf" :
							  "SHP files (*.shp)|*.shp");
	wxFileDialog dlg(this, file_dlg_title, wxEmptyString, wxEmptyString,
					 file_dlg_type, wxFD_SAVE);
	
	if (dlg.ShowModal() != wxID_OK) return;
	wxFileName new_fname = dlg.GetPath();
	wxString new_main_dir = new_fname.GetPathWithSep();
	wxString new_main_name = new_fname.GetName();
	
	wxString new_sp_dbf = new_main_dir + new_main_name + ".dbf";
	wxString new_tm_dbf = new_main_dir + new_main_name + "_time.dbf";
	wxString new_shp = new_main_dir + new_main_name + ".shp";
	wxString new_shx = new_main_dir + new_main_name + ".shx";
	
	wxString cur_sp_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	wxString cur_tm_dbf;
	if (space_time) {
		cur_tm_dbf = grid_base->GetTimeDbfFileName().GetFullPath();
	}
	wxString cur_shp = project_p->GetMainDir()+project_p->GetMainName()+".shp";
	wxString cur_shx = project_p->GetMainDir()+project_p->GetMainName()+".shx";
	
	// Prompt for overwrite permissions
	// in each case, if new file name equals current file name, don't
	// need to ask permission.
	std::vector<wxString> overwrite_list;
	if ((new_sp_dbf.CmpNoCase(cur_sp_dbf) != 0) && wxFileExists(new_sp_dbf)) {
		overwrite_list.push_back(new_sp_dbf);
	}
	if (space_time &&
		(new_tm_dbf.CmpNoCase(cur_tm_dbf) != 0) && wxFileExists(new_tm_dbf)) {
		overwrite_list.push_back(new_tm_dbf);
	}
	if (!table_only && (new_shp.CmpNoCase(cur_shp) != 0)
		&& wxFileExists(new_shp)) {
		overwrite_list.push_back(new_shp);
	}
	if (!table_only && (new_shx.CmpNoCase(cur_shx) != 0)
		&& wxFileExists(new_shx)) {
		overwrite_list.push_back(new_shx);
	}
	
	// Prompt for overwrite permission
	if (overwrite_list.size() > 0) {
		wxString msg((overwrite_list.size() > 1) ? "Files " : "File ");
		for (int i=0; i<overwrite_list.size(); i++) {
			msg << overwrite_list[i];
			if (i < overwrite_list.size()-1) msg << ", ";
		}
		msg << " already exist";
		msg << ((overwrite_list.size() > 1) ? "." : "s.");
		msg << " Ok to overwrite?";
		wxMessageDialog dlg (this, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}

	if (!project_p->IsShpFileNeedsFirstSave()) {
		// Copy shp and shx files as needed
		if (!wxFileExists(new_shp)) {
			if (!wxCopyFile(cur_shp, new_shp, true)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name << ".shp";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
		if (!wxFileExists(new_shx)) {
			if (!wxCopyFile(cur_shx, new_shx, true)) {
				wxString msg("Unable to overwrite ");
				msg << new_main_name << ".shx";
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
	} else {
		std::string err_msg;
		std::string shx_fname(new_shx.ToStdString());
		bool r = Shapefile::writePointIndexFile(shx_fname,
												project_p->index_data, err_msg);
		if (!r) {
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		std::string shp_fname(new_shp.ToStdString());
		r = Shapefile::writePointMainFile(shp_fname,
										  project_p->main_data, err_msg);
		if (!r) {
			wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		project_p->SetShapeFileNeedsFirstSave(false);
	}
	
	if (!table_only) project_p->shp_fname = wxFileName(new_shp);
	grid_base->dbf_file_name = wxFileName(new_sp_dbf);
	grid_base->dbf_file_name_no_ext = grid_base->dbf_file_name.GetName();
	if (space_time) {
		grid_base->dbf_tm_file_name = wxFileName(new_tm_dbf);
		grid_base->dbf_tm_file_name_no_ext =
			grid_base->dbf_tm_file_name.GetName();
	}
	
	OnSaveProject(event);
	
	// Must notify all frames that project name has changed so that they
	// can update titles and legend as needed.  At the moment, no
	// views contain the name of the Project, so no update needed.
}

void MyFrame::OnExportToCsvFile(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetGridBase()) return;
	ExportCsvDlg dlg(this, project_p);
	dlg.ShowModal();
}

bool MyFrame::SaveTableSpace()
{
	if (!project_p || !project_p->GetGridBase()) return false;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	DbfFileHeader backup_header = grid_base->orig_header;

	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxString curr_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	
	wxString err_msg;
	bool success = grid_base->WriteToDbf(curr_dbf, err_msg);
	if (!success) {
		grid_base->orig_header = backup_header;
		wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
		return false;
	}
	project->SetAllowEnableSave(true);
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
								   false);
	LOG_MSG("Table saved successfully");
	return true;
}

bool MyFrame::SaveTableSpaceTime()
{
	if (!project_p || !project_p->GetGridBase()) return false;
	Project* project = project_p;
	DbfGridTableBase* grid_base = project->GetGridBase();
	DbfFileHeader backup_sp_header = grid_base->orig_header;
	DbfFileHeader backup_tm_header = grid_base->orig_header_tm;
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	grid_base->orig_header_tm.year = timeinfo->tm_year+1900;
	grid_base->orig_header_tm.month = timeinfo->tm_mon+1;
	grid_base->orig_header_tm.day = timeinfo->tm_mday;
	
	wxString curr_sp_dbf = grid_base->GetSpaceDbfFileName().GetFullPath();
	wxString curr_tm_dbf = grid_base->GetTimeDbfFileName().GetFullPath();
	
	wxString err_msg;
	bool success = grid_base->WriteToSpaceTimeDbf(curr_sp_dbf,
												  curr_tm_dbf, err_msg);
	if (!success) {
		grid_base->orig_header = backup_sp_header;
		grid_base->orig_header_tm = backup_tm_header;
		wxMessageDialog dlg (this, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
		return false;
	}
	project->SetAllowEnableSave(true);
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), XRCID("ID_SAVE_PROJECT"),
					false);
	LOG_MSG("Space-Time tables saved successfully");
	return true;
}

void MyFrame::OnGeneratePointShpFile(wxCommandEvent& event)
{
	if (!GetProject() || !GetProject()->GetGridBase()) return;
	Project* p = GetProject();
	DbfGridTableBase* grid_base = p->GetGridBase();
	VariableSettingsDlg VS(GetProject(), VariableSettingsDlg::bivariate, false,
						   "New Map Coordinates");
	if (VS.ShowModal() != wxID_OK) return;
	
	std::vector<double> x;
	std::vector<double> y;
	grid_base->col_data[VS.col_ids[0]]->GetVec(x, VS.var_info[0].time);
	grid_base->col_data[VS.col_ids[1]]->GetVec(y, VS.var_info[1].time);
	ShapeUtils::populatePointShpFile(x, y, p->index_data, p->main_data);
	p->CreateShapefileFromPoints(x, y);
	LOG(p->main_data.records.size());
	UpdateToolbarAndMenus();
	MapNewFrame* nf = new MapNewFrame(this, p,
									  CatClassification::no_theme,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnRegressionClassic(wxCommandEvent& event)
{
	wxString regFileName;
	if (IsProjectOpen()) {
		if (project_p->regression_dlg) {
			LOG_MSG("Project::regression_dlg pointer is not null!");
			return;
		}
	} else {
		OnOpenTableOnly(event);		
		if (!IsProjectOpen()) return;
	}
	regFileName = project_p->GetMainDir() + project_p->GetMainName()
		+ "Regression.txt";
		
	RegressionDlg* regDlg;
	regDlg = new RegressionDlg(project_p, this);
	project_p->regression_dlg = regDlg;
	regDlg->Show(true);
}


void MyFrame::DisplayRegression(const wxString dump)
{
	RegressionReportDlg *regReportDlg = new RegressionReportDlg(this, dump);
	regReportDlg->Show(true);
	regReportDlg->m_textbox->SetSelection(0, 0);
}

void MyFrame::OnCondPlotChoices(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnCondPlotChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_COND_MENU");
	
	if (popupMenu) {
		Project* p = GetProject();
		bool proj_open = (p != 0);
		bool shp_proj = proj_open && !p->IsTableOnlyProject();
		
		GeneralWxUtils::EnableMenuItem(popupMenu,
									   XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
									   shp_proj);
		GeneralWxUtils::EnableMenuItem(popupMenu,
									   XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
									   proj_open);
		GeneralWxUtils::EnableMenuItem(popupMenu,
									XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
									proj_open);
		PopupMenu(popupMenu, wxDefaultPosition);
	}
	LOG_MSG("Exiting MyFrame::OnCondPlotChoices");
}

void MyFrame::OnShowConditionalMapView(wxCommandEvent& WXUNUSED(event) )
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
							"Conditional Map Variables",
							"Horizontal Cells", "Vertical Cells",
							"Map Theme");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalMapFrame* subframe =
	new ConditionalMapFrame(MyFrame::theFrame, project_p,
							dlg.var_info, dlg.col_ids,
							"Conditional Map", wxDefaultPosition,
							GeoDaConst::cond_view_default_size);
}

void MyFrame::OnShowConditionalHistView(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
							"Conditional Histogram Variables",
							"Horizontal Cells", "Vertical Cells",
							"Histogram Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalHistogramFrame* subframe =
	new ConditionalHistogramFrame(MyFrame::theFrame, project_p,
								  dlg.var_info, dlg.col_ids,
								  "Conditional Histogram", wxDefaultPosition,
								  GeoDaConst::cond_view_default_size);
}

void MyFrame::OnShowConditionalScatterView(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate, false,
							"Conditional Scatter Plot Variables",
							"Horizontal Cells", "Vertical Cells",
							"Independent Var (x-axis)",
							"Dependent Var (y-axis)");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalScatterPlotFrame* subframe =
	new ConditionalScatterPlotFrame(MyFrame::theFrame, project_p,
									dlg.var_info, dlg.col_ids,
									"Conditional Scatter Plot",
									wxDefaultPosition,
									GeoDaConst::cond_view_default_size);
}

void MyFrame::OnShowCartogramNewView(wxCommandEvent& WXUNUSED(event) )
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
							"Cartogram Variables",
							"Circle Size", "Circle Color");
	if (dlg.ShowModal() != wxID_OK) return;
	
	CartogramNewFrame* subframe =
		new CartogramNewFrame(MyFrame::theFrame, project_p,
							  dlg.var_info, dlg.col_ids,
							  "Cartogram", wxDefaultPosition,
							  GeoDaConst::map_default_size);
}

void MyFrame::OnCartogramImprove1(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(0);
	}
}

void MyFrame::OnCartogramImprove2(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(1);
	}
}

void MyFrame::OnCartogramImprove3(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(2);
	}
}

void MyFrame::OnCartogramImprove4(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(3);
	}
}

void MyFrame::OnCartogramImprove5(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(4);
	}
}

void MyFrame::OnCartogramImprove6(wxCommandEvent& event )
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(5);
	}
}

void MyFrame::OnExploreHist(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreHist");
	
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	
	HistogramFrame* f = new HistogramFrame(MyFrame::theFrame, project_p,
										   VS.var_info,
										   VS.col_ids, "Histogram",
										   wxDefaultPosition,
										   GeoDaConst::hist_default_size);
	
	LOG_MSG("Exiting MyFrame::OnExploreHist");
}

void MyFrame::OnExploreScatterplot(wxCommandEvent& event)
{
	OnExploreScatterNewPlot(event);
}

void MyFrame::OnExploreScatterNewPlot(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
							"Scatter Plot Variables",
							"Independent Var X", "Dependent Var Y");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Scatter Plot");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(MyFrame::theFrame, project_p,
							dlg.var_info, dlg.col_ids,
							false, title, wxDefaultPosition,
							GeoDaConst::scatterplot_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnExploreBubbleChart(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate, false,
							"Bubble Chart Variables",
							"X-Axis", "Y-Axis",
							"Bubble Size", "Standard Deviation Color");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Bubble Chart");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(MyFrame::theFrame, project_p,
							dlg.var_info, dlg.col_ids,
							true, title, wxDefaultPosition,
							GeoDaConst::bubble_chart_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void MyFrame::OnExploreTestMap(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreTestMap");
	
	//MapNewFrame* subframe = new MapNewFrame(frame, project_p,
	//										MapNewCanvas::no_theme,
	//										MapNewCanvas::no_smoothing,
	//										wxDefaultPosition,
	//										GeoDaConst::map_default_size);
	//subframe->UpdateTitle();
	
	//	TestMapFrame *subframe = new TestMapFrame(frame, project_p,
	//											  "Test Map Frame",
	//											  wxDefaultPosition,
	//											  GeoDaConst::map_default_size,
	//											  wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnExploreTestMap");
}


void MyFrame::OnExploreTestTable(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnExploreTestTable");	
	//TestTableFrame *subframe = new TestTableFrame(frame, project_p,
	//											"Test Table Frame",
	//											wxDefaultPosition,
	//											GeoDaConst::map_default_size,
	//											wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting MyFrame::OnExploreTestTable");
}

void MyFrame::OnExploreNewBox(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxSize size = GeoDaConst::boxplot_default_size;
	int w = size.GetWidth();
	if (VS.var_info[0].is_time_variant) size.SetWidth((w*3)/2);
	else size.SetWidth(w/2);
	BoxNewPlotFrame *sf = new BoxNewPlotFrame(MyFrame::theFrame, GetProject(),
											  VS.var_info, VS.col_ids,
											  "Box Plot", wxDefaultPosition,
											  size);
}

void MyFrame::OnExplorePCP(wxCommandEvent& WXUNUSED(event))
{
	PCPDlg dlg(project_p->GetGridBase(),this);
	if (dlg.ShowModal() != wxID_OK) return;
	PCPNewFrame* s = new PCPNewFrame(this, GetProject(), dlg.var_info,
									 dlg.col_ids);
}

void MyFrame::OnExplore3DP(wxCommandEvent& WXUNUSED(event))
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
							"3D Scatter Plot Variables", "X", "Y", "Z");
	if (dlg.ShowModal() != wxID_OK) return;
	
	C3DPlotFrame *subframe =
		new C3DPlotFrame(MyFrame::theFrame, project_p,
						 dlg.var_info, dlg.col_ids,
						 "3D Plot", wxDefaultPosition,
						 GeoDaConst::three_d_default_size,
						 wxDEFAULT_FRAME_STYLE,
						 dlg.v1_single_time, dlg.v1_name_with_time,
						 dlg.v2_single_time, dlg.v2_name_with_time,
						 dlg.v3_single_time, dlg.v3_name_with_time);
	
}

void MyFrame::OnToolOpenNewTable(wxCommandEvent& WXUNUSED(event))
{
	OnOpenNewTable();
}

void MyFrame::OnMoranMenuChoices(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnMoranMenuChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_MORAN_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnMoranMenuChoices");
}

void MyFrame::OnOpenMSPL(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::univariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(MyFrame::theFrame,
													   project_p, lc);
}

void MyFrame::OnOpenGMoran(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::bivariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(MyFrame::theFrame,
													   project_p, lc);
}

void MyFrame::OnOpenMoranEB(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false,
						   "Empirical Bayes Rate Standardization Variables",
						   "Event Variable", "Base Variable");
	if (VS.ShowModal() != wxID_OK) return;
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
										VS.var_info, VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
										false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(MyFrame::theFrame,
													   project_p, lc);
}

void MyFrame::OnLisaMenuChoices(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering MyFrame::OnLisaMenuChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_LISA_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting MyFrame::OnLisaMenuChoices");
}

void MyFrame::OnOpenUniLisa(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;

	GalWeight* gal = GetGal();
	if (!gal) return;
		
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap && !LWO.m_Moran) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::univariate,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(MyFrame::theFrame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, true, false, false);
	}
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, false, false, false,
												  wxDefaultPosition);
	}
}

void MyFrame::OnOpenMultiLisa(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, false);
	if (VS.ShowModal() != wxID_OK) return;

	GalWeight* gal = GetGal();
	if (!gal) return;

	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap &&!LWO.m_Moran) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::bivariate,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(MyFrame::theFrame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, true, true, false);
	}
	
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, false, true, false);
	}
}

void MyFrame::OnOpenLisaEB(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, 9, 0);
	if (VS.ShowModal() != wxID_OK) return;
	
	GalWeight* gal = GetGal();
	if (!gal) return;
	
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) return;
	
	LisaCoordinator* lc = new LisaCoordinator(gal, project_p->GetGridBase(),
											  VS.var_info,
											  VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
											  true);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(MyFrame::theFrame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, true, false, true);
	}
	
	if (LWO.m_SigMap) {
		LisaMapNewFrame *sf = new LisaMapNewFrame(MyFrame::theFrame, project_p,
												  lc, false, false, true);
	}	
}

void MyFrame::OnOpenGetisOrd(wxCommandEvent& event)
{
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false);
	if (VS.ShowModal() != wxID_OK) return;
	if (!GetGal()) return;
		
	GetisOrdChoiceDlg dlg(this);
		
	if (dlg.ShowModal() != wxID_OK) return;
		
	if (!dlg.Gi_ClustMap_norm && !dlg.Gi_SigMap_norm &&
		!dlg.GiStar_ClustMap_norm && !dlg.GiStar_SigMap_norm &&
		!dlg.Gi_ClustMap_perm && !dlg.Gi_SigMap_perm &&
		!dlg.GiStar_ClustMap_perm && !dlg.GiStar_SigMap_perm) return;

	GStatCoordinator* gc = new GStatCoordinator(GetGal(),
												project_p->GetGridBase(),
												VS.var_info, VS.col_ids,
												dlg.row_standardize_weights);
	if (!gc || !gc->IsOk()) {
		// print error message
		delete gc;
		return;
	}
	
	if (dlg.Gi_ClustMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_clus_norm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_SigMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_sig_norm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_ClustMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_clus_norm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_SigMap_norm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_sig_norm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_ClustMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_clus_perm, dlg.row_standardize_weights);
	}
	if (dlg.Gi_SigMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::Gi_sig_perm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_ClustMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_clus_perm, dlg.row_standardize_weights);
	}
	if (dlg.GiStar_SigMap_perm) {
		GetisOrdMapNewFrame* f = new GetisOrdMapNewFrame(this, project_p, gc,
			GetisOrdMapNewFrame::GiStar_sig_perm, dlg.row_standardize_weights);
	}	
}

void MyFrame::OnNewCustomCatClassifA(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	}
}

void MyFrame::OnNewCustomCatClassifB(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifB();
	}
}

void MyFrame::OnNewCustomCatClassifC(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifC();
	}
}

void MyFrame::OnCCClassifA(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);

	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	}
}

void MyFrame::OnCCClassifB(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifB(titles[cc_menu_num]);
	}	
}

void MyFrame::OnCCClassifC(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifC(titles[cc_menu_num]);
	}	
}

void MyFrame::OnCCClassifA0(wxCommandEvent& e) { OnCCClassifA(0); }
void MyFrame::OnCCClassifA1(wxCommandEvent& e) { OnCCClassifA(1); }
void MyFrame::OnCCClassifA2(wxCommandEvent& e) { OnCCClassifA(2); }
void MyFrame::OnCCClassifA3(wxCommandEvent& e) { OnCCClassifA(3); }
void MyFrame::OnCCClassifA4(wxCommandEvent& e) { OnCCClassifA(4); }
void MyFrame::OnCCClassifA5(wxCommandEvent& e) { OnCCClassifA(5); }
void MyFrame::OnCCClassifA6(wxCommandEvent& e) { OnCCClassifA(6); }
void MyFrame::OnCCClassifA7(wxCommandEvent& e) { OnCCClassifA(7); }
void MyFrame::OnCCClassifA8(wxCommandEvent& e) { OnCCClassifA(8); }
void MyFrame::OnCCClassifA9(wxCommandEvent& e) { OnCCClassifA(9); }
void MyFrame::OnCCClassifA10(wxCommandEvent& e) { OnCCClassifA(10); }
void MyFrame::OnCCClassifA11(wxCommandEvent& e) { OnCCClassifA(11); }
void MyFrame::OnCCClassifA12(wxCommandEvent& e) { OnCCClassifA(12); }
void MyFrame::OnCCClassifA13(wxCommandEvent& e) { OnCCClassifA(13); }
void MyFrame::OnCCClassifA14(wxCommandEvent& e) { OnCCClassifA(14); }
void MyFrame::OnCCClassifA15(wxCommandEvent& e) { OnCCClassifA(15); }
void MyFrame::OnCCClassifA16(wxCommandEvent& e) { OnCCClassifA(16); }
void MyFrame::OnCCClassifA17(wxCommandEvent& e) { OnCCClassifA(17); }
void MyFrame::OnCCClassifA18(wxCommandEvent& e) { OnCCClassifA(18); }
void MyFrame::OnCCClassifA19(wxCommandEvent& e) { OnCCClassifA(19); }
void MyFrame::OnCCClassifA20(wxCommandEvent& e) { OnCCClassifA(20); }
void MyFrame::OnCCClassifA21(wxCommandEvent& e) { OnCCClassifA(21); }
void MyFrame::OnCCClassifA22(wxCommandEvent& e) { OnCCClassifA(22); }
void MyFrame::OnCCClassifA23(wxCommandEvent& e) { OnCCClassifA(23); }
void MyFrame::OnCCClassifA24(wxCommandEvent& e) { OnCCClassifA(24); }
void MyFrame::OnCCClassifA25(wxCommandEvent& e) { OnCCClassifA(25); }
void MyFrame::OnCCClassifA26(wxCommandEvent& e) { OnCCClassifA(26); }
void MyFrame::OnCCClassifA27(wxCommandEvent& e) { OnCCClassifA(27); }
void MyFrame::OnCCClassifA28(wxCommandEvent& e) { OnCCClassifA(28); }
void MyFrame::OnCCClassifA29(wxCommandEvent& e) { OnCCClassifA(29); }

void MyFrame::OnCCClassifB0(wxCommandEvent& e) { OnCCClassifB(0); }
void MyFrame::OnCCClassifB1(wxCommandEvent& e) { OnCCClassifB(1); }
void MyFrame::OnCCClassifB2(wxCommandEvent& e) { OnCCClassifB(2); }
void MyFrame::OnCCClassifB3(wxCommandEvent& e) { OnCCClassifB(3); }
void MyFrame::OnCCClassifB4(wxCommandEvent& e) { OnCCClassifB(4); }
void MyFrame::OnCCClassifB5(wxCommandEvent& e) { OnCCClassifB(5); }
void MyFrame::OnCCClassifB6(wxCommandEvent& e) { OnCCClassifB(6); }
void MyFrame::OnCCClassifB7(wxCommandEvent& e) { OnCCClassifB(7); }
void MyFrame::OnCCClassifB8(wxCommandEvent& e) { OnCCClassifB(8); }
void MyFrame::OnCCClassifB9(wxCommandEvent& e) { OnCCClassifB(9); }
void MyFrame::OnCCClassifB10(wxCommandEvent& e) { OnCCClassifB(10); }
void MyFrame::OnCCClassifB11(wxCommandEvent& e) { OnCCClassifB(11); }
void MyFrame::OnCCClassifB12(wxCommandEvent& e) { OnCCClassifB(12); }
void MyFrame::OnCCClassifB13(wxCommandEvent& e) { OnCCClassifB(13); }
void MyFrame::OnCCClassifB14(wxCommandEvent& e) { OnCCClassifB(14); }
void MyFrame::OnCCClassifB15(wxCommandEvent& e) { OnCCClassifB(15); }
void MyFrame::OnCCClassifB16(wxCommandEvent& e) { OnCCClassifB(16); }
void MyFrame::OnCCClassifB17(wxCommandEvent& e) { OnCCClassifB(17); }
void MyFrame::OnCCClassifB18(wxCommandEvent& e) { OnCCClassifB(18); }
void MyFrame::OnCCClassifB19(wxCommandEvent& e) { OnCCClassifB(19); }
void MyFrame::OnCCClassifB20(wxCommandEvent& e) { OnCCClassifB(20); }
void MyFrame::OnCCClassifB21(wxCommandEvent& e) { OnCCClassifB(21); }
void MyFrame::OnCCClassifB22(wxCommandEvent& e) { OnCCClassifB(22); }
void MyFrame::OnCCClassifB23(wxCommandEvent& e) { OnCCClassifB(23); }
void MyFrame::OnCCClassifB24(wxCommandEvent& e) { OnCCClassifB(24); }
void MyFrame::OnCCClassifB25(wxCommandEvent& e) { OnCCClassifB(25); }
void MyFrame::OnCCClassifB26(wxCommandEvent& e) { OnCCClassifB(26); }
void MyFrame::OnCCClassifB27(wxCommandEvent& e) { OnCCClassifB(27); }
void MyFrame::OnCCClassifB28(wxCommandEvent& e) { OnCCClassifB(28); }
void MyFrame::OnCCClassifB29(wxCommandEvent& e) { OnCCClassifB(29); }

void MyFrame::OnCCClassifC0(wxCommandEvent& e) { OnCCClassifC(0); }
void MyFrame::OnCCClassifC1(wxCommandEvent& e) { OnCCClassifC(1); }
void MyFrame::OnCCClassifC2(wxCommandEvent& e) { OnCCClassifC(2); }
void MyFrame::OnCCClassifC3(wxCommandEvent& e) { OnCCClassifC(3); }
void MyFrame::OnCCClassifC4(wxCommandEvent& e) { OnCCClassifC(4); }
void MyFrame::OnCCClassifC5(wxCommandEvent& e) { OnCCClassifC(5); }
void MyFrame::OnCCClassifC6(wxCommandEvent& e) { OnCCClassifC(6); }
void MyFrame::OnCCClassifC7(wxCommandEvent& e) { OnCCClassifC(7); }
void MyFrame::OnCCClassifC8(wxCommandEvent& e) { OnCCClassifC(8); }
void MyFrame::OnCCClassifC9(wxCommandEvent& e) { OnCCClassifC(9); }
void MyFrame::OnCCClassifC10(wxCommandEvent& e) { OnCCClassifC(10); }
void MyFrame::OnCCClassifC11(wxCommandEvent& e) { OnCCClassifC(11); }
void MyFrame::OnCCClassifC12(wxCommandEvent& e) { OnCCClassifC(12); }
void MyFrame::OnCCClassifC13(wxCommandEvent& e) { OnCCClassifC(13); }
void MyFrame::OnCCClassifC14(wxCommandEvent& e) { OnCCClassifC(14); }
void MyFrame::OnCCClassifC15(wxCommandEvent& e) { OnCCClassifC(15); }
void MyFrame::OnCCClassifC16(wxCommandEvent& e) { OnCCClassifC(16); }
void MyFrame::OnCCClassifC17(wxCommandEvent& e) { OnCCClassifC(17); }
void MyFrame::OnCCClassifC18(wxCommandEvent& e) { OnCCClassifC(18); }
void MyFrame::OnCCClassifC19(wxCommandEvent& e) { OnCCClassifC(19); }
void MyFrame::OnCCClassifC20(wxCommandEvent& e) { OnCCClassifC(20); }
void MyFrame::OnCCClassifC21(wxCommandEvent& e) { OnCCClassifC(21); }
void MyFrame::OnCCClassifC22(wxCommandEvent& e) { OnCCClassifC(22); }
void MyFrame::OnCCClassifC23(wxCommandEvent& e) { OnCCClassifC(23); }
void MyFrame::OnCCClassifC24(wxCommandEvent& e) { OnCCClassifC(24); }
void MyFrame::OnCCClassifC25(wxCommandEvent& e) { OnCCClassifC(25); }
void MyFrame::OnCCClassifC26(wxCommandEvent& e) { OnCCClassifC(26); }
void MyFrame::OnCCClassifC27(wxCommandEvent& e) { OnCCClassifC(27); }
void MyFrame::OnCCClassifC28(wxCommandEvent& e) { OnCCClassifC(28); }
void MyFrame::OnCCClassifC29(wxCommandEvent& e) { OnCCClassifC(29); }

void MyFrame::OnOpenThemelessMap(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnThemelessMap(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnThemeless(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnThemeless(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnThemeless(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnThemeless(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnThemeless(event);
	}
}

void MyFrame::OnOpenQuantile(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::quantile,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnQuantile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnQuantile(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnQuantile(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnQuantile(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnQuantile(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnQuantile(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnQuantile(event);
	}
}

void MyFrame::OnOpenPercentile(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::percentile,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnPercentile(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnPercentile(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnPercentile(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnPercentile(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnPercentile(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnPercentile(event);
	}
}

void MyFrame::OnOpenHinge15(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::hinge_15,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnHinge15(event);
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnHinge15(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge15(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnHinge15(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnHinge15(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnHinge15(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnHinge15(event);
	}
}

void MyFrame::OnOpenHinge30(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::hinge_30,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnHinge30(event);
	} else if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnHinge30(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge30(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnHinge30(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnHinge30(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnHinge30(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnHinge30(event);
	}
}

void MyFrame::OnOpenStddev(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::stddev,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnStdDevMap(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnStdDevMap(event);
	}
}

void MyFrame::OnOpenNaturalBreaks(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::natural_breaks,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnNaturalBreaks(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnNaturalBreaks(event);
	}
}

void MyFrame::OnOpenEqualIntervals(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::equal_intervals,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();	
}

void MyFrame::OnEqualIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnEqualIntervals(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnEqualIntervals(event);
	}
}

void MyFrame::OnOpenUniqueValues(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::unique_values,
									  MapNewCanvas::no_smoothing,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnUniqueValues(event);
	} else if (CatClassifFrame* f = dynamic_cast<CatClassifFrame*>(t)) {
		f->OnUniqueValues(event);
	}
}


void MyFrame::OnCondVertThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::no_theme);
	}
}

void MyFrame::OnCondVertQuantile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::quantile);
	}
}

void MyFrame::OnCondVertPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::percentile);
	}
}

void MyFrame::OnCondVertHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_15);
	}
}

void MyFrame::OnCondVertHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_30);
	}
}

void MyFrame::OnCondVertStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::stddev);
	}
}

void MyFrame::OnCondVertNaturalBreaks(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::natural_breaks);
	}
}

void MyFrame::OnCondVertEqualIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::equal_intervals);
	}
}

void MyFrame::OnCondVertUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::unique_values);
	}
}


void MyFrame::OnCondHorizThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::no_theme);
	}
}

void MyFrame::OnCondHorizQuantile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::quantile);
	}
}

void MyFrame::OnCondHorizPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::percentile);
	}
}

void MyFrame::OnCondHorizHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_15);
	}
}

void MyFrame::OnCondHorizHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_30);
	}
}

void MyFrame::OnCondHorizStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::stddev);
	}
}

void MyFrame::OnCondHorizNaturalBreaks(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::natural_breaks);
	}
}

void MyFrame::OnCondHorizEqualIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::equal_intervals);
	}
}

void MyFrame::OnCondHorizUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::unique_values);
	}
}


void MyFrame::OnSaveCategories(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnSaveCategories(event);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnSaveCategories(event);
	}
}

void MyFrame::OnOpenRawrate(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::raw_rate,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnRawrate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnRawrate(event);
	}
}

void MyFrame::OnOpenExcessrisk(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::excess_risk_theme,
									  MapNewCanvas::excess_risk,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnExcessrisk(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnExcessRisk(event);
	}
}

void MyFrame::OnOpenEmpiricalBayes(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::empirical_bayes,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnEmpiricalBayes(event);
	}
}

void MyFrame::OnOpenSpatialRate(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::spatial_rate,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}

void MyFrame::OnSpatialRate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSpatialRate(event);
	}
}

void MyFrame::OnOpenSpatialEmpiricalBayes(wxCommandEvent& event)
{
	MapNewFrame* nf = new MapNewFrame(MyFrame::theFrame, project_p,
									  CatClassification::no_theme,
									  MapNewCanvas::spatial_empirical_bayes,
									  wxDefaultPosition,
									  GeoDaConst::map_default_size);
	nf->UpdateTitle();
}


void MyFrame::OnSpatialEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSpatialEmpiricalBayes(event);
	}
}

void MyFrame::OnSaveResults(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveRates(event);
	}
}

void MyFrame::OnSelectIsolates(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnSelectIsolates(event);
	}
}

void MyFrame::OnSaveConnectivityToTable(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnSaveConnectivityToTable(event);
	}
}

void MyFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnHistogramIntervals(event);
	} else if (ConditionalHistogramFrame* f =
			   dynamic_cast<ConditionalHistogramFrame*>(t)) {
		f->OnHistogramIntervals(event);
	}else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnHistogramIntervals(event);
	}
}

void MyFrame::OnRan99Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan99Per(event);
	}
}

void MyFrame::OnRan199Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan199Per(event);
	}
}

void MyFrame::OnRan499Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (LisaScatterPlotFrame* f
			   = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan499Per(event);
	}
}

void MyFrame::OnRan999Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRan999Per(event);
	}
}

void MyFrame::OnRanOtherPer(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnRanOtherPer(event);
	}
}

void MyFrame::OnSaveMoranI(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSaveMoranI(event);
	}
}

void MyFrame::OnSigFilter05(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter05(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter05(event);
	}
}

void MyFrame::OnSigFilter01(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter01(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter01(event);
	}
}

void MyFrame::OnSigFilter001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter001(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter001(event);
	}
}

void MyFrame::OnSigFilter0001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSigFilter0001(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSigFilter0001(event);
	}
}

void MyFrame::OnAddMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->GetProject()->AddMeanCenters();
	}
}

void MyFrame::OnAddCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->GetProject()->AddCentroids();
	}
}

void MyFrame::OnDisplayMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnDisplayMeanCenters();
	}
}

void MyFrame::OnDisplayCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnDisplayCentroids();
	}
}

void MyFrame::OnDisplayVoronoiDiagram(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnDisplayVoronoiDiagram();
	}
}

void MyFrame::OnSaveVoronoiToShapefile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveVoronoiToShapefile();
	} else {
		if (project_p) project_p->SaveVoronoiToShapefile();
	}
}

void MyFrame::OnSaveMeanCntrsToShapefile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (project_p) project_p->SaveCentersToShapefile(true);
}

void MyFrame::OnSaveCentroidsToShapefile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (project_p) project_p->SaveCentersToShapefile(false);
}

void MyFrame::OnSaveVoronoiDupsToTable(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapNewFrame* f = dynamic_cast<MapNewFrame*>(t)) {
		f->OnSaveVoronoiDupsToTable();
	} else {
		if (project_p) project_p->SaveVoronoiDupsToTable();
	}
}

void MyFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSaveGetisOrd(event);
	}
}

void MyFrame::OnSaveLisa(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSaveLisa(event);
	}
}

void MyFrame::OnSelectCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectCores(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSelectCores(event);
	}
}

void MyFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	}
}

void MyFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapNewFrame* f = dynamic_cast<LisaMapNewFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	} else if (GetisOrdMapNewFrame* f = dynamic_cast<GetisOrdMapNewFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	}
}

void MyFrame::OnAddNeighborsToSelection(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (!project_p) return;
	if (!project_p->GetWManager() ||
		(project_p->GetWManager() &&
		 !project_p->GetWManager()->GetCurrWeight())) {
		// prompt the user for a weights matrix
		GetGal();
	}
	project_p->AddNeighborsToSelection();
}

void MyFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewStandardizedData(event);
	}	
}

void MyFrame::OnViewOriginalData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	}
}

void MyFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	}
}

void MyFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelected(event);
	}
}

void MyFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (ConnectivityHistFrame* f =
			   dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnDisplayStatistics(event);
	}
}

void MyFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnShowAxesThroughOrigin(event);
	}
}

void MyFrame::OnShowAxes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (BoxNewPlotFrame* f = dynamic_cast<BoxNewPlotFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (ConnectivityHistFrame* f =
			   dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (ConditionalHistogramFrame* f =
			   dynamic_cast<ConditionalHistogramFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (PCPNewFrame* f = dynamic_cast<PCPNewFrame*>(t)) {
		f->OnShowAxes(event);
	}
}

void MyFrame::OnDisplayAxesScaleValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalScatterPlotFrame* f =
		dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnDisplayAxesScaleValues(event);
	}
}

void MyFrame::OnDisplaySlopeValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalScatterPlotFrame* f =
		dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnDisplaySlopeValues(event);
	}
}

void MyFrame::OnTimeSyncVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnTimeSyncVariable(var_index);
}

void MyFrame::OnTimeSyncVariable1(wxCommandEvent& event)
{
	OnTimeSyncVariable(0);
}

void MyFrame::OnTimeSyncVariable2(wxCommandEvent& event)
{
	OnTimeSyncVariable(1);
}

void MyFrame::OnTimeSyncVariable3(wxCommandEvent& event)
{
	OnTimeSyncVariable(2);
}

void MyFrame::OnTimeSyncVariable4(wxCommandEvent& event)
{
	OnTimeSyncVariable(3);
}

void MyFrame::OnFixedScaleVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFixedScaleVariable(var_index);
}

void MyFrame::OnFixedScaleVariable1(wxCommandEvent& event)
{
	OnFixedScaleVariable(0);
}

void MyFrame::OnFixedScaleVariable2(wxCommandEvent& event)
{
	OnFixedScaleVariable(1);
}

void MyFrame::OnFixedScaleVariable3(wxCommandEvent& event)
{
	OnFixedScaleVariable(2);
}

void MyFrame::OnFixedScaleVariable4(wxCommandEvent& event)
{
	OnFixedScaleVariable(3);
}

void MyFrame::OnPlotsPerView(int plots_per_view)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerView(plots_per_view);
}

void MyFrame::OnPlotsPerView1(wxCommandEvent& event)
{
	OnPlotsPerView(1);
}

void MyFrame::OnPlotsPerView2(wxCommandEvent& event)
{
	OnPlotsPerView(2);
}

void MyFrame::OnPlotsPerView3(wxCommandEvent& event)
{
	OnPlotsPerView(3);
}

void MyFrame::OnPlotsPerView4(wxCommandEvent& event)
{
	OnPlotsPerView(4);
}

void MyFrame::OnPlotsPerView5(wxCommandEvent& event)
{
	OnPlotsPerView(5);
}

void MyFrame::OnPlotsPerView6(wxCommandEvent& event)
{
	OnPlotsPerView(6);
}

void MyFrame::OnPlotsPerView7(wxCommandEvent& event)
{
	OnPlotsPerView(7);
}

void MyFrame::OnPlotsPerView8(wxCommandEvent& event)
{
	OnPlotsPerView(8);
}

void MyFrame::OnPlotsPerView9(wxCommandEvent& event)
{
	OnPlotsPerView(9);
}

void MyFrame::OnPlotsPerView10(wxCommandEvent& event)
{
	OnPlotsPerView(10);
}

void MyFrame::OnPlotsPerViewOther(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewOther();
}

void MyFrame::OnPlotsPerViewAll(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewAll();
}

void MyFrame::OnDisplayStatusBar(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnDisplayStatusBar(event);
}

void MyFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event) )
{
	wxDialog dlg;
	wxXmlResource::Get()->LoadDialog(&dlg, this, "IDD_ABOUTBOX");
	dlg.ShowModal();
}


