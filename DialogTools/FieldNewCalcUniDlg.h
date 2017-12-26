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

#ifndef __GEODA_CENTER_FIELD_NEW_CALC_UNI_DLG_H__
#define __GEODA_CENTER_FIELD_NEW_CALC_UNI_DLG_H__

#include <vector>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

class Project;
class TableInterface;
class FieldNewCalcSpecialDlg;
class FieldNewCalcBinDlg;
class FieldNewCalcLagDlg;
class FieldNewCalcRateDlg;

class FieldNewCalcUniDlg: public wxPanel
{    
    DECLARE_EVENT_TABLE()

public:
    FieldNewCalcUniDlg(Project* project,
					   wxWindow* parent,
					   wxWindowID id = wxID_ANY, 
					   const wxString& caption = "Univariate",
					   const wxPoint& pos = wxDefaultPosition, 
					   const wxSize& size = wxDefaultSize,
					   long style = wxTAB_TRAVERSAL );

    void CreateControls();
	
	void OnAddColumnClick( wxCommandEvent& event );
    void OnUnaryResultUpdated( wxCommandEvent& event );
	void OnUnaryResultTmUpdated( wxCommandEvent& event );
	void OnUnaryOperatorUpdated( wxCommandEvent& event );
    void OnUnaryOperandUpdated( wxCommandEvent& event );
	void OnUnaryOperandTmUpdated( wxCommandEvent& event );

	void UpdateOtherPanels();
	void SetOtherPanelPointers(FieldNewCalcSpecialDlg* s_panel_s,
							   FieldNewCalcBinDlg* b_panel_s,
							   FieldNewCalcLagDlg* l_panel_s,
							   FieldNewCalcRateDlg* r_panel_s) 
	{
		s_panel=s_panel_s; b_panel=b_panel_s;
		l_panel = l_panel_s; r_panel=r_panel_s; 
	}
	FieldNewCalcSpecialDlg* s_panel;
	FieldNewCalcBinDlg* b_panel;
	FieldNewCalcLagDlg* l_panel;
	FieldNewCalcRateDlg* r_panel;

	bool IsTimeVariant(int col_id);
	bool IsAllTime(int col_id, int tm_sel);
	
	bool all_init;
	bool is_space_time;
	std::vector<wxString> m_var_str;
    wxChoice* m_result;
	wxChoice* m_result_tm;
    wxChoice* m_op;
    wxComboBox* m_var;
	wxChoice* m_var_tm;
	double m_const;
	bool m_valid_const;
	int m_var_sel;
    wxTextCtrl* m_text;
	Project* project;
	TableInterface* table_int;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed
	// in wxGrid
	std::vector<int> col_id_map;
	
	void Apply();
	void InitFieldChoices();
	void InitTime(wxChoice* time_list);
	void Display();
	
	enum OpType {
		assign_op = 0,
		negate_op = 1,
		invert_op = 2,
		sqrt_op = 3,
		log_10_op = 4,
		log_e_op = 5,
		dev_from_mean_op = 6,
		standardize_op = 7,
		shuffle_op = 8,
        mad_op = 9
	};	
	std::vector<wxString> op_string;
};

#endif
