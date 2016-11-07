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
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>

#include "../Project.h"
#include "../ShapeOperations/OGRDataAdapter.h"

#include "BasemapConfDlg.h"


BEGIN_EVENT_TABLE( BasemapConfDlg, wxDialog )
EVT_BUTTON( wxID_OK, BasemapConfDlg::OnOkClick )
EVT_BUTTON( XRCID("ID_NOKIA_RESET"), BasemapConfDlg::OnResetClick )
END_EVENT_TABLE()

BasemapConfDlg::BasemapConfDlg(wxWindow* parent, Project* _p,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos,
                       const wxSize& size )
{
    
    wxLogMessage("Open BasemapConfDlg.");
    p = _p;
    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_BASEMAP_CONF_DLG");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
    m_txt_nokia_uname = XRCCTRL(*this, "IDC_NOKIA_USERNAME",wxTextCtrl);
    m_txt_nokia_key = XRCCTRL(*this, "IDC_NOKIA_KEY",wxTextCtrl);

    
    SetParent(parent);
    SetPosition(pos);
    Centre();
}


void BasemapConfDlg::OnOkClick( wxCommandEvent& event )
{
     wxLogMessage("BasemapConfDlg: Click OK Button.");
    
    std::string nokia_uname(m_txt_nokia_uname->GetValue().Trim().mb_str());
    std::string nokia_key(m_txt_nokia_key->GetValue().Trim().mb_str());
    
    if (!nokia_uname.empty() && !nokia_key.empty()) {
        OGRDataAdapter::GetInstance().AddEntry("nokia_user", nokia_uname.c_str());
        OGRDataAdapter::GetInstance().AddEntry("nokia_key", nokia_key.c_str());
    }
    
    EndDialog(wxID_OK);
}

void BasemapConfDlg::OnResetClick( wxCommandEvent& event )
{
    wxLogMessage("BasemapConfDlg: Click Reset Button.");
    
    std::string nokia_uname = "oRnRceLPyM8OFQQA5LYH";
    std::string nokia_key = "uEt3wtyghaTfPdDHdOsEGQ";
    
    OGRDataAdapter::GetInstance().AddEntry("nokia_user", nokia_uname.c_str());
    OGRDataAdapter::GetInstance().AddEntry("nokia_key", nokia_key.c_str());
    
    EndDialog(wxID_OK);
}
