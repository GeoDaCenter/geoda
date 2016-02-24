/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
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
#include <fstream>
#include <sstream>
#include <string>
#include <locale>
#include <locale.h>
#include <sstream>
#include <iomanip>

#include <boost/foreach.hpp>

#include <sqlite3.h>

#include <wx/sysopt.h>
#include <wx/wxprec.h>
#include <wx/aboutdlg.h>
#include <wx/valtext.h>
#include <wx/image.h>
#include <wx/dcsvg.h>		// SVG DC
#include <wx/dcps.h>		// PostScript DC
#include <wx/dcmemory.h>	// Memory DC
#include <wx/xrc/xmlres.h>	// XRC XML resouces
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/app.h>
#include <wx/sysopt.h>
#include <wx/position.h>
#include <wx/progdlg.h>
#include <wx/filedlg.h>
#include <wx/filefn.h> // for wxCopyFile and wxFileExists
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/regex.h>
#include <wx/numformatter.h>
#include <wx/webview.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/fs_mem.h>
#include <wx/settings.h>

#include "DataViewer/DataChangeType.h"
#include "DataViewer/DbfTable.h"
#include "DataViewer/DataViewerAddColDlg.h"
#include "DataViewer/DataViewerDeleteColDlg.h"
#include "DataViewer/DataViewerEditFieldPropertiesDlg.h"
#include "DataViewer/MergeTableDlg.h"
#include "DataViewer/TableBase.h"
#include "DataViewer/TableFrame.h"
#include "DataViewer/TableInterface.h"
#include "DataViewer/TableState.h"
#include "DataViewer/TimeState.h"
#include "DialogTools/CreatingWeightDlg.h"
#include "DialogTools/CatClassifDlg.h"
#include "DialogTools/PCPDlg.h"
#include "DialogTools/ProjectInfoDlg.h"
#include "DialogTools/FieldNewCalcSheetDlg.h"
#include "DialogTools/CalculatorDlg.h"
#include "DialogTools/DataMovieDlg.h"
#include "DialogTools/ExportCsvDlg.h"
#include "DialogTools/ImportCsvDlg.h"
#include "DialogTools/RangeSelectionDlg.h"
#include "DialogTools/TimeChooserDlg.h"
#include "DialogTools/VarGroupingEditorDlg.h"
#include "DialogTools/VariableSettingsDlg.h"
#include "DialogTools/SelectWeightsDlg.h"
#include "DialogTools/SaveSelectionDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/LisaWhat2OpenDlg.h"
#include "DialogTools/RegressionDlg.h"
#include "DialogTools/RegressionReportDlg.h"
#include "DialogTools/ProgressDlg.h"
#include "DialogTools/GetisOrdChoiceDlg.h"
#include "DialogTools/ConnectDatasourceDlg.h"
#include "DialogTools/ExportDataDlg.h"
#include "DialogTools/TimeEditorDlg.h"
#include "DialogTools/SaveAsDlg.h"
#include "DialogTools/LocaleSetupDlg.h"
#include "DialogTools/WeightsManDlg.h"
#include "DialogTools/PublishDlg.h"
#include "DialogTools/BasemapConfDlg.h"

#include "Explore/CatClassification.h"
#include "Explore/CovSpView.h"
#include "Explore/CorrelParamsDlg.h"
#include "Explore/CorrelogramView.h"
#include "Explore/GetisOrdMapNewView.h"
#include "Explore/LineChartView.h"
#include "Explore/LisaMapNewView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/ConditionalMapView.h"
#include "Explore/ConditionalNewView.h"
#include "Explore/ConditionalScatterPlotView.h"
#include "Explore/ConnectivityHistView.h"
#include "Explore/ConnectivityMapView.h"
#include "Explore/ConditionalHistogramView.h"
#include "Explore/CartogramNewView.h"
#include "Explore/GStatCoordinator.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/ScatterPlotMatView.h"
#include "Explore/MapNewView.h"
#include "Explore/PCPNewView.h"
#include "Explore/HistogramView.h"
#include "Explore/BoxNewPlotView.h"
#include "Explore/3DPlotView.h"
#include "Explore/WebViewExampleWin.h"
#include "Explore/Basemap.h"

//#include "TestMapView.h"

#include "Regression/DiagnosticReport.h"

#include "ShapeOperations/CsvFileUtils.h"
#include "ShpFile.h"
#include "ShapeOperations/ShapeUtils.h"
#include "ShapeOperations/WeightsManager.h"
#include "ShapeOperations/WeightsManState.h"
#include "ShapeOperations/OGRDataAdapter.h"

#include "VarCalc/CalcHelp.h"

#include "GdaException.h"
#include "FramesManager.h"
#include "GdaConst.h"
#include "GeneralWxUtils.h"
#include "VarTools.h"
#include "logger.h"
#include "Project.h"
#include "TemplateFrame.h"
#include "SaveButtonManager.h"
#include "GeoDa.h"
#include "ogrsf_frmts.h"
#include <stdlib.h>
#include "cpl_conv.h"
#include "version.h"


//The XML Handler should be explicitly registered:
#include <wx/xrc/xh_auitoolb.h>

// The following is defined in rc/GdaAppResouces.cpp.  This file was
// compiled with:
/*
 wxrc dialogs.xrc menus.xrc toolbar.xrc \
   --cpp-code --output=GdaAppResources.cpp --function=GdaInitXmlResource
*/
// and combines all resouces file into single source file that is linked into
// the application binary.
extern void GdaInitXmlResource();

const int ID_TEST_MAP_FRAME = wxID_HIGHEST + 10;

IMPLEMENT_APP(GdaApp)

GdaApp::GdaApp() : checker(0), server(0)
{
	LOG_MSG("Entering GdaApp::GdaApp");
	//Don't call wxHandleFatalExceptions so that a core dump file will be
	//produced for debugging.
	//wxHandleFatalExceptions();
	LOG_MSG("Exiting GdaApp::GdaApp");
}

GdaApp::~GdaApp()
{
	if (server) delete server; server = 0;
}

#include "rc/GeoDaIcon-16x16.xpm"

bool GdaApp::OnInit(void)
{
	LOG_MSG("Entering GdaApp::OnInit");

	if (!wxApp::OnInit()) return false;

	OGRDataAdapter::GetInstance();
    
	if (!GeneralWxUtils::isMac()) {
		// GeoDa operates in single-instance mode.  This means that for
		// a given user, only one instance of GeoDa will remain open
		// at a time.  This is not needed on Mac OSX since by default
		// programs are only launched in single instance mode.  This might
		// not be true for launching directly from the command line, so
		// this might need to change in the future.
		checker = new wxSingleInstanceChecker("GdaApp");
		
		if (!checker->IsAnotherRunning()) {
			// This is the first instance of GeoDa running for this
			// user, so this instance becomes the "server."
			LOG_MSG("First instance of GeoDa: creating server for future "
					"program instances.");
			server = new GdaServer;
			if (!server->Create("GdaApp")) {
				LOG_MSG("Error: Failed to create in IPC service.");
			}
		} else {
			// Another instance of GeoDa is already running.  This other
			// instance is acting as the "server" so this instance
			// becomes the "client" and requests that the server open
			// the requested project before this instance exits.
		
			LOG_MSG("Another instance of GeoDa is already running.  Attempting "
					"to communicate with IPC.");
		
			GdaClient* client = new GdaClient;
		
			// host_name will be ignored under DDE (Windows and
			// possibly Linux), but will be used on platforms
			// implementing wxClient/wxServer using TCP/IP.
			wxString host_name("localhost");
		
			wxConnectionBase* connection =
				client->MakeConnection(host_name, "GdaApp", "GdaApp");
			
			if (connection) {
				connection->Execute(cmd_line_proj_file_name);
				connection->Disconnect();
				delete connection;
			} else {
				wxString msg;
				msg << "The existing GeoDa instance may be too busy to ";
				msg << "respond.\nPlease close any open dialogs and try again.";
				LOG_MSG(msg);
			}
			delete client;
			delete checker; // OnExit() won't be called if we return false
			checker = 0;
			return false;
		}
	}
 
    // By defaut, GDAL will use user's system locale to read any input datasource
    int lan = wxLocale::GetSystemLanguage();
    wxString locale_name = wxLocale::GetLanguageCanonicalName(lan);
    setlocale(LC_ALL, locale_name.mb_str());
    CPLsetlocale(LC_ALL, locale_name.mb_str());
    
    // However, user can change the Separators in GeoDa, after re-open the
    // datasource, CSV reader will use the Separators
    struct lconv *poLconv = localeconv();
    CPLSetConfigOption("GDAL_LOCALE_SEPARATOR", poLconv->thousands_sep);
    CPLSetConfigOption("GDAL_LOCALE_DECIMAL", poLconv->decimal_point);
    
    // forcing to C locale, which is used internally in GeoDa
    setlocale(LC_ALL, "C");
    
    // Other GDAL configurations
    //CPLSetConfigOption("SQLITE_LIST_ALL_TABLES", "YES");
    
	// will suppress "iCCP: known incorrect sRGB profile" warning message
	// in wxWidgets 2.9.5.  This is a bug in libpng.  See wxWidgets trac
	// issue #15331 for more details.
	wxLog::SetLogLevel(0);
    
    //wxSystemOptions::SetOption("mac.toolbar.no-native", 1);

    
	GdaConst::init();
	CalcHelp::init();
	
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
    
    
    wxXmlResource::Get()->AddHandler(new wxAuiToolBarXmlHandler);
    wxXmlResource::Get()->InitAllHandlers();
	
	
	//Required for virtual file system archive and memory support
    wxFileSystem::AddHandler(new wxArchiveFSHandler);
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
	
    // Create the memory files
    wxMemoryFSHandler::AddFile("logo.png", 
							   wxBitmap(GeoDaIcon_16x16_xpm), wxBITMAP_TYPE_PNG);
    wxMemoryFSHandler::AddFile("page1.htm",
							   "<html><head><title>File System Example</title>"
							   "<link rel='stylesheet' type='text/css' href='memory:test.css'>"
							   "</head><body><h1>Page 1</h1>"
							   "<p><img src='memory:logo.png'></p>"
							   "<p>Some text about <a href='memory:page2.htm'>Page 2</a>.</p></body>");
    wxMemoryFSHandler::AddFile("page2.htm",
							   "<html><head><title>File System Example</title>"
							   "<link rel='stylesheet' type='text/css' href='memory:test.css'>"
							   "</head><body><h1>Page 2</h1>"
							   "<p><a href='memory:page1.htm'>Page 1</a> was better.</p></body>");
    wxMemoryFSHandler::AddFile("test.css", "h1 {color: red;}");
	
	
    GdaInitXmlResource();  // call the init function in GdaAppResources.cpp	
	
	
	int frameWidth = 980; // 836 // 858
	int frameHeight = 80;
    
	if (GeneralWxUtils::isMac()) {
		frameWidth = 1012; // 643 // 665
		frameHeight = 80;
	}
	if (GeneralWxUtils::isWindows()) {
		// The default is assumed to be Vista / Win 7 family, but can check
		//   with GeneralWxUtils::isVista()
		frameWidth = 1020;
		frameHeight = 120;
		// Override default in case XP family of OSes is detected
		//if (GeneralWxUtils::isXP()) {
		//}
	}
	if (GeneralWxUtils::isUnix()) {  // assumes GTK
		frameWidth = 870; // 826 // 848
 		frameHeight = 84;
	}

    
    
	wxPoint appFramePos = wxDefaultPosition;
	if (GeneralWxUtils::isUnix() || GeneralWxUtils::isMac()) {
		appFramePos = wxPoint(80,60);
	}
    
    int screenX = wxSystemSettings::GetMetric ( wxSYS_SCREEN_X );
    if (screenX < frameWidth) {
        frameWidth = screenX;
        appFramePos = wxPoint(0, 50);
    }

	wxFrame* frame = new GdaFrame("GeoDa", appFramePos,
                                  wxSize(frameWidth, frameHeight),
								  wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX));
    frame->Show(true);
    frame->SetMinSize(wxSize(640, frameHeight));
    
	//GdaFrame::GetGdaFrame()->Show(true);
	SetTopWindow(GdaFrame::GetGdaFrame());
	
	if (GeneralWxUtils::isWindows()) {
		// For XP / Vista / Win 7, the user can select to use font sizes
		// of %100, %125 or %150.
		// Therefore, we might need to slighly increase the window size
		// when sizes > %100 are used in the Display options.
		
		if (GdaFrame::GetGdaFrame()->GetClientSize().GetHeight() < 22) {
			GdaFrame::GetGdaFrame()->SetSize(
				GdaFrame::GetGdaFrame()->GetSize().GetWidth(),
				GdaFrame::GetGdaFrame()->GetSize().GetHeight() +
				(22 - GdaFrame::GetGdaFrame()->GetClientSize().GetHeight()));
		}
	}

	if (!cmd_line_proj_file_name.IsEmpty()) {
		wxString proj_fname(cmd_line_proj_file_name);
		LOG_MSG("Potential project file: " + proj_fname);
		GdaFrame::GetGdaFrame()->OpenProject(proj_fname);
	}
	
	LOG_MSG("Exiting GdaApp::OnInit");
	return true;
}

int GdaApp::OnExit(void)
{
	LOG_MSG("In GdaApp::OnExit");
	if (checker) delete checker;
	return 0;
}

void GdaApp::OnFatalException()
{
	LOG_MSG("In GdaApp::OnFatalException");
	wxMessageBox("Fatal Excption.  Program will likely close now.");
}


const wxCmdLineEntryDesc GdaApp::globalCmdLineDesc [] =
{
	{ wxCMD_LINE_SWITCH, "h", "help",
		"displays help on the command line parameters",
		wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_PARAM, NULL, NULL, "project file",
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_NONE }
};

void GdaApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	LOG_MSG("In GdaApp::OnInitCmdLine");
	parser.SetDesc (GdaApp::globalCmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (wxT("-"));
}

bool GdaApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	LOG_MSG("In GdaApp::OnCmdLineParsed");
	
	for (int i=0; i<parser.GetParamCount(); ++i) {
		LOG_MSG(parser.GetParam(i));
	}
	
	// parser.GetParamCount();
	if ( parser.GetParamCount() > 0) {
		cmd_line_proj_file_name = parser.GetParam(0);
	}
	LOG_MSG(cmd_line_proj_file_name);
	
	return true;
}

void GdaApp::MacOpenFiles(const wxArrayString& fileNames)
{
	LOG_MSG("In GdaApp::MacOpenFiles");
	wxString msg;
	int sz=fileNames.GetCount();
	msg << "Request to open " << sz << " file(s):";
	for (int i=0; i<sz; ++i)
        msg << "\n" << fileNames[i];
	LOG_MSG(msg);
	if (sz > 0) GdaFrame::GetGdaFrame()->OpenProject(fileNames[0]);
}

/*
 * This is the top-level window of the application.
 */

BEGIN_EVENT_TABLE(GdaFrame, wxFrame)

EVT_CHAR_HOOK(GdaFrame::OnKeyEvent)


EVT_MENU(XRCID("ID_NEW_PROJ_FROM_SHP"), GdaFrame::OnNewProjectFromShp)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_SQLITE"), GdaFrame::OnNewProjectFromSqlite)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_CSV"), GdaFrame::OnNewProjectFromCsv)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_DBF"), GdaFrame::OnNewProjectFromDbf)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_GDB"), GdaFrame::OnNewProjectFromGdb)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_JSON"), GdaFrame::OnNewProjectFromJson)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_GML"), GdaFrame::OnNewProjectFromGml)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_KML"), GdaFrame::OnNewProjectFromKml)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_MAPINFO"), GdaFrame::OnNewProjectFromMapinfo)
EVT_MENU(XRCID("ID_NEW_PROJ_FROM_XLS"), GdaFrame::OnNewProjectFromXls)

EVT_MENU(XRCID("ID_NEW_PROJECT"), GdaFrame::OnNewProject)
EVT_TOOL(XRCID("ID_NEW_PROJECT"), GdaFrame::OnNewProject)
EVT_MENU(XRCID("ID_OPEN_PROJECT"), GdaFrame::OnOpenProject)
EVT_TOOL(XRCID("ID_OPEN_PROJECT"), GdaFrame::OnOpenProject)
EVT_MENU(XRCID("ID_SAVE_PROJECT"), GdaFrame::OnSaveProject)
EVT_TOOL(XRCID("ID_SAVE_PROJECT"), GdaFrame::OnSaveProject)
EVT_MENU(XRCID("ID_SAVE_AS_PROJECT"), GdaFrame::OnSaveAsProject)
EVT_TOOL(XRCID("ID_SAVE_AS_PROJECT"), GdaFrame::OnSaveAsProject)
EVT_MENU(XRCID("ID_EXPORT_LAYER"), GdaFrame::OnExportToOGR)
//EVT_TOOL(XRCID("ID_EXPORT_LAYER"), GdaFrame::OnExportToOGR)
EVT_MENU(XRCID("ID_EXPORT_SELECTED"), GdaFrame::OnExportSelectedToOGR)

EVT_MENU(XRCID("ID_SHOW_PROJECT_INFO"), GdaFrame::OnShowProjectInfo)

EVT_MENU(XRCID("wxID_CLOSE"), GdaFrame::OnMenuClose)
EVT_MENU(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
EVT_TOOL(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
EVT_BUTTON(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
EVT_CLOSE(GdaFrame::OnClose)
EVT_MENU(XRCID("wxID_EXIT"), GdaFrame::OnQuit)

EVT_MENU(XRCID("ID_SELECT_WITH_RECT"), GdaFrame::OnSelectWithRect)
EVT_MENU(XRCID("ID_SELECT_WITH_CIRCLE"), GdaFrame::OnSelectWithCircle)
EVT_MENU(XRCID("ID_SELECT_WITH_LINE"), GdaFrame::OnSelectWithLine)
EVT_MENU(XRCID("ID_SELECTION_MODE"), GdaFrame::OnSelectionMode)
EVT_MENU(XRCID("ID_FIT_TO_WINDOW_MODE"), GdaFrame::OnFitToWindowMode)
// Fit-To-Window Mode
EVT_MENU(XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
		 GdaFrame::OnFixedAspectRatioMode)
EVT_MENU(XRCID("ID_ZOOM_MODE"), GdaFrame::OnZoomMode)
EVT_MENU(XRCID("ID_PAN_MODE"), GdaFrame::OnPanMode)
// Print Canvas State to Log File.  Used for debugging.
EVT_MENU(XRCID("ID_PRINT_CANVAS_STATE"), GdaFrame::OnPrintCanvasState)

EVT_MENU(XRCID("ID_CLEAN_BASEMAP"), GdaFrame::OnCleanBasemap)
EVT_MENU(XRCID("ID_NO_BASEMAP"), GdaFrame::OnSetNoBasemap)
EVT_MENU(XRCID("ID_CHANGE_TRANSPARENCY"), GdaFrame::OnChangeMapTransparency)
EVT_MENU(XRCID("ID_BASEMAP_1"), GdaFrame::OnSetBasemap1)
EVT_MENU(XRCID("ID_BASEMAP_2"), GdaFrame::OnSetBasemap2)
EVT_MENU(XRCID("ID_BASEMAP_3"), GdaFrame::OnSetBasemap3)
EVT_MENU(XRCID("ID_BASEMAP_4"), GdaFrame::OnSetBasemap4)
EVT_MENU(XRCID("ID_BASEMAP_5"), GdaFrame::OnSetBasemap5)
EVT_MENU(XRCID("ID_BASEMAP_6"), GdaFrame::OnSetBasemap6)
EVT_MENU(XRCID("ID_BASEMAP_7"), GdaFrame::OnSetBasemap7)
EVT_MENU(XRCID("ID_BASEMAP_8"), GdaFrame::OnSetBasemap8)
EVT_MENU(XRCID("ID_BASEMAP_CONF"), GdaFrame::OnBasemapConfig)



EVT_MENU(XRCID("ID_SAVE_CANVAS_IMAGE_AS"), GdaFrame::OnSaveCanvasImageAs)
EVT_MENU(XRCID("ID_SAVE_SELECTED_TO_COLUMN"),
		 GdaFrame::OnSaveSelectedToColumn)
EVT_MENU(XRCID("ID_CANVAS_BACKGROUND_COLOR"),
		 GdaFrame::OnCanvasBackgroundColor)
EVT_MENU(XRCID("ID_LEGEND_USE_SCI_NOTATION"),
		 GdaFrame::OnLegendUseScientificNotation)
EVT_MENU(XRCID("ID_LEGEND_BACKGROUND_COLOR"),
		 GdaFrame::OnLegendBackgroundColor)
EVT_MENU(XRCID("ID_SELECTABLE_FILL_COLOR"),
		 GdaFrame::OnSelectableFillColor)
EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_COLOR"),
		 GdaFrame::OnSelectableOutlineColor)
EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
		 GdaFrame::OnSelectableOutlineVisible)
EVT_MENU(XRCID("ID_HIGHLIGHT_COLOR"), GdaFrame::OnHighlightColor)

EVT_MENU(XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"),
		 GdaFrame::OnCopyImageToClipboard)
EVT_MENU(XRCID("ID_COPY_LEGEND_TO_CLIPBOARD"),
		 GdaFrame::OnCopyLegendToClipboard)

EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)
EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)
EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)
EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)
EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)
EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)

EVT_MENU(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
		 GdaFrame::OnConnectivityHistView)
EVT_TOOL(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
		 GdaFrame::OnConnectivityHistView)
EVT_BUTTON(XRCID("ID_CONNECTIVITY_HIST_VIEW"),
		   GdaFrame::OnConnectivityHistView)

EVT_MENU(XRCID("ID_CONNECTIVITY_MAP_VIEW"),
		 GdaFrame::OnConnectivityMapView)
EVT_TOOL(XRCID("ID_CONNECTIVITY_MAP_VIEW"),
		 GdaFrame::OnConnectivityMapView)
EVT_BUTTON(XRCID("ID_CONNECTIVITY_MAP_VIEW"),
		   GdaFrame::OnConnectivityMapView)

EVT_MENU(XRCID("ID_SHOW_AXES"), GdaFrame::OnShowAxes)

EVT_TOOL(XRCID("ID_MAP_CHOICES"), GdaFrame::OnMapChoices)

EVT_MENU(XRCID("ID_SHAPE_POINTS_FROM_ASCII"),
		 GdaFrame::OnShapePointsFromASCII)
EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_GRID"),
		 GdaFrame::OnShapePolygonsFromGrid)
EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_BOUNDARY"),
		 GdaFrame::OnShapePolygonsFromBoundary)
EVT_MENU(XRCID("ID_SHAPE_TO_BOUNDARY"), GdaFrame::OnShapeToBoundary)
EVT_MENU(XRCID("ID_POINTS_FROM_TABLE"), GdaFrame::OnGeneratePointShpFile)

// Table menu items
EVT_MENU(XRCID("ID_SHOW_TIME_CHOOSER"), GdaFrame::OnShowTimeChooser)

