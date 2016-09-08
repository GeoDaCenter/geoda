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
#include <wx/textdlg.h>
#include <wx/stopwatch.h>
#include <wx/xrc/xmlres.h>
#include "../GeoDa.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../Project.h"
#include "TimeState.h"
#include "TableBase.h"
#include "TableInterface.h"
#include "TableFrame.h"

BEGIN_EVENT_TABLE(TableFrame, TemplateFrame)
	EVT_ACTIVATE( TableFrame::OnActivate )
	EVT_CLOSE( TableFrame::OnClose )
	EVT_MENU( XRCID("wxID_CLOSE"), TableFrame::OnMenuClose )
	EVT_GRID_CELL_RIGHT_CLICK( TableFrame::OnRightClickEvent )
	EVT_GRID_LABEL_RIGHT_CLICK( TableFrame::OnRightClickEvent )
	EVT_GRID_LABEL_LEFT_CLICK( TableFrame::OnLabelLeftClickEvent )
	EVT_GRID_LABEL_LEFT_DCLICK( TableFrame::OnLabelLeftDClickEvent )
	EVT_GRID_COL_SIZE( TableFrame::OnColSizeEvent )
	EVT_GRID_COL_MOVE( TableFrame::OnColMoveEvent )
	EVT_GRID_CELL_CHANGED( TableFrame::OnCellChanged )

    //EVT_MENU(XRCID("ID_TABLE_GROUP"), TableFrame::OnGroupVariables)
    //EVT_MENU(XRCID("ID_TABLE_UNGROUP"), TableFrame::OnUnGroupVariable)
    EVT_MENU(XRCID("ID_TABLE_RENAME_VARIABLE"), TableFrame::OnRenameVariable)
    /*
	EVT_MENU(XRCID("ID_ENCODING_UTF8"), TableFrame::OnEncodingUTF8)
	EVT_MENU(XRCID("ID_ENCODING_UTF16"), TableFrame::OnEncodingUTF16)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1250"),
			 TableFrame::OnEncodingWindows1250)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1251"),
			 TableFrame::OnEncodingWindows1251)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1254"),
			 TableFrame::OnEncodingWindows1254)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1255"),
			 TableFrame::OnEncodingWindows1255)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1256"),
			 TableFrame::OnEncodingWindows1256)
	EVT_MENU(XRCID("ID_ENCODING_WINDOWS_1258"),
			 TableFrame::OnEncodingWindows1258)
	EVT_MENU(XRCID("ID_ENCODING_CP852"), TableFrame::OnEncodingCP852)
	EVT_MENU(XRCID("ID_ENCODING_CP866"), TableFrame::OnEncodingCP866)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_1"), TableFrame::OnEncodingISO8859_1)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_2"), TableFrame::OnEncodingISO8859_2)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_3"), TableFrame::OnEncodingISO8859_3)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_5"), TableFrame::OnEncodingISO8859_5)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_7"), TableFrame::OnEncodingISO8859_7)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_8_1"), TableFrame::OnEncodingISO8859_8)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_9"), TableFrame::OnEncodingISO8859_9)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_10"), TableFrame::OnEncodingISO8859_10)
	EVT_MENU(XRCID("ID_ENCODING_ISO8859_15"), TableFrame::OnEncodingISO8859_15)
	EVT_MENU(XRCID("ID_ENCODING_GB2312"), TableFrame::OnEncodingGB2312)
	EVT_MENU(XRCID("ID_ENCODING_BIG5"), TableFrame::OnEncodingBIG5)
	EVT_MENU(XRCID("ID_ENCODING_KOI8_R"), TableFrame::OnEncodingKOI8_R)
	EVT_MENU(XRCID("ID_ENCODING_SHIFT_JIS"), TableFrame::OnEncodingSHIFT_JIS)
	EVT_MENU(XRCID("ID_ENCODING_EUC_JP"), TableFrame::OnEncodingEUC_JP)
	EVT_MENU(XRCID("ID_ENCODING_EUC_KR"), TableFrame::OnEncodingEUC_KR)
    */
