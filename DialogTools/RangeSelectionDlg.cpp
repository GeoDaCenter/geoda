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

#include <vector>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include "../GeoDaConst.h"
#include "../Project.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
#include "RangeSelectionDlg.h"

BEGIN_EVENT_TABLE( RangeSelectionDlg, wxDialog )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE"), RangeSelectionDlg::OnFieldChoice )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE_TM"),
			   RangeSelectionDlg::OnFieldChoiceTm )
	EVT_TEXT( XRCID("ID_MIN_TEXT"), RangeSelectionDlg::OnRangeTextChange )
	EVT_TEXT( XRCID("ID_MAX_TEXT"), RangeSelectionDlg::OnRangeTextChange )
    EVT_BUTTON( XRCID("ID_SEL_RANGE_BUTTON"),
			   RangeSelectionDlg::OnSelRangeClick )
	EVT_BUTTON( XRCID("ID_SEL_UNDEF_BUTTON"),
			   RangeSelectionDlg::OnSelUndefClick )
	EVT_BUTTON( XRCID("ID_INVERT_SEL_BUTTON"),
			   RangeSelectionDlg::OnInvertSelClick )
	EVT_BUTTON( XRCID("ID_RANDOM_SEL_BUTTON"),
			   RangeSelectionDlg::OnRandomSelClick )
	EVT_BUTTON( XRCID("ID_ADD_FIELD"), RangeSelectionDlg::OnAddField )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE"),
			   RangeSelectionDlg::OnSaveFieldChoice )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE_TM"),
			   RangeSelectionDlg::OnSaveFieldChoiceTm )
	EVT_CHECKBOX( XRCID("ID_SEL_CHECK_BOX"), RangeSelectionDlg::OnSelCheckBox )
	EVT_TEXT( XRCID("ID_SEL_VAL_TEXT"),
			 RangeSelectionDlg::OnSelUnselTextChange )
	EVT_CHECKBOX( XRCID("ID_UNSEL_CHECK_BOX"),
				 RangeSelectionDlg::OnUnselCheckBox )
	EVT_TEXT( XRCID("ID_UNSEL_VAL_TEXT"),
			 RangeSelectionDlg::OnSelUnselTextChange )
	EVT_BUTTON( XRCID("ID_APPLY_SAVE_BUTTON"),
			   RangeSelectionDlg::OnApplySaveClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), RangeSelectionDlg::OnCloseClick )
	
END_EVENT_TABLE()

RangeSelectionDlg::RangeSelectionDlg(Project* project_p, wxWindow* parent,
									 const wxString& title, const wxPoint& pos)
: project(project_p), grid_base(project_p->GetGridBase()),
current_sel_mcol(wxNOT_FOUND),
m_field_choice(0), m_min_text(0), m_field_static_txt(0), m_field2_static_txt(0),
m_max_text(0), m_sel_range_button(0), m_sel_undef_button(0),
m_invert_sel_button(0), m_random_sel_button(0),
m_save_field_choice(0), m_sel_check_box(0),
m_sel_val_text(0), m_unsel_check_box(0), m_unsel_val_text(0),
m_apply_save_button(0), m_all_init(false), m_selection_made(false),
is_space_time(project_p->GetGridBase()->IsTimeVariant())
{
	SetParent(parent);
    CreateControls();
	m_all_init = true;
	SetTitle(title);
    Centre();
	InitTime();
	FillColIdMap();
	InitField();
	InitSaveField();
	if (m_field_choice_tm) m_field_choice_tm->Disable();
	if (m_save_field_choice_tm) m_save_field_choice_tm->Disable();
}

void RangeSelectionDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_RANGE_SELECTION_DLG");
	m_field_choice = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE")),
								   wxChoice);

	m_field_choice_tm = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE_TM")),
									  wxChoice);
	
	m_min_text = wxDynamicCast(FindWindow(XRCID("ID_MIN_TEXT")),
							   wxTextCtrl);
	m_min_text->Clear();
	m_min_text->AppendText("0");
	m_min_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_field_static_txt = wxDynamicCast(FindWindow(XRCID("ID_FIELD_STATIC_TXT")),
									   wxStaticText);
	m_field2_static_txt =
		wxDynamicCast(FindWindow(XRCID("ID_FIELD2_STATIC_TXT")), wxStaticText);
	
	m_max_text = wxDynamicCast(FindWindow(XRCID("ID_MAX_TEXT")),
							   wxTextCtrl);
	m_max_text->Clear();
	m_max_text->AppendText("1");
	m_max_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_sel_range_button = wxDynamicCast(FindWindow(XRCID("ID_SEL_RANGE_BUTTON")),
									   wxButton);
	m_sel_range_button->Enable(false);
	
	m_sel_undef_button = wxDynamicCast(FindWindow(XRCID("ID_SEL_UNDEF_BUTTON")),
									   wxButton);
	m_sel_undef_button->Enable(false);

	m_invert_sel_button = wxDynamicCast(
						FindWindow(XRCID("ID_INVERT_SEL_BUTTON")), wxButton);
	
	m_random_sel_button = wxDynamicCast(
						FindWindow(XRCID("ID_RANDOM_SEL_BUTTON")), wxButton);
	
	m_save_field_choice =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE")), wxChoice);
	m_save_field_choice_tm =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM")), wxChoice);
	
	m_sel_check_box = wxDynamicCast(FindWindow(XRCID("ID_SEL_CHECK_BOX")),
									wxCheckBox); 
	
	m_sel_val_text = wxDynamicCast(FindWindow(XRCID("ID_SEL_VAL_TEXT")),
									wxTextCtrl);
	m_sel_val_text->Clear();
	m_sel_val_text->AppendText("1");
	m_sel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_unsel_check_box = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_CHECK_BOX")),
									  wxCheckBox); 
	
	m_unsel_val_text = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_VAL_TEXT")),
									 wxTextCtrl);	
	m_unsel_val_text->Clear();
	m_unsel_val_text->AppendText("0");
	m_unsel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_apply_save_button = wxDynamicCast(
		FindWindow(XRCID("ID_APPLY_SAVE_BUTTON")), wxButton);
	m_apply_save_button->Disable();
}

void RangeSelectionDlg::InitTime()
{
	if (!is_space_time) return;
	for (int t=0; t<grid_base->time_steps; t++) {
		wxString tm;
		tm << grid_base->time_ids[t];
		m_field_choice_tm->Append(tm);
		m_save_field_choice_tm->Append(tm);
	}
	m_field_choice_tm->SetSelection(0);
	m_field_choice_tm->Disable();
	m_save_field_choice_tm->SetSelection(0);
	m_save_field_choice_tm->Disable();
}

void RangeSelectionDlg::FillColIdMap()
{
	col_id_map.clear();
	grid_base->FillNumericColIdMap(col_id_map);
}

void RangeSelectionDlg::InitField()
{
	// assumes that FillColIdMap and InitTime has been called previously
	InitField(m_field_choice, m_field_choice_tm);
}

void RangeSelectionDlg::InitSaveField()
{
	// assumes that FillColIdMap and InitTime has been called previously
	InitField(m_save_field_choice, m_save_field_choice_tm);
}	

void RangeSelectionDlg::InitField(wxChoice* field, wxChoice* field_tm)
{
	// assumes that FillColIdMap and InitTime has been called previously
	field->Clear();
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		DbfColContainer& cd = *grid_base->col_data[col];
		if (field_tm && grid_base->IsColTimeVariant(col)) {
			wxString t;
			int t_sel = field_tm->GetSelection();
			t << " (" << grid_base->time_ids[t_sel] << ")";
			field->Append(cd.name + t);
		} else {
			field->Append(cd.name);
		}
	}
}

void RangeSelectionDlg::CheckRangeButtonSettings()
{
	if (!m_all_init) return;
	
	int fc = m_field_choice->GetSelection();
	bool valid_field = fc != wxNOT_FOUND;
	if (valid_field) {
		wxString fn = grid_base->col_data[col_id_map[fc]]->name;
		if (grid_base->col_data[col_id_map[fc]]->time_steps > 1) {
			wxString t;
			t << grid_base->time_ids[m_field_choice_tm->GetSelection()];
			fn << " (" << t << ")";
		}
		m_field_static_txt->SetLabelText(fn);
		m_field2_static_txt->SetLabelText(fn);
	} else {
		m_field_static_txt->SetLabelText("choose a variable");
		m_field2_static_txt->SetLabelText("choose a variable");
	}

	/** Check that min and max range text is valid.  If not valid, set
	 text color to red. */
	double val;
	wxString min_text = m_min_text->GetValue();
	bool min_valid = min_text.ToDouble(&val);
	{
		wxTextAttr style(m_min_text->GetDefaultStyle());
		style.SetTextColour(*(min_valid ? wxBLACK : wxRED));
		m_min_text->SetStyle(0, min_text.length(), style);
	}
	wxString max_text = m_max_text->GetValue();
	bool max_valid = max_text.ToDouble(&val);
	{
		wxTextAttr style(m_max_text->GetDefaultStyle());
		style.SetTextColour(*(max_valid ? wxBLACK : wxRED));
		m_max_text->SetStyle(0, max_text.length(), style);
	}
	
	m_sel_range_button->Enable(min_valid && max_valid && valid_field);
	m_sel_undef_button->Enable(valid_field);
}

