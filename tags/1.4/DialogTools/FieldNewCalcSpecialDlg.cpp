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
#include "../logger.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcSpecialDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"),
			   FieldNewCalcSpecialDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_SPECIAL_RESULT"),
			   FieldNewCalcSpecialDlg::OnSpecialResultUpdated )
	EVT_CHOICE( XRCID("IDC_SPECIAL_RESULT_TM"),
			   FieldNewCalcSpecialDlg::OnSpecialResultTmUpdated )
    EVT_TEXT( XRCID("IDC_SPECIAL_OPERAND1"),
			   FieldNewCalcSpecialDlg::OnSpecialOperand1Updated )
    EVT_CHOICE( XRCID("IDC_SPECIAL_OPERATOR"),
			   FieldNewCalcSpecialDlg::OnSpecialOperatorUpdated )
    EVT_TEXT( XRCID("IDC_SPECIAL_OPERAND2"),
			   FieldNewCalcSpecialDlg::OnSpecialOperand2Updated )
END_EVENT_TABLE()

FieldNewCalcSpecialDlg::FieldNewCalcSpecialDlg(Project* project,
											   wxWindow* parent,
											   wxWindowID id,
											   const wxString& caption, 
											   const wxPoint& pos,
											   const wxSize& size,
											   long style )
: all_init(false), op_string(3), grid_base(project->GetGridBase()),
is_space_time(project->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
	
	op_string[normal_rand] = "NORMAL RANDOM";
	op_string[uniform_rand] = "UNIFORM RANDOM";
	op_string[enumerate] = "ENUMERATE";
		
	for (int i=0, iend=op_string.size(); i<iend; i++) {
		m_op->Append(op_string[i]);
	}
	m_op->SetSelection(0);
	
	InitFieldChoices();
	all_init = true;
}

void FieldNewCalcSpecialDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_SPEC");
    m_result = XRCCTRL(*this, "IDC_SPECIAL_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_SPECIAL_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
	m_op = XRCCTRL(*this, "IDC_SPECIAL_OPERATOR", wxChoice);
	m_var1_label = XRCCTRL(*this, "ID_SPEC_TEXT1", wxStaticText);
    m_var1 = XRCCTRL(*this, "IDC_SPECIAL_OPERAND1", wxTextCtrl);
	m_var1->SetValue("0");
	m_var1_valid = true;
	m_var1_const = 0;
	m_var2_label = XRCCTRL(*this, "ID_SPEC_TEXT2", wxStaticText);
    m_var2 = XRCCTRL(*this, "IDC_SPECIAL_OPERAND2", wxTextCtrl);
	m_var2->SetValue("1.0");
	m_var2_valid = true;
	m_var2_const = 1;
    m_text = XRCCTRL(*this, "IDC_EDIT_SPEC", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcSpecialDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please choose a result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	int result_col = col_id_map[m_result->GetSelection()];

	int op_sel = m_op->GetSelection();
	
	if (!m_var1_valid || !m_var2_valid) {
		wxString msg("Normal distribution requires valid real numbers for "
					 "mean and standard deviation.  The standard "
					 "deviation must be positive and non-zero.");
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
	
	std::vector<double> data(grid_base->GetNumberRows(), 0);
	std::vector<bool> undefined(grid_base->GetNumberRows(), false);
	for (int t=0; t<time_list.size(); t++) {
		switch (m_op->GetSelection()) {
			case normal_rand:
			{
				boost::normal_distribution<> norm_dist(m_var1_const,
													   m_var2_const);
				boost::variate_generator<boost::mt19937&,
				boost::normal_distribution<> > X(rng, norm_dist);
				for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
					data[i] = X();
				}
			}
				break;
			case uniform_rand:
			{
				static boost::uniform_01<boost::mt19937> X(rng);
				for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
					data[i] = X();
				}
			}
				break;
			case enumerate:
			{
				for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
					data[i] = (double) i+1;
				}
			}
				break;
			default:
				return;
				break;
		}
		grid_base->col_data[result_col]->SetFromVec(data, time_list[t]);
		grid_base->col_data[result_col]->SetUndefined(undefined, time_list[t]);
	}
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
}


void FieldNewCalcSpecialDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	m_result->Clear();

	grid_base->FillNumericColIdMap(col_id_map);
	wxString t;
	if (is_space_time) t << " (" << m_result_tm->GetStringSelection() << ")";
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			grid_base->col_data[col_id_map[i]]->time_steps > 1) {
			m_result->Append(grid_base->col_data[col_id_map[i]]->name + t);
		} else {
			m_result->Append(grid_base->col_data[col_id_map[i]]->name);
		}
	}
	
	if (m_result->GetCount() == prev_cnt) {
		m_result->SetSelection(r_sel);
	} else {
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	
	Display();
}

void FieldNewCalcSpecialDlg::UpdateOtherPanels()
{
	u_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	l_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcSpecialDlg::Display()
{
	if (!all_init) return;
	wxString s("");
	wxString lhs(m_result->GetStringSelection());
	wxString rhs("");
	wxString var1 = m_var1_valid ? m_var1->GetValue() : wxString("");
	wxString var2 = m_var2_valid ? m_var2->GetValue() : wxString("");
	
	int op_sel = m_op->GetSelection();
	
	m_var1_label->Show(op_sel == normal_rand);
	m_var2_label->Show(op_sel == normal_rand);
	m_var1->Show(op_sel == normal_rand);
	m_var2->Show(op_sel == normal_rand);
	
	if (op_sel == normal_rand) {
		if (!var1.IsEmpty() && !var2.IsEmpty()) {
			rhs << "Random Gaussian dist with mean=" << var1;
			rhs << ", sd=" << var2;
		}
	} else if (op_sel == uniform_rand) {
		rhs = "Random uniform dist on unit interval";
	} else { // op_sel == enumerate
		rhs = "enumerate as 1, 2, 3, ...";
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
	Refresh();
}

bool FieldNewCalcSpecialDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (grid_base->IsColTimeVariant(col_id));
}

bool FieldNewCalcSpecialDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!grid_base->IsColTimeVariant(col_id)) return false;
	return tm_sel == grid_base->time_steps;
}

void FieldNewCalcSpecialDlg::OnSpecialResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));
    Display();
}

void FieldNewCalcSpecialDlg::OnSpecialResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcSpecialDlg::OnSpecialOperatorUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcSpecialDlg::OnSpecialOperand1Updated( wxCommandEvent& event )
{
	if (!all_init) return;
	wxString var1_val = m_var1->GetValue();
	var1_val.Trim(false);
	var1_val.Trim(true);
	m_var1_valid = var1_val.ToDouble(&m_var1_const);
	Display();
}

void FieldNewCalcSpecialDlg::OnSpecialOperand2Updated( wxCommandEvent& event )
{
	if (!all_init) return;
	wxString var2_val = m_var2->GetValue();
	var2_val.Trim(false);
	var2_val.Trim(true);
	m_var2_valid = var2_val.ToDouble(&m_var2_const);
	// standard deviation must be positive
	if (m_var2_valid && m_var2_const <= 0) m_var2_valid = false;
	Display();
}

void FieldNewCalcSpecialDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (grid_base->col_data[dlg.GetColId()]->time_steps > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnSpecialResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcSpecialDlg::InitTime(wxChoice* time_list)
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

