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

#ifndef __GEODA_CENTER_HIST_INTERVAL_DLG_H__
#define __GEODA_CENTER_HIST_INTERVAL_DLG_H__

#include <wx/dialog.h>
#include <wx/textctrl.h>

class HistIntervalDlg: public wxDialog
{    
    DECLARE_CLASS( HistIntervalDlg )
    DECLARE_EVENT_TABLE()

public:
    HistIntervalDlg( int min_intervals,
					int default_num_intervals,
					int max_intervals,
					wxWindow* parent, wxWindowID id = -1,
					const wxString& caption = "Intervals in the Histogram",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    wxTextCtrl* m_intervals;
	wxString s_int;
	int min_intervals;
	int default_num_intervals;
	int max_intervals;
	int num_intervals;
};

#endif