END_EVENT_TABLE()

TableFrame::TableFrame(wxFrame *parent, Project* project,
					   const wxString& title,
					   const wxPoint& pos, const wxSize& size,
					   const long style)
: TemplateFrame(parent, project, title, pos, size, style),
popup_col(-1)
{
	LOG_MSG("Entering TableFrame::TableFrame");
	
	DisplayStatusBar(true);
	wxString new_title(title);
	new_title << " - " << project->GetProjectTitle();
	SetTitle(new_title);
	supports_timeline_changes = true;
	table_base = new TableBase(project, this);
	TableInterface* table_int = project->GetTableInt();
	grid = new wxGrid(this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
	grid->SetDefaultColSize((grid->GetDefaultColSize() * 4)/3);
    
	// false to not take ownership, but this uncovers a bug in wxWidgets.
	// therefore, we'll have to let the wxGrid take ownership and make
	// sure that the TableFrame is only hidden until the
	// project is finally closed.
	grid->SetTable(table_base, true); 
	grid->EnableDragColMove(true);
	grid->EnableDragCell(false);
    
	// This line causes the row to disappear on Windows and Linux, but not OSX
	//grid->SetSelectionBackground(*wxWHITE);
	for (int i=0, iend=table_base->GetNumberRows(); i<iend; i++) {
		grid->DisableRowResize(i);
	}
    
	grid->SetSelectionMode(wxGrid::wxGridSelectRowsOrColumns);
	//grid->SetSelectionMode(wxGrid::wxGridSelectCells);
	wxStopWatch resize_time;
	for (int i=0, iend=table_base->GetNumberCols(); i<iend; i++) {
        
        GdaConst::FieldType col_type = table_int->GetColType(i);
        
		if (col_type == GdaConst::long64_type) {
			//grid->SetColFormatNumber(i);
            grid->SetColFormatFloat(i,-1,0);
            
		} else if (col_type == GdaConst::double_type) {
			grid->SetColFormatFloat(i, -1, table_int->GetColDispDecimals(i));
            
		} else if (col_type == GdaConst::date_type ||
                   col_type == GdaConst::time_type ||
                   col_type == GdaConst::datetime_type) {
			// leave as a string
		}
		grid->SetColSize(i, -1); // fit column width to lable width
	}
    
	int sample = GenUtils::min<int>(table_base->GetNumberRows(), 10);
    
	for (int i=0, iend=table_base->GetNumberCols(); i<iend; i++) {
		double cur_col_size = grid->GetColSize(i);
		double cur_lbl_len = grid->GetColLabelValue(i).length();
		double avg_cell_len = 0;
		for (int j=0; j<sample-1; ++j) {
			wxString cv = grid->GetCellValue(j, i);
			cv.Trim(true);
			cv.Trim(false);
			avg_cell_len += cv.length();
		}
		if (sample >= 1) { // sample last row
			avg_cell_len += grid->GetCellValue(table_base->GetNumberRows()-1, i).length();
		}
		avg_cell_len /= (double) sample;
		if (avg_cell_len > cur_lbl_len &&
			avg_cell_len >= 1 && cur_lbl_len >= 1) {
			LOG_MSG(wxString::Format("Resizing col %d", i));
			// attempt to scale up col width based on cur_col_size
			double fac = avg_cell_len / cur_lbl_len;
			fac *= 1.2;
			if (fac < 1) fac = 1;
			if (fac < 1.5 && fac > 1) fac = 1.5;
			if (fac > 5) fac = 5;
			grid->SetColSize(i, cur_col_size*fac);
			LOG(grid->GetColLabelValue(i));
			LOG(cur_col_size);
			LOG(cur_col_size*fac);
			LOG(cur_lbl_len);
			LOG(avg_cell_len);
			LOG(fac);
		} else {
			// add a few pixels of buffer to current label
			grid->SetColSize(i, cur_col_size+6);
		}
	}
	
    if (!project->IsFileDataSource()) {
        grid->DisableDragColMove();
    }
    
	LOG_MSG(wxString::Format("Column auto-resize time took %ld ms", resize_time.Time()));
	LOG_MSG("Exiting TableFrame::TableFrame");
}

TableFrame::~TableFrame()
{
	LOG_MSG("In TableFrame::~TableFrame");
	DeregisterAsActive();
}



void TableFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In TableFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("TableFrame", GetTitle());
	}
	event.Skip(false);
}

void TableFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering TableFrame::OnClose");
	if (!GdaFrame::GetGdaFrame()) {
		// This is an exit event, so allow close to proceed
		event.Skip();
		LOG_MSG("Exiting TableFrame::OnClose");
		return;
	}
	if (!GdaFrame::IsProjectOpen()) {
		LOG_MSG("In TableFrame::OnClose and actually closing.");
		event.Skip();
		// NOTE: We should not have to explicitly close the grid, but
		// if we don't an exception is thrown.  Very strange.  Hopefully
		// this doesn't cause a memory leak.
		
		//grid->Destroy();
		// Event handling halts when:
		//   - event.Skip() is not called
		//   - event.Skip(false) is called
		// Event handling continues when:
		//   - event.Skip() is called
		//   - event.Skip(true) is called
		//event.Skip(true);
		//Destroy();
		//Close(true);
	} else {
		LOG_MSG("In TableFrame::OnClose, but just hiding.");
		Hide();
	}
	LOG_MSG("Exiting TableFrame::OnClose");
}

void TableFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In TableFrame::OnMenuClose");
	Hide();
}

void TableFrame::MapMenus()
{
	LOG_MSG("In TableFrame::MapMenus");
	// Map Default Options Menus
    //wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
    wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_TABLE_VIEW_MENU_CONTEXT");
	GeneralWxUtils::ReplaceMenu(GdaFrame::GetGdaFrame()->GetMenuBar(),
								"Options", optMenu);
}

void TableFrame::DisplayPopupMenu( wxGridEvent& ev )
{
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_TABLE_VIEW_MENU_CONTEXT");
	
	TableInterface* ti = table_base->GetTableInt();
	SetEncodingCheckmarks(optMenu, ti->GetFontEncoding());
	popup_col = ev.GetCol();
	
	// Set Group item
	vector<int> sel_cols;
	table_base->GetSelectedCols(sel_cols);
	bool any_sel_time_variant = false;
	for (int i=0; i<sel_cols.size(); i++) {
		if (ti->IsColTimeVariant(sel_cols[i])) {
			any_sel_time_variant = true;
			break;
		}
	}
	bool all_sel_compatible = false;
	if (!any_sel_time_variant && sel_cols.size() > 1) {
		// check for compatible types
		all_sel_compatible = true;
		for (int i=0; i<sel_cols.size(); ++i) {
			if (!ti->IsColNumeric(sel_cols[i])) {
				all_sel_compatible = false;
				break;
			}
		}
	}
    /*
	if (any_sel_time_variant || !all_sel_compatible || sel_cols.size() <= 1 ||
		(ti->GetTimeSteps() > 1 && ti->GetTimeSteps() != sel_cols.size())) {
		optMenu->FindItem(XRCID("ID_TABLE_GROUP"))->Enable(false);
	} else {
		optMenu->FindItem(XRCID("ID_TABLE_GROUP"))->Enable(true);
	}
     
	// Set Ungroup item
	wxString ung_str("Ungroup Variable");
	bool ung_enable = false;
	if (popup_col >= 0 && ti->IsColTimeVariant(popup_col)) {
		wxString col_nm = ti->GetColName(popup_col);
		if (!col_nm.IsEmpty()) {
			ung_str = "Ungroup";
			ung_str << " \"" << col_nm << "\"";
			ung_enable = true;
		}
	}
	optMenu->FindItem(XRCID("ID_TABLE_UNGROUP"))->SetItemLabel(ung_str);
	optMenu->FindItem(XRCID("ID_TABLE_UNGROUP"))->Enable(ung_enable);
	*/
	// Set Rename item
	wxString rename_str("Rename Variable");
	if (popup_col != -1) {
		rename_str << " \"" << ti->GetColName(popup_col) << "\"";
	}
	optMenu->FindItem(XRCID("ID_TABLE_RENAME_VARIABLE"))->SetItemLabel(rename_str);
	bool enable_rename = false;
	if (popup_col!=-1) {
		if (ti->IsColTimeVariant(popup_col)) {
			enable_rename = true;
		} else {
			enable_rename = ti->PermitRenameSimpleCol();
		}
	}
	optMenu->FindItem(XRCID("ID_TABLE_RENAME_VARIABLE"))->Enable(enable_rename);
		
	PopupMenu(optMenu, ev.GetPosition());
}


