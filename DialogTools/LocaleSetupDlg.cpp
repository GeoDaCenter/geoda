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
#include <wx/dir.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>

#include <cpl_error.h>
#include <cpl_conv.h>

#include "../GenUtils.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "LocaleSetupDlg.h"


BEGIN_EVENT_TABLE( LocaleSetupDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_RESET_SYS_LOCALE"), LocaleSetupDlg::OnResetSysLocale )
    EVT_BUTTON( wxID_OK, LocaleSetupDlg::OnOkClick )
END_EVENT_TABLE()

LocaleSetupDlg::LocaleSetupDlg(wxWindow* parent,
                               bool need_reopen_flag,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size )
{
    wxLogMessage("Open LocaleSetupDlg.");
    
    need_reopen = need_reopen_flag;
    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_LOCALE_SETUP_DLG");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
	m_txt_thousands = XRCCTRL(*this, "IDC_FIELD_THOUSANDS",wxTextCtrl);
	m_txt_decimal = XRCCTRL(*this, "IDC_FIELD_DECIMAL",wxTextCtrl);
    m_txt_thousands->SetMaxLength(1);
    m_txt_decimal->SetMaxLength(1);

    const char* thousands_sep = CPLGetConfigOption("GEODA_LOCALE_SEPARATOR", ",");
    const char* decimal_sep = CPLGetConfigOption("GEODA_LOCALE_DECIMAL", ".");

    //struct lconv *poLconv = localeconv();
    //wxString thousands_sep = poLconv->thousands_sep;
    //wxString decimal_point = poLconv->decimal_point;
    
    m_txt_thousands->SetValue(thousands_sep);
    m_txt_decimal->SetValue(decimal_sep);
    
    SetParent(parent);
	SetPosition(pos);
	Centre();
}

void LocaleSetupDlg::OnResetSysLocale( wxCommandEvent& event )
{
    wxLogMessage("Click LocaleSetupDlg::OnResetSysLocale");
    
    struct lconv *poLconv = localeconv();    
    wxString thousands_sep = poLconv->thousands_sep;
    wxString decimal_point = poLconv->decimal_point;
    
    m_txt_thousands->SetValue(thousands_sep);
    m_txt_decimal->SetValue(decimal_point);
    
    wxString msg = _("Reset to system locale successfully.");
    wxMessageDialog msg_dlg(this, msg,
                           _("Reset to system locale information"),
                           wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
    msg_dlg.ShowModal();
    EndDialog(wxID_OK);
}

void LocaleSetupDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("Click LocaleSetupDlg::OnOkClick");
    
    wxString thousands_sep = m_txt_thousands->GetValue();
    wxString decimal_point = m_txt_decimal->GetValue();
    
    CPLSetConfigOption("GEODA_LOCALE_SEPARATOR",
                       (const char*)thousands_sep.mb_str());
    if ( !decimal_point.IsEmpty() )
        CPLSetConfigOption("GEODA_LOCALE_DECIMAL",
                           (const char*)decimal_point.mb_str());
   
    if (need_reopen) {
        wxString msg = _("Locale for numbers has been setup successfully.");
        wxMessageDialog msg_dlg(this, msg,
                               "Setup locale",
                               wxOK | wxOK_DEFAULT | wxICON_INFORMATION);
        msg_dlg.ShowModal();
    }
	EndDialog(wxID_OK);
}
