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

#include <algorithm>
#include <set>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "../ShapeOperations/DbfFile.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "TimeVariantImportDlg.h"

BEGIN_EVENT_TABLE( TimeVariantImportDlg, wxDialog )
    EVT_BUTTON( XRCID("wxID_OK"), TimeVariantImportDlg::OnOkClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), TimeVariantImportDlg::OnCloseClick )
	EVT_BUTTON( XRCID("ID_MOVE_UP_BUTTON"),
			   TimeVariantImportDlg::OnMoveUpClick )
	EVT_BUTTON( XRCID("ID_MOVE_DOWN_BUTTON"),
			   TimeVariantImportDlg::OnMoveDownClick )
	EVT_BUTTON( XRCID("ID_SORT_BUTTON"), TimeVariantImportDlg::OnSortClick )
	EVT_BUTTON( XRCID("ID_REVERSE_BUTTON"),
			   TimeVariantImportDlg::OnReverseClick )
	EVT_BUTTON( XRCID("ID_ADD_TO_LIST_BUTTON"),
			   TimeVariantImportDlg::OnAddToListClick )
	EVT_BUTTON( XRCID("ID_REMOVE_FR_LIST_BUTTON"),
			   TimeVariantImportDlg::OnRemoveFrListClick )
	EVT_LISTBOX_DCLICK( XRCID("ID_ALL_FIELDS_LIST"),
					   TimeVariantImportDlg::OnAddToListClick )
	EVT_LISTBOX( XRCID("ID_ALL_FIELDS_LIST"),
				TimeVariantImportDlg::OnAllFieldsListSelection )
	EVT_LISTBOX_DCLICK( XRCID("ID_INCLUDE_LIST"),
					   TimeVariantImportDlg::OnRemoveFrListClick )
	EVT_LISTBOX( XRCID("ID_INCLUDE_LIST"),
				TimeVariantImportDlg::OnIncludeListSelection )
	EVT_TEXT( XRCID("ID_NEW_FIELD_NAME_TXT_CTRL"),
			 TimeVariantImportDlg::OnNewFieldNameChange )
	EVT_CHECKBOX( XRCID("ID_SHOW_DETAILS"),
				 TimeVariantImportDlg::OnShowDetailsClick )
	EVT_BUTTON( XRCID("ID_CUR_VARS_HELP"),
			   TimeVariantImportDlg::OnCurVarsHelp )
	EVT_BUTTON( XRCID("ID_NEW_SP_TM_VAR_HELP"),
			   TimeVariantImportDlg::OnNewSpTmVarHelp )
	EVT_BUTTON( XRCID("ID_VARS_REMAINING_HELP"),
			   TimeVariantImportDlg::OnVarsRemainingHelp )
	EVT_BUTTON( XRCID("ID_CUR_SP_TM_VARS_HELP"),
			   TimeVariantImportDlg::OnCurSpTmVarsHelp )
END_EVENT_TABLE()

TimeVariantImportDlg::TimeVariantImportDlg(DbfGridTableBase* grid_base_s,
										   wxWindow* parent,
										   const wxString& title,
										   const wxPoint& pos,
										   const wxSize& size, long style)
: grid_base(grid_base_s), time_steps(grid_base_s->GetTimeSteps()),
common_empty(true), show_details(false), all_init(false)
{
	CreateControls();
	SetPosition(pos);
	SetTitle(title);
    Centre();
	all_init = true;
}

void TimeVariantImportDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_TIME_VARIANT_IMPORT_DLG");
	ok_button = wxDynamicCast(FindWindow(XRCID("wxID_OK")),
							  wxButton);
	
	new_field_name_txt_ctrl =
		wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_NAME_TXT_CTRL")),
					  wxTextCtrl);
	new_field_type_stat_txt =
		wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_TYPE_STAT_TXT")),
					  wxStaticText);
	new_field_length_stat_txt =
		wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_LENGTH_STAT_TXT")),
					  wxStaticText);
	new_field_decimals_caption =
		wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_DECIMALS_CAPTION")),
					  wxStaticText);
	new_field_decimals_stat_txt =
		wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_DECIMALS_STAT_TXT")),
					  wxStaticText);
	include_list_stat_txt =
		wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST_STAT_TXT")),
					  wxStaticText);
	
	move_up_button = wxDynamicCast(FindWindow(XRCID("ID_MOVE_UP_BUTTON")),
								   wxButton);
	move_down_button = wxDynamicCast(FindWindow(XRCID("ID_MOVE_DOWN_BUTTON")),
									 wxButton);
	sort_button = wxDynamicCast(FindWindow(XRCID("ID_SORT_BUTTON")),
								wxButton);
	reverse_button = wxDynamicCast(FindWindow(XRCID("ID_REVERSE_BUTTON")),
								   wxButton);
	
	include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")),
								 wxListBox);
	
	add_to_list_button =
		wxDynamicCast(FindWindow(XRCID("ID_ADD_TO_LIST_BUTTON")), wxButton);
	remove_fr_list_button =
		wxDynamicCast(FindWindow(XRCID("ID_REMOVE_FR_LIST_BUTTON")), wxButton);
	
	all_fields_list = wxDynamicCast(FindWindow(XRCID("ID_ALL_FIELDS_LIST")),
									wxListBox);
	all_st_fields_list =
		wxDynamicCast(FindWindow(XRCID("ID_ALL_ST_FIELDS_LIST")), wxListBox);
	show_details_cb = wxDynamicCast(FindWindow(XRCID("ID_SHOW_DETAILS")),
									wxCheckBox);
	
	ResetAllButtons();
	ResetCommonTypeLenDecs();
		
	all_init = true;
	InitAllFieldsList();
}

void TimeVariantImportDlg::InitAllFieldsList()
{
	new_field_name_txt_ctrl->SetValue(wxEmptyString);
	UpdateTimeStepsTxt(0);
	all_fields_to_name.clear();
	name_to_all_fields.clear();
	all_fields_list->Clear();
	all_st_fields_list->Clear();
	include_list->Clear();
	std::vector<int> col_id_map;
	grid_base->FillColIdMap(col_id_map);
	for (int i=0, its=col_id_map.size(); i<its; i++) {
		int col = col_id_map[i];
		if (grid_base->IsColTimeVariant(col)) {
			all_st_fields_list->Append(grid_base->GetColName(col));
			continue;
		}
		GeoDaConst::FieldType type = grid_base->GetColType(col);
		int length = grid_base->GetColLength(col);
		int decimals = grid_base->GetColDecimals(col);
		wxString name(grid_base->GetColName(col));
		wxString s(name);
		if (show_details) {
			if (type == GeoDaConst::double_type) {
				s << ": type=real, length=" << length;
				s << ", decimals=" << decimals;
			} else if (type == GeoDaConst::long64_type) {
				s << ", type=integer, length=" << length;
			} else if (type == GeoDaConst::string_type) {
				s << ", type=string, length=" << length;
			} else if (type == GeoDaConst::date_type) {
				s << ", type=date, length=" << length;
			}
		}
		all_fields_list->Append(s);

		all_fields_to_name[s] = name;
		name_to_all_fields[name] = s;
	}
	// the following is the only way to make scroll window start at the
	// top initially
	if (all_fields_list->GetCount() > 0) all_fields_list->SetSelection(0);
	// the following two calls to set the scroll position should work
	// but currently don't work when window is first initialized
	if (all_fields_list->HasScrollbar(wxVERTICAL)) {
        all_fields_list->SetScrollPos(wxVERTICAL, 0);
    }
	all_fields_list->ScrollLines(0);
}

