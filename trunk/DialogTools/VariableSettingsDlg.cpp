/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../Project.h"
#include "../logger.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "VariableSettingsDlg.h"

BEGIN_EVENT_TABLE(VariableSettingsDlg, wxDialog)
	EVT_CHOICE(XRCID("ID_TIME1"), VariableSettingsDlg::OnTime1)
	EVT_CHOICE(XRCID("ID_TIME2"), VariableSettingsDlg::OnTime2)
	EVT_CHOICE(XRCID("ID_TIME3"), VariableSettingsDlg::OnTime3)
	EVT_CHOICE(XRCID("ID_TIME4"), VariableSettingsDlg::OnTime4)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE1"),
					   VariableSettingsDlg::OnListVariable1DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE2"),
					   VariableSettingsDlg::OnListVariable2DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE3"),
					   VariableSettingsDlg::OnListVariable3DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE4"),
					   VariableSettingsDlg::OnListVariable4DoubleClicked)
	EVT_LISTBOX(XRCID("ID_VARIABLE1"), VariableSettingsDlg::OnVar1Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE2"), VariableSettingsDlg::OnVar2Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE3"), VariableSettingsDlg::OnVar3Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE4"), VariableSettingsDlg::OnVar4Change)
	EVT_SPINCTRL(XRCID("ID_NUM_CATEGORIES_SPIN"),
				 VariableSettingsDlg::OnSpinCtrl)
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
										 const wxString& var2_title_s)
: project(project_s), table_int(project_s->GetTableInt()),
is_time(project_s->GetTableInt()->IsTimeVariant() &&
		project_s->GetTableInt()->GetTimeSteps() > 1 ),
time_steps(project_s->GetTableInt()->GetTimeSteps()),
m_smoother(smoother), // 9: is for MoranI EB Rate Standardization
m_gal(gal),
title(title_s), var1_title(var1_title_s), var2_title(var2_title_s),
set_second_from_first_mode(false), set_fourth_from_third_mode(false),
num_cats_spin(0), num_categories(4),
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
										 const wxString& title_s,
										 const wxString& var1_title_s,
										 const wxString& var2_title_s,
										 const wxString& var3_title_s,
										 const wxString& var4_title_s,
										 bool _set_second_from_first_mode,
										 bool _set_fourth_from_third_mode)
: project(project_s), table_int(project_s->GetTableInt()),
is_time(project_s->GetTableInt()->IsTimeVariant()),
time_steps(project_s->GetTableInt()->GetTimeSteps()),
m_smoother(0), m_gal(0),
title(title_s), var1_title(var1_title_s), var2_title(var2_title_s),
var3_title(var3_title_s), var4_title(var4_title_s),
set_second_from_first_mode(_set_second_from_first_mode),
set_fourth_from_third_mode(_set_fourth_from_third_mode),
num_cats_spin(0), num_categories(4),
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
	
	int num_obs = project->GetNumRecords();
	E = (v_type == rate_smoothed) ? 
		new double[num_obs] : 0;
	P = (v_type == rate_smoothed) ? 
		new double[num_obs] : 0;
	smoothed_results = (v_type == rate_smoothed) ?
		new double[num_obs] : 0;
	m_theme = 0;
	map_theme_lb = 0;
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
	table_int->FillNumericColIdMap(col_id_map);
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		if (table_int->GetColName(col_id_map[i])
			== project->GetDefaultVarName(0)) {
			lb1_cur_sel = i;
			if (set_second_from_first_mode && num_var >= 2) {
				lb2_cur_sel = i;
			}
		}
		if (num_var >= 2 && table_int->GetColName(col_id_map[i])
			== project->GetDefaultVarName(1)) {
			if (!set_second_from_first_mode) {
				lb2_cur_sel = i;
			}
		}
		if (num_var >= 3 && table_int->GetColName(col_id_map[i])
			== project->GetDefaultVarName(2)) {
			lb3_cur_sel = i;
			if (set_fourth_from_third_mode && num_var >= 4) {
				lb4_cur_sel = i;
			}
		}
		if (num_var >= 4 && table_int->GetColName(col_id_map[i])
			== project->GetDefaultVarName(3)) {
			if (!set_fourth_from_third_mode) {
				lb4_cur_sel = i;
			}
		}
	}
	
	if (col_id_map.size() == 0) {
		wxString msg("No numeric variables found.");
		wxMessageDialog dlg (this, msg, "Warning", wxOK | wxICON_WARNING);
		dlg.ShowModal();
		return;
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
	if (FindWindow(XRCID("ID_NUM_CATEGORIES_SPIN"))) {
        num_cats_spin = XRCCTRL(*this, "ID_NUM_CATEGORIES_SPIN", wxSpinCtrl);
		num_categories = num_cats_spin->GetValue();
	}
	
}

