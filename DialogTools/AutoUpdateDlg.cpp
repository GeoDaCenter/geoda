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
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>

#include "stdio.h"
#include <iostream>
#include <sstream>
#include "curl/curl.h"


#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "AutoUpdateDlg.h"

using namespace std;

size_t write_to_string(void *ptr, size_t size, size_t count, void *stream) {
    ((string*)stream)->append((char*)ptr, 0, size*count);
    return size*count;
}

size_t write_to_file(void *ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* stream = (FILE*)userdata;
    if (!stream)
    {
        printf("!!! No stream\n");
        return 0;
    }
    
    size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
    return written;
}

string ReadUrlContent(const char* url)
{
    
    string response;
    
    CURL* curl = curl_easy_init();
    CURLcode res;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
    }
    return response;
}

wxString AutoUpdate::CheckUpdate()
{
    wxString checklist = GetCheckList();
    
    // get current version numbers
    
    return "";
}

wxString AutoUpdate::GetCheckList()
{
    wxString checklistUrl = "http://geodacenter.github.io/updates/checklist";
    // download checklist.txt
    if ( GeneralWxUtils::isWindows()) {
        if (GeneralWxUtils::isX86()) {
            checklistUrl += ".win32.txt";
        } else {
            checklistUrl += ".win64.txt";
        }
    } else if (GeneralWxUtils::isMac()) {
        checklistUrl += ".macosx.txt";
    } else {
        // we don't support auto update on other platforms
        return "";
    }
    
    return ReadUrlContent(checklistUrl);
}

AutoUpdateDlg::AutoUpdateDlg(wxWindow* parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size )
: wxDialog(parent, id, title, pos, size)
{
    
    LOG_MSG("Entering AutoUpdateDlg::AutoUpdateDlg(..)");
   
    // check update
    wxString checklist = AutoUpdate::GetCheckList();
    
    wxString url_update_description;
   
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, "A newer version of GeoDa is found. Do you want to update to version 1.8.4?");
    wxHyperlinkCtrl* whatsnew = new wxHyperlinkCtrl(panel, wxID_ANY, "Check what's new in this update.", url_update_description);
    prg_bar = new wxGauge(panel, wxID_ANY, 100);
    
    
    prg_bar->Hide();
    
    wxBoxSizer* lbl_box = new wxBoxSizer(wxVERTICAL);
    lbl_box->AddSpacer(20);
    lbl_box->Add(lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    lbl_box->Add(whatsnew, 1, wxALIGN_LEFT | wxEXPAND |wxALL, 10);
    lbl_box->Add(prg_bar, 1, wxEXPAND |wxALL, 10);
    
    
    wxButton* btn_save_dummy = new wxButton(panel, wxID_ANY, "Cancel");
    wxButton* btn_apply = new wxButton(panel, wxID_ANY, "Update");
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_save_dummy, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_apply, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(lbl_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(btn_box, 0, wxALIGN_CENTER| wxLEFT | wxRIGHT | wxTOP, 30);
    
    panel->SetSizerAndFit(box);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    
    SetParent(parent);
    SetPosition(pos);
    Centre();
    LOG_MSG("Exiting AutoUpdateDlg::AutoUpdateDlg(..)");
}


void AutoUpdateDlg::OnOkClick( wxCommandEvent& event )
{
    // read the file line by line
    // download the file
    // replace the old file
    
    EndDialog(wxID_OK);
}

void AutoUpdateDlg::OnCancelClick( wxCommandEvent& event )
{

    
    EndDialog(wxID_CANCEL);
}