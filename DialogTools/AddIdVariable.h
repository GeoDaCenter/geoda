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

#ifndef __GEODA_CENTER_ADD_ID_VARIABLE_H__
#define __GEODA_CENTER_ADD_ID_VARIABLE_H__

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <vector>
#include "../DbfFile.h"

class TableInterface;

class AddIdVariable: public wxDialog {
public:
	AddIdVariable(TableInterface* table_int,
				  wxWindow* parent, wxWindowID id = -1,
				  const wxString& caption = _("Add New ID Variable"),
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
	
	void CreateControls();
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
	wxString GetIdVarName();
	
private:
	wxString new_id_var_name;
	wxTextCtrl *new_id_var;
	wxListBox *existing_vars_list;
	std::vector<DbfFieldDesc> fields;
	TableInterface* table_int;
    
	DECLARE_EVENT_TABLE();
};

#endif

