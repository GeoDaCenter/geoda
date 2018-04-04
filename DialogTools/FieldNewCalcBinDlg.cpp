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
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../GenUtils.h"
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcBinDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcBinDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_BINARY_RESULT"),
			   FieldNewCalcBinDlg::OnBinaryResultUpdated )
	EVT_CHOICE( XRCID("IDC_BINARY_RESULT_TM"),
			   FieldNewCalcBinDlg::OnBinaryResultTmUpdated )
	EVT_CHOICE( XRCID("IDC_BINARY_OPERATOR"),
			   FieldNewCalcBinDlg::OnBinaryOperatorUpdated )
    EVT_TEXT( XRCID("IDC_BINARY_OPERAND1"),
			 FieldNewCalcBinDlg::OnBinaryOperand1Updated )
	EVT_COMBOBOX( XRCID("IDC_BINARY_OPERAND1"),
				 FieldNewCalcBinDlg::OnBinaryOperand1Updated )
	EVT_CHOICE( XRCID("IDC_BINARY_OPERAND1_TM"),
			   FieldNewCalcBinDlg::OnBinaryOperand1TmUpdated )
    EVT_TEXT( XRCID("IDC_BINARY_OPERAND2"),
			 FieldNewCalcBinDlg::OnBinaryOperand2Updated )
	EVT_COMBOBOX( XRCID("IDC_BINARY_OPERAND2"),
				 FieldNewCalcBinDlg::OnBinaryOperand2Updated )
	EVT_CHOICE( XRCID("IDC_BINARY_OPERAND2_TM"),
			   FieldNewCalcBinDlg::OnBinaryOperand2TmUpdated )
END_EVENT_TABLE()

FieldNewCalcBinDlg::FieldNewCalcBinDlg(Project* project_s,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption, 
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), op_string(5), project(project_s),
table_int(project_s->GetTableInt()),
m_valid_const1(false), m_const1(1), m_var1_sel(wxNOT_FOUND),
m_valid_const2(false), m_const2(1), m_var2_sel(wxNOT_FOUND),
is_space_time(project_s->GetTableInt()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
    
	op_string[add_op] = "ADD";
	op_string[subtract_op] = "SUBTRACT";
	op_string[multiply_op] = "MULTIPLY";
	op_string[divide_op] = "DIVIDE";
	op_string[power_op] = "POWER";
		
	for (int i=0, iend=op_string.size(); i<iend; i++) {
		m_op->Append(op_string[i]);
	}
	m_op->SetSelection(0);
	
	InitFieldChoices();
	all_init = true;
}

void FieldNewCalcBinDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_BI");
    m_result = XRCCTRL(*this, "IDC_BINARY_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_BINARY_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
	m_var1 = XRCCTRL(*this, "IDC_BINARY_OPERAND1", wxComboBox);
	m_var1_tm = XRCCTRL(*this, "IDC_BINARY_OPERAND1_TM", wxChoice);
	InitTime(m_var1_tm);
    m_op = XRCCTRL(*this, "IDC_BINARY_OPERATOR", wxChoice);
    m_var2 = XRCCTRL(*this, "IDC_BINARY_OPERAND2", wxComboBox);
    m_var2_tm = XRCCTRL(*this, "IDC_BINARY_OPERAND2_TM", wxChoice);
	InitTime(m_var2_tm);
	m_text = XRCCTRL(*this, "IDC_EDIT5", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcBinDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please choose a result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	int result_col = col_id_map[m_result->GetSelection()];
	
	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(result_col);
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;

	int var1_col = wxNOT_FOUND;
	int var2_col = wxNOT_FOUND;
	if (m_var1_sel != wxNOT_FOUND) var1_col = col_id_map[m_var1_sel];
	if (m_var2_sel != wxNOT_FOUND) var2_col = col_id_map[m_var2_sel];
	if ((var1_col == wxNOT_FOUND && !m_valid_const1)
		|| (var2_col == wxNOT_FOUND && !m_valid_const2)) {
		wxString msg("Operation requires two valid "
					 "variable names or constants.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (is_space_time && !IsAllTime(result_col, m_result_tm->GetSelection()) &&
		((var1_col != wxNOT_FOUND &&
		  IsAllTime(var1_col, m_var1_tm->GetSelection())) ||
		 (var2_col != wxNOT_FOUND &&
		  IsAllTime(var2_col, m_var2_tm->GetSelection()))))
	{
		wxString msg("When \"all times\" selected for either variable, result "
					 "field must also be \"all times.\"");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
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
	std::vector<double> data0(rows, 0); // results
	std::vector<bool> undefined0(rows, false); // results
	std::vector<double> data1(rows, 0); // var1
	std::vector<bool> undefined1(rows, false); // var1
	std::vector<double> data2(rows, 0); // var2
	std::vector<bool> undefined2(rows, false); // var2
	
	if (var1_col != wxNOT_FOUND &&
		!IsAllTime(var1_col, m_var1_tm->GetSelection())) {
		int tm = IsTimeVariant(var1_col) ? m_var1_tm->GetSelection() : 0;
		table_int->GetColData(var1_col, tm, data1);
		table_int->GetColUndefined(var1_col, tm, undefined1);
	} else {
		for (int i=0; i<rows; i++) data1[i] = m_const1;
	}
	if (var2_col != wxNOT_FOUND &&
		!IsAllTime(var2_col, m_var2_tm->GetSelection())) {
		int tm = IsTimeVariant(var2_col) ? m_var2_tm->GetSelection() : 0;
		table_int->GetColData(var2_col, tm, data2);
		table_int->GetColUndefined(var2_col, tm, undefined2);
	} else {
		for (int i=0; i<rows; i++) data2[i] = m_const2;
	}
	
	for (int t=0; t<time_list.size(); t++) {
		if (var1_col != wxNOT_FOUND &&
			IsAllTime(var1_col, m_var1_tm->GetSelection()))
		{
			table_int->GetColData(var1_col, time_list[t], data1);
			table_int->GetColUndefined(var1_col, time_list[t], undefined1);
		}
		if (var2_col != wxNOT_FOUND &&
			IsAllTime(var2_col, m_var2_tm->GetSelection()))
		{
			table_int->GetColData(var2_col, time_list[t], data2);
			table_int->GetColUndefined(var2_col, time_list[t], undefined2);
		}
		for (int i=0; i<rows; i++) {
			data0[i] = 0;
			undefined0[i] = undefined1[i] || undefined2[i];
		}
		
		switch (m_op->GetSelection()) {
			case add_op:
			{
				for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
					if (!undefined0[i]) data0[i] = data1[i] + data2[i];
				}
			}
				break;
			case subtract_op:
			{
				for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
					if (!undefined0[i]) data0[i] = data1[i] - data2[i];
				}
			}
				break;
			case multiply_op:
			{
				for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
					if (!undefined0[i]) data0[i] = data1[i] * data2[i];
				}
			}
				break;
			case divide_op:
			{
				for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
					if (!undefined0[i] && data2[i] != 0) {
						data0[i] = data1[i] / data2[i];
					} else {
						data0[i] = 0;
						undefined0[i] = true;
					}
				}			
			}
				break;
			case power_op:
			{
				// If base is negative and exponent is not an integral value,
				// or if base is zero and exponent is negative, not defined.
				for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
					if (data1[i] == 0 && data2[i] < 0) undefined0[i] = true;
					if (!undefined0[i]) {
						if (data1[i] < 0) {
							// since base is negative, result will have an
							// imaginary part if exponent is not an integer
							int i_exp = (int) data2[i];
							double d_i_exp = (double) i_exp;
							if ((data2[i] - d_i_exp) != 0) {
								undefined0[i] = true;
							} else {
								data0[i] = pow(data1[i], i_exp);
							}
						} else {
							data0[i] = pow(data1[i], data2[i]);
						}
					}
				}			
			}
				break;
			default:
				return;
				break;
		}
		table_int->SetColData(result_col, time_list[t], data0);
		table_int->SetColUndefined(result_col, time_list[t], undefined0);
	}
}


void FieldNewCalcBinDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	
	wxString var1_val_orig = m_var1->GetValue();
	wxString var2_val_orig = m_var2->GetValue();
	m_result->Clear();
	{
		int sel1_temp = m_var1_sel;
		m_var1->Clear();
		m_var1_sel = sel1_temp;
		int sel2_temp = m_var2_sel;
		m_var2->Clear();
		m_var2_sel = sel2_temp;
	}
	
	table_int->FillNumericColIdMap(col_id_map);
	m_var1_str.resize(col_id_map.size());
	m_var2_str.resize(col_id_map.size());
	
	wxString r_tm, v1_tm, v2_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		v1_tm << " (" << m_var1_tm->GetStringSelection() << ")";
		v2_tm << " (" << m_var2_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(col_id_map[i]) > 1) {			
			m_result->Append(table_int->GetColName(col_id_map[i]) + r_tm);
			m_var1->Append(table_int->GetColName(col_id_map[i]) + v1_tm);
			m_var1_str[i] = table_int->GetColName(col_id_map[i]) + v1_tm;
			m_var2->Append(table_int->GetColName(col_id_map[i]) + v2_tm);
			m_var2_str[i] = table_int->GetColName(col_id_map[i]) + v2_tm;
		} else {
			m_result->Append(table_int->GetColName(col_id_map[i]));
			m_var1->Append(table_int->GetColName(col_id_map[i]));
			m_var1_str[i] = table_int->GetColName(col_id_map[i]);
			m_var2->Append(table_int->GetColName(col_id_map[i]));
			m_var2_str[i] = table_int->GetColName(col_id_map[i]);
		}	
	}
	
	if (m_result->GetCount() == prev_cnt) {
		// only the time field changed
		m_result->SetSelection(r_sel);
	} else {
		// a new variable might have been added, so find old string
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	
	if (m_var1->GetCount() == prev_cnt) {
		// only the time field changed
		if (m_var1_sel != wxNOT_FOUND) {
			m_var1->SetSelection(m_var1_sel);
            m_var1->SetSelection(m_var1_sel);
            m_var1->SetValue(m_var1->GetStringSelection());
		} else {
			m_var1->SetValue(var1_val_orig);
		}
	} else {
		// a new variable might have been added, so find old string
		if (m_var1_sel != wxNOT_FOUND) {
			m_var1->SetSelection(m_var1->FindString(var1_val_orig));
			m_var1_sel = m_var1->GetSelection();
		} else {
			m_var1->SetValue(var1_val_orig);
		}
	}
	
	if (m_var2->GetCount() == prev_cnt) {
		// only the time field changed
		if (m_var2_sel != wxNOT_FOUND) {
			m_var2->SetSelection(m_var2_sel);
            m_var2->SetSelection(m_var2_sel);
            m_var2->SetValue(m_var2->GetStringSelection());
		} else {
			m_var2->SetValue(var2_val_orig);
		}
	} else {
		// a new variable might have been added, so find old string
		if (m_var2_sel != wxNOT_FOUND) {
			m_var2->SetSelection(m_var2->FindString(var2_val_orig));
			m_var2_sel = m_var2->GetSelection();
		} else {
			m_var2->SetValue(var2_val_orig);
		}
	}
	
	Display();
}

void FieldNewCalcBinDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	l_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcBinDlg::Display()
{
	wxString s("");
	wxString lhs(m_result->GetStringSelection());
	wxString rhs("");	
	wxString var1("");
	if (m_var1_sel != wxNOT_FOUND) var1 = m_var1_str[m_var1_sel];
	if (m_var1_sel == wxNOT_FOUND && m_valid_const1) {
		var1 = m_var1->GetValue();
		var1.Trim(false);
		var1.Trim(true);
	}
	wxString var2("");
	if (m_var2_sel != wxNOT_FOUND) var2 = m_var2_str[m_var2_sel];
	if (m_var2_sel == wxNOT_FOUND && m_valid_const2) {
		var2 = m_var2->GetValue();
		var2.Trim(false);
		var2.Trim(true);
	}
	
	int op_sel = m_op->GetSelection();
	if (op_sel == add_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " + " << var2;
	} else if (op_sel == subtract_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " - " << var2;
	} else if (op_sel == multiply_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " * " << var2;
	} else if (op_sel == divide_op) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " / " << var2;
	} else { // op_sel == power_op
		if (!var1.IsEmpty() && !var2.IsEmpty()) rhs << var1 << " ^ " << var2;
	}
	
	if (lhs.IsEmpty() && rhs.IsEmpty()) {
		s = "";
	} else if (!lhs.IsEmpty() && rhs.IsEmpty()) {
		s << lhs << " =";
	} else if (lhs.IsEmpty() && !rhs.IsEmpty()) {
		s << rhs;
	} else {
		// a good time to enable the apply button.
		s << lhs << " = " << rhs;
	}
	
	m_text->SetValue(s);
}

