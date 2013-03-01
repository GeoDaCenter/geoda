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

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include <wx/regex.h>
#include "DbfGridTableBase.h"
#include "../logger.h"
#include "../ShapeOperations/DbfFile.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GeoDaConst.h"
#include "../GenUtils.h"
#include "DataViewerAddColDlg.h"

BEGIN_EVENT_TABLE( DataViewerAddColDlg, wxDialog )
	EVT_CHOICE( XRCID("ID_CHOICE_TYPE"), DataViewerAddColDlg::OnChoiceType )
    EVT_BUTTON( XRCID("wxID_OK"), DataViewerAddColDlg::OnOkClick )
	EVT_TEXT( XRCID("ID_TEXT_NEW_NAME"), DataViewerAddColDlg::OnEditName )
	EVT_TEXT( XRCID("ID_TEXT_LENGTH"), DataViewerAddColDlg::OnEditLength )
	EVT_TEXT( XRCID("ID_TEXT_DECIMALS"), DataViewerAddColDlg::OnEditDecimals )
	EVT_TEXT( XRCID("ID_TEXT_DISPLAYED_DECIMALS"),
			 DataViewerAddColDlg::OnEditDisplayedDecimals )
END_EVENT_TABLE()

DataViewerAddColDlg::DataViewerAddColDlg(DbfGridTableBase* grid_base_s,
										 wxWindow* parent,
										 bool time_inv_no_as_default_s,
										 bool can_change_time_inv_s,
										 wxString default_name_s,
						GeoDaConst::FieldType default_field_type_s)
: grid_base(grid_base_s), time_inv_no_as_default(time_inv_no_as_default_s),
can_change_time_inv(can_change_time_inv_s), default_name(default_name_s),
default_field_type(default_field_type_s)
{
	SetParent(parent);
    CreateControls();
    Centre();
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	
	grid_base->FillColIdMap(col_id_map);
	for (int i=0, iend=grid_base->col_data.size(); i<iend; i++) {
		curr_col_labels.insert(grid_base->col_data[i]->name.Upper());
	}
	
	for (int i=0; i<grid_base->GetNumberCols(); i++) {
		m_insert_pos->Append(grid_base->col_data[col_id_map[i]]->name.Upper());
	}
	m_insert_pos->Append("end of table");
	m_insert_pos->SetSelection(0);
}


