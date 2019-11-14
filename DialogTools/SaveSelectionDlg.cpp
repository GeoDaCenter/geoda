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

#include <wx/wx.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include "../GdaConst.h"
#include "../Project.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/GalWeight.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "SaveSelectionDlg.h"

BEGIN_EVENT_TABLE( SaveSelectionDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_ADD_FIELD"), SaveSelectionDlg::OnAddField )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE_TM"),
			   SaveSelectionDlg::OnSaveFieldChoiceTm )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE"),
				 SaveSelectionDlg::OnSaveFieldChoice )
	EVT_CHECKBOX( XRCID("ID_SEL_CHECK_BOX"), SaveSelectionDlg::OnSelCheckBox )
	EVT_TEXT( XRCID("ID_SEL_VAL_TEXT"),
			 SaveSelectionDlg::OnSelUnselTextChange )
	EVT_CHECKBOX( XRCID("ID_UNSEL_CHECK_BOX"),
				 SaveSelectionDlg::OnUnselCheckBox )
	EVT_TEXT( XRCID("ID_UNSEL_VAL_TEXT"),
			 SaveSelectionDlg::OnSelUnselTextChange )
	EVT_BUTTON( XRCID("ID_APPLY_SAVE_BUTTON"),
			   SaveSelectionDlg::OnApplySaveClick )
	EVT_BUTTON( XRCID("wxID_CANCEL"), SaveSelectionDlg::OnCancelClick )
END_EVENT_TABLE()

SaveSelectionDlg::SaveSelectionDlg(Project* project_s,
								   wxWindow* parent,
								   wxWindowID id,
								   const wxString& caption,
								   const wxPoint& pos,
								   const wxSize& size, long style )
: project(project_s), table_int(project_s->GetTableInt()),
m_all_init(false), is_space_time(project_s->GetTableInt()->IsTimeVariant()),
gal_weights(NULL)
{
    wxLogMessage("Open SaveSelectionDlg.");

    WeightsManInterface* w_man_int = project->GetWManInt();
    boost::uuids::uuid w_id = w_man_int->GetDefault();
    gal_weights = w_man_int->GetGal(w_id);

	SetParent(parent);
    CreateControls();
    Centre();
	InitTime();
	FillColIdMap();
	InitField();
	m_all_init = true;
}

void SaveSelectionDlg::CreateControls()
{
	if (project->GetTableInt()->IsTimeVariant()) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_SAVE_SELECTION_TM");
	} else {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "IDD_SAVE_SELECTION");
	}

	m_save_field_choice =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE")), wxChoice);
	m_save_field_choice_tm = 0;
	if (FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM"))) {
		m_save_field_choice_tm =
			wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM")),
											wxChoice);
	}

    m_inc_neighbors = wxDynamicCast(FindWindow(XRCID("ID_SEL_CHB_INCLUDE_NBRS")),
                                    wxCheckBox);
    if (gal_weights == NULL) {
        m_inc_neighbors->Hide();
        wxDynamicCast(FindWindow(XRCID("ID_TXT_CHB_INCLUDE_NBRS")),
                      wxStaticText)->Hide();
    }
	m_sel_check_box = wxDynamicCast(FindWindow(XRCID("ID_SEL_CHECK_BOX")),
									wxCheckBox); 
	
	m_sel_val_text = wxDynamicCast(FindWindow(XRCID("ID_SEL_VAL_TEXT")),
								   wxTextCtrl);

    Connect(XRCID("ID_SEL_VAL_TEXT"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(SaveSelectionDlg::OnApplySaveClick));
   
    m_save_sel_var_name = wxDynamicCast(FindWindow(XRCID("ID_SAVE_SEL_VAR_NAME")),
                                        wxTextCtrl);
    m_save_sel_var_name->SetValue("SELECTED");
    Connect(XRCID("ID_SAVE_SEL_VAR_NAME"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(SaveSelectionDlg::OnApplySaveClick));
    
	m_sel_val_text->Clear();
	m_sel_val_text->AppendText("1");
	m_sel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_unsel_check_box = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_CHECK_BOX")),
									  wxCheckBox); 
	
	m_unsel_val_text = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_VAL_TEXT")),
									 wxTextCtrl);
    
    Connect(XRCID("ID_UNSEL_VAL_TEXT"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(SaveSelectionDlg::OnApplySaveClick));
    
	m_unsel_val_text->Clear();
	m_unsel_val_text->AppendText("0");
	m_unsel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_apply_save_button = wxDynamicCast(
						FindWindow(XRCID("ID_APPLY_SAVE_BUTTON")), wxButton);
	//m_apply_save_button->Disable();
}

