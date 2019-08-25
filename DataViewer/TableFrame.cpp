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
    EVT_MENU(XRCID("ID_TABLE_RENAME_VARIABLE"), TableFrame::OnRenameVariable)

END_EVENT_TABLE()

TableFrame::TableFrame(wxFrame *parent, Project* project,
					   const wxString& title,
					   const wxPoint& pos, const wxSize& size,
					   const long style)
    : TemplateFrame(parent, project, title, pos, size, style),
    popup_col(-1)
{
	wxLogMessage("Open TableFrame.");
    wxPanel *panel = new wxPanel(this, wxID_ANY);    
	DisplayStatusBar(true);
	wxString new_title(title);
	new_title << " - " << project->GetProjectTitle();
	SetTitle(new_title);
	supports_timeline_changes = true;
	table_base = new TableBase(project, this);
	TableInterface* table_int = project->GetTableInt();
	grid = new wxGrid(panel, wxID_ANY, wxPoint(0,0), wxSize(100, -1));
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
	for (int i=0, iend=table_base->GetNumberCols(); i<iend; i++) {
        GdaConst::FieldType col_type = table_int->GetColType(i);
		if (col_type == GdaConst::long64_type) {
            grid->RegisterDataType("Long64Type",
                                   new wxGridCellInt64Renderer(),
                                   new wxGridCellInt64Editor());
            grid->SetColFormatCustom(i, "Long64Type");

		} else if (col_type == GdaConst::double_type) {
            int d = table_int->GetColDecimals(i);
            if (d < 0) d = GdaConst::default_dbf_double_decimals;
            int dd = table_int->GetColDispDecimals(i);
            if (dd < 0) dd = GdaConst::default_dbf_double_decimals;
            int w = table_int->GetColLength(i);
            grid->RegisterDataType("DoubleType",
                                    new wxGridCellFloatRenderer(w, dd),
                                    new wxGridCellDoubleEditor(w, d));
			grid->SetColFormatCustom(i, "DoubleType");

		} else if (col_type == GdaConst::date_type ||
                   col_type == GdaConst::time_type ||
                   col_type == GdaConst::datetime_type) {
			// leave as a string
		}
		grid->SetColSize(i, -1); // fit column width to lable width
	}
    
	int sample = std::min(table_base->GetNumberRows(), 25);
    
	for (int i=0, iend=table_base->GetNumberCols(); i<iend; i++) {
		double cur_col_size = grid->GetColSize(i);
		double cur_lbl_len = grid->GetColLabelValue(i).length();
		double max_cell_len = cur_lbl_len;
		for (int j=0; j<sample-1; ++j) {
			//wxString cv = grid->GetCellValue(j, i);
            int cv_length = table_int->GetCellStringLength(j,i,true);
            if (cv_length > max_cell_len) max_cell_len = cv_length;
		}
        // width (pixel) per number
        double pw = 10;
        grid->SetColMinimalWidth(i, cur_lbl_len * pw + 8);
        // attempt to scale up col width based on cur_col_size
        //double fac = 1.2;
        grid->SetColSize(i, max_cell_len * 8);
	}
    grid->ForceRefresh();

    wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);
    box->Add(grid, 1, wxEXPAND | wxALL, 0);
    panel->SetSizerAndFit(box);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
   
    // mouse event should be binded to grid window, not grid itself
    grid->GetGridWindow()->Bind(wxEVT_RIGHT_UP, &TableFrame::OnMouseEvent, this);
}

TableFrame::~TableFrame()
{
	DeregisterAsActive();
}

void TableFrame::OnMouseEvent(wxMouseEvent& event)
{
    if (event.RightUp()) {
        const wxPoint& pos = event.GetPosition();
        wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_TABLE_VIEW_MENU_CONTEXT");
        
        TableInterface* ti = table_base->GetTableInt();
        SetEncodingCheckmarks(optMenu, ti->GetFontEncoding());
        PopupMenu(optMenu);
    }
}

void TableFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In TableFrame::OnActivate");
		RegisterAsActive("TableFrame", GetTitle());
	}
	event.Skip(false);
}

void TableFrame::OnClose(wxCloseEvent& event)
{
	if (!GdaFrame::GetGdaFrame()) {
		// This is an exit event, so allow close to proceed
		event.Skip();
		return;
	}
	if (!GdaFrame::IsProjectOpen()) {
		wxLogMessage("In TableFrame::OnClose and actually closing.");
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
		Hide();
	}
}

std::vector<int> TableFrame::GetRowOrder()
{
    return table_base->GetRowOrder();
}