void TableFrame::SetEncodingCheckmarks(wxMenu* m, wxFontEncoding e)
{
	m->FindItem(XRCID("ID_ENCODING_UTF8"))
		->Check(e==wxFONTENCODING_UTF8);
	m->FindItem(XRCID("ID_ENCODING_UTF16"))
		->Check(e==wxFONTENCODING_UTF16LE);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1250"))
		->Check(e==wxFONTENCODING_CP1250);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1251"))
		->Check(e==wxFONTENCODING_CP1251);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1254"))
		->Check(e==wxFONTENCODING_CP1254);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1255"))
		->Check(e==wxFONTENCODING_CP1255);
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1256"))
		->Check(e==wxFONTENCODING_CP1256);
	///MMM: works after 2.9.4 wxFONTENCODING_CP1258
	m->FindItem(XRCID("ID_ENCODING_WINDOWS_1258"))
		->Check(e==wxFONTENCODING_CP1258);
	m->FindItem(XRCID("ID_ENCODING_CP852"))
		->Check(e==wxFONTENCODING_CP852);
	m->FindItem(XRCID("ID_ENCODING_CP866"))
		->Check(e==wxFONTENCODING_CP866);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_1"))
		->Check(e==wxFONTENCODING_ISO8859_1);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_2"))
		->Check(e==wxFONTENCODING_ISO8859_2);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_3"))
		->Check(e==wxFONTENCODING_ISO8859_3);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_5"))
		->Check(e==wxFONTENCODING_ISO8859_5);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_7"))
		->Check(e==wxFONTENCODING_ISO8859_7);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_8"))
		->Check(e==wxFONTENCODING_ISO8859_8);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_9"))
		->Check(e==wxFONTENCODING_ISO8859_9);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_10"))
		->Check(e==wxFONTENCODING_ISO8859_10);
	m->FindItem(XRCID("ID_ENCODING_ISO_8859_15"))
		->Check(e==wxFONTENCODING_ISO8859_15);
	m->FindItem(XRCID("ID_ENCODING_GB2312"))
		->Check(e==wxFONTENCODING_GB2312);
	m->FindItem(XRCID("ID_ENCODING_BIG5"))
		->Check(e==wxFONTENCODING_BIG5);
	m->FindItem(XRCID("ID_ENCODING_KOI8_R"))
		->Check(e==wxFONTENCODING_KOI8);
	m->FindItem(XRCID("ID_ENCODING_SHIFT_JIS"))
		->Check(e==wxFONTENCODING_SHIFT_JIS);
	m->FindItem(XRCID("ID_ENCODING_EUC_JP"))->Check(e==wxFONTENCODING_EUC_JP);
	m->FindItem(XRCID("ID_ENCODING_EUC_KR"))->Check(e==wxFONTENCODING_EUC_KR);
}

void TableFrame::OnRightClickEvent( wxGridEvent& ev )
{
	DisplayPopupMenu(ev);
}

void TableFrame::OnColSizeEvent( wxGridSizeEvent& ev )
{
	LOG_MSG("Entering TableFrame::OnColSizeEvent");
	LOG(ev.GetRowOrCol());
	LOG(ev.GetPosition().x);
	LOG(ev.GetPosition().y);
	ev.Veto();
	LOG_MSG("Exiting TableFrame::OnColSizeEvent");
}

