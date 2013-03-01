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

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
//#include <wx/richmsgdlg.h> //2.9.2 feature
#include "../GenUtils.h"
#include "../logger.h"
#include "../ShapeOperations/DbfFile.h"
#include "DbfGridTableBase.h"
#include "DataViewerEditFieldPropertiesDlg.h"

BEGIN_EVENT_TABLE( DataViewerEditFieldPropertiesDlg, wxDialog )
	EVT_GRID_LABEL_LEFT_CLICK(
		DataViewerEditFieldPropertiesDlg::OnLabelLeftClickEvent )
	EVT_GRID_CELL_LEFT_CLICK(
		DataViewerEditFieldPropertiesDlg::OnCellClickLeft )
	EVT_GRID_EDITOR_SHOWN(
		DataViewerEditFieldPropertiesDlg::OnCellEditorShown )
	EVT_GRID_EDITOR_HIDDEN(
		DataViewerEditFieldPropertiesDlg::OnCellEditorHidden )
    EVT_BUTTON( wxID_APPLY,
			   DataViewerEditFieldPropertiesDlg::OnApplyButton )
	EVT_BUTTON( wxID_CLOSE,
			   DataViewerEditFieldPropertiesDlg::OnCloseButton )
	EVT_CLOSE( DataViewerEditFieldPropertiesDlg::OnClose )
	EVT_GRID_CELL_CHANGING(DataViewerEditFieldPropertiesDlg::OnCellEdit) 
END_EVENT_TABLE()

const int COL_N = 0; // field name
const int COL_T = 1; // type
const int COL_L = 2; // length
const int COL_D = 3; // decimals
const int COL_DD = 4; // displayed decimals
const int COL_T_INV = 5; // time invariant 
int NUM_COLS = 5;

DataViewerEditFieldPropertiesDlg::DataViewerEditFieldPropertiesDlg(
				DbfGridTableBase* grid_base_s,
				const wxPoint &pos, const wxSize &size )
