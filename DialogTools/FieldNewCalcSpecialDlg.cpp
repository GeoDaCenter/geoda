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
#include <wx/grid.h>
#include "../GenUtils.h"
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableFrame.h"
#include "../DataViewer/TimeState.h"
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

FieldNewCalcSpecialDlg::FieldNewCalcSpecialDlg(Project* project_s,
											   wxWindow* parent,
											   wxWindowID id,
											   const wxString& caption, 
											   const wxPoint& pos,
											   const wxSize& size,
											   long style )
: all_init(false), op_string(3), project(project_s),
table_int(project_s->GetTableInt()),
is_space_time(project_s->GetTableInt()->IsTimeVariant())
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

	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(result_col);
    GdaConst::FieldType col_type = table_int->GetColType(result_col);
    
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;
	
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
		int ts = project->GetTableInt()->GetTimeSteps();
		time_list.resize(ts);
		for (int i=0; i<ts; i++) time_list[i] = i;
	} else {
		int tm = IsTimeVariant(result_col) ? m_result_tm->GetSelection() : 0;
		time_list.resize(1);
		time_list[0] = tm;
	}
    
    int n_rows = table_int->GetNumberRows();
	std::vector<bool> undefined(n_rows, false);
	
	for (int t=0; t<time_list.size(); t++) {
		switch (m_op->GetSelection()) {
			case normal_rand:
			{
				boost::normal_distribution<> norm_dist(m_var1_const, m_var2_const);
				boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > X(rng, norm_dist);
    
                if (col_type == GdaConst::double_type ) {
                    std::vector<double> data(n_rows, 0);
                    for (int i=0; i<n_rows; i++) {
                        data[i] = X();
                    }
                    table_int->SetColData(result_col, time_list[t], data);
                    
                } else if (col_type == GdaConst::long64_type) {
                    std::vector<wxInt64> data(n_rows, 0);
                    for (int i=0; i<n_rows; i++) {
                        data[i] = (wxInt64)X();
                    }
                    table_int->SetColData(result_col, time_list[t], data);
                    
                } else if (col_type == GdaConst::string_type) {
                    std::vector<wxString> data(n_rows, wxEmptyString);
                    for (int i=0; i<n_rows; i++) {
                        data[i] = wxString::Format("%f", X());
                    }
                    table_int->SetColData(result_col, time_list[t], data);
                    
                }
                table_int->SetColUndefined(result_col, time_list[t], undefined);
			}
				break;
			case uniform_rand:
			{
				static boost::uniform_01<boost::mt19937> X(rng);
                
                if (col_type == GdaConst::double_type) {
                    std::vector<double> data(n_rows, 0);
                    for (int i=0; i<n_rows; i++) data[i] = X();
                    table_int->SetColData(result_col, time_list[t], data);
                } else if (col_type == GdaConst::long64_type) {
                    std::vector<wxInt64> data(n_rows, 0);
                    for (int i=0; i<n_rows; i++) data[i] = (wxInt64)X();
                    table_int->SetColData(result_col, time_list[t], data);
                } else if (col_type == GdaConst::string_type) {
                    std::vector<wxString> data(n_rows, wxEmptyString);
                    for (int i=0; i<n_rows; i++) data[i] = wxString::Format("%f", X());
                    table_int->SetColData(result_col, time_list[t], data);
                }
                table_int->SetColUndefined(result_col, time_list[t], undefined);
			}
				break;
			case enumerate:
			{
                std::vector<int> row_order;
                TableFrame* tf = 0;
                wxGrid* g = project->FindTableGrid();
                if (g) tf = (TableFrame*) g->GetParent()->GetParent(); // wxPanel<wxFrame
                if (tf) {
                    row_order = tf->GetRowOrder();
                }
                
                if (col_type == GdaConst::double_type) {
                    std::vector<double> data(n_rows, 0);
                    if (row_order.empty())
                        for (int i=0; i<n_rows; i++) data[i] = i+1;
                    else
                        for (int i=0; i<n_rows; i++) data[row_order[i]] = i+1;
                    
                    table_int->SetColData(result_col, time_list[t], data);
                } else if (col_type == GdaConst::long64_type) {
                    std::vector<wxInt64> data(n_rows, 0);
                    if (row_order.empty())
                        for (int i=0; i<n_rows; i++) data[i] = i+1;
                    else
                        for (int i=0; i<n_rows; i++) data[row_order[i]] = i+1;
                    
                    table_int->SetColData(result_col, time_list[t], data);
                } else if (col_type == GdaConst::string_type) {
                    std::vector<wxString> data(n_rows, wxEmptyString);
                    if (row_order.empty())
                        for (int i=0; i<n_rows; i++) data[i] = wxString::Format("%d", i+1);
                    else 
                        for (int i=0; i<n_rows; i++) data[row_order[i]] = wxString::Format("%d", i+1);
                    table_int->SetColData(result_col, time_list[t], data);
                }                
                table_int->SetColUndefined(result_col, time_list[t], undefined);
			}
				break;
			default:
				return;
				break;
		}
		

	}
}


void FieldNewCalcSpecialDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	m_result->Clear();

	table_int->FillNumericColIdMap(col_id_map);
	wxString t;
	if (is_space_time) t << " (" << m_result_tm->GetStringSelection() << ")";
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(col_id_map[i]) > 1) {
			m_result->Append(table_int->GetColName(col_id_map[i]) + t);
		} else {
			m_result->Append(table_int->GetColName(col_id_map[i]));
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
	return (table_int->IsColTimeVariant(col_id));
}

bool FieldNewCalcSpecialDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!table_int->IsColTimeVariant(col_id)) return false;
	return tm_sel == project->GetTableInt()->GetTimeSteps();
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
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (table_int->GetColTimeSteps(dlg.GetColId()) > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnSpecialResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcSpecialDlg::InitTime(wxChoice* time_list)
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

