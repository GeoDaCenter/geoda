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
#include <string>
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>
#include <wx/tokenzr.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/notebook.h>
#include <wx/config.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include "curl/curl.h"
#include "ogrsf_frmts.h"
#include "cpl_conv.h"

#include "../HLStateInt.h"
#include "../HighlightStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "../GdaJson.h"
#include "../logger.h"
#include "PreferenceDlg.h"

using namespace std;
using namespace GdaJson;

////////////////////////////////////////////////////////////////////////////////
//
// PreferenceDlg
//
////////////////////////////////////////////////////////////////////////////////
PreferenceDlg::PreferenceDlg(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	highlight_state = NULL;
    table_state = NULL;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

PreferenceDlg::PreferenceDlg(wxWindow* parent,
	HLStateInt* _highlight_state,
    TableState* _table_state,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	highlight_state = _highlight_state;
    table_state = _table_state;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

void PreferenceDlg::Init()
{
	ReadFromCache();

    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(880,820), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );

    wxPanel *panel = new wxPanel(scrl);

    long txt_num_style = wxTE_RIGHT | wxTE_PROCESS_ENTER;
    wxPoint pos = wxDefaultPosition;

	wxNotebook* notebook = new wxNotebook(panel, wxID_ANY, pos, wxDefaultSize);
	//  visualization tab
	wxNotebookPage* vis_page = new wxNotebookPage(notebook, wxID_ANY,
                                                  pos, wxSize(560, 680));
#ifdef __WIN32__
	vis_page->SetBackgroundColour(*wxWHITE);
#endif
	notebook->AddPage(vis_page, _("System"));
	wxFlexGridSizer* grid_sizer1 = new wxFlexGridSizer(22, 2, 8, 10);

	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Maps:")), 1);
	grid_sizer1->AddSpacer(10);

	wxString lbl0 = _("Use classic yellow cross-hatching to highlight selection in maps:");
	wxStaticText* lbl_txt0 = new wxStaticText(vis_page, wxID_ANY, lbl0);
	cbox0 = new wxCheckBox(vis_page, XRCID("PREF_USE_CROSSHATCH"), "", pos);
	grid_sizer1->Add(lbl_txt0, 1, wxEXPAND);
	grid_sizer1->Add(cbox0, 0, wxALIGN_RIGHT);
	cbox0->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnCrossHatch, this);

	wxSize sl_sz(200, -1);
	wxSize txt_sz(35, -1);

	wxString lbl1 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt1 = new wxStaticText(vis_page, wxID_ANY, lbl1);
	wxBoxSizer* box1 = new wxBoxSizer(wxHORIZONTAL);
	slider1 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255, pos, sl_sz, wxSL_HORIZONTAL);
	slider_txt1 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER1_TXT"), "",
		pos, txt_sz, wxTE_READONLY | wxTE_RIGHT);
	box1->Add(slider1);
	box1->Add(slider_txt1);
	grid_sizer1->Add(lbl_txt1, 1, wxEXPAND);
	grid_sizer1->Add(box1, 0, wxALIGN_RIGHT);
	slider1->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider1, this);

	wxString lbl2 = _("Set transparency of unhighlighted objects in selection:");
	wxStaticText* lbl_txt2 = new wxStaticText(vis_page, wxID_ANY, lbl2);
	wxBoxSizer* box2 = new wxBoxSizer(wxHORIZONTAL);
	slider2 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255, pos, sl_sz, wxSL_HORIZONTAL);
	slider_txt2 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER2_TXT"), "",
		pos, txt_sz, wxTE_READONLY | wxTE_RIGHT);
	box2->Add(slider2);
	box2->Add(slider_txt2);
	grid_sizer1->Add(lbl_txt2, 1, wxEXPAND);
	grid_sizer1->Add(box2, 0, wxALIGN_RIGHT);
	slider2->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider2, this);

    wxString lbl26 = _("Enable transparency setup of category color in map (Windows only):");
    wxStaticText* lbl_txt26 = new wxStaticText(vis_page, wxID_ANY, lbl26);
    cbox26 = new wxCheckBox(vis_page, XRCID("PREF_USE_CROSSHATCH"), "", pos);
    grid_sizer1->Add(lbl_txt26, 1, wxEXPAND);
    grid_sizer1->Add(cbox26, 0, wxALIGN_RIGHT);
    cbox26->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnEnableTransparencyWin, this);
    
	wxString lbl3 = _("Add basemap automatically:");
	wxStaticText* lbl_txt3 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	//wxStaticText* lbl_txt33 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	cmb33 = new wxComboBox(vis_page, wxID_ANY, "", pos, wxDefaultSize, 0,
                           NULL, wxCB_READONLY);
    cmb33->Append("No basemap");
    wxString basemap_sources = GdaConst::gda_basemap_sources;
    wxString encoded_str= wxString::FromUTF8((const char*)basemap_sources.mb_str());
    if (encoded_str.IsEmpty() == false) {
        basemap_sources = encoded_str;
    }
    vector<wxString> keys;
    wxString newline = basemap_sources.Find('\r') == wxNOT_FOUND ? "\n" : "\r\n";
    wxStringTokenizer tokenizer(basemap_sources, newline);
    while ( tokenizer.HasMoreTokens() ) {
        wxString token = tokenizer.GetNextToken();
        keys.push_back(token.Trim());
    }
    for (int i=0; i<keys.size(); i++) {
        wxString basemap_source = keys[i];
        wxUniChar comma = ',';
        int comma_pos = basemap_source.Find(comma);
        if ( comma_pos != wxNOT_FOUND ) {
            // group.name,url
            wxString group_n_name = basemap_source.BeforeFirst(comma);
            cmb33->Append(group_n_name);
        }
    }
	cmb33->SetSelection(0);
	cmb33->Bind(wxEVT_COMBOBOX, &PreferenceDlg::OnChoice3, this);
    grid_sizer1->Add(lbl_txt3, 1, wxEXPAND);
    grid_sizer1->Add(cmb33, 0, wxALIGN_RIGHT);
    
    grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Draw the values of selected variable on map (input font size):")), 1,
        wxEXPAND);
    wxBoxSizer* box29 = new wxBoxSizer(wxHORIZONTAL);
    cbox_lbl = new wxCheckBox(vis_page, XRCID("PREF_DRAW_LABELS"), "", pos);
    txt_lbl_font = new wxTextCtrl(vis_page, XRCID("PREF_DRAW_LABEL_FONT_SIZE"), "8", pos, wxSize(85, -1), txt_num_style);
    box29->Add(cbox_lbl);
    box29->Add(txt_lbl_font);
    grid_sizer1->Add(box29, 0, wxALIGN_RIGHT);
    cbox_lbl->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDrawLabels, this);
    txt_lbl_font->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnLabelFontSizeEnter, this);

	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Plots:")), 1,
                     wxTOP | wxBOTTOM, 10);
	grid_sizer1->AddSpacer(10);

	wxString lbl6 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt6 = new wxStaticText(vis_page, wxID_ANY, lbl6);
	wxBoxSizer* box6 = new wxBoxSizer(wxHORIZONTAL);
	slider6 = new wxSlider(vis_page, XRCID("PREF_SLIDER6"),
		255, 0, 255, pos, sl_sz, wxSL_HORIZONTAL);
	wxTextCtrl* slider_txt6 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER6_TXT"),
                                             "0.0", pos,
                                             txt_sz, wxTE_READONLY | wxTE_RIGHT);
	lbl_txt6->Hide();
	slider6->Hide();
	slider_txt6->Hide();

	box6->Add(slider6);
	box6->Add(slider_txt6);
	grid_sizer1->Add(lbl_txt6, 1, wxEXPAND);
	grid_sizer1->Add(box6, 0, wxALIGN_RIGHT);
	slider6->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider6, this);
	slider6->Enable(false);

	wxString lbl7 = _("Set transparency of unhighlighted objects in selection:");
	wxStaticText* lbl_txt7 = new wxStaticText(vis_page, wxID_ANY, lbl7);
	wxBoxSizer* box7 = new wxBoxSizer(wxHORIZONTAL);
	slider7 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		pos, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt7 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER7_TXT"), "", pos,
                                 txt_sz, wxTE_READONLY);
	box7->Add(slider7);
	box7->Add(slider_txt7);
	grid_sizer1->Add(lbl_txt7, 1, wxEXPAND);
	grid_sizer1->Add(box7, 0, wxALIGN_RIGHT);
	slider7->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider7, this);


    wxString lbl113 = _("Language:");
    wxStaticText* lbl_txt113 = new wxStaticText(vis_page, wxID_ANY, lbl113);
    cmb113 = new wxComboBox(vis_page, wxID_ANY, "", pos, wxDefaultSize, 0,
                            NULL, wxCB_READONLY);
    cmb113->Append("");
    cmb113->Append("English");
    cmb113->Append("Chinese (Simplified)");
    cmb113->Append("Spanish");
    cmb113->Append("Russian");
    //cmb113->Append("German");
    cmb113->Bind(wxEVT_COMBOBOX, &PreferenceDlg::OnChooseLanguage, this);
    //cmb113->Disable();
    
    grid_sizer1->Add(lbl_txt113, 1, wxEXPAND);
    grid_sizer1->Add(cmb113, 0, wxALIGN_RIGHT);
    
	wxString lbl8 = _("Show Recent/Sample Data panel in Connect Datasource Dialog:");
	wxStaticText* lbl_txt8 = new wxStaticText(vis_page, wxID_ANY, lbl8);
	cbox8 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_RECENT"), "", pos);
	grid_sizer1->Add(lbl_txt8, 1, wxEXPAND);
	grid_sizer1->Add(cbox8, 0, wxALIGN_RIGHT);
	cbox8->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowRecent, this);

	wxString lbl9 = _("Show CSV Configuration in Merge Data Dialog:");
	wxStaticText* lbl_txt9 = new wxStaticText(vis_page, wxID_ANY, lbl9);
	cbox9 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_CSV_IN_MERGE"), "", pos);
	grid_sizer1->Add(lbl_txt9, 1, wxEXPAND);
	grid_sizer1->Add(cbox9, 0, wxALIGN_RIGHT);
	cbox9->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowCsvInMerge, this);

	wxString lbl10 = _("Enable High DPI/Retina support (Mac only):");
	wxStaticText* lbl_txt10 = new wxStaticText(vis_page, wxID_ANY, lbl10);
	cbox10 = new wxCheckBox(vis_page, XRCID("PREF_ENABLE_HDPI"), "", pos);
	grid_sizer1->Add(lbl_txt10, 1, wxEXPAND);
	grid_sizer1->Add(cbox10, 0, wxALIGN_RIGHT);
	cbox10->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnEnableHDPISupport, this);

	wxString lbl4 = _("Disable crash detection for bug report:");
	wxStaticText* lbl_txt4 = new wxStaticText(vis_page, wxID_ANY, lbl4);
	cbox4 = new wxCheckBox(vis_page, XRCID("PREF_CRASH_DETECT"), "", pos);
	grid_sizer1->Add(lbl_txt4, 1, wxEXPAND);
	grid_sizer1->Add(cbox4, 0, wxALIGN_RIGHT);
	cbox4->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableCrashDetect, this);

	wxString lbl5 = _("Disable auto upgrade:");
	wxStaticText* lbl_txt5 = new wxStaticText(vis_page, wxID_ANY, lbl5);
	cbox5 = new wxCheckBox(vis_page, XRCID("PREF_AUTO_UPGRADE"), "", pos);
	grid_sizer1->Add(lbl_txt5, 1, wxEXPAND);
	grid_sizer1->Add(cbox5, 0, wxALIGN_RIGHT);
	cbox5->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableAutoUpgrade, this);

    grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Method:")), 1,
                     wxTOP | wxBOTTOM, 10);
    grid_sizer1->AddSpacer(10);
    
	wxString lbl16 = _("Use specified seed:");
	wxStaticText* lbl_txt16 = new wxStaticText(vis_page, wxID_ANY, lbl16);
	cbox6 = new wxCheckBox(vis_page, XRCID("PREF_USE_SPEC_SEED"), "", pos);
	grid_sizer1->Add(lbl_txt16, 1, wxEXPAND);
	grid_sizer1->Add(cbox6, 0, wxALIGN_RIGHT);
	cbox6->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnUseSpecifiedSeed, this);
   
	wxString lbl17 = _("Set seed for randomization:");
	wxStaticText* lbl_txt17 = new wxStaticText(vis_page, wxID_ANY, lbl17);
	txt_seed = new wxTextCtrl(vis_page, XRCID("PREF_SEED_VALUE"), "", pos,
                              wxSize(85, -1), txt_num_style);
	grid_sizer1->Add(lbl_txt17, 1, wxEXPAND);
	grid_sizer1->Add(txt_seed, 0, wxALIGN_RIGHT);
    txt_seed->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnSeedEnter, this);

	wxString lbl18 = _("Set number of CPU cores manually:");
	wxStaticText* lbl_txt18 = new wxStaticText(vis_page, wxID_ANY, lbl18);
    wxBoxSizer* box18 = new wxBoxSizer(wxHORIZONTAL);
	cbox18 = new wxCheckBox(vis_page, XRCID("PREF_SET_CPU_CORES"), "", pos);
	txt_cores = new wxTextCtrl(vis_page, XRCID("PREF_TXT_CPU_CORES"), "", pos,
                               wxSize(85, -1), txt_num_style);
    box18->Add(cbox18);
    box18->Add(txt_cores);
	grid_sizer1->Add(lbl_txt18, 1, wxEXPAND);
	grid_sizer1->Add(box18, 0, wxALIGN_RIGHT);
	cbox18->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnSetCPUCores, this);
    txt_cores->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnCPUCoresEnter, this);
    
	wxString lbl19 = _("Stopping criterion for power iteration:");
	wxStaticText* lbl_txt19 = new wxStaticText(vis_page, wxID_ANY, lbl19);
	txt_poweriter_eps = new wxTextCtrl(vis_page, XRCID("PREF_POWER_EPS"), "",
                                       pos, wxSize(85, -1), txt_num_style);
	grid_sizer1->Add(lbl_txt19, 1, wxEXPAND);
	grid_sizer1->Add(txt_poweriter_eps, 0, wxALIGN_RIGHT);
    txt_poweriter_eps->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnPowerEpsEnter, this);
    
    wxString lbl20 = _("Use GPU to Accelerate computation:");
    wxStaticText* lbl_txt20 = new wxStaticText(vis_page, wxID_ANY, lbl20);
    cbox_gpu = new wxCheckBox(vis_page, XRCID("PREF_USE_GPU"), "", pos);
    grid_sizer1->Add(lbl_txt20, 1, wxEXPAND);
    grid_sizer1->Add(cbox_gpu, 0, wxALIGN_RIGHT);
    cbox_gpu->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnUseGPU, this);
    
    //lbl_txt20->Hide();
    //cbox_gpu->Hide();
    
	grid_sizer1->AddGrowableCol(0, 1);

	wxBoxSizer *nb_box1 = new wxBoxSizer(wxVERTICAL);
	nb_box1->Add(grid_sizer1, 1, wxEXPAND | wxALL, 20);
	nb_box1->Fit(vis_page);

	vis_page->SetSizer(nb_box1);

	//------------------------------------
	//  datasource (gdal) tab
	wxNotebookPage* gdal_page = new wxNotebookPage(notebook, -1);
