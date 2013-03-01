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

#ifndef __GEODA_CENTER_TIME_VARIANT_IMPORT_DLG_H__
#define __GEODA_CENTER_TIME_VARIANT_IMPORT_DLG_H__

#include <map>
#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include "../GeoDaConst.h"

class DbfGridTableBase;

class TimeVariantImportDlg: public wxDialog
{    
public:
    TimeVariantImportDlg( DbfGridTableBase* grid_base, wxWindow* parent,
				   const wxString& title = "Space-Time Variable Creation Tool", 
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    void CreateControls();
	void InitAllFieldsList();
	
    void OnOkClick( wxCommandEvent& event );
	void OnCloseClick( wxCommandEvent& event );
	
	void OnMoveUpClick( wxCommandEvent& event );
	void OnMoveDownClick( wxCommandEvent& event );
	void OnSortClick( wxCommandEvent& event );
	void OnReverseClick( wxCommandEvent& event );
	void OnAddToListClick( wxCommandEvent& event );
	void OnRemoveFrListClick( wxCommandEvent& event );
	void OnAllFieldsListSelection( wxCommandEvent& event );
	void OnIncludeListSelection( wxCommandEvent& event );
	void OnNewFieldNameChange( wxCommandEvent& event );
	void OnShowDetailsClick( wxCommandEvent& event );
	
	void OnCurVarsHelp( wxCommandEvent& event );
	void OnNewSpTmVarHelp( wxCommandEvent& event );
	void OnVarsRemainingHelp( wxCommandEvent& event );
	void OnCurSpTmVarsHelp( wxCommandEvent& event );
	
	void UpdateCreateButton();
	void UpdateButtons();
	void ResetAllButtons();
	void ResetCommonTypeLenDecs();
	void UpdateTimeStepsTxt(int num_list_items);
	
	bool IsCompatibleWithCommon(int col);
	
private:
	bool all_init;
	wxButton* ok_button;
	
	wxTextCtrl* new_field_name_txt_ctrl;
	wxStaticText* new_field_type_stat_txt;
	wxStaticText* new_field_length_stat_txt;
	wxStaticText* new_field_decimals_caption;
	wxStaticText* new_field_decimals_stat_txt;
	wxStaticText* include_list_stat_txt;
	
	wxButton* move_up_button;
	wxButton* move_down_button;
	wxButton* sort_button;
	wxButton* reverse_button;
	
	wxListBox* include_list;
	
	wxButton* add_to_list_button;
	wxButton* remove_fr_list_button;
	
	wxCheckBox* show_details_cb;
	
	bool show_details;
	wxListBox* all_fields_list;
	wxListBox* all_st_fields_list;
	
	std::map<wxString, wxString> all_fields_to_name;
	std::map<wxString, wxString> name_to_all_fields;
	
	DbfGridTableBase* grid_base;
	
	int time_steps;
	int common_empty;
	int common_length;
	int common_decimals;
	GeoDaConst::FieldType common_type;
	
	DECLARE_EVENT_TABLE()
};

#endif
