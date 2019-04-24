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
#include <stdlib.h>


#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ogrsf_frmts.h"
#include "cpl_conv.h"

#include <wx/utils.h>
#include <wx/sysopt.h>
#include <wx/platinfo.h>
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
#include <wx/uri.h>

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
#include "DialogTools/AutoUpdateDlg.h"
#include "DialogTools/ReportBugDlg.h"
#include "DialogTools/SaveToTableDlg.h"
#include "DialogTools/KMeansDlg.h"
#include "DialogTools/MaxpDlg.h"
#include "DialogTools/SpectralClusteringDlg.h"
#include "DialogTools/HClusterDlg.h"
#include "DialogTools/HDBScanDlg.h"
#include "DialogTools/CreateGridDlg.h"
#include "DialogTools/RedcapDlg.h"
#include "DialogTools/MDSDlg.h"
#include "DialogTools/AggregateDlg.h"
#include "DialogTools/PCASettingsDlg.h"
#include "DialogTools/SkaterDlg.h"
#include "DialogTools/PreferenceDlg.h"
#include "DialogTools/SpatialJoinDlg.h"
#include "DialogTools/MultiVarSettingsDlg.h"
#include "Explore/CatClassification.h"
#include "Explore/CovSpView.h"
#include "Explore/CorrelParamsDlg.h"
#include "Explore/CorrelogramView.h"
#include "Explore/GetisOrdMapNewView.h"
#include "Explore/LineChartView.h"
#include "Explore/LisaMapNewView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/LocalGearyCoordinator.h"
#include "Explore/LocalGearyMapNewView.h"
#include "Explore/ConditionalMapView.h"
#include "Explore/ConditionalNewView.h"
#include "Explore/ConditionalScatterPlotView.h"
#include "Explore/ConnectivityHistView.h"
#include "Explore/ConnectivityMapView.h"
#include "Explore/ConditionalHistogramView.h"
#include "Explore/CartogramNewView.h"
#include "Explore/GStatCoordinator.h"
#include "Explore/MLJCMapNewView.h"
#include "Explore/MLJCCoordinator.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/ScatterPlotMatView.h"
#include "Explore/MapNewView.h"
#include "Explore/PCPNewView.h"
#include "Explore/HistogramView.h"
#include "Explore/BoxNewPlotView.h"
#include "Explore/3DPlotView.h"
#include "Explore/WebViewExampleWin.h"
#include "Explore/Basemap.h"
#include "Explore/ColocationMapView.h"
#include "Explore/GroupingMapView.h"
#include "Regression/DiagnosticReport.h"

#include "ShapeOperations/CsvFileUtils.h"
#include "ShapeOperations/WeightsManager.h"
#include "ShapeOperations/WeightsManState.h"
#include "ShapeOperations/OGRDataAdapter.h"

#include "VarCalc/CalcHelp.h"
#include "Algorithms/redcap.h"
#include "Algorithms/fastcluster.h"

#include "wxTranslationHelper.h"
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
#include "version.h"
#include "arizona/viz3/plots/scatterplot.h"
#include "rc/GeoDaIcon-16x16.xpm"

//The XML Handler should be explicitly registered:
#include <wx/xrc/xh_auitoolb.h>

// The following is defined in rc/GdaAppResouces.cpp.  This file was
// compiled with: wxrc dialogs.xrc menus.xrc toolbar.xrc
//   --cpp-code --output=GdaAppResources.cpp --function=GdaInitXmlResource
// and combines all resouces file into single source file that is linked into
// the application binary.
extern void GdaInitXmlResource();

IMPLEMENT_APP(GdaApp)

GdaApp::GdaApp() : m_pLogFile(0)
{
	//Don't call wxHandleFatalExceptions so that a core dump file will be
	//produced for debugging.
	//wxHandleFatalExceptions();
}

GdaApp::~GdaApp()
{
    wxLog::SetActiveTarget(NULL);
    if (m_pLogFile != NULL){
        fclose(m_pLogFile);
    }
}


bool GdaApp::OnInit(void)
{
	if (!wxApp::OnInit())
        return false;

    // initialize OGR connection
	OGRDataAdapter::GetInstance();

    // load preferences
    PreferenceDlg::ReadFromCache();
    
    // load language here: GdaConst::gda_ui_language
    // search_path is the ./lang directory
    // config_path it the exe directory (every user will have a different config file?)
    wxFileName appFileName(argv[0]);
    appFileName.Normalize(wxPATH_NORM_DOTS|wxPATH_NORM_ABSOLUTE| wxPATH_NORM_TILDE);
    wxString search_path = appFileName.GetPath() + wxFileName::GetPathSeparator() +  "lang";
    // load language from lang/config.ini if user specified any
    wxString config_path = search_path + wxFileName::GetPathSeparator()+ "config.ini";
    bool use_native_config = false;
    m_TranslationHelper = new wxTranslationHelper(*this, search_path, use_native_config);
    m_TranslationHelper->SetConfigPath(config_path);
    m_TranslationHelper->Load();
    // forcing numeric settings to en_US, which is used internally in GeoDa
    setlocale(LC_NUMERIC, "en_US");

    // Other GDAL configurations
    if (GdaConst::hide_sys_table_postgres == false) {
        CPLSetConfigOption("PG_LIST_ALL_TABLES", "YES");
    }
    if (GdaConst::hide_sys_table_sqlite == false) {
        CPLSetConfigOption("SQLITE_LIST_ALL_TABLES", "YES");
    }
    if (GdaConst::gdal_http_timeout >= 0 ) {
        CPLSetConfigOption("GDAL_HTTP_TIMEOUT",
                           wxString::Format("%d", GdaConst::gdal_http_timeout));
    }
    CPLSetConfigOption("OGR_XLS_HEADERS", "FORCE");
    CPLSetConfigOption("OGR_XLSX_HEADERS", "FORCE");
    // For reaching DBF file, set SHAPE_ENCODING to "" to avoid any recoding
    CPLSetConfigOption("SHAPE_ENCODING", "");
    
	// will suppress "iCCP: known incorrect sRGB profile" warning message
	// in wxWidgets 2.9.5.  This is a bug in libpng.  See wxWidgets trac
	// issue #15331 for more details.
    
	GdaConst::init();
	CalcHelp::init();
	
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
    
    wxXmlResource::Get()->AddHandler(new wxAuiToolBarXmlHandler);
    wxXmlResource::Get()->InitAllHandlers();
	
	//Required for virtual file system archive and memory support
    wxFileSystem::AddHandler(new wxArchiveFSHandler);
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
	
    GdaInitXmlResource();  // call the init function in GdaAppResources.cpp	
	
    // check crash
    if (GdaConst::disable_crash_detect == false) {
        std::vector<wxString> items = OGRDataAdapter::GetInstance().GetHistory("NoCrash");
        if (items.size() > 0) {
            wxString no_crash = items[0];
            if (no_crash == "false") {
                // ask user to send crash data
                wxString msg = _("It looks like GeoDa has been terminated abnormally. \nDo you want to send a crash report to GeoDa team? \n\n(Optional) Please leave your email address,\nso we can send a follow-up email once we have a fix.");
                wxString ttl = _("Send Crash Report");
                wxString user_email = GdaConst::gda_user_email;
                wxTextEntryDialog msgDlg(GdaFrame::GetGdaFrame(), msg, ttl, user_email,
                                         wxOK | wxCANCEL | wxCENTRE );
                if (msgDlg.ShowModal() == wxID_OK) {
                    user_email = msgDlg.GetValue();
                    if (user_email != GdaConst::gda_user_email) {
                        OGRDataAdapter::GetInstance().AddEntry("gda_user_email", user_email);
                        GdaConst::gda_user_email = user_email;
                    }
                    wxString ttl = "Crash Report";
                    wxString body;
                    body << "From: " << user_email << "\n Details:";
                    ReportBugDlg::CreateIssue(ttl, body);
                }
            }
        }
        OGRDataAdapter::GetInstance().AddEntry("NoCrash", "false");
    }
    
	int frameWidth = 1020;
	int frameHeight = 80;
    
	if (GeneralWxUtils::isMac()) {
		frameWidth = 1092;
		frameHeight = 80;
	} else if (GeneralWxUtils::isWindows()) {
		frameWidth = 1200;
		frameHeight = 120;
	} else if (GeneralWxUtils::isUnix()) {  // assumes GTK
		frameWidth = 1100;
 		frameHeight = 120;
#ifdef __linux__
        wxLinuxDistributionInfo linux_info = wxGetLinuxDistributionInfo();
        if (linux_info.Description.Lower().Contains("centos")) {
            frameHeight = 180;
        }
#endif
	}

	wxPoint appFramePos = wxPoint(80,60);
    
    int screenX = wxSystemSettings::GetMetric ( wxSYS_SCREEN_X );
    if (screenX < frameWidth) {
        frameWidth = screenX;
        appFramePos = wxPoint(0, 0);
    }

	wxFrame* frame = new GdaFrame("GeoDa", appFramePos,
                                  wxSize(frameWidth, frameHeight),
								  wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX));
    frame->Show(true);
    frame->SetMinSize(wxSize(640, frameHeight));
    
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

    wxPoint welcome_pos = appFramePos;
    welcome_pos.y += 150;
    
    // setup gdaldata directory for libprj
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFile(exePath);
    wxString exeDir = exeFile.GetPathWithSep();
	// Set GEODA_GDAL_DATA 
#ifdef __WIN32__
	wxString gal_data_dir = exeDir + "data";
	wxSetEnv("GEODA_GDAL_DATA", gal_data_dir);
    //CPLSetConfigOption("GEODA_GDAL_DATA", GET_ENCODED_FILENAME(gal_data_dir));
#elif defined __linux__
	wxString gal_data_dir = exeDir + "gdaldata";
	wxSetEnv("GEODA_GDAL_DATA", gal_data_dir);
    CPLSetConfigOption("GEODA_GDAL_DATA", GET_ENCODED_FILENAME(gal_data_dir));
#else
	wxString gal_data_dir = exeDir + "../Resources/gdaldata";
	wxSetEnv("GEODA_GDAL_DATA", gal_data_dir);
    CPLSetConfigOption("GEODA_GDAL_DATA", GET_ENCODED_FILENAME(gal_data_dir));
#endif
   
    // Setup new Logger after crash check
    wxString loggerFile = GenUtils::GetSamplesDir() +"logger.txt";
    
    if (m_pLogFile == NULL) {
#ifdef __WIN32__
		m_pLogFile = _wfopen( loggerFile.wc_str(), L"w+" );
#else
        m_pLogFile = fopen( GET_ENCODED_FILENAME(loggerFile), "w+" );
#endif
        wxLog::SetActiveTarget(new wxLogStderr(m_pLogFile));
    }
    wxLog::EnableLogging(true);
    wxLog::DisableTimestamp();
    wxLog::SetComponentLevel("wx", wxLOG_FatalError);
#ifdef __DEBUG__
    wxLog::SetLogLevel(wxLOG_Message);
#endif
    wxString os_id = GeneralWxUtils::LogOsId();
    wxLogMessage(os_id);
    wxString versionlog = wxString::Format("vs: %d-%d-%d-%d",
                                           Gda::version_major,
                                           Gda::version_minor,
                                           Gda::version_build,
                                           Gda::version_subbuild);
    wxLogMessage(versionlog);
    wxLogMessage(loggerFile);
    
   
    if (!cmd_line_proj_file_name.IsEmpty()) {
        wxString proj_fname(cmd_line_proj_file_name);
        wxArrayString fnames;
        fnames.Add(proj_fname);
        MacOpenFiles(fnames);
    } else {
        if (os_id != "\nos: 1-10-14") {
            wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED, XRCID("ID_NEW_PROJECT"));
            frame->GetEventHandler()->ProcessEvent(ev);
        }
    }

	return true;
}

bool GdaApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if ( parser.GetParamCount() > 0) {
        cmd_line_proj_file_name = parser.GetParam(0);
    }
    return true;
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
	parser.SetDesc (GdaApp::globalCmdLineDesc);
    parser.SetSwitchChars ("-");
}

void GdaApp::MacOpenFiles(const wxArrayString& fileNames)
{
    wxLogMessage("MacOpenFiles");
    wxLogMessage(fileNames[0]);
    int sz=fileNames.GetCount();

    if (sz > 0) {
        wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
        while (node) {
            wxWindow* win = node->GetData();
            if (ConnectDatasourceDlg* w = dynamic_cast<ConnectDatasourceDlg*>(win)) {
				wxLogMessage("Close ConnectDatasourceDlg");
                w->EndModal(wxID_CANCEL);
            }
            node = node->GetNext();
        }
        
        GdaFrame::GetGdaFrame()->OpenProject(fileNames[0]);
    }
}

int GdaApp::OnExit(void)
{
	return 0;
}

void GdaApp::OnFatalException()
{
	wxMessageBox(_("GeoDa has run into a problem and will close."));
}

std::vector<GdaFrame::MenuItem> GdaFrame::htmlMenuItems;
GdaFrame* GdaFrame::gda_frame = 0;
bool GdaFrame::projectOpen = false;
Project* GdaFrame::project_p = 0;
std::list<wxAuiToolBar*> GdaFrame::toolbar_list(0);

