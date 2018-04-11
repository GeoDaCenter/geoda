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



#include <fstream>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/panel.h>
#include <wx/dir.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/notebook.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/hyperlink.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/scoped_thread.hpp>

#include "../logger.h"
#include "../FramesManager.h"
#include "../Project.h"
#include "../Algorithms/geocoding.h"

#include "GeocodingDlg.h"

BEGIN_EVENT_TABLE( GeocodingDlg, wxDialog )
EVT_CLOSE( GeocodingDlg::OnClose )
END_EVENT_TABLE()

using namespace std;

GeocodingDlg::GeocodingDlg(wxWindow* parent, Project* p, const wxString& title, const wxPoint& pos, const wxSize& size )
: wxDialog(NULL, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
frames_manager(p->GetFramesManager()), project(p), table_int(p->GetTableInt()), table_state(p->GetTableState()), t(NULL), stop(false)
{
    wxLogMessage("Open GeocodingDlg().");
    CreateControls();
    Init();
	frames_manager->registerObserver(this);
    table_state->registerObserver(this);
}

GeocodingDlg::~GeocodingDlg()
{
    if (t!= NULL) {
        stop = true;
        t->join();
        delete t;
        t = NULL;
    }
    
    frames_manager->removeObserver(this);
    table_state->removeObserver(this);
}

void GeocodingDlg::OnClose(wxCloseEvent& ev)
{
    Destroy();
}

void GeocodingDlg::update(FramesManager* o)
{
}

void GeocodingDlg::update(TableState* o)
{
}
void GeocodingDlg::Init()
{
    m_choice_vars->Clear();
    std::vector<wxString> names;
    
    table_int->FillStringNameList(names);
    for (int i=0; i<names.size(); i++) {
        m_choice_vars->Append(names[i]);
    }
    m_choice_vars->SetSelection(0);
    
    m_google_input->Clear();
    m_google_input->SetValue("");
    m_google_input->SetFocus();
    m_google_input->SetCanFocus(true);
    
    m_prg->SetValue(0);
}

void GeocodingDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    wxStaticText* st01 = new wxStaticText(panel, wxID_ANY, _("Select variable with addresses:"));
    m_choice_vars = new wxChoice(panel, wxID_ANY, wxDefaultPosition);
    wxBoxSizer *hbox01 = new wxBoxSizer(wxHORIZONTAL);
    hbox01->Add(st01, 0, wxRIGHT, 10);
    hbox01->Add(m_choice_vars, 1, wxEXPAND);
    
    wxStaticBoxSizer *hbox0 = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Input:");
    hbox0->Add(hbox01, 1, wxEXPAND | wxALL, 10);
    
    // parameters
    wxNotebook* notebook = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    wxNotebookPage* google_page = new wxNotebookPage(notebook, -1, wxDefaultPosition, wxSize(560, 180));
    notebook->AddPage(google_page, _("Google Places API"));
    
    wxFlexGridSizer* gbox = new wxFlexGridSizer(5,1,10,0);
    
    wxStaticText* st11 = new wxStaticText(google_page, wxID_ANY, _("Enter your Google API keys (one key per line)"));
    m_google_input = new wxTextCtrl(google_page, wxID_ANY, "", wxDefaultPosition, wxSize(500, 120), wxTE_MULTILINE);
    wxHyperlinkCtrl* lnk = new wxHyperlinkCtrl(google_page, wxID_ANY, _("Click here to get a Google API key"), "https://developers.google.com/maps/documentation/geocoding/start");
    
    wxBoxSizer *vbox10 = new wxBoxSizer(wxVERTICAL);
    vbox10->Add(st11, 0, wxALL, 5);
    vbox10->Add(m_google_input, 1, wxEXPAND|wxALL, 5);
    vbox10->Add(lnk, 0, wxALL, 5);

	wxBoxSizer *nb_box1 = new wxBoxSizer(wxVERTICAL);
	nb_box1->Add(vbox10, 1, wxEXPAND | wxALL, 20);
	nb_box1->Fit(google_page);

    google_page->SetSizer(nb_box1);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(notebook, 0, wxEXPAND);

    // output
    wxStaticText* st21 = new wxStaticText(panel, wxID_ANY, _("Save latitude to field:"));
    m_lat = new wxTextCtrl(panel, wxID_ANY, "G_LAT");
    
    wxStaticText* st22 = new wxStaticText(panel, wxID_ANY, _("Save longitude to field:"));
    m_lng = new wxTextCtrl(panel, wxID_ANY, "G_LNG");
    
	wxFlexGridSizer* grid_sizer1 = new wxFlexGridSizer(2, 2, 5, 5);
    grid_sizer1->Add(st21);
    grid_sizer1->Add(m_lat);
    grid_sizer1->Add(st22);
    grid_sizer1->Add(m_lng);
    
    wxStaticBoxSizer *hbox1 = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Output:"));
	//hbox1->Add(st21);
    hbox1->Add(grid_sizer1, 0, wxEXPAND | wxALL, 10);
    
    // buttons
    okButton = new wxButton(panel, wxID_OK, wxT("Run"), wxDefaultPosition,
                                      wxSize(70, 30));
    stopButton = new wxButton(panel, wxID_ANY, wxT("Stop"), wxDefaultPosition,
                                      wxSize(70, 30));
    closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(stopButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
   
    // progress bar
    m_prg = new wxGauge(panel, wxID_ANY, project->GetNumRecords());
    
    // Container
    vbox->Add(hbox0, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxALL, 10);
    vbox->Add(m_prg, 0, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    //SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &GeocodingDlg::OnOkClick, this);
    stopButton->Bind(wxEVT_BUTTON, &GeocodingDlg::OnStopClick, this);
    closeButton->Bind(wxEVT_BUTTON, &GeocodingDlg::OnCloseClick, this);
}

void GeocodingDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("In GeocodingDlg::OnOkClick()");
    if (t!= NULL) {
        t->join();
        delete t;
        t = NULL;
    }
    t = new boost::thread(boost::bind(&GeocodingDlg::run, this));
}

void GeocodingDlg::OnStopClick( wxCommandEvent& event )
{
    stop = true;
    okButton->Enable();
    stopButton->Disable();
    closeButton->Enable();
}

void GeocodingDlg::run()
{
    wxLogMessage("In GeocodingDlg::run()");
    
    // get select variable
    wxString sel_var = m_choice_vars->GetString(m_choice_vars->GetSelection());
    if (sel_var.empty()) {
        wxMessageDialog dlg(this, _("Please select a field with addresses."),  _("Warning"),
                            wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    // get keys
    wxString google_keys = m_google_input->GetValue();
    if (google_keys.empty() ) {
        wxMessageDialog dlg(this, _("Please enter Google key(s)."),  _("Warning"),
                            wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    vector<wxString> keys;
    wxStringTokenizer tokenizer(google_keys, "\n");
    while ( tokenizer.HasMoreTokens() ) {
        wxString token = tokenizer.GetNextToken();
        keys.push_back(token.Trim());
    }
    
    // detect output fields
    wxString lat_fname = m_lat->GetValue();
    if (lat_fname.empty()) {
        wxMessageDialog dlg(this, _("Please input a field name for saving latitude"),  _("Warning"),wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    wxString lng_fname = m_lng->GetValue();
    if (lng_fname.empty()) {
        wxMessageDialog dlg(this, _("Please input a field name for saving longitude"), _("Warning"), wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    int time=0;
    int col_lat = table_int->FindColId(lat_fname);
    if ( col_lat != wxNOT_FOUND) {
        // detect if column is integer field, if not raise a warning
        if (table_int->GetColType(col_lat) != GdaConst::double_type ) {
            wxString msg = _("This field name already exists (non-float type). Please input a unique name.");
            wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    int col_lng = table_int->FindColId(lng_fname);
    if ( col_lng != wxNOT_FOUND) {
        // detect if column is integer field, if not raise a warning
        if (table_int->GetColType(col_lng) != GdaConst::double_type ) {
            wxString msg = _("This field name already exists (non-float type). Please input a unique name.");
            wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
            dlg.ShowModal();
            return;
        }
    }
    
    // start
    stop = false;
    okButton->Disable();
    stopButton->Enable();
    closeButton->Disable();
    m_prg->SetValue(1);
    
    int n_rows = project->GetNumRecords();
    std::vector<wxString> addresses;
    int col_var = table_int->FindColId(sel_var);
    table_int->GetColData(col_var, time, addresses);
  
    vector<double> lats(n_rows);
    vector<double> lngs(n_rows);
    vector<bool> undefs(n_rows, true);
   
    if ( col_lat != wxNOT_FOUND)
        table_int->GetColData(col_lat, time, lats, undefs);
    if ( col_lng != wxNOT_FOUND)
        table_int->GetColData(col_lng, time, lngs, undefs);
    
    GoogleGeoCoder coder(keys);
    coder.geocoding(addresses, lats, lngs, undefs, m_prg, &stop);
  
    if ( col_lat == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_double_len;
        int m_decimals_val = GdaConst::default_dbf_double_decimals;
        col_lat = table_int->InsertCol(GdaConst::double_type, lat_fname, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    }
    if ( col_lng == wxNOT_FOUND) {
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_double_len;
        int m_decimals_val = GdaConst::default_dbf_double_decimals;
        col_lng = table_int->InsertCol(GdaConst::double_type, lng_fname, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    }
    table_int->SetColData(col_lat, time, coder.lats, coder.undefs);
    table_int->SetColData(col_lng, time, coder.lngs, coder.undefs);
    
    wxString msg = _("Successful Geocode. Please check results in Table.");
    if (!coder.error_msg.empty()) {
        msg << "\n\nDetails:\n\n";
        msg << coder.error_msg;
    }
    wxMessageDialog dlg(this, msg, _("Info"), wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
    stop = false;
    
    okButton->Enable();
    stopButton->Disable();
    closeButton->Enable();
	//EndDialog(wxID_OK);
}

void GeocodingDlg::OnCloseClick( wxCommandEvent& event )
{
    wxLogMessage("In GeocodingDlg::OnOkClick()");
    
	EndDialog(wxID_CANCEL);
}
