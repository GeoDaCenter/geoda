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

#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "../DataViewer/DbfGridTableBase.h"
#include "../Project.h"
#include "../logger.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "VariableSettingsDlg.h"

BEGIN_EVENT_TABLE(VariableSettingsDlg, wxDialog)
	EVT_CHOICE(XRCID("ID_TIME1"), VariableSettingsDlg::OnTime)
	EVT_CHOICE(XRCID("ID_TIME2"), VariableSettingsDlg::OnTime)
	EVT_CHOICE(XRCID("ID_TIME3"), VariableSettingsDlg::OnTime)
	EVT_CHOICE(XRCID("ID_TIME4"), VariableSettingsDlg::OnTime)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE1"),
					   VariableSettingsDlg::OnListVariableDoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE2"),
					   VariableSettingsDlg::OnListVariableDoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE3"),
					   VariableSettingsDlg::OnListVariableDoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE4"),
					   VariableSettingsDlg::OnListVariableDoubleClicked)
	EVT_LISTBOX(XRCID("ID_VARIABLE1"), VariableSettingsDlg::OnVarChange)
	EVT_LISTBOX(XRCID("ID_VARIABLE2"), VariableSettingsDlg::OnVarChange)
	EVT_LISTBOX(XRCID("ID_VARIABLE3"), VariableSettingsDlg::OnVarChange)
	EVT_LISTBOX(XRCID("ID_VARIABLE4"), VariableSettingsDlg::OnVarChange)
	EVT_BUTTON(XRCID("wxID_OK"), VariableSettingsDlg::OnOkClick)
	EVT_BUTTON(XRCID("wxID_CANCEL"), VariableSettingsDlg::OnCancelClick)
END_EVENT_TABLE()

/** This constructor will go away in the future.  When this is called,
 the actual rate smoothing is performed and the results are stored
 in the smoothed_results array.  New code should call the general
 constructor and the actual smoothing should be done by the new code. */
VariableSettingsDlg::VariableSettingsDlg(Project* project_s, short smoother,
										 GalElement* gal,
										 const wxString& title_s,
										 const wxString& var1_title_s,
										 const wxString& var2_title_s,
										 const wxString& var3_title_s,
										 const wxString& var4_title_s)
: project(project_s), grid_base(project_s->GetGridBase()),
is_time(project_s->GetGridBase()->IsTimeVariant()),
time_steps(project_s->GetGridBase()->time_steps),
m_smoother(smoother), // 9: is for MoranI EB Rate Standardization
m_gal(gal),
title(title_s), var1_title(var1_title_s), var2_title(var2_title_s),
var3_title(var3_title_s), var4_title(var4_title_s),
time_ref_var(-1), fill_smoothed_results(true), fill_result_vectors(true),
all_init(false)
{
	Init(rate_smoothed);
	SetParent(0);
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);	
	Centre();
	all_init = true;
}

/** All new code should use this constructor. */
VariableSettingsDlg::VariableSettingsDlg(Project* project_s,
										 VarType v_type_s,
										 bool fill_result_vectors_s,
										 const wxString& title_s,
										 const wxString& var1_title_s,
										 const wxString& var2_title_s,
										 const wxString& var3_title_s,
										 const wxString& var4_title_s)
: project(project_s), grid_base(project_s->GetGridBase()),
is_time(project_s->GetGridBase()->IsTimeVariant()),
time_steps(project_s->GetGridBase()->time_steps),
m_smoother(0), m_gal(0),
title(title_s), var1_title(var1_title_s), var2_title(var2_title_s),
var3_title(var3_title_s), var4_title(var4_title_s),
fill_smoothed_results(false), fill_result_vectors(fill_result_vectors_s),
all_init(false)
{
	Init(v_type_s);
	SetParent(0);
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);	
	Centre();
	all_init = true;
}

VariableSettingsDlg::~VariableSettingsDlg()
{
	if (v1_single_time) delete [] v1_single_time;
	if (v2_single_time) delete [] v2_single_time;
	if (v3_single_time) delete [] v3_single_time;
	if (v4_single_time) delete [] v4_single_time;
	if (E) delete [] E;
	if (P) delete [] P;
	if (smoothed_results) delete [] smoothed_results;
}