void DataViewerAddColDlg::CreateControls()
{
	if (grid_base->IsTimeVariant()) {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_DATA_VIEWER_ADD_COL_TIME_DLG");
	} else {
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_DATA_VIEWER_ADD_COL_DLG");
	}
	m_apply_button = wxDynamicCast(FindWindow(XRCID("wxID_OK")), wxButton);
	
	m_name = wxDynamicCast(FindWindow(XRCID("ID_TEXT_NEW_NAME")), wxTextCtrl);
	m_name->SetValue(default_name);
	wxString name_chars="abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
	wxTextValidator name_validator(wxFILTER_INCLUDE_CHAR_LIST);
	name_validator.SetCharIncludes(name_chars);
	m_name->SetValidator(name_validator);
	m_name_valid = false;
	
	m_time_inv_no = 0;
	m_time_inv_yes = 0;
	if (FindWindow(XRCID("ID_TIME_INV_NO"))) {
		m_time_inv_no = wxDynamicCast(FindWindow(XRCID("ID_TIME_INV_NO")),
									  wxRadioButton);
		m_time_inv_yes = wxDynamicCast(FindWindow(XRCID("ID_TIME_INV_YES")),
									  wxRadioButton);
		m_time_inv_no->SetValue(time_inv_no_as_default);
		m_time_inv_yes->SetValue(!time_inv_no_as_default);
		m_time_inv_no->Enable(can_change_time_inv);
		m_time_inv_yes->Enable(can_change_time_inv);
	}
	
	m_type = wxDynamicCast(FindWindow(XRCID("ID_CHOICE_TYPE")), wxChoice);
	// add options for Float, Integer, String, or Date
	m_type->Append("real (eg 1.03, 45.7)");
	m_type->Append("integer (eg -1, 0, 23)");
	m_type->Append("string (eg New York)");
	m_type->Append("date (eg 20110131)");
	
	m_insert_pos = wxDynamicCast(FindWindow(XRCID("ID_CHOICE_INSERT_POS")),
								 wxChoice);
	
	m_length_lable = wxDynamicCast(FindWindow(XRCID("ID_STATIC_LENGTH")),
								   wxStaticText);
	m_length = wxDynamicCast(FindWindow(XRCID("ID_TEXT_LENGTH")), wxTextCtrl);
	m_length->SetValidator(wxTextValidator(wxFILTER_DIGITS));
	m_length_valid = true;
	
	m_decimals_lable = wxDynamicCast(FindWindow(XRCID("ID_STATIC_DECIMALS")),
									 wxStaticText);
	m_decimals = wxDynamicCast(FindWindow(XRCID("ID_TEXT_DECIMALS")),
							   wxTextCtrl);
	m_decimals->SetValidator(wxTextValidator(wxFILTER_DIGITS));
	m_decimals_valid = true;
	
	m_displayed_decimals_lable = 
		wxDynamicCast(FindWindow(XRCID("ID_STATIC_DISPLAYED_DECIMALS")),
					  wxStaticText);
	m_displayed_decimals =
		wxDynamicCast(FindWindow(XRCID("ID_TEXT_DISPLAYED_DECIMALS")),
					  wxTextCtrl);
	m_displayed_decimals->SetValidator(wxTextValidator(wxFILTER_DIGITS));
	m_displayed_decimals_valid = true;
	
	m_max_label = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MAX_LABEL")),
								wxStaticText);
	m_max_val = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MAX_VAL")),
							  wxStaticText);

	m_min_label = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MIN_LABEL")),
								wxStaticText);
	m_min_val = wxDynamicCast(FindWindow(XRCID("ID_STATIC_MIN_VAL")),
							  wxStaticText);
	
	if (default_field_type == GeoDaConst::double_type) {
		m_type->SetSelection(0);
	} else if (default_field_type == GeoDaConst::long64_type) {
		m_type->SetSelection(1);
	} else if (default_field_type == GeoDaConst::string_type) {
		m_type->SetSelection(2);
	} else if (default_field_type == GeoDaConst::date_type) {
		m_type->SetSelection(3);
	} else {
		m_type->SetSelection(0);
	}
			   
	UpdateApplyButton();
	CheckName();
	SetDefaultsByType(default_field_type);
}

void DataViewerAddColDlg::OnChoiceType( wxCommandEvent& ev )
{
	switch (ev.GetSelection()) {
		case 0:
			if (cur_type == GeoDaConst::double_type) return;
			SetDefaultsByType(GeoDaConst::double_type);
			break;
		case 1:
			if (cur_type == GeoDaConst::long64_type) return;
			SetDefaultsByType(GeoDaConst::long64_type);
			break;
		case 2:
			if (cur_type == GeoDaConst::string_type) return;
			SetDefaultsByType(GeoDaConst::string_type);
			break;
		case 3:
			if (cur_type == GeoDaConst::date_type) return;
			SetDefaultsByType(GeoDaConst::date_type);
			break;
		default:
			// should never get here!
			SetDefaultsByType(GeoDaConst::double_type);
	}
	
}

void DataViewerAddColDlg::SetDefaultsByType(GeoDaConst::FieldType type)
{
	// set some defaults first
	m_length->Enable();
	m_decimals_lable->SetLabelText("Decimals");
	m_decimals->SetValue("0");
	m_decimals->Disable();
	m_displayed_decimals->SetValue("0");
	m_displayed_decimals->Disable();
	m_max_label->SetLabelText("");
	m_max_val->SetLabelText("");
	m_min_label->SetLabelText("");
	m_min_val->SetLabelText("");
	m_length_valid = true;
	m_decimals_valid = true;
	m_displayed_decimals_valid = true;
	switch (type) {
		case GeoDaConst::double_type:
		{
			cur_type = GeoDaConst::double_type;
			m_length_lable->SetLabelText(wxString::Format("Length (max %d)",
								GeoDaConst::max_dbf_double_len));
			m_length->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_double_len));
			m_decimals_lable->SetLabelText(wxString::Format("Decimals (max %d)",
								GeoDaConst::max_dbf_double_decimals));
			m_decimals->Enable();
			m_decimals->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_double_decimals));			  
			m_displayed_decimals->Enable();
			m_displayed_decimals->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_double_decimals));
			m_max_label->SetLabelText("maximum");
			m_max_val->SetLabelText(wxString::Format("%d", 9));
			m_min_label->SetLabelText("minimum");
			m_min_val->SetLabelText(wxString::Format("-%d", 9));
		}
			break;
		case GeoDaConst::long64_type:
		{
			cur_type = GeoDaConst::long64_type;
			m_length_lable->SetLabelText(wxString::Format("Length (max %d)",
								GeoDaConst::max_dbf_long_len));
			m_length->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_long_len));
			m_max_label->SetLabelText("maximum");
			m_max_val->SetLabelText(wxString::Format("%d", 9));
			m_min_label->SetLabelText("minimum");
			m_min_val->SetLabelText(wxString::Format("-%d", 9));
		}
			break;
		case GeoDaConst::string_type:
		{
			cur_type = GeoDaConst::string_type;
			m_length_lable->SetLabelText(wxString::Format("Length (max %d)",
								GeoDaConst::max_dbf_string_len));
			m_length->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_string_len));
		}
			break;
		case GeoDaConst::date_type:
		default:
		{
			cur_type = GeoDaConst::date_type;
			m_length_lable->SetLabelText("Length");
			m_length->SetValue(wxString::Format("%d",
								GeoDaConst::default_dbf_date_len));
			
			m_length->Disable();
		}
	}
	UpdateMinMaxValues();
}

