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

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include <wx/regex.h>
#include "TableInterface.h"
#include "TimeState.h"
#include "../Project.h"
#include "../logger.h"
#include "../DbfFile.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "TableInterface.h"
#include "DataViewerAddColDlg.h"

BEGIN_EVENT_TABLE( DataViewerAddColDlg, wxDialog )
	EVT_CHOICE( XRCID("ID_CHOICE_TYPE"), DataViewerAddColDlg::OnChoiceType )
    EVT_BUTTON( XRCID("wxID_OK"), DataViewerAddColDlg::OnOkClick )
	EVT_TEXT( XRCID("ID_TEXT_NEW_NAME"), DataViewerAddColDlg::OnEditName )
	EVT_TEXT( XRCID("ID_TEXT_LENGTH"), DataViewerAddColDlg::OnEditLength )
	EVT_TEXT( XRCID("ID_TEXT_DECIMALS"), DataViewerAddColDlg::OnEditDecimals )
	EVT_CHOICE( XRCID("ID_DISPLAYED_DECIMALS"),
			 DataViewerAddColDlg::OnChoiceDisplayedDecimals )
END_EVENT_TABLE()

DataViewerAddColDlg::
DataViewerAddColDlg(Project* project_s,
                    wxWindow* parent,
                    bool time_variant_no_as_default_s,
                    bool can_change_time_variant_s,
                    wxString default_name_s,
                    GdaConst::FieldType default_field_type_s)
: project(project_s),
table_int(project_s->GetTableInt()),
time_variant_no_as_default(time_variant_no_as_default_s),
can_change_time_variant(can_change_time_variant_s),
default_name(default_name_s),
default_field_type(default_field_type_s),
m_decimals_val(0), m_length_valid(true),
time_variant(project_s->GetTableInt()->IsTimeVariant()),
fixed_lengths(project_s->GetTableInt()->HasFixedLengths())
{
    wxLogMessage("Open DataViewerAddColDlg.");
    
	SetParent(parent);
    CreateControls();
    Centre();
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	
	table_int->FillColIdMap(col_id_map);
    
	for (int i=0, iend=table_int->GetNumberCols(); i<iend; i++) {
		curr_col_labels.insert(table_int->GetColName(i).Upper());
		m_insert_pos->Append(table_int->GetColName(i).Upper());
	}
    
	m_insert_pos->Append("after last variable");
	m_insert_pos->SetSelection(0);
     
	UpdateApplyButton();
}


