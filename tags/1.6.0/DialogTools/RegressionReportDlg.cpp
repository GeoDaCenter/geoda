/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include <wx/fontdlg.h>
#include "../GeneralWxUtils.h"
#include "RegressionReportDlg.h"

IMPLEMENT_DYNAMIC_CLASS( RegressionReportDlg, wxDialog )

BEGIN_EVENT_TABLE( RegressionReportDlg, wxDialog )
    EVT_CLOSE( RegressionReportDlg::OnClose )
	EVT_MOUSE_EVENTS(RegressionReportDlg::OnMouseEvent)
	EVT_MENU(XRCID("ID_FONT"), RegressionReportDlg::OnFontChanged)
END_EVENT_TABLE()

RegressionReportDlg::RegressionReportDlg( )
{
}

RegressionReportDlg::RegressionReportDlg( wxWindow* parent,
										   wxString showText,
										   wxWindowID id,
										   const wxString& caption,
										   const wxPoint& pos,
										   const wxSize& size, long style )
{
	results = showText;
    Create(parent, id, caption, pos, size, style);
}

bool RegressionReportDlg::Create( wxWindow* parent, wxWindowID id,
								  const wxString& caption,
								  const wxPoint& pos,
								  const wxSize& size, long style )
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    SetParent(parent);
    CreateControls();
    Centre();
	m_textbox->AppendText(results);
    return true;
}

void RegressionReportDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_REGRESSION_REPORT");
    m_textbox = XRCCTRL(*this, "ID_TEXTCTRL1", wxTextCtrl);
	wxTextAttr style(m_textbox->GetDefaultStyle());
	style.SetFontFamily(wxFONTFAMILY_TELETYPE);
	if (GeneralWxUtils::isMac()) {
		style.SetFontSize(14);
	} else {
		style.SetFontSize(10);
	}
	if (GeneralWxUtils::isUnix()) {
		style.SetFontEncoding(wxFONTENCODING_UTF8);
		style.SetFontFaceName("Monospace");
	}
	m_textbox->SetDefaultStyle(style);
}

void RegressionReportDlg::OnMouseEvent(wxMouseEvent& event)
{
	if (event.RightUp())
		PopupMenu(wxXmlResource::Get()->
			LoadMenu("ID_REPORT_VIEW_MENU_CONTEXT"),
			event.GetPosition().x, event.GetPosition().y);
}

void RegressionReportDlg::OnFontChanged(wxCommandEvent& event)
{
	wxFontData data;
	wxFontDialog dlg(this, data);
	wxTextAttr attr;

	if (dlg.ShowModal() == wxID_OK)
	{
		data = dlg.GetFontData();
		m_textbox->GetStyle(0, attr);
		attr.SetFont(data.GetChosenFont());
		m_textbox->SetStyle(0, results.Length(), attr);
	}
}

void RegressionReportDlg::OnClose( wxCloseEvent& event )
{
    Destroy();
}