EVT_MENU(XRCID("ID_SHOW_DATA_MOVIE"), GdaFrame::OnShowDataMovie)
EVT_TOOL(XRCID("ID_SHOW_DATA_MOVIE"), GdaFrame::OnShowDataMovie)
EVT_BUTTON(XRCID("ID_SHOW_DATA_MOVIE"), GdaFrame::OnShowDataMovie)

EVT_MENU(XRCID("ID_SHOW_CAT_CLASSIF"), GdaFrame::OnShowCatClassif)
EVT_TOOL(XRCID("ID_SHOW_CAT_CLASSIF"), GdaFrame::OnShowCatClassif)
EVT_BUTTON(XRCID("ID_SHOW_CAT_CLASSIF"), GdaFrame::OnShowCatClassif)

EVT_MENU(XRCID("ID_VAR_GROUPING_EDITOR"), GdaFrame::OnVarGroupingEditor)
EVT_MENU(XRCID("ID_TIME_EDITOR"), GdaFrame::OnVarGroupingEditor)
EVT_MENU(XRCID("ID_TABLE_MOVE_SELECTED_TO_TOP"), GdaFrame::OnMoveSelectedToTop)
EVT_MENU(XRCID("ID_TABLE_INVERT_SELECTION"), GdaFrame::OnInvertSelection)
EVT_MENU(XRCID("ID_TABLE_CLEAR_SELECTION"), GdaFrame::OnClearSelection)
EVT_MENU(XRCID("ID_TABLE_RANGE_SELECTION"), GdaFrame::OnRangeSelection)
EVT_MENU(XRCID("ID_TABLE_FIELD_CALCULATION"), GdaFrame::OnFieldCalculation)
EVT_MENU(XRCID("ID_CALCULATOR"), GdaFrame::OnCalculator)
EVT_MENU(XRCID("ID_TABLE_ADD_COLUMN"), GdaFrame::OnAddCol)
EVT_MENU(XRCID("ID_TABLE_DELETE_COLUMN"), GdaFrame::OnDeleteCol)
EVT_MENU(XRCID("ID_TABLE_EDIT_FIELD_PROP"),
		 GdaFrame::OnEditFieldProperties)
EVT_MENU(XRCID("ID_TABLE_CHANGE_FIELD_TYPE"), GdaFrame::OnChangeFieldType)
EVT_MENU(XRCID("ID_TABLE_MERGE_TABLE_DATA"),
		 GdaFrame::OnMergeTableData)
EVT_MENU(XRCID("ID_EXPORT_TO_CSV_FILE"),  // not used currently
		 GdaFrame::OnExportToCsvFile)     // not used currently

EVT_MENU(XRCID("ID_REGRESSION_CLASSIC"), GdaFrame::OnRegressionClassic)
EVT_TOOL(XRCID("ID_REGRESSION_CLASSIC"), GdaFrame::OnRegressionClassic)
EVT_TOOL(XRCID("ID_PUBLISH"), GdaFrame::OnPublish)

EVT_TOOL(XRCID("ID_COND_PLOT_CHOICES"), GdaFrame::OnCondPlotChoices)
// The following duplicate entries are needed as a workaround to
// make menu enable/disable work for the menu bar when the same menu
// item appears twice.
EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW_MAP_MENU"),
		 GdaFrame::OnShowConditionalMapView)	
EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
		 GdaFrame::OnShowConditionalMapView)
EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"),
		   GdaFrame::OnShowConditionalMapView)
EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
		 GdaFrame::OnShowConditionalScatterView)
EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"),
		   GdaFrame::OnShowConditionalScatterView)
EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
		 GdaFrame::OnShowConditionalHistView)
EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"),
		   GdaFrame::OnShowConditionalHistView)

EVT_MENU(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
		 GdaFrame::OnShowCartogramNewView)
EVT_TOOL(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
		 GdaFrame::OnShowCartogramNewView)
EVT_BUTTON(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"),
		   GdaFrame::OnShowCartogramNewView)

EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_1"), GdaFrame::OnCartogramImprove1)
EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_2"), GdaFrame::OnCartogramImprove2)
EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_3"), GdaFrame::OnCartogramImprove3)
EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_4"), GdaFrame::OnCartogramImprove4)
EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_5"), GdaFrame::OnCartogramImprove5)
EVT_MENU(XRCID("ID_CARTOGRAM_IMPROVE_6"), GdaFrame::OnCartogramImprove6)

EVT_MENU(XRCID("ID_OPTIONS_HINGE_15"), GdaFrame::OnHinge15)
EVT_MENU(XRCID("ID_OPTIONS_HINGE_30"), GdaFrame::OnHinge30)

EVT_MENU(XRCID("IDM_HIST"), GdaFrame::OnExploreHist)
EVT_TOOL(XRCID("IDM_HIST"), GdaFrame::OnExploreHist)
EVT_BUTTON(XRCID("IDM_HIST"), GdaFrame::OnExploreHist)
EVT_MENU(XRCID("IDM_SCATTERPLOT"), GdaFrame::OnExploreScatterNewPlot)
EVT_MENU(XRCID("IDM_BUBBLECHART"), GdaFrame::OnExploreBubbleChart)
EVT_MENU(XRCID("IDM_SCATTERPLOT_MAT"), GdaFrame::OnExploreScatterPlotMat)
EVT_MENU(XRCID("IDM_CORRELOGRAM"), GdaFrame::OnExploreCorrelogram)
EVT_MENU(XRCID("IDM_COV_SCATTERPLOT"), GdaFrame::OnExploreCovScatterPlot)
EVT_TOOL(XRCID("IDM_SCATTERPLOT"), GdaFrame::OnExploreScatterNewPlot)
EVT_TOOL(XRCID("IDM_BUBBLECHART"), GdaFrame::OnExploreBubbleChart)
EVT_TOOL(XRCID("IDM_SCATTERPLOT_MAT"), GdaFrame::OnExploreScatterPlotMat)
EVT_TOOL(XRCID("IDM_CORRELOGRAM"), GdaFrame::OnExploreCorrelogram)
EVT_TOOL(XRCID("IDM_COV_SCATTERPLOT"), GdaFrame::OnExploreCovScatterPlot)
EVT_BUTTON(XRCID("IDM_SCATTERPLOT"), GdaFrame::OnExploreScatterNewPlot)
EVT_BUTTON(XRCID("IDM_CORRELOGRAM"), GdaFrame::OnExploreCorrelogram)
EVT_BUTTON(XRCID("IDM_BUBBLECHART"), GdaFrame::OnExploreBubbleChart)
EVT_BUTTON(XRCID("IDM_SCATTERPLOT_MAT"), GdaFrame::OnExploreScatterPlotMat)
EVT_BUTTON(XRCID("IDM_COV_SCATTERPLOT"), GdaFrame::OnExploreCovScatterPlot)
EVT_MENU(ID_TEST_MAP_FRAME, GdaFrame::OnExploreTestMap)
EVT_MENU(XRCID("IDM_BOX"), GdaFrame::OnExploreNewBox)
EVT_TOOL(XRCID("IDM_BOX"), GdaFrame::OnExploreNewBox)
EVT_BUTTON(XRCID("IDM_BOX"), GdaFrame::OnExploreNewBox)
EVT_MENU(XRCID("IDM_PCP"), GdaFrame::OnExplorePCP)
EVT_TOOL(XRCID("IDM_PCP"), GdaFrame::OnExplorePCP)
EVT_BUTTON(XRCID("IDM_PCP"), GdaFrame::OnExplorePCP)
EVT_MENU(XRCID("IDM_3DP"), GdaFrame::OnExplore3DP)
EVT_TOOL(XRCID("IDM_3DP"), GdaFrame::OnExplore3DP)
EVT_BUTTON(XRCID("IDM_3DP"), GdaFrame::OnExplore3DP)
EVT_MENU(XRCID("IDM_LINE_CHART"), GdaFrame::OnExploreLineChart)
EVT_TOOL(XRCID("IDM_LINE_CHART"), GdaFrame::OnExploreLineChart)
EVT_BUTTON(XRCID("IDM_LINE_CHART"), GdaFrame::OnExploreLineChart)

EVT_TOOL(XRCID("IDM_NEW_TABLE"), GdaFrame::OnToolOpenNewTable)
EVT_BUTTON(XRCID("IDM_NEW_TABLE"), GdaFrame::OnToolOpenNewTable)

EVT_TOOL(XRCID("ID_MORAN_MENU"), GdaFrame::OnMoranMenuChoices)
EVT_MENU(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
EVT_TOOL(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
EVT_BUTTON(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
EVT_MENU(XRCID("IDM_GMORAN"), GdaFrame::OnOpenDiffMoran)
EVT_TOOL(XRCID("IDM_GMORAN"), GdaFrame::OnOpenDiffMoran)
EVT_BUTTON(XRCID("IDM_GMORAN"), GdaFrame::OnOpenDiffMoran)
EVT_MENU(XRCID("IDM_MORAN_EBRATE"), GdaFrame::OnOpenMoranEB)
EVT_TOOL(XRCID("IDM_MORAN_EBRATE"), GdaFrame::OnOpenMoranEB)
EVT_BUTTON(XRCID("IDM_MORAN_EBRATE"), GdaFrame::OnOpenMoranEB)
EVT_TOOL(XRCID("ID_LISA_MENU"), GdaFrame::OnLisaMenuChoices)
EVT_MENU(XRCID("IDM_UNI_LISA"), GdaFrame::OnOpenUniLisa)
EVT_TOOL(XRCID("IDM_UNI_LISA"), GdaFrame::OnOpenUniLisa)
EVT_BUTTON(XRCID("IDM_UNI_LISA"), GdaFrame::OnOpenUniLisa)
EVT_MENU(XRCID("IDM_MULTI_LISA"), GdaFrame::OnOpenMultiLisa)
EVT_TOOL(XRCID("IDM_MULTI_LISA"), GdaFrame::OnOpenMultiLisa)
EVT_BUTTON(XRCID("IDM_MULTI_LISA"), GdaFrame::OnOpenMultiLisa)
EVT_MENU(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)
EVT_TOOL(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)
EVT_BUTTON(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)

EVT_TOOL(XRCID("IDM_GETIS_ORD_MENU"), GdaFrame::OnGetisMenuChoices)
EVT_BUTTON(XRCID("IDM_GETIS_ORD_MENU"), GdaFrame::OnGetisMenuChoices)

EVT_MENU(XRCID("IDM_LOCAL_G"), GdaFrame::OnOpenGetisOrd)
EVT_MENU(XRCID("IDM_LOCAL_G_STAR"), GdaFrame::OnOpenGetisOrdStar)

EVT_MENU(XRCID("ID_HISTOGRAM_INTERVALS"), GdaFrame::OnHistogramIntervals)
EVT_MENU(XRCID("ID_SAVE_CONNECTIVITY_TO_TABLE"),
		 GdaFrame::OnSaveConnectivityToTable)
EVT_MENU(XRCID("ID_SELECT_ISOLATES"), GdaFrame::OnSelectIsolates)

EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_99PERMUTATION"),
		 GdaFrame::OnRan99Per)
EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_199PERMUTATION"),
		 GdaFrame::OnRan199Per)
EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_499PERMUTATION"),
		 GdaFrame::OnRan499Per)
EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_999PERMUTATION"),
		 GdaFrame::OnRan999Per)
EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_OTHER"),
		 GdaFrame::OnRanOtherPer)

EVT_MENU(XRCID("ID_USE_SPECIFIED_SEED"), GdaFrame::OnUseSpecifiedSeed)
EVT_MENU(XRCID("ID_SPECIFY_SEED_DLG"), GdaFrame::OnSpecifySeedDlg)

EVT_MENU(XRCID("ID_SAVE_MORANI"), GdaFrame::OnSaveMoranI)

EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_05"), GdaFrame::OnSigFilter05)
EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_01"), GdaFrame::OnSigFilter01)
EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_001"), GdaFrame::OnSigFilter001)
EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_0001"), GdaFrame::OnSigFilter0001)

EVT_MENU(XRCID("ID_SAVE_GETIS_ORD"), GdaFrame::OnSaveGetisOrd)
EVT_MENU(XRCID("ID_SAVE_LISA"), GdaFrame::OnSaveLisa)
EVT_MENU(XRCID("ID_SELECT_CORES"), GdaFrame::OnSelectCores)
EVT_MENU(XRCID("ID_SELECT_NEIGHBORS_OF_CORES"),
		 GdaFrame::OnSelectNeighborsOfCores)
EVT_MENU(XRCID("ID_SELECT_CORES_AND_NEIGHBORS"),
		 GdaFrame::OnSelectCoresAndNeighbors)

EVT_MENU(XRCID("ID_MAP_ADDMEANCENTERS"), GdaFrame::OnAddMeanCenters)
EVT_MENU(XRCID("ID_MAP_ADDCENTROIDS"), GdaFrame::OnAddCentroids)
EVT_MENU(XRCID("ID_DISPLAY_MEAN_CENTERS"), GdaFrame::OnDisplayMeanCenters)
EVT_MENU(XRCID("ID_DISPLAY_CENTROIDS"), GdaFrame::OnDisplayCentroids)
EVT_MENU(XRCID("ID_DISPLAY_VORONOI_DIAGRAM"),
		 GdaFrame::OnDisplayVoronoiDiagram)
EVT_MENU(XRCID("ID_EXPORT_VORONOI"),
		 GdaFrame::OnExportVoronoi)
EVT_MENU(XRCID("ID_EXPORT_MEAN_CNTRS"),
		 GdaFrame::OnExportMeanCntrs)
EVT_MENU(XRCID("ID_EXPORT_CENTROIDS"),
		 GdaFrame::OnExportCentroids)
EVT_MENU(XRCID("ID_SAVE_VORONOI_DUPS_TO_TABLE"),
		 GdaFrame::OnSaveVoronoiDupsToTable)

EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"),
		 GdaFrame::OnNewCustomCatClassifA)
EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_B"),
		 GdaFrame::OnNewCustomCatClassifB)
EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_C"),
		 GdaFrame::OnNewCustomCatClassifC)

EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_0, GdaFrame::OnHtmlEntry0)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_1, GdaFrame::OnHtmlEntry1)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_2, GdaFrame::OnHtmlEntry2)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_3, GdaFrame::OnHtmlEntry3)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_4, GdaFrame::OnHtmlEntry4)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_5, GdaFrame::OnHtmlEntry5)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_6, GdaFrame::OnHtmlEntry6)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_7, GdaFrame::OnHtmlEntry7)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_8, GdaFrame::OnHtmlEntry8)
EVT_MENU(GdaConst::ID_HTML_MENU_ENTRY_CHOICE_9, GdaFrame::OnHtmlEntry9)

EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0, GdaFrame::OnCCClassifA0)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A1, GdaFrame::OnCCClassifA1)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A2, GdaFrame::OnCCClassifA2)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A3, GdaFrame::OnCCClassifA3)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A4, GdaFrame::OnCCClassifA4)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A5, GdaFrame::OnCCClassifA5)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A6, GdaFrame::OnCCClassifA6)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A7, GdaFrame::OnCCClassifA7)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A8, GdaFrame::OnCCClassifA8)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A9, GdaFrame::OnCCClassifA9)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A10, GdaFrame::OnCCClassifA10)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A11, GdaFrame::OnCCClassifA11)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A12, GdaFrame::OnCCClassifA12)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A13, GdaFrame::OnCCClassifA13)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A14, GdaFrame::OnCCClassifA14)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A15, GdaFrame::OnCCClassifA15)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A16, GdaFrame::OnCCClassifA16)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A17, GdaFrame::OnCCClassifA17)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A18, GdaFrame::OnCCClassifA18)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A19, GdaFrame::OnCCClassifA19)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A20, GdaFrame::OnCCClassifA20)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A21, GdaFrame::OnCCClassifA21)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A22, GdaFrame::OnCCClassifA22)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A23, GdaFrame::OnCCClassifA23)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A24, GdaFrame::OnCCClassifA24)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A25, GdaFrame::OnCCClassifA25)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A26, GdaFrame::OnCCClassifA26)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A27, GdaFrame::OnCCClassifA27)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A28, GdaFrame::OnCCClassifA28)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A29, GdaFrame::OnCCClassifA29)

EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0, GdaFrame::OnCCClassifB0)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B1, GdaFrame::OnCCClassifB1)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B2, GdaFrame::OnCCClassifB2)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B3, GdaFrame::OnCCClassifB3)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B4, GdaFrame::OnCCClassifB4)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B5, GdaFrame::OnCCClassifB5)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B6, GdaFrame::OnCCClassifB6)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B7, GdaFrame::OnCCClassifB7)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B8, GdaFrame::OnCCClassifB8)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B9, GdaFrame::OnCCClassifB9)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B10, GdaFrame::OnCCClassifB10)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B11, GdaFrame::OnCCClassifB11)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B12, GdaFrame::OnCCClassifB12)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B13, GdaFrame::OnCCClassifB13)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B14, GdaFrame::OnCCClassifB14)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B15, GdaFrame::OnCCClassifB15)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B16, GdaFrame::OnCCClassifB16)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B17, GdaFrame::OnCCClassifB17)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B18, GdaFrame::OnCCClassifB18)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B19, GdaFrame::OnCCClassifB19)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B20, GdaFrame::OnCCClassifB20)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B21, GdaFrame::OnCCClassifB21)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B22, GdaFrame::OnCCClassifB22)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B23, GdaFrame::OnCCClassifB23)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B24, GdaFrame::OnCCClassifB24)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B25, GdaFrame::OnCCClassifB25)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B26, GdaFrame::OnCCClassifB26)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B27, GdaFrame::OnCCClassifB27)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B28, GdaFrame::OnCCClassifB28)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B29, GdaFrame::OnCCClassifB29)

EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0, GdaFrame::OnCCClassifC0)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C1, GdaFrame::OnCCClassifC1)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C2, GdaFrame::OnCCClassifC2)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C3, GdaFrame::OnCCClassifC3)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C4, GdaFrame::OnCCClassifC4)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C5, GdaFrame::OnCCClassifC5)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C6, GdaFrame::OnCCClassifC6)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C7, GdaFrame::OnCCClassifC7)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C8, GdaFrame::OnCCClassifC8)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C9, GdaFrame::OnCCClassifC9)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C10, GdaFrame::OnCCClassifC10)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C11, GdaFrame::OnCCClassifC11)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C12, GdaFrame::OnCCClassifC12)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C13, GdaFrame::OnCCClassifC13)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C14, GdaFrame::OnCCClassifC14)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C15, GdaFrame::OnCCClassifC15)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C16, GdaFrame::OnCCClassifC16)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C17, GdaFrame::OnCCClassifC17)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C18, GdaFrame::OnCCClassifC18)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C19, GdaFrame::OnCCClassifC19)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C20, GdaFrame::OnCCClassifC20)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C21, GdaFrame::OnCCClassifC21)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C22, GdaFrame::OnCCClassifC22)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C23, GdaFrame::OnCCClassifC23)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C24, GdaFrame::OnCCClassifC24)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C25, GdaFrame::OnCCClassifC25)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C26, GdaFrame::OnCCClassifC26)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C27, GdaFrame::OnCCClassifC27)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C28, GdaFrame::OnCCClassifC28)
EVT_MENU(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C29, GdaFrame::OnCCClassifC29)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
		 GdaFrame::OnOpenThemelessMap)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"),
		 GdaFrame::OnOpenThemelessMap)
EVT_MENU(XRCID("ID_MAPANALYSIS_THEMELESS"),
		 GdaFrame::OnThemelessMap)
EVT_MENU(XRCID("ID_COND_VERT_THEMELESS"),
		 GdaFrame::OnCondVertThemelessMap)
EVT_MENU(XRCID("ID_COND_HORIZ_THEMELESS"),
		 GdaFrame::OnCondHorizThemelessMap)

EVT_TOOL(XRCID("ID_OPEN_QUANTILE_1"), GdaFrame::OnOpenQuantile1)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_1"), GdaFrame::OnOpenQuantile1)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_2"), GdaFrame::OnOpenQuantile2)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_2"), GdaFrame::OnOpenQuantile2)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_3"), GdaFrame::OnOpenQuantile3)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_3"), GdaFrame::OnOpenQuantile3)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_4"), GdaFrame::OnOpenQuantile4)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_4"), GdaFrame::OnOpenQuantile4)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_5"), GdaFrame::OnOpenQuantile5)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_5"), GdaFrame::OnOpenQuantile5)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_6"), GdaFrame::OnOpenQuantile6)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_6"), GdaFrame::OnOpenQuantile6)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_7"), GdaFrame::OnOpenQuantile7)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_7"), GdaFrame::OnOpenQuantile7)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_8"), GdaFrame::OnOpenQuantile8)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_8"), GdaFrame::OnOpenQuantile8)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_9"), GdaFrame::OnOpenQuantile9)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_9"), GdaFrame::OnOpenQuantile9)
EVT_TOOL(XRCID("ID_OPEN_QUANTILE_10"), GdaFrame::OnOpenQuantile10)
EVT_MENU(XRCID("ID_OPEN_QUANTILE_10"), GdaFrame::OnOpenQuantile10)

EVT_MENU(XRCID("ID_QUANTILE_1"), GdaFrame::OnQuantile1)
EVT_MENU(XRCID("ID_QUANTILE_2"), GdaFrame::OnQuantile2)
EVT_MENU(XRCID("ID_QUANTILE_3"), GdaFrame::OnQuantile3)
EVT_MENU(XRCID("ID_QUANTILE_4"), GdaFrame::OnQuantile4)
EVT_MENU(XRCID("ID_QUANTILE_5"), GdaFrame::OnQuantile5)
EVT_MENU(XRCID("ID_QUANTILE_6"), GdaFrame::OnQuantile6)
EVT_MENU(XRCID("ID_QUANTILE_7"), GdaFrame::OnQuantile7)
EVT_MENU(XRCID("ID_QUANTILE_8"), GdaFrame::OnQuantile8)
EVT_MENU(XRCID("ID_QUANTILE_9"), GdaFrame::OnQuantile9)
EVT_MENU(XRCID("ID_QUANTILE_10"), GdaFrame::OnQuantile10)

EVT_MENU(XRCID("ID_COND_VERT_QUANT_1"), GdaFrame::OnCondVertQuant1)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_2"), GdaFrame::OnCondVertQuant2)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_3"), GdaFrame::OnCondVertQuant3)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_4"), GdaFrame::OnCondVertQuant4)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_5"), GdaFrame::OnCondVertQuant5)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_6"), GdaFrame::OnCondVertQuant6)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_7"), GdaFrame::OnCondVertQuant7)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_8"), GdaFrame::OnCondVertQuant8)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_9"), GdaFrame::OnCondVertQuant9)
EVT_MENU(XRCID("ID_COND_VERT_QUANT_10"), GdaFrame::OnCondVertQuant10)

EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_1"), GdaFrame::OnCondHorizQuant1)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_2"), GdaFrame::OnCondHorizQuant2)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_3"), GdaFrame::OnCondHorizQuant3)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_4"), GdaFrame::OnCondHorizQuant4)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_5"), GdaFrame::OnCondHorizQuant5)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_6"), GdaFrame::OnCondHorizQuant6)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_7"), GdaFrame::OnCondHorizQuant7)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_8"), GdaFrame::OnCondHorizQuant8)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_9"), GdaFrame::OnCondHorizQuant9)
EVT_MENU(XRCID("ID_COND_HORIZ_QUANT_10"), GdaFrame::OnCondHorizQuant10)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
		 GdaFrame::OnOpenPercentile)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
		 GdaFrame::OnOpenPercentile)
EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"),
		 GdaFrame::OnPercentile)
EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_PERCENTILE"),
		 GdaFrame::OnCondVertPercentile)
EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_PERCENTILE"),
		 GdaFrame::OnCondHorizPercentile)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), GdaFrame::OnOpenHinge15)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_15"), GdaFrame::OnOpenHinge15)
EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_15"), GdaFrame::OnHinge15)
EVT_MENU(XRCID("ID_COND_VERT_HINGE_15"), GdaFrame::OnCondVertHinge15)
EVT_MENU(XRCID("ID_COND_HORIZ_HINGE_15"), GdaFrame::OnCondHorizHinge15)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), GdaFrame::OnOpenHinge30)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_HINGE_30"), GdaFrame::OnOpenHinge30)
EVT_MENU(XRCID("ID_MAPANALYSIS_HINGE_30"), GdaFrame::OnHinge30)
EVT_MENU(XRCID("ID_COND_VERT_HINGE_30"), GdaFrame::OnCondVertHinge30)
EVT_MENU(XRCID("ID_COND_HORIZ_HINGE_30"), GdaFrame::OnCondHorizHinge30)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
		 GdaFrame::OnOpenStddev)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"),
		 GdaFrame::OnOpenStddev)
EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"),
		 GdaFrame::OnStddev)
EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_STDDEV"),
		 GdaFrame::OnCondVertStddev)
EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_STDDEV"),
		 GdaFrame::OnCondHorizStddev)

EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
		 GdaFrame::OnOpenUniqueValues)
EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"),
		 GdaFrame::OnOpenUniqueValues)
EVT_MENU(XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"),
		 GdaFrame::OnUniqueValues)
EVT_MENU(XRCID("ID_COND_VERT_UNIQUE_VALUES"),
		 GdaFrame::OnCondVertUniqueValues)
EVT_MENU(XRCID("ID_COND_HORIZ_UNIQUE_VALUES"),
		 GdaFrame::OnCondHorizUniqueValues)

EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_1"), GdaFrame::OnOpenNaturalBreaks1)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_1"), GdaFrame::OnOpenNaturalBreaks1)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_2"), GdaFrame::OnOpenNaturalBreaks2)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_2"), GdaFrame::OnOpenNaturalBreaks2)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_3"), GdaFrame::OnOpenNaturalBreaks3)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_3"), GdaFrame::OnOpenNaturalBreaks3)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_4"), GdaFrame::OnOpenNaturalBreaks4)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_4"), GdaFrame::OnOpenNaturalBreaks4)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_5"), GdaFrame::OnOpenNaturalBreaks5)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_5"), GdaFrame::OnOpenNaturalBreaks5)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_6"), GdaFrame::OnOpenNaturalBreaks6)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_6"), GdaFrame::OnOpenNaturalBreaks6)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_7"), GdaFrame::OnOpenNaturalBreaks7)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_7"), GdaFrame::OnOpenNaturalBreaks7)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_8"), GdaFrame::OnOpenNaturalBreaks8)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_8"), GdaFrame::OnOpenNaturalBreaks8)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_9"), GdaFrame::OnOpenNaturalBreaks9)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_9"), GdaFrame::OnOpenNaturalBreaks9)
EVT_TOOL(XRCID("ID_OPEN_NATURAL_BREAKS_10"), GdaFrame::OnOpenNaturalBreaks10)
EVT_MENU(XRCID("ID_OPEN_NATURAL_BREAKS_10"), GdaFrame::OnOpenNaturalBreaks10)

EVT_MENU(XRCID("ID_NATURAL_BREAKS_1"), GdaFrame::OnNaturalBreaks1)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_2"), GdaFrame::OnNaturalBreaks2)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_3"), GdaFrame::OnNaturalBreaks3)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_4"), GdaFrame::OnNaturalBreaks4)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_5"), GdaFrame::OnNaturalBreaks5)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_6"), GdaFrame::OnNaturalBreaks6)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_7"), GdaFrame::OnNaturalBreaks7)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_8"), GdaFrame::OnNaturalBreaks8)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_9"), GdaFrame::OnNaturalBreaks9)
EVT_MENU(XRCID("ID_NATURAL_BREAKS_10"), GdaFrame::OnNaturalBreaks10)

EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_1"), GdaFrame::OnCondVertNatBrks1)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_2"), GdaFrame::OnCondVertNatBrks2)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_3"), GdaFrame::OnCondVertNatBrks3)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_4"), GdaFrame::OnCondVertNatBrks4)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_5"), GdaFrame::OnCondVertNatBrks5)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_6"), GdaFrame::OnCondVertNatBrks6)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_7"), GdaFrame::OnCondVertNatBrks7)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_8"), GdaFrame::OnCondVertNatBrks8)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_9"), GdaFrame::OnCondVertNatBrks9)
EVT_MENU(XRCID("ID_COND_VERT_NAT_BRKS_10"), GdaFrame::OnCondVertNatBrks10)

EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_1"), GdaFrame::OnCondHorizNatBrks1)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_2"), GdaFrame::OnCondHorizNatBrks2)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_3"), GdaFrame::OnCondHorizNatBrks3)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_4"), GdaFrame::OnCondHorizNatBrks4)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_5"), GdaFrame::OnCondHorizNatBrks5)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_6"), GdaFrame::OnCondHorizNatBrks6)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_7"), GdaFrame::OnCondHorizNatBrks7)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_8"), GdaFrame::OnCondHorizNatBrks8)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_9"), GdaFrame::OnCondHorizNatBrks9)
EVT_MENU(XRCID("ID_COND_HORIZ_NAT_BRKS_10"), GdaFrame::OnCondHorizNatBrks10)

EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_1"), GdaFrame::OnOpenEqualIntervals1)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_1"), GdaFrame::OnOpenEqualIntervals1)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_2"), GdaFrame::OnOpenEqualIntervals2)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_2"), GdaFrame::OnOpenEqualIntervals2)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_3"), GdaFrame::OnOpenEqualIntervals3)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_3"), GdaFrame::OnOpenEqualIntervals3)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_4"), GdaFrame::OnOpenEqualIntervals4)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_4"), GdaFrame::OnOpenEqualIntervals4)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_5"), GdaFrame::OnOpenEqualIntervals5)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_5"), GdaFrame::OnOpenEqualIntervals5)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_6"), GdaFrame::OnOpenEqualIntervals6)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_6"), GdaFrame::OnOpenEqualIntervals6)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_7"), GdaFrame::OnOpenEqualIntervals7)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_7"), GdaFrame::OnOpenEqualIntervals7)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_8"), GdaFrame::OnOpenEqualIntervals8)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_8"), GdaFrame::OnOpenEqualIntervals8)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_9"), GdaFrame::OnOpenEqualIntervals9)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_9"), GdaFrame::OnOpenEqualIntervals9)
EVT_TOOL(XRCID("ID_OPEN_EQUAL_INTERVALS_10"), GdaFrame::OnOpenEqualIntervals10)
EVT_MENU(XRCID("ID_OPEN_EQUAL_INTERVALS_10"), GdaFrame::OnOpenEqualIntervals10)

EVT_MENU(XRCID("ID_EQUAL_INTERVALS_1"), GdaFrame::OnEqualIntervals1)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_2"), GdaFrame::OnEqualIntervals2)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_3"), GdaFrame::OnEqualIntervals3)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_4"), GdaFrame::OnEqualIntervals4)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_5"), GdaFrame::OnEqualIntervals5)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_6"), GdaFrame::OnEqualIntervals6)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_7"), GdaFrame::OnEqualIntervals7)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_8"), GdaFrame::OnEqualIntervals8)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_9"), GdaFrame::OnEqualIntervals9)
EVT_MENU(XRCID("ID_EQUAL_INTERVALS_10"), GdaFrame::OnEqualIntervals10)

EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_1"), GdaFrame::OnCondVertEquInts1)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_2"), GdaFrame::OnCondVertEquInts2)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_3"), GdaFrame::OnCondVertEquInts3)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_4"), GdaFrame::OnCondVertEquInts4)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_5"), GdaFrame::OnCondVertEquInts5)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_6"), GdaFrame::OnCondVertEquInts6)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_7"), GdaFrame::OnCondVertEquInts7)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_8"), GdaFrame::OnCondVertEquInts8)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_9"), GdaFrame::OnCondVertEquInts9)
EVT_MENU(XRCID("ID_COND_VERT_EQU_INTS_10"), GdaFrame::OnCondVertEquInts10)

EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_1"), GdaFrame::OnCondHorizEquInts1)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_2"), GdaFrame::OnCondHorizEquInts2)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_3"), GdaFrame::OnCondHorizEquInts3)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_4"), GdaFrame::OnCondHorizEquInts4)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_5"), GdaFrame::OnCondHorizEquInts5)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_6"), GdaFrame::OnCondHorizEquInts6)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_7"), GdaFrame::OnCondHorizEquInts7)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_8"), GdaFrame::OnCondHorizEquInts8)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_9"), GdaFrame::OnCondHorizEquInts9)
EVT_MENU(XRCID("ID_COND_HORIZ_EQU_INTS_10"), GdaFrame::OnCondHorizEquInts10)

EVT_MENU(XRCID("ID_SAVE_CATEGORIES"), GdaFrame::OnSaveCategories)
EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), GdaFrame::OnOpenRawrate)
EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_RAWRATE"), GdaFrame::OnOpenRawrate)
EVT_MENU(XRCID("ID_RATES_SMOOTH_RAWRATE"), GdaFrame::OnRawrate)
EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
		 GdaFrame::OnOpenExcessrisk)
EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"),
		 GdaFrame::OnOpenExcessrisk)
EVT_MENU(XRCID("ID_RATES_SMOOTH_EXCESSRISK"),
		 GdaFrame::OnExcessrisk)
EVT_TOOL(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
		 GdaFrame::OnOpenEmpiricalBayes)
EVT_MENU(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"),
		 GdaFrame::OnOpenEmpiricalBayes)
EVT_MENU(XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"),
		 GdaFrame::OnEmpiricalBayes)
EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
		 GdaFrame::OnOpenSpatialRate)
EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"),
		 GdaFrame::OnOpenSpatialRate)
EVT_MENU(XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"),
		 GdaFrame::OnSpatialRate)
EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
		 GdaFrame::OnOpenSpatialEmpiricalBayes)
EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"),
		 GdaFrame::OnOpenSpatialEmpiricalBayes)
EVT_MENU(XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"),
		 GdaFrame::OnSpatialEmpiricalBayes)
EVT_MENU(XRCID("ID_MAPANALYSIS_SAVERESULTS"), GdaFrame::OnSaveResults)
EVT_MENU(XRCID("ID_VIEW_STANDARDIZED_DATA"),
		 GdaFrame::OnViewStandardizedData)
EVT_MENU(XRCID("ID_VIEW_ORIGINAL_DATA"), GdaFrame::OnViewOriginalData)
EVT_MENU(XRCID("ID_VIEW_LINEAR_SMOOTHER"), GdaFrame::OnViewLinearSmoother)
EVT_MENU(XRCID("ID_VIEW_LOWESS_SMOOTHER"), GdaFrame::OnViewLowessSmoother)
EVT_MENU(XRCID("ID_EDIT_LOWESS_PARAMS"), GdaFrame::OnEditLowessParams)
EVT_MENU(XRCID("ID_EDIT_VARIABLES"), GdaFrame::OnEditVariables)
EVT_MENU(XRCID("ID_VIEW_REGIMES_REGRESSION"),
		 GdaFrame::OnViewRegimesRegression)
EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED"),
		 GdaFrame::OnViewRegressionSelected)
EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"),
		 GdaFrame::OnViewRegressionSelectedExcluded)
EVT_MENU(XRCID("ID_COMPARE_REGIMES"), GdaFrame::OnCompareRegimes)
EVT_MENU(XRCID("ID_COMPARE_TIME_PERIODS"), GdaFrame::OnCompareTimePeriods)
EVT_MENU(XRCID("ID_COMPARE_REG_AND_TM_PER"), GdaFrame::OnCompareRegAndTmPer)
EVT_MENU(XRCID("ID_DISPLAY_STATISTICS"), GdaFrame::OnDisplayStatistics)
EVT_MENU(XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"),
		 GdaFrame::OnShowAxesThroughOrigin)
EVT_MENU(XRCID("ID_DISPLAY_AXES_SCALE_VALUES"),
		 GdaFrame::OnDisplayAxesScaleValues)
EVT_MENU(XRCID("ID_DISPLAY_SLOPE_VALUES"),GdaFrame::OnDisplaySlopeValues)

EVT_MENU(XRCID("ID_TABLE_SET_LOCALE"), GdaFrame::OnTableSetLocale)
EVT_MENU(XRCID("ID_ENCODING_UTF8"), GdaFrame::OnEncodingUTF8)
EVT_MENU(XRCID("ID_ENCODING_UTF16"), GdaFrame::OnEncodingUTF16)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1250"), GdaFrame::OnEncodingWindows1250)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1251"), GdaFrame::OnEncodingWindows1251)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1254"),GdaFrame::OnEncodingWindows1254)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1255"),GdaFrame::OnEncodingWindows1255)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1256"),GdaFrame::OnEncodingWindows1256)
EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1258"),GdaFrame::OnEncodingWindows1258)
EVT_MENU(XRCID("ID_ENCODING_CP852"), GdaFrame::OnEncodingCP852)
EVT_MENU(XRCID("ID_ENCODING_CP866"), GdaFrame::OnEncodingCP866)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_1"), GdaFrame::OnEncodingISO8859_1)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_2"), GdaFrame::OnEncodingISO8859_2)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_3"), GdaFrame::OnEncodingISO8859_3)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_5"), GdaFrame::OnEncodingISO8859_5)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_7"), GdaFrame::OnEncodingISO8859_7)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_8"), GdaFrame::OnEncodingISO8859_8)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_9"), GdaFrame::OnEncodingISO8859_9)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_10"), GdaFrame::OnEncodingISO8859_10)
EVT_MENU(XRCID("ID_ENCODING_ISO_8859_15"), GdaFrame::OnEncodingISO8859_15)
EVT_MENU(XRCID("ID_ENCODING_GB2312"), GdaFrame::OnEncodingGB2312)
EVT_MENU(XRCID("ID_ENCODING_BIG5"), GdaFrame::OnEncodingBIG5)
EVT_MENU(XRCID("ID_ENCODING_KOI8_R"), GdaFrame::OnEncodingKOI8_R)
EVT_MENU(XRCID("ID_ENCODING_SHIFT_JIS"), GdaFrame::OnEncodingSHIFT_JIS)
EVT_MENU(XRCID("ID_ENCODING_EUC_JP"), GdaFrame::OnEncodingEUC_JP)
EVT_MENU(XRCID("ID_ENCODING_EUC_KR"), GdaFrame::OnEncodingEUC_KR)

EVT_MENU(GdaConst::ID_TIME_SYNC_VAR1, GdaFrame::OnTimeSyncVariable1)
EVT_MENU(GdaConst::ID_TIME_SYNC_VAR2, GdaFrame::OnTimeSyncVariable2)
EVT_MENU(GdaConst::ID_TIME_SYNC_VAR3, GdaFrame::OnTimeSyncVariable3)
EVT_MENU(GdaConst::ID_TIME_SYNC_VAR4, GdaFrame::OnTimeSyncVariable4)

EVT_MENU(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR1, GdaFrame::OnFixedScaleVariable1)
EVT_MENU(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR2, GdaFrame::OnFixedScaleVariable2)
EVT_MENU(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR3, GdaFrame::OnFixedScaleVariable3)
EVT_MENU(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR4, GdaFrame::OnFixedScaleVariable4)

EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_1, GdaFrame::OnPlotsPerView1)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_2, GdaFrame::OnPlotsPerView2)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_3, GdaFrame::OnPlotsPerView3)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_4, GdaFrame::OnPlotsPerView4)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_5, GdaFrame::OnPlotsPerView5)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_6, GdaFrame::OnPlotsPerView6)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_7, GdaFrame::OnPlotsPerView7)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_8, GdaFrame::OnPlotsPerView8)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_9, GdaFrame::OnPlotsPerView9)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_10, GdaFrame::OnPlotsPerView10)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_OTHER, GdaFrame::OnPlotsPerViewOther)
EVT_MENU(GdaConst::ID_PLOTS_PER_VIEW_ALL, GdaFrame::OnPlotsPerViewAll)

EVT_MENU(XRCID("ID_DISPLAY_STATUS_BAR"), GdaFrame::OnDisplayStatusBar)

EVT_MENU(XRCID("wxID_ABOUT"), GdaFrame::OnHelpAbout)


END_EVENT_TABLE()

std::vector<GdaFrame::MenuItem> GdaFrame::htmlMenuItems;

