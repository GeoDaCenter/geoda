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

#include <cmath>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcDateTimeDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcDateTimeDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcDateTimeDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_DT_RESULT"),
			   FieldNewCalcDateTimeDlg::OnUnaryResultUpdated )
	EVT_CHOICE( XRCID("IDC_DT_RESULT_TM"),
		   FieldNewCalcDateTimeDlg::OnUnaryResultTmUpdated )
	EVT_CHOICE( XRCID("IDC_DT_OPERATOR"),
			   FieldNewCalcDateTimeDlg::OnUnaryOperatorUpdated )
	EVT_TEXT( XRCID("IDC_DT_OPERAND"),
			 FieldNewCalcDateTimeDlg::OnUnaryOperandUpdated )
    EVT_COMBOBOX( XRCID("IDC_DT_OPERAND"),
				 FieldNewCalcDateTimeDlg::OnUnaryOperandUpdated )
	EVT_CHOICE( XRCID("IDC_DT_OPERAND_TM"),
				 FieldNewCalcDateTimeDlg::OnUnaryOperandTmUpdated )
END_EVENT_TABLE()

FieldNewCalcDateTimeDlg::FieldNewCalcDateTimeDlg(Project* project_s,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption,
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), op_string(6), project(project_s),
table_int(project_s->GetTableInt()),
m_valid_const(false), m_const(1), m_var_sel(wxNOT_FOUND),
is_space_time(project_s->GetTableInt()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
    
	op_string[get_year_op] = "Get Year";
	op_string[get_month_op] = "Get Month";
	op_string[get_day_op] = "Get Day";
	op_string[get_hour_op] = "Get Hour";
	op_string[get_minute_op] = "Get Minute";
	op_string[get_second_op] = "Get Second";
	
	for (int i=0, iend=op_string.size(); i<iend; i++) {
		m_op->Append(op_string[i]);
	}
	m_op->SetSelection(0);
	
	InitFieldChoices();
	all_init = true;
}

void FieldNewCalcDateTimeDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_DT");
    m_result = XRCCTRL(*this, "IDC_DT_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_DT_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
    m_op = XRCCTRL(*this, "IDC_DT_OPERATOR", wxChoice);
    m_var = XRCCTRL(*this, "IDC_DT_OPERAND", wxComboBox);
	m_var_tm = XRCCTRL(*this, "IDC_DT_OPERAND_TM", wxChoice);
    InitTime(m_var_tm);
}

void FieldNewCalcDateTimeDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please choose a Result field.");
		wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	int result_col = col_id_map[m_result->GetSelection()];

	int var_col = wxNOT_FOUND;
	if (m_var_sel != wxNOT_FOUND) {
		var_col = dt_col_id_map[m_var_sel];
	}	
	if (var_col == wxNOT_FOUND && !m_valid_const) {
		wxString msg("Operation requires a valid field name or constant.");
		wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (is_space_time && var_col != wxNOT_FOUND &&
		!IsAllTime(result_col, m_result_tm->GetSelection()) &&
		IsAllTime(var_col, m_var_tm->GetSelection())) {
		wxString msg("When \"all times\" selected for variable, result "
					 "field must also be \"all times.\"");
		wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(result_col);
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;


	std::vector<int> time_list;
	if (IsAllTime(result_col, m_result_tm->GetSelection())) {
		int ts = project->GetTableInt()->GetTimeSteps();
		time_list.resize(ts);
		for (int i=0; i<ts; i++) time_list[i] = i;
	} else {
		int tm = IsTimeVariant(result_col) ? m_result_tm->GetSelection() : 0;
		time_list.resize(1);
		time_list[0] = tm;
	}
	
	int rows = table_int->GetNumberRows();
	std::vector<unsigned long long> data(rows, 0); // date/time
	std::vector<bool> undefined(rows, false);
    
	if (var_col != wxNOT_FOUND &&
		!IsAllTime(var_col, m_var_tm->GetSelection()))
    {
		int tm = IsTimeVariant(var_col) ? m_var_tm->GetSelection() : 0;
		table_int->GetColData(var_col,tm, data);
		table_int->GetColUndefined(var_col, tm, undefined);
	} else {
		for (int i=0; i<rows; i++) data[i] = m_const;
	}
    
	std::vector<wxInt64> r_data(rows, 0);
	std::vector<bool> r_undefined(rows, false);
	
	for (int t=0; t<time_list.size(); t++) {
		if (var_col != wxNOT_FOUND &&
			IsAllTime(var_col, m_var_tm->GetSelection()))
		{
			table_int->GetColData(var_col, time_list[t], data);
			table_int->GetColUndefined(var_col, time_list[t], undefined);
		}
		for (int i=0; i<rows; i++) {
			r_undefined[i] = undefined[i];
		}
		switch (m_op->GetSelection()) {
			case get_year_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = data[i] / 10000000000;
				}
			}
				break;
			case get_month_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = (data[i] % 10000000000) / 100000000;
				}
			}
				break;
			case get_day_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = (data[i] % 100000000) / 1000000;
				}
			}
				break;
			case get_hour_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = (data[i] % 1000000) / 10000;
				}
			}
				break;
			case get_minute_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = (data[i]% 10000) / 100;
				}
			}
				break;
			case get_second_op:
			{
				for (int i=0; i<rows; i++) {
                    r_data[i] = data[i] % 100;
				}
			}
				break;

			default:
				return;
				break;
		}
		table_int->SetColData(result_col, time_list[t], r_data);
		table_int->SetColUndefined(result_col, time_list[t], r_undefined);

	}
}

