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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/GalWeight.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "SelectWeightDlg.h"
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcRateDlg, wxPanel )
	EVT_BUTTON( XRCID("ID_ADD_COLUMN"),
			   FieldNewCalcRateDlg::OnAddColumnClick )
	EVT_CHOICE( XRCID("IDC_RATE_RESULT"),
			   FieldNewCalcRateDlg::OnRateResultUpdated )
	EVT_CHOICE( XRCID("IDC_RATE_RESULT_TM"),
			   FieldNewCalcRateDlg::OnRateResultTmUpdated )
	EVT_CHOICE( XRCID("IDC_RATE_OPERATOR"),
			   FieldNewCalcRateDlg::OnMethodChange )
	EVT_CHOICE( XRCID("IDC_RATE_OPERAND1"),
			   FieldNewCalcRateDlg::OnRateOperand1Updated )
	EVT_CHOICE( XRCID("IDC_RATE_OPERAND1_TM"),
			   FieldNewCalcRateDlg::OnRateOperand1TmUpdated )
	EVT_CHOICE( XRCID("IDC_RATE_OPERAND2"),
			   FieldNewCalcRateDlg::OnRateOperand2Updated )
	EVT_CHOICE( XRCID("IDC_RATE_OPERAND2_TM"),
			   FieldNewCalcRateDlg::OnRateOperand2TmUpdated )
	EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"),
			   FieldNewCalcRateDlg::OnOpenWeightClick )
END_EVENT_TABLE()

FieldNewCalcRateDlg::FieldNewCalcRateDlg(Project* project_s,
										 wxWindow* parent,
										 wxWindowID id, const wxString& caption,
										 const wxPoint& pos, const wxSize& size,
										 long style )
: all_init(false), project(project_s),
grid_base(project_s->GetGridBase()), w_manager(project_s->GetWManager()),
is_space_time(project_s->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
    Centre();

	m_method->Append("Raw Rate");
	m_method->Append("Excess Risk");
	m_method->Append("Empirical Bayes");
	m_method->Append("Spatial Rate");
	m_method->Append("Spatial Empirical Bayes");
	m_method->Append("EB Rate Standardization");
	m_method->SetSelection(0);

	InitFieldChoices();

	if (w_manager->IsDefaultWeight()) {
		m_weight->SetSelection(w_manager->GetCurrWeightInd());
	}
	all_init = true;
}

void FieldNewCalcRateDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_FIELDCALC_RATE");
    m_result = XRCCTRL(*this, "IDC_RATE_RESULT", wxChoice);
    m_result_tm = XRCCTRL(*this, "IDC_RATE_RESULT_TM", wxChoice);
	InitTime(m_result_tm);
	m_event = XRCCTRL(*this, "IDC_RATE_OPERAND1", wxChoice);
    m_event_tm = XRCCTRL(*this, "IDC_RATE_OPERAND1_TM", wxChoice);
	InitTime(m_event_tm);
	m_method = XRCCTRL(*this, "IDC_RATE_OPERATOR", wxChoice);
    m_base = XRCCTRL(*this, "IDC_RATE_OPERAND2", wxChoice);
	m_base_tm = XRCCTRL(*this, "IDC_RATE_OPERAND2_TM", wxChoice);
	InitTime(m_base_tm);
    m_weight = XRCCTRL(*this, "IDC_RATE_WEIGHT", wxChoice);
	m_weight_button = XRCCTRL(*this, "ID_OPEN_WEIGHT", wxBitmapButton);
	m_weight->Enable(false);
	m_weight_button->Enable(false);
}

void FieldNewCalcRateDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select a Result field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	const int op = m_method->GetSelection();
	if ((op == 3 || op == 4) && m_weight->GetSelection() == wxNOT_FOUND) {
		wxString msg("Weight matrix required for chosen spatial "
					 "rate method.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_event->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Event field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	if (m_base->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select an Base field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	const int result_col = col_id_map[m_result->GetSelection()];
	const int w = m_weight->GetSelection();
	const int cop1 = col_id_map[m_event->GetSelection()];
	const int cop2 = col_id_map[m_base->GetSelection()];
		
	if (is_space_time && !IsAllTime(result_col, m_result_tm->GetSelection()) &&
		(IsAllTime(cop1, m_event_tm->GetSelection()) ||
		 IsAllTime(cop2, m_base_tm->GetSelection())))
	{
		wxString msg("When \"all times\" selected for either variable, result "
					 "field must also be \"all times.\"");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	GalElement* W;
	if (op == 3 || op == 4)	{
		if (!w_manager) return;
		if (w_manager->GetNumWeights() < 0) return;
		if (w_manager->IsGalWeight(w)) {
			W = (w_manager->GetGalWeight(w))->gal;
		} else {
			wxString msg("Only weights files internally converted "
						 "to GAL format currently supported.  Please "
						 "report this error.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		if (W == NULL) return;
	}

	std::vector<int> time_list;
	if (IsAllTime(result_col, m_result_tm->GetSelection())) {
		time_list.resize(grid_base->time_steps);
		for (int i=0; i<grid_base->time_steps; i++) time_list[i] = i;
	} else {
		int tm = IsTimeVariant(result_col) ? m_result_tm->GetSelection() : 0;
		time_list.resize(1);
		time_list[0] = tm;
	}
	
	const int obs = grid_base->GetNumberRows();
	
	bool Event_undefined = false;
	if (IsAllTime(cop1, m_event_tm->GetSelection())) {
		b_array_type undefined;
		grid_base->col_data[cop1]->GetUndefined(undefined);
		for (int t=0; t<grid_base->time_steps && !Event_undefined; t++) {
			for (int i=0; i<obs && !Event_undefined; i++) {
				if (undefined[t][i]) Event_undefined = true;
			}
		}
	} else {
		std::vector<bool> undefined(obs);
		int tm = IsTimeVariant(cop1) ? m_event_tm->GetSelection() : 0;
		grid_base->col_data[cop1]->GetUndefined(undefined, tm);
		for (int i=0; i<obs && !Event_undefined; i++) {
			if (undefined[i]) Event_undefined = true;
		}		
	}
	if (Event_undefined) {
		wxString msg("Event field has undefined values.  Please define "
					 "missing values or choose a different field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	bool Base_undefined = false;
	if (IsAllTime(cop2, m_base_tm->GetSelection())) {
		b_array_type undefined;
		grid_base->col_data[cop2]->GetUndefined(undefined);
		for (int t=0; t<grid_base->time_steps && !Base_undefined; t++) {
			for (int i=0; i<obs && !Base_undefined; i++) {
				if (undefined[t][i]) Base_undefined = true;
			}
		}
	} else {
		std::vector<bool> undefined(obs);
		int tm = IsTimeVariant(cop2) ? m_base_tm->GetSelection() : 0;
		grid_base->col_data[cop2]->GetUndefined(undefined, tm);
		for (int i=0; i<obs && !Base_undefined; i++) {
			if (undefined[i]) Base_undefined = true;
		}
	}
	if (Base_undefined) {
		wxString msg("Base field has undefined values.  Please define "
					 "missing values or choose a different field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	bool Base_non_positive = false;
	if (IsAllTime(cop2, m_base_tm->GetSelection())) {
		d_array_type data;
		grid_base->col_data[cop2]->GetVec(data);
		for (int t=0; t<grid_base->time_steps && !Base_non_positive; t++) {
			for (int i=0; i<obs && !Base_non_positive; i++) {
				if (data[t][i] <= 0) Base_non_positive = true;
			}
		}
	} else {
		std::vector<double> data(obs);
		int tm = IsTimeVariant(cop2) ? m_base_tm->GetSelection() : 0;
		grid_base->col_data[cop2]->GetVec(data, tm);
		for (int i=0; i<obs && !Base_non_positive; i++) {
			if (data[i] <= 0) Base_non_positive = true;
		}
	}
	if (Base_non_positive) {
		wxString msg("Base field has zero or negative values, but all base "
					 "values must be strictly greater than zero. "
					 "Computation aborted.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	bool has_undefined = false;
	double* B = new double[obs]; // Base variable vector == cop2
	double* E = new double[obs]; // Event variable vector == cop1
	double* r = new double[obs]; // result vector
	std::vector<double> data(obs);

	if (!IsAllTime(cop2, m_base_tm->GetSelection())) {
		int tm = IsTimeVariant(cop2) ? m_base_tm->GetSelection() : 0;
		grid_base->col_data[cop2]->GetVec(data, tm);
		for (int i=0; i<obs; i++) B[i] = data[i];
	}
	if (!IsAllTime(cop1, m_event_tm->GetSelection())) {
		int tm = IsTimeVariant(cop1) ? m_event_tm->GetSelection() : 0;
		grid_base->col_data[cop1]->GetVec(data, tm);
		for (int i=0; i<obs; i++) E[i] = data[i];
	}
	
	for (int t=0; t<time_list.size(); t++) {
		if (IsAllTime(cop2, m_base_tm->GetSelection())) {
			grid_base->col_data[cop2]->GetVec(data, time_list[t]);
			for (int i=0; i<obs; i++) B[i] = data[i];
		}
		if (IsAllTime(cop1, m_event_tm->GetSelection())) {
			grid_base->col_data[cop1]->GetVec(data, time_list[t]);
			for (int i=0; i<obs; i++) E[i] = data[i];
		}
		for (int i=0; i<obs; i++) r[i] = -9999;
	
		std::vector<bool> undef_r;
		switch (op) {
			case 0:
				GeoDaAlgs::RateSmoother_RawRate(obs, B, E, r, undef_r);
				break;
			case 1:
				GeoDaAlgs::RateSmoother_ExcessRisk(obs, B, E, r, undef_r);
				break;
			case 2:
				GeoDaAlgs::RateSmoother_EBS(obs, B, E, r, undef_r);
				break;
			case 3:
				has_undefined = GeoDaAlgs::RateSmoother_SRS(obs, W, B, E, r,
															undef_r);
				break;
			case 4:
				has_undefined = GeoDaAlgs::RateSmoother_SEBS(obs, W, B, E, r,
															 undef_r);
				break;
			case 5:
				GeoDaAlgs::RateStandardizeEB(obs, B, E, r, undef_r);
				break;
			default:
				break;
		}
	
		for (int i=0; i<obs; i++) data[i] = r[i];
		grid_base->col_data[result_col]->SetFromVec(data, time_list[t]);
		grid_base->col_data[result_col]->SetUndefined(undef_r, time_list[t]);
	}
	
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	if (B) delete [] B; B = NULL;
	if (E) delete [] E; E = NULL;
	if (r) delete [] r; r = NULL;
	
	if (has_undefined) {
		wxString msg("Some calculated values were undefined and this is "
					 "most likely due to neighborless observations in the "
					 "weight matrix. Rate calculation successful for "
					 "observations with neighbors.");
		wxMessageDialog dlg (this, msg, "Success / Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	} else {
		wxString msg("Rate calculation successful.");
		wxMessageDialog dlg (this, msg, "Success", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
}


void FieldNewCalcRateDlg::InitFieldChoices()
{
	wxString r_str_sel = m_result->GetStringSelection();
	int r_sel = m_result->GetSelection();
	int prev_cnt = m_result->GetCount();
	wxString event_str_sel = m_event->GetStringSelection();
	int event_sel = m_event->GetSelection();
	wxString base_str_sel = m_base->GetStringSelection();
	int base_sel = m_base->GetSelection();
	wxString w_str_sel = m_weight->GetStringSelection();
	m_result->Clear();
	m_event->Clear();
	m_base->Clear();
	m_weight->Clear();
	
	grid_base->FillNumericColIdMap(col_id_map);
	
	wxString r_tm, event_tm, base_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		event_tm << " (" << m_event_tm->GetStringSelection() << ")";
		base_tm << " (" << m_base_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			grid_base->col_data[col_id_map[i]]->time_steps > 1) {			
			m_result->Append(grid_base->col_data[col_id_map[i]]->name + r_tm);
			m_event->Append(grid_base->col_data[col_id_map[i]]->name +event_tm);
			m_base->Append(grid_base->col_data[col_id_map[i]]->name + base_tm);
		} else {
			m_result->Append(grid_base->col_data[col_id_map[i]]->name);
			m_event->Append(grid_base->col_data[col_id_map[i]]->name);
			m_base->Append(grid_base->col_data[col_id_map[i]]->name);
		}
	}
	
	if (w_manager->GetNumWeights() > 0) {
		for (int i=0; i<w_manager->GetNumWeights(); i++) {
			m_weight->Append(w_manager->GetWFilename(i));
		}
	}
	if (m_result->GetCount() == prev_cnt) {
		m_result->SetSelection(r_sel);
	} else {
		m_result->SetSelection(m_result->FindString(r_str_sel));
	}
	if (m_event->GetCount() == prev_cnt) {
		m_event->SetSelection(event_sel);
	} else {
		m_event->SetSelection(m_event->FindString(event_str_sel));
	}
	if (m_base->GetCount() == prev_cnt) {
		m_base->SetSelection(base_sel);
	} else {
		m_base->SetSelection(m_base->FindString(base_str_sel));
	}
	m_weight->SetSelection(m_weight->FindString(w_str_sel));
}

void FieldNewCalcRateDlg::UpdateOtherPanels()
{
	s_panel->InitFieldChoices();
	u_panel->InitFieldChoices();
	b_panel->InitFieldChoices();
	l_panel->InitFieldChoices(); 
}

bool FieldNewCalcRateDlg::IsTimeVariant(int col_id)
{
	if (!is_space_time) return false;
	return (grid_base->IsColTimeVariant(col_id));
}

bool FieldNewCalcRateDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!grid_base->IsColTimeVariant(col_id)) return false;
	return tm_sel == grid_base->time_steps;
}

void FieldNewCalcRateDlg::OnRateResultUpdated( wxCommandEvent& event )
{
	int sel = m_result->GetSelection();
	m_result_tm->Enable(sel != wxNOT_FOUND &&
						IsTimeVariant(col_id_map[sel]));
}

void FieldNewCalcRateDlg::OnRateResultTmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
}

void FieldNewCalcRateDlg::OnRateOperand1Updated( wxCommandEvent& event )
{
	int sel = m_event->GetSelection();
	m_event_tm->Enable(sel != wxNOT_FOUND &&
					   IsTimeVariant(col_id_map[sel]));	
}

void FieldNewCalcRateDlg::OnRateOperand1TmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
}

void FieldNewCalcRateDlg::OnRateOperand2Updated( wxCommandEvent& event )
{
	int sel = m_base->GetSelection();
	m_base_tm->Enable(sel != wxNOT_FOUND &&
					  IsTimeVariant(col_id_map[sel]));
}

void FieldNewCalcRateDlg::OnRateOperand2TmUpdated( wxCommandEvent& event )
{
	InitFieldChoices();
}

void FieldNewCalcRateDlg::OnOpenWeightClick( wxCommandEvent& event )
{
	SelectWeightDlg dlg(project, this);
	dlg.ShowModal();
 	
	m_weight->Clear();
	for (int i=0; i<w_manager->GetNumWeights(); i++) {
		m_weight->Append(w_manager->GetWFilename(i));
	}
	if (w_manager->GetCurrWeightInd() >=0 ) {
		m_weight->SetSelection(w_manager->GetCurrWeightInd());
	}
	InitFieldChoices(); // call in case AddId was called.
	UpdateOtherPanels();
}

void FieldNewCalcRateDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (grid_base->col_data[dlg.GetColId()]->time_steps > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnRateResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcRateDlg::OnMethodChange( wxCommandEvent& event )
{
	const int op = m_method->GetSelection();
	m_weight->Enable(op == 3 || op == 4);
	m_weight_button->Enable(op == 3 || op == 4);
}

void FieldNewCalcRateDlg::InitTime(wxChoice* time_list)
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