#ifdef __WIN32__
	gdal_page->SetBackgroundColour(*wxWHITE);
#endif
	notebook->AddPage(gdal_page, "Data Source");
	wxFlexGridSizer* grid_sizer2 = new wxFlexGridSizer(10, 2, 8, 10);

	wxString lbl21 = _("Hide system table in Postgresql connection:");
	wxStaticText* lbl_txt21 = new wxStaticText(gdal_page, wxID_ANY, lbl21);
	cbox21 = new wxCheckBox(gdal_page, wxID_ANY, "", pos);
	grid_sizer2->Add(lbl_txt21, 1, wxEXPAND | wxTOP, 10);
	grid_sizer2->Add(cbox21, 0, wxALIGN_RIGHT | wxTOP, 13);
	cbox21->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTablePostGIS, this);


	wxString lbl22 = _("Hide system table in SQLITE connection:");
	wxStaticText* lbl_txt22 = new wxStaticText(gdal_page, wxID_ANY, lbl22);
	cbox22 = new wxCheckBox(gdal_page, wxID_ANY, "", pos);
	grid_sizer2->Add(lbl_txt22, 1, wxEXPAND);
	grid_sizer2->Add(cbox22, 0, wxALIGN_RIGHT);
	cbox22->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTableSQLITE, this);

    
	wxString lbl23 = _("Http connection timeout (seconds) for e.g. WFS, Geojson etc.:");
	wxStaticText* lbl_txt23 = new wxStaticText(gdal_page, wxID_ANY, lbl23);
	txt23 = new wxTextCtrl(gdal_page, XRCID("ID_HTTP_TIMEOUT"), "", pos,
                           txt_sz, txt_num_style);
	grid_sizer2->Add(lbl_txt23, 1, wxEXPAND);
	grid_sizer2->Add(txt23, 0, wxALIGN_RIGHT);
	txt23->Bind(wxEVT_TEXT, &PreferenceDlg::OnTimeoutInput, this);
   
	wxString lbl24 = _("Date/Time formats (using comma to separate formats):");
	wxStaticText* lbl_txt24 = new wxStaticText(gdal_page, wxID_ANY, lbl24);
	txt24 = new wxTextCtrl(gdal_page, XRCID("ID_DATETIME_FORMATS"), "", pos,
                           wxSize(200, -1), txt_num_style);
	grid_sizer2->Add(lbl_txt24, 1, wxEXPAND);
	grid_sizer2->Add(txt24, 0, wxALIGN_RIGHT);
	txt24->Bind(wxEVT_TEXT, &PreferenceDlg::OnDateTimeInput, this);

    wxString lbl25 = _("Default displayed decimal places in Table:");
    wxStaticText* lbl_txt25 = new wxStaticText(gdal_page, wxID_ANY, lbl25);
    txt25 = new wxTextCtrl(gdal_page, XRCID("ID_DISPLAYED_DECIMALS"), "", pos,
                           txt_sz, txt_num_style);
    grid_sizer2->Add(lbl_txt25, 1, wxEXPAND);
    grid_sizer2->Add(txt25, 0, wxALIGN_RIGHT);
    txt25->Bind(wxEVT_TEXT, &PreferenceDlg::OnDisplayDecimal, this);

    wxString lbl27 = _("Create CSVT when saving CSV file:");
    wxStaticText* lbl_txt27 = new wxStaticText(gdal_page, wxID_ANY, lbl27);
    cbox_csvt = new wxCheckBox(gdal_page, wxID_ANY, "", pos);
    grid_sizer2->Add(lbl_txt27, 1, wxEXPAND);
    grid_sizer2->Add(cbox_csvt, 0, wxALIGN_RIGHT);
    cbox_csvt->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnCreateCSVT, this);

	grid_sizer2->AddGrowableCol(0, 1);

	wxBoxSizer *nb_box2 = new wxBoxSizer(wxVERTICAL);
	nb_box2->Add(grid_sizer2, 1, wxEXPAND | wxALL, 20);
	nb_box2->Fit(gdal_page);

	gdal_page->SetSizer(nb_box2);

	SetupControls();

	// overall

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    wxSize bt_sz = wxSize(70, 30);
	wxButton *resetButton = new wxButton(panel, wxID_ANY, _("Reset"), pos, bt_sz);
	wxButton *closeButton = new wxButton(panel, wxID_OK, _("Close"), pos, bt_sz);
	resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PreferenceDlg::OnReset, this);

	hbox->Add(resetButton, 1);
	hbox->Add(closeButton, 1, wxLEFT, 5);

	vbox->Add(notebook, 1, wxEXPAND | wxALL, 10);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

    panel->SetSizer(vbox);

    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);

    scrl->SetSizer(panelSizer);

    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(scrl, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);

	Centre();
	ShowModal();

	Destroy();
}