void VariableSettingsDlg::Init(VarType var_type)
{
	v_type = var_type;
	if (var_type == univariate) {
		num_var = 1;
	} else if (var_type == bivariate || var_type == rate_smoothed) {
		num_var = 2;
	} else if (var_type == trivariate) {
		num_var = 3;
	} else { // (var_type == quadvariate)
		num_var = 4;
	}
	
	num_obs = project->GetNumRecords();
	E = (v_type == rate_smoothed && fill_result_vectors) ? 
		new double[num_obs] : 0;
	P = (v_type == rate_smoothed && fill_result_vectors) ? 
		new double[num_obs] : 0;
	smoothed_results = (v_type == rate_smoothed && fill_result_vectors) ?
		new double[num_obs] : 0;
	m_theme = 0;
	map_theme_lb = 0;
	v1_single_time = (v_type != rate_smoothed && fill_result_vectors) ?
		new double[num_obs] : 0;
	v2_single_time = (v_type != rate_smoothed && fill_result_vectors) ?
		new double[num_obs] : 0;
	v3_single_time = (v_type != rate_smoothed && fill_result_vectors) ?
		new double[num_obs] : 0;
	v4_single_time = (v_type != rate_smoothed && fill_result_vectors) ?
		new double[num_obs] : 0;
	lb1 = 0;
	lb2 = 0;
	lb3 = 0;
	lb4 = 0;
	time_lb1 = 0;
	time_lb2 = 0;
	time_lb3 = 0;
	time_lb4 = 0;
	CreateControls();
	v1_time = 0;
	v2_time = 0;
	v3_time = 0;
	v4_time = 0;
	InitTimeChoices();
	lb1_cur_sel = 0;
	lb2_cur_sel = 0;
	lb3_cur_sel = 0;
	lb4_cur_sel = 0;
	grid_base->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (grid_base->GetColName(col_id_map[i])
			== project->default_v1_name) lb1_cur_sel = i;
		if (num_var >= 2 && grid_base->GetColName(col_id_map[i])
			== project->default_v2_name) lb2_cur_sel = i;
		if (num_var >= 3 && grid_base->GetColName(col_id_map[i])
			== project->default_v3_name) lb3_cur_sel = i;
		if (num_var >= 4 && grid_base->GetColName(col_id_map[i])
			== project->default_v4_name) lb4_cur_sel = i;
	}	
	InitFieldChoices();
	
	if (map_theme_lb) {
		map_theme_lb->Clear();
		map_theme_lb->Append("Quantile Map");
		map_theme_lb->Append("Percentile Map");
		map_theme_lb->Append("Box Map (Hinge=1.5)");
		map_theme_lb->Append("Box Map (Hinge=3.0)");
		map_theme_lb->Append("Standard Deviation Map");
		map_theme_lb->Append("Natural Breaks");
		map_theme_lb->Append("Equal Intervals");
		map_theme_lb->SetSelection(0);
		if (m_smoother == 9 || m_smoother == 5) map_theme_lb->Enable(false);
	}
}