void GdaFrame::UpdateToolbarAndMenus()
{
	// This method is called when no particular window is currently active.
	// In this case, the close menu item should be disabled.
	// some change
  
    GeneralWxUtils::EnableMenuItem(GetMenuBar(), _("File"), wxID_CLOSE, false);
    Project* p = GetProject();
	bool proj_open = (p != 0);
	bool shp_proj = proj_open && !p->IsTableOnlyProject();
	bool table_proj = proj_open && p->IsTableOnlyProject();
	//bool time_variant = proj_open && p->GetTableInt()->IsTimeVariant();
    bool time_variant = proj_open;
    
	wxMenuBar* mb = GetMenuBar();

	SetTitle("GeoDa");
	if (proj_open && project_p) SetTitle(project_p->GetProjectTitle());
	
    EnableTool(XRCID("ID_NEW_PROJECT"), !proj_open);
	EnableTool(XRCID("ID_OPEN_PROJECT"), !proj_open);
	EnableTool(XRCID("ID_CLOSE_PROJECT"), proj_open);
	
	GeneralWxUtils::EnableMenuItem(mb, _("File"), XRCID("ID_NEW_PROJECT"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("File"), XRCID("ID_OPEN_PROJECT"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("File"), XRCID("ID_CLOSE_PROJECT"), proj_open);
		
	if (!proj_open) {
		// Disable only if project not open.  Otherwise, leave changing
		// Save state to SaveButtonManager
		EnableTool(XRCID("ID_SAVE_PROJECT"), proj_open);
		GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_PROJECT"), false);
        UpdateRecentDatasourceMenu();
	}
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_NEW_PROJ_FROM_RECENT_MENU"), !proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_AS_PROJECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_LAYER"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_EXPORT_SELECTED"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_PROJECT_INFO"), proj_open);
	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), true);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), false);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), false);
    GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CUSTOM"), false);

    EnableTool(XRCID("ID_TOOLS_MENU"), proj_open);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), proj_open);
	EnableTool(XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	EnableTool(XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	EnableTool(XRCID("ID_CONNECTIVITY_MAP_VIEW"), proj_open);

    GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_TABLE_SPATIAL_JOIN"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_HIERARCHICAL_MAP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_TOOLS_WEIGHTS_MANAGER"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_TOOLS_WEIGHTS_CREATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_CONNECTIVITY_HIST_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_CONNECTIVITY_MAP_VIEW"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, _("Tools"), XRCID("ID_POINTS_FROM_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"), false);

	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECT_WITH_CUSTOM"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SELECTION_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ADJUST_AXIS_PRECISION"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_ZOOM_MODE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_PAN_MODE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, _("Explore"), proj_open);
	
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
    
    EnableTool(XRCID("ID_CLUSTERING_CHOICES"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_CLUSTERING_MENU"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_PCA"), shp_proj);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_KMEANS"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_KMEDIANS"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_KMEDOIDS"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_TOOLS_DATA_HCLUSTER"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb,XRCID("ID_TOOLS_DATA_HDBSCAN"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_MAXP"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_SKATER"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_SPECTRAL"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_REDCAP"), proj_open);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_TOOLS_DATA_MDS"), proj_open);
	
	EnableTool(XRCID("IDM_3DP"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_3DP"), proj_open);
	
	EnableTool(XRCID("IDM_LINE_CHART"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LINE_CHART"), proj_open);

	EnableTool(XRCID("IDM_NEW_TABLE"), proj_open);
	GeneralWxUtils::EnableMenuAll(mb, _("Table"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SHOW_TIME_CHOOSER"),time_variant);
    
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
	EnableTool(XRCID("IDM_DMORAN"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_DMORAN"), proj_open);
	EnableTool(XRCID("IDM_MORAN_EBRATE"), proj_open);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MORAN_EBRATE"), proj_open);
	EnableTool(XRCID("IDM_UNI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_UNI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_MULTI_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MULTI_LISA"), shp_proj);
	EnableTool(XRCID("IDM_DIFF_LISA"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_DIFF_LISA"), shp_proj);
	EnableTool(XRCID("IDM_LISA_EBRATE"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LISA_EBRATE"), shp_proj);
	EnableTool(XRCID("IDM_LOCAL_G"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LOCAL_G"), shp_proj);
	EnableTool(XRCID("IDM_LOCAL_G_STAR"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LOCAL_G_STAR"), shp_proj);
	EnableTool(XRCID("IDM_LOCAL_JOINT_COUNT"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_LOCAL_JOINT_COUNT"), shp_proj);
	EnableTool(XRCID("IDM_BIV_LJC"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_BIV_LJC"), shp_proj);
    EnableTool(XRCID("IDM_MUL_LJC"), shp_proj);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MUL_LJC"), shp_proj);

    
    EnableTool(XRCID("IDM_UNI_LOCAL_GEARY"), shp_proj);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_UNI_LOCAL_GEARY"), shp_proj);
    EnableTool(XRCID("IDM_MUL_LOCAL_GEARY"), shp_proj);
    GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_MUL_LOCAL_GEARY"), shp_proj);
	
	EnableTool(XRCID("IDM_CORRELOGRAM"), shp_proj);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("IDM_CORRELOGRAM"), shp_proj);
	
	GeneralWxUtils::EnableMenuAll(mb, _("Map"), shp_proj);
	EnableTool(XRCID("ID_MAP_CHOICES"), shp_proj);
	EnableTool(XRCID("ID_DATA_MOVIE"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), shp_proj);
	EnableTool(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), shp_proj);
	
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_REGRESSION_CLASSIC"), proj_open);
	EnableTool(XRCID("ID_REGRESSION_CLASSIC"), proj_open);
    EnableTool(XRCID("ID_PUBLISH"), proj_open && project_p->GetDatasourceType()==GdaConst::ds_cartodb);
	
	//Empty out the Options menu:
	wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);
    
    //Empty Custom category menu:
    wxMenuItem* mi = mb->FindItem(XRCID("ID_OPEN_CUSTOM_BREAKS_SUBMENU"));
    wxMenu* sm = mi->GetSubMenu();
    if (sm) {
        // clean
        wxMenuItemList items = sm->GetMenuItems();
        for (int i=0; i<items.size(); i++) {
            sm->Delete(items[i]);
        }
        if (project_p) {
            vector<wxString> titles;
            CatClassifManager* ccm = project_p->GetCatClassifManager();
            ccm->GetTitles(titles);
            
            sm->Append(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"),
                       _("Create New Custom"),
                       _("Create new custom categories classification."));
            sm->AppendSeparator();
            
            for (size_t j=0; j<titles.size(); j++) {
                wxMenuItem* new_mi = sm->Append(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0+j, titles[j]);
            }
            GdaFrame::GetGdaFrame()->Bind(wxEVT_COMMAND_MENU_SELECTED,
                                          &GdaFrame::OnEmptyCustomCategoryClick,
                                          GdaFrame::GetGdaFrame(),
                                          GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0,
                                          GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0 + titles.size());
        }
    }

    // Reset encoding
    if (proj_open == false) SetEncodingCheckmarks(wxFONTENCODING_UTF8);
}

void GdaFrame::SetMenusToDefault()
{
	// This method disables all menu items that are not
	// in one of File, Tools, Methods, or Help menus.
	wxMenuBar* mb = GetMenuBar();
	if (!mb) return;
	wxString menuText = wxEmptyString;
	int menuCnt = mb->GetMenuCount();
	for (int i=0; i<menuCnt; i++) {
		mb->GetMenu(i);
		menuText = mb->GetMenuLabelText(i);
		if ( (menuText != _("File")) &&
			 (menuText != _("Tools")) &&
			 (menuText != _("Table")) &&
			 (menuText != _("Help")) ) {
			GeneralWxUtils::EnableMenuAll(mb, menuText, false);
		}
	}
}

GdaFrame::GdaFrame(const wxString& title, const wxPoint& pos,
				   const wxSize& size, long style)
: wxFrame(NULL, wxID_ANY, title, pos, size, style)
{
	SetBackgroundColour(*wxWHITE);
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
	SetMenuBar(wxXmlResource::Get()->LoadMenuBar("ID_SHARED_MAIN_MENU"));

	if (!GetHtmlMenuItems() || htmlMenuItems.size() == 0) {
	} else {
		wxMenuBar* mb = GetMenuBar();
		int exp_menu_ind = mb->FindMenu(_("Explore"));
		wxMenu* exp_menu = mb->GetMenu(exp_menu_ind);
		wxMenu* html_menu = new wxMenu();
		int base_id = GdaConst::ID_HTML_MENU_ENTRY_CHOICE_0;
		for (size_t i=0; i<htmlMenuItems.size(); ++i) {
			html_menu->Append(base_id++, htmlMenuItems[i].menu_title);
		}
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
    
    //CallAfter(&GdaFrame::ShowOpenDatasourceDlg,wxPoint(80, 220),true);
    
    // check update in a new thread
    if (GdaConst::disable_auto_upgrade == false) {
        CallAfter(&GdaFrame::CheckUpdate);
    }
}

GdaFrame::~GdaFrame()
{
	GdaFrame::gda_frame = 0;
}

void GdaFrame::CheckUpdate()
{
    wxLogMessage("Check auto update:");
    
    wxString version = AutoUpdate::CheckUpdate();
    if (!version.IsEmpty()) {
        wxLogMessage("current version:");
        wxLogMessage(version);
        
        wxString skip_version;
        std::vector<wxString> items = OGRDataAdapter::GetInstance().GetHistory("no_update_version");
        if (items.size()>0)
            skip_version = items[0];
        
        if (skip_version == version)
            return;
       
        bool showSkip = true;
        AutoUpdateDlg updateDlg(NULL, showSkip);
        if (updateDlg.ShowModal() == wxID_NO) {
            OGRDataAdapter::GetInstance().AddEntry("no_update_version", version);
        }
    }
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
		if (tb)
            tb->EnableTool(wxXmlResource::GetXRCID(id_str), enable);
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
	TableFrame* tf = 0;
	wxGrid* g = project_p->FindTableGrid();
	if (g) tf = (TableFrame*) g->GetParent()->GetParent(); // wxPanel<wxFrame
	if (!tf) {
		wxString msg = _("The Table should always be open, although somtimes it is hidden while the project is open.  This condition has been violated.  Please report this to the program developers.");
		wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING);
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
}

/** returns false if user wants to abort the operation */
bool GdaFrame::OnCloseProject(bool ignore_unsaved_changes)
{
	if (IsProjectOpen() && !ignore_unsaved_changes) {
        
		bool is_new_project = project_p->GetProjectFullPath().empty() || !wxFileExists(project_p->GetProjectFullPath());
        
		bool unsaved_meta_data = project_p->GetTableInt()->ProjectChangedSinceLastSave();
        
		bool unsaved_ds_data = project_p->GetTableInt()->ChangedSinceLastSave();
	
        
		wxString msg;
		wxString title;
        
		if (unsaved_ds_data || unsaved_meta_data) {
            title = _("Do you want to save your data?");
            msg << "\n";
            msg << _("There are unsaved data source or weights/time definition changes.");
            
            wxMessageDialog msgDlg(this, msg, title,
                                   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
            if (msgDlg.ShowModal() == wxID_YES) {
                if (project_p) {
                    if (unsaved_ds_data) {
                        try {
                            project_p->SaveDataSourceData();
                            try {
                                project_p->SaveProjectConf();
                            } catch( GdaException& e) {
                                // do nothing if save project configuration failed, since it's not critical
                            }
                        } catch (GdaException& e) {
                            // the title of the message dialog is empty, because
                            // the message could be an "eror" message or an "info" message
                            // e.g. "Saving data source cancelled"
                            wxMessageDialog dlg (this, e.what(), "", wxOK | wxICON_ERROR);
                            dlg.ShowModal();
                        }
                    }
                }
            }
		}
        
        
        if (project_p->IsDataTypeChanged()) {
            wxString msg = _("Geometries have been added to existing Table-only data source. Do you want to save them as a new datasource?");
            wxMessageDialog show_export_dlg(this, msg, _("Geometries not saved"),
                                            wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION);
            if (show_export_dlg.ShowModal() == wxID_YES) {
                // e.g. dbf, add geometries (points), to shapefile
                ExportDataDlg dlg(this, project_p);
                dlg.ShowModal();
            }
            
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
                    w->Close(true);
                    w->Destroy();
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
			w->DisconnectFromProject();
		}
        if (CreateGridDlg* w = dynamic_cast<CreateGridDlg*>(win)) {
            w->Close(true);
        }
        node = node->GetNext();
    }

	UpdateToolbarAndMenus();
	
    MapCanvas::ResetThumbnail();
	return true;
}


void GdaFrame::OnClose(wxCloseEvent& event)
{
	wxLogMessage("Click GdaFrame::OnClose");

    if (IsProjectOpen()) {
        wxString prj_full_path = project_p->GetProjectFullPath();
        bool is_new_project = prj_full_path.empty() || (wxFileExists(prj_full_path)==false);
        bool unsaved_meta_data = project_p->GetTableInt()->ProjectChangedSinceLastSave();
        bool unsaved_ds_data = project_p->GetTableInt()->ChangedSinceLastSave();
        
        wxString msg;
        wxString title;
        if (unsaved_ds_data || unsaved_meta_data) {
            title = _("Exit with unsaved changes?");
            msg << "\n";
            msg << _("There are unsaved data source or weights/time definition changes.");
            
        } else {
            title = _("Exit?");
            msg = _("OK to Exit?");
        }
        
        if (IsProjectOpen()) {
            wxMessageDialog msgDlg(this, msg, title,
                                   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
            // Show the message dialog, and if it returns wxID_YES...
            if (msgDlg.ShowModal() != wxID_YES)
                return;
        }
        
        OnCloseProject(true);
    }

	// Close windows not associated managed by Project
	wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
		if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
			w->Close(true);
		}
        if (CreateGridDlg* w = dynamic_cast<CreateGridDlg*>(win)) {
            w->Close(true);
        }
        node = node->GetNext();
    }
	
    // close GeoDa successfully, mark it
    OGRDataAdapter::GetInstance().AddEntry("NoCrash", "true");
    
    wxMilliSleep(100);
    Destroy();
}

void GdaFrame::OnMenuClose(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnMenuClose");
	Close(); // This will result in a call to OnClose
}

void GdaFrame::OnCloseProjectEvt(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnCloseProjectEvt");
	OnCloseProject();
}

void GdaFrame::UpdateRecentDatasourceMenu()
{
    try {
        // update recent opened datasource menu
        RecentDatasource recent_ds;
        std::vector<wxString> recent_ds_list = recent_ds.GetList();
        
        int recent_menu_xrcid = XRCID("ID_NEW_PROJ_FROM_RECENT_MENU");
        wxMenuBar* mb = GetMenuBar();
        if (mb == NULL)
            return;
        
        wxMenuItem* mi = mb->FindItem(recent_menu_xrcid);
        if (mi == NULL)
            return;
        
        wxMenu* recent_menu = mi->GetSubMenu();
        if (recent_menu == NULL)
            return;
        
        int n_recent_menuitems = recent_menu->GetMenuItemCount();
        
        for (int i=n_recent_menuitems-1; i >=0;  i--) {
            wxMenuItem* recent_item = recent_menu->FindItemByPosition(i);
            recent_menu->Destroy(recent_item);
        }
        
        for (size_t i=0; i<recent_ds_list.size(); i++ ) {
			if (recent_ds_list[i].IsEmpty()) continue;
            wxMenuItem* recent_item = recent_menu->Append(wxID_ANY,
                                                          recent_ds_list[i]);
            int xrc_id = recent_item->GetId();
            Connect(xrc_id, wxEVT_COMMAND_TOOL_CLICKED,
                    wxCommandEventHandler(GdaFrame::OnRecentDSClick));
        }
    } catch(GdaException ex) {
        // do nothing but write to LOG
    }
}

void GdaFrame::RemoveInvalidRecentDS()
{
    RecentDatasource recent_ds;
    recent_ds.DeleteLastRecord();
}

void GdaFrame::OnCustomCategoryClick(wxCommandEvent& event)
{
    int xrc_id = event.GetId();
    if (project_p) {
        CatClassifManager* ccm = project_p->GetCatClassifManager();
        if (!ccm) return;
        vector<wxString> titles;
        ccm->GetTitles(titles);
       
        int idx = xrc_id - GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0;
        if (idx < 0 || idx >= titles.size()) return;
            
        wxString cc_title = titles[idx];

        TemplateFrame* t = TemplateFrame::GetActiveFrame();
        if (!t) return;
        if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        } else if (ConditionalMapFrame* f = dynamic_cast<ConditionalMapFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        } else if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        } else if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        } else if (CartogramNewFrame* f = dynamic_cast<CartogramNewFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        } else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
            f->OnCustomCatClassifA(cc_title);
        }
    }
}

void GdaFrame::OnEmptyCustomCategoryClick(wxCommandEvent& event)
{
    int xrc_id = event.GetId();
    if (project_p) {
        CatClassifManager* ccm = project_p->GetCatClassifManager();
        if (!ccm) return;
        vector<wxString> titles;
        ccm->GetTitles(titles);

        int idx = xrc_id - GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0;
        if (idx < 0 || idx >= titles.size()) return;

        wxString cc_title = titles[idx];
        CatClassifState* cc_state = ccm->FindClassifState(cc_title);
        CatClassifDef& cc_def = cc_state->GetCatClassif();
        wxString fld_name = cc_def.assoc_db_fld_name;
        project_p->SetDefaultVarName(0, fld_name);
        
        VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate);
        if (dlg.ShowModal() != wxID_OK) return;
        MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
                                    dlg.var_info, dlg.col_ids,
                                    CatClassification::no_theme,
                                    MapCanvas::no_smoothing, 1,
                                    boost::uuids::nil_uuid(),
                                    wxDefaultPosition,
                                    GdaConst::map_default_size);
        nf->ChangeMapType(CatClassification::custom, MapCanvas::no_smoothing, 4, boost::uuids::nil_uuid(), true, dlg.var_info, dlg.col_ids, cc_title);
        nf->UpdateTitle();
    }
}

void GdaFrame::OnRecentDSClick(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnRecentDSClick");
    
    if (project_p)
        return;
    
    int xrc_id = event.GetId();
    wxMenuBar* mb = GetMenuBar();
    if (mb == NULL)
        return;
    
    wxMenuItem* mi = mb->FindItem(xrc_id);
    if (mi == NULL)
        return;
 
    wxString ds_name = mi->GetItemLabelText();
    if (ds_name.IsEmpty())
        return;
    
    wxLogMessage(ds_name);
    
    if (ds_name.EndsWith(".gda")) {
        OpenProject(ds_name);
        return;
    }
    
    RecentDatasource recent_ds;
    IDataSource* ds = recent_ds.GetDatasource(ds_name);
    if (ds == NULL) {
        // raise message dialog show can't connect to datasource
        wxString msg = _("Can't connect to datasource: ") + ds_name;
        wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString layername = recent_ds.GetLayerName(ds_name);
    if (layername.IsEmpty())
        return;
    
    try {
        wxString proj_title = layername;
        project_p = new Project(proj_title, layername, ds);
        
    } catch (GdaException& e) {
        RemoveInvalidRecentDS();
        wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    InitWithProject();
}

/** New Project opened by the user from outside the program, for example
 when a user double clicks on a datasource type associated with GeoDa, or
 when a user right clicks on a file and chooses "Open with GeoDa."  Can
 also be called from the shortcut "New Project From" File menu option. */
void GdaFrame::NewProjectFromFile(const wxString& full_file_path)
{
	wxString proj_title = wxFileName(full_file_path).GetName();
    wxString layer_name = proj_title;
	
	try {
		FileDataSource fds(full_file_path);
	
        // this datasource will be freed when dlg exit, so make a copy
        // in project_p
        project_p = new Project(proj_title, layer_name, &fds);
    } catch (GdaException& e) {
        RemoveInvalidRecentDS();
        wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
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

        wxMessageDialog dlg (this, error_msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
    
    InitWithProject();
}

/** New Project opened by the user from within GeoDa */
void GdaFrame::OnNewProject(wxCommandEvent& event)
{
	wxLogMessage("Click GdaFrame::OnNewProject");

    ShowOpenDatasourceDlg(wxPoint(80, 220));
}

void GdaFrame::ShowOpenDatasourceDlg(wxPoint pos, bool init)
{
    if (init && project_p) {
        return;
    }

    ConnectDatasourceDlg dlg(this, pos);
    if (dlg.ShowModal() != wxID_OK) {
        // when open a gda file, which already handles in
        // ConnectDatasoureDlg()
        // and the dlg will return wxID_CANCLE
        return;
	}
    
    wxString proj_title = dlg.GetProjectTitle();
    wxString layer_name = dlg.GetLayerName();
    IDataSource* datasource = dlg.GetDataSource();
    
    try {
        // this datasource will be freed when dlg exit, so make a copy
        // in project_p
        project_p = new Project(proj_title, layer_name, datasource);
       
        if (!project_p->IsValid()) {
            // do noting
            return;
        }
    } catch (GdaException& e) {
        RemoveInvalidRecentDS();
        wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    wxString error_msg;
    if (!project_p) {
        error_msg << _("Could not initialize new project.");
    } else if (!project_p->IsValid()) {
        error_msg << _("Could not initialize new project.");
        error_msg << project_p->GetOpenErrorMessage();
    }
    
    if (!error_msg.IsEmpty()) {
        delete project_p;
        project_p = NULL;
        wxMessageDialog dlg (this, error_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    InitWithProject();
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
	wxLogMessage("GdaFrame::OpenProject()");
    wxLogMessage(full_proj_path);
    
    wxString msg;
    wxFileName fn(full_proj_path);
    if (fn.GetExt().CmpNoCase("gda") != 0) {
        // open from a raw file/ds
        if (IsProjectOpen()) {
            Raise();
            msg = _("You have requested to create a new file project %s  while another project is open. Please close project %s and try again.");
            msg = wxString::Format(msg, full_proj_path, project_p->GetProjectTitle());
            return;
        }
        NewProjectFromFile(full_proj_path);
        return;
    }
    
    if (!wxFileExists(full_proj_path)) {
        msg = _("Error: \"%s\" not found.");
        msg = wxString::Format(msg, full_proj_path);
    }
    if (!msg.IsEmpty()) {
        LOG_MSG(msg);
        wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
	
	// if requested project to open is already open, just raise current window
	if (IsProjectOpen()) {
		if (project_p->GetProjectFullPath().CmpNoCase(full_proj_path) == 0) {
			Raise();
			return;
		}
		Raise();
		wxString msg;
        msg = _("You have requested to create a new file project %s  while another project is open. Please close project %s and try again.");
        msg = wxString::Format(msg, full_proj_path, project_p->GetProjectTitle());
        wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
		return;
	}

    
    if (project_p) {
        // close any existing windows and project
        FramesManager* fm = project_p->GetFramesManager();
		if (fm && fm->getNumberObservers() > 0)
            if(!OnCloseProject())
                return;
	}

    try {
        project_p = new Project(full_proj_path);
        if (!project_p->IsValid()) {
            RemoveInvalidRecentDS();
            wxString msg = _("Error while opening project:\n\n");
            msg << project_p->GetOpenErrorMessage();
            throw GdaException(msg.c_str());
        }

    } catch (GdaException &e) {
        wxString msg( e.what(), wxConvUTF8 ) ;
        wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		delete project_p;
        project_p = 0;
		return;
    }

    InitWithProject(full_proj_path);
}


void GdaFrame::OnOpenProject(wxCommandEvent& event)
{
	wxLogMessage("Click GdaFrame::OnOpenProject");
	if (project_p && project_p->GetFramesManager()->getNumberObservers() > 0) {
		if (!OnCloseProject())
            return;
	}
	wxString wildcard = "GeoDa Project (*.gda)|*.gda";
	wxFileDialog dlg(this, _("GeoDa Project File to Open"), "", "", wildcard);
	if (dlg.ShowModal() != wxID_OK)
        return;

	OpenProject(dlg.GetPath());
}

void GdaFrame::InitWithProject(wxString gda_file_path)
{
	wxLogMessage("Click GdaFrame::InitWithProject()");
    // By this point, we know that project has created as
    // TopFrameManager object with delete_if_empty = false
   
    RecentDatasource recent_ds;
    
    if (gda_file_path.IsEmpty()) {
        recent_ds.Add(project_p->GetDataSource(), project_p->GetProjectTitle());
    } else {
        recent_ds.Add(gda_file_path, gda_file_path, project_p->GetProjectTitle());
    }
    
    // This call is very improtant because we need the wxGrid to
    // take ownership of the TableBase instance (due to bug in wxWidgets)
    TableFrame* tf = new TableFrame(this, project_p, _("Table"),
                                    wxDefaultPosition,
                                    GdaConst::table_default_size,
                                    wxDEFAULT_FRAME_STYLE);
    if (project_p->IsTableOnlyProject()) {
        tf->Show(true);
        tf->Raise();
    }
    
    
    if (!project_p->IsTableOnlyProject()) {
        std::vector<int> col_ids;
        std::vector<GdaVarTools::VarInfo> var_info;
        WeightsManInterface* w_int = project_p->GetWManInt();
        boost::uuids::uuid w_id = boost::uuids::nil_uuid();
        if (w_int) {
            w_id = w_int->GetDefault();
        }
        MapFrame* nf = new MapFrame(GdaFrame::gda_frame, project_p,
                                    var_info, col_ids,
                                    CatClassification::no_theme,
                                    MapCanvas::no_smoothing, 1,
                                    w_id,
                                    wxPoint(80,160),
                                    GdaConst::map_default_size);
        nf->UpdateTitle();
    }
    
    SetProjectOpen(true);
    UpdateToolbarAndMenus();
    
    // Associate Project with Calculator if open
    wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
        if (CalculatorDlg* w = dynamic_cast<CalculatorDlg*>(win)) {
            w->ConnectToProject(GetProject());
        }
        node = node->GetNext();
    }
}

void GdaFrame::OnSaveProject(wxCommandEvent& event)
{
	wxLogMessage("Click GdaFrame::OnSaveProject");
	if (!project_p)
        return;
	try {
        project_p->SaveDataSourceData();
        // don't need to raise error if save project configuration failed
        try {
            project_p->SaveProjectConf();
        } catch( GdaException& e) {
            // do nothing if save project configuration failed,
            // since it's not critical
        }        
	} catch (GdaException& e) {
        // the title of the message dialog is empty, because
        // the message could be an "eror" message or an "info" message
        // e.g. "Saving data source cancelled"
		wxMessageDialog dlg (this, e.what(), "", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	// here, we know Data Source data was saved successfully
	SaveButtonManager* sbm = project_p->GetSaveButtonManager();
	if (sbm) {
		sbm->SetAllowEnableSave(true);
		sbm->SetMetaDataSaveNeeded(false);
	}
}

void GdaFrame::OnSaveAsProject(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSaveAsProject");
	if (!project_p || !project_p->GetTableInt())
        return;    
	wxString proj_fname = project_p->GetProjectFullPath();
	bool is_new_project = (proj_fname.empty() || !wxFileExists(proj_fname));

    if (is_new_project) {
        wxString msg = _("A project file contains extra information not directly stored in the data source such as variable order and grouping.");
        wxMessageDialog proj_create_dlg(this, msg, _("Create Project File Now?"),
                                        wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION);
        if (proj_create_dlg.ShowModal() != wxID_YES) return;
        wxString wildcard = _("GeoDa Project (*.gda)|*.gda");
        wxFileDialog dlg(this, _("New Project Filename"), "", "",
                         wildcard, wxFD_SAVE);
        if (dlg.ShowModal() != wxID_OK) return;
        try {
            proj_fname = dlg.GetPath();
            project_p->SpecifyProjectConfFile(proj_fname);
        } catch (GdaException& e) {
            wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
    }
	try {
        project_p->SaveProjectConf();
        wxString msg = _("Saved successfully.");
        wxMessageDialog dlg(this, msg , _("Info"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
	} catch (GdaException& e) {
		wxMessageDialog dlg (this, e.what(), _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	wxLogMessage(_("Wrote GeoDa Project File: ") + proj_fname);
}

void GdaFrame::OnSelectWithRect(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectWithRect");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    
    if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
        wxLogMessage("ScatterPlotMatFrame::OnSelectWithRect");
        f->OnSelectWithRect(event);
    } else {
        t->OnSelectWithRect(event);
    }
}

void GdaFrame::OnSelectWithCircle(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectWithCircle");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithCircle(event);
}

void GdaFrame::OnSelectWithLine(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectWithLine");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectWithLine(event);
}

void GdaFrame::OnSelectWithCustom(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectWithCustom");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    t->OnSelectWithCustom(event);
}

void GdaFrame::OnSelectionMode(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectionMode");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectionMode(event);
}

void GdaFrame::OnFitToWindowMode(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnFitToWindowMode");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnFitToWindowMode(event);
}

void GdaFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnFixedAspectRatioMode");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnFixedAspectRatioMode(event);
}

void GdaFrame::OnSetAxisDisplayPrecision(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSetDisplayPrecision");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnSetAxisDisplayPrecision(event);
}

void GdaFrame::OnZoomMode(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnZoomMode");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnZoomMode(event);
}

void GdaFrame::OnPanMode(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnPanMode");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPanMode(event);
}

void GdaFrame::OnPrintCanvasState(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnPrintCanvasState");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnPrintCanvasState(event);
	
	// Add this menu item to the XRC file to see this debugging option:
	//<object class="wxMenuItem" name="ID_PRINT_CANVAS_STATE">
	//  <label>Print Canvas State to Log File</label>
    //</object>s
}

void GdaFrame::OnChangeMapTransparency(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnChangeMapTransparency");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t)
        return;
    
	MapFrame* f = dynamic_cast<MapFrame*>(t);
    if (f)
        f->OnChangeMapTransparency();
}
void GdaFrame::OnCleanBasemap(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnCleanBasemap");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (t) {
        if (MapFrame* f = dynamic_cast<MapFrame*>(t)) return
            f->CleanBasemap();
    }
}
void GdaFrame::OnSetNoBasemap(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSetNoBasemap");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    
	MapFrame* f = dynamic_cast<MapFrame*>(t);
    if (f) f->SetNoBasemap();
}

void GdaFrame::OnBasemapConfig(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnBasemapConfig");
    BasemapConfDlg dlg(this,project_p);
    dlg.ShowModal();
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (t) t->OnRefreshMap(event);
}

void GdaFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSaveCanvasImageAs");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (t) t->OnSaveCanvasImageAs(event);
}

void GdaFrame::OnQuit(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnQuit");
	// Generate a wxCloseEvent for GdaFrame.  GdaFrame::OnClose will
	// be called and will give the user a chance to not exit program.
    // Close windows not associated managed by Project
	Close();
    event.Skip(false);
}

void GdaFrame::OnSaveSelectedToColumn(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSaveSelectedToColumn");
	SaveSelectionDlg dlg(project_p, this);
	dlg.ShowModal();
}

void GdaFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnCanvasBackgroundColor");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCanvasBackgroundColor(event);
}

void GdaFrame::OnLegendUseScientificNotation(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnLegendUseScientificNotation");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnLegendUseScientificNotation(event);
}

void GdaFrame::OnLegendDisplayPrecision(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnLegendDisplayPrecision");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    t->OnLegendDisplayPrecision(event);
}

void GdaFrame::OnLegendBackgroundColor(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnLegendBackgroundColor");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnLegendBackgroundColor(event);
}

void GdaFrame::OnSelectableFillColor(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectableFillColor");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
        f->OnSelectableFillColor(event);
    } else {
        t->OnSelectableFillColor(event);
    }
}

void GdaFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectableOutlineColor");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    
    if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
        f->OnSelectableOutlineColor(event);
    } else {
        t->OnSelectableOutlineColor(event);
    }
}

void GdaFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnSelectableOutlineVisible");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnSelectableOutlineVisible(event);
}

void GdaFrame::OnShowMapBoundary(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnShowMapBoundary");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnShowMapBoundary(event);
    }
}

void GdaFrame::OnHighlightColor(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnHighlightColor");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
        f->OnHighlightColor(event);
    } else {
        t->OnHighlightColor(event);
    }
}

void GdaFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnCopyImageToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnCopyImageToClipboard(event);
}


void GdaFrame::OnCopyLegendToClipboard(wxCommandEvent& event)
{
    wxLogMessage("Click GdaFrame::OnCopyLegendToClipboard");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    t->OnCopyLegendToClipboard(event);
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

void GdaFrame::OnToolsDataPCA(wxCommandEvent& WXUNUSED(event) )
{
	Project* p = GetProject();
	if (!p) return;
   
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (PCASettingsDlg* w = dynamic_cast<PCASettingsDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    PCASettingsDlg* dlg = new PCASettingsDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataKMeans(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (KMeansDlg* w = dynamic_cast<KMeansDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    KMeansDlg* dlg = new KMeansDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataKMedians(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (KMediansDlg* w = dynamic_cast<KMediansDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    KMediansDlg* dlg = new KMediansDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataKMedoids(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (KMedoidsDlg* w = dynamic_cast<KMedoidsDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    KMedoidsDlg* dlg = new KMedoidsDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataMaxP(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (MaxpDlg* w = dynamic_cast<MaxpDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    MaxpDlg* dlg = new MaxpDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataSkater(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (SkaterDlg* w = dynamic_cast<SkaterDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    SkaterDlg* dlg = new SkaterDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataRedcap(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (RedcapDlg* w = dynamic_cast<RedcapDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    RedcapDlg* dlg = new RedcapDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataMDS(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (MDSDlg* w = dynamic_cast<MDSDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    MDSDlg* dlg = new MDSDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataSpectral(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (SpectralClusteringDlg* w = dynamic_cast<SpectralClusteringDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    SpectralClusteringDlg* dlg = new SpectralClusteringDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataHCluster(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (HClusterDlg* w = dynamic_cast<HClusterDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    HClusterDlg* dlg = new HClusterDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsDataHDBScan(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    FramesManager* fm = p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (HDBScanDlg* w = dynamic_cast<HDBScanDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    HDBScanDlg* dlg = new HDBScanDlg(this, p);
    dlg->Show(true);
}

void GdaFrame::OnToolsWeightsManager(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("Click GdaFrame::OnToolsWeightsManager");
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (WeightsManFrame* w = dynamic_cast<WeightsManFrame*>(*it)) {
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	WeightsManFrame* f = new WeightsManFrame(this, p);
}

void GdaFrame::OnToolsWeightsCreate(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("Click GdaFrame::OnToolsWeightsCreate");
	Project* p = GetProject();
	if (!p || !p->GetTableInt()) return;
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (CreatingWeightDlg* w = dynamic_cast<CreatingWeightDlg*>(*it)) {
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	CreatingWeightDlg* dlg = new CreatingWeightDlg(0, p);
	dlg->Show(true);
}

void GdaFrame::OnConnectivityHistView(wxCommandEvent& event )
{
	boost::uuids::uuid id = GetWeightsId();
	if (id.is_nil()) return;
	ConnectivityHistFrame* f = new ConnectivityHistFrame(this, project_p, id);
}

void GdaFrame::OnConnectivityMapView(wxCommandEvent& event )
{
	boost::uuids::uuid id = GetWeightsId("Choose Weights for Connectivity Map");
	if (id.is_nil()) return;
    if (project_p->isTableOnly) return;
    ConnectivityMapFrame* f =
		new ConnectivityMapFrame(this, project_p, id,
								 wxDefaultPosition,
								 GdaConst::conn_map_default_size);
}

void GdaFrame::ShowConnectivityMapView(boost::uuids::uuid weights_id)
{
	if (!project_p || !project_p->GetWManInt() ||
		weights_id.is_nil()) return;
	
	ConnectivityMapFrame* f =
		new ConnectivityMapFrame(this, project_p, weights_id,
								 wxDefaultPosition,
								 GdaConst::conn_map_default_size);
}

void GdaFrame::OnMapChoices(wxCommandEvent& event)
{
	wxLogMessage("Entering GdaFrame::OnMapChoices");
	wxMenu* popupMenu = 0;
	if (GeneralWxUtils::isMac()) {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES");
	} else {
		popupMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_CHOICES_NO_ICONS");
	}
	
    if (popupMenu) {
        int m_id = popupMenu->FindItem(_("Custom Breaks"));
        wxMenuItem* mi = popupMenu->FindItem(m_id);
        if (mi) {
            wxMenu* sm = mi->GetSubMenu();
            if (sm) {
                // clean
                wxMenuItemList items = sm->GetMenuItems();
                for (int i=0; i<items.size(); i++) {
                    sm->Delete(items[i]);
                }
                vector<wxString> titles;
                CatClassifManager* ccm = project_p->GetCatClassifManager();
                ccm->GetTitles(titles);
               
                sm->Append(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"), _("Create New Custom"), _("Create new custom categories classification."));
                sm->AppendSeparator();
                
                for (size_t j=0; j<titles.size(); j++) {
                    wxMenuItem* new_mi = sm->Append(GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0+j, titles[j]);
                }
                GdaFrame::GetGdaFrame()->Bind(wxEVT_COMMAND_MENU_SELECTED,
                                              &GdaFrame::OnCustomCategoryClick,
                                              GdaFrame::GetGdaFrame(),
                                              GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0,
                                              GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0 + titles.size());
            }
        }
        PopupMenu(popupMenu, wxDefaultPosition);
    }
}

#include "DialogTools/CreateGridDlg.h"
void GdaFrame::OnShapePolygonsFromGrid(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("Open CreateGridDlg");
    // check if dialog has already been opened
    wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* win = node->GetData();
        if (CreateGridDlg* w = dynamic_cast<CreateGridDlg*>(win)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
        node = node->GetNext();
    }
    
    CreateGridDlg* dlg =  new CreateGridDlg(this);
    dlg->Show(true);
}

#include "DialogTools/Bnd2ShpDlg.h"
void GdaFrame::OnShapePolygonsFromBoundary(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("Open Bnd2ShpDlg");
	Bnd2ShpDlg dlg(this);
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
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            pt = w->GetPosition();
            opened = true;
        }
    }
    if (!opened) {
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
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            w->SetPosition(wxPoint(pt.x, pt.y + 130));
            opened =true;
            break;
        }
    }
    if (!opened) {
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	CatClassifFrame* dlg = new CatClassifFrame(this, project_p, false, true);
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return w;
		}
	}
	
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	TimeEditorDlg* dlg = new TimeEditorDlg(0, GetProject()->GetFramesManager(),
										   GetProject()->GetTableState(),
										   GetProject()->GetTableInt());
	dlg->Show(true);
}

void GdaFrame::OnMoveSelectedToTop(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnMoveSelectedToTop");
	if (!project_p || !project_p->FindTableBase()) return;
	project_p->FindTableBase()->MoveSelectedToTop();
}

void GdaFrame::OnInvertSelection(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnInvertSelection");
	if (!project_p || !project_p->FindTableBase()) return;
    
	HighlightState& hs = *project_p->GetHighlightState();
	hs.SetEventType(HLStateInt::invert);
	hs.notifyObservers();
}

void GdaFrame::OnClearSelection(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnClearSelection");
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	FieldNewCalcSheetDlg* dlg =
		new FieldNewCalcSheetDlg(GetProject(), this,
								 wxID_ANY, _("Calculator"),
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
        node = node->GetNext();
    }

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
	/*
	FramesManager* fm = p->GetFramesManager();
	std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
	std::list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (DataViewerEditFieldPropertiesDlg* w
			= dynamic_cast<DataViewerEditFieldPropertiesDlg*>(*it))
		{
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	*/
	DataViewerEditFieldPropertiesDlg
	dlg(p, wxDefaultPosition, wxSize(600, 400));
	dlg.ShowModal();
}

/*
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
			w->Show(true);
			w->Maximize(false);
			w->Raise();
			return;
		}
	}
	
	DataChangeTypeFrame* dlg = new DataChangeTypeFrame(this, p);
}
 */


void GdaFrame::OnMergeTableData(wxCommandEvent& event)
{
	if (!project_p || !project_p->FindTableBase()) return;

    FramesManager* fm = project_p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (MergeTableDlg* w = dynamic_cast<MergeTableDlg*>(*it))
        {
            w->Init();
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
	MergeTableDlg* dlg =  new MergeTableDlg(this,
                                            project_p,
                                            wxDefaultPosition);
	dlg->Show(true);
}

void GdaFrame::OnAggregateData(wxCommandEvent& event)
{
    if (!project_p || !project_p->FindTableBase()) return;
    
    FramesManager* fm = project_p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (AggregationDlg* w = dynamic_cast<AggregationDlg*>(*it))
        {
            w->Init();
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    AggregationDlg* dlg = new AggregationDlg(this, project_p);
    dlg->Show(true);
}

void GdaFrame::OnSpatialJoin(wxCommandEvent& event)
{
    if (!project_p || !project_p->FindTableBase()) return;
    
    if (project_p->IsTableOnlyProject()) {
        wxMessageDialog dlg (this,
                             _("Spatial Join does not work with Table only datasource."),
                             _("Info"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    if (project_p->GetMapLayerCount() == 0) {
        wxMessageDialog dlg (this,
                             _("Please load another layer using map window to apply Spatial Join."),
                             _("Info"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    SpatialJoinDlg dlg(this, project_p);
    if (dlg.ShowModal() == wxID_OK) {
        OnOpenNewTable();
    }
}

void GdaFrame::OnGroupingMap(wxCommandEvent& event)
{
    if (!project_p || !project_p->FindTableBase()) return;
    
    if (project_p->IsTableOnlyProject()) {
        wxMessageDialog dlg (this,
                             _("Hierachical Map does not work with Table only datasource."),
                             _("Info"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    
    HierachicalMapSelectDlg dlg(this, project_p);
    if (dlg.ShowModal() == wxID_OK) {
        HierachicalMapFrame* nf = new HierachicalMapFrame(this, project_p,
                                                    dlg.GetVarInfo(),
                                                    dlg.GetColIds(),
                                                    dlg.GetWUID(),
                                                    dlg.GetTitle(),
                                                    wxDefaultPosition,
                                                    GdaConst::map_default_size);
        nf->Show(true);
    }
}

void GdaFrame::OnExportSelectedToOGR(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt()) return;
    
    vector<int> selected_rows;
    project_p->GetSelectedRows(selected_rows);
    if ( selected_rows.empty() ) {
        wxMessageDialog dlg (this,
                             _("Please select features first."),
                             _("Info"), wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
        return;
    }
	ExportDataDlg dlg(this, project_p, true);
	dlg.ShowModal();
}

void GdaFrame::OnExportToOGR(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt())
        return;
	ExportDataDlg dlg(this, project_p);
	dlg.ShowModal();
}

/** Export to CSV.  This is not used currently. */
void GdaFrame::OnExportToCsvFile(wxCommandEvent& event)
{
	if (!project_p || !project_p->GetTableInt())
        return;
	ExportCsvDlg dlg(this, project_p);
	dlg.ShowModal();
}

void GdaFrame::OnShowProjectInfo(wxCommandEvent& event)
{
	if (!project_p) return;
	ProjectInfoDlg dlg(project_p);
	dlg.ShowModal();
}

void GdaFrame::OnPreferenceSetup(wxCommandEvent& event)
{
    if (!project_p) {
    	PreferenceDlg dlg(this);
    	dlg.Show(true);
    } else {
    	PreferenceDlg dlg(this, project_p->GetHighlightState(),
                          project_p->GetTableState());
    	dlg.Show(true);
    }
}

void GdaFrame::OnHtmlEntry(int entry)
{
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
    wxLogMessage("In GdaFrame::OnGeneratePointShpFile()");
    
    if (!GetProject() || !GetProject()->GetTableInt()) {
        return;
    }
	Project* p = GetProject();
	TableInterface* table_int = p->GetTableInt();
	VariableSettingsDlg VS(GetProject(), VariableSettingsDlg::bivariate,
                           false, false,
                           _("New Map Coordinates"),
                           _("First Variable (X/Longitude)"),
                           _("Second Variable (Y/Latitude)"));
    if (VS.ShowModal() != wxID_OK) {
        return;
    }
	std::vector<double> xs;
	std::vector<double> ys;
    std::vector<bool> undefs;
    std::vector<bool> undefs_x(p->GetNumRecords());
    std::vector<bool> undefs_y(p->GetNumRecords());
    
	table_int->GetColData(VS.col_ids[0], VS.var_info[0].time, xs, undefs_x);
	table_int->GetColData(VS.col_ids[1], VS.var_info[1].time, ys, undefs_y);

    for (int i=0; i<undefs_x.size(); i++) {
        bool undef = undefs_x[i] || undefs_y[i];
        undefs.push_back(undef);
    }
    
    if (p->IsTableOnlyProject()) {
        int n_rows = xs.size();
        
        // clean p->main_data
        if (!p->main_data.records.empty()) {
            p->main_data.records.clear();
        }
        p->main_data.header.shape_type = Shapefile::POINT_TYP;
        p->main_data.records.resize(n_rows);
        
    	double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
        bool is_first = true;
        for ( int i=0; i < n_rows; i++ ) {
            Shapefile::PointContents* pc = new Shapefile::PointContents();
            pc->shape_type = Shapefile::POINT_TYP;
            pc->x = xs[i];
            pc->y = ys[i];
            pc->shape_type = undefs[i] ? 0 : 1;
            p->main_data.records[i].contents_p = pc;
            
            if (undefs[i])
                continue;
            
            if (is_first) {
                min_x = max_x = xs[i];
                min_y = max_y = ys[i];
                is_first = false;
            }
            if ( xs[i] < min_x )
                min_x = xs[i];
            else if ( xs[i] > max_x )
                max_x = xs[i];
            
            if ( ys[i] < min_y )
                min_y = ys[i];
            else if ( ys[i] > max_y )
                max_y = ys[i];
        }
       
    	p->main_data.header.bbox_x_min = min_x;
    	p->main_data.header.bbox_y_min = min_y;
    	p->main_data.header.bbox_x_max = max_x;
    	p->main_data.header.bbox_y_max = max_y;
    	p->main_data.header.bbox_z_min = 0;
    	p->main_data.header.bbox_z_max = 0;
    	p->main_data.header.bbox_m_min = 0;
    	p->main_data.header.bbox_m_max = 0;
        
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
    } else {
        // for other cases, just save as points with table
        std::vector<GdaShape*> points;
        int n_rows = xs.size();
        for ( int i=0; i < n_rows; i++ ) {
            GdaShape* p = new GdaPoint(xs[i], ys[i]);
            points.push_back(p);
        }
        ExportDataDlg export_dlg(NULL, points, Shapefile::POINT_TYP, p,false);
        
        export_dlg.ShowModal();
    }
}

void GdaFrame::OnRegressionClassic(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnRegressionClassic()");
    Project* p = GetProject();
    if (p == NULL)
        return;
    
    FramesManager* fm = project_p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (RegressionDlg* w = dynamic_cast<RegressionDlg*>(*it))
        {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    RegressionDlg* dlg = new RegressionDlg(project_p, this);
    dlg->Show(true);
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
    wxLogMessage("In GdaFrame::DisplayRegression()");
    Project* p = GetProject();
    if (!p) return;
    
	RegressionReportDlg *regReportDlg = new RegressionReportDlg(this, dump);
	regReportDlg->Show(true);
	regReportDlg->m_textbox->SetSelection(0, 0);
}

void GdaFrame::OnCondPlotChoices(wxCommandEvent& WXUNUSED(event))
{
    wxLogMessage("In GdaFrame::OnCondPlotChoices()");
    Project* p = GetProject();
    if (!p) return;
    
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
}

void GdaFrame::OnClusteringChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
    wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_CLUSTERING_MENU");
    
    if (popupMenu) {
        Project* p = GetProject();
        bool proj_open = (p != 0);
        bool shp_proj = proj_open;
        
        GeneralWxUtils::EnableMenuItem(popupMenu, XRCID("ID_TOOLS_DATA_PCA"),shp_proj);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_KMEANS"), proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_KMEDIANS"), proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_KMEDOIDS"), proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_HCLUSTER"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_HDBSCAN"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_SPECTRAL"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_MAXP"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_SKATER"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_REDCAP"),proj_open);
        GeneralWxUtils::EnableMenuItem(popupMenu,XRCID("ID_TOOLS_DATA_MDS"),proj_open);
        
        PopupMenu(popupMenu, wxDefaultPosition);
    }
}

void GdaFrame::OnShowConditionalMapView(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
    int style = VariableSettingsDlg::ALLOW_STRING_IN_FIRST | VariableSettingsDlg::ALLOW_STRING_IN_SECOND | VariableSettingsDlg::ALLOW_EMPTY_IN_FIRST |  VariableSettingsDlg::ALLOW_EMPTY_IN_SECOND ;
	if (p->GetTableInt()->IsTimeVariant()) style = style | VariableSettingsDlg::SHOW_TIME;

    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, style,
                            _("Conditional Map Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"),
                            _("Map Theme"));
    
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalMapFrame* subframe =
	new ConditionalMapFrame(GdaFrame::gda_frame, project_p,
							dlg.var_info, dlg.col_ids,
							_("Conditional Map"), wxDefaultPosition,
							GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowConditionalHistView(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;

    int style = VariableSettingsDlg::ALLOW_STRING_IN_FIRST | VariableSettingsDlg::ALLOW_STRING_IN_SECOND | VariableSettingsDlg::ALLOW_EMPTY_IN_FIRST |  VariableSettingsDlg::ALLOW_EMPTY_IN_SECOND;
    if (p->GetTableInt()->IsTimeVariant()) style = style | VariableSettingsDlg::SHOW_TIME;

    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::trivariate, style,
                            _("Conditional Histogram Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"),
                            _("Histogram Variable"));
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalHistogramFrame* subframe =
	new ConditionalHistogramFrame(GdaFrame::gda_frame, project_p,
								  dlg.var_info, dlg.col_ids,
								  _("Conditional Histogram"), wxDefaultPosition,
								  GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowConditionalScatterView(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;

    int style = VariableSettingsDlg::ALLOW_STRING_IN_FIRST | VariableSettingsDlg::ALLOW_STRING_IN_SECOND | VariableSettingsDlg::ALLOW_EMPTY_IN_FIRST |  VariableSettingsDlg::ALLOW_EMPTY_IN_SECOND;
    if (p->GetTableInt()->IsTimeVariant()) style = style | VariableSettingsDlg::SHOW_TIME;

    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::quadvariate, style,
                            _("Conditional Scatter Plot Variables"),
                            _("Horizontal Cells"),
                            _("Vertical Cells"),
                            _("Independent Var (x-axis)"),
                            _("Dependent Var (y-axis)"));
    
	if (dlg.ShowModal() != wxID_OK) return;
	
	ConditionalScatterPlotFrame* subframe =
	new ConditionalScatterPlotFrame(GdaFrame::gda_frame, project_p,
									dlg.var_info, dlg.col_ids,
									_("Conditional Scatter Plot"),
									wxDefaultPosition,
									GdaConst::cond_view_default_size);
}

void GdaFrame::OnShowCartogramNewView(wxCommandEvent& WXUNUSED(event) )
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false, false,
                            _("Cartogram Variables"),
							_("Circle Size"), _("Circle Color"), "", "",
							true, false); // set second var from first
	if (dlg.ShowModal() != wxID_OK)
        return;
	
	CartogramNewFrame* subframe =
		new CartogramNewFrame(GdaFrame::gda_frame, project_p,
							  dlg.var_info, dlg.col_ids,
							  _("Cartogram"), wxDefaultPosition,
							  GdaConst::map_default_size);
}

void GdaFrame::OnCartogramImprove1(wxCommandEvent& event )
{
    wxLogMessage("In  GdaFrame::OnCartogramImprove1()");
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
    wxLogMessage("In  GdaFrame::OnCartogramImprove2()");
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
    wxLogMessage("In  GdaFrame::OnCartogramImprove3()");
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
    wxLogMessage("In  GdaFrame::OnCartogramImprove4()");
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
    wxLogMessage("In  GdaFrame::OnCartogramImprove5()");
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
    wxLogMessage("In  GdaFrame::OnCartogramImprove6()");
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
    Project* p = GetProject();
    if (!p) return;
    
    bool show_str_var = true;
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate,
                           // default values
                           false,false,_("Variable Settings"),"","","","",false,false,false,
                           show_str_var);
	if (VS.ShowModal() != wxID_OK)
        return;
	
	HistogramFrame* f = new HistogramFrame(GdaFrame::gda_frame, project_p,
										   VS.var_info, VS.col_ids,
                                           _("Histogram"),
										   wxDefaultPosition,
										   GdaConst::hist_default_size);
}

void GdaFrame::OnExploreScatterNewPlot(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate,
                            false,
                            false,
                            _("Scatter Plot Variables"),
							_("Independent Var X"),
                            _("Dependent Var Y"));
	if (dlg.ShowModal() != wxID_OK)
        return;
	
    wxString title;
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
							_("Bubble Chart Variables"),
							_("X-Axis"), _("Y-Axis"),
							_("Bubble Size"), _("Standard Deviation Color"),
							false, true); // set fourth variable from third
	if (dlg.ShowModal() != wxID_OK) return;
	
    wxString title;
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
                            _("Scatter Plot Matrix"), wxDefaultPosition,
                            GdaConst::scatterplot_default_size);
}

void GdaFrame::OnExploreTestMap(wxCommandEvent& WXUNUSED(event))
{
    /*
    MapFrame* subframe = new MapFrame(frame, project_p,
                                      MapCanvas::no_theme,
                                      MapCanvas::no_smoothing, 1,
                                      boost::uuids::nil_uuid(),
                                      wxDefaultPosition,
                                      GdaConst::map_default_size);
    subframe->UpdateTitle();
    
    TestMapFrame *subframe = new TestMapFrame(frame, project_p,
                                              "Test Map Frame",
                                              wxDefaultPosition,
                                              GdaConst::map_default_size,
                                              wxDEFAULT_FRAME_STYLE);
     */
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
                                        _("Box Plot"), wxDefaultPosition,
                                        size);
}

void GdaFrame::OnExplorePCP(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	PCPDlg dlg(p,this);
	if (dlg.ShowModal() != wxID_OK)
        return;
	PCPFrame* s = new PCPFrame(this, p, dlg.var_info, dlg.col_ids);
}

void GdaFrame::OnExplore3DP(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg dlg(p, VariableSettingsDlg::trivariate, false, false,
                            _("3D Scatter Plot Variables"), "X", "Y", "Z");
	if (dlg.ShowModal() != wxID_OK)
        return;
		
	C3DPlotFrame *subframe =
		new C3DPlotFrame(GdaFrame::gda_frame, p,
						 dlg.var_info, dlg.col_ids,
						 _("3D Plot"), wxDefaultPosition,
						 GdaConst::three_d_default_size,
						 wxDEFAULT_FRAME_STYLE);
}

void GdaFrame::OnExploreLineChart(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;

	LineChartFrame* f = new LineChartFrame(GdaFrame::gda_frame, project_p,
                                           _("Average Comparison Chart"),
                                           wxDefaultPosition,
                                           GdaConst::line_chart_default_size);
}

void GdaFrame::OnExploreCovScatterPlot(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate,
                           false, true, _("Variable Choice"), _("Variable"));
	if (VS.ShowModal() != wxID_OK)
        return;
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
    
    CorrelogramFrame* f = new CorrelogramFrame(GdaFrame::gda_frame, p,
                                               _("Correlogram"), wxDefaultPosition,
                                               GdaConst::scatterplot_default_size);
}

void GdaFrame::OnToolOpenNewTable(wxCommandEvent& WXUNUSED(event))
{
	OnOpenNewTable();
}

void GdaFrame::OnToolsChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;

    wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_TOOLS_MENU");

    if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
}

void GdaFrame::OnMoranMenuChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_MORAN_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
}

void GdaFrame::OnOpenMSPL(wxCommandEvent& event)
{
    wxLogMessage("Open LisaScatterPlotFrame (OnOpenMSPL).");
    
    Project* p = GetProject();
    if (!p) return;
  
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK)
        return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
   
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool cont = true;
    for (int i=0; i<p->GetNumRecords(); i++) {
        if (gw->gal[i].Size() == 0 ) {
            wxMessageDialog dlg (this, _("Moran scatter plot is not supported when isolates are present in weights. Do you want to continue by removing the isolates when compute Moran's I?"), _("Warning"), wxYES_NO | wxICON_WARNING);
            if (dlg.ShowModal() == wxID_NO) {
                cont = false;
            }
            break;
        }
    }
    if (!cont) {
        return;
    }
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info, VS.col_ids,
											  LisaCoordinator::univariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
													   project_p, lc);
}
void GdaFrame::OnOpenDiffMoran(wxCommandEvent& event)
{
    wxLogMessage("Open LisaScatterPlotFrame (OnOpenDiffMoran).");
    
    Project* p = GetProject();
    if (!p) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    TableInterface* table_int = p->GetTableInt();
    
    bool has_time = table_int->IsTimeVariant();
    if (has_time == false) {
        wxMessageDialog dlg (this, _("Differential Moran's I tests whether the change in a variable over time is spatially correlated.\n\nPlease first group variables by time period: Select Time --> Time Editor."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    DiffMoranVarSettingDlg VS(project_p);
    if (VS.ShowModal() != wxID_OK) {
        return;
    }
    
    int col_idx = VS.col_ids[0];
    if (table_int->GetColType(col_idx, 0) != GdaConst::double_type &&
        table_int->GetColType(col_idx, 0) != GdaConst::long64_type) {
        wxMessageDialog dlg (this, _("The selected variable is not numeric. Please select another variable."), _("Variable Type Error"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil())
        return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool cont = true;
    for (int i=0; i<p->GetNumRecords(); i++) {
        if (gw->gal[i].Size() == 0 ) {
            wxMessageDialog dlg (this, _("Moran scatter plot is not supported when isolates are present in weights. Do you want to continue by removing the isolates when compute Moran's I?"), _("Warning"), wxYES_NO | wxICON_WARNING);
            if (dlg.ShowModal() == wxID_NO) {
                cont = false;
            }
            break;
        }
    }
    if (!cont) {
        return;
    }
    
    LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
                                              VS.var_info, VS.col_ids,
                                              LisaCoordinator::differential,
                                              false);
    
    LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
                                                       project_p, lc);

}

void GdaFrame::OnOpenGMoran(wxCommandEvent& event)
{
    wxLogMessage("Open LisaScatterPlotFrame (OnOpenGMoran).");
    
    Project* p = GetProject();
    if (!p) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    bool show_weights = true;
    bool show_distance = false;
    wxString title = _("Bivariate Moran Variable Settings");
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, show_weights, show_distance, title);
	if (VS.ShowModal() != wxID_OK)
        return;
    
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil())
        return;
	
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool cont = true;
    for (int i=0; i<p->GetNumRecords(); i++) {
        if (gw->gal[i].Size() == 0 ) {
            wxMessageDialog dlg (this, _("Moran scatter plot is not supported when isolates are present in weights. Do you want to continue by removing the isolates when compute Moran's I?"), _("Warning"), wxYES_NO | wxICON_WARNING);
            if (dlg.ShowModal() == wxID_NO) {
                cont = false;
            }
            break;
        }
    }
    if (!cont) {
        return;
    }
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
											  VS.var_info, VS.col_ids,
                                              LisaCoordinator::bivariate,
											  false);
	
	LisaScatterPlotFrame *f = new LisaScatterPlotFrame(GdaFrame::gda_frame,
													   project_p, lc);
}

void GdaFrame::OnOpenMoranEB(wxCommandEvent& event)
{
    wxLogMessage("Open LisaScatterPlotFrame (OnOpenMoranEB).");
    Project* p = GetProject();
    if (!p) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true,
												 false,
												 _("Empirical Bayes Rate Standardization Variables"),
												 _("Event Variable"), _("Base Variable"));
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
	
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    bool cont = true;
    for (int i=0; i<p->GetNumRecords(); i++) {
        if (gw->gal[i].Size() == 0 ) {
            wxMessageDialog dlg (this, _("Moran scatter plot is not supported when isolates are present in weights. Do you want to continue by removing the isolates when compute Moran's I?"), _("Warning"), wxYES_NO | wxICON_WARNING);
            if (dlg.ShowModal() == wxID_NO) {
                cont = false;
            }
            break;
        }
    }
    if (!cont) {
        return;
    }
    
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
    
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_LISA_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
}

void GdaFrame::OnGetisMenuChoices(wxCommandEvent& WXUNUSED(event))
{
    Project* p = GetProject();
    if (!p) return;
    
	wxMenu* popupMenu = wxXmlResource::Get()->LoadMenu("ID_GETIS_MENU");
	
	if (popupMenu) PopupMenu(popupMenu, wxDefaultPosition);
}

void GdaFrame::OnOpenUniLocalGeary(wxCommandEvent& event)
{
    wxLogMessage("Open LocalGearyFrame (OnOpenUniLocalGeary).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
		
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
	LocalGearyWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
	
    
	LocalGearyCoordinator* lc = new LocalGearyCoordinator(w_id, p,
											  VS.var_info,
											  VS.col_ids,
											  LocalGearyCoordinator::univariate,
											  true, LWO.m_RowStand);


	if (LWO.m_ClustMap) {
		LocalGearyMapFrame *sf = new LocalGearyMapFrame(GdaFrame::gda_frame, p,
												  lc, true, false, false);
	}
	if (LWO.m_SigMap) {
		LocalGearyMapFrame *sf = new LocalGearyMapFrame(GdaFrame::gda_frame, p,
												  lc, false, false, false,
												  wxDefaultPosition);
	}
}

void GdaFrame::OnOpenMultiLocalGeary(wxCommandEvent& event)
{
    wxLogMessage("Open LocalGearyFrame (OnOpenMultiLocalGeary).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	MultiVariableSettingsDlg VS(p);
	if (VS.ShowModal() != wxID_OK) return;
    
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
		
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
	LocalGearyWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
	
    
	LocalGearyCoordinator* lc = new LocalGearyCoordinator(w_id, p,
											  VS.var_info,
											  VS.col_ids,
											  LocalGearyCoordinator::multivariate,
											  true, LWO.m_RowStand);

	if (LWO.m_ClustMap) {
		LocalGearyMapFrame *sf = new LocalGearyMapFrame(GdaFrame::gda_frame, p,
												  lc, true, false, false);
	}
	if (LWO.m_SigMap) {
		LocalGearyMapFrame *sf = new LocalGearyMapFrame(GdaFrame::gda_frame, p,
												  lc, false, false, false,
												  wxDefaultPosition);
	}
}
void GdaFrame::OnOpenUniLisa(wxCommandEvent& event)
{
    wxLogMessage("Open LisaMapFrame (OnOpenUniLisa).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
		
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap && !LWO.m_Moran) return;
	
    
	LisaCoordinator* lc = new LisaCoordinator(w_id, p,
											  VS.var_info,
											  VS.col_ids,
											  LisaCoordinator::univariate,
											  true, LWO.m_RowStand);

	if (LWO.m_Moran) {
		LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(GdaFrame::gda_frame,
															p, lc);
	}
	if (LWO.m_ClustMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, p,
												  lc, true, false, false);
	}
	if (LWO.m_SigMap) {
		LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, p,
												  lc, false, false, false,
												  wxDefaultPosition);
	}
}

void GdaFrame::OnOpenMultiLisa(wxCommandEvent& event)
{
    wxLogMessage("Open LisaMapFrame (OnOpenMultiLisa).");
    
    Project* project = GetProject();
    if (!project) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
        
    }
    
    VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true,
                           false);
    if (VS.ShowModal() != wxID_OK) return;
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil()) return;
   
    GalWeight* gw = w_man_int->GetGal(w_id);
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    LisaWhat2OpenDlg LWO(this);
    if (LWO.ShowModal() != wxID_OK) return;
    if (!LWO.m_ClustMap && !LWO.m_SigMap &&!LWO.m_Moran) return;
    
    LisaCoordinator* lc = new LisaCoordinator(w_id, project_p,
                                              VS.var_info,
                                              VS.col_ids,
                                              LisaCoordinator::bivariate,
                                              true, LWO.m_RowStand);
    
    if (LWO.m_Moran) {
        LisaScatterPlotFrame *sf = new LisaScatterPlotFrame(GdaFrame::gda_frame,
                                                            project_p, lc);
    }
    if (LWO.m_ClustMap) {
        LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
                                            lc, true, true, false);
    }
    
    if (LWO.m_SigMap) {
        LisaMapFrame *sf = new LisaMapFrame(GdaFrame::gda_frame, project_p,
                                            lc, false, true, false);
    }
}

void GdaFrame::OnOpenDiffLisa(wxCommandEvent& event)
{
    wxLogMessage("Open LisaMapFrame (OnOpenDiffLisa).");
    
    Project* p = GetProject();
    if (!p) return;
    
	//VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true, false);
    
    Project* project = GetProject();
    TableInterface* table_int = project->GetTableInt();
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
        
    }
    
    bool has_time = table_int->IsTimeVariant();
    if (has_time == false) {
        wxMessageDialog dlg (this, _("Differential Moran's I tests whether the change in a variable over time is spatially correlated.\n\nPlease first group variables by time period: Select Time --> Time Editor."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    DiffMoranVarSettingDlg VS(project_p);
	if (VS.ShowModal() != wxID_OK) return;
    
    int col_idx = VS.col_ids[0];
    if (table_int->GetColType(col_idx, 0) != GdaConst::double_type &&
        table_int->GetColType(col_idx, 0) != GdaConst::long64_type) {
        wxString msg = _("The selected variable is not numeric. Please select another variable.");
        wxMessageDialog dlg (this, msg, _("Variable Type Error"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;

	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap &&!LWO.m_Moran) return;
	
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
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
    wxLogMessage("Open LisaMapFrame (OnOpenLisaEB).");
    
    Project* p = GetProject();
    if (!p) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxString msg = _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager.");
        wxMessageDialog dlg (this, msg, _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	// Note: this is the only call to this particular constructor
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate,
                           true,false,
                           _("Rates Variable Settings"),
                           _("Event Variable"), _("Base Variable"));
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
	
    GalWeight* gw = w_man_int->GetGal(w_id);
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
	LisaWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_Moran && !LWO.m_SigMap) return;
	
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

void GdaFrame::OnOpenLocalJoinCount(wxCommandEvent& event)
{
    wxLogMessage("Open OnOpenLocalJoinCount().");
    Project* p = GetProject();
    if (!p) return;

    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxString msg = _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager.");
        wxMessageDialog dlg (this, msg, _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false, "Binary Variable Settings");
    if (VS.ShowModal() != wxID_OK) return;
    
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil()) return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    // check if binary data
    std::vector<double> data;
    TableInterface* table_int = p->GetTableInt();
    table_int->GetColData(VS.col_ids[0], VS.var_info[0].time, data);
    for (int i=0; i<data.size(); i++) {
        if (data[i] !=0 && data[i] != 1) {
            wxString msg = _("Please select a binary variable for Local Join Count.");
            wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
            dlg.ShowModal();
            return;
        }
    }
    
    JCCoordinator* lc = new JCCoordinator(w_id, p, VS.var_info, VS.col_ids);
    MLJCMapFrame *sf = new MLJCMapFrame(GdaFrame::gda_frame, p, lc, false);
}

void GdaFrame::OnOpenBivariateLJC(wxCommandEvent& event)
{
    wxLogMessage("Enter OnOpenBivariateLJC()");
    
    Project* p = GetProject();
    if (!p) return;
   
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
        
    }
    
    VariableSettingsDlg VS(project_p, VariableSettingsDlg::bivariate, true, false);
    if (VS.ShowModal() != wxID_OK) return;
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil()) return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    // check if binary data
    std::vector<double> data;
    TableInterface* table_int = p->GetTableInt();
    table_int->GetColData(VS.col_ids[0], VS.var_info[0].time, data);
    for (int i=0; i<data.size(); i++) {
        if (data[i] !=0 && data[i] != 1) {
            wxString msg = _("Please select two binary variables for Bivariate Local Join Count.");
            wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
            dlg.ShowModal();
            return;
        }
    }
    table_int->GetColData(VS.col_ids[1], VS.var_info[1].time, data);
    for (int i=0; i<data.size(); i++) {
        if (data[i] !=0 && data[i] != 1) {
            wxString msg = _("Please select two binary variables for Bivariate Local Join Count.");
            wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
            dlg.ShowModal();
            return;
        }
    }
    
	JCCoordinator* lc = new JCCoordinator(w_id, p, VS.var_info, VS.col_ids);
    MLJCMapFrame *sf = new MLJCMapFrame(GdaFrame::gda_frame, p, lc, false);
}

void GdaFrame::OnOpenMultiLJC(wxCommandEvent& event)
{
    wxLogMessage("Open OnOpenMultiLJC (OnOpenMultiLJC).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxMessageDialog dlg (this, _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager."), _("No Weights Found"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    MultiVariableSettingsDlg VS(p);
    if (VS.ShowModal() != wxID_OK) return;
    
    boost::uuids::uuid w_id = VS.GetWeightsId();
    if (w_id.is_nil()) return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxMessageDialog dlg (this, _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager\n to define a valid weights file."), _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
   
    int num_vars = VS.var_info.size();
    
    // check if binary data
    std::vector<double> data;
    TableInterface* table_int = p->GetTableInt();
    for (int c=0; c<num_vars; c++) {
        table_int->GetColData(VS.col_ids[c], VS.var_info[c].time, data);
        for (int i=0; i<data.size(); i++) {
            if (data[i] !=0 && data[i] != 1) {
                wxString msg = _("Please select binary variables for Multivariate Local Join Count.");
                wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
                dlg.ShowModal();
                return;
            }
        }
    }
    
    // check if more than 2 variables has colocation
    if (num_vars > 2) {
        std::vector<d_array_type> data(num_vars); // data[variable][time][obs]
        std::vector<b_array_type> undef_data(num_vars);
        for (int i=0; i<VS.var_info.size(); i++) {
            table_int->GetColData(VS.col_ids[i], data[i]);
            table_int->GetColUndefined(VS.col_ids[i], undef_data[i]);
        }
        GalElement* W = gw->gal;
        int t = 0;
        int num_obs = p->GetNumRecords();
        if (p->GetTimeState()) t = p->GetTimeState()->GetCurrTime();
        vector<int> local_t;
        for (int v=0; v<num_vars; v++) {
            if (data[v].size()==1) {
                local_t.push_back(0);
            } else {
                local_t.push_back(t);
            }
        }
        vector<bool> undefs;
        for (int i=0; i<num_obs; i++){
            bool is_undef = false;
            for (int v=0; v<undef_data.size(); v++) {
                for (int var_t=0; var_t<undef_data[v].size(); var_t++){
                    is_undef = is_undef || undef_data[v][var_t][i];
                }
            }
            undefs.push_back(is_undef);
        }
        int* zz = new int[num_obs];
        for (int i=0; i<num_obs; i++) zz[i] = 1;
        for (int i=0; i<num_obs; i++) {
            if (undefs[i] == true) {
                zz[i] = 0;
                continue;
            }
            for (int v=0; v<num_vars; v++) {
                int _t = local_t[v];
                int _v = data[v][_t][i];
                zz[i] = zz[i] * _v;
            }
        }
        int sum = 0;
        for (int i=0; i<num_obs; i++) {
            sum += zz[i];
        }
        bool nocolocation = sum == 0;
        if (nocolocation) {
            wxMessageDialog dlg (this, _("Multivariate Local Join Count only applies to co-location case. The selected variables have no co-location. Please change your selection, or use Univariate/Bivariate Local Join Count."), _("Error"), wxOK | wxICON_WARNING);
            dlg.ShowModal();
            return;
        }
    }
    
    
    JCCoordinator* lc = new JCCoordinator(w_id, p, VS.var_info, VS.col_ids);
    MLJCMapFrame *sf = new MLJCMapFrame(GdaFrame::gda_frame, p, lc, false);
}

void GdaFrame::OnOpenGetisOrdStar(wxCommandEvent& event)
{
    wxLogMessage("Open GetisOrdMapFrame (OnOpenGetisOrdStar).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxString msg = _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager.");
        wxMessageDialog dlg (this, msg, "No Weights Found", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
   
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
  
    
    
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
    wxLogMessage("Open GetisOrdMapFrame (OnOpenGetisOrd).");
    
    Project* p = GetProject();
    if (!p) return;
    
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = p->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (weights_ids.size()==0) {
        wxString msg = _("GeoDa could not find the required weights file. \nPlease specify weights in Tools > Weights Manager.");
        wxMessageDialog dlg (this, msg, "No Weights Found", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
	VariableSettingsDlg VS(project_p, VariableSettingsDlg::univariate, true, false);
	if (VS.ShowModal() != wxID_OK) return;
	boost::uuids::uuid w_id = VS.GetWeightsId();
	if (w_id.is_nil()) return;
    
    GalWeight* gw = w_man_int->GetGal(w_id);
    
    if (gw == NULL) {
        wxString msg = _("Invalid Weights Information:\n\n The selected weights file is not valid.\n Please choose another weights file, or use Tools > Weights > Weights Manager to define a valid weights file.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
		
	GetisWhat2OpenDlg LWO(this);
	if (LWO.ShowModal() != wxID_OK) return;
	if (!LWO.m_ClustMap && !LWO.m_SigMap) return;
    
	GStatCoordinator* gc = new GStatCoordinator(w_id, project_p,
                                                VS.var_info, VS.col_ids,
                                                LWO.m_RowStand);
    
	if (!gc || !gc->IsOk()) {
		// print error message
		delete gc;
		return;
	}
    
    if (LWO.m_NormMap && LWO.m_ClustMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,
                                                   GetisOrdMapFrame::Gi_clus_norm,
                                                   LWO.m_RowStand);
    }
    if (LWO.m_NormMap && LWO.m_SigMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,
                                                   GetisOrdMapFrame::Gi_sig_norm,
                                                   LWO.m_RowStand);
    }
    if (!LWO.m_NormMap && LWO.m_ClustMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,
                                                   GetisOrdMapFrame::Gi_clus_perm,
                                                   LWO.m_RowStand);
    }
    if (!LWO.m_NormMap && LWO.m_SigMap) {
        GetisOrdMapFrame* f = new GetisOrdMapFrame(this, project_p, gc,
                                                   GetisOrdMapFrame::Gi_sig_perm,
                                                   LWO.m_RowStand);
    }
}

void GdaFrame::OnNewCustomCatClassifA(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnNewCustomCatClassifA()");
    
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
    wxLogMessage("In GdaFrame::OnNewCustomCatClassifB()");
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifB();
	}
}

void GdaFrame::OnNewCustomCatClassifC(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnNewCustomCatClassifC()");
    
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnNewCustomCatClassifC();
	}
}

void GdaFrame::OnCCClassifA(int cc_menu_num)
{
    wxLogMessage("In GdaFrame::OnCCClassifA()");
    
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;

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

void GdaFrame::OnCustomCategoryClick_B(wxCommandEvent& event)
{
    int xrc_id = event.GetId();
    
    if (project_p) {
        CatClassifManager* ccm = project_p->GetCatClassifManager();
        if (!ccm) return;
        vector<wxString> titles;
        ccm->GetTitles(titles);
        
        int idx = xrc_id - GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0;
        if (idx < 0 || idx >= titles.size()) return;
        
        OnCCClassifB(idx);
    }
}

void GdaFrame::OnCustomCategoryClick_C(wxCommandEvent& event)
{
    int xrc_id = event.GetId();
    
    if (project_p) {
        CatClassifManager* ccm = project_p->GetCatClassifManager();
        if (!ccm) return;
        vector<wxString> titles;
        ccm->GetTitles(titles);
        
        int idx = xrc_id - GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0;
        if (idx < 0 || idx >= titles.size()) return;
        
        OnCCClassifC(idx);
    }
}
void GdaFrame::OnCCClassifB(int cc_menu_num)
{
    wxLogMessage("In GdaFrame::OnCCClassifB()");
    
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifB(titles[cc_menu_num]);
	}	
}

void GdaFrame::OnCCClassifC(int cc_menu_num)
{
    wxLogMessage("In GdaFrame::OnCCClassifC()");
    
	CatClassifManager* ccm = project_p->GetCatClassifManager();
	if (!ccm) return;
	std::vector<wxString> titles;
	ccm->GetTitles(titles);
	if (cc_menu_num < 0 || cc_menu_num >= (int)titles.size()) return;
	
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->OnCustomCatClassifC(titles[cc_menu_num]);
	}	
}

void GdaFrame::OnOpenThemelessMap(wxCommandEvent& event)
{
    wxLogMessage("Open Themeless Map (OnOpenThemelessMap).");
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
    wxLogMessage("In GdaFrame::OnThemelessMap()");
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
    wxLogMessage(wxString::Format("In GdaFrame::OpenQuantile(%d)", num_cats));
    
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToQuantile(%d)", num_cats));
    
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
    wxLogMessage("In GdaFrame::OnOpenPercentile()");
    
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
    wxLogMessage("In GdaFrame::OnPercentile()");
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
    wxLogMessage("In GdaFrame::OnOpenHinge15()");
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
    wxLogMessage("In GdaFrame::OnHinge15()");
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
    wxLogMessage("In GdaFrame::OnOpenHinge30()");
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
    wxLogMessage("In GdaFrame::OnHinge30()");
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
    wxLogMessage("In GdaFrame::OnOpenStddev()");
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
    wxLogMessage("In GdaFrame::OnStddev()");
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
    wxLogMessage(wxString::Format("In GdaFrame::OpenNaturalBreaks(%d)", num_cats));
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToNaturalBreaks(%d)", num_cats));
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
    wxLogMessage(wxString::Format("In GdaFrame::OpenEqualIntervals(%d)", num_cats));
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToEqualIntervals(%d)", num_cats));
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
    wxLogMessage("In GdaFrame::OnOpenUniqueValues()");
    bool show_str_var = true;
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::univariate,
                            // default values
                            false, false, _("Variable Settings"), "", "","","",false, false, false,
                            show_str_var);
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

void GdaFrame::OnOpenColocationMap(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnOpenColocationMap()");
    FramesManager* fm = project_p->GetFramesManager();
    std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (ColocationSelectDlg* w = dynamic_cast<ColocationSelectDlg*>(*it)) {
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            return;
        }
    }
    
    ColocationSelectDlg* dlg = new ColocationSelectDlg(this, project_p);
    dlg->Show(true);
}

void GdaFrame::OnUniqueValues(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnUniqueValues()");
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
    wxLogMessage("In GdaFrame::OnCondVertThemelessMap()");
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondVertQuant(%d)", num_cats));
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::quantile, num_cats);
	}
}

void GdaFrame::OnCondVertPercentile(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondVertPercentile()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::percentile, 6);
	}
}

void GdaFrame::OnCondVertHinge15(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondVertHinge15()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_15, 6);
	}
}

void GdaFrame::OnCondVertHinge30(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondVertHinge30()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::hinge_30, 6);
	}
}

void GdaFrame::OnCondVertStddev(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondVertStddev()");
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondVertNatBrks(%d)", num_cats));
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondVertEquInts(%d)", num_cats));
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::equal_intervals, num_cats);
	}
}

void GdaFrame::OnCondVertUniqueValues(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondVertUniqueValues()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeVertThemeType(CatClassification::unique_values, 4);
	}
}

void GdaFrame::OnCondHorizThemelessMap(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizThemelessMap()");
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondHorizQuant(%d)", num_cats));
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::quantile, num_cats);
	}
}

void GdaFrame::OnCondHorizPercentile(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizPercentile()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::percentile, 6);
	}
}

void GdaFrame::OnCondHorizHinge15(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizHinge15()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_15, 6);
	}
}

void GdaFrame::OnCondHorizHinge30(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizHinge30()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::hinge_30, 6);
	}
}

void GdaFrame::OnCondHorizStddev(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizStddev()");
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondHorizNatBrks(%d)", num_cats));
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
    wxLogMessage(wxString::Format("In GdaFrame::ChangeToCondHorizEquInts(%d)", num_cats));
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::equal_intervals, num_cats);
	}
}

void GdaFrame::OnCondHorizUniqueValues(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCondHorizUniqueValues()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
		f->ChangeHorizThemeType(CatClassification::unique_values, 4);
	}
}


void GdaFrame::OnSaveCategories(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSaveCategories()");
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
    wxLogMessage("In GdaFrame::OnOpenRawrate()");
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
    wxLogMessage("In GdaFrame::OnRawrate()");
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
    wxLogMessage("In GdaFrame::OnOpenExcessrisk()");
    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::bivariate, false,
                            false,
                            _("Excess Risk Map Variable Settings"),
                            _("Event Variable"), _("Base Variable"));
    
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
    wxLogMessage("In GdaFrame::OnExcessrisk()");
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
    wxLogMessage("In GdaFrame::OnOpenEmpiricalBayes()");
    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
                            false, false,
                            _("Empirical Bayes Smoothed Variable Settings"),
                            _("Event Variable"), _("Base Variable"));
    
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
    wxLogMessage("In GdaFrame::OnEmpiricalBayes()");
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
    wxLogMessage("In GdaFrame::OnOpenSpatialRate()");
	VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
                            true, false,
                            _("Spatial Rate Smoothed Variable Settings"),
                            _("Event Variable"), _("Base Variable"));
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
    wxLogMessage("In GdaFrame::OnSpatialRate()");
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
    wxLogMessage("In GdaFrame::OnOpenSpatialEmpiricalBayes()");
    VariableSettingsDlg dlg(project_p, VariableSettingsDlg::rate_smoothed,
                            true, false,
                            _("Empirical Spatial Rate Smoothed Variable Settings"),
                            _("Event Variable"), _("Base Variable"));
    
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
    wxLogMessage("In OnSpatialEmpiricalBayes()");
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
    wxLogMessage("In OnSaveResults()");
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
    wxLogMessage("In OnSelectIsolates()");
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
    wxLogMessage("In OnSaveConnectivityToTable()");
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
    wxLogMessage("In OnHistogramIntervals()");
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
    wxLogMessage("In OnRan99Per()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
		f->OnRan99Per(event);
	} else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
		f->OnRan99Per(event);
	}
}

void GdaFrame::OnRan199Per(wxCommandEvent& event)
{
    wxLogMessage("In OnRan199Per()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan199Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan199Per(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnRan199Per(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnRan199Per(event);
    }
}

void GdaFrame::OnRan499Per(wxCommandEvent& event)
{
    wxLogMessage("In OnRan499Per()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (LisaScatterPlotFrame* f
			   = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan499Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan499Per(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnRan499Per(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnRan499Per(event);
    }
}

void GdaFrame::OnRan999Per(wxCommandEvent& event)
{
    wxLogMessage("In OnRan999Per()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRan999Per(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRan999Per(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnRan999Per(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnRan999Per(event);
    }
}

void GdaFrame::OnRanOtherPer(wxCommandEvent& event)
{
    wxLogMessage("In OnRanOtherPer()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (LisaScatterPlotFrame* f
			  = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnRanOtherPer(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnRanOtherPer(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnRanOtherPer(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnRanOtherPer(event);
    }
}

void GdaFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
    wxLogMessage("In OnUseSpecifiedSeed()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnUseSpecifiedSeed(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnUseSpecifiedSeed(event);
    } else if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
        f->OnUseSpecifiedSeed(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnUseSpecifiedSeed(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnUseSpecifiedSeed(event);
    }
}

void GdaFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
    wxLogMessage("In OnSpecifySeedDlg()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
	} else if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSpecifySeedDlg(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSpecifySeedDlg(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSpecifySeedDlg(event);
    }
}

void GdaFrame::OnDisplayPrecision(wxCommandEvent& event)
{
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
        f->OnDisplayPrecision(event);
    } else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
        f->OnDisplayPrecision(event);
    } else if (BoxPlotFrame* f = dynamic_cast<BoxPlotFrame*>(t)) {
        f->OnDisplayPrecision(event);
    } else if (HistogramFrame* f = dynamic_cast<HistogramFrame*>(t)) {
        f->OnDisplayPrecision(event);
    } else if (ConditionalNewFrame* f = dynamic_cast<ConditionalNewFrame*>(t)) {
        f->OnDisplayPrecision(event);
    } else if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
        f->OnDisplayPrecision(event);
    }
}

void GdaFrame::OnSaveMoranI(wxCommandEvent& event)
{
    wxLogMessage("In OnSaveMoranI()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnSaveMoranI(event);
	}
}

void GdaFrame::OnSigFilter05(wxCommandEvent& event)
{
    wxLogMessage("In OnSigFilter05()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter05(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter05(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSigFilter05(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSigFilter05(event);
    }
}

void GdaFrame::OnSigFilter01(wxCommandEvent& event)
{
    wxLogMessage("In OnSigFilter01()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter01(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter01(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSigFilter01(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSigFilter01(event);
    }
}

void GdaFrame::OnSigFilter001(wxCommandEvent& event)
{
    wxLogMessage("In OnSigFilter001()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter001(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSigFilter001(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSigFilter001(event);
    }
}

void GdaFrame::OnSigFilter0001(wxCommandEvent& event)
{
    wxLogMessage("In OnSigFilter0001()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSigFilter0001(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSigFilter0001(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSigFilter0001(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSigFilter0001(event);
    }
}

void GdaFrame::OnSigFilterSetup(wxCommandEvent& event)
{
    wxLogMessage("In OnSigFilterSetup()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
        f->OnSigFilterSetup(event);
    } else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
        f->OnSigFilterSetup(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSigFilterSetup(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSigFilterSetup(event);
    }
}

void GdaFrame::OnAddMeanCenters(wxCommandEvent& event)
{
    wxLogMessage("In OnAddMeanCenters()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->GetProject()->AddMeanCenters();
	}
}

void GdaFrame::OnAddCentroids(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnAddCentroids()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->GetProject()->AddCentroids();
	}
}

void GdaFrame::OnDisplayMeanCenters(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayMeanCenters()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayMeanCenters();
	}
}

void GdaFrame::OnDisplayWeightsGraph(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayWeightsGraph()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnDisplayWeightsGraph(event);
    }
}

void GdaFrame::OnDisplayMapWithGraph(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayMapWithGraph()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnDisplayMapWithGraph(event);
    }
}

void GdaFrame::OnChangeGraphThickness(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeGraphThickness()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnChangeGraphThickness(event);
    }
}

