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
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>

#include "../Project.h"
#include "../GeoDaWebProxy.h"

#include "PublishDlg.h"


BEGIN_EVENT_TABLE( PublishDlg, wxDialog )
    EVT_BUTTON( wxID_OK, PublishDlg::OnOkClick )
END_EVENT_TABLE()

PublishDlg::PublishDlg(wxWindow* parent, Project* _p,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size )
{
    p = _p;
    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_GEODA_PUBLISH_DLG");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
	m_txt_uname = XRCCTRL(*this, "IDC_GEODA_USERNAME",wxTextCtrl);
	m_txt_key = XRCCTRL(*this, "IDC_GEODA_KEY",wxTextCtrl);
	m_txt_title = XRCCTRL(*this, "IDC_GEODA_PUBLISH_TITLE",wxTextCtrl);
	m_txt_description = XRCCTRL(*this, "IDC_GEODA_PUBLISH_DESCRIPTION",wxTextCtrl);
    
    m_txt_uname->SetValue("lixun910");
    m_txt_uname->Enable(false);
    
    m_txt_key->SetValue("asdjk23989234kasdlfj29");
    m_txt_key->Enable(false);
    
    SetParent(parent);
	SetPosition(pos);
	Centre();
}


void PublishDlg::OnOkClick( wxCommandEvent& event )
{
    wxString title(m_txt_title->GetValue());
    wxString description(m_txt_description->GetValue() );
    
    
    //GeoDaWebProxy geodaweb;
    //geodaweb.Publish(p, title, description);
    
	EndDialog(wxID_OK);
}