void VariableSettingsDlg::CreateControls()
{
	if (num_var == 1 && is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_TIME_DLG_1");
	}
	if (num_var == 1 && !is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_DLG_1");
	}
	if (num_var == 2 && is_time) {
		if (v_type == rate_smoothed) {
			wxXmlResource::Get()->LoadDialog(this, GetParent(),
											 "ID_VAR_SETTINGS_TIME_DLG_RATE");
		} else {
			wxXmlResource::Get()->LoadDialog(this, GetParent(),
											 "ID_VAR_SETTINGS_TIME_DLG_2");
		}
	}
	if (num_var == 2 && !is_time) {
		if (v_type == rate_smoothed) {
			wxXmlResource::Get()->LoadDialog(this, GetParent(),
											 "ID_VAR_SETTINGS_DLG_RATE");
		} else {
			wxXmlResource::Get()->LoadDialog(this, GetParent(),
											 "ID_VAR_SETTINGS_DLG_2");
		}
	}
	if (num_var == 3 && is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_TIME_DLG_3");
	}
	if (num_var == 3 && !is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_DLG_3");
	}
	if (num_var == 4 && is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_TIME_DLG_4");
	}
	if (num_var == 4 && !is_time) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_DLG_4");
	}
	if (is_time) {
		time_lb1 = XRCCTRL(*this, "ID_TIME1", wxChoice);
		if (num_var >= 2) {
			time_lb2 = XRCCTRL(*this, "ID_TIME2", wxChoice);
		}
		if (num_var >= 3) {
			time_lb3 = XRCCTRL(*this, "ID_TIME3", wxChoice);
		}
		if (num_var >= 4) {
			time_lb4 = XRCCTRL(*this, "ID_TIME4", wxChoice);
		}
	}
	SetTitle(title);
	wxStaticText* st;
	if (FindWindow(XRCID("ID_VAR1_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR1_NAME", wxStaticText);
		st->SetLabelText(var1_title);
	}
	if (FindWindow(XRCID("ID_VAR2_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR2_NAME", wxStaticText);
		st->SetLabelText(var2_title);
	}
	if (FindWindow(XRCID("ID_VAR3_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR3_NAME", wxStaticText);
		st->SetLabelText(var3_title);
	}
	if (FindWindow(XRCID("ID_VAR4_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR4_NAME", wxStaticText);
		st->SetLabelText(var4_title);
	}
	lb1 = XRCCTRL(*this, "ID_VARIABLE1", wxListBox);
	if (num_var >= 2) lb2 = XRCCTRL(*this, "ID_VARIABLE2", wxListBox);
	if (num_var >= 3) lb3 = XRCCTRL(*this, "ID_VARIABLE3", wxListBox);
	if (num_var >= 4) lb4 = XRCCTRL(*this, "ID_VARIABLE4", wxListBox);
	
	if (FindWindow(XRCID("ID_THEMATIC"))) {
        map_theme_lb = XRCCTRL(*this, "ID_THEMATIC", wxChoice);
	}
}

void VariableSettingsDlg::OnListVariableDoubleClicked(wxCommandEvent& event)
{
	if (!all_init) return;
	OnOkClick(event);
}

void VariableSettingsDlg::OnTime(wxCommandEvent& event)
{
	if (!all_init) return;
	v1_time = time_lb1->GetSelection();
	if (num_var >= 2) v2_time = time_lb2->GetSelection();
	if (num_var >= 3) v3_time = time_lb3->GetSelection();
	if (num_var >= 4) v4_time = time_lb4->GetSelection();
	InitFieldChoices();
}

void VariableSettingsDlg::OnVarChange(wxCommandEvent& event)
{
	if (!all_init) return;
	lb1_cur_sel = lb1->GetSelection();
	if (num_var >= 2) lb2_cur_sel = lb2->GetSelection();
	if (num_var >= 3) lb3_cur_sel = lb3->GetSelection();
	if (num_var >= 4) lb4_cur_sel = lb4->GetSelection();
}

void VariableSettingsDlg::OnCancelClick(wxCommandEvent& event)
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void VariableSettingsDlg::OnOkClick(wxCommandEvent& event)
{
	if (map_theme_lb) m_theme = map_theme_lb->GetSelection();
	
	if (lb1->GetSelection() == wxNOT_FOUND) {
		wxString msg("No field chosen first variable.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	v1_col_id = col_id_map[lb1->GetSelection()];
	v1_name_with_time = lb1->GetString(lb1->GetSelection());
	v1_name = grid_base->GetColName(v1_col_id);
	project->default_v1_name = v1_name;
	if (is_time) {
		v1_time = time_lb1->GetSelection();
		project->default_v1_time = v1_time;
		if (!grid_base->IsColTimeVariant(v1_col_id)) v1_time = 0;
	}
	if (num_var >= 2) {
		if (lb2->GetSelection() == wxNOT_FOUND) {
			wxString msg("No field chosen for second variable.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v2_col_id = col_id_map[lb2->GetSelection()];
		v2_name_with_time = lb2->GetString(lb2->GetSelection());
		v2_name = grid_base->GetColName(v2_col_id);
		project->default_v2_name = v2_name;
		if (is_time) {
			v2_time = time_lb2->GetSelection();
			project->default_v2_time = v2_time;
			if (!grid_base->IsColTimeVariant(v2_col_id)) v2_time = 0;
		}
	}
	if (num_var >= 3) {
		if (lb3->GetSelection() == wxNOT_FOUND) {
			wxString msg("No field chosen for third variable.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v3_col_id = col_id_map[lb3->GetSelection()];
		v3_name_with_time = lb3->GetString(lb3->GetSelection());
		v3_name = grid_base->GetColName(v3_col_id);
		project->default_v3_name = v3_name;
		if (is_time) {
			v3_time = time_lb3->GetSelection();
			project->default_v3_time = v3_time;
			if (!grid_base->IsColTimeVariant(v3_col_id)) v3_time = 0;
		}
	}
	if (num_var >= 4) {
		if (lb4->GetSelection() == wxNOT_FOUND) {
			wxString msg("No field chosen for fourth variable.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v4_col_id = col_id_map[lb4->GetSelection()];
		v4_name_with_time = lb4->GetString(lb4->GetSelection());
		v4_name = grid_base->GetColName(v4_col_id);
		project->default_v4_name = v4_name;
		if (is_time) {
			v4_time = time_lb4->GetSelection();
			project->default_v4_time = v4_time;
			if (!grid_base->IsColTimeVariant(v4_col_id)) v4_time = 0;
		}
	}
	
	if (v_type == rate_smoothed) {
		if (!FillSmoothedResults()) return;
	} else {
		FillData();
	}

	event.Skip();
	EndDialog(wxID_OK);
}

void VariableSettingsDlg::InitTimeChoices()
{
	if (!is_time) return;
	for (int i=0; i<time_steps; i++) {
		wxString s;
		s << grid_base->time_ids[i];
		time_lb1->Append(s);
		if (num_var >= 2) time_lb2->Append(s);
		if (num_var >= 3) time_lb3->Append(s);
		if (num_var >= 4) time_lb4->Append(s);
	}
	v1_time = project->default_v1_time;
	time_lb1->SetSelection(v1_time);
	if (num_var >= 2) {
		v2_time = project->default_v2_time;
		time_lb2->SetSelection(v2_time);
	}
	if (num_var >= 3) {
		v3_time = project->default_v3_time;
		time_lb3->SetSelection(v3_time);
	}
	if (num_var >= 4) {
		v4_time = project->default_v4_time;
		time_lb4->SetSelection(v4_time);
	}
}

void VariableSettingsDlg::InitFieldChoices()
{
	wxString t1;
	wxString t2;
	wxString t3;
	wxString t4;
	if (is_time) {
		t1 << " (" << grid_base->time_ids[v1_time] << ")";
		t2 << " (" << grid_base->time_ids[v2_time] << ")";
		t3 << " (" << grid_base->time_ids[v3_time] << ")";
		t4 << " (" << grid_base->time_ids[v4_time] << ")";
	}
	
	lb1->Clear();
	if (num_var >= 2) lb2->Clear();
	if (num_var >= 3) lb3->Clear();
	if (num_var >= 4) lb4->Clear();

	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = grid_base->GetColName(col_id_map[i]);
		if (grid_base->IsColTimeVariant(col_id_map[i])) name << t1;
		lb1->Append(name);
		if (num_var >= 2) {
			wxString name = grid_base->GetColName(col_id_map[i]);
			if (grid_base->IsColTimeVariant(col_id_map[i])) name << t2;
			lb2->Append(name);
		} 
		if (num_var >= 3) {
			wxString name = grid_base->GetColName(col_id_map[i]);
			if (grid_base->IsColTimeVariant(col_id_map[i])) name << t3;
			lb3->Append(name);
		}
		if (num_var >= 4) {
			wxString name = grid_base->GetColName(col_id_map[i]);
			if (grid_base->IsColTimeVariant(col_id_map[i])) name << t4;
			lb4->Append(name);
		}
	}
	lb1->SetSelection(lb1_cur_sel);
	if (num_var >= 2) lb2->SetSelection(lb2_cur_sel);
	if (num_var >= 3) lb3->SetSelection(lb3_cur_sel);
	if (num_var >= 4) lb4->SetSelection(lb4_cur_sel);
}

void VariableSettingsDlg::FillData()
{
	std::vector<double> data;
	std::vector<bool> undefined;
	if (fill_result_vectors) {
		data.resize(grid_base->GetNumberRows());
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) data[i] = 0;
		undefined.resize(grid_base->GetNumberRows());
	}
	
	col_ids.resize(num_var);
	var_info.resize(num_var);
	if (num_var >= 1) {
		v1_col_id = col_id_map[lb1->GetSelection()];
		v1_name = grid_base->GetColName(v1_col_id);
		col_ids[0] = v1_col_id;
		var_info[0].time = v1_time;
		if (fill_result_vectors) {
			grid_base->GetColData(col_ids[0], var_info[0].time, data);
			grid_base->GetColUndefined(col_ids[0], var_info[0].time, undefined);
			for (int i=0, iend=data.size(); i<iend; i++) {
				v1_single_time[i] = undefined[i] ? 0.0 : data[i];
			}
		}
	}
	if (num_var >= 2) {
		v2_col_id = col_id_map[lb2->GetSelection()];
		v2_name = grid_base->GetColName(v2_col_id);
		col_ids[1] = v2_col_id;
		var_info[1].time = v2_time;
		if (fill_result_vectors) {
			grid_base->GetColData(col_ids[1], var_info[1].time, data);
			grid_base->GetColUndefined(col_ids[1], var_info[1].time, undefined);
			for (int i=0, iend=data.size(); i<iend; i++) {
				v2_single_time[i] = undefined[i] ? 0.0 : data[i];
			}
		}
	}
	if (num_var >= 3) {
		v3_col_id = col_id_map[lb3->GetSelection()];
		v3_name = grid_base->GetColName(v3_col_id);
		col_ids[2] = v3_col_id;
		var_info[2].time = v3_time;
		if (fill_result_vectors) {
			grid_base->GetColData(col_ids[2], var_info[2].time, data);
			grid_base->GetColUndefined(col_ids[2], var_info[2].time, undefined);
			for (int i=0, iend=data.size(); i<iend; i++) {
				v3_single_time[i] = undefined[i] ? 0.0 : data[i];
			}
		}
	}
	if (num_var >= 4) {
		v4_col_id = col_id_map[lb4->GetSelection()];
		v4_name = grid_base->GetColName(v4_col_id);
		col_ids[3] = v4_col_id;
		var_info[3].time = v4_time;
		if (fill_result_vectors) {
			grid_base->GetColData(col_ids[3], var_info[3].time, data);
			grid_base->GetColUndefined(col_ids[3], var_info[3].time, undefined);
			for (int i=0, iend=data.size(); i<iend; i++) {
				v4_single_time[i] = undefined[i] ? 0.0 : data[i];
			}
		}
	}
	
	for (int i=0; i<num_var; i++) {
		// Set Primary GeoDaVarInfo attributes
		var_info[i].name = grid_base->GetColName(col_ids[i]);
		var_info[i].is_time_variant = grid_base->IsColTimeVariant(col_ids[i]);
		// var_info[i].time already set above
		grid_base->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	//GeoDa::PrintVarInfoVector(var_info);
}

bool VariableSettingsDlg::FillSmoothedResults()
{
	std::vector<double> data;
	std::vector<bool> undefined;
	if (fill_result_vectors) {
		data.resize(grid_base->GetNumberRows());
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) data[i] = 0;
		undefined.resize(grid_base->GetNumberRows());
	}
	
	col_ids.resize(num_var);
	var_info.resize(num_var);
	v1_col_id = col_id_map[lb1->GetSelection()];
	v1_name = grid_base->GetColName(v1_col_id);
	col_ids[0] = v1_col_id;
	var_info[0].time = v1_time;
	if (fill_result_vectors) {
		int col1 = col_id_map[lb1->GetSelection()];
		grid_base->GetColData(col1, v1_time, data);
		grid_base->GetColUndefined(col1, v1_time, undefined);
		for (int i=0, iend=data.size(); i<iend; i++) {
			E[i] = undefined[i] ? 0.0 : data[i];
		}	
	}
	
	v2_col_id = col_id_map[lb2->GetSelection()];
	v2_name = grid_base->GetColName(v2_col_id);
	col_ids[1] = v2_col_id;
	var_info[1].time = v2_time;
	if (fill_result_vectors) {
		int col2 = col_id_map[lb2->GetSelection()];
		grid_base->GetColData(col2, v2_time, data);
		grid_base->GetColUndefined(col2, v2_time, undefined);
		for (int i=0, iend=data.size(); i<iend; i++) {
			P[i] = undefined[i] ? 0.0 : data[i];
		}
	}
	
	for (int i=0; i<num_var; i++) {
		// Set Primary GeoDaVarInfo attributes
		var_info[i].name = grid_base->GetColName(col_ids[i]);
		var_info[i].is_time_variant = grid_base->IsColTimeVariant(col_ids[i]);
		// var_info[i].time already set above
		grid_base->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	GeoDa::UpdateVarInfoSecondaryAttribs(var_info);
	//GeoDa::PrintVarInfoVector(var_info);
	
	if (!fill_result_vectors) return true;
	
	for (int i=0; i<num_obs; i++) {
		if (P[i] <= 0) {
			wxString msg("Base values contain non-positive numbers. "
						 "No rate computed.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return false;
		}
	}

	switch (m_smoother) {
		case 1:
			GeoDaAlgs::RateSmoother_SRS(num_obs, m_gal, P, E,
										smoothed_results, m_undef_r);
			break;
		case 2:
			GeoDaAlgs::RateSmoother_EBS(num_obs, P, E,
										smoothed_results, m_undef_r);
			break;
		case 3:
			GeoDaAlgs::RateSmoother_SEBS(num_obs, m_gal, P, E,
										 smoothed_results, m_undef_r);
			break;
		case 4:
			GeoDaAlgs::RateSmoother_RawRate(num_obs, P, E,
											smoothed_results, m_undef_r);
			break;
		case 5:
			GeoDaAlgs::RateSmoother_ExcessRisk(num_obs, P, E,
											   smoothed_results, m_undef_r);
			break;
		case 9:
			if (!GeoDaAlgs::RateStandardizeEB(num_obs, P, E,
											  smoothed_results, m_undef_r)) {
				wxString msg("Emprical Bayes Rate Standardization failed.");
				wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return false;
			}
			break;
		default:
			break;
	}
	return true;
}