void PreferenceDlg::OnReset(wxCommandEvent& ev)
{
    GdaConst::gda_create_csvt = false;
    GdaConst::gda_use_gpu = false;
    GdaConst::gda_ui_language = 0;
    GdaConst::gda_eigen_tol = 1.0E-8;
    GdaConst::gda_draw_map_labels = false;
    GdaConst::gda_map_label_font_size = 8;
	GdaConst::gda_set_cpu_cores = true;
	GdaConst::gda_cpu_cores = 8;
	GdaConst::use_cross_hatching = false;
	GdaConst::transparency_highlighted = 255;
	GdaConst::transparency_unhighlighted = 100;
	//GdaConst::transparency_map_on_basemap = 200;
	GdaConst::use_basemap_by_default = false;
	GdaConst::default_basemap_selection = 0;
	GdaConst::hide_sys_table_postgres = false;
	GdaConst::hide_sys_table_sqlite = false;
	GdaConst::disable_crash_detect = false;
	GdaConst::disable_auto_upgrade = false;
	GdaConst::plot_transparency_highlighted = 255;
	GdaConst::plot_transparency_unhighlighted = 50;
	GdaConst::show_recent_sample_connect_ds_dialog = true;
	GdaConst::show_csv_configure_in_merge = true;
	GdaConst::enable_high_dpi_support = true;
    GdaConst::gdal_http_timeout = 5;
    GdaConst::use_gda_user_seed= true;
    GdaConst::gda_user_seed = 123456789;
    GdaConst::default_display_decimals = 6;
    GdaConst::gda_datetime_formats_str = "%Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S,%m/%d/%Y %H:%M:%S,%Y-%m-%d,%m/%d/%Y,%Y/%m/%d,%H:%M:%S,%H:%M,%Y/%m/%d %H:%M %p";
    GdaConst::gda_enable_set_transparency_windows = false;
    if (!GdaConst::gda_datetime_formats_str.empty()) {
        wxString patterns = GdaConst::gda_datetime_formats_str;
        wxStringTokenizer tokenizer(patterns, ",");
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            GdaConst::gda_datetime_formats.push_back(token);
        }
        GdaConst::gda_datetime_formats_str = patterns;
    }

	SetupControls();

	OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
	ogr_adapt.AddEntry("use_cross_hatching", "0");
	ogr_adapt.AddEntry("transparency_highlighted", "255");
	ogr_adapt.AddEntry("transparency_unhighlighted", "100");
	ogr_adapt.AddEntry("use_basemap_by_default", "0");
	ogr_adapt.AddEntry("default_basemap_selection", "0");
	ogr_adapt.AddEntry("hide_sys_table_postgres", "0");
	ogr_adapt.AddEntry("hide_sys_table_sqlite", "0");
	ogr_adapt.AddEntry("disable_crash_detect", "0");
	ogr_adapt.AddEntry("disable_auto_upgrade", "0");
	ogr_adapt.AddEntry("plot_transparency_highlighted", "255");
	ogr_adapt.AddEntry("plot_transparency_unhighlighted", "50");
	ogr_adapt.AddEntry("show_recent_sample_connect_ds_dialog", "1");
	ogr_adapt.AddEntry("show_csv_configure_in_merge", "1");
	ogr_adapt.AddEntry("enable_high_dpi_support", "1");
	ogr_adapt.AddEntry("gdal_http_timeout", "5");
	ogr_adapt.AddEntry("use_gda_user_seed", "1");
	ogr_adapt.AddEntry("gda_user_seed", "123456789");
	ogr_adapt.AddEntry("gda_datetime_formats_str", "%Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S,%m/%d/%Y %H:%M:%S,%Y-%m-%d,%m/%d/%Y,%Y/%m/%d,%H:%M:%S,%H:%M,%Y/%m/%d %H:%M %p");
	ogr_adapt.AddEntry("gda_cpu_cores", "8");
	ogr_adapt.AddEntry("gda_set_cpu_cores", "1");
	ogr_adapt.AddEntry("gda_eigen_tol", "1.0E-8");
    ogr_adapt.AddEntry("gda_ui_language", "0");
    ogr_adapt.AddEntry("gda_use_gpu", "0");
    ogr_adapt.AddEntry("gda_displayed_decimals", "6");
    ogr_adapt.AddEntry("gda_enable_set_transparency_windows", "0");
    ogr_adapt.AddEntry("gda_create_csvt", "0");
    ogr_adapt.AddEntry("gda_draw_map_labels", "0");
    ogr_adapt.AddEntry("gda_map_label_font_size", "8");
}