void SaveSelectionDlg::InitTime()
{
	if (!is_space_time) return;
	for (int t=0; t<project->GetTableInt()->GetTimeSteps(); t++) {
		wxString tm;
		tm << project->GetTableInt()->GetTimeString(t);
		m_save_field_choice_tm->Append(tm);
	}
	m_save_field_choice_tm->SetSelection(0);
	m_save_field_choice_tm->Disable();
}

void SaveSelectionDlg::FillColIdMap()
{
	col_id_map.clear();
	table_int->FillNumericColIdMap(col_id_map);
}

void SaveSelectionDlg::InitField()
{
	// assumes that FillColIdMap and InitTime has been called previously
	m_save_field_choice->Clear();
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		if (table_int->IsColTimeVariant(col)) {
			wxString t;
			int t_sel = m_save_field_choice_tm->GetSelection();
			t << " (" << project->GetTableInt()->GetTimeString(t_sel) << ")";
			m_save_field_choice->Append(table_int->GetColName(col) + t);
		} else {
			m_save_field_choice->Append(table_int->GetColName(col));
		}
	}
}


void SaveSelectionDlg::OnAddField( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnAddField");
    
	DataViewerAddColDlg dlg(project, this, false, true, "SELECT", GdaConst::long64_type);
	if (dlg.ShowModal() != wxID_OK) return;
	int col = dlg.GetColId();
	if (table_int->GetColType(col) != GdaConst::long64_type &&
		table_int->GetColType(col) != GdaConst::double_type) return;

	FillColIdMap();
	InitField();
	
	for (int i=0; i<col_id_map.size(); i++) {
		if (col == col_id_map[i]) {
			m_save_field_choice->SetSelection(i);
		}
	}
	
	EnableTimeField();
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSaveFieldChoice( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnSaveFieldChoice");
    
	EnableTimeField();
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSaveFieldChoiceTm( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnSaveFieldChoiceTm");
    
	if (!is_space_time) return;
	
	int prev_col = -1;
	if (m_save_field_choice->GetSelection() != wxNOT_FOUND) {
		prev_col = col_id_map[m_save_field_choice->GetSelection()];
	}
	
	InitField();
	
	if (prev_col != -1) {
		for (int i=0; i<col_id_map.size(); i++) {
			if (prev_col == col_id_map[i]) {
				m_save_field_choice->SetSelection(i);
			}
		}
	}
	CheckApplySaveSettings();
}

void SaveSelectionDlg::EnableTimeField()
{
	if (!is_space_time) return;
	if (m_save_field_choice->GetSelection() == wxNOT_FOUND) {
		m_save_field_choice_tm->Disable();
		return;
	}
	int col = col_id_map[m_save_field_choice->GetSelection()];
	m_save_field_choice_tm->Enable(table_int->IsColTimeVariant(col));
}

void SaveSelectionDlg::OnSelCheckBox( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnSelCheckBox");
    
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnUnselCheckBox( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnUnselCheckBox");
    
	CheckApplySaveSettings();
}

void SaveSelectionDlg::OnSelUnselTextChange( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnSelUnselTextChange");
    
	CheckApplySaveSettings();
}

void SaveSelectionDlg::CheckApplySaveSettings()
{
	if (!m_all_init) return;
	
	//bool target_field_empty = m_save_field_choice->GetSelection()==wxNOT_FOUND;
    bool target_field_empty =m_save_sel_var_name->GetValue().IsEmpty();
    
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
								 !unsel_checked));

}

/** The Apply button is only enable when Selected / Unselected values
 are valid (only when checked), and at least one checkbox is
 selected.  The Target Variable is not empty, but has not been
 checked for validity. */
void SaveSelectionDlg::OnApplySaveClick( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnApplySaveClick");
    
    wxString field_name = m_save_sel_var_name->GetValue();
    if (field_name.empty()) {
        wxMessageDialog dlg(this, _("Variable name can't be empty."),
                            _("Error"), wxOK | wxICON_ERROR );
        dlg.ShowModal();
        return;
    }
    
    int col = table_int->FindColId(field_name);
    
    if ( col == wxNOT_FOUND) {
        // create a new integer field
        int col_insert_pos = table_int->GetNumberCols();
        int time_steps = 1;
        int m_length_val = GdaConst::default_dbf_long_len;
        int m_decimals_val = 0;
        col = table_int->InsertCol(GdaConst::long64_type, field_name, col_insert_pos, time_steps, m_length_val, m_decimals_val);
    }
   
    if (col <= 0) {
        return;
    }
    
    int write_col = col;
	
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
	if (table_int->GetColTimeSteps(write_col) > 1 &&
		m_save_field_choice_tm) {
		sf_tm = m_save_field_choice_tm->GetSelection();
	}

    bool with_neighbors = false;
    if (gal_weights != NULL) {
        if (m_inc_neighbors->IsChecked()) {
            with_neighbors = true;
        }
    }

	std::vector<bool>& h = project->GetHighlightState()->GetHighlight();
	// write_col now refers to a valid field in grid base, so write out
	// results to that field.
	int obs = h.size();
	std::vector<bool> undefined, selected(obs, false);
	if (table_int->GetColType(write_col) == GdaConst::long64_type) {
		wxInt64 sel_c_i = sel_c;
		wxInt64 unsel_c_i = unsel_c;
		std::vector<wxInt64> t(table_int->GetNumberRows());
		table_int->GetColData(write_col, sf_tm, t);
		table_int->GetColUndefined(write_col, sf_tm, undefined);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c_i;
                    selected[i] = true;
					undefined[i] = false;
                    // add neighbors
                    if (gal_weights && with_neighbors) {
                        GalElement& e = gal_weights->gal[i];
                        for (int j=0, jend=e.Size(); j<jend; j++) {
                            int obs = e[j];
                            t[obs] = sel_c_i;
                            selected[obs] = true;
                            undefined[obs] = false;
                        }
                    }
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (selected[i] == false) {
					t[i] = unsel_c_i;
					undefined[i] = false;
				}
			}
		}
		table_int->SetColData(write_col, sf_tm, t);
		table_int->SetColUndefined(write_col, sf_tm, undefined);
	} else if (table_int->GetColType(write_col) == GdaConst::double_type) {
		std::vector<double> t(table_int->GetNumberRows());
		table_int->GetColData(write_col, sf_tm, t);
		table_int->GetColUndefined(write_col, sf_tm, undefined);
		if (sel_checked) {
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c;
                    selected[i] = true;
					undefined[i] = false;
                    // add neighbors
                    if (gal_weights && with_neighbors) {
                        GalElement& e = gal_weights->gal[i];
                        for (int j=0, jend=e.Size(); j<jend; j++) {
                            int obs = e[j];
                            t[obs] = sel_c;
                            selected[obs] = true;
                            undefined[obs] = false;
                        }
                    }
				}
			}
		}
		if (unsel_checked) {
			for (int i=0; i<obs; i++) {
				if (selected[i] == false) {
					t[i] = unsel_c;
					undefined[i] = false;
				}
			}
		}
		table_int->SetColData(write_col, sf_tm, t);
		table_int->SetColUndefined(write_col, sf_tm, undefined);
	} else {
		wxString msg = _("Chosen field is not a numeric type.  Please select a numeric type field.");

		wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	wxString msg = _("Values assigned to target field successfully.");
	wxMessageDialog dlg(this, msg, "Success", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
	OnCancelClick(event);
}

void SaveSelectionDlg::OnCancelClick( wxCommandEvent& event )
{
    wxLogMessage("Click SaveSelectionDlg::OnCancelClick");
    
	event.Skip();
	EndDialog(wxID_CANCEL);
}