: wxDialog(0, wxID_ANY, "Variable Properties", pos, size,
		   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
grid_base(grid_base_s), is_space_time(grid_base_s->time_steps > 1),
cell_editor_open(false),
reenable_apply_after_cell_editor_hidden(false)
{
	if (is_space_time) NUM_COLS = 6;
    CreateControls();
	SetTitle("Variable Properties - " + grid_base->GetDbfNameNoExt());
    Centre();
}

void DataViewerEditFieldPropertiesDlg::CreateControls()
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::CreateControls");
	
	grid_base->FillColIdMap(col_id_map);
	
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	field_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(200,400));
	field_grid->CreateGrid(grid_base->GetNumberCols(), NUM_COLS,
						   wxGrid::wxGridSelectRows);
	
	field_grid->SetColLabelValue(COL_N, "variable name");
	field_grid->SetColLabelValue(COL_T, "type");
	field_grid->SetColLabelValue(COL_L, "length");
	field_grid->SetColLabelValue(COL_D, "decimals");
	field_grid->SetColLabelValue(COL_DD, "displayed\ndecimals");
	if (is_space_time) field_grid->SetColLabelValue(COL_T_INV,
													"time\ninvariant");

	field_grid->HideRowLabels();
	
	field_grid->BeginBatch();
	for (int i=0, iend=grid_base->GetNumberCols(); i<iend; i++) {
		DbfColContainer& cd = *(grid_base->col_data[col_id_map[i]]);
		wxString name = cd.name.Upper();
		std::map<wxString, int>::iterator it = fn_freq.find(name);
		if (fn_freq.find(name) != fn_freq.end()) {
			fn_freq[name]++;
		} else {
			fn_freq[name] = 1;
		}
		field_grid->SetCellValue(i, COL_N, cd.name);
		field_grid->SetCellAlignment(i, COL_N, wxALIGN_CENTRE, wxALIGN_CENTRE);
		field_grid->SetReadOnly(i, COL_T, true);
		field_grid->SetCellAlignment(i, COL_T, wxALIGN_CENTRE, wxALIGN_CENTRE);
		field_grid->SetCellTextColour(i, COL_T,
									  GeoDaConst::table_no_edit_color);
		field_grid->SetColFormatNumber(COL_L);
		field_grid->SetColFormatNumber(COL_D);
		field_grid->SetColFormatNumber(COL_DD);
		if (cd.type == GeoDaConst::double_type) {
			field_grid->SetCellValue(i, COL_T, "real");
			field_grid->SetCellValue(i, COL_D,
							wxString::Format("%d", cd.decimals));
			field_grid->SetCellValue(i, COL_DD, 
							wxString::Format("%d", cd.displayed_decimals));
		} else if (cd.type == GeoDaConst::long64_type) {
			field_grid->SetCellValue(i, COL_T, "integer");
			field_grid->SetReadOnly(i, COL_D, true);
			field_grid->SetReadOnly(i, COL_DD, true);
		} else if (cd.type == GeoDaConst::date_type) {
			field_grid->SetCellValue(i, COL_T, "date");
			field_grid->SetReadOnly(i, COL_L, true); // should be length 8
			field_grid->SetReadOnly(i, COL_D, true);
			field_grid->SetReadOnly(i, COL_DD, true);			
		} else {
			field_grid->SetCellValue(i, COL_T, "string");
			field_grid->SetReadOnly(i, COL_D, true);
			field_grid->SetReadOnly(i, COL_DD, true);
		}
		field_grid->SetCellValue(i, COL_L,
								 wxString::Format("%d", cd.field_len));
		if (is_space_time) {
			wxString val(cd.time_steps > 1 ? "no" : "yes");
			field_grid->SetCellValue(i, COL_T_INV, val);
			field_grid->SetCellAlignment(i, COL_T_INV, wxALIGN_CENTRE,
										 wxALIGN_CENTRE);
			field_grid->SetCellTextColour(i, COL_T_INV,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_T_INV, true);
		}
		if (grid_base->IsSpaceTimeIdField(cd.name)) {
			field_grid->SetCellTextColour(i, COL_N,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_N, true);
			field_grid->SetCellTextColour(i, COL_T,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_T, true);
			field_grid->SetCellTextColour(i, COL_L,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_L, true);
			field_grid->SetCellTextColour(i, COL_D,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_D, true);
			field_grid->SetCellTextColour(i, COL_DD,
										  GeoDaConst::table_no_edit_color);
			field_grid->SetReadOnly(i, COL_DD, true);
		}
	}
	for (int i=0, iend=grid_base->GetNumberCols(); i<iend; i++) {
		DbfColContainer& cd = *(grid_base->col_data[col_id_map[i]]);
		if (fn_freq[cd.name.Upper()] > 1) {
			field_grid->SetCellTextColour(i, COL_N, *wxRED);
		} else {
			field_grid->SetCellTextColour(i, COL_N, *wxBLACK);
			if (grid_base->IsSpaceTimeIdField(cd.name)) {
				field_grid->SetCellTextColour(i, COL_N,
											  GeoDaConst::table_no_edit_color);
				field_grid->SetReadOnly(i, COL_N, true);
			}
		}
	}
	field_grid->EndBatch();

	field_grid->SetColLabelSize(wxGRID_AUTOSIZE);
	field_grid->SetColSize(COL_N, 135);

	for (int i=0, iend=field_grid->GetNumberRows(); i<iend; i++) {
		field_grid->EnableDragRowSize(false);
	}
	for (int i=0; i<NUM_COLS; i++) field_grid->EnableDragColSize(false);
	int field_grid_width = field_grid->GetRowLabelSize() + 30;
	for (int i=0; i<NUM_COLS; i++) field_grid_width +=field_grid->GetColSize(i);
	LOG(field_grid_width);
	int field_grid_height = field_grid->GetColLabelSize();
	int max_rows = field_grid->GetNumberRows();
	if (max_rows > 300) max_rows = 300;
	for (int i=0; i<max_rows; i++) {
		field_grid_height += field_grid->GetRowSize(i);
	}
	int min_height = field_grid_height < 300 ? field_grid_height : 300;
	field_grid->SetMinClientSize(wxSize(field_grid_width, min_height));
	field_grid->SetMinSize(wxSize(field_grid_width, min_height));
	// setting SetMaxSize to field_grid_width was causing wxAssert fail in Windows
	SetMaxSize(wxSize(-1, -1));
	
	top_sizer->Add(field_grid, 1, wxALIGN_CENTRE | wxBOTTOM, 3);
	
	g_sizer = new wxGridSizer(2, wxSize(3,0));
	
	m_max_label = new wxStaticText(this, wxID_ANY, "");
	m_max_val = new wxStaticText(this, wxID_ANY, "");
	m_min_label = new wxStaticText(this, wxID_ANY, "");
	m_min_val = new wxStaticText(this, wxID_ANY, "");
	
	//Add (wxWindow *window, int proportion=0, int flag=0, int border=0);
	g_sizer->Add(m_max_label, 0, wxALIGN_RIGHT | wxALL, 1);
	g_sizer->Add(m_max_val, 0, wxALIGN_LEFT | wxALL, 1);
	g_sizer->Add(m_min_label, 0, wxALIGN_RIGHT | wxALL, 1);
	g_sizer->Add(m_min_val, 0, wxALIGN_LEFT | wxALL, 1);

	top_sizer->Add(g_sizer, 0, wxALIGN_CENTRE, 0);
	
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	apply_button = new wxButton(this, wxID_APPLY, "Apply");
	apply_button->Enable(false);
	button_sizer->Add(apply_button, 0, wxALL, 5);
	button_sizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALL, 5);
	
	top_sizer->Add(button_sizer, 0, wxALIGN_CENTRE | wxTOP | wxBOTTOM, 5);	
	
	top_sizer->SetSizeHints(this);
	SetSizer(top_sizer);
	
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::CreateControls");	
}