void PreferenceDlg::SetupControls()
{
	cbox0->SetValue(GdaConst::use_cross_hatching);
	slider1->SetValue(GdaConst::transparency_highlighted);
	wxString t_hl = wxString::Format("%.2f", (255 - GdaConst::transparency_highlighted) / 255.0);
	slider_txt1->SetValue(t_hl);
	slider2->SetValue(GdaConst::transparency_unhighlighted);
	wxString t_uhl = wxString::Format("%.2f", (255 - GdaConst::transparency_unhighlighted) / 255.0);
	slider_txt2->SetValue(t_uhl);
	if (GdaConst::use_basemap_by_default) {
		cmb33->SetSelection(GdaConst::default_basemap_selection);
	}
	else {
		cmb33->SetSelection(0);
	}

	slider7->SetValue(GdaConst::plot_transparency_unhighlighted);
	wxString t_p_hl = wxString::Format("%.2f", (255 - GdaConst::plot_transparency_unhighlighted) / 255.0);
	slider_txt7->SetValue(t_p_hl);

	cbox4->SetValue(GdaConst::disable_crash_detect);
	cbox5->SetValue(GdaConst::disable_auto_upgrade);
	cbox21->SetValue(GdaConst::hide_sys_table_postgres);
	cbox22->SetValue(GdaConst::hide_sys_table_sqlite);
	cbox8->SetValue(GdaConst::show_recent_sample_connect_ds_dialog);
	cbox9->SetValue(GdaConst::show_csv_configure_in_merge);
	cbox10->SetValue(GdaConst::enable_high_dpi_support);
    
    txt23->SetValue(wxString::Format("%d", GdaConst::gdal_http_timeout));
    txt24->SetValue(GdaConst::gda_datetime_formats_str);
    txt25->SetValue(wxString::Format("%d", GdaConst::default_display_decimals));

    cbox6->SetValue(GdaConst::use_gda_user_seed);
    wxString t_seed;
    t_seed << GdaConst::gda_user_seed;
    txt_seed->SetValue(t_seed);
    
    cbox18->SetValue(GdaConst::gda_set_cpu_cores);
    wxString t_cores;
    t_cores << GdaConst::gda_cpu_cores;
    txt_cores->SetValue(t_cores);
    
    wxString t_power_eps;
    t_power_eps << GdaConst::gda_eigen_tol;
    txt_poweriter_eps->SetValue(t_power_eps);
    
    cmb113->SetSelection(GdaConst::gda_ui_language);
    
    cbox_gpu->SetValue(GdaConst::gda_use_gpu);
    cbox26->SetValue(GdaConst::gda_enable_set_transparency_windows);

    cbox_csvt->SetValue(GdaConst::gda_create_csvt);
    
    cbox_lbl->SetValue(GdaConst::gda_draw_map_labels);
    wxString t_lbl_font_size;
    t_lbl_font_size << GdaConst::gda_map_label_font_size;
    txt_cores->SetValue(t_lbl_font_size);
}