void TimeVariantImportDlg::OnOkClick( wxCommandEvent& event )
{
	if (!all_init) return;
	
	// check that new field name is valid
	wxString time_field = new_field_name_txt_ctrl->GetValue();
	time_field.Trim(true);
	time_field.Trim(false);
	time_field = time_field.Upper();
	if ( !DbfFileUtils::isValidFieldName(time_field) ) {
		wxString msg;
		msg += "Error: \"" + time_field + "\" is an invalid ";
		msg += "field name.  A valid field name is between one and ten ";
		msg += "characters long.  The first character must be alphabetic,";
		msg += " and the remaining characters can be either alphanumeric ";
		msg += "or underscores.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	// check that new field name does not already exist
	if ( grid_base->ColNameExists(time_field) ) {
		wxString msg;
		msg << time_field << " already exists in the Table. Please choose ";
		msg << "a different name.";
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}

	LOG_MSG(wxString::Format("Inserting new time-variant column %s into Table",
							 time_field.Upper().c_str()));
	int col_pos = grid_base->GetNumberCols();
	grid_base->InsertCol(col_pos, time_steps, common_type, time_field,
						 common_length, common_decimals, common_decimals,
						 true, false);
	
	// since we inserted in the last position, we know that col_pos
	// correctly refers to the newly created time-variant column.
	for (int t=0; t<time_steps; t++) {
		int sp_col = grid_base->FindColId(include_list->GetString(t));
		grid_base->col_data[sp_col]->CopyVectorToRawData();
		if (!grid_base->CopySpColRawToTmColRaw(sp_col, col_pos, t)) {
			wxString msg;
			msg << "An error occurred while copying data into the new ";
			msg << "time-variant field.  Please report this error.";
			wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
	}
	
	// delete time-invariant fields from the Table
	for (int t=0; t<time_steps; t++) {
		int sp_col = grid_base->FindColId(include_list->GetString(t));
		grid_base->DeleteCol(sp_col);
	}
	
    grid_base->SetChangedSinceLastSave(true);
    MyFrame::theFrame->UpdateToolbarAndMenus();
	
	InitAllFieldsList();
	ResetAllButtons();
	ResetCommonTypeLenDecs();
	common_empty = true;
}

void TimeVariantImportDlg::OnCloseClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CLOSE);
}

void TimeVariantImportDlg::OnMoveUpClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnMoveUpClick");
	if (!all_init) return;
	int cnt = include_list->GetCount();
	int cur_sel = include_list->GetSelection();
	if (cur_sel == wxNOT_FOUND || cnt < 2 || cur_sel == 0 ) return;
	
	wxString cur_sel_str = include_list->GetStringSelection();
	include_list->Delete(cur_sel);
	include_list->Insert(cur_sel_str, cur_sel-1);
	include_list->SetSelection(include_list->FindString(cur_sel_str));
	
	UpdateButtons();
}

void TimeVariantImportDlg::OnMoveDownClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnMoveDownClick");
	if (!all_init) return;
	int cnt = include_list->GetCount();
	int cur_sel = include_list->GetSelection();
	if (cur_sel == wxNOT_FOUND || cnt < 2 || cur_sel == cnt-1) return;
	
	wxString cur_sel_str = include_list->GetStringSelection();
	include_list->Delete(cur_sel);
	include_list->Insert(cur_sel_str, cur_sel+1);
	include_list->SetSelection(include_list->FindString(cur_sel_str));
		
	UpdateButtons();
}

void TimeVariantImportDlg::OnSortClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnSortClick");
	if (!all_init) return;
	
	wxString cur_sel_str = include_list->GetStringSelection();
	int cnt = include_list->GetCount();
	std::vector<wxString> sorted(cnt);
	for (int i=0; i<cnt; i++) sorted[i] = include_list->GetString(i);
	std::sort(sorted.begin(), sorted.end());
	include_list->Clear();
	for (int i=0; i<cnt; i++) include_list->Append(sorted[i]);
	include_list->SetSelection(include_list->FindString(cur_sel_str));
	
	UpdateButtons();
}