void VariableSettingsDlg::OnListVariable1DoubleClicked(wxCommandEvent& event)
{
	if (!all_init) return;
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		if (is_time) {
			time_lb2->SetSelection(v1_time);
			v2_time = v1_time;
		}
	}
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable2DoubleClicked(wxCommandEvent& event)
{
	if (!all_init) return;
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable3DoubleClicked(wxCommandEvent& event)
{
	if (!all_init) return;
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		if (is_time) {
			time_lb4->SetSelection(v3_time);
			v4_time = v3_time;
		}
	}
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable4DoubleClicked(wxCommandEvent& event)
{
	if (!all_init) return;
	OnOkClick(event);
}

void VariableSettingsDlg::OnTime1(wxCommandEvent& event)
{
	if (!all_init) return;
	v1_time = time_lb1->GetSelection();
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		time_lb2->SetSelection(v1_time);
		v2_time = v1_time;
	}
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime2(wxCommandEvent& event)
{
	if (!all_init) return;
	v2_time = time_lb2->GetSelection();
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime3(wxCommandEvent& event)
{
	if (!all_init) return;
	v3_time = time_lb3->GetSelection();
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		time_lb4->SetSelection(v3_time);
		v4_time = v3_time;
	}
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime4(wxCommandEvent& event)
{
	if (!all_init) return;
	v4_time = time_lb4->GetSelection();
	InitFieldChoices();
}

void VariableSettingsDlg::OnVar1Change(wxCommandEvent& event)
{
	if (!all_init) return;
	lb1_cur_sel = lb1->GetSelection();
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		if (is_time) {
			time_lb2->SetSelection(v1_time);
			v2_time = v1_time;
		}
	}
}

void VariableSettingsDlg::OnVar2Change(wxCommandEvent& event)
{
	if (!all_init) return;
	lb2_cur_sel = lb2->GetSelection();
}

void VariableSettingsDlg::OnVar3Change(wxCommandEvent& event)
{
	if (!all_init) return;
	lb3_cur_sel = lb3->GetSelection();
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		if (is_time) {
			time_lb4->SetSelection(v3_time);
			v4_time = v3_time;
		}
	}
}

void VariableSettingsDlg::OnVar4Change(wxCommandEvent& event)
{
	if (!all_init) return;
	lb4_cur_sel = lb4->GetSelection();
}