void DataViewerAddColDlg::OnOkClick( wxCommandEvent& ev )
{
	LOG_MSG("Entering DataViewerAddColDlg::OnOkClick");
    wxString colname = m_name->GetValue();
	colname.Trim(true);  // trim white-space from right of string
	colname.Trim(false); // trim white-space from left of string
	
	if(colname == wxEmptyString) {
		wxString msg("Error: Table variable name empty.");
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
	
	if ( !DbfFileUtils::isValidFieldName(colname) ) {
		wxString msg;
		msg += "Error: \"" + colname + "\" is an invalid ";
		msg += "variable name.  A valid variable name is between one and ten ";
		msg += "characters long.  The first character must be alphabetic,";
		msg += " and the remaining characters can be either alphanumeric ";
		msg += "or underscores.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	long displayed_decimals = 0;
	m_displayed_decimals->GetValue().ToLong(&displayed_decimals);
	
	// Note: before we get to this point, all parameters are assumed
	// to be valid, except for double_type, where we need to check
	// that the particular combination of decimals and length is valid
	
	if (cur_type == GeoDaConst::double_type) {
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(m_length_val, m_decimals_val,
										  &suggest_len, &suggest_dec);
		if (m_length_val != suggest_len || m_decimals_val != suggest_dec) {
			wxString msg;
			msg += "Error: Due to restrictions on the DBF file format, the ";
			msg += "particular combination of length and decimals entered is ";
			msg += "not valid.  Based on your current choices, we recommend ";
			msg << "length = " << suggest_len;
			msg << " and decimals = " << suggest_dec;
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	int col_insert_pos;
	if (m_insert_pos->GetSelection() >= grid_base->GetNumberCols()) {
		col_insert_pos = grid_base->GetNumberCols();
	} else {
		// One might think we want to use the col_id_map here, but it
		// turns out we need to specify the position in the visual
		// order it appears in the Table.
		col_insert_pos = m_insert_pos->GetSelection();
	}
	int time_steps = 1; // non-space-time column by default	
	if (m_time_inv_no && m_time_inv_no->GetValue()) {
		time_steps = grid_base->time_steps;
	}

	LOG_MSG(wxString::Format("Inserting new column %s into Table",
							 colname.Upper().c_str()));
	grid_base->InsertCol(col_insert_pos, time_steps, cur_type, colname.Upper(),
						 m_length_val, m_decimals_val, displayed_decimals);
	final_col_name = colname.Upper();
	final_col_id = col_insert_pos;

	ev.Skip();
	EndDialog(wxID_OK);
	LOG_MSG("Exiting DataViewerAddColDlg::OnOkClick");
}

void DataViewerAddColDlg::OnEditName( wxCommandEvent& ev )
{
	CheckName();
}

void DataViewerAddColDlg::CheckName()
{
	wxString s = m_name->GetValue();
	if (s.length() > 0) {
		wxString f = s.SubString(0, 0);
		if (f == "_" || f == "0" || f == "1" || f == "2" || f == "3" ||
			f == "4" || f == "5" || f == "6" || f == "7" || f == "8" ||
			f == "9") {
			s = s.SubString(1, s.length()-1);
			m_name->ChangeValue(s);
		}
	}
	if (DbfFileUtils::isValidFieldName(s) || s.length() == 0) {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxBLACK);
		m_name->SetStyle(0, s.length(), style);
	} else {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxRED);
		m_name->SetStyle(0, s.length(), style);
	}
	m_name_valid = (DbfFileUtils::isValidFieldName(s) &&
					!grid_base->IsSpaceTimeIdField(s));
	
	UpdateApplyButton();	
}

void DataViewerAddColDlg::OnEditLength( wxCommandEvent& ev )
{
	wxString s = m_length->GetValue();
	long val = 0;
	bool is_num = s.ToLong(&val);
	m_length_val = val;
	
	long min_v;
	long max_v;
	if (cur_type == GeoDaConst::date_type) {
		min_v = GeoDaConst::min_dbf_date_len;
		max_v = GeoDaConst::max_dbf_date_len;
	} else if (cur_type == GeoDaConst::long64_type) {
		min_v = GeoDaConst::min_dbf_long_len;
		max_v = GeoDaConst::max_dbf_long_len;
	} else if (cur_type == GeoDaConst::double_type) {
		min_v = GeoDaConst::min_dbf_double_len;
		max_v = GeoDaConst::max_dbf_double_len;
	} else  { // cur_type == GeoDaConst::string_type
		min_v = GeoDaConst::min_dbf_string_len;
		max_v = GeoDaConst::max_dbf_string_len;
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
	if (!cur_type == GeoDaConst::double_type) {
		m_decimals_valid = true;
		UpdateApplyButton();
		return;
	}
	wxString s = m_decimals->GetValue();
	long val = 0;
	bool is_num = s.ToLong(&val);
	m_decimals_val = val;
	long min_v = GeoDaConst::min_dbf_double_decimals;
	long max_v = GeoDaConst::max_dbf_double_decimals;
		
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

void DataViewerAddColDlg::OnEditDisplayedDecimals( wxCommandEvent& ev )
{
	if (!cur_type == GeoDaConst::double_type) {
		m_displayed_decimals_valid = true;
		UpdateApplyButton();
		return;
	}
	wxString s = m_displayed_decimals->GetValue();
	long val = 0;
	bool is_num = s.ToCLong(&val);
	long min_v = 0;
	long max_v = GeoDaConst::max_dbf_double_decimals;
	
	if ((is_num && val >= min_v && val <= max_v) || s.length() == 0) {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxBLACK);
		m_name->SetStyle(0, s.length(), style);
	} else {
		wxTextAttr style(m_name->GetDefaultStyle());
		style.SetTextColour(*wxRED);
		m_name->SetStyle(0, s.length(), style);
	}
	m_displayed_decimals_valid = is_num && val >= min_v && val <= max_v;
	UpdateMinMaxValues();
	UpdateApplyButton();
}

void DataViewerAddColDlg::UpdateMinMaxValues()
{
	m_max_val->SetLabelText("");
	m_min_val->SetLabelText("");
	if (!m_length_valid || !m_decimals_valid ||
		!(cur_type == GeoDaConst::double_type ||
		  cur_type == GeoDaConst::long64_type)) {
		return;
	}
	
	long length = 8;
	m_length->GetValue().ToCLong(&length);
	if (cur_type == GeoDaConst::double_type) {
		long decimals = 0;
		m_decimals->GetValue().ToLong(&decimals);
		int suggest_len;
		int suggest_dec;
		DbfFileUtils::SuggestDoubleParams(length, decimals,
										  &suggest_len, &suggest_dec);
		if (length == suggest_len && decimals == suggest_dec) {
			m_max_val->SetLabelText(DbfFileUtils::GetMaxDoubleString(length,
																	 decimals));
			m_min_val->SetLabelText(DbfFileUtils::GetMinDoubleString(length,
																	 decimals));
		}
	} else { // cur_type == GeoDaConst::long64_type
		m_max_val->SetLabelText(DbfFileUtils::GetMaxIntString(length));
		m_min_val->SetLabelText(DbfFileUtils::GetMinIntString(length));
	}
}

void DataViewerAddColDlg::UpdateApplyButton()
{
	if (m_name_valid && m_length_valid && m_decimals_valid &&
		m_displayed_decimals_valid && m_length_val > m_decimals_val) {
		m_apply_button->Enable();
	} else {
		m_apply_button->Disable();
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


