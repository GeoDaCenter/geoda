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
#include "../ShapeOperations/RateSmoothing.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "ExportDataDlg.h"
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
table_int(project_s->GetTableInt()), w_man_int(project_s->GetWManInt()),
is_space_time(project_s->GetTableInt()->IsTimeVariant())
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
	InitWeightsList();
	
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
    m_weights = XRCCTRL(*this, "IDC_RATE_WEIGHT", wxChoice);
	//m_weights_button = XRCCTRL(*this, "ID_OPEN_WEIGHT", wxBitmapButton);
	m_weights->Enable(false);
	//m_weights_button->Enable(false);
}

void FieldNewCalcRateDlg::SaveValidSubsetAs()
{
    
}

void FieldNewCalcRateDlg::Apply()
{
	if (m_result->GetSelection() == wxNOT_FOUND) {
		wxString msg("Please select a results field.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	const int op = m_method->GetSelection();
	if ((op == 3 || op == 4) && GetWeightsId().is_nil()) {
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
	const int cop1 = col_id_map[m_event->GetSelection()];
	const int cop2 = col_id_map[m_base->GetSelection()];
	
	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(result_col);
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;
	
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

	boost::uuids::uuid weights_id = GetWeightsId();
	if (op == 3 || op == 4)	{
		if (!w_man_int->IsValid(weights_id)) {
			wxString msg("Was not able to load weights matrix.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
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
	
	const int obs = table_int->GetNumberRows();
	
	HighlightState* highlight_state = project->GetHighlightState();
	std::vector<bool>& hs = highlight_state->GetHighlight();

	bool has_undefined = false;
	double* B = new double[obs]; // Base variable vector == cop2
	double* E = new double[obs]; // Event variable vector == cop1
	double* r = new double[obs]; // result vector
    
	std::vector<double> data(obs);
	std::vector<bool> undef_r(obs, false);

	if (!IsAllTime(cop2, m_base_tm->GetSelection())) {
        std::vector<bool> undef(obs);
		int tm = IsTimeVariant(cop2) ? m_base_tm->GetSelection() : 0;
		table_int->GetColData(cop2, tm, data);
        table_int->GetColUndefined(cop2, tm, undef);
        for (int i=0; i<obs; i++) {
            B[i] = data[i];
            undef_r[i] == undef_r[i] || undef[i];
        }
	}
	if (!IsAllTime(cop1, m_event_tm->GetSelection())) {
        std::vector<bool> undef(obs);
		int tm = IsTimeVariant(cop1) ? m_event_tm->GetSelection() : 0;
		table_int->GetColData(cop1, tm, data);
        table_int->GetColUndefined(cop1, tm, undef);
        for (int i=0; i<obs; i++) {
            E[i] = data[i];
            undef_r[i] == undef_r[i] || undef[i];
        }
	}
	
	for (int t=0; t<time_list.size(); t++) {
		if (IsAllTime(cop2, m_base_tm->GetSelection())) {
            std::vector<bool> undef(obs);
			table_int->GetColData(cop2, time_list[t], data);
            table_int->GetColUndefined(cop2, time_list[t], undef);
            for (int i=0; i<obs; i++) {
                B[i] = data[i];
                undef_r[i] == undef_r[i] || undef[i];
            }
		}
		if (IsAllTime(cop1, m_event_tm->GetSelection())) {
            std::vector<bool> undef(obs);
			table_int->GetColData(cop1, time_list[t], data);
            table_int->GetColUndefined(cop1, time_list[t], undef);
            for (int i=0; i<obs; i++) {
                E[i] = data[i];
                undef_r[i] == undef_r[i] || undef[i];
            }
		}
        
		for (int i=0; i<obs; i++)
            r[i] = -9999;
	

		switch (op) {
			case 0:
				has_undefined = GdaAlgs::RateSmoother_RawRate(obs, B, E, r, undef_r);
				break;
			case 1:
				has_undefined = GdaAlgs::RateSmoother_ExcessRisk(obs, B, E, r, undef_r);
				break;
			case 2:
				has_undefined = GdaAlgs::RateSmoother_EBS(obs, B, E, r, undef_r);
				break;
			case 3:
				has_undefined = GdaAlgs::RateSmoother_SRS(obs, w_man_int,
														  weights_id, B, E, r,
														  undef_r);
				break;
			case 4:
				has_undefined = GdaAlgs::RateSmoother_SEBS(obs, w_man_int,
														   weights_id, B, E, r,
														   undef_r);
				break;
			case 5:
				has_undefined = GdaAlgs::RateStandardizeEB(obs, B, E, r, undef_r);
				break;
			default:
				break;
		}
	
		for (int i=0; i<obs; i++) data[i] = r[i];
		table_int->SetColData(result_col, time_list[t], data);
		table_int->SetColUndefined(result_col, time_list[t], undef_r);

	}
	
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
	m_result->Clear();
	m_event->Clear();
	m_base->Clear();
	
	table_int->FillNumericColIdMap(col_id_map);
	
	wxString r_tm, event_tm, base_tm;
	if (is_space_time) {
		r_tm << " (" << m_result_tm->GetStringSelection() << ")";
		event_tm << " (" << m_event_tm->GetStringSelection() << ")";
		base_tm << " (" << m_base_tm->GetStringSelection() << ")";
	}
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (is_space_time &&
			table_int->GetColTimeSteps(col_id_map[i]) > 1) {			
			m_result->Append(table_int->GetColName(col_id_map[i]) + r_tm);
			m_event->Append(table_int->GetColName(col_id_map[i]) +event_tm);
			m_base->Append(table_int->GetColName(col_id_map[i]) + base_tm);
		} else {
			m_result->Append(table_int->GetColName(col_id_map[i]));
			m_event->Append(table_int->GetColName(col_id_map[i]));
			m_base->Append(table_int->GetColName(col_id_map[i]));
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
	return (table_int->IsColTimeVariant(col_id));
}

bool FieldNewCalcRateDlg::IsAllTime(int col_id, int tm_sel)
{
	if (!is_space_time) return false;
	if (!table_int->IsColTimeVariant(col_id)) return false;
	return tm_sel == project->GetTableInt()->GetTimeSteps();
}

/** Refreshes weights list and remembers previous selection if
 weights choice is still there and a selection was previously made */
void FieldNewCalcRateDlg::InitWeightsList()
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
boost::uuids::uuid FieldNewCalcRateDlg::GetWeightsId()
{
	long sel = m_weights->GetSelection();
	if (w_ids.size() == 0 || sel == wxNOT_FOUND) {
		return boost::uuids::nil_uuid();
	}
	return w_ids[sel];
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
	GdaFrame::GetGdaFrame()->OnToolsWeightsManager(event);
}

void FieldNewCalcRateDlg::OnAddColumnClick( wxCommandEvent& event )
{
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	InitFieldChoices();
	wxString sel_str = dlg.GetColName();
	if (table_int->GetColTimeSteps(dlg.GetColId()) > 1) {
		sel_str << " (" << m_result_tm->GetStringSelection() << ")";
	}
	m_result->SetSelection(m_result->FindString(sel_str));
	OnRateResultUpdated(event);
	UpdateOtherPanels();
}

void FieldNewCalcRateDlg::OnMethodChange( wxCommandEvent& event )
{
	const int op = m_method->GetSelection();
	m_weights->Enable(op == 3 || op == 4);
	//m_weights_button->Enable(op == 3 || op == 4);
}

void FieldNewCalcRateDlg::InitTime(wxChoice* time_list)
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