void GdaFrame::OnChangeGraphColor(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeGraphColor()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnChangeGraphColor(event);
    }
}

void GdaFrame::OnChangeConnSelectedColor(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeConnSelectedColor()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnChangeConnSelectedColor(event);
    }
}

void GdaFrame::OnChangeConnRootSize(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeConnRootSize()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (HierachicalMapFrame* f = dynamic_cast<HierachicalMapFrame*>(t)) {
        f->OnChangeConnRootSize(event);
    }
}

void GdaFrame::OnChangeConnRootColor(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeConnRootColor()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (HierachicalMapFrame* f = dynamic_cast<HierachicalMapFrame*>(t)) {
        f->OnChangeConnRootColor(event);
    }
}

void GdaFrame::OnChangeNeighborFillColor(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnChangeNeighborFillColor()");
    TemplateFrame* t = TemplateFrame::GetActiveFrame();
    if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnChangeNeighborFillColor(event);
    }
}

void GdaFrame::OnDisplayCentroids(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayCentroids()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayCentroids();
	}
}

void GdaFrame::OnDisplayVoronoiDiagram(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayVoronoiDiagram()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnDisplayVoronoiDiagram();
	}
}

void GdaFrame::OnExportVoronoi(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnExportVoronoi()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnExportVoronoi();
	}
}