void GdaFrame::UpdateToolbarAndMenus()
{
	// This method is called when no particular window is currently active.
	// In this case, the close menu item should be disabled.

	LOG_MSG("In GdaFrame::UpdateToolbarAndMenus");
	GeneralWxUtils::EnableMenuItem(GetMenuBar(), "File", wxID_CLOSE,
								   false);

	Project* p = GetProject();
	bool proj_open = (p != 0);
	bool shp_proj = proj_open && !p->IsTableOnlyProject();
	bool table_proj = proj_open && p->IsTableOnlyProject();
	//bool time_variant = proj_open && p->GetTableInt()->IsTimeVariant();
    bool time_variant = proj_open;
    
	wxMenuBar* mb = GetMenuBar();

	// Reset the toolbar frame title to default.
	SetTitle("GeoDa");
	if (proj_open && project_p) SetTitle(project_p->GetProjectTitle());
	
    EnableTool(XRCID("ID_NEW_PROJECT"), !proj_open);
	EnableTool(XRCID("ID_OPEN_PROJECT"), !proj_open);
	EnableTool(XRCID("ID_CLOSE_PROJECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJECT"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_OPEN_PROJECT"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_CLOSE_PROJECT"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_SHP"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_SQLITE"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_CSV"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_DBF"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_GDB"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_JSON"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_GML"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_KML"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_MAPINFO"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "File", XRCID("ID_NEW_PROJ_FROM_XLS"), !proj_open);
	
	if (!proj_open) {
		// Disable only if project not open.  Otherwise, leave changing
		// Save state to SaveButtonManager
		EnableTool(XRCID("ID_SAVE_PROJECT"), proj_open);
		GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_PROJECT"), false);
	}
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_AS_PROJECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_LAYER"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_SELECTED"), proj_open);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_PROJECT_INFO"), proj_open);
	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), true);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), false);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), false);

	EnableTool(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), proj_open);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	EnableTool(XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	EnableTool(XRCID("ID_CONNECTIVITY_MAP_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools", XRCID("ID_TOOLS_WEIGHTS_MANAGER"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools", XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools", XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools", XRCID("ID_CONNECTIVITY_MAP_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, "Tools", XRCID("ID_POINTS_FROM_TABLE"), table_proj);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"), false);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_PAN_MODE"), proj_open);	
		
	GeneralWxUtils::EnableMenuAll(mb, "Explore", proj_open);
	EnableTool(XRCID("IDM_BOX"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BOX"), proj_open);

	EnableTool(XRCID("IDM_HIST"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_HIST"), proj_open);

	EnableTool(XRCID("IDM_SCATTERPLOT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTERPLOT"), proj_open);
	EnableTool(XRCID("IDM_BUBBLECHART"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BUBBLECHART"), proj_open);
	EnableTool(XRCID("IDM_SCATTERPLOT_MAT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_SCATTERPLOT_MAT"), proj_open);
	EnableTool(XRCID("IDM_COV_SCATTERPLOT"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_COV_SCATTERPLOT"), shp_proj);
	
	EnableTool(XRCID("IDM_PCP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_PCP"), proj_open);

	EnableTool(XRCID("ID_COND_PLOT_CHOICES"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COND_MENU"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"), proj_open);
	
	EnableTool(XRCID("IDM_3DP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_3DP"), proj_open);
	
	EnableTool(XRCID("IDM_LINE_CHART"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LINE_CHART"), proj_open);

	EnableTool(XRCID("IDM_NEW_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, "Table", proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_TIME_CHOOSER"),time_variant);
    
	// Temporarily removed for 1.6 release work
	//<object class="wxMenuItem" name="ID_CALCULATOR">
	//  <label>Calculator</label>
	//</object>
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_CALCULATOR"), true);
	EnableTool(XRCID("ID_SHOW_TIME_CHOOSER"), time_variant);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_DATA_MOVIE"), proj_open);
	EnableTool(XRCID("ID_SHOW_DATA_MOVIE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_CAT_CLASSIF"), proj_open);
	EnableTool(XRCID("ID_SHOW_CAT_CLASSIF"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_VAR_GROUPING_EDITOR"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TIME_EDITOR"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_TO_CSV_FILE"), proj_open);
		
	EnableTool(XRCID("ID_MORAN_MENU"), proj_open);
	EnableTool(XRCID("ID_LISA_MENU"), shp_proj);
	EnableTool(XRCID("IDM_GETIS_ORD_MENU"), shp_proj);
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
	EnableTool(XRCID("IDM_LOCAL_G"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LOCAL_G"), shp_proj);
	EnableTool(XRCID("IDM_LOCAL_G_STAR"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LOCAL_G_STAR"), shp_proj);
	
	
	EnableTool(XRCID("IDM_CORRELOGRAM"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_CORRELOGRAM"), shp_proj);
	
	GeneralWxUtils::EnableMenuAll(mb, "Map", shp_proj);
	EnableTool(XRCID("ID_MAP_CHOICES"), shp_proj);
	EnableTool(XRCID("ID_DATA_MOVIE"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), shp_proj);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_REGRESSION_CLASSIC"), proj_open);
	EnableTool(XRCID("ID_REGRESSION_CLASSIC"), proj_open);
    EnableTool(XRCID("ID_PUBLISH"), proj_open && project_p->GetDatasourceType()==GdaConst::ds_cartodb);
	
	//Empty out the Options menu:
	wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
}

void GdaFrame::SetMenusToDefault()
{
	LOG_MSG("Entering GdaFrame::SetMenusToDefault");
	// This method disables all menu items that are not
	// in one of File, Tools, Methods, or Help menus.
	wxMenuBar* mb = GetMenuBar();
	if (!mb) return;
	//wxMenu* menu = NULL;
	wxString menuText = wxEmptyString;
	int menuCnt = mb->GetMenuCount();
	for (int i=0; i<menuCnt; i++) {
		mb->GetMenu(i);
		menuText = mb->GetMenuLabelText(i);
		if ( (menuText != "File") &&
			 (menuText != "Tools") &&
			 (menuText != "Table") &&
			 (menuText != "Help") ) {
			GeneralWxUtils::EnableMenuAll(mb, menuText, false);
		}
	}

	LOG_MSG("Entering GdaFrame::SetMenusToDefault");
}

GdaFrame* GdaFrame::gda_frame = 0;
bool GdaFrame::projectOpen = false;
Project* GdaFrame::project_p = 0;
std::list<wxAuiToolBar*> GdaFrame::toolbar_list(0);

GdaFrame::GdaFrame(const wxString& title, const wxPoint& pos,
				   const wxSize& size, long style)
: wxFrame(NULL, -1, title, pos, size, style)
{
	LOG_MSG("Entering GdaFrame::GdaFrame");
		
	SetBackgroundColour(*wxWHITE);
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
	SetMenuBar(wxXmlResource::Get()->LoadMenuBar("ID_SHARED_MAIN_MENU"));

	if (!GetHtmlMenuItems() || htmlMenuItems.size() == 0) {
		LOG_MSG("Failed to get Html Menu Items from Preferences sqlite DB");
	} else {
		wxMenuBar* mb = GetMenuBar();
		int exp_menu_ind = mb->FindMenu("Explore");
		wxMenu* exp_menu = mb->GetMenu(exp_menu_ind);
		wxMenu* html_menu = new wxMenu();
		int base_id = GdaConst::ID_HTML_MENU_ENTRY_CHOICE_0;
		for (size_t i=0; i<htmlMenuItems.size(); ++i) {
			html_menu->Append(base_id++, htmlMenuItems[i].menu_title);
		}
		exp_menu->AppendSubMenu(html_menu, GdaConst::html_submenu_title);
	}
	
    
    wxObject* tb_obj = wxXmlResource::Get()->LoadObject(this, "ToolBar", "wxAuiToolBar");
	wxAuiToolBar* tb1 = (wxAuiToolBar*)tb_obj;
    tb1->SetMargins(10,10);
    tb1->SetMinSize(GetMinSize());
    tb1->Realize();

    tb1->Connect(wxEVT_SIZE, wxSizeEventHandler(GdaFrame::OnSize), NULL, this);

    
    toolbar_list.push_front(tb1);
    
    
    gda_frame = this;

	
	
	SetMenusToDefault();
 	UpdateToolbarAndMenus();
    SetEncodingCheckmarks(wxFONTENCODING_UTF8);
		
	LOG_MSG("Exiting GdaFrame::GdaFrame");
}

GdaFrame::~GdaFrame()
{
	LOG_MSG("Entering GdaFrame::~GdaFrame()");
	GdaFrame::gda_frame = 0;
	LOG_MSG("Exiting GdaFrame::~GdaFrame()");
}

void GdaFrame::OnSize(wxSizeEvent& event)
{
    BOOST_FOREACH( wxAuiToolBar* tb, toolbar_list ) {
        if (tb)	{
            tb->SetOverflowVisible(!tb->GetToolFitsByIndex(tb->GetToolCount()-1));
            tb->Refresh();
        }
    }
    event.Skip();
}

void GdaFrame::EnableTool(int xrc_id, bool enable)
{
	BOOST_FOREACH( wxAuiToolBar* tb, toolbar_list ) {
        if (tb)	{
            tb->EnableTool(xrc_id, enable);
            tb->Refresh();
        }
	}
}

void GdaFrame::EnableTool(const wxString& id_str, bool enable)
{
	BOOST_FOREACH( wxAuiToolBar* tb, toolbar_list ) {
		if (tb)	tb->EnableTool(wxXmlResource::GetXRCID(id_str), enable);
	}
}

bool GdaFrame::IsProjectOpen()
{
	return projectOpen;
}

void GdaFrame::SetProjectOpen(bool open)
{
	projectOpen = open;
}

boost::uuids::uuid GdaFrame::GetWeightsId(const wxString& caption)
{
	SelectWeightsDlg dlg(project_p, this, caption);
	if (dlg.ShowModal()!= wxID_OK) return boost::uuids::nil_uuid();
	project_p->GetWManInt()->MakeDefault(dlg.GetSelWeightsId());
	return dlg.GetSelWeightsId();
}

void GdaFrame::OnOpenNewTable()
{
	LOG_MSG("Entering GdaFrame::OnOpenNewTable");
	TableFrame* tf = 0;
	wxGrid* g = project_p->FindTableGrid();
	if (g) tf = (TableFrame*) g->GetParent();
	if (!tf) {
		wxString msg("The Table should always be open, although somtimes it "
					 "is hidden while the project is open.  This condition "
					 "has been violated.  Please report this to "
					 "the program developers.");
		wxMessageDialog dlg(this, msg, "Warning", wxOK | wxICON_WARNING);
		dlg.ShowModal();
		tf = new TableFrame(gda_frame, project_p,
							GdaConst::table_frame_title,
							wxDefaultPosition,
							GdaConst::table_default_size,
							wxDEFAULT_FRAME_STYLE);
	}
	tf->Show(true);
	tf->Maximize(false);
	tf->Raise();
	
	LOG_MSG("Exiting GdaFrame::OnOpenNewTable");
}

/** returns false if user wants to abort the operation */
bool GdaFrame::OnCloseProject(bool ignore_unsaved_changes)
{
	LOG_MSG("Entering GdaFrame::OnCloseProject");
	
	if (IsProjectOpen() && !ignore_unsaved_changes) {
		bool is_new_project = (project_p->GetProjectFullPath().empty() ||
							   !wxFileExists(project_p->GetProjectFullPath()));
		bool unsaved_meta_data = is_new_project ||
			(project_p->GetSaveButtonManager() &&
			 project_p->GetSaveButtonManager()->IsMetaDataSaveNeeded());
		bool unsaved_ds_data =
			project_p->GetTableInt()->ChangedSinceLastSave();
	
		wxString msg;
		wxString title;
		//if (is_new_project || unsaved_ds_data || unsaved_ds_data) {
		if (unsaved_ds_data) {

			title = "Close with unsaved changes?";
			
			msg << "\n";			
			
			msg << "There are unsaved data source (Table) changes.\n\n";			
			
			msg << "To save your work, go to File > Save";
			
			wxMessageDialog msgDlg(this, msg, title,
								   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if (msgDlg.ShowModal() != wxID_YES) return false;
		}
	}
	
	SetProjectOpen(false);
	if (project_p) {
        if (project_p->GetTableState()) {
            project_p->GetTableState()->closeAndDeleteWhenEmpty();
        }
        if (project_p->GetTimeState()) {
            project_p->GetTimeState()->closeAndDeleteWhenEmpty();
        }
		if (project_p->GetWManState()) {
            project_p->GetWManState()->closeAndDeleteWhenEmpty();
        }
        
        if (project_p->GetFramesManager()) {
            project_p->GetFramesManager()->closeAndDeleteWhenEmpty();
            std::list<FramesManagerObserver*> observers(project_p->GetFramesManager()->getCopyObservers());
            std::list<FramesManagerObserver*>::iterator it;
            for (it=observers.begin(); it != observers.end(); ++it) {
				FramesManagerObserver* fmo = *it;
				wxTopLevelWindow* w = dynamic_cast<wxTopLevelWindow*>(fmo);
                if (w) {
                    wxString msg="Calling Close(true) for wxTopLevelWindow: ";
					msg << w->GetTitle();
                    LOG_MSG(msg);
                    w->Close(true);
                }
            }
        }
        
		if (project_p->GetHighlightState()) {
			project_p->GetHighlightState()->closeAndDeleteWhenEmpty();
		}
		if (project_p->GetConMapHlightState()) {
			project_p->GetConMapHlightState()->closeAndDeleteWhenEmpty();
		}
		project_p->CleanupPairsHLState();
	}
	if (project_p) delete project_p; project_p = 0;

	// notify wxTopLevelWindows not managed by Project
	wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
		if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
			LOG_MSG("CalculatorDlg already opened.");
			w->DisconnectFromProject();
		}
        node = node->GetNext();
    }

	UpdateToolbarAndMenus();
	
	return true;
}


void GdaFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnClose");
	
	bool is_new_project = (IsProjectOpen() &&
						   (project_p->GetProjectFullPath().empty() ||
							!wxFileExists(project_p->GetProjectFullPath())));
	bool unsaved_meta_data = is_new_project ||
		(IsProjectOpen() && project_p->GetSaveButtonManager() &&
		project_p->GetSaveButtonManager()->IsMetaDataSaveNeeded());
	bool unsaved_ds_data = (IsProjectOpen() &&
							project_p->GetTableInt()->ChangedSinceLastSave());
	
	wxString msg;
	wxString title;
	if (unsaved_ds_data) {
		title = "Exit with unsaved changes?";
		
		msg << "\n";
		
        msg << "There are ";
		
		if (!is_new_project) {
			msg << "unsaved project file changes, and ";
		}
		
		if (unsaved_ds_data) {
			msg << "unsaved data source (Table) changes.\n\n";
		}
		
		if (!is_new_project) {
            msg << "To save your project, go to File > Save Project\n\n";
        }
		msg << "To save your work, go to File > Save";
	} else {
		title = "Exit?";
		msg = "Ok to Exit?";
	}
	
	if (IsProjectOpen()) {
		wxMessageDialog msgDlg(this, msg, title,
							   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	
		// Show the message dialog, and if it returns wxID_YES...
		if (msgDlg.ShowModal() != wxID_YES) return;
	}
	
	OnCloseProject(true);

	// Close windows not associated managed by Project
	wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
		if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
			LOG_MSG("Closing Calculator");
			w->Close(true);
		}
        node = node->GetNext();
    }
	Destroy();

	LOG_MSG("Exiting GdaFrame::OnClose");
}

void GdaFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnMenuClose");
	Close(); // This will result in a call to OnClose
    LOG_MSG("Exiting GdaFrame::OnMenuClose");
}

void GdaFrame::OnCloseProjectEvt(wxCommandEvent& event)
{
	LOG_MSG("In GdaFrame::OnCloseProjectEvt");
	OnCloseProject();
}

/** New Project opened by the user from outside the program, for example
 when a user double clicks on a datasource type associated with GeoDa, or
 when a user right clicks on a file and chooses "Open with GeoDa."  Can
 also be called from the shortcut "New Project From" File menu option. */
void GdaFrame::NewProjectFromFile(const wxString& full_file_path)
{
	LOG_MSG("Entering GdaFrame::NewProjectFromFile");
	
	wxString proj_title = wxFileName(full_file_path).GetName();
    wxString layer_name = proj_title;
	
	try {
		FileDataSource fds(full_file_path);
	
        // this datasource will be freed when dlg exit, so make a copy
        // in project_p
        project_p = new Project(proj_title, layer_name, &fds);
    } catch (GdaException& e) {
        wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
    
    wxString error_msg;
    if (!project_p) {
        error_msg << "Error: Could not initialize new project.";
    } else if (!project_p->IsValid()) {
        error_msg << "Error:";
        error_msg << project_p->GetOpenErrorMessage();
    }
	if (!error_msg.IsEmpty()) {

        wxMessageDialog dlg (this, error_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
    
	// By this point, we know that project has created as
	// TopFrameManager object with delete_if_empty = false
	
	// This call is very improtant because we need the wxGrid to
	// take ownership of the TableBase instance (bug in wxWidgets)
	
	TableFrame* tf;
	tf = new TableFrame(this, project_p,
						GdaConst::table_frame_title,
						wxDefaultPosition,
						GdaConst::table_default_size,
						wxDEFAULT_FRAME_STYLE);
	if (project_p->IsTableOnlyProject()) tf->Show(true);
	
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
	
	if (!project_p->IsTableOnlyProject()) {
		std::vector<int> col_ids;
		std::vector<GdaVarTools::VarInfo> var_info;
		MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
										  var_info, col_ids,
										  CatClassification::no_theme,
										  MapCanvas::no_smoothing, 1,
										  boost::uuids::nil_uuid(),
										  wxDefaultPosition,
										  GdaConst::map_default_size);
		nf->UpdateTitle();
	}
	
	LOG_MSG("Exiting GdaFrame::NewProjectFromFile");
}

/** New Project opened by the user from within GeoDa */
void GdaFrame::OnNewProject(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnNewProject");
	ConnectDatasourceDlg dlg(this);
	if (dlg.ShowModal() != wxID_OK) return;
    
	wxString proj_title = dlg.GetProjectTitle();
    wxString layer_name = dlg.GetLayerName();
    IDataSource* datasource = dlg.GetDataSource();
    try {
        // this datasource will be freed when dlg exit, so make a copy
        // in project_p
        project_p = new Project(proj_title, layer_name, datasource);
    } catch (GdaException& e) {
        wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
    
    wxString error_msg;
    if (!project_p) {
        error_msg << "Error: Could not initialize new project.";
    } else if (!project_p->IsValid()) {
        error_msg << "Error:";
        error_msg << project_p->GetOpenErrorMessage();
    }
	if (!error_msg.IsEmpty()) {
        /*if(datasource) {
            delete datasource;
            datasource = NULL;
        }*/
        wxMessageDialog dlg (this, error_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
    
	// By this point, we know that project has created as
	// TopFrameManager object with delete_if_empty = false
	
	// This call is very improtant because we need the wxGrid to
	// take ownership of the TableBase instance (bug in wxWidgets)
	
	TableFrame* tf;
	tf = new TableFrame(this, project_p,
						GdaConst::table_frame_title,
						wxDefaultPosition,
						GdaConst::table_default_size,
						wxDEFAULT_FRAME_STYLE);
	if (project_p->IsTableOnlyProject()) tf->Show(true);
	
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
	
	if (!project_p->IsTableOnlyProject()) {
		std::vector<int> col_ids;
		std::vector<GdaVarTools::VarInfo> var_info;
		MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
										  var_info, col_ids,
										  CatClassification::no_theme,
										  MapCanvas::no_smoothing, 1,
										  boost::uuids::nil_uuid(),
										  wxPoint(80,160),
										  GdaConst::map_default_size);
		nf->UpdateTitle();
	}
	
	LOG_MSG("Exiting GdaFrame::OnNewProject");	
}


void GdaFrame::OnNewProjectFromShp(wxCommandEvent& event)
{
	wxString wc = "ESRI Shapefile (*.shp)|*.shp";
	wxFileDialog dlg(this,"New Project From Shapefile", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromSqlite(wxCommandEvent& event)
{
	wxString wc = "SQLite/SpatiaLite (*.sqlite)|*.sqlite";
	wxFileDialog dlg(this,"New Project From SQLite/SpatiaLite", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromCsv(wxCommandEvent& event)
{
	wxString wc = "Comma Separated Value (*.csv)|*.csv";
	wxFileDialog dlg(this,"New Project From CSV", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromDbf(wxCommandEvent& event)
{
	wxString wc = "dBase Database File (*.dbf)|*.dbf";
	wxFileDialog dlg(this,"New Project From dBase", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromGdb(wxCommandEvent& event)
{
	wxString wc = "ESRI File Geodatabase (*.gdb)|*.gdb";
	wxFileDialog dlg(this,"New Project From ESRI File Gdb", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromJson(wxCommandEvent& event)
{
	wxString wc = "GeoJSON (*.geojson;*.json)|*.geojson;*.json|"
	"GeoJSON (*.geojson)|*.geojson|"
	"GeoJSON (*.json)|*.json";
	wxFileDialog dlg(this,"New Project From JSON", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromGml(wxCommandEvent& event)
{
	wxString wc = "Geography Markup Language (*.gml)|*.gml";
	wxFileDialog dlg(this,"New Project From GML", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromKml(wxCommandEvent& event)
{
	wxString wc = "Keyhole Markup Language (*.kml)|*.kml";
	wxFileDialog dlg(this,"New Project From KML", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromMapinfo(wxCommandEvent& event)
{
	wxString wc = "MapInfo (*.tab;*.mif;*.mid)|*.tab;*.mif;*.mid|"
	"MapInfo Tab (*.tab)|*.tab|"
	"MapInfo MID (*.mid)|*.mid|"
	"MapInfo MID (*.mif)|*.mif";
	wxFileDialog dlg(this,"New Project From Mapinfo", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}

void GdaFrame::OnNewProjectFromXls(wxCommandEvent& event)
{
	wxString wc = "MS Excel (*.xls)|*.xls";
	wxFileDialog dlg(this,"New Project From MS Excel", "", "", wc);
	if (dlg.ShowModal() != wxID_OK) return;
	NewProjectFromFile(dlg.GetPath());
}


/**
 OpenProject can be called while another project file is currently open.  This
 can happen currently on Mac OSX when the user double-clicks on multiple
 project files.  At present, we will simply present a message to the user
 saying that the current project must be closed first.  In the future,
 especially when multiple layers are supported, we could do something
 fancier.
 */
void GdaFrame::OpenProject(const wxString& full_proj_path)
{
	LOG_MSG("Entering GdaFrame::OpenProject");
	{
		wxString msg;
		wxFileName fn(full_proj_path);
		if (fn.GetExt().CmpNoCase("gda") != 0) {
			// msg << "Error: \"" << full_proj_path << "\" not a project file.";
			if (IsProjectOpen()) {
				Raise();
				wxString msg;
				msg << "You have requested to create a new file project ";
				msg << full_proj_path;
				msg << " while another project is open.  Please close project ";
				msg << project_p->GetProjectTitle();
				msg << " and try again.";
				LOG_MSG(msg);
				LOG_MSG("Exiting GdaFrame::OpenProject");
				return;
			}
			NewProjectFromFile(full_proj_path);
			LOG_MSG("Exiting GdaFrame::OpenProject");
			return;
		}	
		if (!wxFileExists(full_proj_path)) {
            msg << "Error: \"" << full_proj_path << "\" not found.";
		}
		if (!msg.IsEmpty()) {
			LOG_MSG(msg);
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	// if requested project to open is already open, just raise
	// current window
	if (IsProjectOpen()) {
		if (project_p->GetProjectFullPath().CmpNoCase(full_proj_path) == 0) {
			LOG_MSG("Requested project is already open.");
			Raise();
			return;
		}
		Raise();
		wxString msg;
		msg << "You have requested to open project ";
		msg << full_proj_path;
		msg << " while another project is open.  Please close project ";
		msg << project_p->GetProjectTitle();
		msg << " and try again.";
		LOG_MSG(msg);
		// For now, don't display a warning message to the user,
		// just raise the toplevel window in either case.
		//wxMessageDialog dlg (this, msg, "Information",
		//					 wxOK | wxICON_INFORMATION);
		//dlg.ShowModal();
		return;
	}
	
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		LOG_MSG("Open wxTopLevelWindows found for current project, "
				"calling OnCloseProject()");
		if (!OnCloseProject()) return;
	}

    try {
        project_p = new Project(full_proj_path);
        if (!project_p->IsValid()) {
            wxString msg("Error while opening project:\n\n");
            msg << project_p->GetOpenErrorMessage();
            throw GdaException(msg.c_str());
        }

    } catch (GdaException &e) {
        wxString msg( e.what() ) ;
        wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		delete project_p; project_p = 0;
		return;
    }
	
	// By this point, we know that project has created as
	// TopFrameManager object with delete_if_empty = false
	
	// This call is very improtant because we need the wxGrid to
	// take ownership of the TableBase instance (due to bug in wxWidgets)
	TableFrame* tf;
	tf = new TableFrame(this, project_p,
						GdaConst::table_frame_title,
						wxDefaultPosition,
						GdaConst::table_default_size,
						wxDEFAULT_FRAME_STYLE);
	if (project_p->IsTableOnlyProject())
        tf->Show(true);
	
	SetProjectOpen(true);
	UpdateToolbarAndMenus();
	
	// Associate Project with Calculator if open
	wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
		if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
			LOG_MSG("Notifying Calculator of new Project.");
			w->ConnectToProject(GetProject());
		}
        node = node->GetNext();
    }

	if (!project_p->IsTableOnlyProject()) {
		std::vector<int> col_ids;
		std::vector<GdaVarTools::VarInfo> var_info;
		MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
										  var_info, col_ids,
										  CatClassification::no_theme,
										  MapCanvas::no_smoothing, 1,
										  boost::uuids::nil_uuid(),
										  wxPoint(80,160),
										  GdaConst::map_default_size);
		nf->UpdateTitle();
	}
}


void GdaFrame::OnOpenProject(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnOpenProject");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		if (!OnCloseProject())
            return;
	}
	wxString wildcard = "GeoDa Project (*.gda)|*.gda";
	wxFileDialog dlg(this, "GeoDa Project File to Open", "", "", wildcard);
	if (dlg.ShowModal() != wxID_OK) return;

	OpenProject(dlg.GetPath());
}

void GdaFrame::OnSaveProject(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnSaveProject");
	if (!project_p) return;

    // check if locale changed, if it did, warn people to export
    /*
    setlocale(LC_ALL, "");
    struct lconv *poLconv = localeconv();
    wxString sys_thousands_sep = poLconv->thousands_sep;
    wxString sys_decimal_point = poLconv->decimal_point;
    setlocale(LC_ALL, "C");
    if ( sys_decimal_point !=  "." ) {
        wxString msg = "GeoDa will save data (numbers) using standard C "
        "locale, which uses dot \".\" as decimal point. It's different "
        "than the current locale. If you want to keep your locale, please "
        "export to a new datasource. Do you want to save?";
        wxMessageDialog dlg(this, msg, "GeoDa Locale Information.",
                            wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (dlg.ShowModal() != wxID_YES) {
            return;
        }
    }
    */
    
	// Save Data Source changes first if needed
	try {
        if (project_p->IsDataTypeChanged()) {
            // e.g. dbf, add geometries (points), to shapefile
        	ExportDataDlg dlg(this, project_p);
        	dlg.ShowModal();
        } else {
    		project_p->SaveDataSourceData();
            try {
                project_p->SaveProjectConf();
            } catch( GdaException& e) {}
        }
	}
	catch (GdaException& e) {
		wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	// We know Data Source data was saved successfully
    
	SaveButtonManager* sbm = project_p->GetSaveButtonManager();
	if (sbm) {
		sbm->SetAllowEnableSave(true);
		sbm->SetMetaDataSaveNeeded(false);
	}
	
	LOG_MSG("Exiting GdaFrame::OnSaveProject");
}

void GdaFrame::OnSaveAsProject(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
    
	// Following 2 lines are original SaveAsProject dlg
    //SaveAsDlg dlg(this, project_p);
    //dlg.ShowModal();
	
	wxString proj_fname = project_p->GetProjectFullPath();
	bool is_new_project = (proj_fname.empty() || !wxFileExists(proj_fname));
	
	wxString msg;

		msg << "A project file contains extra information not directly ";
		msg << "stored in the data source such as variable order and grouping.";
		
		wxMessageDialog proj_create_dlg(this, msg, "Create Project File Now?",
										wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION);
		if (proj_create_dlg.ShowModal() != wxID_YES) return;
		
		wxString wildcard = "GeoDa Project (*.gda)|*.gda";
		wxFileDialog dlg(this, "New Project Filename", "", "", wildcard,
						 wxFD_SAVE);
		if (dlg.ShowModal() != wxID_OK) return;
		
		try {
			project_p->SpecifyProjectConfFile(dlg.GetPath());
		} catch (GdaException& e) {
			wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	
	try {
        project_p->SaveProjectConf();
		
        wxString msg = "Saved successfully.";
        if ( project_p->IsTableOnlyProject() &&
            !project_p->main_data.records.empty() ) {
            // case: users create geometries in a table-only project
            msg << "\n\nWarning: newly created geometries are not saved. ";
            msg << "Please use \"Export\" to save geometries and related data.";
        }
        wxMessageDialog dlg(this, msg , "Info", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
	} catch (GdaException& e) {
		wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	LOG_MSG(wxString("Wrote GeoDa Project File: ") + proj_fname);
}

void GdaFrame::OnSelectWithRect(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithRect(event);
}

void GdaFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithCircle(event);
}

void GdaFrame::OnSelectWithLine(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithLine(event);
}

void GdaFrame::OnSelectionMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectionMode(event);
}

void GdaFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFitToWindowMode(event);
}

void GdaFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnFixedAspectRatioMode(event);
}

void GdaFrame::OnZoomMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnZoomMode(event);
}

void GdaFrame::OnPanMode(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPanMode(event);
}

void GdaFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called GdaFrame::OnPrintCanvasState");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnPrintCanvasState(event);
	
	// Add this menu item to the XRC file to see this debugging option:
	//<object class="wxMenuItem" name="ID_PRINT_CANVAS_STATE">
	//  <label>Print Canvas State to Log File</label>
    //</object>s
}

void GdaFrame::OnChangeMapTransparency(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t)
        return;
    
	MapFrame* f = dynamic_cast<MapFrame*>(t);
    if (f)
        f->OnChangeMapTransparency();
}
void GdaFrame::OnCleanBasemap(wxCommandEvent& event)
{
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (t) {
        if (MapFrame* f = dynamic_cast<MapFrame*>(t)) return
            f->CleanBasemap();
    }
}
void GdaFrame::OnSetNoBasemap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(false,0);
    SetBasemapCheckmarks(0);
}
void GdaFrame::OnSetBasemap1(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,1);
    SetBasemapCheckmarks(1);
}
void GdaFrame::OnSetBasemap2(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,2);
    SetBasemapCheckmarks(2);
}
void GdaFrame::OnSetBasemap3(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,3);
    SetBasemapCheckmarks(3);
}
void GdaFrame::OnSetBasemap4(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,4);
    SetBasemapCheckmarks(4);
}
void GdaFrame::OnSetBasemap5(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,5);
    SetBasemapCheckmarks(5);
}
void GdaFrame::OnSetBasemap6(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,6);
    SetBasemapCheckmarks(6);
}
void GdaFrame::OnSetBasemap7(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,7);
    SetBasemapCheckmarks(7);
}

void GdaFrame::OnSetBasemap8(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnDrawBasemap(true,8);
    SetBasemapCheckmarks(8);
}

void GdaFrame::OnBasemapConfig(wxCommandEvent& event)
{
    BasemapConfDlg dlg(this,project_p);
    dlg.ShowModal();
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (t) t->OnRefreshMap(event);
}

void GdaFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnSaveCanvasImageAs(event);
}

void GdaFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering GdaFrame::OnQuit");
	// Generate a wxCloseEvent for GdaFrame.  GdaFrame::OnClose will
	// be called and will give the user a chance to not exit program.
	Close();
	LOG_MSG("Exiting GdaFrame::OnQuit");
}

void GdaFrame::OnSaveSelectedToColumn(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnSaveSelectedToColumn");
	SaveSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
	LOG_MSG("Exiting GdaFrame::OnSaveSelectedToColumn");
}

void GdaFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCanvasBackgroundColor(event);
}

void GdaFrame::OnLegendUseScientificNotation(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnLegendUseScientificNotation(event);
}

void GdaFrame::OnLegendBackgroundColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnLegendBackgroundColor(event);
}

void GdaFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableFillColor(event);	
}

void GdaFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineColor(event);
}

void GdaFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineVisible(event);
}

void GdaFrame::OnHighlightColor(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnHighlightColor(event);
}

void GdaFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnCopyImageToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCopyImageToClipboard(event);
	LOG_MSG("Exiting GdaFrame::OnCopyImageToClipboard");
}


void GdaFrame::OnCopyLegendToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnCopyLegendToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnCopyLegendToClipboard(event);
	LOG_MSG("Exiting GdaFrame::OnCopyLegendToClipboard");
}

void GdaFrame::OnKeyEvent(wxKeyEvent& event)
{
	Project* project = GetProject();
	if (event.GetModifiers() == wxMOD_CMD &&
		project && project->GetTimeState() &&
		project->GetTableInt()->IsTimeVariant() &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		project->GetTimeState()->SetCurrTime(
							project->GetTimeState()->GetCurrTime() + del);
		if (project->GetTimeState()->GetCurrTime() < 0) {
			project->GetTimeState()->SetCurrTime(
								project->GetTableInt()->GetTimeSteps()-1);
		} else if (project->GetTimeState()->GetCurrTime()
				   >= project->GetTableInt()->GetTimeSteps()) {
			project->GetTimeState()->SetCurrTime(0);
		}
		if (project->GetTimeState()) {
			project->GetTimeState()->notifyObservers();
		}
		return;
	}
	event.Skip();
}

void GdaFrame::OnToolsWeightsManager(wxCommandEvent& WXUNUSED(event) )
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (WeightsManFrame* w = dynamic_cast<WeightsManFrame*>(*it)) {
			LOG_MSG("Weights Manager already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new Weights Manager");
	WeightsManFrame* f = new WeightsManFrame(this, p);
}

void GdaFrame::OnToolsWeightsCreate(wxCommandEvent& WXUNUSED(event) )
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (CreatingWeightDlg* w = dynamic_cast<CreatingWeightDlg*>(*it)) {
			LOG_MSG("CreatingWeightDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new CreatingWeightDlg");
	CreatingWeightDlg* dlg = new CreatingWeightDlg(0, p);
	dlg->Show(true);
}

void GdaFrame::OnConnectivityHistView(wxCommandEvent& event )
{
	LOG_MSG("Entering GdaFrame::OnConnectivityHistView");
	boost::uuids::uuid id = GetWeightsId();
	if (id.is_nil()) return;
	ConnectivityHistFrame* f = new ConnectivityHistFrame(this, project_p, id);
	LOG_MSG("Exiting GdaFrame::OnConnectivityHistView");
}

void GdaFrame::OnConnectivityMapView(wxCommandEvent& event )
{
	LOG_MSG("Entering GdaFrame::OnConnectivityMapView");
	boost::uuids::uuid id = GetWeightsId("Choose Weights for Connectivity Map");
	if (id.is_nil()) return;
	ConnectivityMapFrame* f =
		new ConnectivityMapFrame(this, project_p, id,
								 wxDefaultPosition,
								 GdaConst::conn_map_default_size);
	LOG_MSG("Exiting GdaFrame::OnConnectivityMapView");
}

void GdaFrame::ShowConnectivityMapView(boost::uuids::uuid weights_id)
{
	LOG_MSG("Entering GdaFrame::ShowConnectivityMapView");
	if (!project_p || !project_p->GetWManInt() ||
		weights_id.is_nil()) return;
	
	ConnectivityMapFrame* f =
		new ConnectivityMapFrame(this, project_p, weights_id,
								 wxDefaultPosition,
								 GdaConst::conn_map_default_size);
	LOG_MSG("Exiting GdaFrame::ShowConnectivityMapView");
}

void GdaFrame::OnMapChoices(wxCommandEvent& event)
{
	LOG_MSG("Entering GdaFrame::OnMapChoices");
	wxMenu* popupMenu = 0;
	if (GeneralWxUtils::isMac()) {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES");
	} else {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES_NO_ICONS");
	}
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting GdaFrame::OnMapChoices");
}

#include "DialogTools/ASC2SHPDlg.h"
void GdaFrame::OnShapePointsFromASCII(wxCommandEvent& WXUNUSED(event) )
{
	ASC2SHPDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/CreateGridDlg.h"
void GdaFrame::OnShapePolygonsFromGrid(wxCommandEvent& WXUNUSED(event) )
{
	CreateGridDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/Bnd2ShpDlg.h"
void GdaFrame::OnShapePolygonsFromBoundary(wxCommandEvent& WXUNUSED(event) )
{
	Bnd2ShpDlg dlg(this);
	dlg.ShowModal();
}

#include "DialogTools/SHP2ASCDlg.h" 
void GdaFrame::OnShapeToBoundary(wxCommandEvent& WXUNUSED(event) )
{
	SHP2ASCDlg dlg(this);
	dlg.ShowModal();
}

void GdaFrame::OnShowTimeChooser(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
    
    wxPoint pt;
    
    bool hasTime = p->GetTableInt()->IsTimeVariant();
    bool opened = false;
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (TimeChooserDlg* w = dynamic_cast<TimeChooserDlg*>(*it)) {
            LOG_MSG("TimeChooserDlg already opened.");
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            pt = w->GetPosition();
            opened = true;
        }
    }
    if (!opened) {
        LOG_MSG("Opening a new TimeChooserDlg");
        TimeChooserDlg* dlg = new TimeChooserDlg(0, p->GetFramesManager(),
											 p->GetTimeState(),
											 p->GetTableState(),
											 p->GetTableInt());
        dlg->Show(true);
        pt = dlg->GetPosition();
    }

    opened = false;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (VarGroupingEditorDlg* w = dynamic_cast<VarGroupingEditorDlg*>(*it))
        {
            LOG_MSG("VarGroupingEditorDlg already opened.");
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            w->SetPosition(wxPoint(pt.x, pt.y + 130));
            opened =true;
            break;
        }
    }
    if (!opened) {
        LOG_MSG("Opening a new VarGroupingEditorDlg");
        VarGroupingEditorDlg* dlg = new VarGroupingEditorDlg(p, this);
        dlg->Show(true);
        int start_x = pt.x - 200;
        if (start_x) start_x = 0;
        dlg->SetPosition(wxPoint(pt.x, pt.y + 130));
    }
}

void GdaFrame::OnShowDataMovie(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (DataMovieDlg* w = dynamic_cast<DataMovieDlg*>(*it)) {
			LOG_MSG("DataMovieDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new DataMovieDlg");
	DataMovieDlg* dlg = new DataMovieDlg(0, p->GetFramesManager(),
										 p->GetTableState(),
										 p->GetTimeState(),
										 p->GetTableInt(),
										 p->GetHighlightState());
	dlg->Show(true);
}

void GdaFrame::OnShowCatClassif(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
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

CatClassifFrame* GdaFrame::GetCatClassifFrame(bool useScientificNotation)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return 0;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (CatClassifFrame* w = dynamic_cast<CatClassifFrame*>(*it)) {
			LOG_MSG("Cateogry Editor already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return w;
		}
	}
	
	LOG_MSG("Opening a new Cateogry Editor");
	CatClassifFrame* dlg = new CatClassifFrame(this, project_p, useScientificNotation);
	return dlg;
}

void GdaFrame::OnVarGroupingEditor(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (VarGroupingEditorDlg* w = dynamic_cast<VarGroupingEditorDlg*>(*it))
		{
			LOG_MSG("VarGroupingEditorDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new VarGroupingEditorDlg");
	VarGroupingEditorDlg* dlg = new VarGroupingEditorDlg(GetProject(), this);
	dlg->Show(true);
}

void GdaFrame::OnTimeEditor(wxCommandEvent& event)
{
	if (!GetProject() || !GetProject()->GetTableInt()) return;
	FramesManager* fm = GetProject()->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (TimeEditorDlg* w = dynamic_cast<TimeEditorDlg*>(*it)) {
			LOG_MSG("TimeEditorDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new TimeEditorDlg");
	TimeEditorDlg* dlg = new TimeEditorDlg(0, GetProject()->GetFramesManager(),
										   GetProject()->GetTableState(),
										   GetProject()->GetTableInt());
	dlg->Show(true);
}

void GdaFrame::OnMoveSelectedToTop(wxCommandEvent& event)
{
	if (!project_p || !project_p->FindTableBase()) return;
	project_p->FindTableBase()->MoveSelectedToTop();
}

void GdaFrame::OnInvertSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->FindTableBase()) return;
    
	HighlightState& hs = *project_p->GetHighlightState();
	hs.SetEventType(HLStateInt::invert);
	hs.notifyObservers();
}

void GdaFrame::OnClearSelection(wxCommandEvent& event)
{
	if (!project_p || !project_p->FindTableBase()) return;
	project_p->FindTableBase()->DeselectAllRows();
}

void GdaFrame::OnRangeSelection(wxCommandEvent& event)
{
	if (!GetProject() || !GetProject()->GetTableInt()) return;
	FramesManager* fm = GetProject()->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (RangeSelectionDlg* w = dynamic_cast<RangeSelectionDlg*>(*it)) {
			LOG_MSG("RangeSelectionDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new RangeSelectionDlg");
	RangeSelectionDlg* dlg = new RangeSelectionDlg(0, GetProject(),
											GetProject()->GetFramesManager(),
											GetProject()->GetTableState());
	dlg->Show(true);
}

void GdaFrame::OnFieldCalculation(wxCommandEvent& event)
{
	if (!GetProject()) return;
	FramesManager* fm = GetProject()->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (FieldNewCalcSheetDlg* w = dynamic_cast<FieldNewCalcSheetDlg*>(*it)) {
			LOG_MSG("FieldNewCalcSheetDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new FieldNewCalcSheetDlg");
	FieldNewCalcSheetDlg* dlg =
		new FieldNewCalcSheetDlg(GetProject(), this,
								 wxID_ANY, "Variable Calculation",
								 wxDefaultPosition,
								 wxSize(700, 500));
	dlg->Show(true);
}

void GdaFrame::OnCalculator(wxCommandEvent& event)
{
	wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
		if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
			LOG_MSG("CalculatorDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
        node = node->GetNext();
    }

	LOG_MSG("Opening a new CalculatorDlg");
	// GetProject() could be NULL, but CalculatorDlg supports this.
	CalculatorDlg* dlg = new CalculatorDlg(GetProject(), this);
	dlg->Show(true);
}

void GdaFrame::OnAddCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
	
	DataViewerAddColDlg dlg(project_p, this);
	dlg.ShowModal();
}

void GdaFrame::OnDeleteCol(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
	
	DataViewerDeleteColDlg dlg(project_p->GetTableInt(), this);
	dlg.ShowModal();
}

void GdaFrame::OnEditFieldProperties(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (DataViewerEditFieldPropertiesDlg* w
			= dynamic_cast<DataViewerEditFieldPropertiesDlg*>(*it))
		{
			LOG_MSG("DataViewerEditFieldPropertiesDlg already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new DataViewerEditFieldPropertiesDlg");
	DataViewerEditFieldPropertiesDlg* dlg;
	dlg = new DataViewerEditFieldPropertiesDlg(p, wxDefaultPosition,
											   wxSize(600, 400));
	dlg->Show(true);
}

void GdaFrame::OnChangeFieldType(wxCommandEvent& event)
{
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (DataChangeTypeFrame* w	= dynamic_cast<DataChangeTypeFrame*>(*it))
		{
			LOG_MSG("DataChangeTypeFrame already opened.");
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	LOG_MSG("Opening a new DataChangeTypeFrame");
	DataChangeTypeFrame* dlg = new DataChangeTypeFrame(this, p);
}


void GdaFrame::OnMergeTableData(wxCommandEvent& event)
{
	if (!project_p || !project_p->FindTableBase()) return;
	
	MergeTableDlg dlg(project_p->GetTableInt(), wxDefaultPosition);
	dlg.ShowModal();
}

void GdaFrame::OnExportSelectedToOGR(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
    vector<int> selected_rows;
    project_p->GetSelectedRows(selected_rows);
    if ( selected_rows.empty() ) {
        wxMessageDialog dlg (this,
                             "Please select features first.",
                             "Info", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
        return;
    }
	ExportDataDlg dlg(this, project_p, true);
	dlg.ShowModal();
}

void GdaFrame::OnExportToOGR(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
	ExportDataDlg dlg(this, project_p);
	dlg.ShowModal();
}

/** Export to CSV.  This is not used currently. */
void GdaFrame::OnExportToCsvFile(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
	ExportCsvDlg dlg(this, project_p);
	dlg.ShowModal();
}

void GdaFrame::OnShowProjectInfo(wxCommandEvent& event)
{
	if (!project_p) return;
	ProjectInfoDlg dlg(project_p);
	dlg.ShowModal();
}

void GdaFrame::OnHtmlEntry(int entry)
{
	LOG_MSG("In GdaFrame::OnHtmlEntry");
	LOG(entry);
	LOG(htmlMenuItems[entry].menu_title);
	LOG(htmlMenuItems[entry].url);
	if (!GetProject()) return;
	WebViewExampleDlg* dlg =
	new WebViewExampleDlg(project_p,
						  htmlMenuItems[entry].url,
						  this, wxID_ANY,
						  htmlMenuItems[entry].menu_title);
	dlg->Show(true);
}

void GdaFrame::OnHtmlEntry0(wxCommandEvent& event) { OnHtmlEntry(0); }
void GdaFrame::OnHtmlEntry1(wxCommandEvent& event) { OnHtmlEntry(1); }
void GdaFrame::OnHtmlEntry2(wxCommandEvent& event) { OnHtmlEntry(2); }
void GdaFrame::OnHtmlEntry3(wxCommandEvent& event) { OnHtmlEntry(3); }
void GdaFrame::OnHtmlEntry4(wxCommandEvent& event) { OnHtmlEntry(4); }
void GdaFrame::OnHtmlEntry5(wxCommandEvent& event) { OnHtmlEntry(5); }
void GdaFrame::OnHtmlEntry6(wxCommandEvent& event) { OnHtmlEntry(6); }
void GdaFrame::OnHtmlEntry7(wxCommandEvent& event) { OnHtmlEntry(7); }
void GdaFrame::OnHtmlEntry8(wxCommandEvent& event) { OnHtmlEntry(8); }
void GdaFrame::OnHtmlEntry9(wxCommandEvent& event) { OnHtmlEntry(9); }

void GdaFrame::OnGeneratePointShpFile(wxCommandEvent& event)
{
	if (!GetProject() || !GetProject()->GetTableInt()) return;
	Project* p = GetProject();
	TableInterface* table_int = p->GetTableInt();
	VariableSettingsDlg VS(GetProject(), VariableSettingsDlg::bivariate, false, false, "New Map Coordinates","First Variable (X/Longitude)", "Second Variable (Y/Latitue)");
	if (VS.ShowModal() != wxID_OK) return;
	
	std::vector<double> xs;
	std::vector<double> ys;
	table_int->GetColData(VS.col_ids[0], VS.var_info[0].time, xs);
	table_int->GetColData(VS.col_ids[1], VS.var_info[1].time, ys);

    int n_rows = xs.size();
    
    // clean p->main_data
    if (!p->main_data.records.empty()) {
        p->main_data.records.clear();
    }
    p->main_data.header.shape_type = Shapefile::POINT_TYP;
    p->main_data.records.resize(n_rows);
    
	double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
    
    for ( int i=0; i < n_rows; i++ ) {
        if ( i==0 ) {
            min_x = max_x = xs[i];
            min_y = max_y = ys[i];
        }
        if ( xs[i] < min_x ) min_x = xs[i];
        else if ( xs[i] > max_x ) max_x = xs[i];
        if ( ys[i] < min_y ) min_y = ys[i];
        else if ( ys[i] > max_y ) max_y = ys[i];
        
        Shapefile::PointContents* pc = new Shapefile::PointContents();
        pc->shape_type = Shapefile::POINT_TYP;
        pc->x = xs[i];
        pc->y = ys[i];
        p->main_data.records[i].contents_p = pc;
    }
   
	p->main_data.header.bbox_x_min = min_x;
	p->main_data.header.bbox_y_min = min_y;
	p->main_data.header.bbox_x_max = max_x;
	p->main_data.header.bbox_y_max = max_y;
	p->main_data.header.bbox_z_min = 0;
	p->main_data.header.bbox_z_max = 0;
	p->main_data.header.bbox_m_min = 0;
	p->main_data.header.bbox_m_max = 0;
    
	//LOG(p->main_data.records.size());
    p->isTableOnly = false;
	UpdateToolbarAndMenus();
	std::vector<int> col_ids;
	std::vector<GdaVarTools::VarInfo> var_info;
	MapFrame* nf = new MapFrame(this, p, var_info, col_ids,
									  CatClassification::no_theme,
									  MapCanvas::no_smoothing, 1,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnRegressionClassic(wxCommandEvent& event)
{
    
    Project* p = GetProject();
    if (p) {
        RegressionDlg* dlg = new RegressionDlg(project_p, this);
        dlg->Show(true);
    }
}

void GdaFrame::OnPublish(wxCommandEvent& event)
{
    
	Project* p = GetProject();
    if (p) {
        PublishDlg dlg(this,p);
        dlg.ShowModal();
        
    }
}

void GdaFrame::DisplayRegression(const wxString dump)
{
    Project* p = GetProject();
    if (!p) return;
    
	RegressionReportDlg *regReportDlg = new RegressionReportDlg(this, dump);
	regReportDlg->Show(true);
	regReportDlg->m_textbox->SetSelection(0, 0);
}

void GdaFrame::OnCondPlotChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	LOG_MSG("Entering GdaFrame::OnCondPlotChoices");
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
	LOG_MSG("Exiting GdaFrame::OnCondPlotChoices");
}

void GdaFrame::OnShowConditionalMapView(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
													false, "Conditional Map Variables",
													"Horizontal Cells", "Vertical Cells", "Map Theme");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalMapFrame* subframe =
	new ConditionalMapFrame(GdaFrame::gda_frame, project_p,
							dlg.var_info, dlg.col_ids,
							"Conditional Map", wxDefaultPosition,
							GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowConditionalHistView(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, false,
													false,
							"Conditional Histogram Variables",
							"Horizontal Cells", "Vertical Cells",
							"Histogram Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalHistogramFrame* subframe =
	new ConditionalHistogramFrame(GdaFrame::gda_frame, project_p,
								  dlg.var_info, dlg.col_ids,
								  "Conditional Histogram", wxDefaultPosition,
								  GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowConditionalScatterView(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate,
							false, false,
							"Conditional Scatter Plot Variables",
							"Horizontal Cells", "Vertical Cells",
							"Independent Var (x-axis)",
							"Dependent Var (y-axis)");
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalScatterPlotFrame* subframe =
	new ConditionalScatterPlotFrame(GdaFrame::gda_frame, project_p,
									dlg.var_info, dlg.col_ids,
									"Conditional Scatter Plot",
									wxDefaultPosition,
									GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowCartogramNewView(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
													false, "Cartogram Variables",
							"Circle Size", "Circle Color", "", "",
							true, false); // set second var from first
	if (dlg.ShowModal() != wxID_OK) return;
	
	CartogramNewFrame* subframe =
		new CartogramNewFrame(GdaFrame::gda_frame, project_p,
							  dlg.var_info, dlg.col_ids,
							  "Cartogram", wxDefaultPosition,
							  GdaConst::map_default_size);
}

void GdaFrame::OnCartogramImprove1(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(0);
	}
}

void GdaFrame::OnCartogramImprove2(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(1);
	}
}

void GdaFrame::OnCartogramImprove3(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(2);
	}
}

void GdaFrame::OnCartogramImprove4(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(3);
	}
}

void GdaFrame::OnCartogramImprove5(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(4);
	}
}

void GdaFrame::OnCartogramImprove6(wxCommandEvent& event )
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->CartogramImproveLevel(5);
	}
}

void GdaFrame::OnExploreHist(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering GdaFrame::OnExploreHist");
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate);
	if (VS.ShowModal() != wxID_OK) return;
	
	HistogramFrame* f = new HistogramFrame(GdaFrame::gda_frame, project_p,
										   VS.var_info,
										   VS.col_ids, "Histogram",
										   wxDefaultPosition,
										   GdaConst::hist_default_size);
	
	LOG_MSG("Exiting GdaFrame::OnExploreHist");
}

void GdaFrame::OnExploreScatterNewPlot(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
													false, "Scatter Plot Variables",
							"Independent Var X", "Dependent Var Y");
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Scatter Plot");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(GdaFrame::gda_frame, project_p,
							dlg.var_info, dlg.col_ids,
							false, title, wxDefaultPosition,
							GdaConst::scatterplot_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void GdaFrame::OnExploreBubbleChart(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate,
							false, false,
							"Bubble Chart Variables",
							"X-Axis", "Y-Axis",
							"Bubble Size", "Standard Deviation Color",
							false, true); // set fourth variable from third
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxString title("Bubble Chart");
	ScatterNewPlotFrame* subframe =
	new ScatterNewPlotFrame(GdaFrame::gda_frame, project_p,
							dlg.var_info, dlg.col_ids,
							true, title, wxDefaultPosition,
							GdaConst::bubble_chart_default_size,
							wxDEFAULT_FRAME_STYLE);
}

void GdaFrame::OnExploreScatterPlotMat(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	ScatterPlotMatFrame* f =
		new ScatterPlotMatFrame(GdaFrame::gda_frame, project_p,
														"Scatter Plot Matrix", wxDefaultPosition,
														GdaConst::scatterplot_default_size);
}

void GdaFrame::OnExploreTestMap(wxCommandEvent& WXUNUSED(event))
{
	LOG_MSG("Entering GdaFrame::OnExploreTestMap");
	
	//MapFrame* subframe = new MapFrame(frame, project_p,
	//										MapCanvas::no_theme,
	//										MapCanvas::no_smoothing, 1,
	//                                      boost::uuids::nil_uuid(),
	//										wxDefaultPosition,
	//										GdaConst::map_default_size);
	//subframe->UpdateTitle();
	
	//	TestMapFrame *subframe = new TestMapFrame(frame, project_p,
	//											  "Test Map Frame",
	//											  wxDefaultPosition,
	//											  GdaConst::map_default_size,
	//											  wxDEFAULT_FRAME_STYLE);
	LOG_MSG("Exiting GdaFrame::OnExploreTestMap");
}

void GdaFrame::OnExploreNewBox(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate);
	if (VS.ShowModal() != wxID_OK) return;
	
	wxSize size = GdaConst::boxplot_default_size;
	int w = size.GetWidth();
	if (VS.var_info[0].is_time_variant) size.SetWidth((w*3)/2);
	else size.SetWidth(w/2);
	BoxPlotFrame *sf = new BoxPlotFrame(GdaFrame::gda_frame, GetProject(),
											  VS.var_info, VS.col_ids,
											  "Box Plot", wxDefaultPosition,
											  size);
}

void GdaFrame::OnExplorePCP(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	PCPDlg dlg(p,this);
	if (dlg.ShowModal() != wxID_OK) return;
	PCPFrame* s = new PCPFrame(this, p, dlg.var_info, dlg.col_ids);
}

void GdaFrame::OnExplore3DP(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(p, VariableSettingsDlg::trivariate, false, false, "3D Scatter Plot Variables", "X", "Y", "Z");
	if (dlg.ShowModal() != wxID_OK) return;
		
	C3DPlotFrame *subframe =
		new C3DPlotFrame(GdaFrame::gda_frame, p,
						 dlg.var_info, dlg.col_ids,
						 "3D Plot", wxDefaultPosition,
						 GdaConst::three_d_default_size,
						 wxDEFAULT_FRAME_STYLE);
}

void GdaFrame::OnExploreLineChart(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;

    bool hide_time = true;
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false, false,
                           "Variable Selection",
                           "", "", "", "", false, false, hide_time);
	if (VS.ShowModal() != wxID_OK) return;
    
	LineChartFrame* f = new LineChartFrame(GdaFrame::gda_frame, project_p, VS.var_info, VS.col_ids, "Average Comparison Chart", wxDefaultPosition, GdaConst::line_chart_default_size);
}

void GdaFrame::OnExploreCovScatterPlot(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, false, true, "Variable Choice", "Variable");
	if (VS.ShowModal() != wxID_OK) return;
	GdaVarTools::VarInfo& v = VS.var_info[0];
	std::vector<wxString> tm_strs;
	project_p->GetTableInt()->GetTimeStrings(tm_strs);
	GdaVarTools::Manager var_man(tm_strs);
	var_man.AppendVar(v.name, v.min, v.max, v.time,
										v.sync_with_global_time, v.fixed_scale);
	CovSpFrame* f = new CovSpFrame(GdaFrame::gda_frame, project_p, var_man,
																 VS.GetDistanceMetric(),
																 VS.GetDistanceUnits());
}

void GdaFrame::OnExploreCorrelogram(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
    CorrelParamsFrame dlg(project_p);
    if (dlg.ShowModal() != wxID_OK) return;
    
    CorrelogramFrame* f = new CorrelogramFrame(GdaFrame::gda_frame, project_p, dlg.correl_params, dlg.var_man, "Correlogram", wxDefaultPosition, GdaConst::scatterplot_default_size);
}

void GdaFrame::OnToolOpenNewTable(wxCommandEvent& WXUNUSED(event))
{
	OnOpenNewTable();
}

void GdaFrame::OnMoranMenuChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	LOG_MSG("Entering GdaFrame::OnMoranMenuChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_MORAN_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting GdaFrame::OnMoranMenuChoices");
}

void GdaFrame::OnOpenMSPL(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
   
    Project* project = GetProject();
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gw = w_man_int->GetGal(w_id);
    if (gw == NULL) return;
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::univariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
													   project_p, lc);
}
void GdaFrame::OnOpenDiffMoran(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
    Project* project = GetProject();
    TableInterface* table_int = project->GetTableInt();
    
    bool has_time = table_int->IsTimeVariant();
    if (has_time == false) {
        wxMessageDialog dlg (this, "Differential Moran's I tests whether the change in a variable over time is spatially correlated.\n\n Please first group the a variable by time period: Select Time --> Time Editor.", "Warning", wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, "No Weights Found:\n\n This feature requires weights, but none defined.\n Please use Tools > Weights > Weights Manager\n to define weights.", "Warning", wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
        
    }
    
    DiffMoranVarSettingDlg VS(project_p);
    if (VS.ShowModal() != wxID_OK) {
        return;
    }
    
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil())
        return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL)
        return;
    
    LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
                                              VS.var_info, VS.col_ids,
                                              LisaCoordinator::differential,
                                              false);
    
    LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
                                                       project_p, lc);

}

void GdaFrame::OnOpenGMoran(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
    bool show_weights = true;
    bool show_distance = false;
    wxString title = "Differential Moran Variable Settings";
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, show_weights, show_distance, title);
	if (VS.ShowModal() != wxID_OK)
        return;
    
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil())
        return;
	
    Project* project = GetProject();
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL)
        return;
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info, VS.col_ids,
                                              LisaCoordinator::differential,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
													   project_p, lc);
}

