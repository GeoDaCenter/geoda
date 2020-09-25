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

#ifndef __GEODA_CENTER_SAVE_TO_TALBE_DLG_H__
#define __GEODA_CENTER_SAVE_TO_TALBE_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "../GdaConst.h"

class Project;
class TableInterface;

struct SaveToTableEntry
{
	SaveToTableEntry(): d_val(0), l_val(0), undefined(0) {}
	std::vector<double>* d_val;
	std::vector<wxInt64>* l_val;
	std::vector<bool>* undefined;
	wxString label;
	wxString field_default;
	// if type is double, then only show or allow double fields.  If type
	// is long64 then show both fields, and create a long64 field if a new
	// field is specified.
	GdaConst::FieldType type;
};

class SaveToTableDlg: public wxDialog
{    
public:
    SaveToTableDlg( Project* project, wxWindow* parent,
				   const std::vector<SaveToTableEntry>& data,
				   const wxString& title = _("Save Results"), 
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    void CreateControls();
	void OnCheck( wxCommandEvent& event );
	void OnAddFieldButton( wxCommandEvent& event );
	void OnFieldChoice( wxCommandEvent& event );
	void OnTimeChoice( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
	void OnCloseClick( wxCommandEvent& event );
    void SetCheck(size_t i, bool flag);
    
    std::vector<int> new_col_ids;
    std::vector<wxString> new_col_names;
    
private:
	void FillColIdMaps();
	void InitField(int button);
	void InitFields();
	void InitTime();
	void EnableTimeField(int button);
	void UpdateFieldTms(int button);
	void UpdateOkButton();
	
	bool all_init;
	bool is_space_time;
	wxButton* m_ok_button;
    wxStaticText* m_field_label;
    std::vector<wxTextCtrl*> m_txt_field;
	std::vector<wxCheckBox*> m_check;
	std::vector<wxButton*> m_add_button;
	std::vector<wxChoice*> m_field;
	std::vector<wxChoice*> m_time;
	const std::vector<SaveToTableEntry>& data;

	std::vector< std::vector<int> > col_id_maps;
	
	Project* project;
	TableInterface* table_int;
	
	DECLARE_EVENT_TABLE()
};

#endif