void GdaFrame::OnExportMeanCntrs(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnExportMeanCntrs()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnExportMeanCntrs();
    }
}

void GdaFrame::OnExportCentroids(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnExportCentroids()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
        f->OnExportCentroids();
    }
}

void GdaFrame::OnSaveVoronoiDupsToTable(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSaveVoronoiDupsToTable()");
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
    wxLogMessage("In GdaFrame::OnSaveGetisOrd()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSaveGetisOrd(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSaveMLJC(event);
    }
}

void GdaFrame::OnSaveLisa(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSaveLisa()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSaveResult(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSaveLocalGeary(event);
    }
}

void GdaFrame::OnSaveColocation(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSaveColocation()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ColocationMapFrame* f = dynamic_cast<ColocationMapFrame*>(t)) {
		f->OnSave(event);
    }
}

void GdaFrame::OnSelectCores(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSelectCores()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCores(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSelectCores(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSelectCores(event);
    }
}

void GdaFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSelectNeighborsOfCores()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectNeighborsOfCores(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSelectNeighborsOfCores(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSelectNeighborsOfCores(event);
    }
}

void GdaFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSelectCoresAndNeighbors()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnSelectCoresAndNeighbors(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnSelectCoresAndNeighbors(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnSelectCoresAndNeighbors(event);
    }
    
}

void GdaFrame::OnAddNeighborToSelection(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnAddNeighborToSelection()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (MapFrame* f = dynamic_cast<MapFrame*>(t)) {
		f->OnAddNeighborToSelection(event);
    } else if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnAddNeighborToSelection(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnAddNeighborToSelection(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnAddNeighborToSelection(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnAddNeighborToSelection(event);
    }
}

void GdaFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnShowAsConditionalMap()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LisaMapFrame* f = dynamic_cast<LisaMapFrame*>(t)) {
		f->OnShowAsConditionalMap(event);
	} else if (GetisOrdMapFrame* f = dynamic_cast<GetisOrdMapFrame*>(t)) {
		f->OnShowAsConditionalMap(event);
    } else if (LocalGearyMapFrame* f = dynamic_cast<LocalGearyMapFrame*>(t)) {
        f->OnShowAsConditionalMap(event);
    } else if (MLJCMapFrame* f = dynamic_cast<MLJCMapFrame*>(t)) {
        f->OnShowAsConditionalMap(event);
    }
}

void GdaFrame::OnViewStandardizedData(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewStandardizedData()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewStandardizedData(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewStandardizedData(event);
	}	
}

void GdaFrame::OnViewOriginalData(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewOriginalData()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (PCPFrame* f = dynamic_cast<PCPFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewOriginalData(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewOriginalData(event);
	}
}

void GdaFrame::OnViewLinearSmoother(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewLinearSmoother()");
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
    wxLogMessage("In GdaFrame::OnViewLowessSmoother()");
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
    wxLogMessage("In GdaFrame::OnEditLowessParams()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnEditLowessParams(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnEditLowessParams(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnEditLowessParams(event);
    } else if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
        f->OnEditLowessParams(event);
	} else if (ConditionalScatterPlotFrame* f =
						 dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnEditLowessParams(event);
	}
}

void GdaFrame::OnEditVariables(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnEditVariables()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnShowVarsChooser(event);
	} else if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
		f->OnShowCorrelParams(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnShowVarsChooser(event);
	} else if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		//f->OnShowVarsChooser(event);
	}
}

