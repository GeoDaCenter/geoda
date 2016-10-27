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
#include <wx/combobox.h>
#include "curl/curl.h"

#include "ReportBugDlg.h"

using namespace std;

ReportResultDlg::ReportResultDlg( wxWindow* parent, wxString issue_url,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size)
: wxDialog(parent, id, title, pos, size)
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );
    
    wxString result_tip = _("Thanks for reporting bug! You can click the following link to check and trace the reported bug. \n\nGeoDa team thanks you to upload your data or screenshots for troubleshooting using this link or send to spatial@uchicago.edu privately.");
    
    wxStaticText* lbl_tip = new wxStaticText(panel, wxID_ANY, result_tip);
    m_hyperlink1 = new wxHyperlinkCtrl(panel, wxID_ANY, issue_url,
                                       issue_url);
    bSizer->Add(lbl_tip,  1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    bSizer->Add(m_hyperlink1,  1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 0);
    
    panel->SetSizerAndFit(bSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    Centre( wxBOTH );
}

ReportResultDlg::~ReportResultDlg()
{
}


size_t write_to_string_(void *ptr, size_t size, size_t count, void *stream) {
    ((string*)stream)->append((char*)ptr, 0, size*count);
    return size*count;
}

string CreateIssueOnGithub(const string& post_data)
{
    string url = "https://api.github.com/repos/lixun910/colamap/issues";
    
    string header_str = "Authorization: token ab6738a78ec5c1c21d18b1059563e0132438f409";
   
    string header_str1 = "User-Agent: GeoDaTester";
    string response;
    
    CURL* curl = curl_easy_init();
    CURLcode res;
    if (curl) {
        struct curl_slist *chunk = NULL;
        
        chunk = curl_slist_append(chunk, header_str.c_str());
        chunk = curl_slist_append(chunk, header_str1.c_str());
       
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
    //
    // Description: please briefly describe what went wrong
    // Steps you took before something went wrong (Optional):
    // Data you used (Optional): __________________________
    //
    // Create controls UI
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
   
    wxString desc_tip = _("[Please briefly describe what went wrong]");
    wxString steps_txt = _("[Steps you took before something went wrong]");
    
    wxTextCtrl* title_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, desc_tip);
    wxTextCtrl* steps_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, steps_txt,
                                                wxDefaultPosition,
                                                wxSize(500,200),
                                                wxTE_MULTILINE);
    
    wxString data_txt = _("Share part of your data for troubleshooting? (first 10 records)");
    wxCheckBox* data_chk = new wxCheckBox(panel, wxID_ANY, data_txt);
  
    wxString user_tip = _("Your Github account (Optional):");
    wxStaticText* lbl_user = new wxStaticText(panel, wxID_ANY, user_tip);
    wxTextCtrl* user_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
                                               wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer* user_box = new wxBoxSizer(wxHORIZONTAL);
    user_box->Add(lbl_user);
    user_box->Add(user_txt_ctrl);
    
    wxString email_tip = _("Your Email address (Optional):");
    wxStaticText* lbl_email = new wxStaticText(panel, wxID_ANY, email_tip);
    wxTextCtrl* email_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
                                                wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer* email_box = new wxBoxSizer(wxHORIZONTAL);
    email_box->Add(lbl_email);
    email_box->AddSpacer(10);
    email_box->Add(email_txt_ctrl);
    
    // buttons
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, "Cancel",
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    wxButton* btn_submit = new wxButton(panel, wxID_ANY, "Submit Bug Report",
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_submit, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(title_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(steps_txt_ctrl, 0, wxALIGN_TOP|wxEXPAND|wxLEFT|wxRIGHT | wxTOP, 10);
    box->Add(data_chk, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    box->Add(user_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(email_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(btn_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    
    panel->SetSizerAndFit(box);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    
    SetParent(parent);
    SetPosition(pos);
    Centre();
   
    
}

ReportBugDlg::~ReportBugDlg()
{
    
}

void ReportBugDlg::OnOkClick(wxCommandEvent& event)
{
    //wxString rst = CreateIssueOnGithub("{\"title\": \"Test reporting bug from GeoDa software\", \"body\": \"We should have one\"}");

    // parse results
    
    ReportResultDlg dlg(this, "https://github.com/GeoDaCenter/geoda/issues/511");
    dlg.ShowModal();
}

void ReportBugDlg::OnCancelClick(wxCommandEvent& event)
{
}