void DataViewerAddColDlg::CreateControls()
{
    SetBackgroundColour(*wxWHITE);
	if (time_variant && fixed_lengths) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_DATA_VIEWER_ADD_COL_TIME_FIXED_DLG");
	} else if (time_variant && !fixed_lengths) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_DATA_VIEWER_ADD_COL_TIME_DLG");
	} else if (!time_variant && fixed_lengths) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_DATA_VIEWER_ADD_COL_FIXED_DLG");
	} else { // !time_variant && !fixed_lengths 
		wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_DATA_VIEWER_ADD_COL_DLG");
	}
	m_apply_button = wxDynamicCast(FindWindow(XRCID("wxID_OK")), wxButton);
	
	m_name = wxDynamicCast(FindWindow(XRCID("ID_TEXT_NEW_NAME")), wxTextCtrl);
	m_name->SetValue(default_name);
	wxString name_chars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
	wxTextValidator name_validator(wxFILTER_INCLUDE_CHAR_LIST);
	name_validator.SetCharIncludes(name_chars);
	m_name->SetValidator(name_validator);
	m_name_valid = false;
	
	m_time_variant_no = 0;
	m_time_variant_yes = 0;
	if (FindWindow(XRCID("ID_TIME_VARIANT_NO"))) {
		m_time_variant_no = wxDynamicCast(FindWindow(XRCID("ID_TIME_VARIANT_NO")), wxRadioButton);
		m_time_variant_yes = wxDynamicCast(FindWindow(XRCID("ID_TIME_VARIANT_YES")), wxRadioButton);
		m_time_variant_no->SetValue(time_variant_no_as_default);
		m_time_variant_yes->SetValue(!time_variant_no_as_default);
		m_time_variant_no->Enable(can_change_time_variant);
		m_time_variant_yes->Enable(can_change_time_variant);
	}
	
	m_type = wxDynamicCast(FindWindow(XRCID("ID_CHOICE_TYPE")), wxChoice);
	// add options for Float, Integer, String, or Date
	m_type->Append("real (eg 1.03, 45.7)");
	m_type->Append("integer (eg -1, 0, 23)");
	m_type->Append("string (eg New York)");
	//m_type->Append("date (eg 20110131)");
	
	wxStaticText* mt = wxDynamicCast(FindWindow(XRCID("ID_STATIC_INSERT_POS")), wxStaticText);
	m_insert_pos = wxDynamicCast(FindWindow(XRCID("ID_CHOICE_INSERT_POS")), wxChoice);
    
    if ( !project->IsFileDataSource()) {
        mt->Disable();
        m_insert_pos->Disable();
    }

	m_displayed_decimals_lable =  wxDynamicCast(FindWindow(XRCID("ID_STATIC_DISPLAYED_DECIMALS")), wxStaticText);
	m_displayed_decimals = wxDynamicCast(FindWindow(XRCID("ID_DISPLAYED_DECIMALS")), wxChoice);
	m_displayed_decimals->Append("default");
	m_displayed_decimals->Append("1");
	m_displayed_decimals->Append("2");
	m_displayed_decimals->Append("3");
	m_displayed_decimals->Append("4");
	m_displayed_decimals->Append("5");
	m_displayed_decimals->Append("6");
	m_displayed_decimals->Append("7");
	m_displayed_decimals->Append("8");
	m_displayed_decimals->Append("9");
	m_displayed_decimals->Append("10");
	
	if (fixed_lengths) {
		m_length_lable = wxDynamicCast(FindWindow(XRCID("ID_STATIC_LENGTH")), wxStaticText);
		m_length = wxDynamicCast(FindWindow(XRCID("ID_TEXT_LENGTH")), wxTextCtrl);
		m_length->SetValidator(wxTextValidator(wxFILTER_DIGITS));
		m_length_valid = true;

		m_decimals_lable = wxDynamicCast(FindWindow(XRCID("ID_STATIC_DECIMALS")), wxStaticText);
		m_decimals = wxDynamicCast(FindWindow(XRCID("ID_TEXT_DECIMALS")), wxTextCtrl);
		m_decimals->SetValidator(wxTextValidator(wxFILTER_DIGITS));
		m_decimals_valid = true;
	
		m_max_label = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MAX_LABEL")), wxStaticText);
		m_max_val = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MAX_VAL")),  wxStaticText);

		m_min_label = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MIN_LABEL")), wxStaticText);
		m_min_val = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MIN_VAL")), wxStaticText);
	}
	
	if (default_field_type == GdaConst::double_type) {
		m_type->SetSelection(0);
	} else if (default_field_type == GdaConst::long64_type) {
		m_type->SetSelection(1);
	} else if (default_field_type == GdaConst::string_type) {
		m_type->SetSelection(2);
	} else if (default_field_type == GdaConst::date_type) {
		m_type->SetSelection(3);
	} else {
		m_type->SetSelection(0);
	}

    
    Connect(XRCID("ID_TEXT_NEW_NAME"), wxEVT_COMMAND_TEXT_ENTER,wxCommandEventHandler(DataViewerAddColDlg::OnOkClick));
    Connect(XRCID("ID_TEXT_LENGTH"), wxEVT_COMMAND_TEXT_ENTER,wxCommandEventHandler(DataViewerAddColDlg::OnOkClick));
    Connect(XRCID("ID_TEXT_DECIMALS"), wxEVT_COMMAND_TEXT_ENTER,wxCommandEventHandler(DataViewerAddColDlg::OnOkClick));
    
	CheckName();
	SetDefaultsByType(default_field_type);
}

