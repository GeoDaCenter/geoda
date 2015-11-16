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

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "../logger.h"
#include "../Project.h"
#include "../GeoDa.h"
#include "../DbfFile.h"
#include "../FramesManager.h"
#include "TableInterface.h"
#include "TableState.h"
#include "DataViewerEditFieldPropertiesDlg.h"
#include "../GdaException.h"

BEGIN_EVENT_TABLE( DataViewerEditFieldPropertiesDlg, wxDialog )
	EVT_GRID_EDITOR_SHOWN(
		DataViewerEditFieldPropertiesDlg::OnCellEditorShown )
	EVT_GRID_EDITOR_HIDDEN(
		DataViewerEditFieldPropertiesDlg::OnCellEditorHidden )
	EVT_BUTTON( wxID_CLOSE,
			   DataViewerEditFieldPropertiesDlg::OnCloseButton )
	EVT_CLOSE( DataViewerEditFieldPropertiesDlg::OnClose )
	EVT_GRID_CELL_CHANGING(DataViewerEditFieldPropertiesDlg::OnCellChanging) 
END_EVENT_TABLE()

DataViewerEditFieldPropertiesDlg::DataViewerEditFieldPropertiesDlg(
				Project* project_s,
				const wxPoint &pos, const wxSize &size )
: wxDialog(0, wxID_ANY, "Variable Properties", pos, size,
		   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
project(project_s), table_int(project_s->GetTableInt()),
cell_editor_open(false),
field_grid(0), frames_manager(project_s->GetFramesManager()),
table_state(project_s->GetTableState())
{
	// determine columns to show and assign ids
	NUM_COLS = 0;
	COL_N = NUM_COLS++; // field name
	COL_T = NUM_COLS++; // type
	COL_PG = NUM_COLS++; // parent group name
	COL_TM = NUM_COLS++; // time period
	COL_L = -1; // length
	if (table_int->HasFixedLengths()) COL_L = NUM_COLS++;
	COL_D = -1; // decimal places
	if (table_int->HasFixedDecimals()) COL_D = NUM_COLS++;
	COL_DD = -1; // displayed decimal places
	if (table_int->PermitChangeDisplayedDecimals()) COL_DD = NUM_COLS++;
	
	COL_MIN = -1; // min value possible
	if (table_int->HasFixedDecimals()) COL_MIN = NUM_COLS++;
	COL_MAX = -1; // max value possible
	if (table_int->HasFixedDecimals()) COL_MAX = NUM_COLS++;
	
    CreateControls();
	SetTitle("Variable Properties - " + table_int->GetTableName());
    Centre();
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
}

DataViewerEditFieldPropertiesDlg::~DataViewerEditFieldPropertiesDlg()
{
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
}

void DataViewerEditFieldPropertiesDlg::CreateControls()
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::CreateControls");
	
	//wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	field_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(200,400));
	field_grid->CreateGrid(table_int->GetNumberCols(), NUM_COLS,
						   wxGrid::wxGridSelectRows);
	if (COL_N!=-1) field_grid->SetColLabelValue(COL_N, "variable name");
	if (COL_T!=-1) field_grid->SetColLabelValue(COL_T, "type");
	if (COL_L!=-1) field_grid->SetColLabelValue(COL_L, "length");
	if (COL_D!=-1) field_grid->SetColLabelValue(COL_D, "decimal\nplaces");
	if (COL_DD!=-1) field_grid->SetColLabelValue(COL_DD,
												 "displayed\ndecimal places");
	if (COL_PG!=-1) field_grid->SetColLabelValue(COL_PG, "parent group");
	if (COL_TM!=-1) field_grid->SetColLabelValue(COL_TM, "time");
	if (COL_MIN!=-1) field_grid->SetColLabelValue(COL_MIN, "minimum\npossible");
	if (COL_MAX!=-1) field_grid->SetColLabelValue(COL_MAX, "maximum\npossible");

	field_grid->HideRowLabels();
	
	InitTable();

	field_grid->AutoSize();

	for (int i=0, iend=field_grid->GetNumberRows(); i<iend; i++) {
		field_grid->EnableDragRowSize(false);
	}
	for (int i=0; i<NUM_COLS; i++) field_grid->EnableDragColSize(true);
	
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::CreateControls");	
}

