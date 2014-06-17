/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_SAVE_SELECTION_DLG_H__
#define __GEODA_CENTER_SAVE_SELECTION_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>

class TableInterface;
class Project;

class SaveSelectionDlg: public wxDialog
{
public:
    SaveSelectionDlg(Project* Project, wxWindow* parent,
					 wxWindowID id = wxID_ANY,
					 const wxString& caption = "Save Selection",
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
	void OnAddField( wxCommandEvent& event );
	void OnSaveFieldChoice( wxCommandEvent& event );
	void OnSaveFieldChoiceTm( wxCommandEvent& event );
	void EnableTimeField();
	void OnSelCheckBox( wxCommandEvent& event );
	void OnUnselCheckBox( wxCommandEvent& event );
	void OnSelUnselTextChange( wxCommandEvent& event);
	void OnApplySaveClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );

private:
	void FillColIdMap();
	void InitField();
	void InitTime();
	
	wxChoice* m_save_field_choice;
	wxChoice* m_save_field_choice_tm;
	wxCheckBox* m_sel_check_box;	
	wxTextCtrl* m_sel_val_text;
	wxCheckBox* m_unsel_check_box;	
	wxTextCtrl* m_unsel_val_text;
	wxButton* m_apply_save_button;
	
	bool m_all_init;
	bool is_space_time;
	void CheckApplySaveSettings();
	
	// col_id_map[i] is a map from the i'th item in the fields drop-down
	// to the actual col_id_map.  Items in the fields dropdown are in the
	// order displayed in wxGrid
	TableInterface* table_int;
	Project* project;
	std::vector<int> col_id_map;
	
	DECLARE_EVENT_TABLE()
};

#endif