void GdaFrame::OnOpenMoranEB(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true,
												 false,
												 "Empirical Bayes Rate Standardization Variables",
												 "Event Variable", "Base Variable");
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
	
    Project* project = GetProject();
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) return;
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
										VS.var_info, VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
										false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
													   project_p, lc);
}

void GdaFrame::OnLisaMenuChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	LOG_MSG("Entering GdaFrame::OnLisaMenuChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_LISA_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting GdaFrame::OnLisaMenuChoices");
}

void GdaFrame::OnGetisMenuChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	LOG_MSG("Entering GdaFrame::OnGetisMenuChoices");
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_GETIS_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
	LOG_MSG("Exiting GdaFrame::OnGetisMenuChoices");
}

void GdaFrame::OnOpenUniLisa(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true,
												 false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
		
    Project* project = GetProject();
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) return;
    
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap && !LWO.m_Moran) return;
	
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::univariate,
											  true, LWO.m_RowStand);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(GdaFrame::gda_frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, true, false, false);
	}
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, false, false, false,
												  wxDefaultPosition);
	}
}

void GdaFrame::OnOpenMultiLisa(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	//VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true, false);
    
    Project* project = GetProject();
    TableInterface* table_int = project->GetTableInt();
    
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, "No Weights Found:\n\n This feature requires weights, but none defined.\n Please use Tools > Weights > Weights Manager\n to define weights.", "Warning", wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
        
    }
    
    bool has_time = table_int->IsTimeVariant();
    if (has_time == false) {
        wxMessageDialog dlg (this, "Please define time first. \n\n Note: Goto menu: Time->Time Editor.", "Warning", wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    DiffMoranVarSettingDlg VS(project_p);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;

	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap &&!LWO.m_Moran) return;
	
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) return;
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::differential,
											  true, LWO.m_RowStand);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(GdaFrame::gda_frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, true, false, false);
	}
	
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, false, false, false);
	}
}