// When this function is called, it is assumed that all values for length,
// decimals and displayed decimals are valid.
void DataViewerEditFieldPropertiesDlg::OnApplyButton( wxCommandEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnApplyButton");
	
	// check every row against data in grid_base to find changes and
	// make changes as needed.  Detect for possible truncation of data
	// when length is reduced or when length-decimals is reduced and
	// raise a warning dialog.  Actually, let's do this at the time
	// of cell editing and include a checkbox to suppress further
	// warnings.
	
	// When numberic data gets truncated, write out the maximum (or
	// minimum if negative) value out.
	
	// 2.9.2 feature
	//wxRichMessageDialog dlg((wxWindow*) this,
	//						wxString("Welcome to my program!"), wxString("foo"),
	//						wxOK|wxCENTER, wxDefaultPosition);
	//dlg.ShowCheckBox("Don't show this again");
	//dlg.ShowModal();
	
	//if (dlg.IsCheckBoxChecked()) {
		// do something
	//}
	//
	
	// first, check for duplicates and give option to abort
	for (std::map<wxString, int>::iterator it=fn_freq.begin();
		 it!=fn_freq.end(); it++) {
		if ((*it).second > 1) {
			wxString msg;
			msg << "Variable name \"" << (*it).first << "\" appears ";
			msg << (*it).second << " times in the Table. It is recommended ";
			msg << "to change names highlighted in red before proceeding. ";
			msg << "Ok to proceed?";
			wxMessageDialog dlg(this, msg, "Warning",
								wxICON_EXCLAMATION|wxYES_NO|wxNO_DEFAULT);
			if (dlg.ShowModal() != wxID_YES) {
				return;
			}
		}
	}
	
	// this can possibly be null, so always check before
	// dereferencing grid
	wxGrid* grid = grid_base->GetView();
	
	for (int id=0; id<field_grid->GetNumberRows(); id++) {
		DbfColContainer& cd = *(grid_base->col_data[col_id_map[id]]);
		wxString orig_name = cd.name.Upper();
		int orig_len = cd.field_len;
		int orig_dec = cd.decimals;
		int orig_ddec = cd.displayed_decimals;
		long val;
		field_grid->GetCellValue(id, COL_L).ToLong(&val);
		int new_len = val;
		int new_dec = 0;
		int new_ddec = 0;
		wxString new_name = field_grid->GetCellValue(id, COL_N).Upper();
		new_name.Trim(false);
		new_name.Trim(true);
		if (cd.type == GeoDaConst::double_type) {
			field_grid->GetCellValue(id, COL_D).ToLong(&val);
			new_dec = val;
			field_grid->GetCellValue(id, COL_DD).ToLong(&val);
			new_ddec = val;			
		}
		if (orig_len != new_len || orig_dec != new_dec ||
			orig_ddec != new_ddec || orig_name != new_name )
		{
			LOG(orig_len); LOG(new_len); LOG(orig_dec); LOG(new_dec);
			LOG(orig_ddec); LOG(new_ddec); LOG(orig_name); LOG(new_name);
			LOG_MSG(wxString::Format("field %d (col_id %d) changed", id,
									 col_id_map[id]));
			bool r = cd.ChangeProperties(new_name, new_len, new_dec, new_ddec);
			if (!r) {
				wxString msg;
				msg << "Variable Properties update for field ";
				msg << cd.name << " failed. Keeping original values for ";
				msg << "this field.";
				wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
				dlg.ShowModal();
				field_grid->SetCellValue(id, COL_N, orig_name);
				field_grid->SetCellValue(id, COL_L,
										 wxString::Format("%d", orig_len));
				if (cd.type == GeoDaConst::double_type) {
					field_grid->SetCellValue(id, COL_D,
											 wxString::Format("%d", orig_dec));
					field_grid->SetCellValue(id, COL_DD,
											 wxString::Format("%d", orig_ddec));
				}
			} else if (new_ddec != orig_ddec || new_dec != orig_dec) {
				// update floating point col properties in the wxGrid
				// it seems strange that we have to use the mapped id
				// here rather than the order visible in the grid
				if (grid) grid->SetColFormatFloat(col_id_map[id], -1,
										GenUtils::min<int>(new_ddec, new_dec));
			}
		}
	}
	ShowFieldProperties(-1); // reset field properties to blank
	apply_button->Enable(false);
	if (grid) grid->Refresh();
	ev.Skip();
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnApplyButton");
}


