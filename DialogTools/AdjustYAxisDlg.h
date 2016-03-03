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

#ifndef __GEODA_CENTER_ADJUST_Y_AXIS_DLG_H__
#define __GEODA_CENTER_ADJUST_Y_AXIS_DLG_H__

#include <wx/dialog.h>
#include <wx/textctrl.h>

class AdjustYAxisDlg: public wxDialog
{    
    DECLARE_CLASS( AdjustYAxisDlg )
    DECLARE_EVENT_TABLE()

public:
    AdjustYAxisDlg( wxString min_val,
					wxString max_val,
					wxWindow* parent, wxWindowID id = -1,
					const wxString& caption = "Adjust Values of Y Axis",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    wxTextCtrl* m_min_val;
    wxTextCtrl* m_max_val;
    
	double min_val;
	double max_val;
    
	double o_min_val;
	double o_max_val;
    
    wxString s_min_val;
    wxString s_max_val;
};

#endif