void DataViewerEditFieldPropertiesDlg::InitTable()
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::InitTable");
	if (!field_grid) return;
	LOG(table_int->IsReadOnly());
	LOG(table_int->PermitRenameSimpleCol());
	LOG(table_int->HasFixedLengths());
	LOG(table_int->PermitChangeLength());
	LOG(table_int->HasFixedDecimals());
	LOG(table_int->PermitChangeDecimals());
	LOG(table_int->PermitChangeDisplayedDecimals());
	
	field_grid->DeleteRows(0, field_grid->GetNumberRows());
	fn_freq.clear();
	
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	std::vector<int> col_id_map;
	table_int->FillColIdMap(col_id_map);
	
	UpdateTmStrMap();
	field_grid->BeginBatch();
	if (field_grid->GetNumberRows() > 0) {
		field_grid->DeleteRows(0, field_grid->GetNumberRows());
	}
	int r = -1; // current row number
	for (int c=0, cend=table_int->GetNumberCols(); c<cend; ++c) {
		int cid = col_id_map[c];
		
		for (int t=0, tend=table_int->GetColTimeSteps(cid); t<tend; ++t) {
			if (table_int->GetColType(cid, t) == GdaConst::placeholder_type) {
				continue;
			}
			GdaConst::FieldType type = table_int->GetColType(cid, t);
			field_grid->AppendRows(1);
			++r; // increment current row number
			
			// variable name
			wxString name = table_int->GetColName(cid, t);
			std::map<wxString, int>::iterator it = fn_freq.find(name);
			if (fn_freq.find(name) != fn_freq.end()) {
				fn_freq[name]++;
			} else {
				fn_freq[name] = 1;
			}
			field_grid->SetCellValue(r, COL_N, name);
			field_grid->SetCellAlignment(r, COL_N, wxALIGN_CENTRE,
										 wxALIGN_CENTRE);
			if (!table_int->PermitRenameSimpleCol()) {
				field_grid->SetCellTextColour(r, COL_N,
											  GdaConst::table_no_edit_color);
				field_grid->SetReadOnly(r, COL_N, true);
			}
			if (tend > 1) {
				// parent group
				wxString pg = table_int->GetColName(cid);
				field_grid->SetCellValue(r, COL_PG, pg);
				field_grid->SetCellAlignment(r, COL_PG, wxALIGN_CENTRE,
											 wxALIGN_CENTRE);
				
				// time
				field_grid->SetCellValue(r, COL_TM,
										 table_int->GetTimeString(t));
			} else {
				field_grid->SetCellTextColour(r, COL_PG,
											  GdaConst::table_no_edit_color);
				field_grid->SetReadOnly(r, COL_PG, true);
			}
			field_grid->SetCellAlignment(r, COL_TM, wxALIGN_CENTRE,
										 wxALIGN_CENTRE);
			field_grid->SetCellTextColour(r, COL_TM,
										  GdaConst::table_no_edit_color);
			field_grid->SetReadOnly(r, COL_TM, true);
		
			// type
			field_grid->SetCellAlignment(r, COL_T, wxALIGN_CENTRE,
										 wxALIGN_CENTRE);
			field_grid->SetCellTextColour(r, COL_T,
										  GdaConst::table_no_edit_color);
			field_grid->SetReadOnly(r, COL_T, true);
			if (type == GdaConst::double_type) {
				field_grid->SetCellValue(r, COL_T, "real");
			} else if (type == GdaConst::long64_type) {
				field_grid->SetCellValue(r, COL_T, "integer");
			} else if (type == GdaConst::date_type) {
				field_grid->SetCellValue(r, COL_T, "date");
			} else {
				field_grid->SetCellValue(r, COL_T, "string");
			}
			
			// length
			if (COL_L != -1) {
				bool can_edit = (table_int->PermitChangeLength() &&
								 (type == GdaConst::double_type ||
								  type == GdaConst::long64_type ||
								  type == GdaConst::string_type));
				if (can_edit) {
					field_grid->SetCellTextColour(r, COL_L,
											GdaConst::table_no_edit_color);
				} else {
					field_grid->SetCellTextColour(r, COL_L, *wxBLACK);
				}
				field_grid->SetReadOnly(r, COL_L, !can_edit);
				wxString lv;
				lv << table_int->GetColLength(cid, t);
				field_grid->SetCellValue(r, COL_L, lv);
				field_grid->SetColFormatNumber(COL_L);
			}
			
			// decimals
			if (COL_D != -1) {
				if (type != GdaConst::double_type ||
					!table_int->PermitChangeDecimals()) {
					field_grid->SetCellTextColour(r, COL_D,
												  GdaConst::table_no_edit_color);
					field_grid->SetReadOnly(r, COL_D, true);
				} else {
					field_grid->SetCellTextColour(r, COL_D, *wxBLACK);
					field_grid->SetReadOnly(r, COL_D, false);
				}
				if (type == GdaConst::double_type) {
					wxString dv;
					dv << table_int->GetColDecimals(cid, t);
					field_grid->SetCellValue(r, COL_D, dv);
					field_grid->SetColFormatNumber(COL_D);
				}
			}
			
			// displayed decimals
			if (COL_DD != -1) {
				if (type != GdaConst::double_type ||
					!table_int->PermitChangeDisplayedDecimals()) {
					field_grid->SetCellTextColour(r, COL_DD,
												  GdaConst::table_no_edit_color);
					field_grid->SetReadOnly(r, COL_DD, true);
				} else {
					field_grid->SetCellTextColour(r, COL_DD, *wxBLACK);
					field_grid->SetReadOnly(r, COL_DD, false);
				}
				if (type == GdaConst::double_type) {
					wxString ddv;
					if (table_int->GetColDispDecimals(cid) > 0) {
						ddv << table_int->GetColDispDecimals(cid);
					} else {
						// otherwise default (-1) shown as ""
						ddv = "";
					}
					field_grid->SetCellValue(r, COL_DD, ddv);
					field_grid->SetColFormatNumber(COL_DD);
				}
			}
			
			UpdateMinMax(r);
			if (COL_MIN != -1 && COL_MAX != -1) {
				field_grid->SetReadOnly(r, COL_MIN, true);
				field_grid->SetReadOnly(r, COL_MAX, true);
				field_grid->SetCellTextColour(r, COL_MIN,
											  GdaConst::table_no_edit_color);
				field_grid->SetCellTextColour(r, COL_MAX,
											  GdaConst::table_no_edit_color);
				field_grid->SetCellAlignment(r, COL_MIN, wxALIGN_CENTRE,
											 wxALIGN_CENTRE);
				field_grid->SetCellAlignment(r, COL_MAX, wxALIGN_CENTRE,
											 wxALIGN_CENTRE);
			}
		}
	}
	for (int i=0, iend=table_int->GetNumberCols(); i<iend; i++) {
		int cid = col_id_map[i];
		if (fn_freq[table_int->GetColName(cid).Upper()] > 1) {
			field_grid->SetCellTextColour(i, COL_N, *wxRED);
		} else {
			field_grid->SetCellTextColour(i, COL_N, *wxBLACK);
		}
	}
	field_grid->EndBatch();
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::InitTable");
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
	// MMM: This was preventing GeoDa from exiting properly when
	// the cell editor was open.
	//if (cell_editor_open) return;
	Destroy();
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnClose");
}