void DataViewerEditFieldPropertiesDlg::OnCloseButton( wxCommandEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnCloseButton");
	ev.Skip();
	Close();
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnCloseButton");
}

void DataViewerEditFieldPropertiesDlg::OnClose( wxCloseEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnClose");
	if (cell_editor_open) return;
	if (apply_button->IsEnabled()) {
		wxString msg("Ok to close dialog without applying changes?");
		wxMessageDialog dlg(this, msg, "Unsaved Changes",
							wxCANCEL| wxYES_NO | wxYES_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) {
			return;
		}
	}
	ev.Skip();
	EndDialog(wxID_OK);
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnClose");
}


void DataViewerEditFieldPropertiesDlg::OnCellEdit( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnCellEdit");
	int row = ev.GetRow();
	int col = ev.GetCol();
	wxString cur_str = field_grid->GetCellValue(row, col);
	cur_str.Trim(false);
	cur_str.Trim(true);
	wxString new_str = ev.GetString();
	new_str.Trim(false);
	new_str.Trim(true);
	long cur_val=0, new_val=0;
	if (col == COL_L || col == COL_D || col == COL_DD) {
		cur_str.ToLong(&cur_val);
		new_str.ToLong(&new_val);
	}
	
	if (row > -1 && col > -1) {
		LOG_MSG(wxString::Format("%s -> %s",
						(const_cast<char*>((const char*)
								field_grid->GetCellValue(row, col).mb_str())),
						(const_cast<char*>((const char*)
								ev.GetString().mb_str()))));
		DbfColContainer& cd = *(grid_base->col_data[col_id_map[row]]);
		if (cd.type != GeoDaConst::double_type && col > COL_L) {
			// this should never happen since non double_type cells in cols
			// COL_D and COL_DD were set as read-only
			ev.Veto();
			return;
		}
		if (cur_str == new_str) {
			// nothing changed, so don't enable Apply button
			ev.Veto();
			return;
		}
		long min_v;
		long max_v;
		if (col == COL_N) {
			new_str.MakeUpper();
			if (!DbfFileUtils::isValidFieldName(new_str)) {
				wxString msg;
				msg << "\"" << new_str << "\" is an invalid ";
				msg << "variable name.  ";
				msg << "A valid variable name is between one and ";
				msg << "ten characters long.  The first character must be ";
				msg << "alphabetic, and the remaining characters can be ";
				msg << "either alphanumeric or underscores.";
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				ev.Veto();
				return;
			}
			fn_freq.clear();
			for (int i=0, iend=field_grid->GetNumberRows(); i<iend; i++) {
				std::map<wxString, int>::iterator it;
				wxString key = field_grid->GetCellValue(i, COL_N).Upper();
				if (i==row) key = new_str;
				it = fn_freq.find(key);
				if (it != fn_freq.end()) {
					(*it).second++;
				} else {
					fn_freq[key] = 1;
				}
			}
			if (fn_freq[new_str] > 1) {
				wxString msg;
				msg << "\"" << new_str <<"\" is a duplicate variable name and ";
				msg << "cannot be used, please choose a non-duplicate field ";
				msg << "name.";
				wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
				dlg.ShowModal();
				ev.Veto();
				return;
			}
			for (int i=0, iend=field_grid->GetNumberRows(); i<iend; i++) {
				wxString key = field_grid->GetCellValue(i, COL_N).Upper();
				if (fn_freq[key] > 1) {
					field_grid->SetCellTextColour(i, COL_N, *wxRED);
				} else {
					field_grid->SetCellTextColour(i, COL_N, *wxBLACK);
				}
			}
		} else if (col == COL_L) {
			if (cd.type == GeoDaConst::date_type) {
				min_v = GeoDaConst::min_dbf_date_len;
				max_v = GeoDaConst::max_dbf_date_len;
			} else if (cd.type == GeoDaConst::long64_type) {
				min_v = GeoDaConst::min_dbf_long_len;
				max_v = GeoDaConst::max_dbf_long_len;
			} else if (cd.type == GeoDaConst::double_type) {
				min_v = GeoDaConst::min_dbf_double_len;
				max_v = GeoDaConst::max_dbf_double_len;
			} else  { // cd.type == GeoDaConst::string_type
				min_v = GeoDaConst::min_dbf_string_len;
				max_v = GeoDaConst::max_dbf_string_len;
			}
			if (cd.type == GeoDaConst::string_type) {
				if (new_val < min_v || max_v < new_val) {
					wxString msg;
					msg << "The length of a string field must be at least ";
					msg << min_v << " and at most " << max_v;
					msg << ". Keeping original value.";
					wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
					dlg.ShowModal();
					ev.Veto();
					return;
				}
			} else if (cd.type == GeoDaConst::date_type) {
				// should never get here since date_type has no editable fields
				ev.Veto();
			} else if (cd.type == GeoDaConst::long64_type) {
				if (new_val < min_v || max_v < new_val) {
					wxString msg;
					msg << "The length of an integral numeric field must be";
					msg << " at least " << min_v << " and at most " << max_v;
					msg << ". Keeping original value.";
					wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
					dlg.ShowModal();
					ev.Veto();
					return;
				}
			} else { // cd.type == GeoDaConst::double_type 
				if (new_val < min_v || max_v < new_val) {
					wxString msg;
					msg << "The length of a non-integral numeric field must be";
					msg << " at least " << min_v << " and at most " << max_v;
					msg << ". Keeping original value.";
					wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
					dlg.ShowModal();
					ev.Veto();
					return;
				}
				long cur_dec = 0;
				field_grid->GetCellValue(row, COL_D).ToLong(&cur_dec);
				// Automatically tweak length and decimals to acceptable values 
				int suggest_len;
				int suggest_dec;
				DbfFileUtils::SuggestDoubleParams(new_val, cur_dec,
												  &suggest_len, &suggest_dec);
				field_grid->SetCellValue(row, COL_L,
										 wxString::Format("%d", suggest_len));
				field_grid->SetCellValue(row, COL_D,
										 wxString::Format("%d", suggest_dec));
				// Since we explictly set the value of both cells above,
				// we need to veto the event in case we want to write a
				// different value than the user chose.
				ev.Veto();
			}
		} else if (col == COL_D) { // we know this is a double_type field
			min_v = GeoDaConst::min_dbf_double_decimals;
			max_v = GeoDaConst::max_dbf_double_decimals;
			if (new_val < min_v || max_v < new_val) {
				wxString msg;
				msg << "The number of decimal places for a non-integral ";
				msg << "numeric field must be at least " << min_v;
				msg << " and at most " << max_v;
				msg << ". Keeping original value.";
				wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
				dlg.ShowModal();
				ev.Veto();
				return;
			}
			long cur_len = 0;
			field_grid->GetCellValue(row, COL_L).ToLong(&cur_len);
			// Automatically tweak length and decimals to acceptable values
			int suggest_len;
			int suggest_dec;
			DbfFileUtils::SuggestDoubleParams(cur_len, new_val,
											  &suggest_len, &suggest_dec);
			field_grid->SetCellValue(row, COL_L,
									 wxString::Format("%d", suggest_len));
			field_grid->SetCellValue(row, COL_D,
									 wxString::Format("%d", suggest_dec));
			// Since we explictly set the value of both cells above,
			// we need to veto the event in case we want to write a
			// different value than the user chose.
			ev.Veto();
		} else if (col == COL_DD) { // we know this is a double_type field
			min_v = 0;
			max_v = GeoDaConst::max_dbf_double_decimals;
			if (new_val < min_v || max_v < new_val) {
				wxString msg;
				msg << "The number of displayed decimal places for a ";
				msg << "non-integral numeric field must be at least " << min_v;
				msg << " and at most " << max_v;
				msg << " Keeping original value.";
				wxMessageDialog dlg(this, msg, "Error", wxOK|wxICON_ERROR);
				dlg.ShowModal();
				ev.Veto();
				return;
			}
		}
		// if the code execution makes it this far, then a cell value of the
		// table has been changed.
		apply_button->Enable(true);
		if (cd.type == GeoDaConst::double_type) {
			// it is possible that the values for length and decimals are
			// individually valid, but that the combination of values is not
			// valid.  If this happens, then change these cells in the table to
			// red.  Otherwise, format them both as black.
			long length;
			field_grid->GetCellValue(row, COL_L).ToLong(&length);
			long decimals;
			field_grid->GetCellValue(row, COL_D).ToLong(&decimals);
			int suggest_len;
			int suggest_dec;
			DbfFileUtils::SuggestDoubleParams(length, decimals,
											  &suggest_len, &suggest_dec);
			if (length != suggest_len || decimals != suggest_dec) {
				// set length and decimals cell text color to red
				field_grid->SetCellTextColour(row, COL_L, *wxRED);
				field_grid->SetCellTextColour(row, COL_D, *wxRED);
			} else {
				field_grid->SetCellTextColour(row, COL_L, *wxBLACK);
				field_grid->SetCellTextColour(row, COL_D, *wxBLACK);
			}
		}
		ShowFieldProperties(row);
	}
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnCellEdit");
}

