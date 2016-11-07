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

#include "ProgressDlg.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/xrc/xmlres.h>

BEGIN_EVENT_TABLE( ProgressDlg, wxDialog )
	EVT_BUTTON( XRCID("wxID_OK"), ProgressDlg::OnOkClick)
END_EVENT_TABLE()

const int gauge_res = 200;

ProgressDlg::ProgressDlg(wxWindow* parent, wxWindowID id,
						 const wxString& title, const wxPoint& pos,
						 const wxSize& size, long style )
: wxDialog(parent, id, title, pos, size, style),
	message(wxEmptyString), scaled_val(0)
{
	CenterOnScreen();
	SetSize(wxDefaultCoord, wxDefaultCoord, 300, 120);
	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(topSizer);
	wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
	topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	
	gauge = new wxGauge(this, wxID_ANY, gauge_res, wxDefaultPosition,
						wxSize(200, 20));
	
	boxSizer->Add(gauge, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	
	static_text1 = new wxStaticText( this, wxID_STATIC,
									"Progress...",
									wxDefaultPosition, wxDefaultSize, 0);
	boxSizer->Add(static_text1, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	
	//ok_button = new wxButton( this, wxID_OK, "OK" );
	
	// boxSizer->Add(ok_button, 1, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
}

void ProgressDlg::StatusUpdate(double val,
							   const wxString& msg )
{
	MessageUpdate(msg);
	ValueUpdate(val);
}

void ProgressDlg::MessageUpdate( const wxString& msg )
{
	if (message == msg) return;
	message = msg;
	static_text1->SetLabelText(message);
	Update();
}

void ProgressDlg::ValueUpdate( double val )
{
	if ( val < 0 ) val = 0;
	if ( val > 1 ) val = 1;
	double val_x = val * gauge_res;
	if (val_x == scaled_val) return;
	scaled_val = val_x;
	gauge->SetValue( scaled_val );
	Update();
}

void ProgressDlg::OnOkClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_OK);
}