void PreferenceDlg::ReadFromCache()
{
    OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
    
	vector<wxString> transp_h = ogr_adapt.GetHistory("transparency_highlighted");
	if (!transp_h.empty()) {
		long transp_l = 0;
		wxString transp = transp_h[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_highlighted = transp_l;
		}
	}
	vector<wxString> transp_uh = ogr_adapt.GetHistory("transparency_unhighlighted");
	if (!transp_uh.empty()) {
		long transp_l = 0;
		wxString transp = transp_uh[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_unhighlighted = transp_l;
		}
	}
	vector<wxString> plot_transparency_unhighlighted = ogr_adapt.GetHistory("plot_transparency_unhighlighted");
	if (!plot_transparency_unhighlighted.empty()) {
		long transp_l = 0;
		wxString transp = plot_transparency_unhighlighted[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::plot_transparency_unhighlighted = transp_l;
		}
	}
	vector<wxString> basemap_sel = ogr_adapt.GetHistory("default_basemap_selection");
	if (!basemap_sel.empty()) {
		long sel_l = 0;
		wxString sel = basemap_sel[0];
		if (sel.ToLong(&sel_l)) {
			GdaConst::default_basemap_selection = sel_l;
		}
	}
	vector<wxString> basemap_default = ogr_adapt.GetHistory("use_basemap_by_default");
	if (!basemap_default.empty()) {
		long sel_l = 0;
		wxString sel = basemap_default[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::use_basemap_by_default = true;
			else if (sel_l == 0)
				GdaConst::use_basemap_by_default = false;
		}
	}
	vector<wxString> crossht_sel = ogr_adapt.GetHistory("use_cross_hatching");
	if (!crossht_sel.empty()) {
		long cross_l = 0;
		wxString cross = crossht_sel[0];
		if (cross.ToLong(&cross_l)) {
			if (cross_l == 1)
				GdaConst::use_cross_hatching = true;
			else if (cross_l == 0)
				GdaConst::use_cross_hatching = false;
		}
	}
    vector<wxString> enable_transp_sel = ogr_adapt.GetHistory("gda_enable_set_transparency_windows");
    if (!enable_transp_sel.empty()) {
        long enable_l = 0;
        wxString enable_transp = enable_transp_sel[0];
        if (enable_transp.ToLong(&enable_l)) {
            if (enable_l == 1)
                GdaConst::gda_enable_set_transparency_windows = true;
            else if (enable_l == 0)
                GdaConst::gda_enable_set_transparency_windows = false;
        }
    }
	vector<wxString> postgres_sys_sel = ogr_adapt.GetHistory("hide_sys_table_postgres");
	if (!postgres_sys_sel.empty()) {
		long sel_l = 0;
		wxString sel = postgres_sys_sel[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::hide_sys_table_postgres = true;
			else if (sel_l == 0)
				GdaConst::hide_sys_table_postgres = false;
		}
	}
	vector<wxString> hide_sys_table_sqlite = ogr_adapt.GetHistory("hide_sys_table_sqlite");
	if (!hide_sys_table_sqlite.empty()) {
		long sel_l = 0;
		wxString sel = hide_sys_table_sqlite[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::hide_sys_table_sqlite = true;
			else if (sel_l == 0)
				GdaConst::hide_sys_table_sqlite = false;
		}
	}
	vector<wxString> disable_crash_detect = ogr_adapt.GetHistory("disable_crash_detect");
	if (!disable_crash_detect.empty()) {
		long sel_l = 0;
		wxString sel = disable_crash_detect[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::disable_crash_detect = true;
			else if (sel_l == 0)
				GdaConst::disable_crash_detect = false;
		}
	}
	vector<wxString> disable_auto_upgrade = ogr_adapt.GetHistory("disable_auto_upgrade");
	if (!disable_auto_upgrade.empty()) {
		long sel_l = 0;
		wxString sel = disable_auto_upgrade[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::disable_auto_upgrade = true;
			else if (sel_l == 0)
				GdaConst::disable_auto_upgrade = false;
		}
	}

	vector<wxString> show_recent_sample_connect_ds_dialog = ogr_adapt.GetHistory("show_recent_sample_connect_ds_dialog");
	if (!show_recent_sample_connect_ds_dialog.empty()) {
		long sel_l = 0;
		wxString sel = show_recent_sample_connect_ds_dialog[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::show_recent_sample_connect_ds_dialog = true;
			else if (sel_l == 0)
				GdaConst::show_recent_sample_connect_ds_dialog = false;
		}
	}

	vector<wxString> show_csv_configure_in_merge = ogr_adapt.GetHistory("show_csv_configure_in_merge");
	if (!show_csv_configure_in_merge.empty()) {
		long sel_l = 0;
		wxString sel = show_csv_configure_in_merge[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::show_csv_configure_in_merge = true;
			else if (sel_l == 0)
				GdaConst::show_csv_configure_in_merge = false;
		}
	}
	vector<wxString> enable_high_dpi_support = ogr_adapt.GetHistory("enable_high_dpi_support");
	if (!enable_high_dpi_support.empty()) {
		long sel_l = 0;
		wxString sel = enable_high_dpi_support[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::enable_high_dpi_support = true;
			else if (sel_l == 0)
				GdaConst::enable_high_dpi_support = false;
		}
	}
	vector<wxString> gdal_http_timeout = ogr_adapt.GetHistory("gdal_http_timeout");
	if (!gdal_http_timeout.empty()) {
		long sel_l = 0;
		wxString sel = gdal_http_timeout[0];
		if (sel.ToLong(&sel_l)) {
            GdaConst::gdal_http_timeout = sel_l;
		}
	}
    
    vector<wxString> gda_datetime_formats_str = ogr_adapt.GetHistory("gda_datetime_formats_str");
    if (!gda_datetime_formats_str.empty()) {
        wxString patterns = gda_datetime_formats_str[0];
        wxStringTokenizer tokenizer(patterns, ",");
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            GdaConst::gda_datetime_formats.push_back(token);
        }
        GdaConst::gda_datetime_formats_str = patterns;
    }
    
    vector<wxString> gda_user_seed = ogr_adapt.GetHistory("gda_user_seed");
    if (!gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_user_seed = sel_l;
        }
    }
    vector<wxString> use_gda_user_seed = ogr_adapt.GetHistory("use_gda_user_seed");
    if (!use_gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = use_gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1) {
                GdaConst::use_gda_user_seed = true;
                srand(GdaConst::gda_user_seed);
            } else if (sel_l == 0) {
                GdaConst::use_gda_user_seed = false;
            }
        }
    }
    
    vector<wxString> gda_set_cpu_cores = ogr_adapt.GetHistory("gda_set_cpu_cores");
    if (!gda_set_cpu_cores.empty()) {
        long sel_l = 0;
        wxString sel = gda_set_cpu_cores[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::gda_set_cpu_cores = true;
            else if (sel_l == 0)
                GdaConst::gda_set_cpu_cores = false;
        }
    }
    vector<wxString> gda_cpu_cores = ogr_adapt.GetHistory("gda_cpu_cores");
    if (!gda_cpu_cores.empty()) {
        long sel_l = 0;
        wxString sel = gda_cpu_cores[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_cpu_cores = sel_l;
        }
    }
    
    vector<wxString> gda_eigen_tol = ogr_adapt.GetHistory("gda_eigen_tol");
    if (!gda_eigen_tol.empty()) {
        double sel_l = 0;
        wxString sel = gda_eigen_tol[0];
        if (sel.ToDouble(&sel_l)) {
            GdaConst::gda_eigen_tol = sel_l;
        }
    }
    
    vector<wxString> gda_ui_language = ogr_adapt.GetHistory("gda_ui_language");
    if (!gda_ui_language.empty()) {
        long sel_l = 0;
        wxString sel = gda_ui_language[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_ui_language = sel_l;
        }
    }
    
    vector<wxString> gda_use_gpu = ogr_adapt.GetHistory("gda_use_gpu");
    if (!gda_use_gpu.empty()) {
        long sel_l = 0;
        wxString sel = gda_use_gpu[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
            GdaConst::gda_use_gpu = true;
            else if (sel_l == 0)
            GdaConst::gda_use_gpu = false;
        }
    }

    vector<wxString> gda_create_csvt = ogr_adapt.GetHistory("gda_create_csvt");
    if (!gda_create_csvt.empty()) {
        long sel_l = 0;
        wxString sel = gda_create_csvt[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::gda_create_csvt = true;
            else if (sel_l == 0)
                GdaConst::gda_create_csvt = false;
        }
    }

    vector<wxString> gda_disp_decimals = ogr_adapt.GetHistory("gda_displayed_decimals");
    if (!gda_disp_decimals.empty()) {
        long sel_l = 0;
        wxString sel = gda_disp_decimals[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::default_display_decimals = sel_l;
        }
    }

    vector<wxString> gda_draw_map_labels = ogr_adapt.GetHistory("gda_draw_map_labels");
    if (!gda_draw_map_labels.empty()) {
        long sel_l = 0;
        wxString sel = gda_draw_map_labels[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::gda_draw_map_labels = true;
            else if (sel_l == 0)
                GdaConst::gda_draw_map_labels = false;
        }
    }
    vector<wxString> gda_map_label_font_size = ogr_adapt.GetHistory("gda_map_label_font_size");
    if (!gda_map_label_font_size.empty()) {
        long sel_l = 0;
        wxString sel = gda_map_label_font_size[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_map_label_font_size = sel_l;
        }
    }
    
    // following are not in this UI, but still global variable
    vector<wxString> gda_user_email = ogr_adapt.GetHistory("gda_user_email");
    if (!gda_user_email.empty()) {
        wxString email = gda_user_email[0];
        GdaConst::gda_user_email = email;
    }
}

