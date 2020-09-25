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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "LisaWhat2OpenDlg.h"

IMPLEMENT_CLASS( LisaWhat2OpenDlg, wxDialog )

BEGIN_EVENT_TABLE( LisaWhat2OpenDlg, wxDialog )
    EVT_BUTTON( wxID_OK, LisaWhat2OpenDlg::OnOkClick )
END_EVENT_TABLE()

LisaWhat2OpenDlg::LisaWhat2OpenDlg( wxWindow* parent, wxWindowID id,
								   const wxString& caption, const wxPoint& pos,
								   const wxSize& size, long style )
{
    m_RowStand = true;
	SetParent(parent);
    CreateControls();
    Centre();
}

void LisaWhat2OpenDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_LISAWINDOWS2OPEN");
    if (FindWindow(wxXmlResource::GetXRCID("IDC_CHECK1")))
        m_check1 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK1")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK2")))
        m_check2 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK2")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK3")))
        m_check3 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK3")), wxCheckBox);
    //if (FindWindow(XRCID("IDC_CHECK4")))
    //    m_check4 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK4")), wxCheckBox);
}

void LisaWhat2OpenDlg::HideMoranScatter()
{
    m_check3->Hide();
}
void LisaWhat2OpenDlg::OnOkClick( wxCommandEvent& event )
{
	m_SigMap = m_check1->GetValue();
	m_ClustMap = m_check2->GetValue();
	m_Moran = m_check3->GetValue();
    //m_RowStand = m_check4->GetValue();
    
	event.Skip();
	EndDialog(wxID_OK);	
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( GetisWhat2OpenDlg, wxDialog )

BEGIN_EVENT_TABLE( GetisWhat2OpenDlg, wxDialog )
EVT_BUTTON( wxID_OK, GetisWhat2OpenDlg::OnOkClick )
END_EVENT_TABLE()

GetisWhat2OpenDlg::GetisWhat2OpenDlg( wxWindow* parent,
                                     bool _show_row_stand, wxWindowID id,
                                     const wxString& caption, const wxPoint& pos,
                                     const wxSize& size, long style )
{
    show_row_stand = _show_row_stand;
    SetParent(parent);
    CreateControls();
    Centre();
}

void GetisWhat2OpenDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_GETISWINDOWS2OPEN");
    if (FindWindow(wxXmlResource::GetXRCID("IDC_CHECK1")))
        m_check1 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK1")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK2")))
        m_check2 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK2")), wxCheckBox);

    if (FindWindow(XRCID("IDC_CHECK3")))
        m_check3 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK3")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK4")))
        m_check4 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK4")), wxCheckBox);
    m_check3->Hide();
    if (!show_row_stand) {
        m_check4->SetValue(false);
        m_check4->Hide();
    }
}

void GetisWhat2OpenDlg::OnOkClick( wxCommandEvent& event )
{
    m_SigMap = m_check1->GetValue();
    m_ClustMap = m_check2->GetValue();
    m_NormMap = m_check3->GetValue();
    m_RowStand= m_check4->GetValue();

    
    event.Skip();
    EndDialog(wxID_OK);	
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS( LocalGearyWhat2OpenDlg, wxDialog )

BEGIN_EVENT_TABLE( LocalGearyWhat2OpenDlg, wxDialog )
EVT_BUTTON( wxID_OK, LocalGearyWhat2OpenDlg::OnOkClick )
END_EVENT_TABLE()

LocalGearyWhat2OpenDlg::LocalGearyWhat2OpenDlg( wxWindow* parent, wxWindowID id,
                                   const wxString& caption, const wxPoint& pos,
                                   const wxSize& size, long style )
{
    m_RowStand = true;
    SetParent(parent);
    CreateControls();
    Centre();
}

void LocalGearyWhat2OpenDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_LISAWINDOWS2OPEN");
    if (FindWindow(wxXmlResource::GetXRCID("IDC_CHECK1")))
        m_check1 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK1")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK2")))
        m_check2 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK2")), wxCheckBox);
    if (FindWindow(XRCID("IDC_CHECK3"))) {
        m_check3 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK3")), wxCheckBox);
        m_check3->Hide();
    }
    //if (FindWindow(XRCID("IDC_CHECK4")))
    //    m_check4 = wxDynamicCast(FindWindow(XRCID("IDC_CHECK4")), wxCheckBox);
}

void LocalGearyWhat2OpenDlg::OnOkClick( wxCommandEvent& event )
{
    m_SigMap = m_check1->GetValue();
    m_ClustMap = m_check2->GetValue();
    //m_Moran = m_check3->GetValue();
    //m_RowStand = m_check4->GetValue();
    
    event.Skip();
    EndDialog(wxID_OK);	
}

