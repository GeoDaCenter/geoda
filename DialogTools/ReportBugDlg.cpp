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
#include <wx/statbmp.h>
#include <wx/filedlg.h>
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
#include "ReportBugDlg.h"

using namespace std;
using namespace GdaJson;


IMPLEMENT_CLASS(WelcomeSelectionStyleDlg, wxDialog)

BEGIN_EVENT_TABLE(WelcomeSelectionStyleDlg, wxDialog)
END_EVENT_TABLE()

WelcomeSelectionStyleDlg::WelcomeSelectionStyleDlg(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
{
	SetParent(parent);
	SetPosition(pos);

	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_WELCOME_SELECTION_STYLE");

	wxStaticBitmap* m_sel_style1 = XRCCTRL(*this, "IDC_SELECTION_STYLE1", wxStaticBitmap);
	wxStaticBitmap* m_sel_style2 = XRCCTRL(*this, "IDC_SELECTION_STYLE2", wxStaticBitmap);

	m_sel_style1->Bind(wxEVT_LEFT_DOWN, &WelcomeSelectionStyleDlg::OnStyle1, this);
	m_sel_style2->Bind(wxEVT_LEFT_DOWN, &WelcomeSelectionStyleDlg::OnStyle2, this);

	Centre();
	Move(pos);

	ShowModal();

	Destroy();
}

void WelcomeSelectionStyleDlg::OnStyle1(wxMouseEvent& ev)
{
	GdaConst::use_cross_hatching = false;
	OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "0");
	EndDialog(wxID_OK);
}

void WelcomeSelectionStyleDlg::OnStyle2(wxMouseEvent& ev)
{
	GdaConst::use_cross_hatching = true;
	OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "1");
	EndDialog(wxID_OK);
}

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
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
	highlight_state = NULL;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