void DataViewerAddColDlg::OnChoiceType( wxCommandEvent& ev )
{
    wxLogMessage("In DataViewerAddColDlg::OnChoiceType()");
    
	switch (ev.GetSelection()) {
		case 0:
			if (cur_type == GdaConst::double_type) return;
			SetDefaultsByType(GdaConst::double_type);
			break;
		case 1:
			if (cur_type == GdaConst::long64_type) return;
			SetDefaultsByType(GdaConst::long64_type);
			break;
		case 2:
			if (cur_type == GdaConst::string_type) return;
			SetDefaultsByType(GdaConst::string_type);
			break;
		case 3:
			if (cur_type == GdaConst::date_type) return;
			SetDefaultsByType(GdaConst::date_type);
			break;
		default:
			// should never get here!
			SetDefaultsByType(GdaConst::double_type);
	}
	
}

void DataViewerAddColDlg::SetDefaultsByType(GdaConst::FieldType type)
{
	// set some defaults first
	m_displayed_decimals->SetSelection(0);
	m_displayed_decimals->Disable();
	if (fixed_lengths) {
		m_length->Enable();
		m_decimals_lable->SetLabelText("Decimals");
		m_decimals->SetValue("7");
		m_decimals->Disable();
		m_max_label->SetLabelText("");
		m_max_val->SetLabelText("");
		m_min_label->SetLabelText("");
		m_min_val->SetLabelText("");
		m_length_valid = true;
		m_decimals_valid = true;
	}
	
	switch (type) {
		case GdaConst::double_type:
		{
			cur_type = GdaConst::double_type;
			m_displayed_decimals->Enable();
			m_displayed_decimals->SetSelection(0);
			if (fixed_lengths) {
				m_length_lable->SetLabelText(wxString::Format("Length (max %d)", GdaConst::max_dbf_double_len));
				m_length->SetValue(wxString::Format("%d", GdaConst::default_dbf_double_len));
				m_decimals_lable->SetLabelText(wxString::Format("Decimals (max %d)", GdaConst::max_dbf_double_decimals));
				m_decimals->Enable();
				m_decimals->SetValue(wxString::Format("%d", GdaConst::default_dbf_double_decimals));

				m_max_label->SetLabelText("maximum");
				m_max_val->SetLabelText(wxString::Format("%d", 9));
				m_min_label->SetLabelText("minimum");
				m_min_val->SetLabelText(wxString::Format("-%d", 9));
			}
		}
			break;
		case GdaConst::long64_type:
		{
			cur_type = GdaConst::long64_type;
			if (fixed_lengths) {
				m_length_lable->SetLabelText(wxString::Format("Length (max %d)", GdaConst::max_dbf_long_len));
				m_length->SetValue(wxString::Format("%d", GdaConst::default_dbf_long_len));
				m_max_label->SetLabelText("maximum");
				m_max_val->SetLabelText(wxString::Format("%d", 9));
				m_min_label->SetLabelText("minimum");
				m_min_val->SetLabelText(wxString::Format("-%d", 9));
			}
		}
			break;
		case GdaConst::string_type:
		{
			cur_type = GdaConst::string_type;
			if (fixed_lengths) {
				m_length_lable->SetLabelText(wxString::Format("Length (max %d)", GdaConst::max_dbf_string_len));
				m_length->SetValue(wxString::Format("%d", GdaConst::default_dbf_string_len));
			}
		}
			break;
		case GdaConst::date_type:
		default:
		{
			cur_type = GdaConst::date_type;
			if (fixed_lengths) {
				m_length_lable->SetLabelText("Length");
				m_length->SetValue(wxString::Format("%d", GdaConst::default_dbf_date_len));
				m_length->Disable();
			}
		}
	}
	if (fixed_lengths) UpdateMinMaxValues();
}

