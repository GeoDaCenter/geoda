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

#ifndef __GEODA_CENTER_CREATE_SP_TM_PROJECT_DLG_H__
#define __GEODA_CENTER_CREATE_SP_TM_PROJECT_DLG_H__

#include <vector>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include "../ShapeOperations/DbfFile.h"

class Project;
class DbfGridTableBase;

class CreateSpTmProjectDlg: public wxDialog 
{
public:
	CreateSpTmProjectDlg(wxWindow* parent, Project* project,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize );
    void CreateControls();
	
	void OnOkClick( wxCommandEvent& event );
	void OnFieldChoice( wxCommandEvent& event );
	void OnAddFieldClick( wxCommandEvent& event );
	void OnOutputDbfFileClick( wxCommandEvent& event );
	void OnTimeFieldNameTxtChange( wxCommandEvent& event );
	void OnIntegerTxtChange( wxCommandEvent& event );
	
	void OnSpaceIdVarHelp( wxCommandEvent& event );
	void OnSpaceDbfFileHelp( wxCommandEvent& event );
	void OnTimeDbfFileHelp( wxCommandEvent& event );
	void OnNewTimeIdVarHelp( wxCommandEvent& event );
	void OnTimePeriodsHelp( wxCommandEvent& event );

	static const int max_time_periods = 10000;
private:
	void InitFieldChoices();
	void UpdateOkButton();
	
	wxButton* add_new_field_btn;
	wxChoice* field_choice;
	wxStaticText* sp_dbf_file_static_txt;
	wxStaticText* tm_dbf_file_static_txt;
	wxTextCtrl* output_tm_dbf_txt;
	wxBitmapButton* output_time_dbf_btn;
	wxTextCtrl* time_field_name_txt;
	wxTextCtrl* integer_txt;
	wxStaticText* num_times_static_txt;
	
	bool all_init;
	int num_obs;
	std::vector<wxInt64> time_ids;
	bool time_ids_valid;
	Project* project;
	DbfGridTableBase* grid_base;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed in wxGrid
	std::vector<int> col_id_map;
	
	DECLARE_EVENT_TABLE()
};

#endif
