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

#ifndef __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___
#define __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___

#include <vector>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include "../GenUtils.h"

class DbfGridTableBase;
class GalElement;
class Project;

class VariableSettingsDlg: public wxDialog
{
public:
	enum VarType {
		univariate, bivariate, trivariate, quadvariate, rate_smoothed
	};

	VariableSettingsDlg(Project* project, short smoother, GalElement* gal,
						const wxString& title="Rates Variable Settings",
						const wxString& var1_title="Event Variable",
						const wxString& var2_title="Base Variable",
						const wxString& var3_title="Third Variable",
						const wxString& var4_title="Fourth Variable");
	VariableSettingsDlg( Project* project, VarType v_type,
						bool fill_result_vectors,
						const wxString& title="Variable Settings",
						const wxString& var1_title="First Variable (X)",
						const wxString& var2_title="Second Variable (Y)",
						const wxString& var3_title="Third Variable (Z)",
						const wxString& var4_title="Fourth Variable");
	virtual ~VariableSettingsDlg();
    void CreateControls();
	void Init(VarType var_type);
	
    void OnListVariableDoubleClicked( wxCommandEvent& event );
	void OnVarChange( wxCommandEvent& event );
	void OnTime( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

	bool fill_result_vectors; // when true, only var_info and col_ids are filled
	bool fill_smoothed_results; // only true when first constructor is called
	double* smoothed_results; // for rate_smoothed
	std::vector<bool> m_undef_r; // for rate_smoothed
	int m_theme; // for rate_smoothed
	
	double*	v1_single_time;
	double* v2_single_time;
	double* v3_single_time;
	double* v4_single_time;
	wxString v1_name_with_time;
	wxString v2_name_with_time;
	wxString v3_name_with_time;
	wxString v4_name_with_time;
	
	wxString v1_name;
	wxString v2_name;
	wxString v3_name;
	wxString v4_name;
	int v1_time;
	int v2_time;
	int v3_time;
	int v4_time;
	int v1_col_id;
	int v2_col_id;
	int v3_col_id;
	int v4_col_id;
	
	std::vector<int> col_ids;
	std::vector<GeoDaVarInfo> var_info;
	
	int time_ref_var; // which var_info item has is_ref_variable == true, if any

protected:
	VarType v_type;
	wxListBox* lb1;
    wxListBox* lb2;
	wxListBox* lb3;
	wxListBox* lb4;
	int lb1_cur_sel;
	int lb2_cur_sel;
	int lb3_cur_sel;
	int lb4_cur_sel;
	wxChoice* time_lb1;
	wxChoice* time_lb2;
	wxChoice* time_lb3;
	wxChoice* time_lb4;
	
	wxString title;
	wxString var1_title;
	wxString var2_title;
	wxString var3_title;
	wxString var4_title;
	
	wxChoice* map_theme_lb; // for rate_smoothed
	short m_smoother; // for rate_smoothed
	
	bool all_init;
	
	int num_var; // 1, 2, 3, or 4
	bool is_time;
	int time_steps;

	long num_obs; // for rate_smoothed
	double*	E; // for rate_smoothed
	double* P; // for rate_smoothed
	GalElement* m_gal; // for rate_smoothed
	
	Project* project;
	DbfGridTableBase* grid_base;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed in wxGrid
	std::vector<int> col_id_map;

	void InitTimeChoices();
	void InitFieldChoices();
	void FillData();
	bool FillSmoothedResults();
	
	DECLARE_EVENT_TABLE()
};

#endif
