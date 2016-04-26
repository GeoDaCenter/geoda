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

#ifndef __GEODA_CENTER_FIELD_NEW_CALC_SPECIAL_DLG_H__
#define __GEODA_CENTER_FIELD_NEW_CALC_SPECIAL_DLG_H__

#include <vector>
#include <wx/string.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

class Project;
class TableInterface;
class FieldNewCalcUniDlg;
class FieldNewCalcBinDlg;
class FieldNewCalcLagDlg;
class FieldNewCalcRateDlg;

class FieldNewCalcSpecialDlg: public wxPanel
{    
    DECLARE_EVENT_TABLE()

public:
    FieldNewCalcSpecialDlg(Project* project,
					   wxWindow* parent,
					   wxWindowID id = wxID_ANY,
					   const wxString& caption = "Special",
					   const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize,
					   long style = wxTAB_TRAVERSAL );

    void CreateControls();

	void OnAddColumnClick( wxCommandEvent& event );
    void OnSpecialResultUpdated( wxCommandEvent& event );
    void OnSpecialResultTmUpdated( wxCommandEvent& event );
    void OnSpecialOperand1Updated( wxCommandEvent& event );
    void OnSpecialOperatorUpdated( wxCommandEvent& event );
    void OnSpecialOperand2Updated( wxCommandEvent& event );
	
	void UpdateOtherPanels();
	void SetOtherPanelPointers(FieldNewCalcUniDlg* u_panel_s,
							   FieldNewCalcBinDlg* b_panel_s,
							   FieldNewCalcLagDlg* l_panel_s,
							   FieldNewCalcRateDlg* r_panel_s) {
		u_panel=u_panel_s; b_panel=b_panel_s; 
		l_panel=l_panel_s; r_panel=r_panel_s; }
	FieldNewCalcUniDlg* u_panel;
	FieldNewCalcBinDlg* b_panel;
	FieldNewCalcLagDlg* l_panel;
	FieldNewCalcRateDlg* r_panel;

	bool IsTimeVariant(int col_id);
	bool IsAllTime(int col_id, int tm_sel);
	
	bool all_init;
	bool is_space_time;
    wxChoice* m_result;
	wxChoice* m_result_tm;
	wxChoice* m_op;
	wxStaticText* m_var1_label;
    wxTextCtrl* m_var1;
	bool m_var1_valid; // is m_var1 a valid constant
	double m_var1_const; // actual value of m_var1
	wxStaticText* m_var2_label;
    wxTextCtrl* m_var2;
	bool m_var2_valid; // is m_var1 a valid constant
	double m_var2_const; // actual value of m_var1
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
		normal_rand = 0,
		uniform_rand = 1,
		enumerate = 2
	};	
	std::vector<wxString> op_string;
};

#endif