void TableFrame::OnColMoveEvent( wxGridEvent& ev )
{
	LOG_MSG("Entering TableFrame::OnColMoveEvent");
	LOG(ev.GetCol());
	LOG(ev.GetPosition().x);
	LOG(ev.GetPosition().y);	
	table_base->notifyColMove();
	LOG_MSG("Exiting TableFrame::OnColMoveEvent");
}

/**
 * The Table row and column selection behaviour follows the conventions
 * for file selection on Mac/Windows/Linux:
 *   - A single click selects a row/col and deselects everything else.
 *   - Cmd (Ctrl on Win/Lin) click allows multiple item selection/deselection
 *   - Shift click selects a range of items and deselects everything outside
 *     of that range.
 *
 * Notes:
 *   - ev.GetRow() refers to the displayed row numbers in the table, not
 *     the possibly permuted rows.  However, the TableBase::FromGrid___ 
 *     take care of this
 *   - ev.GetCol() refers to the permuted columns, not necessarily the
 *     visible column order since wxGrid hides column moves from
 *     the underlying wxGridTableBase.  This somewhat complicates
 *     shift-click selection for columns.
 *
 * The proper shift-selection function requires a anchor point stack.  We
 * have implemented a stateless shift select which feels quite logical in
 * practice.
 */
void TableFrame::OnLabelLeftClickEvent( wxGridEvent& ev )
{
	using namespace std;
	LOG_MSG("Entering TableFrame::OnLabelLeftClickEvent");
	LOG(ev.GetCol());
	LOG(ev.GetRow());
	LOG(ev.ShiftDown());
	LOG(ev.CmdDown());
	int row = ev.GetRow();
	int col = ev.GetCol();
	TableInterface* table_int = project->GetTableInt();
	int rows = table_int->GetNumberRows();
	int cols = table_int->GetNumberCols();
	if (col < 0 && row >= 0) {
		if (!ev.ShiftDown() && !ev.CmdDown()) {
			table_base->FromGridSelectOnlyRow(row);
		} else if (!ev.ShiftDown()) {
			if (table_base->FromGridIsSelectedRow(row)) {
				table_base->FromGridDeselectRow(row);
			} else {
				table_base->FromGridSelectRow(row);
			}
		} else {
			// shift down
			bool sel_found = false;
			int first_sel = -1;
			int last_sel = -1;
			for (int i=0; i<rows; i++) {
				if (table_base->FromGridIsSelectedRow(i)) {
					if (!sel_found) {
						first_sel = i;
						last_sel = i;
						sel_found = true;
					}
					if (i < first_sel) first_sel = i;
					if (i > last_sel) last_sel = i;
				}
			}
			
			if (!sel_found) {
				first_sel = row;
				last_sel = row;
			} else if (row <= first_sel) {
				last_sel = first_sel;
				first_sel = row;
			} else if (row >= last_sel) {
				first_sel = last_sel;
				last_sel = row;
			} else {
				// in the middle, so leave endpoints as they are
			}
						
			table_base->FromGridSelectRowRange(first_sel, last_sel);
		}
		// unselect whatever the wxGrid object selects since we are doing our
		// own selection.
		grid->ClearSelection();
		grid->Refresh();
	} else if (col >= 0) {
        // deal with column selection
		if (!ev.ShiftDown() && !ev.CmdDown()) table_base->DeselectAllCols();
		if (!ev.ShiftDown()) {
			if (table_base->FromGridIsSelectedCol(col)) {
				table_base->FromGridDeselectCol(col);
			} else {
				table_base->FromGridSelectCol(col);
			}
		} else {
			// shift down.  For columns, we need to work with displayed
			// order.  So, need to get a translation from displayed col to col
			// and col to displayed col.
			vector<int> col_to_dispc(cols);
			vector<int> dispc_to_col(cols);
			for (int i=0; i<cols; i++) col_to_dispc[i] = grid->GetColPos(i);
			for (int i=0; i<cols; i++) dispc_to_col[i] = grid->GetColAt(i);
			bool sel_found = false;
			int dpos_first_sel = -1;
			int dpos_last_sel = -1;
			for (int dpos=0; dpos<cols; ++dpos) {
				if (table_base->FromGridIsSelectedCol(dispc_to_col[dpos])) {
					if (!sel_found) {
						dpos_first_sel = dpos;
						dpos_last_sel = dpos;
						sel_found = true;
					}
					if (dpos < dpos_first_sel) dpos_first_sel = dpos;
					if (dpos > dpos_last_sel) dpos_last_sel = dpos;
				}
			}
			int dispc = col_to_dispc[col];
			
			if (!sel_found) {
				dpos_first_sel = dispc;
				dpos_last_sel = dispc;
			} else if (dispc <= dpos_first_sel) {
				dpos_last_sel = dpos_first_sel;
				dpos_first_sel = dispc;
			} else if (dispc >= dpos_last_sel) {
				dpos_first_sel = dpos_last_sel;
				dpos_last_sel = dispc;
			} else {
				// in the middle, so leave endpoints as they are
			}
			
			for (int dc=0; dc<cols; ++dc) {
				int id = dispc_to_col[dc];
				if (dc < dpos_first_sel || dc > dpos_last_sel) {
					if (table_base->FromGridIsSelectedCol(id)) {
						table_base->FromGridDeselectCol(id);
					}
				} else {
					if (!table_base->FromGridIsSelectedCol(id)) {
						table_base->FromGridSelectCol(id);
					}
				}
			}
		}
		// unselect whatever the wxGrid object selects since we are doing our
		// own selection.
		grid->ClearSelection();
		grid->Refresh();
        ev.Skip();  // Col move doesn't work if we don't call ev.Skip()
	} else if (col < 0 && row < 0) {
		// Deselect all rows and columns
		table_base->DeselectAllRows();
		table_base->DeselectAllCols();
		// unselect whatever the wxGrid object selects since we are doing our
		// own selection.
		grid->ClearSelection();
		grid->Refresh();
        //ev.Skip();
	}
	
	LOG_MSG("Exiting TableFrame::OnLabelLeftClickEvent");
}

