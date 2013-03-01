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

#ifndef __GEODA_CENTER_FIELD_NEW_CALC_RATE_DLG_H__
#define __GEODA_CENTER_FIELD_NEW_CALC_RATE_DLG_H__

#include <vector>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/panel.h>

class Project;
class DbfGridTableBase;
class WeightsManager;
class FieldNewCalcSpecialDlg;
class FieldNewCalcUniDlg;
class FieldNewCalcBinDlg;
class FieldNewCalcLagDlg;

class FieldNewCalcRateDlg: public wxPanel
{
	DECLARE_EVENT_TABLE();

public:
    FieldNewCalcRateDlg(Project* project,
						wxWindow* parent,
						wxWindowID id = wxID_ANY, 
						const wxString& caption = "Rates",
						const wxPoint& pos = wxDefaultPosition, 
						const wxSize& size = wxDefaultSize, 
						long style = wxTAB_TRAVERSAL );
	
    void CreateControls();

	void OnAddColumnClick( wxCommandEvent& event );
	void OnRateResultUpdated( wxCommandEvent& event );
    void OnRateResultTmUpdated( wxCommandEvent& event );
	void OnRateOperand1Updated( wxCommandEvent& event );
	void OnRateOperand1TmUpdated( wxCommandEvent& event );
    void OnRateOperand2Updated( wxCommandEvent& event );
	void OnRateOperand2TmUpdated( wxCommandEvent& event );
	void OnOpenWeightClick( wxCommandEvent& event );
	void OnMethodChange( wxCommandEvent& event );
	
	void UpdateOtherPanels();
	void SetOtherPanelPointers(FieldNewCalcSpecialDlg* s_panel_s,
							   FieldNewCalcUniDlg* u_panel_s,
							   FieldNewCalcBinDlg* b_panel_s,
							   FieldNewCalcLagDlg* l_panel_s) {
		s_panel=s_panel_s; u_panel=u_panel_s;
		b_panel=b_panel_s; l_panel = l_panel_s; }
	FieldNewCalcSpecialDlg* s_panel;
	FieldNewCalcUniDlg* u_panel;
	FieldNewCalcBinDlg* b_panel;
	FieldNewCalcLagDlg* l_panel;
	
	bool IsTimeVariant(int col_id);
	bool IsAllTime(int col_id, int tm_sel);
	
	bool all_init;
	bool is_space_time;
    wxChoice* m_result;
	wxChoice* m_result_tm;
    wxChoice* m_event;
	wxChoice* m_event_tm;
    wxChoice* m_method;
    wxChoice* m_base;
	wxChoice* m_base_tm;
    wxChoice* m_weight;
	wxBitmapButton* m_weight_button;
	Project* project;
	WeightsManager* w_manager;
	DbfGridTableBase* grid_base;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed
	// in wxGrid
	std::vector<int> col_id_map;
	
	void Apply();
	void InitFieldChoices();
	void InitTime(wxChoice* time_list);
};

#endif