void FieldNewCalcDateTimeDlg::InitFieldChoices()
{
	table_int->FillNumericColIdMap(col_id_map);
	table_int->FillDateTimeColIdMap(dt_col_id_map);
   
    // integer/double fields
	wxString var_val_orig = m_var->GetValue();
	int sel_temp = m_var_sel;
	m_var->Clear();
	m_var_sel = sel_temp;
	m_var_str.resize(dt_col_id_map.size());
    wxString v_tm;
	if (is_space_time) {
		v_tm << " (" << m_var_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=dt_col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(dt_col_id_map[i]) > 1) {
			m_var->Append(table_int->GetColName(dt_col_id_map[i]) + v_tm);
			m_var_str[i] = table_int->GetColName(dt_col_id_map[i]) + v_tm;
		} else {
			m_var->Append(table_int->GetColName(dt_col_id_map[i]));
			m_var_str[i] = table_int->GetColName(dt_col_id_map[i]);
		}
	}

    // date/time fields
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	m_result->Clear();
    wxString r_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(col_id_map[i]) > 1) {
			m_result->Append(table_int->GetColName(col_id_map[i]) + r_tm);
		} else {
			m_result->Append(table_int->GetColName(col_id_map[i]));
		}
	}
	if (m_result->GetCount() == prev_cnt) {
		// only the time field changed
		m_result->SetSelection(r_sel);
	} else {
		// a new variable might have been added, so find old string
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	
	if (m_var->GetCount() == prev_cnt) {
		// only the time field changed
		if (m_var_sel != wxNOT_FOUND) {
			m_var->SetSelection(m_var_sel);
		} else {
			m_var->SetValue(var_val_orig);
		}
	} else {
		// a new variable might have been added, so find old string
		if (m_var_sel != wxNOT_FOUND) {
			m_var->SetSelection(m_var->FindString(var_val_orig));
			m_var_sel = m_var->GetSelection();
		} else {
			m_var->SetValue(var_val_orig);
		}
	}
	
	Display();
}

void FieldNewCalcDateTimeDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	l_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcDateTimeDlg::Display()
{
}

bool FieldNewCalcDateTimeDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (table_int->IsColTimeVariant(col_id));
}

bool FieldNewCalcDateTimeDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!table_int->IsColTimeVariant(col_id)) return false;
	return tm_sel == project->GetTableInt()->GetTimeSteps();
}

void FieldNewCalcDateTimeDlg::OnUnaryResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));
    Display();
}

void FieldNewCalcDateTimeDlg::OnUnaryResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcDateTimeDlg::OnUnaryOperatorUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcDateTimeDlg::OnUnaryOperandUpdated( wxCommandEvent& event )
{
	if (!all_init) return;
	
	wxString var_val = m_var->GetValue();
	var_val.Trim(false);
	var_val.Trim(true);
	if (m_var->GetValue() != m_var->GetStringSelection()) {
		// User has typed something in manually.
		// if value matches some item on list, then set list to that
		// otherwise, set selection back to wxNOT_FOUND
		m_var_sel = wxNOT_FOUND;
		for (int i=0, i_end=m_var_str.size(); m_var_sel==-1 && i<i_end; i++) {
			if (var_val.IsSameAs(m_var_str[i], false)) m_var_sel = i;
		}
		if (m_var_sel != wxNOT_FOUND) {
			// don't use SetSelection because otherwise it will
			// be difficult to type in string names that have prefixes that
			// match someing in m_var_str
			//m_var->SetSelection(m_var_sel);
		} else {
			m_valid_const = var_val.ToDouble(&m_const);
		}
	} else {
		m_var_sel = m_var->GetSelection();
	}
	m_var_tm->Enable(m_var_sel != wxNOT_FOUND &&
					 table_int->GetColTimeSteps(col_id_map[m_var_sel]) > 1);
	Display();
}

void FieldNewCalcDateTimeDlg::OnUnaryOperandTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcDateTimeDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (table_int->GetColTimeSteps(dlg.GetColId()) > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnUnaryResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcDateTimeDlg::InitTime(wxChoice* time_list)
{
	time_list->Clear();
	for (int i=0; i<project->GetTableInt()->GetTimeSteps(); i++) {
		wxString t;
		t << project->GetTableInt()->GetTimeString(i);
		time_list->Append(t);
	}
	time_list->Append("all times");
	time_list->SetSelection(project->GetTableInt()->GetTimeSteps());
	time_list->Disable();
	time_list->Show(is_space_time);
}
