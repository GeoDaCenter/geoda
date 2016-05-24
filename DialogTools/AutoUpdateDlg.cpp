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
#include <vector>
#include <queue>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>
#include <wx/tokenzr.h>
#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>

#include "stdio.h"
#include <iostream>
#include <sstream>
#include "curl/curl.h"


#include "../version.h"
#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../GdaException.h"
#include "../ShapeOperations/OGRDataAdapter.h"
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
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
    }
    return response;
}

bool DownloadUrl(const char* url, const char* filepath)
{
    FILE* fp;
    CURL* curl = curl_easy_init();
    CURLcode res;
    if (curl) {
        fp = fopen(filepath, "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
            
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);
            if ( res ) {
                // download error
                return false;
            }
            return true;
        }
    }
    return false;
}

// return Version or empty string
wxString AutoUpdate::CheckUpdate()
{
    bool isTestMode = false;
    std::vector<std::string> test_mode = OGRDataAdapter::GetInstance().GetHistory("test_mode");
    if (!test_mode.empty() && test_mode[0] == "yes") {
        isTestMode = true;
    }
    
    wxString checklist = GetCheckList();
    wxStringTokenizer tokenizer;
    
    tokenizer.SetString(checklist, "\r\n");
    if (!tokenizer.HasMoreTokens()) {
        tokenizer.SetString(checklist, "\n");
        if (!tokenizer.HasMoreTokens()) {
            return "";
        }
    }
    wxString version = tokenizer.GetNextToken();
    
    wxString version_regex = "^[0-9]\\.[0-9]\\.[0-9]+(\\.[0-9]+)?$";
    wxRegEx regex;
    regex.Compile(version_regex);
    if (!regex.Matches(version)) {
        return "";
    }
    
    // get current version numbers
    wxStringTokenizer ver_tokenizer(version, ".");
    int update_major = wxAtoi(ver_tokenizer.GetNextToken());
    int update_minor = wxAtoi(ver_tokenizer.GetNextToken());
    int update_build = wxAtoi(ver_tokenizer.GetNextToken());
   
    int update_subbuild = 0;
    
    if (ver_tokenizer.HasMoreTokens()) {
        update_subbuild = wxAtoi(ver_tokenizer.GetNextToken());
    }
    
    if (update_major < Gda::version_major) return "";
    if (update_major > Gda::version_major) return version;
    // update_major == version_major
    if (update_minor < Gda::version_minor) return "";
    if (update_minor > Gda::version_minor) return version;
    // update_minor == version_minor
    
    if (update_build > Gda::version_build && update_build % 2 == 0) {
        // released version
        return version;
    }
    
    // could be a testing version
    if (isTestMode) {
        if (update_build > Gda::version_build) { // e.g. 1.8.5 vs 1.8.4
            return version;
        }
        if (update_build == Gda::version_build && update_build %2 == 1) {
            // e.g. 1.8.5
            if ( update_subbuild > Gda::version_subbuild) {
                return version;
            }
        }
    }
    
    return "";
}

wxString AutoUpdate::GetVersion(wxString checklist)
{
    wxStringTokenizer tokenizer;
    
    tokenizer.SetString(checklist, "\r\n");
    if (!tokenizer.HasMoreTokens()) {
        tokenizer.SetString(checklist, "\n");
        if (!tokenizer.HasMoreTokens()) {
            return "";
        }
    }
    wxString version = tokenizer.GetNextToken();
    
    return version;
}

wxString AutoUpdate::GetUpdateUrl(wxString checklist)
{
    wxStringTokenizer tokenizer;
    
    tokenizer.SetString(checklist, "\r\n");
    if (!tokenizer.HasMoreTokens()) {
        tokenizer.SetString(checklist, "\n");
        if (!tokenizer.HasMoreTokens()) {
            return "";
        }
    }
    
    if (!tokenizer.HasMoreTokens()) return "";
    wxString version = tokenizer.GetNextToken();
    
    if (!tokenizer.HasMoreTokens()) return "";
    wxString updateUrl = tokenizer.GetNextToken();
    
    return updateUrl;
}

wxString AutoUpdate::GetCheckList()
{
    bool isTestMode = false;
    std::vector<std::string> test_mode = OGRDataAdapter::GetInstance().GetHistory("test_mode");
    if (!test_mode.empty() && test_mode[0] == "yes") {
        isTestMode = true;
    }
   
    wxString checklistUrl = "http://geodacenter.github.io/updates/checklist";
    
    if (isTestMode) {
        checklistUrl = "http://geodacenter.github.io/updates/test.checklist";
    }
    
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
                             bool showSkip,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size )