void GdaFrame::OnOpenLisaEB(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	// Note: this is the only call to this particular constructor
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true,
												 false, "Rates Variable Settings",
												 "Event Variable", "Base Variable");
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
	
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) return;
	
    Project* project = GetProject();
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) return;
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info,
											  VS.col_ids,
										LisaCoordinator::eb_rate_standardized,
											  true, LWO.m_RowStand);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(GdaFrame::gda_frame,
															project_p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, true, false, true);
	}
	
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
												  lc, false, false, true);
	}	
}

void GdaFrame::OnOpenGetisOrdStar(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
   
	GetisWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
    
    
	GStatCoordinator* gc = new GStatCoordinator(w_id, project_p, VS.var_info, VS.col_ids, LWO.m_RowStand);
	if (!gc || !gc->IsOk()) {
		// print error message
		delete gc;
		return;
	}
    
	if (LWO.m_NormMap && LWO.m_ClustMap) {
		GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::GiStar_clus_norm, LWO.m_RowStand);
	}
	if (LWO.m_NormMap && LWO.m_SigMap) {
		GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::GiStar_sig_norm, LWO.m_RowStand);
	}
	if (!LWO.m_NormMap && LWO.m_ClustMap) {
		GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::GiStar_clus_perm, LWO.m_RowStand);
	}
	if (!LWO.m_NormMap && LWO.m_SigMap) {
		GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,GetisOrdMapFrame::GiStar_sig_perm, LWO.m_RowStand);
	}
}

void GdaFrame::OnOpenGetisOrd(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
		
	GetisWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
    
	//GetisOrdChoiceDlg dlg(this);
	//if (dlg.ShowModal() != wxID_OK) return;
	//if (!dlg.Gi_ClustMap_norm && !dlg.Gi_SigMap_norm &&
	//	!dlg.GiStar_ClustMap_norm && !dlg.GiStar_SigMap_norm &&
	//	!dlg.Gi_ClustMap_perm && !dlg.Gi_SigMap_perm &&
	//	!dlg.GiStar_ClustMap_perm && !dlg.GiStar_SigMap_perm) return;

	GStatCoordinator* gc = new GStatCoordinator(w_id, project_p, VS.var_info, VS.col_ids, LWO.m_RowStand);
    
	if (!gc || !gc->IsOk()) {
		// print error message
		delete gc;
		return;
	}
    
    if (LWO.m_NormMap && LWO.m_ClustMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::Gi_clus_norm, LWO.m_RowStand);
    }
    if (LWO.m_NormMap && LWO.m_SigMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::Gi_sig_norm, LWO.m_RowStand);
    }
    if (!LWO.m_NormMap && LWO.m_ClustMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc, GetisOrdMapFrame::Gi_clus_perm, LWO.m_RowStand);
    }
    if (!LWO.m_NormMap && LWO.m_SigMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,GetisOrdMapFrame::Gi_sig_perm, LWO.m_RowStand);
    }
}

void GdaFrame::OnNewCustomCatClassifA(wxCommandEvent& event)
{
    Project* p = GetProject();
    if (!p) return;
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnNewCustomCatClassifA();
	}
}

void GdaFrame::OnNewCustomCatClassifB(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifB();
	}
}

void GdaFrame::OnNewCustomCatClassifC(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifC();
	}
}

void GdaFrame::OnCCClassifA(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);

	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnCustomCatClassifA(titles[cc_menu_num]);
	}
}

void GdaFrame::OnCCClassifB(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifB(titles[cc_menu_num]);
	}	
}

void GdaFrame::OnCCClassifC(int cc_menu_num)
{
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;
	LOG_MSG(titles[cc_menu_num]);
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifC(titles[cc_menu_num]);
	}	
}

void GdaFrame::OnCCClassifA0(wxCommandEvent& e) { OnCCClassifA(0); }
void GdaFrame::OnCCClassifA1(wxCommandEvent& e) { OnCCClassifA(1); }
void GdaFrame::OnCCClassifA2(wxCommandEvent& e) { OnCCClassifA(2); }
void GdaFrame::OnCCClassifA3(wxCommandEvent& e) { OnCCClassifA(3); }
void GdaFrame::OnCCClassifA4(wxCommandEvent& e) { OnCCClassifA(4); }
void GdaFrame::OnCCClassifA5(wxCommandEvent& e) { OnCCClassifA(5); }
void GdaFrame::OnCCClassifA6(wxCommandEvent& e) { OnCCClassifA(6); }
void GdaFrame::OnCCClassifA7(wxCommandEvent& e) { OnCCClassifA(7); }
void GdaFrame::OnCCClassifA8(wxCommandEvent& e) { OnCCClassifA(8); }
void GdaFrame::OnCCClassifA9(wxCommandEvent& e) { OnCCClassifA(9); }
void GdaFrame::OnCCClassifA10(wxCommandEvent& e) { OnCCClassifA(10); }
void GdaFrame::OnCCClassifA11(wxCommandEvent& e) { OnCCClassifA(11); }
void GdaFrame::OnCCClassifA12(wxCommandEvent& e) { OnCCClassifA(12); }
void GdaFrame::OnCCClassifA13(wxCommandEvent& e) { OnCCClassifA(13); }
void GdaFrame::OnCCClassifA14(wxCommandEvent& e) { OnCCClassifA(14); }
void GdaFrame::OnCCClassifA15(wxCommandEvent& e) { OnCCClassifA(15); }
void GdaFrame::OnCCClassifA16(wxCommandEvent& e) { OnCCClassifA(16); }
void GdaFrame::OnCCClassifA17(wxCommandEvent& e) { OnCCClassifA(17); }
void GdaFrame::OnCCClassifA18(wxCommandEvent& e) { OnCCClassifA(18); }
void GdaFrame::OnCCClassifA19(wxCommandEvent& e) { OnCCClassifA(19); }
void GdaFrame::OnCCClassifA20(wxCommandEvent& e) { OnCCClassifA(20); }
void GdaFrame::OnCCClassifA21(wxCommandEvent& e) { OnCCClassifA(21); }
void GdaFrame::OnCCClassifA22(wxCommandEvent& e) { OnCCClassifA(22); }
void GdaFrame::OnCCClassifA23(wxCommandEvent& e) { OnCCClassifA(23); }
void GdaFrame::OnCCClassifA24(wxCommandEvent& e) { OnCCClassifA(24); }
void GdaFrame::OnCCClassifA25(wxCommandEvent& e) { OnCCClassifA(25); }
void GdaFrame::OnCCClassifA26(wxCommandEvent& e) { OnCCClassifA(26); }
void GdaFrame::OnCCClassifA27(wxCommandEvent& e) { OnCCClassifA(27); }
void GdaFrame::OnCCClassifA28(wxCommandEvent& e) { OnCCClassifA(28); }
void GdaFrame::OnCCClassifA29(wxCommandEvent& e) { OnCCClassifA(29); }

void GdaFrame::OnCCClassifB0(wxCommandEvent& e) { OnCCClassifB(0); }
void GdaFrame::OnCCClassifB1(wxCommandEvent& e) { OnCCClassifB(1); }
void GdaFrame::OnCCClassifB2(wxCommandEvent& e) { OnCCClassifB(2); }
void GdaFrame::OnCCClassifB3(wxCommandEvent& e) { OnCCClassifB(3); }
void GdaFrame::OnCCClassifB4(wxCommandEvent& e) { OnCCClassifB(4); }
void GdaFrame::OnCCClassifB5(wxCommandEvent& e) { OnCCClassifB(5); }
void GdaFrame::OnCCClassifB6(wxCommandEvent& e) { OnCCClassifB(6); }
void GdaFrame::OnCCClassifB7(wxCommandEvent& e) { OnCCClassifB(7); }
void GdaFrame::OnCCClassifB8(wxCommandEvent& e) { OnCCClassifB(8); }
void GdaFrame::OnCCClassifB9(wxCommandEvent& e) { OnCCClassifB(9); }
void GdaFrame::OnCCClassifB10(wxCommandEvent& e) { OnCCClassifB(10); }
void GdaFrame::OnCCClassifB11(wxCommandEvent& e) { OnCCClassifB(11); }
void GdaFrame::OnCCClassifB12(wxCommandEvent& e) { OnCCClassifB(12); }
void GdaFrame::OnCCClassifB13(wxCommandEvent& e) { OnCCClassifB(13); }
void GdaFrame::OnCCClassifB14(wxCommandEvent& e) { OnCCClassifB(14); }
void GdaFrame::OnCCClassifB15(wxCommandEvent& e) { OnCCClassifB(15); }
void GdaFrame::OnCCClassifB16(wxCommandEvent& e) { OnCCClassifB(16); }
void GdaFrame::OnCCClassifB17(wxCommandEvent& e) { OnCCClassifB(17); }
void GdaFrame::OnCCClassifB18(wxCommandEvent& e) { OnCCClassifB(18); }
void GdaFrame::OnCCClassifB19(wxCommandEvent& e) { OnCCClassifB(19); }
void GdaFrame::OnCCClassifB20(wxCommandEvent& e) { OnCCClassifB(20); }
void GdaFrame::OnCCClassifB21(wxCommandEvent& e) { OnCCClassifB(21); }
void GdaFrame::OnCCClassifB22(wxCommandEvent& e) { OnCCClassifB(22); }
void GdaFrame::OnCCClassifB23(wxCommandEvent& e) { OnCCClassifB(23); }
void GdaFrame::OnCCClassifB24(wxCommandEvent& e) { OnCCClassifB(24); }
void GdaFrame::OnCCClassifB25(wxCommandEvent& e) { OnCCClassifB(25); }
void GdaFrame::OnCCClassifB26(wxCommandEvent& e) { OnCCClassifB(26); }
void GdaFrame::OnCCClassifB27(wxCommandEvent& e) { OnCCClassifB(27); }
void GdaFrame::OnCCClassifB28(wxCommandEvent& e) { OnCCClassifB(28); }
void GdaFrame::OnCCClassifB29(wxCommandEvent& e) { OnCCClassifB(29); }

void GdaFrame::OnCCClassifC0(wxCommandEvent& e) { OnCCClassifC(0); }
void GdaFrame::OnCCClassifC1(wxCommandEvent& e) { OnCCClassifC(1); }
void GdaFrame::OnCCClassifC2(wxCommandEvent& e) { OnCCClassifC(2); }
void GdaFrame::OnCCClassifC3(wxCommandEvent& e) { OnCCClassifC(3); }
void GdaFrame::OnCCClassifC4(wxCommandEvent& e) { OnCCClassifC(4); }
void GdaFrame::OnCCClassifC5(wxCommandEvent& e) { OnCCClassifC(5); }
void GdaFrame::OnCCClassifC6(wxCommandEvent& e) { OnCCClassifC(6); }
void GdaFrame::OnCCClassifC7(wxCommandEvent& e) { OnCCClassifC(7); }
void GdaFrame::OnCCClassifC8(wxCommandEvent& e) { OnCCClassifC(8); }
void GdaFrame::OnCCClassifC9(wxCommandEvent& e) { OnCCClassifC(9); }
void GdaFrame::OnCCClassifC10(wxCommandEvent& e) { OnCCClassifC(10); }
void GdaFrame::OnCCClassifC11(wxCommandEvent& e) { OnCCClassifC(11); }
void GdaFrame::OnCCClassifC12(wxCommandEvent& e) { OnCCClassifC(12); }
void GdaFrame::OnCCClassifC13(wxCommandEvent& e) { OnCCClassifC(13); }
void GdaFrame::OnCCClassifC14(wxCommandEvent& e) { OnCCClassifC(14); }
void GdaFrame::OnCCClassifC15(wxCommandEvent& e) { OnCCClassifC(15); }
void GdaFrame::OnCCClassifC16(wxCommandEvent& e) { OnCCClassifC(16); }
void GdaFrame::OnCCClassifC17(wxCommandEvent& e) { OnCCClassifC(17); }
void GdaFrame::OnCCClassifC18(wxCommandEvent& e) { OnCCClassifC(18); }
void GdaFrame::OnCCClassifC19(wxCommandEvent& e) { OnCCClassifC(19); }
void GdaFrame::OnCCClassifC20(wxCommandEvent& e) { OnCCClassifC(20); }
void GdaFrame::OnCCClassifC21(wxCommandEvent& e) { OnCCClassifC(21); }
void GdaFrame::OnCCClassifC22(wxCommandEvent& e) { OnCCClassifC(22); }
void GdaFrame::OnCCClassifC23(wxCommandEvent& e) { OnCCClassifC(23); }
void GdaFrame::OnCCClassifC24(wxCommandEvent& e) { OnCCClassifC(24); }
void GdaFrame::OnCCClassifC25(wxCommandEvent& e) { OnCCClassifC(25); }
void GdaFrame::OnCCClassifC26(wxCommandEvent& e) { OnCCClassifC(26); }
void GdaFrame::OnCCClassifC27(wxCommandEvent& e) { OnCCClassifC(27); }
void GdaFrame::OnCCClassifC28(wxCommandEvent& e) { OnCCClassifC(28); }
void GdaFrame::OnCCClassifC29(wxCommandEvent& e) { OnCCClassifC(29); }

