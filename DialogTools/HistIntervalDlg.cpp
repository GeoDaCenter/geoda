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
#include <wx/valtext.h>
#include "HistIntervalDlg.h"

IMPLEMENT_CLASS( HistIntervalDlg, wxDialog )

BEGIN_EVENT_TABLE( HistIntervalDlg, wxDialog )
    EVT_BUTTON( wxID_OK, HistIntervalDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, HistIntervalDlg::OnCancelClick )
END_EVENT_TABLE()

HistIntervalDlg::HistIntervalDlg( int min_intervals_s,
								 int default_num_intervals_s,
								 int max_intervals_s,
								 wxWindow* parent,
								 wxWindowID id,
								 const wxString& caption,
								 const wxPoint& pos, const wxSize& size,
								 long style )
: min_intervals(min_intervals_s),
default_num_intervals(default_num_intervals_s),
max_intervals(max_intervals_s),
num_intervals(default_num_intervals_s)
{
	wxString t;
	t << default_num_intervals;
	s_int = t;

	SetParent(parent);
    CreateControls();
    Centre();
}

void HistIntervalDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_INTERVALS");
	m_intervals = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_INTERVAL")),
								wxTextCtrl);
	
    m_intervals->SetValidator( wxTextValidator(wxFILTER_NUMERIC, &s_int) );
}

void HistIntervalDlg::OnOkClick( wxCommandEvent& event )
{
	if (!m_intervals->GetValue().IsNumber()) {
		wxMessageBox("Please enter a valid positive integer");
		return;
	}
	long val;
	m_intervals->GetValue().ToLong(&val);
	if (val < min_intervals) val = min_intervals;
	if (val > max_intervals) val = max_intervals;
	num_intervals = val;
	
    event.Skip();
	EndDialog(wxID_OK);
}

void HistIntervalDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
	EndDialog(wxID_CANCEL);

}
