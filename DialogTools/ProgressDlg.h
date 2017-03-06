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

#ifndef __GEODA_CENTER_PROGRESS_DLG_H__
#define __GEODA_CENTER_PROGRESS_DLG_H__

#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/stattext.h>

class ProgressDlg: public wxDialog
{
public:
	ProgressDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
				const wxString& title = _("Progress"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION | wxDEFAULT_DIALOG_STYLE );
	void StatusUpdate( double val, const wxString& msg );
	void MessageUpdate( const wxString& msg );
	void ValueUpdate( double val );
	void OnOkClick( wxCommandEvent& event );
	wxStaticText* static_text1;
	wxGauge* gauge;
	wxButton* ok_button;

private:
	wxString message;
	double scaled_val;
	
	DECLARE_EVENT_TABLE()
};

#endif