void GdaFrame::OnOpenThemelessMap(wxCommandEvent& event)
{
	std::vector<int> col_ids;
	std::vector<GdaVarTools::VarInfo> var_info;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  var_info, col_ids,
									  CatClassification::no_theme,
									  MapCanvas::no_smoothing, 1,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnThemelessMap();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnThemeless();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnThemeless();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnThemeless();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnThemeless();
	}
}

void GdaFrame::OnOpenQuantile1(wxCommandEvent& e) { OpenQuantile(1); }
void GdaFrame::OnOpenQuantile2(wxCommandEvent& e) { OpenQuantile(2); }
void GdaFrame::OnOpenQuantile3(wxCommandEvent& e) { OpenQuantile(3); }
void GdaFrame::OnOpenQuantile4(wxCommandEvent& e) { OpenQuantile(4); }
void GdaFrame::OnOpenQuantile5(wxCommandEvent& e) { OpenQuantile(5); }
void GdaFrame::OnOpenQuantile6(wxCommandEvent& e) { OpenQuantile(6); }
void GdaFrame::OnOpenQuantile7(wxCommandEvent& e) { OpenQuantile(7); }
void GdaFrame::OnOpenQuantile8(wxCommandEvent& e) { OpenQuantile(8); }
void GdaFrame::OnOpenQuantile9(wxCommandEvent& e) { OpenQuantile(9); }
void GdaFrame::OnOpenQuantile10(wxCommandEvent& e) { OpenQuantile(10); }

void GdaFrame::OpenQuantile(int num_cats)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::quantile,
									  MapCanvas::no_smoothing, num_cats,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnQuantile1(wxCommandEvent& e) { ChangeToQuantile(1); }
void GdaFrame::OnQuantile2(wxCommandEvent& e) { ChangeToQuantile(2); }
void GdaFrame::OnQuantile3(wxCommandEvent& e) { ChangeToQuantile(3); }
void GdaFrame::OnQuantile4(wxCommandEvent& e) { ChangeToQuantile(4); }
void GdaFrame::OnQuantile5(wxCommandEvent& e) { ChangeToQuantile(5); }
void GdaFrame::OnQuantile6(wxCommandEvent& e) { ChangeToQuantile(6); }
void GdaFrame::OnQuantile7(wxCommandEvent& e) { ChangeToQuantile(7); }
void GdaFrame::OnQuantile8(wxCommandEvent& e) { ChangeToQuantile(8); }
void GdaFrame::OnQuantile9(wxCommandEvent& e) { ChangeToQuantile(9); }
void GdaFrame::OnQuantile10(wxCommandEvent& e) { ChangeToQuantile(10); }

void GdaFrame::ChangeToQuantile(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	wxCommandEvent event;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnQuantile(num_cats);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnQuantile(num_cats);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnQuantile(num_cats);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnQuantile(num_cats);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnQuantile(num_cats);
	}
}

void GdaFrame::OnOpenPercentile(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::percentile,
									  MapCanvas::no_smoothing, 6,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnPercentile();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnPercentile();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnPercentile();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnPercentile();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnPercentile();
	}
}

void GdaFrame::OnOpenHinge15(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::hinge_15,
									  MapCanvas::no_smoothing, 6,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnHinge15(event);
	} else if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnHinge15();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge15();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnHinge15();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnHinge15();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnHinge15();
	}
}

void GdaFrame::OnOpenHinge30(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::hinge_30,
									  MapCanvas::no_smoothing, 6,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnHinge30(event);
	} else if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnHinge30();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnHinge30();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnHinge30();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnHinge30();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnHinge30();
	}
}

void GdaFrame::OnOpenStddev(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::stddev,
									  MapCanvas::no_smoothing, 6,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnStdDevMap();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnStdDevMap();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnStdDevMap();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnStdDevMap();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnStdDevMap();
	}
}

void GdaFrame::OnOpenNaturalBreaks1(wxCommandEvent& e) { OpenNaturalBreaks(1); }
void GdaFrame::OnOpenNaturalBreaks2(wxCommandEvent& e) { OpenNaturalBreaks(2); }
void GdaFrame::OnOpenNaturalBreaks3(wxCommandEvent& e) { OpenNaturalBreaks(3); }
void GdaFrame::OnOpenNaturalBreaks4(wxCommandEvent& e) { OpenNaturalBreaks(4); }
void GdaFrame::OnOpenNaturalBreaks5(wxCommandEvent& e) { OpenNaturalBreaks(5); }
void GdaFrame::OnOpenNaturalBreaks6(wxCommandEvent& e) { OpenNaturalBreaks(6); }
void GdaFrame::OnOpenNaturalBreaks7(wxCommandEvent& e) { OpenNaturalBreaks(7); }
void GdaFrame::OnOpenNaturalBreaks8(wxCommandEvent& e) { OpenNaturalBreaks(8); }
void GdaFrame::OnOpenNaturalBreaks9(wxCommandEvent& e) { OpenNaturalBreaks(9); }
void GdaFrame::OnOpenNaturalBreaks10(wxCommandEvent& e) {OpenNaturalBreaks(10);}

void GdaFrame::OpenNaturalBreaks(int num_cats)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::natural_breaks,
									  MapCanvas::no_smoothing, num_cats,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnNaturalBreaks1(wxCommandEvent& e) { ChangeToNaturalBreaks(1); }
void GdaFrame::OnNaturalBreaks2(wxCommandEvent& e) { ChangeToNaturalBreaks(2); }
void GdaFrame::OnNaturalBreaks3(wxCommandEvent& e) { ChangeToNaturalBreaks(3); }
void GdaFrame::OnNaturalBreaks4(wxCommandEvent& e) { ChangeToNaturalBreaks(4); }
void GdaFrame::OnNaturalBreaks5(wxCommandEvent& e) { ChangeToNaturalBreaks(5); }
void GdaFrame::OnNaturalBreaks6(wxCommandEvent& e) { ChangeToNaturalBreaks(6); }
void GdaFrame::OnNaturalBreaks7(wxCommandEvent& e) { ChangeToNaturalBreaks(7); }
void GdaFrame::OnNaturalBreaks8(wxCommandEvent& e) { ChangeToNaturalBreaks(8); }
void GdaFrame::OnNaturalBreaks9(wxCommandEvent& e) { ChangeToNaturalBreaks(9); }
void GdaFrame::OnNaturalBreaks10(wxCommandEvent& e) {ChangeToNaturalBreaks(10);}

void GdaFrame::ChangeToNaturalBreaks(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	wxCommandEvent event;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnNaturalBreaks(num_cats);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnNaturalBreaks(num_cats);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnNaturalBreaks(num_cats);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnNaturalBreaks(num_cats);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnNaturalBreaks(num_cats);
	}
}

void GdaFrame::OnOpenEqualIntervals1(wxCommandEvent& e)
{ OpenEqualIntervals(1); }
void GdaFrame::OnOpenEqualIntervals2(wxCommandEvent& e)
{ OpenEqualIntervals(2); }
void GdaFrame::OnOpenEqualIntervals3(wxCommandEvent& e)
{ OpenEqualIntervals(3); }
void GdaFrame::OnOpenEqualIntervals4(wxCommandEvent& e)
{ OpenEqualIntervals(4); }
void GdaFrame::OnOpenEqualIntervals5(wxCommandEvent& e)
{ OpenEqualIntervals(5); }
void GdaFrame::OnOpenEqualIntervals6(wxCommandEvent& e)
{ OpenEqualIntervals(6); }
void GdaFrame::OnOpenEqualIntervals7(wxCommandEvent& e)
{ OpenEqualIntervals(7); }
void GdaFrame::OnOpenEqualIntervals8(wxCommandEvent& e)
{ OpenEqualIntervals(8); }
void GdaFrame::OnOpenEqualIntervals9(wxCommandEvent& e)
{ OpenEqualIntervals(9); }
void GdaFrame::OnOpenEqualIntervals10(wxCommandEvent& e)
{ OpenEqualIntervals(10); }

void GdaFrame::OpenEqualIntervals(int num_cats)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::equal_intervals,
									  MapCanvas::no_smoothing, num_cats,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();	
}

void GdaFrame::OnEqualIntervals1(wxCommandEvent& e)
{ ChangeToEqualIntervals(1); }
void GdaFrame::OnEqualIntervals2(wxCommandEvent& e)
{ ChangeToEqualIntervals(2); }
void GdaFrame::OnEqualIntervals3(wxCommandEvent& e)
{ ChangeToEqualIntervals(3); }
void GdaFrame::OnEqualIntervals4(wxCommandEvent& e)
{ ChangeToEqualIntervals(4); }
void GdaFrame::OnEqualIntervals5(wxCommandEvent& e)
{ ChangeToEqualIntervals(5); }
void GdaFrame::OnEqualIntervals6(wxCommandEvent& e)
{ ChangeToEqualIntervals(6); }
void GdaFrame::OnEqualIntervals7(wxCommandEvent& e)
{ ChangeToEqualIntervals(7); }
void GdaFrame::OnEqualIntervals8(wxCommandEvent& e)
{ ChangeToEqualIntervals(8); }
void GdaFrame::OnEqualIntervals9(wxCommandEvent& e)
{ ChangeToEqualIntervals(9); }
void GdaFrame::OnEqualIntervals10(wxCommandEvent& e)
{ ChangeToEqualIntervals(10); }

void GdaFrame::ChangeToEqualIntervals(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	wxCommandEvent event;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnEqualIntervals(num_cats);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnEqualIntervals(num_cats);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnEqualIntervals(num_cats);
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnEqualIntervals(num_cats);
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnEqualIntervals(num_cats);
	}
}

void GdaFrame::OnOpenUniqueValues(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
                                  dlg.var_info, dlg.col_ids,
                                  CatClassification::unique_values,
                                  MapCanvas::no_smoothing, 4,
                                  boost::uuids::nil_uuid(),
                                  wxDefaultPosition,
                                  GdaConst::map_default_size);
    nf->UpdateTitle();
}

void GdaFrame::OnUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnUniqueValues();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnUniqueValues();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnUniqueValues();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnUniqueValues();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnUniqueValues();
	}
}


void GdaFrame::OnCondVertThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::no_theme, 1);
	}
}

void GdaFrame::OnCondVertQuant1(wxCommandEvent& e)
{ ChangeToCondVertQuant(1); }
void GdaFrame::OnCondVertQuant2(wxCommandEvent& e)
{ ChangeToCondVertQuant(2); }
void GdaFrame::OnCondVertQuant3(wxCommandEvent& e)
{ ChangeToCondVertQuant(3); }
void GdaFrame::OnCondVertQuant4(wxCommandEvent& e)
{ ChangeToCondVertQuant(4); }
void GdaFrame::OnCondVertQuant5(wxCommandEvent& e)
{ ChangeToCondVertQuant(5); }
void GdaFrame::OnCondVertQuant6(wxCommandEvent& e)
{ ChangeToCondVertQuant(6); }
void GdaFrame::OnCondVertQuant7(wxCommandEvent& e)
{ ChangeToCondVertQuant(7); }
void GdaFrame::OnCondVertQuant8(wxCommandEvent& e)
{ ChangeToCondVertQuant(8); }
void GdaFrame::OnCondVertQuant9(wxCommandEvent& e)
{ ChangeToCondVertQuant(9); }
void GdaFrame::OnCondVertQuant10(wxCommandEvent& e)
{ ChangeToCondVertQuant(10); }

void GdaFrame::ChangeToCondVertQuant(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::quantile, num_cats);
	}
}

void GdaFrame::OnCondVertPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::percentile, 6);
	}
}

void GdaFrame::OnCondVertHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_15, 6);
	}
}

void GdaFrame::OnCondVertHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_30, 6);
	}
}

void GdaFrame::OnCondVertStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::stddev, 6);
	}
}

void GdaFrame::OnCondVertNatBrks1(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(1); }
void GdaFrame::OnCondVertNatBrks2(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(2); }
void GdaFrame::OnCondVertNatBrks3(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(3); }
void GdaFrame::OnCondVertNatBrks4(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(4); }
void GdaFrame::OnCondVertNatBrks5(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(5); }
void GdaFrame::OnCondVertNatBrks6(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(6); }
void GdaFrame::OnCondVertNatBrks7(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(7); }
void GdaFrame::OnCondVertNatBrks8(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(8); }
void GdaFrame::OnCondVertNatBrks9(wxCommandEvent& e)
{ ChangeToCondVertNatBrks(9); }
void GdaFrame::OnCondVertNatBrks10(wxCommandEvent& e)
{ChangeToCondVertNatBrks(10);}

void GdaFrame::ChangeToCondVertNatBrks(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::natural_breaks, num_cats);
	}
}

void GdaFrame::OnCondVertEquInts1(wxCommandEvent& e)
{ ChangeToCondVertEquInts(1); }
void GdaFrame::OnCondVertEquInts2(wxCommandEvent& e)
{ ChangeToCondVertEquInts(2); }
void GdaFrame::OnCondVertEquInts3(wxCommandEvent& e)
{ ChangeToCondVertEquInts(3); }
void GdaFrame::OnCondVertEquInts4(wxCommandEvent& e)
{ ChangeToCondVertEquInts(4); }
void GdaFrame::OnCondVertEquInts5(wxCommandEvent& e)
{ ChangeToCondVertEquInts(5); }
void GdaFrame::OnCondVertEquInts6(wxCommandEvent& e)
{ ChangeToCondVertEquInts(6); }
void GdaFrame::OnCondVertEquInts7(wxCommandEvent& e)
{ ChangeToCondVertEquInts(7); }
void GdaFrame::OnCondVertEquInts8(wxCommandEvent& e)
{ ChangeToCondVertEquInts(8); }
void GdaFrame::OnCondVertEquInts9(wxCommandEvent& e)
{ ChangeToCondVertEquInts(9); }
void GdaFrame::OnCondVertEquInts10(wxCommandEvent& e)
{ ChangeToCondVertEquInts(10); }

void GdaFrame::ChangeToCondVertEquInts(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::equal_intervals, num_cats);
	}
}

void GdaFrame::OnCondVertUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::unique_values, 4);
	}
}


void GdaFrame::OnCondHorizThemelessMap(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::no_theme, 1);
	}
}

void GdaFrame::OnCondHorizQuant1(wxCommandEvent& e)
{ ChangeToCondHorizQuant(1); }
void GdaFrame::OnCondHorizQuant2(wxCommandEvent& e)
{ ChangeToCondHorizQuant(2); }
void GdaFrame::OnCondHorizQuant3(wxCommandEvent& e)
{ ChangeToCondHorizQuant(3); }
void GdaFrame::OnCondHorizQuant4(wxCommandEvent& e)
{ ChangeToCondHorizQuant(4); }
void GdaFrame::OnCondHorizQuant5(wxCommandEvent& e)
{ ChangeToCondHorizQuant(5); }
void GdaFrame::OnCondHorizQuant6(wxCommandEvent& e)
{ ChangeToCondHorizQuant(6); }
void GdaFrame::OnCondHorizQuant7(wxCommandEvent& e)
{ ChangeToCondHorizQuant(7); }
void GdaFrame::OnCondHorizQuant8(wxCommandEvent& e)
{ ChangeToCondHorizQuant(8); }
void GdaFrame::OnCondHorizQuant9(wxCommandEvent& e)
{ ChangeToCondHorizQuant(9); }
void GdaFrame::OnCondHorizQuant10(wxCommandEvent& e)
{ ChangeToCondHorizQuant(10); }

void GdaFrame::ChangeToCondHorizQuant(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::quantile, num_cats);
	}
}

void GdaFrame::OnCondHorizPercentile(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::percentile, 6);
	}
}

void GdaFrame::OnCondHorizHinge15(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_15, 6);
	}
}

void GdaFrame::OnCondHorizHinge30(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_30, 6);
	}
}

void GdaFrame::OnCondHorizStddev(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::stddev, 6);
	}
}

void GdaFrame::OnCondHorizNatBrks1(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(1); }
void GdaFrame::OnCondHorizNatBrks2(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(2); }
void GdaFrame::OnCondHorizNatBrks3(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(3); }
void GdaFrame::OnCondHorizNatBrks4(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(4); }
void GdaFrame::OnCondHorizNatBrks5(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(5); }
void GdaFrame::OnCondHorizNatBrks6(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(6); }
void GdaFrame::OnCondHorizNatBrks7(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(7); }
void GdaFrame::OnCondHorizNatBrks8(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(8); }
void GdaFrame::OnCondHorizNatBrks9(wxCommandEvent& e)
{ ChangeToCondHorizNatBrks(9); }
void GdaFrame::OnCondHorizNatBrks10(wxCommandEvent& e)
{ChangeToCondHorizNatBrks(10);}

void GdaFrame::ChangeToCondHorizNatBrks(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::natural_breaks, num_cats);
	}
}

void GdaFrame::OnCondHorizEquInts1(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(1); }
void GdaFrame::OnCondHorizEquInts2(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(2); }
void GdaFrame::OnCondHorizEquInts3(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(3); }
void GdaFrame::OnCondHorizEquInts4(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(4); }
void GdaFrame::OnCondHorizEquInts5(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(5); }
void GdaFrame::OnCondHorizEquInts6(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(6); }
void GdaFrame::OnCondHorizEquInts7(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(7); }
void GdaFrame::OnCondHorizEquInts8(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(8); }
void GdaFrame::OnCondHorizEquInts9(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(9); }
void GdaFrame::OnCondHorizEquInts10(wxCommandEvent& e)
{ ChangeToCondHorizEquInts(10); }

void GdaFrame::ChangeToCondHorizEquInts(int num_cats)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::equal_intervals, num_cats);
	}
}

void GdaFrame::OnCondHorizUniqueValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::unique_values, 4);
	}
}


void GdaFrame::OnSaveCategories(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSaveCategories();
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnSaveCategories();
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnSaveCategories();
	} else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
		f->OnSaveCategories();
	} else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
		f->OnSaveCategories();
	}
}

void GdaFrame::OnOpenRawrate(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
													false, false,
							"Raw Rate Smoothed Variable Settings",
							"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  dlg.GetCatClassifType(),
									  MapCanvas::raw_rate,
									  dlg.GetNumCategories(),
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnRawrate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnRawrate();
	}
}

void GdaFrame::OnOpenExcessrisk(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
													false,
													"Excess Risk Map Variable Settings",
													"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  CatClassification::excess_risk_theme,
									  MapCanvas::excess_risk, 6,
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnExcessrisk(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnExcessRisk();
	}
}

void GdaFrame::OnOpenEmpiricalBayes(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
													false, false,
													"Empirical Bayes Smoothed Variable Settings",
													"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  dlg.GetCatClassifType(),
									  MapCanvas::empirical_bayes,
									  dlg.GetNumCategories(),
									  boost::uuids::nil_uuid(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnEmpiricalBayes();
	}
}

void GdaFrame::OnOpenSpatialRate(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
													true, false,
													"Spatial Rate Smoothed Variable Settings",
													"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  dlg.GetCatClassifType(),
									  MapCanvas::spatial_rate,
									  dlg.GetNumCategories(),
									  dlg.GetWeightsId(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}

void GdaFrame::OnSpatialRate(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSpatialRate();
	}
}

void GdaFrame::OnOpenSpatialEmpiricalBayes(wxCommandEvent& event)
{
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
													true, false,
													"Empirical Spatial Rate Smoothed Variable Settings",
													"Event Variable", "Base Variable");
	if (dlg.ShowModal() != wxID_OK) return;
	MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
									  dlg.var_info, dlg.col_ids,
									  dlg.GetCatClassifType(),
									  MapCanvas::spatial_empirical_bayes,
									  dlg.GetNumCategories(),
									  dlg.GetWeightsId(),
									  wxDefaultPosition,
									  GdaConst::map_default_size);
	nf->UpdateTitle();
}


void GdaFrame::OnSpatialEmpiricalBayes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSpatialEmpiricalBayes();
	}
}

void GdaFrame::OnSaveResults(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSaveRates();
	}
}

void GdaFrame::OnSelectIsolates(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnSelectIsolates(event);
	} else if (WeightsManFrame* f = dynamic_cast<WeightsManFrame*>(t)) {
		f->OnSelectIsolates(event);
	}
}

void GdaFrame::OnSaveConnectivityToTable(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnSaveConnectivityToTable(event);
	} else if (WeightsManFrame* f = dynamic_cast<WeightsManFrame*>(t)) {
		f->OnSaveConnectivityToTable(event);
	}
}

void GdaFrame::OnHistogramIntervals(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConnectivityHistFrame* f = dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnHistogramIntervals(event);
	} else if (WeightsManFrame* f = dynamic_cast<WeightsManFrame*>(t)) {
		f->OnHistogramIntervals(event);
	} else if (ConditionalHistogramFrame* f =
			   dynamic_cast<ConditionalHistogramFrame*>(t)) {
		f->OnHistogramIntervals(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnHistogramIntervals(event);
	}
}

void GdaFrame::OnRan99Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan99Per(event);
	}
}

void GdaFrame::OnRan199Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan199Per(event);
	}
}

void GdaFrame::OnRan499Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (LisaScatterPlotFrame* f
			   = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan499Per(event);
	}
}

void GdaFrame::OnRan999Per(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan999Per(event);
	}
}

void GdaFrame::OnRanOtherPer(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRanOtherPer(event);
	}
}

void GdaFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnUseSpecifiedSeed(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnUseSpecifiedSeed(event);
    } else if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
        f->OnUseSpecifiedSeed(event);
	}
}

void GdaFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
	} else if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
    }
}

void GdaFrame::OnSaveMoranI(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSaveMoranI(event);
	}
}

void GdaFrame::OnSigFilter05(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter05(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter05(event);
	}
}

void GdaFrame::OnSigFilter01(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter01(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter01(event);
	}
}

void GdaFrame::OnSigFilter001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter001(event);
	}
}

void GdaFrame::OnSigFilter0001(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter0001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter0001(event);
	}
}

void GdaFrame::OnAddMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->GetProject()->AddMeanCenters();
	}
}

void GdaFrame::OnAddCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->GetProject()->AddCentroids();
	}
}

void GdaFrame::OnDisplayMeanCenters(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayMeanCenters();
	}
}

void GdaFrame::OnDisplayCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayCentroids();
	}
}

void GdaFrame::OnDisplayVoronoiDiagram(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayVoronoiDiagram();
	}
}

void GdaFrame::OnExportVoronoi(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnExportVoronoi();
	} else {
		if (project_p) project_p->ExportVoronoi();
	}
}

void GdaFrame::OnExportMeanCntrs(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (project_p) project_p->ExportCenters(true);
}

void GdaFrame::OnExportCentroids(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (project_p) project_p->ExportCenters(false);
}

void GdaFrame::OnSaveVoronoiDupsToTable(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnSaveVoronoiDupsToTable();
	} else {
		if (project_p) project_p->SaveVoronoiDupsToTable();
	}
}

void GdaFrame::OnSaveGetisOrd(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSaveGetisOrd(event);
	}
}

void GdaFrame::OnSaveLisa(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSaveLisa(event);
	}
}

void GdaFrame::OnSelectCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCores(event);
	}
}

void GdaFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	}
}

void GdaFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	}
}

