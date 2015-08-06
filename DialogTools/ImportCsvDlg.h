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

#ifndef __GEODA_CENTER_IMPORT_CSV_DLG_H__
#define __GEODA_CENTER_IMPORT_CSV_DLG_H__

#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>

class ImportCsvDlg: public wxDialog
{
public:
	ImportCsvDlg(wxWindow* parent,
				 std::vector<std::string>& first_row,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize );
    void CreateControls();
	
	void OnOkClick( wxCommandEvent& event );
	void OnFirstRowCsvHelp( wxCommandEvent& event );
	void OnIncludeVarNamesHelp( wxCommandEvent& event );
	
	bool contains_var_names;
	
private:
	void Init();
	
	bool all_init;
	
	std::vector<std::string> first_row;
	wxTextCtrl* first_row_csv_txt;
	wxCheckBox* include_var_names_cb;
	
	DECLARE_EVENT_TABLE()
};

#endif