void VariableSettingsDlg::OnSpinCtrl( wxSpinEvent& event )
{
	if (!num_cats_spin) return;
	num_categories = num_cats_spin->GetValue();
	if (num_categories < num_cats_spin->GetMin()) {
		num_categories = num_cats_spin->GetMin();
	}
	if (num_categories > num_cats_spin->GetMax()) {
		num_categories = num_cats_spin->GetMax();
	}
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
		wxString msg("No field chosen for first variable.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	v1_col_id = col_id_map[lb1->GetSelection()];
	v1_name = table_int->GetColName(v1_col_id);
	project->SetDefaultVarName(0, v1_name);
	if (is_time) {
		v1_time = time_lb1->GetSelection();
		project->SetDefaultVarTime(0, v1_time);
		if (!table_int->IsColTimeVariant(v1_col_id)) v1_time = 0;
	}
	if (num_var >= 2) {
		if (lb2->GetSelection() == wxNOT_FOUND) {
			wxString msg("No field chosen for second variable.");
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v2_col_id = col_id_map[lb2->GetSelection()];
		v2_name = table_int->GetColName(v2_col_id);
		project->SetDefaultVarName(1, v2_name);
		if (is_time) {
			v2_time = time_lb2->GetSelection();
			project->SetDefaultVarTime(1, v2_time);
			if (!table_int->IsColTimeVariant(v2_col_id)) v2_time = 0;
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
		v3_name = table_int->GetColName(v3_col_id);
		project->SetDefaultVarName(2, v3_name);
		if (is_time) {
			v3_time = time_lb3->GetSelection();
			project->SetDefaultVarTime(2, v3_time);
			if (!table_int->IsColTimeVariant(v3_col_id)) v3_time = 0;
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
		v4_name = table_int->GetColName(v4_col_id);
		project->SetDefaultVarName(3, v4_name);
		if (is_time) {
			v4_time = time_lb4->GetSelection();
			project->SetDefaultVarTime(3, v4_time);
			if (!table_int->IsColTimeVariant(v4_col_id)) v4_time = 0;
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

// Theme choice for Rate Smoothed variable settings
CatClassification::CatClassifType VariableSettingsDlg::GetCatClassifType()
{
	if (m_theme == 0) return CatClassification::quantile;
	if (m_theme == 1) return CatClassification::percentile;
	if (m_theme == 2) return CatClassification::hinge_15;
	if (m_theme == 3) return CatClassification::hinge_30;
	if (m_theme == 4) return CatClassification::stddev;
	if (m_theme == 5) return CatClassification::natural_breaks;
	if (m_theme == 6) return CatClassification::equal_intervals;
	return CatClassification::quantile; 
}

// Number of categories for Rate Smoothed variable settings
int VariableSettingsDlg::GetNumCategories()
{
	CatClassification::CatClassifType cc_type = GetCatClassifType();
	if (cc_type == CatClassification::quantile ||
		cc_type == CatClassification::natural_breaks ||
		cc_type == CatClassification::equal_intervals) {
		return num_categories;
	} else {
		return 6;
	}
}

void VariableSettingsDlg::InitTimeChoices()
{
	if (!is_time) return;
	for (int i=0; i<time_steps; i++) {
		wxString s;
		s << table_int->GetTimeString(i);
		time_lb1->Append(s);
		if (num_var >= 2) time_lb2->Append(s);
		if (num_var >= 3) time_lb3->Append(s);
		if (num_var >= 4) time_lb4->Append(s);
	}
	v1_time = project->GetDefaultVarTime(0);
	time_lb1->SetSelection(v1_time);
	if (num_var >= 2) {
		v2_time = project->GetDefaultVarTime(1);
		time_lb2->SetSelection(v2_time);
	}
	if (num_var >= 3) {
		v3_time = project->GetDefaultVarTime(2);
		time_lb3->SetSelection(v3_time);
	}
	if (num_var >= 4) {
		v4_time = project->GetDefaultVarTime(3);
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
		t1 << " (" << table_int->GetTimeString(v1_time) << ")";
		t2 << " (" << table_int->GetTimeString(v2_time) << ")";
		t3 << " (" << table_int->GetTimeString(v3_time) << ")";
		t4 << " (" << table_int->GetTimeString(v4_time) << ")";
	}
	
	lb1->Clear();
	if (num_var >= 2) lb2->Clear();
	if (num_var >= 3) lb3->Clear();
	if (num_var >= 4) lb4->Clear();

	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		wxString name = table_int->GetColName(col_id_map[i]);
		if (table_int->IsColTimeVariant(col_id_map[i])) name << t1;
		lb1->Append(name);
		if (num_var >= 2) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i])) name << t2;
			lb2->Append(name);
		} 
		if (num_var >= 3) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i])) name << t3;
			lb3->Append(name);
		}
		if (num_var >= 4) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i])) name << t4;
			lb4->Append(name);
		}
	}
	int pos = lb1->GetScrollPos(wxVERTICAL);
	lb1->SetSelection(lb1_cur_sel);
	lb1->SetFirstItem(lb1->GetSelection());
	if (num_var >= 2) {
		lb2->SetSelection(lb2_cur_sel);
		lb2->SetFirstItem(lb2->GetSelection());
	}
	if (num_var >= 3) {
		lb3->SetSelection(lb3_cur_sel);
		lb3->SetFirstItem(lb3->GetSelection());
	}
	if (num_var >= 4) {
		lb4->SetSelection(lb4_cur_sel);
		lb4->SetFirstItem(lb4->GetSelection());
	}
}