void PreferenceDlg::OnChooseLanguage(wxCommandEvent& ev)
{
    int lan_sel = ev.GetSelection();
    GdaConst::gda_ui_language = lan_sel;
    wxString sel_str;
    sel_str << GdaConst::gda_ui_language;
    OGRDataAdapter::GetInstance().AddEntry("gda_ui_language", sel_str);
    
    // also update the lang/config.ini content
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFile(exePath);
    wxString exeDir = exeFile.GetPathWithSep();
    wxString configPath = exeDir + "lang" + wxFileName::GetPathSeparator() + "config.ini";
    wxConfigBase * config = new wxFileConfig("GeoDa", wxEmptyString, configPath);
    
    if (lan_sel > 0) {
        long language = wxLANGUAGE_UNKNOWN;
        if (lan_sel == 1) {
            language = wxLANGUAGE_ENGLISH + 1;
        } else if (lan_sel == 2) {
            language = 45;//wxLANGUAGE_CHINESE + 1;
        } else if (lan_sel == 3) {
            language = 179;//wxLANGUAGE_SPANISH;
        } else if (lan_sel == 4) {
            language = wxLANGUAGE_RUSSIAN;
        }
        config->DeleteEntry("Translation");
        config->SetPath("Translation");
        config->Write("Language", language);
        config->Flush();
        delete config;
    }
    
    wxString msg = _("Please restart GeoDa to apply the language setup.");
    wxMessageDialog dlg(NULL, msg, _("Info"), wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
}

void PreferenceDlg::OnDateTimeInput(wxCommandEvent& ev)
{
    GdaConst::gda_datetime_formats.clear();
    wxString formats_str = txt24->GetValue();
    wxStringTokenizer tokenizer(formats_str, ",");
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        GdaConst::gda_datetime_formats.push_back(token);
    }
    GdaConst::gda_datetime_formats_str = formats_str;
    OGRDataAdapter::GetInstance().AddEntry("gda_datetime_formats_str", formats_str);
}