void DataViewerAddColDlg::OnOkClick( wxCommandEvent& ev )
{
	wxLogMessage("Entering DataViewerAddColDlg::OnOkClick");
    wxString colname = m_name->GetValue();
	colname.Trim(true);  // trim white-space from right of string
	colname.Trim(false); // trim white-space from left of string
	
	if (colname == wxEmptyString) {
		wxString msg("Error: The table variable name is empty.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (curr_col_labels.find(colname.Upper()) != curr_col_labels.end()) {
		wxString msg;
		msg += "Error: \"" + colname.Upper();
		msg += "\" already exists in Table, please specify a different name.";
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
    bool m_name_valid = table_int->IsValidDBColName(colname);
	//if ( !DbfFileUtils::isValidFieldName(colname) ) {
    if (!m_name_valid) {
		wxString msg;
		msg += "Error: \"" + colname + "\" is an invalid variable name. The first character must be alphabetic, and the remaining characters can be either alphanumeric or underscores. For DBF table, a valid variable name is between one and ten characters long.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	long displayed_decimals = m_displayed_decimals->GetSelection();
	if (displayed_decimals == 0) displayed_decimals = -1;

	if (fixed_lengths) {
		// Note: before we get to this point, all parameters are assumed
		// to be valid, except for double_type, where we need to check
		// that the particular combination of decimals and length is valid
	
		if (cur_type == GdaConst::double_type) {
			int suggest_len;
			int suggest_dec;
			DbfFileUtils::SuggestDoubleParams(m_length_val, m_decimals_val,
											  &suggest_len, &suggest_dec);
			if (m_length_val != suggest_len || m_decimals_val != suggest_dec) {
				wxString m;
				m << "Error: Due to restrictions on the DBF file format, the ";
				m << "particular combination of length and decimals entered is ";
				m << "not valid.  Based on your current choices, we recommend ";
				m << "length = " << suggest_len;
				m << " and decimals = " << suggest_dec;
				wxMessageDialog dlg(this, m, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				return;
			}
		}
	}
	
	int col_insert_pos;
    
    if ( m_insert_pos->IsEnabled() ) {
        if (m_insert_pos->GetSelection() >= table_int->GetNumberCols()) {
            col_insert_pos = table_int->GetNumberCols();
        } else {
            // One might think we want to use the col_id_map here, but it
            // turns out we need to specify the position in the visual
            // order it appears in the Table.
            col_insert_pos = m_insert_pos->GetSelection();
        }
    } else {
        col_insert_pos = table_int->GetNumberCols();
    }

	int time_steps = 1; // non-space-time column by default	
	if (m_time_variant_yes && m_time_variant_yes->GetValue()) {
		time_steps = table_int->GetTimeSteps();
	}

	wxLogMessage(wxString::Format("Inserting new column %s into Table", colname.Upper()));
	
	bool success;
	if (fixed_lengths) {
		success = (table_int->InsertCol(cur_type, colname.Upper(),
									   col_insert_pos, time_steps,
									   m_length_val, m_decimals_val) != -1);
	} else {
		success = (table_int->InsertCol(cur_type, colname.Upper(),
									   col_insert_pos, time_steps) != -1);
	}
	
	if (!success) {
		wxString msg = _("Could not create a new variable. Possibly a read-only data source.");
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	final_col_name = colname.Upper();
	final_col_id = col_insert_pos;
    

	if (table_int->PermitChangeDisplayedDecimals()) {
		table_int->ColChangeDisplayedDecimals(final_col_id, displayed_decimals);
	}
    
	wxGrid* g = project->FindTableGrid();
    if (g) {
        g->GoToCell(1, col_insert_pos);
    }
    
	ev.Skip();
	EndDialog(wxID_OK);
}

void DataViewerAddColDlg::OnEditName( wxCommandEvent& ev )
{
    wxLogMessage("DataViewerAddColDlg::OnEditName()");
	CheckName();
}

void DataViewerAddColDlg::CheckName()
{
	wxString s = m_name->GetValue();
    m_name_valid = table_int->IsValidDBColName(s);
    if ( m_name_valid ) {
        m_name->SetForegroundColour(*wxBLACK);
	} else {
        m_name->SetForegroundColour(*wxRED);
	}
	
	UpdateApplyButton();	
}

void DataViewerAddColDlg::OnEditLength( wxCommandEvent& ev )
{
    wxLogMessage("DataViewerAddColDlg::OnEditLength()");
    
	if (!fixed_lengths) return;
	wxString s = m_length->GetValue();
	long val = 0;
	bool is_num = s.ToLong(&val);
	m_length_val = val;
	
	long min_v;
	long max_v;
	if (cur_type == GdaConst::date_type) {
		min_v = GdaConst::min_dbf_date_len;
		max_v = GdaConst::max_dbf_date_len;
	} else if (cur_type == GdaConst::long64_type) {
		min_v = GdaConst::min_dbf_long_len;
		max_v = GdaConst::max_dbf_long_len;
	} else if (cur_type == GdaConst::double_type) {
		min_v = GdaConst::min_dbf_double_len;
		max_v = GdaConst::max_dbf_double_len;
	} else  { // cur_type == GdaConst::string_type
		min_v = GdaConst::min_dbf_string_len;
		max_v = GdaConst::max_dbf_string_len;
	}
	
	if ((is_num && val >= min_v && val <= max_v) || s.length() == 0) {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxBLACK);
		m_name->SetStyle(0, s.length(), style);
	} else {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxRED);
		m_name->SetStyle(0, s.length(), style);
	}
	m_length_valid = is_num && val >= min_v && val <= max_v;
	UpdateMinMaxValues();
	UpdateApplyButton();
}

void DataViewerAddColDlg::OnEditDecimals( wxCommandEvent& ev )
{
    wxLogMessage("DataViewerAddColDlg::OnEditDecimals()");
    
	if (!fixed_lengths)
        return;
	if (!cur_type == GdaConst::double_type) {
		m_decimals_valid = true;
		UpdateApplyButton();
		return;
	}
	wxString s = m_decimals->GetValue();
	long val = 0;
	bool is_num = s.ToLong(&val);
	m_decimals_val = val;
	long min_v = GdaConst::min_dbf_double_decimals;
	long max_v = GdaConst::max_dbf_double_decimals;
		
	if ((is_num && val >= min_v && val <= max_v) || s.length() == 0) {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxBLACK);
		m_name->SetStyle(0, s.length(), style);
	} else {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxRED);
		m_name->SetStyle(0, s.length(), style);
	}
	m_decimals_valid = is_num && val >= min_v && val <= max_v;
	UpdateMinMaxValues();
	UpdateApplyButton();
}

void DataViewerAddColDlg::OnChoiceDisplayedDecimals( wxCommandEvent& ev )
{
    wxLogMessage("DataViewerAddColDlg::OnChoiceDisplayedDecimals()");
    
	if (fixed_lengths && cur_type == GdaConst::double_type)
        UpdateMinMaxValues();
    
	UpdateApplyButton();
}

void DataViewerAddColDlg::UpdateMinMaxValues()
{
	if (!fixed_lengths) return;

    m_max_val->SetLabelText("");
	m_min_val->SetLabelText("");
	
    if (!m_length_valid || !m_decimals_valid ||
		!(cur_type == GdaConst::double_type ||
		  cur_type == GdaConst::long64_type)) {
		return;
	}
	
	long length = 8;
	m_length->GetValue().ToCLong(&length);
	if (cur_type == GdaConst::double_type) {
		long decimals = 0;
		m_decimals->GetValue().ToLong(&decimals);
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(length, decimals, &suggest_len, &suggest_dec);
		if (length == suggest_len && decimals == suggest_dec) {
			m_max_val->SetLabelText(DbfFileUtils::GetMaxDoubleString(length, decimals));
			m_min_val->SetLabelText(DbfFileUtils::GetMinDoubleString(length, decimals));
		}
	} else { // cur_type == GdaConst::long64_type
		m_max_val->SetLabelText(DbfFileUtils::GetMaxIntString(length));
		m_min_val->SetLabelText(DbfFileUtils::GetMinIntString(length));
	}
}

void DataViewerAddColDlg::UpdateApplyButton()
{
	if (fixed_lengths) {
		bool enable = false;
		if (cur_type == GdaConst::double_type) {
			enable = (m_name_valid && m_length_valid &&
					  m_decimals_valid &&
					  m_length_val > m_decimals_val);
		} else {
			enable = (m_name_valid && m_length_valid);
		}
		m_apply_button->Enable(enable);
	} else {
		m_apply_button->Enable(m_name_valid);
	}
}

wxString DataViewerAddColDlg::GetColName()
{
	return final_col_name;
}

int DataViewerAddColDlg::GetColId()
{
	return final_col_id;
}