: wxDialog(parent, id, title, pos, size)
{
    
    LOG_MSG("Entering AutoUpdateDlg::AutoUpdateDlg(..)");
   
    // check update, suppose CheckUpdate() return true
    checklist = AutoUpdate::GetCheckList();
    version = AutoUpdate::GetVersion(checklist);
    wxString url_update_description = AutoUpdate::GetUpdateUrl(checklist);
    
    wxString update_text;
	update_text << "A newer version of GeoDa is found. Do you want to update to version ";
    update_text << version;
	update_text << "?";
    
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, update_text);
    wxHyperlinkCtrl* whatsnew = new wxHyperlinkCtrl(panel, wxID_ANY, "Check what's new in this update.", url_update_description);
    prg_bar = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize);
    
    
    
    wxBoxSizer* lbl_box = new wxBoxSizer(wxVERTICAL);
    lbl_box->AddSpacer(20);
    lbl_box->Add(lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    lbl_box->Add(whatsnew, 1, wxALIGN_LEFT | wxEXPAND |wxALL, 10);
    lbl_box->Add(prg_bar, 1, wxEXPAND |wxALL, 10);
    
    wxButton* btn_skip = NULL;
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, "Cancel", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    if (showSkip)
        btn_skip = new wxButton(panel, wxID_ANY, "Skip");
    wxButton* btn_update= new wxButton(panel, wxID_ANY, "Update", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    if (showSkip) {
        btn_box->Add(btn_skip, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
        btn_skip->Connect(wxEVT_BUTTON, wxCommandEventHandler(AutoUpdateDlg::OnSkipClick), NULL, this);
    }
    btn_box->Add(btn_update, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
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
    
    prg_bar->Hide();
    
    btn_update->Connect(wxEVT_BUTTON, wxCommandEventHandler(AutoUpdateDlg::OnOkClick), NULL, this);
    btn_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(AutoUpdateDlg::OnCancelClick), NULL, this);
                        
    LOG_MSG("Exiting AutoUpdateDlg::AutoUpdateDlg(..)");
}


void AutoUpdateDlg::OnOkClick( wxCommandEvent& event )
{
    bool success = false;
    
    try {
        // read the file line by line
        std::queue<wxString> lines;
        
        wxStringTokenizer tokenizer;
        
        tokenizer.SetString(checklist, "\r\n");
        if (!tokenizer.HasMoreTokens()) {
            tokenizer.SetString(checklist, "\n");
            if (!tokenizer.HasMoreTokens()) {
                throw GdaException("");
            }
        }
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            lines.push(token);
        }
       
        int n = (int)lines.size();
        int jobs = (n-2) / 3 + 1; // skip first and second lines
        wxProgressDialog progressDlg("", "Downloading updates...",
                                     jobs, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE);
        progressDlg.Update(1);
        if (n > 2 && (n-2) % 3 == 0) {
            lines.pop(); // version
            lines.pop(); // description page
       
            wxString exePath = wxStandardPaths::Get().GetExecutablePath();
            wxFileName exeFile(exePath);
            wxString exeDir = exeFile.GetPathWithSep();
            
            int current_job = 2;
            while (lines.size() >= 3) {
                // download the file, check the file
                wxString file_name = lines.front();
                lines.pop();
                wxString file_url = lines.front();
                lines.pop();
                wxString file_size = lines.front();
                lines.pop();
                
                file_name = exeDir + file_name;
                wxString update_file_name = file_name + ".update";
                wxString backup_file_name = file_name + ".backup";
                int size = wxAtoi(file_size);
                
				wxRemoveFile(backup_file_name);
                wxRemoveFile(update_file_name);
                
                if (DownloadUrl(file_url.mb_str(), update_file_name.mb_str())){
                    // check file size
                    wxFileName updateFile(update_file_name);
                    wxULongLong update_size = updateFile.GetSize();
                    
                    if (update_size != size )
                        throw GdaException("");
                    
                    // replace the old file
                    wxRenameFile(file_name, backup_file_name);
                    wxRenameFile(update_file_name, file_name);
                    
					wxRemoveFile(backup_file_name);
					wxRemoveFile(update_file_name);

                    success = true;
                }
                progressDlg.Update(current_job++);
            }
        }
    } catch(...) {
        // raise warning message
        success = false;
    }
   
    if (success) {
        wxMessageDialog msgDlg(this,
                               "Please restart GeoDa to finish installing updates.",
                               "Update GeoDa completed",
                               wxOK |wxICON_INFORMATION);
        msgDlg.ShowModal();
        EndDialog(wxID_OK);
    } else {
        // raise warning message
        wxMessageDialog msgDlg(this,
                               "Please check your network connection, or contact GeoDa support team.",
                               "Update GeoDa failed",
                               wxOK |wxICON_ERROR);
        msgDlg.ShowModal();
    }
}

void AutoUpdateDlg::OnCancelClick( wxCommandEvent& event )
{
    EndDialog(wxID_CANCEL);
}

void AutoUpdateDlg::OnSkipClick( wxCommandEvent& event )
{
    EndDialog(wxID_NO);
}
wxString AutoUpdateDlg::GetVersion()
{
    return version;
}