void PreferenceDlg::OnDisplayDecimal(wxCommandEvent& ev)
{
    wxString val = txt25->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::default_display_decimals = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_displayed_decimals", val);
        if (table_state) {
            table_state->SetRefreshEvtTyp();
            table_state->notifyObservers();
        }
    }
}

void PreferenceDlg::OnTimeoutInput(wxCommandEvent& ev)
{
    wxString sec_str = txt23->GetValue();
    long sec;
    if (sec_str.ToLong(&sec)) {
        if (sec >= 0) {
            GdaConst::gdal_http_timeout = sec;
            OGRDataAdapter::GetInstance().AddEntry("gdal_http_timeout", sec_str);
            CPLSetConfigOption("GDAL_HTTP_TIMEOUT", sec_str);
        }
    }
}

void PreferenceDlg::OnSlider1(wxCommandEvent& ev)
{
	int val = slider1->GetValue();
	GdaConst::transparency_highlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("transparency_highlighted", transp_str);
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER1_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider2(wxCommandEvent& ev)
{
	int val = slider2->GetValue();
	GdaConst::transparency_unhighlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("transparency_unhighlighted", transp_str);
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER2_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider6(wxCommandEvent& ev)
{
	int val = slider6->GetValue();
	GdaConst::plot_transparency_highlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_highlighted", transp_str);
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER6_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider7(wxCommandEvent& ev)
{
	int val = slider7->GetValue();
	GdaConst::plot_transparency_unhighlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_unhighlighted", transp_str);
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER7_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}

void PreferenceDlg::OnChoice3(wxCommandEvent& ev)
{
	int basemap_sel = ev.GetSelection();
	if (basemap_sel <= 0) {
		GdaConst::use_basemap_by_default = false;
		OGRDataAdapter::GetInstance().AddEntry("use_basemap_by_default", "0");
	}
	else {
		GdaConst::use_basemap_by_default = true;
		GdaConst::default_basemap_selection = basemap_sel;
		wxString sel_str;
		sel_str << GdaConst::default_basemap_selection;
		OGRDataAdapter::GetInstance().AddEntry("use_basemap_by_default", "1");
		OGRDataAdapter::GetInstance().AddEntry("default_basemap_selection", sel_str);
	}
}

void PreferenceDlg::OnCrossHatch(wxCommandEvent& ev)
{
	int crosshatch_sel = ev.GetSelection();
	if (crosshatch_sel == 0) {
		GdaConst::use_cross_hatching = false;
		OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "0");
	}
	else if (crosshatch_sel == 1) {
		GdaConst::use_cross_hatching = true;
		OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "1");
	}
	if (highlight_state) {
		highlight_state->notifyObservers();
	}
}

void PreferenceDlg::OnEnableTransparencyWin(wxCommandEvent& ev)
{
    int setup_transp_sel = ev.GetSelection();
    if (setup_transp_sel == 0) {
        GdaConst::gda_enable_set_transparency_windows = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_enable_set_transparency_windows", "0");
    }
    else if (setup_transp_sel == 1) {
        GdaConst::gda_enable_set_transparency_windows = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_enable_set_transparency_windows", "1");
    }
}

void PreferenceDlg::OnHideTablePostGIS(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::hide_sys_table_postgres = false;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_postgres", "0");
	}
	else {
		GdaConst::hide_sys_table_postgres = true;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_postgres", "1");
	}
}