void TimeVariantImportDlg::OnReverseClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnReverseClick");
	if (!all_init) return;

	wxString cur_sel_str = include_list->GetStringSelection();
	int cnt = include_list->GetCount();
	std::vector<wxString> order(cnt);
	for (int i=0; i<cnt; i++) order[i] = include_list->GetString(i);
	include_list->Clear();
	for (int i=0; i<cnt; i++) include_list->Append(order[(cnt-1)-i]);
	include_list->SetSelection(include_list->FindString(cur_sel_str));
		
	UpdateButtons();
}

void TimeVariantImportDlg::OnAddToListClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnAddToListClick");
	if (!all_init) return;
	
	int sel = all_fields_list->GetSelection();
	if (sel == wxNOT_FOUND ||
		time_steps == include_list->GetCount()) return;
	wxString string_sel(all_fields_list->GetStringSelection());
	int sel_col = grid_base->FindColId(all_fields_to_name[string_sel]);
	
	if (!IsCompatibleWithCommon(sel_col)) return;
	
	GeoDaConst::FieldType type = grid_base->GetColType(sel_col);
	int length = grid_base->GetColLength(sel_col);
	int decimals = grid_base->GetColDecimals(sel_col);
	wxString name(grid_base->GetColName(sel_col));
	
	if (include_list->GetCount() == 0) {
		common_empty = false;
		common_length = length;
		common_type = type;
		common_decimals = decimals;
		wxString s;
		if (type == GeoDaConst::double_type) {
			s << common_decimals;
		} else {
			s << "not applicable";
		}
		new_field_decimals_stat_txt->SetLabelText(s);
		
		{
			wxString s;
			s << common_length;
			new_field_length_stat_txt->SetLabelText(s);
		}
		
		if (common_type == GeoDaConst::double_type) {
			new_field_type_stat_txt->SetLabelText("real");
		} else if (type == GeoDaConst::long64_type) {
			new_field_type_stat_txt->SetLabelText("integer");
		} else if (type == GeoDaConst::string_type) {
			new_field_type_stat_txt->SetLabelText("string");
		} else if (type == GeoDaConst::date_type) {
			new_field_type_stat_txt->SetLabelText("date");
		}
		
		Refresh();
	}
	
	include_list->Append(name);
	all_fields_list->Delete(sel);
	if (sel >= all_fields_list->GetCount()) sel = all_fields_list->GetCount()-1;
	if (sel >= 0 && sel < all_fields_list->GetCount()) {
		all_fields_list->Select(sel);
	}
	UpdateTimeStepsTxt(include_list->GetCount());
	
	UpdateButtons();
}

void TimeVariantImportDlg::OnRemoveFrListClick( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnRemoveFrListClick");
	if (!all_init) return;
	
	int sel = include_list->GetSelection();
	if (sel == wxNOT_FOUND) return;
	wxString string_sel(include_list->GetString(sel));
	LOG(string_sel);

	all_fields_list->DeselectAll();
	all_fields_list->Append(name_to_all_fields[string_sel]);
	LOG(name_to_all_fields[string_sel]);

	
	// This is a bug in wxListBox where duplicate items are
	// sometimes displayed, even though the internal strings
	// are correct.  We will therefore attempt to correct this
	// by clearing the all_fields_list and reinserting all its
	// items
	std::vector<wxString> cpy(all_fields_list->GetCount());
	for (int i=0; i<all_fields_list->GetCount(); i++) {
		cpy[i] = all_fields_list->GetString(i); 
	}
	all_fields_list->Clear();
	for (int i=0; i<cpy.size(); i++) {
		all_fields_list->Append(cpy[i]);
	}
		
	include_list->Delete(sel);
	UpdateTimeStepsTxt(include_list->GetCount());
	
	if (include_list->GetCount() == 0) {
		common_empty = true;
		ResetCommonTypeLenDecs();
		Refresh();
	}
	
	UpdateButtons();
}