void GdaFrame::OnSaveStatsToCsv(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnSaveStatsToCsv()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
		f->OnSaveResult(event);
	}
}
void GdaFrame::OnViewRegimesRegression(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewRegimesRegression()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
    if (LisaScatterPlotFrame* f = dynamic_cast<LisaScatterPlotFrame*>(t)) {
		f->OnViewRegimesRegression(event);
    } else if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	} else if (ScatterPlotMatFrame* f = dynamic_cast<ScatterPlotMatFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	} else if (CovSpFrame* f = dynamic_cast<CovSpFrame*>(t)) {
		f->OnViewRegimesRegression(event);
	} 
}

void GdaFrame::OnViewRegressionSelectedExcluded(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewRegressionSelectedExcluded()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelectedExcluded(event);
	}
}

void GdaFrame::OnViewRegressionSelected(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnViewRegressionSelected()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnViewRegressionSelected(event);
	}
}

void GdaFrame::OnCompareRegimes(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCompareRegimes()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f, "ID_COMPARE_REGIMES");
	}
}

void GdaFrame::OnCompareTimePeriods(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCompareTimePeriods()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f,"ID_COMPARE_TIME_PERIODS");
	}
}

void GdaFrame::OnCompareRegAndTmPer(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnCompareRegAndTmPer()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=
			new LineChartEventDelay(f,"ID_COMPARE_REG_AND_TM_PER");
	}
}