PreferenceDlg::PreferenceDlg(wxWindow* parent,
	HLStateInt* _highlight_state,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
	highlight_state = _highlight_state;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

void PreferenceDlg::Init()
{
	ReadFromCache();

	wxNotebook* notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	//  visualization tab
	wxNotebookPage* vis_page = new wxNotebookPage(notebook, -1, wxDefaultPosition, wxSize(560, 580));
#ifdef __WIN32__
	vis_page->SetBackgroundColour(*wxWHITE);
#endif
	notebook->AddPage(vis_page, "System");
	wxFlexGridSizer* grid_sizer1 = new wxFlexGridSizer(16, 2, 12, 15);

	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Maps:")), 1);
	grid_sizer1->AddSpacer(10);

	wxString lbl0 = _("Use classic yellow cross-hatching to highlight selection in maps:");
	wxStaticText* lbl_txt0 = new wxStaticText(vis_page, wxID_ANY, lbl0);
	cbox0 = new wxCheckBox(vis_page, XRCID("PREF_USE_CROSSHATCH"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt0, 1, wxEXPAND);
	grid_sizer1->Add(cbox0, 0, wxALIGN_RIGHT);
	cbox0->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnCrossHatch, this);

	wxSize sl_sz(200, -1);
	wxSize txt_sz(35, -1);

	wxString lbl1 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt1 = new wxStaticText(vis_page, wxID_ANY, lbl1);
	wxBoxSizer* box1 = new wxBoxSizer(wxHORIZONTAL);
	slider1 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt1 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER1_TXT"), "",
		wxDefaultPosition, txt_sz, wxTE_READONLY);
	box1->Add(slider1);
	box1->Add(slider_txt1);
	grid_sizer1->Add(lbl_txt1, 1, wxEXPAND);
	grid_sizer1->Add(box1, 0, wxALIGN_RIGHT);
	slider1->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider1, this);

	wxString lbl2 = _("Set transparency of unhighlighted objects in selection:");
	wxStaticText* lbl_txt2 = new wxStaticText(vis_page, wxID_ANY, lbl2);
	wxBoxSizer* box2 = new wxBoxSizer(wxHORIZONTAL);
	slider2 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt2 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER2_TXT"), "",
		wxDefaultPosition, txt_sz, wxTE_READONLY);
	box2->Add(slider2);
	box2->Add(slider_txt2);
	grid_sizer1->Add(lbl_txt2, 1, wxEXPAND);
	grid_sizer1->Add(box2, 0, wxALIGN_RIGHT);
	slider2->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider2, this);


	wxString lbl3 = _("Add basemap automatically:");
	wxStaticText* lbl_txt3 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	//wxStaticText* lbl_txt33 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	cmb33 = new wxComboBox(vis_page, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	cmb33->Append("No basemap");
	cmb33->Append("Carto Light");
	cmb33->Append("Carto Dark");
	cmb33->Append("Carto Light (No Labels)");
	cmb33->Append("Carto Dark (No Labels)");
	cmb33->Append("Nokia Day");
	cmb33->Append("Nokia Night");
	cmb33->Append("Nokia Hybrid");
	cmb33->Append("Nokia Satellite");
	cmb33->SetSelection(0);
	cmb33->Bind(wxEVT_COMBOBOX, &PreferenceDlg::OnChoice3, this);

	grid_sizer1->Add(lbl_txt3, 1, wxEXPAND);
	grid_sizer1->Add(cmb33, 0, wxALIGN_RIGHT);


	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Plots:")), 1,
		wxTOP | wxBOTTOM, 20);
	grid_sizer1->AddSpacer(10);


	wxString lbl6 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt6 = new wxStaticText(vis_page, wxID_ANY, lbl6);
	wxBoxSizer* box6 = new wxBoxSizer(wxHORIZONTAL);
	slider6 = new wxSlider(vis_page, XRCID("PREF_SLIDER6"),
		255, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	wxTextCtrl* slider_txt6 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER6_TXT"), "0.0", wxDefaultPosition, txt_sz, wxTE_READONLY);
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
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt7 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER7_TXT"), "", wxDefaultPosition, txt_sz, wxTE_READONLY);
	box7->Add(slider7);
	box7->Add(slider_txt7);
	grid_sizer1->Add(lbl_txt7, 1, wxEXPAND);
	grid_sizer1->Add(box7, 0, wxALIGN_RIGHT);
	slider7->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider7, this);


	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("System:")), 1,
		wxTOP | wxBOTTOM, 20);
	grid_sizer1->AddSpacer(10);


	wxString lbl8 = _("Show Recent/Sample Data panel in Connect Datasource Dialog:");
	wxStaticText* lbl_txt8 = new wxStaticText(vis_page, wxID_ANY, lbl8);
	cbox8 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_RECENT"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt8, 1, wxEXPAND);
	grid_sizer1->Add(cbox8, 0, wxALIGN_RIGHT);
	cbox8->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowRecent, this);

	wxString lbl9 = _("Show CSV Configuration in Merge Data Dialog:");
	wxStaticText* lbl_txt9 = new wxStaticText(vis_page, wxID_ANY, lbl9);
	cbox9 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_CSV_IN_MERGE"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt9, 1, wxEXPAND);
	grid_sizer1->Add(cbox9, 0, wxALIGN_RIGHT);
	cbox9->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowCsvInMerge, this);

	wxString lbl10 = _("Enable High DPI/Retina support (Mac only):");
	wxStaticText* lbl_txt10 = new wxStaticText(vis_page, wxID_ANY, lbl10);
	cbox10 = new wxCheckBox(vis_page, XRCID("PREF_ENABLE_HDPI"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt10, 1, wxEXPAND);
	grid_sizer1->Add(cbox10, 0, wxALIGN_RIGHT);
	cbox10->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnEnableHDPISupport, this);

	wxString lbl4 = _("Disable crash detection for bug report:");
	wxStaticText* lbl_txt4 = new wxStaticText(vis_page, wxID_ANY, lbl4);
	cbox4 = new wxCheckBox(vis_page, XRCID("PREF_CRASH_DETECT"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt4, 1, wxEXPAND);
	grid_sizer1->Add(cbox4, 0, wxALIGN_RIGHT);
	cbox4->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableCrashDetect, this);

	wxString lbl5 = _("Disable auto upgrade:");
	wxStaticText* lbl_txt5 = new wxStaticText(vis_page, wxID_ANY, lbl5);
	cbox5 = new wxCheckBox(vis_page, XRCID("PREF_AUTO_UPGRADE"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt5, 1, wxEXPAND);
	grid_sizer1->Add(cbox5, 0, wxALIGN_RIGHT);
	cbox5->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableAutoUpgrade, this);

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
	wxFlexGridSizer* grid_sizer2 = new wxFlexGridSizer(10, 2, 15, 20);

	wxString lbl21 = _("Hide system table in Postgresql connection:");
	wxStaticText* lbl_txt21 = new wxStaticText(gdal_page, wxID_ANY, lbl21);
	cbox21 = new wxCheckBox(gdal_page, wxID_ANY, "", wxDefaultPosition);
	grid_sizer2->Add(lbl_txt21, 1, wxEXPAND | wxTOP, 10);
	grid_sizer2->Add(cbox21, 0, wxALIGN_RIGHT | wxTOP, 13);
	cbox21->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTablePostGIS, this);


	wxString lbl22 = _("Hide system table in SQLITE connection:");
	wxStaticText* lbl_txt22 = new wxStaticText(gdal_page, wxID_ANY, lbl22);
	cbox22 = new wxCheckBox(gdal_page, wxID_ANY, "", wxDefaultPosition);
	grid_sizer2->Add(lbl_txt22, 1, wxEXPAND);
	grid_sizer2->Add(cbox22, 0, wxALIGN_RIGHT);
	cbox22->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTableSQLITE, this);

    
	wxString lbl23 = _("Http connection timeout (seconds) for e.g. WFS, Geojson etc.:");
	wxStaticText* lbl_txt23 = new wxStaticText(gdal_page, wxID_ANY, lbl23);
	txt23 = new wxTextCtrl(gdal_page, XRCID("ID_HTTP_TIMEOUT"), "", wxDefaultPosition, txt_sz, wxTE_PROCESS_ENTER);
	grid_sizer2->Add(lbl_txt23, 1, wxEXPAND);
	grid_sizer2->Add(txt23, 0, wxALIGN_RIGHT);
	txt23->Bind(wxEVT_TEXT, &PreferenceDlg::OnTimeoutInput, this);
   
	grid_sizer2->AddGrowableCol(0, 1);

	wxBoxSizer *nb_box2 = new wxBoxSizer(wxVERTICAL);
	nb_box2->Add(grid_sizer2, 1, wxEXPAND | wxALL, 20);
	nb_box2->Fit(gdal_page);

	gdal_page->SetSizer(nb_box2);

	SetupControls();

	// overall

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	wxButton *resetButton = new wxButton(this, -1, _("Reset"), wxDefaultPosition, wxSize(70, 30));
	wxButton *closeButton = new wxButton(this, wxID_OK, _("Close"), wxDefaultPosition, wxSize(70, 30));
	resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PreferenceDlg::OnReset, this);

	hbox->Add(resetButton, 1);
	hbox->Add(closeButton, 1, wxLEFT, 5);

	vbox->Add(notebook, 1, wxEXPAND | wxALL, 10);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizer(vbox);
	vbox->Fit(this);
    
	Centre();
	ShowModal();

	Destroy();
}