bool FieldNewCalcBinDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (table_int->IsColTimeVariant(col_id));
}

bool FieldNewCalcBinDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!table_int->IsColTimeVariant(col_id)) return false;
	return tm_sel == project->GetTableInt()->GetTimeSteps();
}

void FieldNewCalcBinDlg::OnBinaryResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));
    Display();
}

void FieldNewCalcBinDlg::OnBinaryResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcBinDlg::OnBinaryOperatorUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand1Updated( wxCommandEvent& event )
{
	if (!all_init) return;
	
	wxString var1_val = m_var1->GetValue();
	var1_val.Trim(false);
	var1_val.Trim(true);
	if (m_var1->GetValue() != m_var1->GetStringSelection()) {
		// User has typed something in manually.
		// if value matches some item on list, then set list to that
		// otherwise, set selection back to wxNOT_FOUND
		m_var1_sel = wxNOT_FOUND;
		for (int i=0, i_end=m_var1_str.size(); m_var1_sel==-1 && i<i_end; i++) {
			if (var1_val.IsSameAs(m_var1_str[i], false)) m_var1_sel = i;
		}
		if (m_var1_sel != wxNOT_FOUND) {
			// don't use SetSelection because otherwise it will
			// be difficult to type in string names that have prefixes that
			// match someing in m_var1_str
			//m_var1->SetSelection(m_var1_sel);
		} else {
			m_valid_const1 = var1_val.ToDouble(&m_const1);
		}
	} else {
		m_var1_sel = m_var1->GetSelection();
	}
	m_var1_tm->Enable(m_var1_sel != wxNOT_FOUND &&
					  table_int->GetColTimeSteps(col_id_map[m_var1_sel]) > 1);
	Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand1TmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand2Updated( wxCommandEvent& event )
{
	if (!all_init) return;
	
	wxString var2_val = m_var2->GetValue();
	var2_val.Trim(false);
	var2_val.Trim(true);
	if (m_var2->GetValue() != m_var2->GetStringSelection()) {
		// User has typed something in manually.
		// if value matches some item on list, then set list to that
		// otherwise, set selection back to wxNOT_FOUND
		m_var2_sel = wxNOT_FOUND;
		for (int i=0, i_end=m_var2_str.size(); m_var2_sel==-1 && i<i_end; i++) {
			if (var2_val.IsSameAs(m_var2_str[i], false)) m_var2_sel = i;
		}
		if (m_var2_sel != wxNOT_FOUND) {
			// don't use SetSelection because otherwise it will
			// be difficult to type in string names that have prefixes that
			// match someing in m_var2_str
			//m_var2->SetSelection(m_var2_sel);
		} else {
			m_valid_const2 = var2_val.ToDouble(&m_const2);
		}
	} else {
		m_var2_sel = m_var2->GetSelection();
	}
	m_var2_tm->Enable(m_var2_sel != wxNOT_FOUND &&
					  table_int->GetColTimeSteps(col_id_map[m_var2_sel]) > 1);
	Display();
}

void FieldNewCalcBinDlg::OnBinaryOperand2TmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcBinDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (table_int->GetColTimeSteps(dlg.GetColId()) > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnBinaryResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcBinDlg::InitTime(wxChoice* time_list)
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