void DataViewerEditFieldPropertiesDlg::OnCellChanging( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerEditFieldPropertiesDlg::OnCellChanging");
	int row = ev.GetRow();
	int col = ev.GetCol();
	if (row < 0 || col < 0) {
		ev.Veto();
		return;
	}
	int time = 0;
	wxString pg = field_grid->GetCellValue(row, COL_PG);
	wxString name;
	if (pg.IsEmpty()) {
		name = field_grid->GetCellValue(row, COL_N);
	} else {
		name = pg;
		time = tm_str_map[field_grid->GetCellValue(row, COL_TM)];
	}
	int cid = table_int->FindColId(name);
	if (cid < 0) {
		ev.Veto();
		return;
	}
	LOG(cid);
	LOG(time);
	
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
	
	wxString msg;
	msg << "changing row " << row << ", col " << col << " from ";
	msg << cur_str << " to " << new_str;
	LOG_MSG(msg);
	
	GdaConst::FieldType type = GdaConst::unknown_type;
	wxString type_str = field_grid->GetCellValue(row, COL_T);
	if (type_str == "real") {
		type = GdaConst::double_type;
	} else if (type_str == "integer") {
		type = GdaConst::long64_type;
	} else if (type_str == "string") {
		type = GdaConst::string_type;
	} else if (type_str == "date") {
		type = GdaConst::date_type;
	} else {
		ev.Veto();
		LOG_MSG("Unknown field type in properties table.");
		return;
	}
	
	
	if (col == COL_T || col == COL_TM || col == COL_MIN || col == COL_MAX ||
		(type != GdaConst::double_type && (col == COL_D || col == COL_DD))) {
		ev.Veto();
		LOG_MSG("illegal cell to edit.");
		return;
	}
	
	if (cur_str == new_str || new_str.IsEmpty()) {
		ev.Veto();
		LOG_MSG("empty new string or cell unchanged.");
		return;
	}

	if (col == COL_D || col == COL_DD || col == COL_L) {
		if (!Project::CanModifyGrpAndShowMsgIfNot(table_state, pg)) {
			ev.Veto();
			return;
		}
	}
	
	long min_v;
	long max_v;
	if (col == COL_N) {
		if (table_int->DoesNameExist(new_str, false) ||
			!table_int->IsValidDBColName(new_str)) {
			wxString m;
			m << "Variable name \"" << new_str << "\" is either a duplicate ";
			m << "or is invalid. Please enter an alternative, non-duplicate ";
			m << "variable name. A valid variable name is between one and ";
			m << "ten characters long. The first character must be a letter, ";
			m << "and the remaining characters can be either letters, ";
			m << "numbers or underscores.";
			wxMessageDialog dlg(this, m, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			ev.Veto();
			return;
		}
		// proceed with rename
		table_int->RenameSimpleCol(cid, time, new_str);
	} else if (col == COL_PG) {
		if (new_str.IsEmpty()) {
			LOG_MSG("empty parent group.  vetoing name change.");
			ev.Veto();
			return;
		}
		if (table_int->DoesNameExist(new_str, false) ||
			!table_int->IsValidGroupName(new_str)) {
			wxString m;
			m << "Variable name \"" << new_str << "\" is either a duplicate ";
			m << "or is invalid. Please enter an alternative, non-duplicate ";
			m << "variable name.";
			wxMessageDialog dlg(this, m, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			ev.Veto();
			return;
		}
		// proceed with rename
		table_int->RenameGroup(cid, new_str);
	} else if (col == COL_L &&
			   table_int->HasFixedLengths() &&
			   table_int->PermitChangeLength()) {
		if (table_int->GetColType(cid) == GdaConst::date_type) {
			min_v = GdaConst::min_dbf_date_len;
			max_v = GdaConst::max_dbf_date_len;
		} else if (table_int->GetColType(cid) == GdaConst::long64_type) {
			min_v = GdaConst::min_dbf_long_len;
			max_v = GdaConst::max_dbf_long_len;
		} else if (table_int->GetColType(cid) == GdaConst::double_type) {
			min_v = GdaConst::min_dbf_double_len;
			max_v = GdaConst::max_dbf_double_len;
		} else  { // table_int->GetColType(cid) == GdaConst::string_type
			min_v = GdaConst::min_dbf_string_len;
			max_v = GdaConst::max_dbf_string_len;
		}
		if (table_int->GetColType(cid) == GdaConst::string_type) {
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
			// proceed with length change
			table_int->ColChangeProperties(cid, time, new_val);
		} else if (table_int->GetColType(cid) == GdaConst::date_type) {
			// should never get here since date_type has no editable fields
			ev.Veto();
		} else if (table_int->GetColType(cid) == GdaConst::long64_type) {
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
			// proceed with length change
			table_int->ColChangeProperties(cid, time, new_val);
		} else { // table_int->GetColType(cid) == GdaConst::double_type 
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
			// proceed with length change
			table_int->ColChangeProperties(cid, time, suggest_len, suggest_dec);
		}
		ev.Veto();
	} else if (col == COL_D && table_int->PermitChangeDecimals()
			   && table_int->HasFixedLengths())
	{ // we know this is a double_type field
		min_v = GdaConst::min_dbf_double_decimals;
		max_v = GdaConst::max_dbf_double_decimals;
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
		table_int->ColChangeProperties(cid, time, suggest_len, suggest_dec);
        //todo: add to change decimals visually
        table_int->ColChangeDisplayedDecimals(cid, suggest_dec);
	} else if (col == COL_DD &&
			   table_int->PermitChangeDisplayedDecimals())
	{ // we know this is a double_type field
		if (new_val > 0) {
			if (table_int->HasFixedDecimals()) {
				min_v = 0;
				max_v = GdaConst::max_dbf_double_decimals;
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
			} else {
				if (new_val < 0 || new_val > 20) {
					ev.Veto();
					return;
				}
			}
		} else {
			ev.Veto();
			return;
			// new_val = -1; // Change back to default value
		}
		table_int->ColChangeDisplayedDecimals(cid, new_val);
	}
	// if the code execution makes it this far, then a cell value of the
	// table has been changed.
	GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
	
	if (table_int->GetColType(cid) == GdaConst::double_type &&
		table_int->HasFixedLengths() && COL_D != -1 && COL_L != -1) {
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
	UpdateMinMax(row);
	
	LOG_MSG("Exiting DataViewerEditFieldPropertiesDlg::OnCellChanging");
}

void DataViewerEditFieldPropertiesDlg::OnCellEditorShown( wxGridEvent& ev )
{
	LOG_MSG("In DataViewerEditFieldPropertiesDlg::OnCellEditorShown");
	cell_editor_open = true;
}

void DataViewerEditFieldPropertiesDlg::OnCellEditorHidden( wxGridEvent& ev )
{
	LOG_MSG("In DataViewerEditFieldPropertiesDlg::OnCellEditorHidden");
	cell_editor_open = false;
}

void DataViewerEditFieldPropertiesDlg::UpdateMinMax(int row)
{
	if (COL_MIN == -1 || COL_MAX == -1) return;
	if (row < 0) return;
	wxString pg = field_grid->GetCellValue(row, COL_PG);
	wxString name;
	if (pg.IsEmpty()) {
		name = field_grid->GetCellValue(row, COL_N);
	} else {
		name = pg;
	}
	
	int cid = table_int->FindColId(name);
	if (cid < 0) {
		field_grid->SetCellValue(row, COL_MAX, "");
		field_grid->SetCellValue(row, COL_MIN, "");
		return;
	}
	int t=0;
	wxString tm_str = field_grid->GetCellValue(row, COL_TM);
	LOG(tm_str);
	if (!tm_str.IsEmpty()) t = tm_str_map[tm_str];
	
	wxString smax;
	wxString smin;
	GdaConst::FieldType type = table_int->GetColType(cid, t);
	if (type == GdaConst::double_type || type == GdaConst::long64_type) {
		
		long length;
		field_grid->GetCellValue(row, COL_L).ToLong(&length);
		if (type == GdaConst::double_type) {
			long decimals;
			field_grid->GetCellValue(row, COL_D).ToLong(&decimals);
			int suggest_len;
			int suggest_dec;
			DbfFileUtils::SuggestDoubleParams(length, decimals,
											  &suggest_len, &suggest_dec);
			if (length == suggest_len && decimals == suggest_dec) {
				smax << DbfFileUtils::GetMaxDoubleString(length, decimals);
				smin << DbfFileUtils::GetMinDoubleString(length, decimals);
			}
		} else { // type == GdaConst::long64_type
			smax << DbfFileUtils::GetMaxIntString(length);
			smin << DbfFileUtils::GetMinIntString(length);
		}
	}
	field_grid->SetCellValue(row, COL_MAX, smax);
	field_grid->SetCellValue(row, COL_MIN, smin);
}

void DataViewerEditFieldPropertiesDlg::UpdateTmStrMap()
{
	using namespace std;
	tm_str_map.clear();
	vector<wxString> tm_strs;
	table_int->GetTimeStrings(tm_strs);
	for (int t=0, tt=tm_strs.size(); t<tt; ++t) tm_str_map[tm_strs[t]] = t;
}

void DataViewerEditFieldPropertiesDlg::update(FramesManager* o)
{
}

void DataViewerEditFieldPropertiesDlg::update(TableState* o)
{
	InitTable();
}