void PreferenceDlg::OnHideTableSQLITE(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::hide_sys_table_sqlite = false;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_sqlite", "0");
	}
	else {
		GdaConst::hide_sys_table_sqlite = true;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_sqlite", "1");

	}
}
void PreferenceDlg::OnDisableCrashDetect(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::disable_crash_detect = false;
		OGRDataAdapter::GetInstance().AddEntry("disable_crash_detect", "0");
	}
	else {
		GdaConst::disable_crash_detect = true;
		OGRDataAdapter::GetInstance().AddEntry("disable_crash_detect", "1");

	}
}
void PreferenceDlg::OnDisableAutoUpgrade(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::disable_auto_upgrade = false;
		OGRDataAdapter::GetInstance().AddEntry("disable_auto_upgrade", "0");
	}
	else {
		GdaConst::disable_auto_upgrade = true;
		OGRDataAdapter::GetInstance().AddEntry("disable_auto_upgrade", "1");

	}
}
void PreferenceDlg::OnShowRecent(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::show_recent_sample_connect_ds_dialog = false;
		OGRDataAdapter::GetInstance().AddEntry("show_recent_sample_connect_ds_dialog", "0");
	}
	else {
		GdaConst::show_recent_sample_connect_ds_dialog = true;
		OGRDataAdapter::GetInstance().AddEntry("show_recent_sample_connect_ds_dialog", "1");

	}
}
void PreferenceDlg::OnShowCsvInMerge(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::show_csv_configure_in_merge = false;
		OGRDataAdapter::GetInstance().AddEntry("show_csv_configure_in_merge", "0");
	}
	else {
		GdaConst::show_csv_configure_in_merge = true;
		OGRDataAdapter::GetInstance().AddEntry("show_csv_configure_in_merge", "1");
	}
}
void PreferenceDlg::OnEnableHDPISupport(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::enable_high_dpi_support = false;
		OGRDataAdapter::GetInstance().AddEntry("enable_high_dpi_support", "0");
	}
	else {
		GdaConst::enable_high_dpi_support = true;
		OGRDataAdapter::GetInstance().AddEntry("enable_high_dpi_support", "1");
	}
}
void PreferenceDlg::OnUseSpecifiedSeed(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "0");
    }
    else {
        GdaConst::use_gda_user_seed = true;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
    }
}
void PreferenceDlg::OnSeedEnter(wxCommandEvent& ev)
{
    wxString val = txt_seed->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::gda_user_seed = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_user_seed", val);
    }
}
void PreferenceDlg::OnSetCPUCores(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::gda_set_cpu_cores = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_set_cpu_cores", "0");
        txt_cores->Disable();
    }
    else {
        GdaConst::gda_set_cpu_cores = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_set_cpu_cores", "1");
        txt_cores->Enable();
    }
}
void PreferenceDlg::OnCPUCoresEnter(wxCommandEvent& ev)
{
    wxString val = txt_cores->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::gda_cpu_cores = (int)_val;
        OGRDataAdapter::GetInstance().AddEntry("gda_cpu_cores", val);
    }
}

void PreferenceDlg::OnDrawLabels(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::gda_draw_map_labels = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_draw_map_labels", "0");
        txt_cores->Disable();
    }
    else {
        GdaConst::gda_draw_map_labels = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_draw_map_labels", "1");
        txt_cores->Enable();
    }
}
void PreferenceDlg::OnLabelFontSizeEnter(wxCommandEvent& ev)
{
    wxString val = txt_lbl_font->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::gda_map_label_font_size = (int)_val;
        OGRDataAdapter::GetInstance().AddEntry("gda_map_label_font_size", val);
    }
}

void PreferenceDlg::OnPowerEpsEnter(wxCommandEvent& ev)
{
    wxString val = txt_poweriter_eps->GetValue();
    double _val;
    if (val.ToDouble(&_val)) {
        GdaConst::gda_eigen_tol = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_eigen_tol", val);
    }
}
void PreferenceDlg::OnUseGPU(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::gda_use_gpu = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_use_gpu", "0");
    }
    else {
        GdaConst::gda_use_gpu = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_use_gpu", "1");
    }
}
void PreferenceDlg::OnCreateCSVT(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::gda_create_csvt = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_create_csvt", "0");
    }
    else {
        GdaConst::gda_create_csvt = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_create_csvt", "1");
    }
}
