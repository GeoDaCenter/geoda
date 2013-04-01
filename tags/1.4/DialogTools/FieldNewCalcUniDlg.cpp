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
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcUniDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcUniDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_UNARY_RESULT"),
			   FieldNewCalcUniDlg::OnUnaryResultUpdated )
	EVT_CHOICE( XRCID("IDC_UNARY_RESULT_TM"),
		   FieldNewCalcUniDlg::OnUnaryResultTmUpdated )
	EVT_CHOICE( XRCID("IDC_UNARY_OPERATOR"),
			   FieldNewCalcUniDlg::OnUnaryOperatorUpdated )
	EVT_TEXT( XRCID("IDC_UNARY_OPERAND"),
			 FieldNewCalcUniDlg::OnUnaryOperandUpdated )
    EVT_COMBOBOX( XRCID("IDC_UNARY_OPERAND"),
				 FieldNewCalcUniDlg::OnUnaryOperandUpdated )
	EVT_CHOICE( XRCID("IDC_UNARY_OPERAND_TM"),
				 FieldNewCalcUniDlg::OnUnaryOperandTmUpdated )
END_EVENT_TABLE()

FieldNewCalcUniDlg::FieldNewCalcUniDlg(Project* project,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption,
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), op_string(9), grid_base(project->GetGridBase()),
m_valid_const(false), m_const(1), m_var_sel(wxNOT_FOUND),
is_space_time(project->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
    
	op_string[assign_op] = "ASSIGN";
	op_string[negate_op] = "NEGATIVE";
	op_string[invert_op] = "INVERT";
	op_string[sqrt_op] = "SQUARE ROOT";
	op_string[log_10_op] = "LOG (base 10)";
	op_string[log_e_op] = "LOG (base e)";
	op_string[dev_from_mean_op] = "DEVIATION FROM MEAN";
	op_string[standardize_op] = "STANDARDIZED";
	op_string[shuffle_op] = "SHUFFLE";
	
	for (int i=0, iend=op_string.size(); i<iend; i++) {
		m_op->Append(op_string[i]);
	}
	m_op->SetSelection(0);
	
	InitFieldChoices();
	all_init = true;
}

void FieldNewCalcUniDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_UN");
    m_result = XRCCTRL(*this, "IDC_UNARY_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_UNARY_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
    m_op = XRCCTRL(*this, "IDC_UNARY_OPERATOR", wxChoice);
    m_var = XRCCTRL(*this, "IDC_UNARY_OPERAND", wxComboBox);
	m_var_tm = XRCCTRL(*this, "IDC_UNARY_OPERAND_TM", wxChoice);
    InitTime(m_var_tm);
	m_text = XRCCTRL(*this, "IDC_EDIT1", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcUniDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please choose a Result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	int result_col = col_id_map[m_result->GetSelection()];

	int var_col = wxNOT_FOUND;
	if (m_var_sel != wxNOT_FOUND) {
		var_col = col_id_map[m_var_sel];
	}	
	if (var_col == wxNOT_FOUND && !m_valid_const) {
		wxString msg("Operation requires a valid field name or constant.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (is_space_time && var_col != wxNOT_FOUND &&
		!IsAllTime(result_col, m_result_tm->GetSelection()) &&
		IsAllTime(var_col, m_var_tm->GetSelection())) {
		wxString msg("When \"all times\" selected for variable, result "
					 "field must also be \"all times.\"");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	
	std::vector<int> time_list;
	if (IsAllTime(result_col, m_result_tm->GetSelection())) {
		time_list.resize(grid_base->time_steps);
		for (int i=0; i<grid_base->time_steps; i++) time_list[i] = i;
	} else {
		int tm = IsTimeVariant(result_col) ? m_result_tm->GetSelection() : 0;
		time_list.resize(1);
		time_list[0] = tm;
	}
	
	int rows = grid_base->GetNumberRows();
	std::vector<double> data(rows, 0);
	std::vector<bool> undefined(rows, false);
	if (var_col != wxNOT_FOUND &&
		!IsAllTime(var_col, m_var_tm->GetSelection())) {
		int tm = IsTimeVariant(var_col) ? m_var_tm->GetSelection() : 0;
		grid_base->col_data[var_col]->GetVec(data, tm);
		grid_base->col_data[var_col]->GetUndefined(undefined, tm);
	} else {
		for (int i=0; i<rows; i++) data[i] = m_const;
	}
	std::vector<double> r_data(grid_base->GetNumberRows(), 0);
	std::vector<bool> r_undefined(grid_base->GetNumberRows(), false);
	
	for (int t=0; t<time_list.size(); t++) {
		if (var_col != wxNOT_FOUND &&
			IsAllTime(var_col, m_var_tm->GetSelection()))
		{
			grid_base->col_data[var_col]->GetVec(data, time_list[t]);
			grid_base->col_data[var_col]->GetUndefined(undefined, time_list[t]);
		}
		for (int i=0; i<rows; i++) {
			r_data[i] = data[i];
			r_undefined[i] = undefined[i];
		}
		switch (m_op->GetSelection()) {
			case assign_op:
			{
			}
				break;
			case negate_op:
			{
				for (int i=0; i<rows; i++) {
					if (!undefined[i]) r_data[i] = -data[i];
				}
			}
				break;
			case invert_op:
			{
				for (int i=0; i<rows; i++) {
					if (!undefined[i] && data[i] != 0) {
						r_data[i] = 1.0 / data[i];
					} else {
						r_data[i] = 0;
						r_undefined[i] = true;
					}
				}
			}
				break;
			case sqrt_op:
			{
				for (int i=0; i<rows; i++) {
					if (!undefined[i] && data[i] >= 0) {
						r_data[i] = sqrt(data[i]);
					} else {
						r_data[i] = 0;
						r_undefined[i] = true;
					}
				}
			}
				break;
			case log_10_op:
			{
				for (int i=0; i<rows; i++) {
					if (!undefined[i] && data[i] > 0) {
						r_data[i] = log10(data[i]);
					} else {
						r_data[i] = 0;
						r_undefined[i] = true;
					}
				}
			}
				break;			
			case log_e_op:
			{
				for (int i=0; i<rows; i++) {
					if (!undefined[i] && data[i] > 0) {
						r_data[i] = log(data[i]);
					} else {
						r_data[i] = 0;
						r_undefined[i] = true;
					}
				}
			}
				break;
			case dev_from_mean_op:
			{
				for (int i=0; i<rows; i++) {
					if (undefined[i]) {
						wxString msg;
						msg << "Observation " << i;
						msg << " is undefined. ";
						msg << "Operation aborted.";
						wxMessageDialog dlg (this, msg, "Error",
											 wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}
					r_data[i] = data[i];
				}
				GenUtils::DeviationFromMean(r_data);
			}
				break;
			case standardize_op:
			{
				for (int i=0; i<rows; i++) {
					if (undefined[i]) {
						wxString msg;
						msg << "Observation ";
						msg << i << " is undefined. ";
						msg << "Operation aborted.";
						wxMessageDialog dlg (this, msg, "Error",
											 wxOK | wxICON_ERROR);
						dlg.ShowModal();
						return;
					}
					r_data[i] = data[i];
				}
				double ssum = 0.0;
				for (int i=0; i<rows; i++) ssum += r_data[i] * r_data[i];
				if (ssum == 0) {
					wxString msg("Standard deviation is 0, operation aborted.");
					wxMessageDialog dlg (this, msg, "Error", wxOK|wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
				GenUtils::StandardizeData(r_data);
			}
				break;
			case shuffle_op:
			{
				for (int i=0; i<rows; i++) {
					r_data[i] = data[i];
					r_undefined[i] = undefined[i];
				}
				static boost::random::uniform_int_distribution<> X(0, rows-1);
				// X(rng) -> returns a uniform random number from 0 to rows-1;
				for (int i=0; i<rows; i++) {
					// swap each item in data with a random position in data.
					// This will produce a random permutation
					int r = X(rng);
					double d_t = r_data[r];
					bool u_t = r_undefined[r];
					r_data[r] = r_data[i];
					r_undefined[r] = r_undefined[i];
					r_data[i] = d_t;
					r_undefined[i] = u_t;
					if (undefined[i]) r_data[i] = 0;
					if (undefined[r]) r_data[r] = 0;
				}
			}
				break;
			default:
				return;
				break;
		}
		grid_base->col_data[result_col]->SetFromVec(r_data, time_list[t]);
		grid_base->col_data[result_col]->SetUndefined(r_undefined, time_list[t]);
	}
}

void FieldNewCalcUniDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	
	wxString var_val_orig = m_var->GetValue();
	m_result->Clear();
	{
		int sel_temp = m_var_sel;
		m_var->Clear();
		m_var_sel = sel_temp;
	}
	
	grid_base->FillNumericColIdMap(col_id_map);
	m_var_str.resize(col_id_map.size());

	wxString r_tm, v_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		v_tm << " (" << m_var_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			grid_base->col_data[col_id_map[i]]->time_steps > 1) {			
			m_result->Append(grid_base->col_data[col_id_map[i]]->name + r_tm);
			m_var->Append(grid_base->col_data[col_id_map[i]]->name + v_tm);
			m_var_str[i] = grid_base->col_data[col_id_map[i]]->name + v_tm;
		} else {
			m_result->Append(grid_base->col_data[col_id_map[i]]->name);
			m_var->Append(grid_base->col_data[col_id_map[i]]->name);
			m_var_str[i] = grid_base->col_data[col_id_map[i]]->name;
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

void FieldNewCalcUniDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	l_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcUniDlg::Display()
{
	if (!all_init) return;
	wxString s("");
	wxString lhs(m_result->GetStringSelection());
	wxString rhs("");
	wxString var("");
	if (m_var_sel != wxNOT_FOUND) var = m_var_str[m_var_sel];
	if (m_var_sel == wxNOT_FOUND && m_valid_const) {
		var = m_var->GetValue();
		var.Trim(false);
		var.Trim(true);
	}
	
	int op_sel = m_op->GetSelection();
	if (op_sel == assign_op) {
		rhs = var;
	} else if (op_sel == negate_op) {
		if (!var.IsEmpty()) rhs << "-" << var;
	} else if (op_sel == invert_op) {
		if (!var.IsEmpty()) rhs << "1/" << var;
	} else if (op_sel == sqrt_op) {
		if (!var.IsEmpty()) rhs << "sqrt( " << var << " )";
	} else if (op_sel == log_10_op) {
		if (!var.IsEmpty()) rhs << "log( " << var << " )";
	} else if (op_sel == log_e_op) {
		if (!var.IsEmpty()) rhs << "ln( " << var << " )";
	} else if (op_sel == dev_from_mean_op) {
		if (!var.IsEmpty()) rhs << "dev from mean of " << var;
	} else if (op_sel == standardize_op) {
		if (!var.IsEmpty()) rhs << "standardized dev from mean of " << var;
	} else { // op_sel == shuffle_op
		if (!var.IsEmpty()) rhs << "randomly permute values in " << var;
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

bool FieldNewCalcUniDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (grid_base->IsColTimeVariant(col_id));
}

bool FieldNewCalcUniDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!grid_base->IsColTimeVariant(col_id)) return false;
	return tm_sel == grid_base->time_steps;
}

void FieldNewCalcUniDlg::OnUnaryResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));
    Display();
}

void FieldNewCalcUniDlg::OnUnaryResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcUniDlg::OnUnaryOperatorUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcUniDlg::OnUnaryOperandUpdated( wxCommandEvent& event )
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
					 grid_base->col_data[col_id_map[m_var_sel]]->time_steps>1);
	Display();
}

void FieldNewCalcUniDlg::OnUnaryOperandTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcUniDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (grid_base->col_data[dlg.GetColId()]->time_steps > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnUnaryResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcUniDlg::InitTime(wxChoice* time_list)
{
	time_list->Clear();
	for (int i=0; i<grid_base->time_steps; i++) {
		wxString t;
		t << grid_base->time_ids[i];
		time_list->Append(t);
	}
	time_list->Append("all times");
	time_list->SetSelection(grid_base->time_steps);
	time_list->Disable();
	time_list->Show(is_space_time);
}