void GdaFrame::OnDisplayStatistics(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayStatistics()");
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
	} else if (CorrelogramFrame* f = dynamic_cast<CorrelogramFrame*>(t)) {
		f->OnDisplayStatistics(event);
	} else if (LineChartFrame* f = dynamic_cast<LineChartFrame*>(t)) {
		LineChartEventDelay* l=new LineChartEventDelay(f, "ID_DISPLAY_STATISTICS");
	}
}

void GdaFrame::OnShowAxesThroughOrigin(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnShowAxesThroughOrigin()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ScatterNewPlotFrame* f = dynamic_cast<ScatterNewPlotFrame*>(t)) {
		f->OnShowAxesThroughOrigin(event);
	}
}

void GdaFrame::OnShowAxes(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnShowAxes()");
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
    wxLogMessage("In GdaFrame::OnDisplayAxesScaleValues()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	if (ConditionalScatterPlotFrame* f =
		dynamic_cast<ConditionalScatterPlotFrame*>(t)) {
		f->OnDisplayAxesScaleValues(event);
	}
}

void GdaFrame::OnDisplaySlopeValues(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplaySlopeValues()");
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
    wxLogMessage("In GdaFrame::OnTimeSyncVariable1()");
	OnTimeSyncVariable(0);
}

void GdaFrame::OnTimeSyncVariable2(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnTimeSyncVariable2()");
	OnTimeSyncVariable(1);
}

