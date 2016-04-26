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

#ifndef __GEODA_CENTER_EXPORT_CSV_DLG_H__
#define __GEODA_CENTER_EXPORT_CSV_DLG_H__

#include <vector>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include "../DbfFile.h"

class Project;
class TableInterface;

class ExportCsvDlg: public wxDialog 
{
public:
	ExportCsvDlg(wxWindow* parent, Project* project,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize );
    void CreateControls();
	
	void OnOkClick( wxCommandEvent& event );
	void OnIncludeVarNamesHelp( wxCommandEvent& event );

private:
	bool all_init;
	Project* project;
	TableInterface* table_int;
	
	wxCheckBox* include_var_names_cb;
	
	DECLARE_EVENT_TABLE()
};

#endif