void TableFrame::OnMenuClose(wxCommandEvent& event)
{
	Hide();
}

void TableFrame::MapMenus()
{
	// Map Default Options Menus
    wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_TABLE_VIEW_MENU_CONTEXT");
	GeneralWxUtils::ReplaceMenu(GdaFrame::GetGdaFrame()->GetMenuBar(), _("Options"), optMenu);
}

void TableFrame::DisplayPopupMenu( wxGridEvent& ev )
{
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_TABLE_VIEW_MENU_CONTEXT");
	
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
	// Set Rename item
	wxString rename_str(_("Rename Variable"));
	if (popup_col != -1) {
		rename_str << " \"" << ti->GetColName(popup_col) << "\"";
	}
    wxMenuItem* rename_mu = optMenu->FindItem(XRCID("ID_TABLE_RENAME_VARIABLE"));
    rename_mu->SetItemLabel(rename_str);
	bool enable_rename = false;
	if (popup_col!=-1) {
		if (ti->IsColTimeVariant(popup_col)) {
			enable_rename = true;
		} else {
			enable_rename = ti->PermitRenameSimpleCol();
		}
	}
    
	rename_mu->Enable(enable_rename);

    // Set meta-data
    std::map<wxString, wxString> meta_data = ti->GetMetaData(popup_col);
    if (meta_data.empty() == false) {
        wxMenu* imp = new wxMenu;
        std::map<wxString, wxString>::iterator it;
        for (it = meta_data.begin(); it != meta_data.end(); it++) {
            wxString lbl = it->first + ": " + it->second;
            imp->Append(XRCID(lbl), lbl);
        }
        optMenu->AppendSubMenu(imp, _("Meta-data"));
    }
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
	ev.Veto();
}

void TableFrame::OnColMoveEvent( wxGridEvent& ev )
{
    wxLogMessage("In TableFrame::OnColMoveEvent()");
	table_base->notifyColMove();
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
    wxLogMessage("In TableFrame::OnLabelLeftClickEvent()");
	using namespace std;
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
}

void TableFrame::OnLabelLeftDClickEvent( wxGridEvent& ev)
{
	wxLogMessage("In TableFrame::OnLabelLeftDClickEvent()");
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
}

void TableFrame::OnCellChanged( wxGridEvent& ev )
{
	wxLogMessage("In TableFrame::OnCellChanged()");
	TableInterface* ti = table_base->GetTableInt();
	if (ti->IsSetCellFromStringFail()) {
		wxMessageDialog dlg(this, ti->GetSetCellFromStringFailMsg(), _("Warning"),
							wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		ev.Veto();
	}
    //grid->SetCellAlignment (ev.GetRow(), ev.GetCol(), wxALIGN_RIGHT);
    //grid->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_RIGHT);
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
				wxMessageDialog dlg (this, msg, _("Warning"),
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

    if (new_name.IsEmpty()) return;

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
		dlg_title << _("Rename Space-Time Variable");
		initial_msg << _("New space-time variable name");
	} else {
		dlg_title << _("Rename Variable");
		initial_msg << _("New variable name");
	}

	bool done = false;
	wxString curr_name = ti->GetColName(curr_col);
	wxString new_name = initial_name;
	wxString error_pre_msg = wxEmptyString; 
	error_pre_msg = _("Variable name is either a duplicate or is invalid. Please\nenter an alternative, non-duplicate variable name.\n\n");
	wxString error_msg = wxEmptyString;

	bool first = true;
    // rename field name may allow different case, e.g. ID->id
    bool case_sensitive = project->IsFieldCaseSensitive();

	while (!done) {
		wxString m = is_group_col ? error_pre_msg : error_pre_msg + error_msg;
		wxTextEntryDialog dlg(this, (first ? initial_msg : m), dlg_title,
							  new_name);
		if (dlg.ShowModal() == wxID_OK) {
			new_name = dlg.GetValue();
			new_name.Trim(false);
			new_name.Trim(true);

            bool is_name_exist = false;
            bool case_change = new_name.CmpNoCase(curr_name) == 0;
            if ( !case_change && ti->DoesNameExist(new_name, false)) {
                is_name_exist = true;
            }
            
            // is_name_exist includes if new_name equals (case) curr_name
            if (is_name_exist == false ) {
                if (is_group_col) {
                    if (ti->IsValidGroupName(new_name)) {
                        done = true;
                    }
                } else {
                    if (ti->IsValidDBColName(new_name, &error_msg)) {
                        done = true;
                    }
                }
            }
			first = false;
		} else {
			new_name = "";
			done = true;
		}
	}
	return new_name;
}