void VariableSettingsDlg::FillData()
{
	col_ids.resize(num_var);
	var_info.resize(num_var);
	if (num_var >= 1) {
		v1_col_id = col_id_map[lb1->GetSelection()];
		v1_name = table_int->GetColName(v1_col_id);
		col_ids[0] = v1_col_id;
		var_info[0].time = v1_time;
	}
	if (num_var >= 2) {
		v2_col_id = col_id_map[lb2->GetSelection()];
		v2_name = table_int->GetColName(v2_col_id);
		col_ids[1] = v2_col_id;
		var_info[1].time = v2_time;
	}
	if (num_var >= 3) {
		v3_col_id = col_id_map[lb3->GetSelection()];
		v3_name = table_int->GetColName(v3_col_id);
		col_ids[2] = v3_col_id;
		var_info[2].time = v3_time;
	}
	if (num_var >= 4) {
		v4_col_id = col_id_map[lb4->GetSelection()];
		v4_name = table_int->GetColName(v4_col_id);
		col_ids[3] = v4_col_id;
		var_info[3].time = v4_time;
	}
	
	for (int i=0; i<num_var; i++) {
		// Set Primary GeoDaVarInfo attributes
		var_info[i].name = table_int->GetColName(col_ids[i]);
		var_info[i].is_time_variant = table_int->IsColTimeVariant(col_ids[i]);
		// var_info[i].time already set above
		table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	Gda::UpdateVarInfoSecondaryAttribs(var_info);
	//Gda::PrintVarInfoVector(var_info);
}

bool VariableSettingsDlg::FillSmoothedResults()
{
	std::vector<double> data;
	std::vector<bool> undefined;
	data.resize(table_int->GetNumberRows());
	for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) data[i] = 0;
	undefined.resize(table_int->GetNumberRows());
	
	col_ids.resize(num_var);
	var_info.resize(num_var);
	v1_col_id = col_id_map[lb1->GetSelection()];
	v1_name = table_int->GetColName(v1_col_id);
	col_ids[0] = v1_col_id;
	var_info[0].time = v1_time;
	
	int col1 = col_id_map[lb1->GetSelection()];
	table_int->GetColData(col1, v1_time, data);
	table_int->GetColUndefined(col1, v1_time, undefined);
	for (int i=0, iend=data.size(); i<iend; i++) {
		E[i] = undefined[i] ? 0.0 : data[i];
	}	

	
	v2_col_id = col_id_map[lb2->GetSelection()];
	v2_name = table_int->GetColName(v2_col_id);
	col_ids[1] = v2_col_id;
	var_info[1].time = v2_time;
	
	int col2 = col_id_map[lb2->GetSelection()];
	table_int->GetColData(col2, v2_time, data);
	table_int->GetColUndefined(col2, v2_time, undefined);
	for (int i=0, iend=data.size(); i<iend; i++) {
		P[i] = undefined[i] ? 0.0 : data[i];
	}
	
	for (int i=0; i<num_var; i++) {
		// Set Primary GeoDaVarInfo attributes
		var_info[i].name = table_int->GetColName(col_ids[i]);
		var_info[i].is_time_variant = table_int->IsColTimeVariant(col_ids[i]);
		// var_info[i].time already set above
		table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	Gda::UpdateVarInfoSecondaryAttribs(var_info);
	//Gda::PrintVarInfoVector(var_info);
	
	int num_obs = project->GetNumRecords();
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
			GdaAlgs::RateSmoother_SRS(num_obs, m_gal, P, E,
										smoothed_results, m_undef_r);
			break;
		case 2:
			GdaAlgs::RateSmoother_EBS(num_obs, P, E,
										smoothed_results, m_undef_r);
			break;
		case 3:
			GdaAlgs::RateSmoother_SEBS(num_obs, m_gal, P, E,
										 smoothed_results, m_undef_r);
			break;
		case 4:
			GdaAlgs::RateSmoother_RawRate(num_obs, P, E,
											smoothed_results, m_undef_r);
			break;
		case 5:
			GdaAlgs::RateSmoother_ExcessRisk(num_obs, P, E,
											   smoothed_results, m_undef_r);
			break;
		case 9:
			if (!GdaAlgs::RateStandardizeEB(num_obs, P, E,
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
