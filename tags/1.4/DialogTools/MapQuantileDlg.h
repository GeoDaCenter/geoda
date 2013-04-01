/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_MAP_QUANTILE_DLG_H__
#define __GEODA_CENTER_MAP_QUANTILE_DLG_H__

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

class MapQuantileDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    MapQuantileDlg( wxWindow* parent,
				   int min_classes_s,
				   int max_classes_s,
				   int default_classes_s,
				   const wxString& title,
				   const wxString& text = "Number of Classes");
	
    void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );

	int classes;
	int min_classes;
	int max_classes;
	int default_classes;
	wxStaticText* stat_text;
    wxSpinCtrl* m_classes;
};

#endif