void PreferenceDlg::OnReset(wxCommandEvent& ev)
{
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
	GdaConst::show_csv_configure_in_merge = false;
	GdaConst::enable_high_dpi_support = true;
    GdaConst::gdal_http_timeout = 5;
    

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
	ogr_adapt.AddEntry("show_csv_configure_in_merge", "0");
	ogr_adapt.AddEntry("enable_high_dpi_support", "1");
	ogr_adapt.AddEntry("gdal_http_timeout", "5");
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
}

void PreferenceDlg::ReadFromCache()
{
	vector<string> transp_h = OGRDataAdapter::GetInstance().GetHistory("transparency_highlighted");
	if (!transp_h.empty()) {
		long transp_l = 0;
		wxString transp = transp_h[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_highlighted = transp_l;
		}
	}
	vector<string> transp_uh = OGRDataAdapter::GetInstance().GetHistory("transparency_unhighlighted");
	if (!transp_uh.empty()) {
		long transp_l = 0;
		wxString transp = transp_uh[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_unhighlighted = transp_l;
		}
	}
	vector<string> plot_transparency_unhighlighted = OGRDataAdapter::GetInstance().GetHistory("plot_transparency_unhighlighted");
	if (!plot_transparency_unhighlighted.empty()) {
		long transp_l = 0;
		wxString transp = plot_transparency_unhighlighted[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::plot_transparency_unhighlighted = transp_l;
		}
	}
	vector<string> basemap_sel = OGRDataAdapter::GetInstance().GetHistory("default_basemap_selection");
	if (!basemap_sel.empty()) {
		long sel_l = 0;
		wxString sel = basemap_sel[0];
		if (sel.ToLong(&sel_l)) {
			GdaConst::default_basemap_selection = sel_l;
		}
	}
	vector<string> basemap_default = OGRDataAdapter::GetInstance().GetHistory("use_basemap_by_default");
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
	vector<string> crossht_sel = OGRDataAdapter::GetInstance().GetHistory("use_cross_hatching");
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
	vector<string> postgres_sys_sel = OGRDataAdapter::GetInstance().GetHistory("hide_sys_table_postgres");
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
	vector<string> hide_sys_table_sqlite = OGRDataAdapter::GetInstance().GetHistory("hide_sys_table_sqlite");
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
	vector<string> disable_crash_detect = OGRDataAdapter::GetInstance().GetHistory("disable_crash_detect");
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
	vector<string> disable_auto_upgrade = OGRDataAdapter::GetInstance().GetHistory("disable_auto_upgrade");
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

	vector<string> show_recent_sample_connect_ds_dialog = OGRDataAdapter::GetInstance().GetHistory("show_recent_sample_connect_ds_dialog");
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

	vector<string> show_csv_configure_in_merge = OGRDataAdapter::GetInstance().GetHistory("show_csv_configure_in_merge");
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
	vector<string> enable_high_dpi_support = OGRDataAdapter::GetInstance().GetHistory("enable_high_dpi_support");
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
	vector<string> gdal_http_timeout = OGRDataAdapter::GetInstance().GetHistory("gdal_http_timeout");
	if (!gdal_http_timeout.empty()) {
		long sel_l = 0;
		wxString sel = gdal_http_timeout[0];
		if (sel.ToLong(&sel_l)) {
            GdaConst::gdal_http_timeout = sel_l;
		}
	}
    
    // following are not in this UI, but still global variable
    vector<string> gda_user_seed = OGRDataAdapter::GetInstance().GetHistory("gda_user_seed");
    if (!gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_user_seed = sel_l;
        }
    }
    vector<string> use_gda_user_seed = OGRDataAdapter::GetInstance().GetHistory("use_gda_user_seed");
    if (!use_gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = use_gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::use_gda_user_seed = true;
            else if (sel_l == 0)
                GdaConst::use_gda_user_seed = false;
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
            OGRDataAdapter::GetInstance().AddEntry("gdal_http_timeout", sec_str.ToStdString());
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
	OGRDataAdapter::GetInstance().AddEntry("transparency_highlighted", transp_str.ToStdString());
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
	OGRDataAdapter::GetInstance().AddEntry("transparency_unhighlighted", transp_str.ToStdString());
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
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_highlighted", transp_str.ToStdString());
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
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_unhighlighted", transp_str.ToStdString());
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
		OGRDataAdapter::GetInstance().AddEntry("default_basemap_selection", sel_str.ToStdString());
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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ReportResultDlg::ReportResultDlg(wxWindow* parent, wxString issue_url,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size)
{
	wxPanel* panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);

	wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);

	wxString result_tip = _("Thank you for helping us improve GeoDa with your bug report! \n\nYou can track our response and add screenshots or details here (or email us at spatial@uchicago.edu):");

	wxStaticText* lbl_tip = new wxStaticText(panel, wxID_ANY, result_tip);
	m_hyperlink1 = new wxHyperlinkCtrl(panel, wxID_ANY, issue_url,
		issue_url);
	bSizer->Add(lbl_tip, 1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	bSizer->Add(m_hyperlink1, 1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 0);

	panel->SetSizerAndFit(bSizer);

	wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
	sizerAll->Add(panel, 1, wxEXPAND | wxALL);
	SetSizer(sizerAll);
	SetAutoLayout(true);
	Centre(wxBOTH);
}