void GdaFrame::OnTimeSyncVariable3(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnTimeSyncVariable3()");
	OnTimeSyncVariable(2);
}

void GdaFrame::OnTimeSyncVariable4(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnTimeSyncVariable4()");
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
    wxLogMessage("In GdaFrame::OnFixedScaleVariable1()");
	OnFixedScaleVariable(0);
}

void GdaFrame::OnFixedScaleVariable2(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnFixedScaleVariable2()");
	OnFixedScaleVariable(1);
}

void GdaFrame::OnFixedScaleVariable3(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnFixedScaleVariable3()");
	OnFixedScaleVariable(2);
}

void GdaFrame::OnFixedScaleVariable4(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnFixedScaleVariable4()");
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
    wxLogMessage("In GdaFrame::OnPlotsPerView1()");
	OnPlotsPerView(1);
}

void GdaFrame::OnPlotsPerView2(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView2()");
	OnPlotsPerView(2);
}

void GdaFrame::OnPlotsPerView3(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView3()");
	OnPlotsPerView(3);
}

void GdaFrame::OnPlotsPerView4(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView4()");
	OnPlotsPerView(4);
}

void GdaFrame::OnPlotsPerView5(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView5()");
	OnPlotsPerView(5);
}

void GdaFrame::OnPlotsPerView6(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView6()");
	OnPlotsPerView(6);
}

void GdaFrame::OnPlotsPerView7(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView7()");
	OnPlotsPerView(7);
}

void GdaFrame::OnPlotsPerView8(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView8()");
	OnPlotsPerView(8);
}

void GdaFrame::OnPlotsPerView9(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView9()");
	OnPlotsPerView(9);
}

void GdaFrame::OnPlotsPerView10(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView10()");
	OnPlotsPerView(10);
}

void GdaFrame::OnPlotsPerViewOther(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView11()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewOther();
}

void GdaFrame::OnPlotsPerViewAll(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnPlotsPerView12()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnPlotsPerViewAll();
}

void GdaFrame::OnDisplayStatusBar(wxCommandEvent& event)
{
    wxLogMessage("In GdaFrame::OnDisplayStatusBar()");
	TemplateFrame* t = TemplateFrame::GetActiveFrame();
	if (!t) return;
	t->OnDisplayStatusBar(event);
}

void GdaFrame::OnReportBug(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("In GdaFrame::OnReportBug()");
    ReportBugDlg bugDlg(this);
    bugDlg.ShowModal();
  
    
    /*
    wxString toEmail = "spatial@uchicago.edu";
    wxString subject = "Report GeoDa Bug";
    wxString content;
    
    wxString logger_path;
    if (GeneralWxUtils::isMac()) {
        logger_path <<  GenUtils::GetBasemapCacheDir() <<  "../../../logger.txt";
    } else {
        logger_path <<  GenUtils::GetBasemapCacheDir() << "\\logger.txt";
    }
    wxTextFile tfile;
    tfile.Open(logger_path);
    while(!tfile.Eof())
    {
        content << tfile.GetNextLine() << "\n";
    }
   
    wxString mail = wxString::Format("mailto:%s?subject=%s&body=[Bug Description]: please simply describe the bug\n\n[Data]: Point/Polygon? can you share your data for troubleshooting?\n\n[Details]:\n%s", toEmail, subject, content);
    wxURI url(mail);
    wxString encoded_url = "open " + url.BuildURI();
    
    wxExecute(encoded_url);
     */
}

void GdaFrame::OnCheckUpdates(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("In GdaFrame::OnCheckUpdates()");
    wxString version =  AutoUpdate::CheckUpdate();
    if (!version.IsEmpty()) {
        AutoUpdateDlg dlg(this);
        dlg.ShowModal();
    } else {
        wxMessageDialog msgDlg(this,
                               _("Your GeoDa is already up-to-date."),
                               _("No update required"),
                               wxOK |wxICON_INFORMATION);
        msgDlg.ShowModal();
    }
}

void GdaFrame::OnCheckTestMode(wxCommandEvent& event)
{
    std::string checked = "no";
    if (!event.IsChecked()) {
        checked = "yes";
    }
    
    wxLogMessage("In GdaFrame::OnCheckTestMode():");
    
    OGRDataAdapter::GetInstance().AddEntry("test_mode", checked);
}

void GdaFrame::OnDonate(wxCommandEvent& WXUNUSED(event) )
{
    wxString donate_url = "https://giving.uchicago.edu/site/Donation2?1838.donation=form1&df_id=1838&mfc_pref=T&set.Designee=1901";
    wxLaunchDefaultBrowser(donate_url);
}

void GdaFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event) )
{
    wxLogMessage("In GdaFrame::OnHelpAbout()");
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
    
    if (Gda::version_subbuild > 0) {
        vl_s << "." << Gda::version_subbuild;
    }
    
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

    wxButton* btn_donate = dynamic_cast<wxButton*>(wxWindow::FindWindowById(XRCID("wxID_DONATE"), &dlg));
    wxButton* btn_update = dynamic_cast<wxButton*>(wxWindow::FindWindowById(XRCID("ID_CHECKUPDATES"), &dlg));
    wxCheckBox* chk_testmode_stable = dynamic_cast<wxCheckBox*>(wxWindow::FindWindowById(XRCID("IDC_CHECK_TESTMODE_STABLE"), &dlg));
    std::vector<wxString> test_mode = OGRDataAdapter::GetInstance().GetHistory("test_mode");
   
    bool isTestMode = false;
    if (!test_mode.empty()) {
        if (test_mode[0] == "yes") {
            isTestMode = true;
            chk_testmode_stable->SetValue(false);
        } else {
            chk_testmode_stable->SetValue(true);
        }
    }

    btn_donate->Connect(wxEVT_BUTTON, wxCommandEventHandler(GdaFrame::OnDonate), NULL, this);
    chk_testmode_stable->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(GdaFrame::OnCheckTestMode), NULL, this);
    btn_update->Connect(wxEVT_BUTTON, wxCommandEventHandler(GdaFrame::OnCheckUpdates), NULL, this);
	dlg.ShowModal();
}

void GdaFrame::OnTableSetLocale(wxCommandEvent& event)
{
    // show a number locale setup dialog, then save it to GDAL global env
    LocaleSetupDlg localeDlg(this);
    localeDlg.ShowModal();
}

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

bool GdaFrame::GetHtmlMenuItems()
{
	return GetHtmlMenuItemsJson();
}

bool GdaFrame::GetHtmlMenuItemsJson()
{
	return true;
}

bool GdaFrame::GetHtmlMenuItemsSqlite()
{
  return false;
}

int GdaFrame::sqlite3_GetHtmlMenuItemsCB(void *data, int argc,
										 char **argv, char **azColName)
{
	//if (argc != 2) return SQLITE_ERROR;
	//htmlMenuItems.push_back(MenuItem(argv[0], argv[1]));
	//return SQLITE_OK;
	return 0;
}

LineChartEventDelay::LineChartEventDelay()
: lc_frame(0)
{
}

LineChartEventDelay::LineChartEventDelay(LineChartFrame* lc_frame_,
                                         const wxString& cb_name_)
: lc_frame(lc_frame_), cb_name(cb_name_)
{
	StartOnce(100);
}

LineChartEventDelay::~LineChartEventDelay()
{
}