void TimeVariantImportDlg::OnAllFieldsListSelection( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnAllFieldsListSelection");
	if (!all_init) return;
	UpdateButtons();
}

void TimeVariantImportDlg::OnIncludeListSelection( wxCommandEvent& event )
{
	LOG_MSG("In TimeVariantImportDlg::OnIncludeListSelection");
	if (!all_init) return;
	UpdateButtons();
}

void TimeVariantImportDlg::OnNewFieldNameChange( wxCommandEvent& event )
{
	if (!all_init) return;
	UpdateCreateButton();
}

void TimeVariantImportDlg::OnShowDetailsClick( wxCommandEvent& event )
{
	if (!all_init) return;
	show_details = (show_details_cb->GetValue() == 1);
	// repopulate all_fields_list and all_fields_to_name and
	// name_to_all_fields
	int cur_sel = all_fields_list->GetSelection();
	int vert_scroll_pos = all_fields_list->GetScrollPos(wxVERTICAL);
	LOG(vert_scroll_pos);
	
	std::vector<wxString> new_all_fields_list(all_fields_list->GetCount());
	for (int i=0; i<all_fields_list->GetCount(); i++) {
		wxString col_nm = all_fields_to_name[all_fields_list->GetString(i)];
		int col = grid_base->FindColId(col_nm);
		GeoDaConst::FieldType type = grid_base->GetColType(col);
		int length = grid_base->GetColLength(col);
		int decimals = grid_base->GetColDecimals(col);
		wxString name(grid_base->GetColName(col));
		wxString s(name);
		if (show_details) {
			if (type == GeoDaConst::double_type) {
				s << ": type=real, length=" << length;
				s << ", decimals=" << decimals;
			} else if (type == GeoDaConst::long64_type) {
				s << ", type=integer, length=" << length;
			} else if (type == GeoDaConst::string_type) {
				s << ", type=string, length=" << length;
			} else if (type == GeoDaConst::date_type) {
				s << ", type=date, length=" << length;
			}
		}
		new_all_fields_list[i] = s;
	}
	
	all_fields_list->Clear();
	for (int i=0; i<new_all_fields_list.size(); i++) {
		all_fields_list->Append(new_all_fields_list[i]);
	}
	if (cur_sel != wxNOT_FOUND) all_fields_list->SetSelection(cur_sel);
    if (all_fields_list->HasScrollbar(wxVERTICAL)) {
        all_fields_list->SetScrollPos(wxVERTICAL, vert_scroll_pos);
    }

	all_fields_to_name.clear();
	name_to_all_fields.clear();
	std::vector<int> col_id_map;
	grid_base->FillColIdMap(col_id_map);
	for (int i=0, its=col_id_map.size(); i<its; i++) {
		int col = col_id_map[i];
		if (grid_base->IsColTimeVariant(col)) continue;
		GeoDaConst::FieldType type = grid_base->GetColType(col);
		int length = grid_base->GetColLength(col);
		int decimals = grid_base->GetColDecimals(col);
		wxString name(grid_base->GetColName(col));
		wxString s(name);
		if (show_details) {
			if (type == GeoDaConst::double_type) {
				s << ": type=real, length=" << length;
				s << ", decimals=" << decimals;
			} else if (type == GeoDaConst::long64_type) {
				s << ", type=integer, length=" << length;
			} else if (type == GeoDaConst::string_type) {
				s << ", type=string, length=" << length;
			} else if (type == GeoDaConst::date_type) {
				s << ", type=date, length=" << length;
			}
		}
		all_fields_to_name[s] = name;
		name_to_all_fields[name] = s;
	}
	
	Refresh();
}

void TimeVariantImportDlg::UpdateCreateButton()
{
	ok_button->Enable(!new_field_name_txt_ctrl->GetValue().IsEmpty() &&
					  include_list->GetCount() == time_steps);
}