ReportResultDlg::~ReportResultDlg()
{
}


size_t write_to_string_(void *ptr, size_t size, size_t count, void *stream) {
	((string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

string CreateIssueOnGithub(string& post_data)
{
	std::vector<std::string> tester_ids = OGRDataAdapter::GetInstance().GetHistory("tester_id");
	if (tester_ids.empty()) {
		return "";
	}

	wxString tester_id = tester_ids[0];

	string url = "https://api.github.com/repos/GeoDaCenter/geoda/issues";

	wxString header_auth = "Authorization: token " + tester_id;

	wxString header_user_agent = "User-Agent: GeoDaTester";

	string response;

	CURL* curl = curl_easy_init();
	CURLcode res;
	if (curl) {
		struct curl_slist *chunk = NULL;

		chunk = curl_slist_append(chunk, header_auth.c_str());
		chunk = curl_slist_append(chunk, header_user_agent.c_str());

		// set our custom set of headers
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string_);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		/* free the custom headers */
		curl_slist_free_all(chunk);

	}
	return response;
}



ReportBugDlg::ReportBugDlg(wxWindow* parent, wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size)
{
	wxLogMessage("Open ReportBugDlg.");
	//
	// Description: please briefly describe what went wrong
	// Steps you took before something went wrong (Optional):
	// Data you used (Optional): __________________________
	//
	// Create controls UI
	wxPanel* panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);

	desc_tip = _("[Please briefly describe what went wrong]");
	steps_txt = _("[Steps you took before something went wrong]");

	title_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, desc_tip);
	steps_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, steps_txt,
		wxDefaultPosition,
		wxSize(500, 200),
		wxTE_MULTILINE);

	//wxString data_txt = _("Share part of your data for troubleshooting? (first 10 records)");
	//wxCheckBox* data_chk = new wxCheckBox(panel, wxID_ANY, data_txt);

	wxString user_tip = _("Your Github account (Optional):");
	wxStaticText* lbl_user = new wxStaticText(panel, wxID_ANY, user_tip);
	user_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
		wxDefaultPosition, wxSize(150, -1));
	wxBoxSizer* user_box = new wxBoxSizer(wxHORIZONTAL);
	user_box->Add(lbl_user);
	user_box->Add(user_txt_ctrl);

	wxString email_tip = _("Your Email address (Optional):");
	wxStaticText* lbl_email = new wxStaticText(panel, wxID_ANY, email_tip);
	email_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
		wxDefaultPosition, wxSize(150, -1));
	wxBoxSizer* email_box = new wxBoxSizer(wxHORIZONTAL);
	email_box->Add(lbl_email);
	email_box->AddSpacer(10);
	email_box->Add(email_txt_ctrl);

	// buttons
	wxButton* btn_cancel = new wxButton(panel, wxID_CANCEL, _("Cancel"),
		wxDefaultPosition,
		wxDefaultSize, wxBU_EXACTFIT);
	wxButton* btn_submit = new wxButton(panel, wxID_ANY, _("Submit Bug Report"),
		wxDefaultPosition,
		wxDefaultSize, wxBU_EXACTFIT);

	wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
	btn_box->Add(btn_cancel, 1, wxALIGN_CENTER | wxALL, 10);
	btn_box->Add(btn_submit, 1, wxALIGN_CENTER | wxALL, 10);

	wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
	box->Add(title_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(steps_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	//box->Add(data_chk, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
	box->Add(user_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(email_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(btn_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

	panel->SetSizerAndFit(box);

	wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
	sizerAll->Add(panel, 1, wxEXPAND | wxALL);
	SetSizer(sizerAll);
	SetAutoLayout(true);

	btn_submit->Bind(wxEVT_BUTTON, &ReportBugDlg::OnOkClick, this);


	SetParent(parent);
	SetPosition(pos);
	Centre();


}

ReportBugDlg::~ReportBugDlg()
{

}

void ReportBugDlg::OnOkClick(wxCommandEvent& event)
{
	wxLogMessage("In ReportBugDlg::OnOkClick()");
	//wxString rst = CreateIssueOnGithub("{\"title\": \"Test reporting bug from GeoDa software\", \"body\": \"We should have one\"}");

	wxString title = title_txt_ctrl->GetValue();
	wxString body = steps_txt_ctrl->GetValue();
	wxString user_github = user_txt_ctrl->GetValue();
	wxString email = email_txt_ctrl->GetValue();

	if (title.IsEmpty() || title == desc_tip) {
		wxMessageDialog msgDlg(this,
			_("Please briefly describe what went wrong."),
			_("Input is required"),
			wxOK | wxICON_INFORMATION);
		msgDlg.ShowModal();
		return;
	}
	if (body.IsEmpty() || body == steps_txt) {
		wxMessageDialog msgDlg(this,
			_("Please describe steps you took before something went wrong."),
			_("Input is required"),
			wxOK | wxICON_INFORMATION);
		msgDlg.ShowModal();
		return;
	}

	if (!user_github.IsEmpty()) {
		body << "\\n\\n@" << user_github << " " << email;
	}

	body << "\\n\\n";

	bool result = CreateIssue(title, body);

	if (result) {
		EndDialog(wxID_OK);
		return;
	}

	wxMessageDialog msgDlg(this,
		_("Oops. GeoDa was unable to submit a bug report. Please try again or create it here instead: https://github.com/GeoDaCenter/geoda/issues Thanks!"),
		_("Submit Bug Error"),
		wxOK | wxICON_INFORMATION);
	msgDlg.ShowModal();

	wxLogMessage("Submit Bug Report Error:");
	wxLogMessage("title:");
	wxLogMessage(title);
}

void ReportBugDlg::OnCancelClick(wxCommandEvent& event)
{

}

bool ReportBugDlg::CreateIssue(wxString title, wxString body)
{
	body.Replace("\n", "\\n");
	// get log file to body
	wxString logger_path;
	logger_path << GenUtils::GetSamplesDir() << "logger.txt";
    
	body << "\\n";
    
	wxTextFile tfile;
    if (tfile.Open(logger_path)) {
    	while (!tfile.Eof())
    	{
    		body << tfile.GetNextLine() << "\\n";
    	}
    }

	body.Replace("\"", "'");
	body.Replace("\t", "");
	body.Replace("\r", "");

	wxString labels = "[\"AutoBugReport\"]";
	//wxString assignees = "[\"GeoDaTester\"]";

	//wxString msg_templ = "{\"title\": \"%s\", \"body\": \"%s\", \"labels\": %s}";
	wxString json_msg = "{\"title\": \"";
	json_msg << title;
	json_msg << "\", \"body\": \"";
	json_msg << body;
	json_msg << "\", \"labels\": ";
	json_msg << labels;
	json_msg << "}";
	//wxString json_msg = wxString::Format(msg_templ, title, body, labels);

	string msg(json_msg.mb_str(wxConvUTF8));

	string result = CreateIssueOnGithub(msg);

	// parse results

	if (!result.empty()) {
		json_spirit::Value v;
		try {
			if (!json_spirit::read(result, v)) {
				throw std::runtime_error("Could not parse title as JSON");
			}
			json_spirit::Value url_val;
			if (!GdaJson::findValue(v, url_val, "html_url")) {
				throw std::runtime_error("could not find url");
			}
			wxString url_issue = url_val.get_str();
			ReportResultDlg dlg(NULL, url_issue);
			dlg.ShowModal();
			return true;
		}
		catch (std::runtime_error e) {
			wxString msg;
			msg << "JSON parsing failed: ";
			msg << e.what();
		}
	}
	return false;
}
