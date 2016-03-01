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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../GeoDa.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/GalWeight.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../logger.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcLagDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"), FieldNewCalcLagDlg::OnAddColumnClick )
    EVT_CHOICE( XRCID("IDC_LAG_RESULT"),
			   FieldNewCalcLagDlg::OnLagResultUpdated )
	EVT_CHOICE( XRCID("IDC_LAG_RESULT_TM"),
		   FieldNewCalcLagDlg::OnLagResultTmUpdated )
    EVT_CHOICE( XRCID("IDC_CURRENTUSED_W"),
			   FieldNewCalcLagDlg::OnCurrentusedWUpdated )
    EVT_CHOICE( XRCID("IDC_LAG_OPERAND"),
			   FieldNewCalcLagDlg::OnLagOperandUpdated )
	EVT_CHOICE( XRCID("IDC_LAG_OPERAND_TM"),
			   FieldNewCalcLagDlg::OnLagOperandTmUpdated )
	EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"), FieldNewCalcLagDlg::OnOpenWeightClick )
END_EVENT_TABLE()

FieldNewCalcLagDlg::FieldNewCalcLagDlg(Project* project_s,
									   wxWindow* parent,
									   wxWindowID id, const wxString& caption,
									   const wxPoint& pos, const wxSize& size,
									   long style )
: all_init(false), project(project_s),
table_int(project_s->GetTableInt()), w_man_int(project_s->GetWManInt()),
is_space_time(project_s->GetTableInt()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();
	
	InitFieldChoices();
	InitWeightsList();
	m_text->SetValue(wxEmptyString);

	all_init = true;
	Display();
}

