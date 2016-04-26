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

#ifndef __GEODA_CENTER_DATA_VIEWER_ADD_COL_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_ADD_COL_DLG_H__

#include <map>
#include <set>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include "../GdaConst.h"

class Project;
class TableInterface;

class DataViewerAddColDlg: public wxDialog
{
public:
    DataViewerAddColDlg(Project* project,
						wxWindow* parent,
                        bool time_variant_no_as_default=true,
						bool can_change_time_variant=true,
						wxString default_name=wxEmptyString,
			GdaConst::FieldType default_field_type=GdaConst::double_type);
    void CreateControls();
	void OnChoiceType( wxCommandEvent& ev );
	void SetDefaultsByType(GdaConst::FieldType type);
    void OnOkClick( wxCommandEvent& ev );
	void OnEditName( wxCommandEvent& ev );
	void CheckName();
	void OnEditLength( wxCommandEvent& ev );
	void OnEditDecimals( wxCommandEvent& ev );
	void OnChoiceDisplayedDecimals( wxCommandEvent& ev );
	void UpdateMinMaxValues();
	void UpdateApplyButton();
	
	wxString GetColName();
	int GetColId();
	
	wxRadioButton* m_time_variant_no;
	wxRadioButton* m_time_variant_yes;
	wxButton* m_apply_button;
	wxTextCtrl* m_name;
	bool m_name_valid;
	wxChoice* m_type;
	wxChoice* m_insert_pos;
	wxStaticText* m_length_lable;
	wxTextCtrl* m_length;
	int m_length_val;
	bool m_length_valid;
	wxStaticText* m_decimals_lable;
	wxTextCtrl* m_decimals;
	int m_decimals_val;
	bool m_decimals_valid;
	wxStaticText* m_displayed_decimals_lable;
	wxChoice* m_displayed_decimals;
	wxStaticText* m_max_label;
	wxStaticText* m_max_val;
	wxStaticText* m_min_label;
	wxStaticText* m_min_val;
	GdaConst::FieldType cur_type;
	wxGrid* grid;
	Project* project;
	TableInterface* table_int;
private:
	wxString default_name;
	GdaConst::FieldType default_field_type;
	bool time_variant_no_as_default;
	bool can_change_time_variant;
	wxString final_col_name;
	int final_col_id;
	std::set<wxString> curr_col_labels;
	bool time_variant;
	bool fixed_lengths;
	DECLARE_EVENT_TABLE()
};

#endif