void TimeVariantImportDlg::UpdateButtons()
{
	int cnt = include_list->GetCount();
	move_up_button->Enable(cnt > 1 &&
						   include_list->GetSelection() != wxNOT_FOUND &&
						   include_list->GetSelection() > 0);
	move_down_button->Enable(cnt > 1 &&
							 include_list->GetSelection() != wxNOT_FOUND &&
							 include_list->GetSelection() < cnt-1);
	sort_button->Enable(cnt > 1);
	reverse_button->Enable(cnt > 1);
	
	{
		bool enable = false;
		int sel = all_fields_list->GetSelection();
		wxString string_sel(all_fields_list->GetStringSelection());
		int sel_col = grid_base->FindColId(all_fields_to_name[string_sel]);
		enable = (sel != wxNOT_FOUND &&
				  time_steps != include_list->GetCount() &&
				  IsCompatibleWithCommon(sel_col));
		add_to_list_button->Enable(enable);
	}
	
	remove_fr_list_button->Enable(include_list->GetSelection() != wxNOT_FOUND);
	
	UpdateCreateButton();
}

void TimeVariantImportDlg::ResetAllButtons()
{
	ok_button->Disable();
	move_up_button->Disable();
	move_down_button->Disable();
	sort_button->Disable();
	reverse_button->Disable();
	add_to_list_button->Disable();
	remove_fr_list_button->Disable();
}

void TimeVariantImportDlg::ResetCommonTypeLenDecs()
{
	new_field_type_stat_txt->SetLabelText("");
	new_field_length_stat_txt->SetLabelText("");
	new_field_decimals_caption->Show(true);
	new_field_decimals_stat_txt->SetLabelText("");
	new_field_decimals_stat_txt->Show(true);
}

void TimeVariantImportDlg::UpdateTimeStepsTxt(int num_list_items)
{
	wxString s;
	s << num_list_items << " of " << time_steps << " variables to include";
	include_list_stat_txt->SetLabelText(s);
}

bool TimeVariantImportDlg::IsCompatibleWithCommon(int col)
{
	if (common_empty) return true;
	GeoDaConst::FieldType type = grid_base->GetColType(col);
	int length = grid_base->GetColLength(col);
	int decimals = grid_base->GetColDecimals(col);

	if (type != common_type) return false;
	if (length != common_length) return false;
	if (type == GeoDaConst::double_type &&
		decimals != common_decimals) return false;
	
	return true;
}

void TimeVariantImportDlg::OnCurVarsHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "List of existing, time-invariant (do not change with time) ";
	msg << "variables. Move variables that measure the same property, ";
	msg << "but at different times into the list to the right.\n\n";
	msg << "For example, to combine the four population variables ";
	msg << "POP_2000, POP_2001, POP_2002, and POP_2003 into one ";
	msg << "space-time variable called POP, move each of these to the ";
	msg << "list on the right and specify POP for the new space-time ";
	msg << "variable name.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void TimeVariantImportDlg::OnNewSpTmVarHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Desired name for new space-time variable. As an example, ";
	msg << "if you were to group the four existing variables ";
	msg << "POP_2000, POP_2001, POP_2002, and POP_2003 into one ";
	msg << "space-time variable, a logical name-choice might be POP.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void TimeVariantImportDlg::OnVarsRemainingHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Must choose existing (non-time dependent) variables: ";
	msg << "one variable for each time period. ";
	msg << "Variables must be arranged in ascending order with ";
	msg << "the variable corresponding to the first time period on top.";
	msg << "\n\n";
	msg << "All variables to be grouped together must have identical ";
	msg << "properties. To edit variable properties, close this tool ";
	msg << "and open the Edit Variable Properties dialog from the Tools ";
	msg << "menu.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void TimeVariantImportDlg::OnCurSpTmVarsHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "This is the list of existing space-time variables for ";
	msg << "reference purposes. As new space-time variables are ";
	msg << "created, they will appear on this list.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}