void FieldNewCalcLagDlg::CreateControls()
{
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_LAG");
    m_result = XRCCTRL(*this, "IDC_LAG_RESULT", wxChoice);
	m_result_tm = XRCCTRL(*this, "IDC_LAG_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
    m_weights = XRCCTRL(*this, "IDC_CURRENTUSED_W", wxChoice);
    m_var = XRCCTRL(*this, "IDC_LAG_OPERAND", wxChoice);
	m_var_tm = XRCCTRL(*this, "IDC_LAG_OPERAND_TM", wxChoice);
    InitTime(m_var_tm);
	m_text = XRCCTRL(*this, "IDC_EDIT6", wxTextCtrl);
	m_text->SetMaxLength(0);
}

void FieldNewCalcLagDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select a results field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (GetWeightsId().is_nil()) {
		wxString msg("Please specify a Weights matrix.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_var->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Variable field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	int result_col = col_id_map[m_result->GetSelection()];
	int var_col = col_id_map[m_var->GetSelection()];
	
	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(result_col);
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;
	
	if (is_space_time &&
		!IsAllTime(result_col, m_result_tm->GetSelection()) &&
		IsAllTime(var_col, m_var_tm->GetSelection())) {
		wxString msg("When \"all times\" selected for variable, result "
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
	
	std::vector<double> data(table_int->GetNumberRows(), 0);
	std::vector<bool> undefined(table_int->GetNumberRows(), false);
	if (!IsAllTime(var_col, m_var_tm->GetSelection())) {
		int tm = IsTimeVariant(var_col) ? m_var_tm->GetSelection() : 0;
		table_int->GetColData(var_col, tm, data);
		table_int->GetColUndefined(var_col, tm, undefined);
	}
	
	int rows = table_int->GetNumberRows();
	std::vector<double> r_data(table_int->GetNumberRows(), 0);
	std::vector<bool> r_undefined(table_int->GetNumberRows(), false);
	
	boost::uuids::uuid id = GetWeightsId();
	GalElement* W = NULL;
	{
		GalWeight* gw = w_man_int->GetGal(id);
		W = gw ? gw->gal : NULL;
		if (W == NULL) {
			wxString msg("Was not able to load weights matrix.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	for (int t=0; t<time_list.size(); t++) {
		for (int i=0; i<rows; i++) {
			r_data[i] = 0;
			r_undefined[i] = false;
		}
		if (IsAllTime(var_col, m_var_tm->GetSelection())) {
			table_int->GetColData(var_col, time_list[t], data);
			table_int->GetColUndefined(var_col, time_list[t], undefined);
		}
		// Row-standardized lag calculation.
		for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
			double lag = 0;
			const GalElement& elm_i = W[i];
			if (elm_i.Size() == 0) r_undefined[i] = true;
			for (int j=0, sz=W[i].Size(); j<sz && !r_undefined[i]; j++) {
				if (undefined[elm_i[j]]) {
					r_undefined[i] = true;
				} else {
					lag += data[elm_i[j]];
				}
			}
			r_data[i] = r_undefined[i] ? 0 : lag /= W[i].Size();
		}
		table_int->SetColData(result_col, time_list[t], r_data);
		table_int->SetColUndefined(result_col, time_list[t], r_undefined);

	}
}


void FieldNewCalcLagDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	wxString v_str_sel = m_var->GetStringSelection();
	int v_sel = m_var->GetSelection();
	m_result->Clear();
	m_var->Clear();

	table_int->FillNumericColIdMap(col_id_map);
	
	wxString r_tm, v_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		v_tm << " (" << m_var_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(col_id_map[i]) > 1) {			
			m_result->Append(table_int->GetColName(col_id_map[i]) + r_tm);
			m_var->Append(table_int->GetColName(col_id_map[i]) + v_tm);
		} else {
			m_result->Append(table_int->GetColName(col_id_map[i]));
			m_var->Append(table_int->GetColName(col_id_map[i]));
		}
	}
	
	if (m_result->GetCount() == prev_cnt) {
		m_result->SetSelection(r_sel);
	} else {
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	if (m_var->GetCount() == prev_cnt) {
		m_var->SetSelection(v_sel);
	} else {
		m_var->SetSelection(m_var->FindString(v_str_sel));
	}

	Display();
}

void FieldNewCalcLagDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	r_panel->InitFieldChoices();
}

void FieldNewCalcLagDlg::Display()
{
	wxString s = "";
	wxString lhs = m_result->GetStringSelection();
	wxString rhs = "";
	wxString w_str = "";
	
	if (!GetWeightsId().is_nil() && m_var->GetSelection() != wxNOT_FOUND)
	{
		wxString wname = w_man_int->GetShortDispName(GetWeightsId());
		rhs << wname << " * " << m_var->GetStringSelection();
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

bool FieldNewCalcLagDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (table_int->IsColTimeVariant(col_id));
}

bool FieldNewCalcLagDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!table_int->IsColTimeVariant(col_id)) return false;
	return tm_sel == project->GetTableInt()->GetTimeSteps();
}

/** Refreshes weights list and remembers previous selection if
 weights choice is still there and a selection was previously made */
void FieldNewCalcLagDlg::InitWeightsList()
{
	boost::uuids::uuid old_id = GetWeightsId();
	w_ids.clear();
	w_man_int->GetIds(w_ids);
	m_weights->Clear();
	for (size_t i=0; i<w_ids.size(); ++i) {
		m_weights->Append(w_man_int->GetLongDispName(w_ids[i]));
	}
	m_weights->SetSelection(wxNOT_FOUND);
	if (old_id.is_nil() && !w_man_int->GetDefault().is_nil()) {
		for (long i=0; i<w_ids.size(); ++i) {
			if (w_ids[i] == w_man_int->GetDefault()) {
				m_weights->SetSelection(i);
			}
		}
	} else if (!old_id.is_nil()) {
		for (long i=0; i<w_ids.size(); ++i) {
			if (w_ids[i] == old_id) m_weights->SetSelection(i);
		}
	}
}

/** Returns weights selection or nil if none selected */
boost::uuids::uuid FieldNewCalcLagDlg::GetWeightsId()
{
	long sel = m_weights->GetSelection();
	if (w_ids.size() == 0 || sel == wxNOT_FOUND) {
		return boost::uuids::nil_uuid();
	}
	return w_ids[sel];
}

void FieldNewCalcLagDlg::OnLagResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));	
    Display();
}

void FieldNewCalcLagDlg::OnLagResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcLagDlg::OnCurrentusedWUpdated( wxCommandEvent& event )
{
    Display();
}

void FieldNewCalcLagDlg::OnLagOperandUpdated( wxCommandEvent& event )
{
	int sel = m_var->GetSelection();
	m_var_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));	
    Display();
}

void FieldNewCalcLagDlg::OnLagOperandTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
    Display();
}

void FieldNewCalcLagDlg::OnOpenWeightClick( wxCommandEvent& event )
{
	GdaFrame::GetGdaFrame()->OnToolsWeightsManager(event);
}

void FieldNewCalcLagDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (table_int->GetColTimeSteps(dlg.GetColId()) > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnLagResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcLagDlg::InitTime(wxChoice* time_list)
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