void TableFrame::OnLabelLeftDClickEvent( wxGridEvent& ev)
{
	LOG_MSG("Entering TableFrame::OnLabelLeftDClickEvent");	
	LOG(ev.GetCol());
	LOG(ev.GetRow());
	LOG(ev.ShiftDown());
	int row = ev.GetRow();
	int col = ev.GetCol();
	if (col >= 0 && row < 0) {
		int sort_col = table_base->GetSortingCol();
		bool ascending = table_base->IsSortedAscending();
		if (sort_col == col) {
			if (!ascending) {
				table_base->SortByDefaultDecending();
			} else {
				table_base->SortByCol(col, false);
			}
		} else {
			table_base->SortByCol(col, true);
		}
		// unselect whatever the wxGrid object selects since we are doing our
		// own selection.
		grid->ClearSelection();
		grid->Refresh();
	} else if (col < 0 && row <0) {
		int sort_col = table_base->GetSortingCol();
		bool ascending = table_base->IsSortedAscending();
		if (sort_col == col) {
			if (ascending) {
				table_base->SortByDefaultDecending();
			} else {
				table_base->SortByDefaultAscending();
			}
		} else {
			table_base->SortByDefaultDecending();
		}
		grid->ClearSelection();
		grid->Refresh();
	} else {
		ev.Skip(); // continue processing this event	
	}
	LOG_MSG("Exiting TableFrame::OnLabelLeftDClickEvent");
}