void DataViewerEditFieldPropertiesDlg::OnCellClickLeft( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnCellClickLeft");
	int row = ev.GetRow();
	int col = ev.GetCol();
	LOG(row);
	LOG(col);
	ShowFieldProperties(row);
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnCellClickLeft");
	ev.Skip();
}

void DataViewerEditFieldPropertiesDlg::OnCellEditorShown( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnCellEditorShown");
	cell_editor_open = true;
	if (apply_button->IsEnabled()) {
		reenable_apply_after_cell_editor_hidden = true;
		apply_button->Enable(false);
	} else {
		reenable_apply_after_cell_editor_hidden = false;
	}
	int row = ev.GetRow();
	int col = ev.GetCol();
	LOG(row);
	LOG(col);
	ShowFieldProperties(row);
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnCellEditorShown");
	ev.Skip();
}

void DataViewerEditFieldPropertiesDlg::OnCellEditorHidden( wxGridEvent& ev )
{
	LOG_MSG("In DataViewerEditFieldPropertiesDlg::OnCellEditorHidden");
	if (reenable_apply_after_cell_editor_hidden) {
		apply_button->Enable(true);
		reenable_apply_after_cell_editor_hidden = false;
	}
	cell_editor_open = false;
	ev.Skip();
}

void DataViewerEditFieldPropertiesDlg::OnLabelLeftClickEvent(wxGridEvent& ev)
{
	LOG_MSG("Entering "
			"DataViewerEditFieldPropertiesDlg::OnLabelLeftClickEvent");
	int row = ev.GetRow();
	int col = ev.GetCol();
	LOG(row);
	LOG(col);
	ShowFieldProperties(row);
	LOG_MSG("Exiting "
			"DataViewerEditFieldPropertiesDlg::OnLabelLeftClickEvent");
	ev.Skip();
}