void LineChartEventDelay::Notify() {
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

/*
 * This is the top-level window of the application.
 */
BEGIN_EVENT_TABLE(GdaFrame, wxFrame)
    EVT_CHAR_HOOK(GdaFrame::OnKeyEvent)
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
    EVT_MENU(XRCID("wxID_PREFERENCES"), GdaFrame::OnPreferenceSetup)
    EVT_MENU(XRCID("wxID_CLOSE"), GdaFrame::OnMenuClose)
    EVT_MENU(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
    EVT_TOOL(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
    EVT_BUTTON(XRCID("ID_CLOSE_PROJECT"), GdaFrame::OnCloseProjectEvt)
    EVT_CLOSE(GdaFrame::OnClose)
    EVT_MENU(XRCID("wxID_EXIT"), GdaFrame::OnQuit)
    EVT_MENU(XRCID("ID_SELECT_WITH_RECT"), GdaFrame::OnSelectWithRect)
    EVT_MENU(XRCID("ID_SELECT_WITH_CIRCLE"), GdaFrame::OnSelectWithCircle)
    EVT_MENU(XRCID("ID_SELECT_WITH_LINE"), GdaFrame::OnSelectWithLine)
    EVT_MENU(XRCID("ID_SELECT_WITH_CUSTOM"), GdaFrame::OnSelectWithCustom)
    EVT_MENU(XRCID("ID_SELECTION_MODE"), GdaFrame::OnSelectionMode)
    EVT_MENU(XRCID("ID_FIT_TO_WINDOW_MODE"), GdaFrame::OnFitToWindowMode)
    // Fit-To-Window Mode
    EVT_MENU(XRCID("ID_FIXED_ASPECT_RATIO_MODE"), GdaFrame::OnFixedAspectRatioMode)
    EVT_MENU(XRCID("ID_ADJUST_AXIS_PRECISION"), GdaFrame::OnSetAxisDisplayPrecision)
    EVT_MENU(XRCID("ID_ZOOM_MODE"), GdaFrame::OnZoomMode)
    EVT_MENU(XRCID("ID_PAN_MODE"), GdaFrame::OnPanMode)
    // Print Canvas State to Log File.  Used for debugging.
    EVT_MENU(XRCID("ID_PRINT_CANVAS_STATE"), GdaFrame::OnPrintCanvasState)
    EVT_MENU(XRCID("ID_CLEAN_BASEMAP"), GdaFrame::OnCleanBasemap)
    EVT_MENU(XRCID("ID_NO_BASEMAP"), GdaFrame::OnSetNoBasemap)
    EVT_MENU(XRCID("ID_CHANGE_TRANSPARENCY"), GdaFrame::OnChangeMapTransparency)
    EVT_MENU(XRCID("ID_BASEMAP_CONF"), GdaFrame::OnBasemapConfig)
    EVT_MENU(XRCID("ID_SAVE_CANVAS_IMAGE_AS"), GdaFrame::OnSaveCanvasImageAs)
    EVT_MENU(XRCID("ID_SAVE_SELECTED_TO_COLUMN"), GdaFrame::OnSaveSelectedToColumn)
    EVT_MENU(XRCID("ID_CANVAS_BACKGROUND_COLOR"), GdaFrame::OnCanvasBackgroundColor)
    EVT_MENU(XRCID("ID_LEGEND_USE_SCI_NOTATION"), GdaFrame::OnLegendUseScientificNotation)
    EVT_MENU(XRCID("ID_LEGEND_DISPLAY_PRECISION"), GdaFrame::OnLegendDisplayPrecision)
    EVT_MENU(XRCID("ID_LEGEND_BACKGROUND_COLOR"), GdaFrame::OnLegendBackgroundColor)
    EVT_MENU(XRCID("ID_SELECTABLE_FILL_COLOR"), GdaFrame::OnSelectableFillColor)
    EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_COLOR"), GdaFrame::OnSelectableOutlineColor)
    EVT_MENU(XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"), GdaFrame::OnSelectableOutlineVisible)
    EVT_MENU(XRCID("ID_MAP_SHOW_MAP_CONTOUR"), GdaFrame::OnShowMapBoundary)
    EVT_MENU(XRCID("ID_HIGHLIGHT_COLOR"), GdaFrame::OnHighlightColor)
    EVT_MENU(XRCID("ID_COPY_IMAGE_TO_CLIPBOARD"), GdaFrame::OnCopyImageToClipboard)
    EVT_MENU(XRCID("ID_COPY_LEGEND_TO_CLIPBOARD"), GdaFrame::OnCopyLegendToClipboard)
    EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)
    EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)

    EVT_MENU(XRCID("ID_TOOLS_DATA_PCA"), GdaFrame::OnToolsDataPCA)
    EVT_MENU(XRCID("ID_TOOLS_DATA_KMEANS"), GdaFrame::OnToolsDataKMeans)
    EVT_MENU(XRCID("ID_TOOLS_DATA_KMEDIANS"), GdaFrame::OnToolsDataKMedians)
    EVT_MENU(XRCID("ID_TOOLS_DATA_KMEDOIDS"), GdaFrame::OnToolsDataKMedoids)
    EVT_MENU(XRCID("ID_TOOLS_DATA_HCLUSTER"), GdaFrame::OnToolsDataHCluster)
    EVT_MENU(XRCID("ID_TOOLS_DATA_HDBSCAN"), GdaFrame::OnToolsDataHDBScan)
    EVT_MENU(XRCID("ID_TOOLS_DATA_MAXP"), GdaFrame::OnToolsDataMaxP)
    EVT_MENU(XRCID("ID_TOOLS_DATA_SKATER"), GdaFrame::OnToolsDataSkater)
    EVT_MENU(XRCID("ID_TOOLS_DATA_SPECTRAL"), GdaFrame::OnToolsDataSpectral)
    EVT_MENU(XRCID("ID_TOOLS_DATA_REDCAP"), GdaFrame::OnToolsDataRedcap)
    EVT_MENU(XRCID("ID_TOOLS_DATA_MDS"), GdaFrame::OnToolsDataMDS)

    EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_MANAGER"), GdaFrame::OnToolsWeightsManager)
    EVT_MENU(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)
    EVT_TOOL(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)
    EVT_BUTTON(XRCID("ID_TOOLS_WEIGHTS_CREATE"), GdaFrame::OnToolsWeightsCreate)
    EVT_MENU(XRCID("ID_CONNECTIVITY_HIST_VIEW"), GdaFrame::OnConnectivityHistView)
    EVT_TOOL(XRCID("ID_CONNECTIVITY_HIST_VIEW"), GdaFrame::OnConnectivityHistView)
    EVT_BUTTON(XRCID("ID_CONNECTIVITY_HIST_VIEW"), GdaFrame::OnConnectivityHistView)
    EVT_MENU(XRCID("ID_CONNECTIVITY_MAP_VIEW"), GdaFrame::OnConnectivityMapView)
    EVT_TOOL(XRCID("ID_CONNECTIVITY_MAP_VIEW"), GdaFrame::OnConnectivityMapView)
    EVT_BUTTON(XRCID("ID_CONNECTIVITY_MAP_VIEW"), GdaFrame::OnConnectivityMapView)
    EVT_MENU(XRCID("ID_SHOW_AXES"), GdaFrame::OnShowAxes)
    EVT_TOOL(XRCID("ID_MAP_CHOICES"), GdaFrame::OnMapChoices)
    EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_GRID"), GdaFrame::OnShapePolygonsFromGrid)
    EVT_MENU(XRCID("ID_SHAPE_POLYGONS_FROM_BOUNDARY"), GdaFrame::OnShapePolygonsFromBoundary)
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
    EVT_MENU(XRCID("ID_TABLE_EDIT_FIELD_PROP"), GdaFrame::OnEditFieldProperties)
    //EVT_MENU(XRCID("ID_TABLE_CHANGE_FIELD_TYPE"), GdaFrame::OnChangeFieldType)
    EVT_MENU(XRCID("ID_TABLE_MERGE_TABLE_DATA"), GdaFrame::OnMergeTableData)
    EVT_MENU(XRCID("ID_TABLE_AGGREGATION_DATA"), GdaFrame::OnAggregateData)
    EVT_MENU(XRCID("ID_TABLE_SPATIAL_JOIN"), GdaFrame::OnSpatialJoin)
    EVT_MENU(XRCID("ID_HIERARCHICAL_MAP"), GdaFrame::OnGroupingMap)
    EVT_MENU(XRCID("ID_EXPORT_TO_CSV_FILE"),   GdaFrame::OnExportToCsvFile) // not used
    EVT_MENU(XRCID("ID_REGRESSION_CLASSIC"), GdaFrame::OnRegressionClassic)
    EVT_TOOL(XRCID("ID_REGRESSION_CLASSIC"), GdaFrame::OnRegressionClassic)
    EVT_TOOL(XRCID("ID_PUBLISH"), GdaFrame::OnPublish)
    EVT_TOOL(XRCID("ID_COND_PLOT_CHOICES"), GdaFrame::OnCondPlotChoices)
    EVT_TOOL(XRCID("ID_CLUSTERING_CHOICES"), GdaFrame::OnClusteringChoices)
    // The following duplicate entries are needed as a workaround to
    // make menu enable/disable work for the menu bar when the same menu
    // item appears twice.
    EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW_MAP_MENU"), GdaFrame::OnShowConditionalMapView)
    EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), GdaFrame::OnShowConditionalMapView)
    EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_MAP_VIEW"), GdaFrame::OnShowConditionalMapView)
    EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"), GdaFrame::OnShowConditionalScatterView)
    EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_SCATTER_VIEW"), GdaFrame::OnShowConditionalScatterView)
    EVT_MENU(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"), GdaFrame::OnShowConditionalHistView)
    EVT_BUTTON(XRCID("ID_SHOW_CONDITIONAL_HIST_VIEW"), GdaFrame::OnShowConditionalHistView)
    EVT_MENU(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), GdaFrame::OnShowCartogramNewView)
    EVT_TOOL(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), GdaFrame::OnShowCartogramNewView)
    EVT_BUTTON(XRCID("ID_SHOW_CARTOGRAM_NEW_VIEW"), GdaFrame::OnShowCartogramNewView)
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
    EVT_TOOL(XRCID("ID_TOOLS_MENU"), GdaFrame::OnToolsChoices)
    EVT_MENU(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
    EVT_TOOL(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
    EVT_BUTTON(XRCID("IDM_MSPL"), GdaFrame::OnOpenMSPL)
    EVT_MENU(XRCID("IDM_DMORAN"), GdaFrame::OnOpenDiffMoran)
    EVT_TOOL(XRCID("IDM_DMORAN"), GdaFrame::OnOpenDiffMoran)
    EVT_BUTTON(XRCID("IDM_DMORAN"), GdaFrame::OnOpenDiffMoran)
    EVT_MENU(XRCID("IDM_GMORAN"), GdaFrame::OnOpenGMoran)
    EVT_TOOL(XRCID("IDM_GMORAN"), GdaFrame::OnOpenGMoran)
    EVT_BUTTON(XRCID("IDM_GMORAN"), GdaFrame::OnOpenGMoran)
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

    EVT_MENU(XRCID("IDM_DIFF_LISA"), GdaFrame::OnOpenDiffLisa)
    EVT_TOOL(XRCID("IDM_DIFF_LISA"), GdaFrame::OnOpenDiffLisa)
    EVT_BUTTON(XRCID("IDM_DIFF_LISA"), GdaFrame::OnOpenDiffLisa)

    EVT_MENU(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)
    EVT_TOOL(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)
    EVT_BUTTON(XRCID("IDM_LISA_EBRATE"), GdaFrame::OnOpenLisaEB)

    EVT_MENU(XRCID("IDM_UNI_LOCAL_GEARY"), GdaFrame::OnOpenUniLocalGeary)
    EVT_TOOL(XRCID("IDM_UNI_LOCAL_GEARY"), GdaFrame::OnOpenUniLocalGeary)
    EVT_BUTTON(XRCID("IDM_UNI_LOCAL_GEARY"), GdaFrame::OnOpenUniLocalGeary)

    EVT_MENU(XRCID("IDM_MUL_LOCAL_GEARY"), GdaFrame::OnOpenMultiLocalGeary)
    EVT_TOOL(XRCID("IDM_MUL_LOCAL_GEARY"), GdaFrame::OnOpenMultiLocalGeary)
    EVT_BUTTON(XRCID("IDM_MUL_LOCAL_GEARY"), GdaFrame::OnOpenMultiLocalGeary)

    EVT_TOOL(XRCID("IDM_GETIS_ORD_MENU"), GdaFrame::OnGetisMenuChoices)
    EVT_BUTTON(XRCID("IDM_GETIS_ORD_MENU"), GdaFrame::OnGetisMenuChoices)

    EVT_MENU(XRCID("IDM_LOCAL_G"), GdaFrame::OnOpenGetisOrd)
    EVT_MENU(XRCID("IDM_LOCAL_G_STAR"), GdaFrame::OnOpenGetisOrdStar)
    EVT_MENU(XRCID("IDM_LOCAL_JOINT_COUNT"), GdaFrame::OnOpenLocalJoinCount)

    EVT_MENU(XRCID("IDM_BIV_LJC"), GdaFrame::OnOpenBivariateLJC)
    EVT_TOOL(XRCID("IDM_BIV_LJC"), GdaFrame::OnOpenBivariateLJC)
    EVT_MENU(XRCID("IDM_MUL_LJC"), GdaFrame::OnOpenMultiLJC)
    EVT_TOOL(XRCID("IDM_MUL_LJC"), GdaFrame::OnOpenMultiLJC)

    EVT_MENU(XRCID("ID_VIEW_DISPLAY_PRECISION"),GdaFrame::OnDisplayPrecision)
    EVT_MENU(XRCID("ID_HISTOGRAM_INTERVALS"), GdaFrame::OnHistogramIntervals)
    EVT_MENU(XRCID("ID_SAVE_CONNECTIVITY_TO_TABLE"), GdaFrame::OnSaveConnectivityToTable)
    EVT_MENU(XRCID("ID_SELECT_ISOLATES"), GdaFrame::OnSelectIsolates)
    EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_99PERMUTATION"), GdaFrame::OnRan99Per)
    EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_199PERMUTATION"), GdaFrame::OnRan199Per)
    EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_499PERMUTATION"), GdaFrame::OnRan499Per)
    EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_999PERMUTATION"), GdaFrame::OnRan999Per)
    EVT_MENU(XRCID("ID_OPTIONS_RANDOMIZATION_OTHER"), GdaFrame::OnRanOtherPer)
    EVT_MENU(XRCID("ID_USE_SPECIFIED_SEED"), GdaFrame::OnUseSpecifiedSeed)
    EVT_MENU(XRCID("ID_SPECIFY_SEED_DLG"), GdaFrame::OnSpecifySeedDlg)
    EVT_MENU(XRCID("ID_SAVE_MORANI"), GdaFrame::OnSaveMoranI)
    EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_05"), GdaFrame::OnSigFilter05)
    EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_01"), GdaFrame::OnSigFilter01)
    EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_001"), GdaFrame::OnSigFilter001)
    EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_0001"), GdaFrame::OnSigFilter0001)
    EVT_MENU(XRCID("ID_SIGNIFICANCE_FILTER_SETUP"), GdaFrame::OnSigFilterSetup)
    EVT_MENU(XRCID("ID_SAVE_GETIS_ORD"), GdaFrame::OnSaveGetisOrd)
    EVT_MENU(XRCID("ID_SAVE_LISA"), GdaFrame::OnSaveLisa)
    EVT_MENU(XRCID("ID_SAVE_COLOCATION"), GdaFrame::OnSaveColocation)
    EVT_MENU(XRCID("ID_SELECT_CORES"), GdaFrame::OnSelectCores)
    EVT_MENU(XRCID("ID_SHOW_AS_COND_MAP"), GdaFrame::OnShowAsConditionalMap)
    EVT_MENU(XRCID("ID_ADD_NEIGHBORS_TO_SELECTION"), GdaFrame::OnAddNeighborToSelection)
    EVT_MENU(XRCID("ID_DISPLAY_WEIGHTS_GRAPH"), GdaFrame::OnDisplayWeightsGraph)
    EVT_MENU(XRCID("ID_HIDE_MAP_WITH_GRAPH"), GdaFrame::OnDisplayMapWithGraph)
    EVT_MENU(XRCID("ID_WEIGHTS_GRAPH_THICKNESS_LIGHT"), GdaFrame::OnChangeGraphThickness)
    EVT_MENU(XRCID("ID_WEIGHTS_GRAPH_THICKNESS_NORM"), GdaFrame::OnChangeGraphThickness)
    EVT_MENU(XRCID("ID_WEIGHTS_GRAPH_THICKNESS_STRONG"), GdaFrame::OnChangeGraphThickness)
    EVT_MENU(XRCID("ID_WEIGHTS_GRAPH_COLOR"), GdaFrame::OnChangeGraphColor)
    EVT_MENU(XRCID("ID_CONN_SELECTED_COLOR"), GdaFrame::OnChangeConnSelectedColor)
EVT_MENU(XRCID("ID_CONN_ROOT_SIZE"), GdaFrame::OnChangeConnRootSize)
EVT_MENU(XRCID("ID_CONN_ROOT_COLOR"), GdaFrame::OnChangeConnRootColor)
    EVT_MENU(XRCID("ID_CONN_NEIGHBOR_FILL_COLOR"), GdaFrame::OnChangeNeighborFillColor)
    EVT_MENU(XRCID("ID_SELECT_NEIGHBORS_OF_CORES"), GdaFrame::OnSelectNeighborsOfCores)
    EVT_MENU(XRCID("ID_SELECT_CORES_AND_NEIGHBORS"), GdaFrame::OnSelectCoresAndNeighbors)
    EVT_MENU(XRCID("ID_MAP_ADDMEANCENTERS"), GdaFrame::OnAddMeanCenters)
    EVT_MENU(XRCID("ID_MAP_ADDCENTROIDS"), GdaFrame::OnAddCentroids)
    EVT_MENU(XRCID("ID_DISPLAY_MEAN_CENTERS"), GdaFrame::OnDisplayMeanCenters)
    EVT_MENU(XRCID("ID_DISPLAY_CENTROIDS"), GdaFrame::OnDisplayCentroids)
    EVT_MENU(XRCID("ID_DISPLAY_VORONOI_DIAGRAM"), GdaFrame::OnDisplayVoronoiDiagram)
    EVT_MENU(XRCID("ID_EXPORT_VORONOI"), GdaFrame::OnExportVoronoi)
    EVT_MENU(XRCID("ID_EXPORT_MEAN_CNTRS"), GdaFrame::OnExportMeanCntrs)
    EVT_MENU(XRCID("ID_EXPORT_CENTROIDS"), GdaFrame::OnExportCentroids)
    EVT_MENU(XRCID("ID_SAVE_VORONOI_DUPS_TO_TABLE"), GdaFrame::OnSaveVoronoiDupsToTable)
    EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A"), GdaFrame::OnNewCustomCatClassifA)
    EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_B"), GdaFrame::OnNewCustomCatClassifB)
    EVT_MENU(XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_C"), GdaFrame::OnNewCustomCatClassifC)
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

    EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"), GdaFrame::OnOpenThemelessMap)
    EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_THEMELESS"), GdaFrame::OnOpenThemelessMap)
    EVT_MENU(XRCID("ID_MAPANALYSIS_THEMELESS"), GdaFrame::OnThemelessMap)
    EVT_MENU(XRCID("ID_COND_VERT_THEMELESS"), GdaFrame::OnCondVertThemelessMap)
    EVT_MENU(XRCID("ID_COND_HORIZ_THEMELESS"), GdaFrame::OnCondHorizThemelessMap)
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
    EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"), GdaFrame::OnOpenPercentile)
    EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_PERCENTILE"), GdaFrame::OnOpenPercentile)
    EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_PERCENTILE"), GdaFrame::OnPercentile)
    EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_PERCENTILE"), GdaFrame::OnCondVertPercentile)
    EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_PERCENTILE"), GdaFrame::OnCondHorizPercentile)
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
    EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"), GdaFrame::OnOpenStddev)
    EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_CHOROPLETH_STDDEV"), GdaFrame::OnOpenStddev)
    EVT_MENU(XRCID("ID_MAPANALYSIS_CHOROPLETH_STDDEV"), GdaFrame::OnStddev)
    EVT_MENU(XRCID("ID_COND_VERT_CHOROPLETH_STDDEV"), GdaFrame::OnCondVertStddev)
    EVT_MENU(XRCID("ID_COND_HORIZ_CHOROPLETH_STDDEV"), GdaFrame::OnCondHorizStddev)

    EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"), GdaFrame::OnOpenUniqueValues)
    EVT_MENU(XRCID("ID_OPEN_MAPANALYSIS_UNIQUE_VALUES"), GdaFrame::OnOpenUniqueValues)
    EVT_MENU(XRCID("ID_MAPANALYSIS_UNIQUE_VALUES"), GdaFrame::OnUniqueValues)

    EVT_TOOL(XRCID("ID_OPEN_MAPANALYSIS_COLOCATION"), GdaFrame::OnOpenColocationMap)
    EVT_MENU(XRCID("ID_MAPANALYSIS_COLOCATION"), GdaFrame::OnOpenColocationMap)

    EVT_MENU(XRCID("ID_COND_VERT_UNIQUE_VALUES"), GdaFrame::OnCondVertUniqueValues)
    EVT_MENU(XRCID("ID_COND_HORIZ_UNIQUE_VALUES"), GdaFrame::OnCondHorizUniqueValues)
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
    EVT_TOOL(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"), GdaFrame::OnOpenExcessrisk)
    EVT_MENU(XRCID("ID_OPEN_RATES_SMOOTH_EXCESSRISK"), GdaFrame::OnOpenExcessrisk)
    EVT_MENU(XRCID("ID_RATES_SMOOTH_EXCESSRISK"), GdaFrame::OnExcessrisk)
    EVT_TOOL(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"), GdaFrame::OnOpenEmpiricalBayes)
    EVT_MENU(XRCID("ID_OPEN_RATES_EMPIRICAL_BAYES_SMOOTHER"), GdaFrame::OnOpenEmpiricalBayes)
    EVT_MENU(XRCID("ID_RATES_EMPIRICAL_BAYES_SMOOTHER"), GdaFrame::OnEmpiricalBayes)
    EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"), GdaFrame::OnOpenSpatialRate)
    EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_RATE_SMOOTHER"), GdaFrame::OnOpenSpatialRate)
    EVT_MENU(XRCID("ID_RATES_SPATIAL_RATE_SMOOTHER"), GdaFrame::OnSpatialRate)
    EVT_TOOL(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"), GdaFrame::OnOpenSpatialEmpiricalBayes)
    EVT_MENU(XRCID("ID_OPEN_RATES_SPATIAL_EMPIRICAL_BAYES"), GdaFrame::OnOpenSpatialEmpiricalBayes)
    EVT_MENU(XRCID("ID_RATES_SPATIAL_EMPIRICAL_BAYES"), GdaFrame::OnSpatialEmpiricalBayes)
    EVT_MENU(XRCID("ID_MAPANALYSIS_SAVERESULTS"), GdaFrame::OnSaveResults)
    EVT_MENU(XRCID("ID_VIEW_STANDARDIZED_DATA"), GdaFrame::OnViewStandardizedData)
    EVT_MENU(XRCID("ID_VIEW_ORIGINAL_DATA"), GdaFrame::OnViewOriginalData)
    EVT_MENU(XRCID("ID_VIEW_LINEAR_SMOOTHER"), GdaFrame::OnViewLinearSmoother)
    EVT_MENU(XRCID("ID_VIEW_LOWESS_SMOOTHER"), GdaFrame::OnViewLowessSmoother)
    EVT_MENU(XRCID("ID_EDIT_LOWESS_PARAMS"), GdaFrame::OnEditLowessParams)
    EVT_MENU(XRCID("ID_EDIT_VARIABLES"), GdaFrame::OnEditVariables)
    EVT_MENU(XRCID("ID_SAVE_CORRELOGRAM_STATS"), GdaFrame::OnSaveStatsToCsv)
    EVT_MENU(XRCID("ID_VIEW_REGIMES_REGRESSION"), GdaFrame::OnViewRegimesRegression)
    EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED"), GdaFrame::OnViewRegressionSelected)
    EVT_MENU(XRCID("ID_VIEW_REGRESSION_SELECTED_EXCLUDED"), GdaFrame::OnViewRegressionSelectedExcluded)
    EVT_MENU(XRCID("ID_COMPARE_REGIMES"), GdaFrame::OnCompareRegimes)
    EVT_MENU(XRCID("ID_COMPARE_TIME_PERIODS"), GdaFrame::OnCompareTimePeriods)
    EVT_MENU(XRCID("ID_COMPARE_REG_AND_TM_PER"), GdaFrame::OnCompareRegAndTmPer)
    EVT_MENU(XRCID("ID_DISPLAY_STATISTICS"), GdaFrame::OnDisplayStatistics)
    EVT_MENU(XRCID("ID_CORRELOGRAM_DISPLAY_STATS"), GdaFrame::OnDisplayStatistics)

    EVT_MENU(XRCID("ID_SHOW_AXES_THROUGH_ORIGIN"), GdaFrame::OnShowAxesThroughOrigin)
    EVT_MENU(XRCID("ID_DISPLAY_AXES_SCALE_VALUES"), GdaFrame::OnDisplayAxesScaleValues)
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
    EVT_MENU(XRCID("wxID_DONATE"), GdaFrame::OnDonate)
    EVT_MENU(XRCID("wxID_CHECKUPDATES"), GdaFrame::OnCheckUpdates)
    EVT_MENU(XRCID("wxID_REPORTBUG"), GdaFrame::OnReportBug)
END_EVENT_TABLE()