void TableFrame::OnCellChanged( wxGridEvent& ev )
{
	TableInterface* ti = table_base->GetTableInt();
	if (ti->IsSetCellFromStringFail()) {
		wxMessageDialog dlg(this, ti->GetSetCellFromStringFailMsg(), "Warning",
							wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		ev.Veto();
	}
	ev.Skip();
}

void TableFrame::update(TimeState* o)
{
	// A possible title change or time step change.
	Refresh();
}

class cid_to_pos_pair
{
public:
	int cid;
	int pos;
	static bool less_than(const cid_to_pos_pair& i,
						  const cid_to_pos_pair& j) {
		return (i.pos<j.pos);
	}
};

void TableFrame::OnGroupVariables( wxCommandEvent& event)
{
	using namespace std;
	TableInterface* ti = table_base->GetTableInt();
    vector<int> sel_cols;
	table_base->GetSelectedCols(sel_cols);
	vector<cid_to_pos_pair> cid_to_disp(sel_cols.size());
	for (int i=0; i<sel_cols.size(); i++) {
		cid_to_pos_pair p;
		p.cid = sel_cols[i];
		p.pos = grid->GetColPos(sel_cols[i]);
		cid_to_disp[i] = p;
	}
	sort(cid_to_disp.begin(), cid_to_disp.end(), cid_to_pos_pair::less_than);
	for (int i=0; i<sel_cols.size(); i++) {
		sel_cols[i] = cid_to_disp[i].cid;
	}
	
	vector<wxString> names(sel_cols.size());
	for (int i=0; i<sel_cols.size(); ++i) names[i]=ti->GetColName(sel_cols[i]);
	wxString grp_nm = ti->SuggestGroupName(names);
    if (sel_cols.size() > 0) {
		if (ti->GetTimeSteps() == 1 && sel_cols.size() > 1) {
			if (table_state->GetNumDisallowTimelineChanges() > 0) {
				wxString msg = table_state->GetDisallowTimelineChangesMsg();
				wxMessageDialog dlg (this, msg, "Warning",
									 wxOK | wxICON_INFORMATION);
				dlg.ShowModal();
				return;
			}
		}
        ti->GroupCols(sel_cols, grp_nm, sel_cols[0]);
    }
    table_base->DeselectAllCols();
	grid->Refresh();
	GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
}

void TableFrame::OnUnGroupVariable(wxCommandEvent& event )
{
	if (popup_col < 0) return;
	table_base->GetTableInt()->UngroupCol(popup_col);
	popup_col = -1;
    table_base->DeselectAllCols();
	grid->Refresh();
}

void TableFrame::OnRenameVariable(wxCommandEvent& event)
{
	if (popup_col < 0) return; 
	TableInterface* ti=table_base->GetTableInt();
	wxString curr_name = ti->GetColName(popup_col);
	wxString new_name = PromptRenameColName(ti, popup_col, curr_name);
	if (new_name.CmpNoCase(curr_name)==0 || new_name.IsEmpty()) return;
	ti->RenameGroup(popup_col, new_name);

	popup_col = -1;
	grid->Refresh();
}

wxString TableFrame::PromptRenameColName(TableInterface* ti, int curr_col,
										 const wxString& initial_name)
{
	bool is_group_col = ti->IsColTimeVariant(curr_col);
	wxString initial_msg;
	wxString dlg_title;
	if (is_group_col) {
		dlg_title << "Rename Space-Time Variable";
		initial_msg << "New space-time variable name";
	} else {
		dlg_title << "Rename Variable";
		initial_msg << "New variable name";
	}

	bool done = false;
	wxString curr_name = ti->GetColName(curr_col);
	wxString new_name = initial_name;
	wxString error_pre_msg = wxEmptyString; 
	error_pre_msg 
		<< "Variable name is either a duplicate or is invalid. Please\n"
		<< "enter an alternative, non-duplicate variable name.\n\n";
	wxString error_msg = wxEmptyString;

	bool first = true;
	while (!done) {
		wxString m = is_group_col ? error_pre_msg : error_pre_msg + error_msg;
		wxTextEntryDialog dlg(this, (first ? initial_msg : m), dlg_title,
							  new_name);
		if (dlg.ShowModal() == wxID_OK) {
			new_name = dlg.GetValue();
			new_name.Trim(false);
			new_name.Trim(true);
			done = ((new_name.CmpNoCase(curr_name)==0) ||
					(is_group_col && ti->IsValidGroupName(new_name)
					 && !ti->DoesNameExist(new_name, false)) ||
					(!is_group_col && ti->IsValidDBColName(new_name, &error_msg)
					 && !ti->DoesNameExist(new_name, false)));
			first = false;
		} else {
			new_name = "";
			done = true;
		}
	}
	return new_name;
}