void RangeSelectionDlg::OnFieldChoice( wxCommandEvent& event )
{
	EnableTimeField();
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnFieldChoiceTm( wxCommandEvent& event )
{
	if (!is_space_time) return;
	
	int prev_col = -1;
	if (m_field_choice->GetSelection() != wxNOT_FOUND) {
		prev_col = col_id_map[m_field_choice->GetSelection()];
	}
	
	InitField();
	
	if (prev_col != -1) {
		for (int i=0; i<col_id_map.size(); i++) {
			if (prev_col == col_id_map[i]) {
				m_field_choice->SetSelection(i);
			}
		}
	}
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnRangeTextChange( wxCommandEvent& event )
{
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnSelRangeClick( wxCommandEvent& event )
{
	LOG_MSG("Entering RangeSelectionDlg::OnApplySelClick");
	HighlightState& hs = *project->highlight_state;
	std::vector<bool>& h = hs.GetHighlight();
	std::vector<int>& nh = hs.GetNewlyHighlighted();
	std::vector<int>& nuh = hs.GetNewlyUnhighlighted();
	int nh_cnt = 0;
	int nuh_cnt = 0;
	if (m_field_choice->GetSelection() == wxNOT_FOUND) return;
	int mcol = col_id_map[m_field_choice->GetSelection()];
	int f_tm = 0;
	if (grid_base->col_data[mcol]->time_steps > 1) {
		f_tm = m_field_choice_tm->GetSelection();
	}
	DbfColContainer& cd = *grid_base->col_data[mcol];
	LOG(cd.name);
	double min_dval = 0;
	m_min_text->GetValue().ToDouble(&min_dval);
	double max_dval = 1;
	m_max_text->GetValue().ToDouble(&max_dval);
	std::vector<bool> undefined;
	cd.GetUndefined(undefined, f_tm);
	if (cd.type == GeoDaConst::long64_type) {
		wxInt64 min_ival = ceil(min_dval);
		wxInt64 max_ival = floor(max_dval);
		std::vector<wxInt64> vec;
		cd.GetVec(vec, f_tm);
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
			if (!undefined[i] && min_ival <= vec[i] && vec[i] <= max_ival) {
				if (!h[i]) nh[nh_cnt++]=i;
			} else {
				if (h[i]) nuh[nuh_cnt++]=i;
			}
		}
	} else if (cd.type == GeoDaConst::double_type) {
		std::vector<double> vec;
		cd.GetVec(vec, f_tm);
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
			if (!undefined[i] && min_dval <= vec[i] && vec[i] <= max_dval) {
				if (!h[i]) nh[nh_cnt++]=i;
			} else {
				if (h[i]) nuh[nuh_cnt++]=i;
			}
		}
	} else {
		wxString msg("Selected field is not a numeric type.  Please report "
					 "this bug.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (nh_cnt > 0 || nuh_cnt > 0) {
		hs.SetEventType(HighlightState::delta);
		hs.SetTotalNewlyHighlighted(nh_cnt);
		hs.SetTotalNewlyUnhighlighted(nuh_cnt);
		hs.notifyObservers();
	}
	current_sel_mcol = mcol;
	m_selection_made = true;
	CheckApplySaveSettings();
	LOG_MSG("Exiting RangeSelectionDlg::OnApplySelClick");
}

void RangeSelectionDlg::OnSelUndefClick( wxCommandEvent& event )
{
	HighlightState& hs = *project->highlight_state;
	hs.SetEventType(HighlightState::unhighlight_all);
	hs.notifyObservers();
	
	std::vector<bool>& h = hs.GetHighlight();
	std::vector<int>& nh = hs.GetNewlyHighlighted();
	std::vector<int>& nuh = hs.GetNewlyUnhighlighted();
	int nh_cnt = 0;
	int nuh_cnt = 0;
	if (m_field_choice->GetSelection() == wxNOT_FOUND) return;
	int mcol = col_id_map[m_field_choice->GetSelection()];
	int f_tm = 0;
	if (grid_base->col_data[mcol]->time_steps > 1) {
		f_tm = m_field_choice_tm->GetSelection();
	}
	DbfColContainer& cd = *grid_base->col_data[mcol];
	std::vector<bool> undefined;
	cd.GetUndefined(undefined, f_tm);
	for (int i=0, iend=h.size(); i<iend; i++) {
		if (undefined[i]) nh[nh_cnt++] = i;
	}
	if (nh_cnt > 0) {
		hs.SetEventType(HighlightState::delta);
		hs.SetTotalNewlyHighlighted(nh_cnt);
		hs.SetTotalNewlyUnhighlighted(nuh_cnt);
		hs.notifyObservers();
	}
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnRandomSelClick( wxCommandEvent& event )
{
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::uniform_01<boost::mt19937> X(rng);
	
	HighlightState& hs = *project->highlight_state;
	std::vector<bool>& h = hs.GetHighlight();
	std::vector<int>& nh = hs.GetNewlyHighlighted();
	std::vector<int>& nuh = hs.GetNewlyUnhighlighted();
	int nh_cnt = 0;
	int nuh_cnt = 0;
	int total_obs = h.size();
	for (int i=0; i<total_obs; i++) {
		bool sel = X() < 0.5;
		if (sel && !h[i]) {
			nh[nh_cnt++] = i;
		} else if (!sel && h[i]) {
			nuh[nuh_cnt++] = i;
		}
	}
	hs.SetEventType(HighlightState::delta);
	hs.SetTotalNewlyHighlighted(nh_cnt);
	hs.SetTotalNewlyUnhighlighted(nuh_cnt);
	hs.notifyObservers();
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnInvertSelClick( wxCommandEvent& event )
{
	HighlightState& hs = *project->highlight_state;
	hs.SetEventType(HighlightState::invert);
	hs.notifyObservers();
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnAddField( wxCommandEvent& event )
{
	int prev_field_col = -1;
	if (m_field_choice->GetSelection() != wxNOT_FOUND) {
		prev_field_col = col_id_map[m_field_choice->GetSelection()];
	}
	
	DataViewerAddColDlg dlg(grid_base, this, false, true, "SELECT");
	if (dlg.ShowModal() != wxID_OK) return;
	int col = dlg.GetColId();
	if (grid_base->GetColType(col) != GeoDaConst::long64_type &&
		grid_base->GetColType(col) != GeoDaConst::double_type) return;
	
	FillColIdMap();
	InitField();
	InitSaveField();
	
	for (int i=0; i<col_id_map.size(); i++) {
		if (prev_field_col == col_id_map[i]) {
			m_field_choice->SetSelection(i);
		}
	}
	
	for (int i=0; i<col_id_map.size(); i++) {
		if (col == col_id_map[i]) {
			m_save_field_choice->SetSelection(i);
		}
	}
	
	EnableTimeField();
	EnableTimeSaveField();
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSaveFieldChoice( wxCommandEvent& event )
{
	EnableTimeSaveField();
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSaveFieldChoiceTm( wxCommandEvent& event )
{
	if (!is_space_time) return;
	
	int prev_col = -1;
	if (m_save_field_choice->GetSelection() != wxNOT_FOUND) {
		prev_col = col_id_map[m_save_field_choice->GetSelection()];
	}
	
	InitSaveField();
	
	if (prev_col != -1) {
		for (int i=0; i<col_id_map.size(); i++) {
			if (prev_col == col_id_map[i]) {
				m_save_field_choice->SetSelection(i);
			}
		}
	}
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSelCheckBox( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnUnselCheckBox( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSelUnselTextChange( wxCommandEvent& event )
{
	CheckApplySaveSettings();
}

void RangeSelectionDlg::EnableTimeField()
{
	if (!is_space_time) return;
	if (m_field_choice->GetSelection() == wxNOT_FOUND) {
		m_field_choice_tm->Disable();
		return;
	}
	int col = col_id_map[m_field_choice->GetSelection()];
	m_field_choice_tm->Enable(grid_base->IsColTimeVariant(col));
}

void RangeSelectionDlg::EnableTimeSaveField()
{
	if (!is_space_time) return;
	if (m_save_field_choice->GetSelection() == wxNOT_FOUND) {
		m_save_field_choice_tm->Disable();
		return;
	}
	int col = col_id_map[m_save_field_choice->GetSelection()];
	m_save_field_choice_tm->Enable(grid_base->IsColTimeVariant(col));
}

void RangeSelectionDlg::CheckApplySaveSettings()
{
	if (!m_all_init) return;

	bool target_field_empty = m_save_field_choice->GetSelection()==wxNOT_FOUND;
	
	// Check that m_sel_val_text and m_unsel_val_text is valid.
	// If not valid, set text color to red.
	double val;
	wxString sel_text = m_sel_val_text->GetValue();
	bool sel_valid = sel_text.ToDouble(&val);
	{
		wxTextAttr style(m_sel_val_text->GetDefaultStyle());
		style.SetTextColour(*(sel_valid ? wxBLACK : wxRED));
		m_sel_val_text->SetStyle(0, sel_text.length(), style);
	}
	wxString unsel_text = m_unsel_val_text->GetValue();
	bool unsel_valid = unsel_text.ToDouble(&val);
	{
		wxTextAttr style(m_unsel_val_text->GetDefaultStyle());
		style.SetTextColour(*(unsel_valid ? wxBLACK : wxRED));
		m_unsel_val_text->SetStyle(0, unsel_text.length(), style);
	}
	
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
	
	m_apply_save_button->Enable(!target_field_empty &&
								(sel_checked || unsel_checked) &&
								((sel_checked && sel_valid) || !sel_checked) &&
								((unsel_checked && unsel_valid) ||
								 !unsel_checked) && m_selection_made);
}

/** The Apply button is only enable when Selected / Unselected values
 are valid (only when checked), and at least one checkbox is
 selected.  The Target Variable is not empty, but has not been
 checked for validity. */
void RangeSelectionDlg::OnApplySaveClick( wxCommandEvent& event )
{
	int write_col = col_id_map[m_save_field_choice->GetSelection()];
		
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
	
	double sel_c = 1;
	if (sel_checked) {
		wxString sel_c_str = m_sel_val_text->GetValue();
		sel_c_str.Trim(false); sel_c_str.Trim(true);
		sel_c_str.ToDouble(&sel_c);
	}
	double unsel_c = 0;
	if (unsel_checked) {
		wxString unsel_c_str = m_unsel_val_text->GetValue();
		unsel_c_str.Trim(false); unsel_c_str.Trim(true);
		unsel_c_str.ToDouble(&unsel_c);
	}
	
	int sf_tm = 0;
	if (grid_base->col_data[write_col]->time_steps > 1 &&
		m_save_field_choice_tm) {
		sf_tm = m_save_field_choice_tm->GetSelection();
	}
	
	std::vector<bool>& h = project->highlight_state->GetHighlight();
	// write_col now refers to a valid field in grid base, so write out
	// results to that field.
	int obs = h.size();
	DbfColContainer& cd = *grid_base->col_data[write_col];
	std::vector<bool> undefined;
	if (cd.type == GeoDaConst::long64_type) {
		wxInt64 sel_c_i = sel_c;
		wxInt64 unsel_c_i = unsel_c;
		std::vector<wxInt64> t(grid_base->GetNumberRows());
		cd.GetVec(t, sf_tm);
		cd.GetUndefined(undefined, sf_tm);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c_i;
					undefined[i] = false;
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c_i;
					undefined[i] = false;
				}
			}
		}
		cd.SetFromVec(t, sf_tm);
		cd.SetUndefined(undefined, sf_tm);
	} else if (cd.type == GeoDaConst::double_type) {
		std::vector<double> t(grid_base->GetNumberRows());
		cd.GetVec(t, sf_tm);
		cd.GetUndefined(undefined, sf_tm);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c;
					undefined[i] = false;
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c;
					undefined[i] = false;
				}
			}
		}
		cd.SetFromVec(t, sf_tm);
		cd.SetUndefined(undefined, sf_tm);
	} else {
		wxString msg = "Chosen field is not a numeric type.  This is likely ";
		msg << "a bug. Please report this.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	if (grid_base->GetView()) grid_base->GetView()->Refresh();
	wxString msg = "Values assigned to target field successfully.";
	wxMessageDialog dlg(this, msg, "Success", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void RangeSelectionDlg::OnCloseClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CLOSE);
}