void DataViewerEditFieldPropertiesDlg::ShowFieldProperties(int row)
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::ShowFieldProperties");
	int mrow = row < 0 ? -1 : col_id_map[row];
	if (mrow >= 0 &&
		(grid_base->col_data[mrow]->type == GeoDaConst::long64_type
		 || grid_base->col_data[mrow]->type == GeoDaConst::double_type))
	{
		m_max_label->SetLabelText(grid_base->col_data[mrow]->name + " maximum");
		m_min_label->SetLabelText(grid_base->col_data[mrow]->name + " minimum");
		long length;
		field_grid->GetCellValue(row, COL_L).ToLong(&length);
		if (grid_base->col_data[mrow]->type == GeoDaConst::double_type) {
			long decimals;
			field_grid->GetCellValue(row, COL_D).ToLong(&decimals);
			int suggest_len;
			int suggest_dec;
			DbfFileUtils::SuggestDoubleParams(length, decimals,
											  &suggest_len, &suggest_dec);
			if (length == suggest_len && decimals == suggest_dec) {
				m_max_val->SetLabelText(
					DbfFileUtils::GetMaxDoubleString(length, decimals));
				m_min_val->SetLabelText(
					DbfFileUtils::GetMinDoubleString(length, decimals));
			} else {
				m_max_val->SetLabelText("");
				m_min_val->SetLabelText("");
			}
		} else { // GeoDaConst::long64_type
			m_max_val->SetLabelText(DbfFileUtils::GetMaxIntString(length));
			m_min_val->SetLabelText(DbfFileUtils::GetMinIntString(length));
		}
	} else {	
		m_max_label->SetLabelText("");
		m_max_val->SetLabelText("");
		m_min_label->SetLabelText("");
		m_min_val->SetLabelText("");
	}
	g_sizer->Layout();	
	
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::ShowFieldProperties");
}