void GdaFrame::OnViewStandardizedData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewStandardizedData(event);
	}	
}

void GdaFrame::OnViewOriginalData(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	}
}

void GdaFrame::OnViewLinearSmoother(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewLinearSmoother(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewLinearSmoother(event);
	} else if (ConditionalScatterPlotFrame* f =
						 dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnViewLinearSmoother(event);
	}
}

void GdaFrame::OnViewLowessSmoother(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewLowessSmoother(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewLowessSmoother(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnViewLowessSmoother(event);
	} else if (ConditionalScatterPlotFrame* f =
						 dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnViewLowessSmoother(event);
	}
}

void GdaFrame::OnEditLowessParams(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnEditLowessParams(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnEditLowessParams(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnEditLowessParams(event);
	} else if (ConditionalScatterPlotFrame* f =
						 dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnEditLowessParams(event);
	}
}

void GdaFrame::OnEditVariables(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnShowVarsChooser(event);
	} else if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
		f->OnShowCorrelParams(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnShowVarsChooser(event);
	} else if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		f->OnShowVarsChooser(event);
	}
}

void GdaFrame::OnViewRegimesRegression(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	}
}

void GdaFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	}
}

void GdaFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelected(event);
	}
}

void GdaFrame::OnCompareRegimes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f, "ID_COMPARE_REGIMES");
	}
}

void GdaFrame::OnCompareTimePeriods(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f,"ID_COMPARE_TIME_PERIODS");
	}
}

void GdaFrame::OnCompareRegAndTmPer(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=
			new LineChartEventDelay(f,"ID_COMPARE_REG_AND_TM_PER");
	}
}

void GdaFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (ConnectivityHistFrame* f =
			   dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (WeightsManFrame* f = dynamic_cast<WeightsManFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f, "ID_DISPLAY_STATISTICS");
	}
}

void GdaFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnShowAxesThroughOrigin(event);
	}
}

void GdaFrame::OnShowAxes(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (ConnectivityHistFrame* f =
			   dynamic_cast<ConnectivityHistFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (ConditionalHistogramFrame* f =
			   dynamic_cast<ConditionalHistogramFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (WeightsManFrame* f = dynamic_cast<WeightsManFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
		f->OnShowAxes(event);
	} else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnShowAxes(event);
	}
}

void GdaFrame::OnDisplayAxesScaleValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalScatterPlotFrame* f =
		dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnDisplayAxesScaleValues(event);
	}
}

void GdaFrame::OnDisplaySlopeValues(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalScatterPlotFrame* f =
		dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnDisplaySlopeValues(event);
	} else if (ScatterPlotMatFrame* f =
						 dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnDisplaySlopeValues(event);
	}
}

void GdaFrame::OnTimeSyncVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnTimeSyncVariable(var_index);
}

void GdaFrame::OnTimeSyncVariable1(wxCommandEvent& event)
{
	OnTimeSyncVariable(0);
}

void GdaFrame::OnTimeSyncVariable2(wxCommandEvent& event)
{
	OnTimeSyncVariable(1);
}

void GdaFrame::OnTimeSyncVariable3(wxCommandEvent& event)
{
	OnTimeSyncVariable(2);
}

void GdaFrame::OnTimeSyncVariable4(wxCommandEvent& event)
{
	OnTimeSyncVariable(3);
}

void GdaFrame::OnFixedScaleVariable(int var_index)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFixedScaleVariable(var_index);
}

void GdaFrame::OnFixedScaleVariable1(wxCommandEvent& event)
{
	OnFixedScaleVariable(0);
}

void GdaFrame::OnFixedScaleVariable2(wxCommandEvent& event)
{
	OnFixedScaleVariable(1);
}

void GdaFrame::OnFixedScaleVariable3(wxCommandEvent& event)
{
	OnFixedScaleVariable(2);
}

void GdaFrame::OnFixedScaleVariable4(wxCommandEvent& event)
{
	OnFixedScaleVariable(3);
}

void GdaFrame::OnPlotsPerView(int plots_per_view)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerView(plots_per_view);
}

void GdaFrame::OnPlotsPerView1(wxCommandEvent& event)
{
	OnPlotsPerView(1);
}

void GdaFrame::OnPlotsPerView2(wxCommandEvent& event)
{
	OnPlotsPerView(2);
}

void GdaFrame::OnPlotsPerView3(wxCommandEvent& event)
{
	OnPlotsPerView(3);
}

void GdaFrame::OnPlotsPerView4(wxCommandEvent& event)
{
	OnPlotsPerView(4);
}

void GdaFrame::OnPlotsPerView5(wxCommandEvent& event)
{
	OnPlotsPerView(5);
}

void GdaFrame::OnPlotsPerView6(wxCommandEvent& event)
{
	OnPlotsPerView(6);
}

void GdaFrame::OnPlotsPerView7(wxCommandEvent& event)
{
	OnPlotsPerView(7);
}

void GdaFrame::OnPlotsPerView8(wxCommandEvent& event)
{
	OnPlotsPerView(8);
}

void GdaFrame::OnPlotsPerView9(wxCommandEvent& event)
{
	OnPlotsPerView(9);
}

void GdaFrame::OnPlotsPerView10(wxCommandEvent& event)
{
	OnPlotsPerView(10);
}

void GdaFrame::OnPlotsPerViewOther(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewOther();
}

void GdaFrame::OnPlotsPerViewAll(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewAll();
}

void GdaFrame::OnDisplayStatusBar(wxCommandEvent& event)
{
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnDisplayStatusBar(event);
}

void GdaFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event) )
{
	wxDialog dlg;
	wxXmlResource::Get()->LoadDialog(&dlg, this, "IDD_ABOUTBOX");
	
	wxStaticText* cr = dynamic_cast<wxStaticText*>
		(wxWindow::FindWindowById(XRCID("ID_COPYRIGHT"), &dlg));
	wxString cr_s;
	cr_s << "Copyright (C) 2011-" << Gda::version_year << " by Luc Anselin";
	if (cr) cr->SetLabelText(cr_s);
	
	wxStaticText* arr = dynamic_cast<wxStaticText*>
		(wxWindow::FindWindowById(XRCID("ID_ALL_RIGHTS_RESERVED"), &dlg));
	wxString arr_s;
	arr_s << "All Rights Reserved";
	if (arr) arr->SetLabelText(arr_s);
	
	wxStaticText* vl = dynamic_cast<wxStaticText*>
		(wxWindow::FindWindowById(XRCID("ID_VERSION_LABEL"), &dlg));
	wxString vl_s;
	vl_s << "GeoDa " << Gda::version_major << "." << Gda::version_minor << ".";
	vl_s << Gda::version_build;
	if (Gda::version_type == 0) {
		vl_s << " (alpha),";
	} else if (Gda::version_type == 1) {
        if (Gda::version_night > 0)
            vl_s << "-" << Gda::version_night << " (nightly),";
        else
    		vl_s << " (beta),";
	} // otherwise assumed to be release
	vl_s << " " << Gda::version_day << " ";
	if (Gda::version_month == 1) {
		vl_s << "January";
	} else if (Gda::version_month == 2) {
		vl_s << "February";
	} else if (Gda::version_month == 3) {
		vl_s << "March";
	} else if (Gda::version_month == 4) {
		vl_s << "April";
	} else if (Gda::version_month == 5) {
		vl_s << "May";
	} else if (Gda::version_month == 6) {
		vl_s << "June";
	} else if (Gda::version_month == 7) {
		vl_s << "July";
	} else if (Gda::version_month == 8) {
		vl_s << "August";
	} else if (Gda::version_month == 9) {
		vl_s << "September";
	} else if (Gda::version_month == 10) {
		vl_s << "October";
	} else if (Gda::version_month == 11) {
		vl_s << "November";
	} else {
		vl_s << "December";
	} 
	vl_s << " " << Gda::version_year;
	if (vl) vl->SetLabelText(vl_s);
	
	dlg.ShowModal();
}

void GdaFrame::OnTableSetLocale(wxCommandEvent& event)
{
    // show a number locale setup dialog, then save it to GDAL global env
    LocaleSetupDlg localeDlg(this);
    localeDlg.ShowModal();
}
//------------------------------------------------------------------------------
// Following are functions for all encoding events
//------------------------------------------------------------------------------
void GdaFrame::OnEncodingUTF8(wxCommandEvent& event)
{
	if (!project_p) return;
    project_p->GetTableInt()->SetEncoding(wxFONTENCODING_UTF8);
    SetEncodingCheckmarks(wxFONTENCODING_UTF8);
}
void GdaFrame::OnEncodingUTF16(wxCommandEvent& event)
{
	if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_UTF16LE);
    SetEncodingCheckmarks(wxFONTENCODING_UTF16LE);
}
void GdaFrame::OnEncodingWindows1250(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1250);
    SetEncodingCheckmarks(wxFONTENCODING_CP1250);
}
void GdaFrame::OnEncodingWindows1251(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1251);
    SetEncodingCheckmarks(wxFONTENCODING_CP1251);
}
void GdaFrame::OnEncodingWindows1254(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1254);
    SetEncodingCheckmarks(wxFONTENCODING_CP1254);
}
void GdaFrame::OnEncodingWindows1255(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1255);
    SetEncodingCheckmarks(wxFONTENCODING_CP1255);
}
void GdaFrame::OnEncodingWindows1256(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1256);
    SetEncodingCheckmarks(wxFONTENCODING_CP1256);
}
void GdaFrame::OnEncodingWindows1258(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP1258);
    SetEncodingCheckmarks(wxFONTENCODING_CP1258);
}
void GdaFrame::OnEncodingCP852(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP852);
    SetEncodingCheckmarks(wxFONTENCODING_CP852);
}
void GdaFrame::OnEncodingCP866(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_CP866);
    SetEncodingCheckmarks(wxFONTENCODING_CP866);
}
void GdaFrame::OnEncodingISO8859_1(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_1);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_1);
}
void GdaFrame::OnEncodingISO8859_2(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_2);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_2);
}
void GdaFrame::OnEncodingISO8859_3(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_3);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_3);
}
void GdaFrame::OnEncodingISO8859_5(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_5);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_5);
}
void GdaFrame::OnEncodingISO8859_7(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_7);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_7);
}
void GdaFrame::OnEncodingISO8859_8(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_8);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_8);
}
void GdaFrame::OnEncodingISO8859_9(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_9);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_9);
}
void GdaFrame::OnEncodingISO8859_10(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_10);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_10);
}
void GdaFrame::OnEncodingISO8859_15(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_ISO8859_15);
    SetEncodingCheckmarks(wxFONTENCODING_ISO8859_15);
}
void GdaFrame::OnEncodingGB2312(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_GB2312);
    SetEncodingCheckmarks(wxFONTENCODING_GB2312);
}
void GdaFrame::OnEncodingBIG5(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_BIG5);
    SetEncodingCheckmarks(wxFONTENCODING_BIG5);
}
void GdaFrame::OnEncodingKOI8_R(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_KOI8);
    SetEncodingCheckmarks(wxFONTENCODING_KOI8);
}
void GdaFrame::OnEncodingSHIFT_JIS(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_SHIFT_JIS);
    SetEncodingCheckmarks(wxFONTENCODING_SHIFT_JIS);
}
void GdaFrame::OnEncodingEUC_JP(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_EUC_JP);
    SetEncodingCheckmarks(wxFONTENCODING_EUC_JP);
}
void GdaFrame::OnEncodingEUC_KR(wxCommandEvent& event)
{
    if (!project_p) return;
	project_p->GetTableInt()->SetEncoding(wxFONTENCODING_EUC_KR);
    SetEncodingCheckmarks(wxFONTENCODING_EUC_KR);
}
void GdaFrame::SetEncodingCheckmarks(wxFontEncoding e)
{
	wxMenuBar* m = GetMenuBar();
	m->FindItem(XRCID("ID_ENCODING_UTF8"))->Check(e==wxFONTENCODING_UTF8);
	m->FindItem(XRCID("ID_ENCODING_UTF16"))->Check(e==wxFONTENCODING_UTF16LE);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1250"))->Check(e==wxFONTENCODING_CP1250);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1251"))->Check(e==wxFONTENCODING_CP1251);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1254"))->Check(e==wxFONTENCODING_CP1254);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1255"))->Check(e==wxFONTENCODING_CP1255);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1256"))->Check(e==wxFONTENCODING_CP1256);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1258"))->Check(e==wxFONTENCODING_CP1258);
	m->FindItem(XRCID("ID_ENCODING_CP852"))->Check(e==wxFONTENCODING_CP852);
	m->FindItem(XRCID("ID_ENCODING_CP866"))->Check(e==wxFONTENCODING_CP866);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_1"))->Check(e==wxFONTENCODING_ISO8859_1);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_2"))->Check(e==wxFONTENCODING_ISO8859_2);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_3"))->Check(e==wxFONTENCODING_ISO8859_3);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_5"))->Check(e==wxFONTENCODING_ISO8859_5);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_7"))->Check(e==wxFONTENCODING_ISO8859_7);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_8"))->Check(e==wxFONTENCODING_ISO8859_8);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_9"))->Check(e==wxFONTENCODING_ISO8859_9);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_10"))->Check(e==wxFONTENCODING_ISO8859_10);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_15"))->Check(e==wxFONTENCODING_ISO8859_15);
	m->FindItem(XRCID("ID_ENCODING_GB2312"))->Check(e==wxFONTENCODING_GB2312);
	m->FindItem(XRCID("ID_ENCODING_BIG5"))->Check(e==wxFONTENCODING_BIG5);
	m->FindItem(XRCID("ID_ENCODING_KOI8_R"))->Check(e==wxFONTENCODING_KOI8);
	m->FindItem(XRCID("ID_ENCODING_SHIFT_JIS"))->Check(e==wxFONTENCODING_SHIFT_JIS);
	m->FindItem(XRCID("ID_ENCODING_EUC_JP"))->Check(e==wxFONTENCODING_EUC_JP);
	m->FindItem(XRCID("ID_ENCODING_EUC_KR"))->Check(e==wxFONTENCODING_EUC_KR);
}

void GdaFrame::SetBasemapCheckmarks(int idx)
{
    /*
    wxMenuBar* m = GetMenuBar();
    m->FindItem(XRCID("ID_NO_BASEMAP"))->Check(idx==0);
    m->FindItem(XRCID("ID_BASEMAP_1"))->Check(idx==1);
    m->FindItem(XRCID("ID_BASEMAP_2"))->Check(idx==2);
    m->FindItem(XRCID("ID_BASEMAP_3"))->Check(idx==3);
    m->FindItem(XRCID("ID_BASEMAP_4"))->Check(idx==4);
    m->FindItem(XRCID("ID_BASEMAP_5"))->Check(idx==5);
    m->FindItem(XRCID("ID_BASEMAP_6"))->Check(idx==6);
    m->FindItem(XRCID("ID_BASEMAP_7"))->Check(idx==7);
    m->FindItem(XRCID("ID_BASEMAP_8"))->Check(idx==8);
     */
}
//------------------------------------------------------------------------------
// End of functions for all encoding events
//------------------------------------------------------------------------------


bool GdaFrame::GetHtmlMenuItems()
{
	return GetHtmlMenuItemsJson();
}

bool GdaFrame::GetHtmlMenuItemsJson()
{
	LOG_MSG("Entering GdaFrame::GetHtmlMenuItemsJson");
	using namespace json_spirit;
	using namespace GdaJson;
	
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFnPath(wxStandardPaths::Get().GetExecutablePath());
	wxString prefs_fn = exeFnPath.GetPathWithSep() 
		+ GdaConst::gda_prefs_fname_json;
	if (!wxFileExists(prefs_fn)) {
		LOG_MSG("Could not find " + prefs_fn);
		return false;
	}
	
	htmlMenuItems.clear();
	std::ifstream ifs;
	try {
		ifs.open(GET_ENCODED_FILENAME(prefs_fn), std::ifstream::in);
		if (!(ifs.is_open() && ifs.good())) {
			wxString msg("Could not read JSON prefs file: ");
			msg << prefs_fn;
			throw std::runtime_error(prefs_fn.ToStdString());
		}
		//ifs.close();
		LOG_MSG("Opened " + prefs_fn);
		const wxString ent_key(GdaConst::gda_prefs_html_table);
		const wxString menu_col(GdaConst::gda_prefs_html_table_menu);
		const wxString url_col(GdaConst::gda_prefs_html_table_url);
		
		Value pf_val;
		if (!json_spirit::read(ifs, pf_val)) {
			wxString msg("Could not parse JSON prefs file: ");
			msg << prefs_fn;
			throw std::runtime_error(prefs_fn.ToStdString());
		}
		if (pf_val.type() != json_spirit::obj_type) {
			throw std::runtime_error("JSON pref content not a JSON Object");
		}
		Value html_ents;
		if (!findValue(pf_val, html_ents, ent_key)) return true;
		if (html_ents.type() != json_spirit::array_type) {
			throw std::runtime_error("Html menu entries must be an array");
		}
		Array& html_ents_a(html_ents.get_array());
		for (Array::const_iterator i=html_ents_a.begin(); i!=html_ents_a.end();
			 ++i)
		{
			wxString title = getStrValFromObj((*i), menu_col);
			wxString url = getStrValFromObj((*i), url_col);
			wxString wp("web_plugins");
			// if url begins with "web_plugins", we assume the file is a
			// path relative to the geoda_prefs.json resource file location.
			// In this case, we must convert to an absolute path.
			if (url.Left(wp.length()) == wp) {
				LOG_MSG("Converting url relative path to absolute path");
				wxString cpy_url = url;
				url = "file://";
				url << exeFnPath.GetPathWithSep() << cpy_url;
				LOG_MSG("New url: " + url);
			}
			if (!title.IsEmpty()) htmlMenuItems.push_back(MenuItem(title, url));
		}
	}
	catch (std::runtime_error e) {
		wxString msg("Error reading JSON prefs file: ");
		msg << e.what();
		LOG_MSG(msg);
		return false;
	}
	
	LOG_MSG("html_entries:");
	for (size_t i=0; i<htmlMenuItems.size(); ++i) {
		wxString msg;
		msg << "title: " << htmlMenuItems[i].menu_title << ", ";
		msg << "url: " << htmlMenuItems[i].url;
		LOG_MSG(msg);
	}
	
	LOG_MSG("Exiting GdaFrame::GetHtmlMenuItemsJson");
	return true;
}

bool GdaFrame::GetHtmlMenuItemsSqlite()
{
  /*
	LOG_MSG("Entering GdaFrame::GetHtmlMenuItemsSqlite");
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName temp(wxStandardPaths::Get().GetExecutablePath());
	wxString prefs_fn = temp.GetPathWithSep() +GdaConst::gda_prefs_fname_sqlite;
	if (!wxFileExists(prefs_fn)) {
		LOG_MSG("Could not find " + prefs_fn);
		return false;
	}
	LOG_MSG("Found and opening " + prefs_fn);
	
	sqlite3* db;
	int rc; // sqlite3 result code
	char *zErrMsg = 0;
	const char* data = "Callback function called";
	
	// Open DB
	rc = sqlite3_open(prefs_fn.c_str(), &db);
	if (rc) {
		wxString err_msg(sqlite3_errmsg(db));
		LOG_MSG("Can't open database: " + err_msg);
		return false;
	}
	LOG_MSG("Database open success.");
		
	// Execute SQL statement
	htmlMenuItems.clear();
	rc = sqlite3_exec(db, "SELECT * from html_entries",
					  GdaFrame::sqlite3_GetHtmlMenuItemsCB,
					  (void*)data, &zErrMsg);
	if ( rc != SQLITE_OK ){
		wxString sql_err_msg(zErrMsg);
		sqlite3_free(zErrMsg);
		LOG_MSG("SQL error: " + sql_err_msg);
		return false;
	} else {
		LOG_MSG("SQL success.");
	}
	LOG_MSG("html_entries:");
	for (size_t i=0; i<htmlMenuItems.size(); ++i) {
		wxString msg;
		msg << "title: " << htmlMenuItems[i].menu_title << ", ";
		msg << "url: " << htmlMenuItems[i].url;
		LOG_MSG(msg);
	}
	
	sqlite3_close(db);
	LOG_MSG("Exiting GdaFrame::GetHtmlMenuItemsSqlite");
	return true;
  */
  return false;
}

int GdaFrame::sqlite3_GetHtmlMenuItemsCB(void *data, int argc,
										 char **argv, char **azColName)
{
	if (argc != 2) return SQLITE_ERROR;
	htmlMenuItems.push_back(MenuItem(argv[0], argv[1]));
	return SQLITE_OK;
}

wxConnectionBase* GdaServer::OnAcceptConnection(const wxString& topic)
{
	LOG_MSG("In GdaServer::OnAcceptConnection");
	if (topic.CmpNoCase("GdaApp") == 0) {
		// Check there are no modal dialogs active
		wxWindowList::Node* node = wxTopLevelWindows.GetFirst();
		while (node) {
			wxDialog* dlg = wxDynamicCast(node->GetData(), wxDialog);
			if (dlg && dlg->IsModal()) return 0;
			node = node->GetNext();
		}
		return new GdaConnection();
	} else {
		return 0;
	}
}

wxConnectionBase* GdaClient::OnMakeConnection()
{
	return new GdaConnection;
}

bool GdaConnection::OnExec(const wxString &topic, const wxString &data)
{
	LOG_MSG("In GdaConnection::OnExec");
	GdaFrame* frame = wxDynamicCast(wxGetApp().GetTopWindow(), GdaFrame);
	wxString filename(data);
	if (filename.IsEmpty()) {
		if (frame) frame->Raise();
	} else {
		frame->OpenProject(filename);
	}
	return true;
}

LineChartEventDelay::LineChartEventDelay()
: lc_frame(0)
{
	LOG_MSG("In LineChartEventDelay::LineChartEventDelay");
}

LineChartEventDelay::LineChartEventDelay(LineChartFrame* lc_frame_,
																				 const wxString& cb_name_)
: lc_frame(lc_frame_), cb_name(cb_name_)
{
	LOG_MSG("Created LineChartEventDelay::LineChartEventDelay for callback: "
					+ cb_name);
	StartOnce(100);
}

LineChartEventDelay::~LineChartEventDelay()
{
	LOG_MSG("In LineChartEventDelay::~LineChartEventDelay");
}

void LineChartEventDelay::Notify() {
	LOG_MSG("In LineChartEventDelay::Notify");
	Stop();
	wxCommandEvent ev;
	if (cb_name == "ID_COMPARE_REGIMES") {
		lc_frame->OnCompareRegimes(ev);
	} else if (cb_name == "ID_COMPARE_TIME_PERIODS") {
		lc_frame->OnCompareTimePeriods(ev);
	} else if (cb_name == "ID_COMPARE_REG_AND_TM_PER") {
		lc_frame->OnCompareRegAndTmPer(ev);
	} else if (cb_name == "ID_DISPLAY_STATISTICS") {
		lc_frame->OnDisplayStatistics(ev);
	} else {
		LOG_MSG("No matching callback for cb: " + cb_name);
	}
	delete this;
}
