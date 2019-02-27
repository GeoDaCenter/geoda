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
#include <wx/statusbr.h>

#include <boost/foreach.hpp>
#include <vector>
#include "../HighlightState.h"
#include "../GenUtils.h"
#include "../GeneralWxUtils.h"
#include "TableState.h"
#include "TimeState.h"
#include "TableInterface.h"
#include "TableBase.h"
#include "../Project.h"
#include "../logger.h"
#include "../SaveButtonManager.h"
#include "TableFrame.h"

wxGridCellInt64Renderer::wxGridCellInt64Renderer(int width)
{
    m_width = width;
    SetWidth(width);
}

wxGridCellRenderer *wxGridCellInt64Renderer::Clone() const
{
    wxGridCellInt64Renderer *renderer = new wxGridCellInt64Renderer;
    renderer->m_width = m_width;
    return renderer;
}

wxSize wxGridCellInt64Renderer::GetBestSize(wxGrid& grid,
                                            wxGridCellAttr& attr,
                                            wxDC& dc,
                                            int row, int col)
{
    return DoGetBestSize(attr, dc, GetString(grid, row, col));
}

void wxGridCellInt64Renderer::Draw(wxGrid& grid,
                                   wxGridCellAttr& attr,
                                   wxDC& dc,
                                   const wxRect& rectCell,
                                   int row, int col,
                                   bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);

    SetTextColoursAndFont(grid, attr, dc, isSelected);

    // draw the text right aligned by default
    int hAlign = wxALIGN_RIGHT,
    vAlign = wxALIGN_INVALID;
    attr.GetNonDefaultAlignment(&hAlign, &vAlign);

    wxRect rect = rectCell;
    rect.Inflate(-1);

    grid.DrawTextRectangle(dc, GetString(grid, row, col), rect, hAlign, vAlign);
}

wxString wxGridCellInt64Renderer::GetString(const wxGrid& grid, int row, int col)
{
    wxGridTableBase *table = grid.GetTable();

    bool hasDouble;
    double val;
    wxString text = table->GetValue(row, col);
    return text;
}

wxGridCellInt64Editor::wxGridCellInt64Editor(int width)
{
    m_width = width;
}

void wxGridCellInt64Editor::Create(wxWindow* parent,
                                   wxWindowID id,
                                   wxEvtHandler* evtHandler)
{
    wxGridCellTextEditor::Create(parent, id, evtHandler);

#if wxUSE_VALIDATORS
    wxString name_chars="-+0123456789";
    wxTextValidator name_validator(wxFILTER_INCLUDE_CHAR_LIST);
    name_validator.SetCharIncludes(name_chars);
    Text()->SetValidator(name_validator);
#endif
}

void wxGridCellInt64Editor::BeginEdit(int row, int col, wxGrid* grid)
{
    // first get the value
    wxGridTableBase *table = grid->GetTable();
    const wxString value = table->GetValue(row, col);
    if ( !value.empty() ) {
        if ( !value.ToLongLong(&m_value) ) {
            wxFAIL_MSG( wxT("this cell doesn't have int64 value") );
            return;
        }
        DoBeginEdit(value);
    } else {
        DoBeginEdit("0");
    }
}

bool wxGridCellInt64Editor::EndEdit(int WXUNUSED(row),
                                    int WXUNUSED(col),
                                    const wxGrid* WXUNUSED(grid),
                                    const wxString& oldval, wxString *newval)
{
    long long value = 0;
    wxString text;

    text = Text()->GetValue();
    if ( text.empty() ) {
        if ( oldval.empty() )
            return false;
    } else {
        // non-empty text now (maybe 0)
        if ( !text.ToLongLong(&value) )
            return false;

        // if value == m_value == 0 but old text was "" and new one is
        // "0" something still did change
        if ( value == m_value && (value || !oldval.empty()) )
            return false;
    }
    m_value = value;

    if ( newval )
        *newval = text;

    return true;
}

void wxGridCellInt64Editor::ApplyEdit(int row, int col, wxGrid* grid)
{
    wxGridTableBase * const table = grid->GetTable();
    table->SetValue(row, col, wxString::Format("%lld", m_value));
}

void wxGridCellInt64Editor::Reset()
{
    DoReset(GetString());
}

bool wxGridCellInt64Editor::IsAcceptedKey(wxKeyEvent& event)
{
    if ( wxGridCellEditor::IsAcceptedKey(event) ) {
        int keycode = event.GetKeyCode();
        if ( (keycode < 128) &&
            (wxIsdigit(keycode) || keycode == '+' || keycode == '-')) {
            return true;
        }
    }
    return false;
}

void wxGridCellInt64Editor::StartingKey(wxKeyEvent& event)
{
    int keycode = event.GetKeyCode();
    if ( wxIsdigit(keycode) || keycode == '+' || keycode == '-') {
        wxGridCellTextEditor::StartingKey(event);
        // skip Skip() below
        return;
    }
    event.Skip();
}

// return the value in the spin control if it is there (the text control otherwise)
wxString wxGridCellInt64Editor::GetValue() const
{
    wxString s = Text()->GetValue();
    return s;
}


class TableCellAttrProvider : public wxGridCellAttrProvider
{
public:
	TableCellAttrProvider(std::vector<int>& row_order,
						  std::vector<bool>& selected,
                          std::vector<bool>& selected_cols);
	virtual ~TableCellAttrProvider();
	
	virtual wxGridCellAttr *GetAttr(int row, int col,
									wxGridCellAttr::wxAttrKind kind) const;
	
private:
	wxGridCellAttr* attrForAll;	
	std::vector<int>& row_order;
	std::vector<bool>& selected;
    std::vector<bool>& selected_cols;
};

TableCellAttrProvider::TableCellAttrProvider(std::vector<int>& row_order_,
											 std::vector<bool>& selected_,
                                             std::vector<bool>& selected_cols_)
: row_order(row_order_), selected(selected_), selected_cols(selected_cols_)
{
	attrForAll = new wxGridCellAttr;
}

TableCellAttrProvider::~TableCellAttrProvider()
{
	attrForAll->DecRef();
}

wxGridCellAttr *TableCellAttrProvider::GetAttr(int row, int col,
									wxGridCellAttr::wxAttrKind kind ) const
{
    wxGridCellAttr *attr = wxGridCellAttrProvider::GetAttr(row, col, kind);
	
	bool row_sel = (row >= 0 && selected[row_order[row]]);
	bool col_sel = (selected_cols.size()>col && col >=0 && selected_cols[col]);
    
	if ( !attr ) {
		attr = attrForAll;
		attr->IncRef();
	}
	if ( !attr->HasBackgroundColour() ) {
		wxGridCellAttr *attrNew = attr->Clone();
		attr->DecRef();
		attr = attrNew;
	}
	if (row_sel && col_sel) {
		attr->SetBackgroundColour(GdaConst::table_row_and_col_sel_color);
	} else if (row_sel) {
		attr->SetBackgroundColour(GdaConst::table_row_sel_color);
	} else if (col_sel) {
		attr->SetBackgroundColour(GdaConst::table_col_sel_color);
	} else {
		attr->SetBackgroundColour(*wxWHITE);
	}
	
    return attr;
}


TableBase::TableBase(Project* _project,TemplateFrame* t_frame)
	: project(_project), highlight_state(_project->GetHighlightState()),
	hs(_project->GetHighlightState()->GetHighlight()),
	table_state(_project->GetTableState()), table_int(_project->GetTableInt()),
	time_state(_project->GetTimeState()), cols(table_int->GetNumberCols()),
	rows(_project->GetNumRecords()), row_order(_project->GetNumRecords()),
	sorting_col(-1), sorting_ascending(false)
{
    template_frame = t_frame;
	SortByDefaultDecending();
	
    for(int i=0;i<cols;i++) hs_col.push_back(false);
	SetAttrProvider(new TableCellAttrProvider(row_order, hs, hs_col));
	
	highlight_state->registerObserver(this);
	table_state->registerTableBase(this);
	time_state->registerObserver(this);
    
    UpdateStatusBar();
}

TableBase::~TableBase()
{
	highlight_state->removeObserver(this);
	table_state->removeObserver(this);
	time_state->removeObserver(this);
}

void TableBase::UpdateStatusBar()
{
    wxStatusBar* sb = template_frame->GetStatusBar();
    if (!sb) return;
    wxString s;
    s << _("#row=") << project->GetNumRecords() << " ";
    if (highlight_state->GetTotalHighlighted()> 0) {
        s << _("#selected=") << highlight_state->GetTotalHighlighted() << "  ";
    }
    
    sb->SetStatusText(s);
}

/** Only wxGrid should call this, others should use Selected(int row) */
bool TableBase::FromGridIsSelectedRow(int row)
{
	return hs[row_order[row]];
}

/** Only wxGrid should call this */
void TableBase::FromGridSelectOnlyRow(int row)
{
	int hl_size = highlight_state->GetHighlightSize();
    
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
    
	for (int i=0; i<hl_size; i++) {
		if (i != row_order[row]) {
            if (hs[i])  {
                hs[i] = false;
                selection_changed = true;
            }
		} else {
            if (!hs[i]) {
                hs[i] = true;
                selection_changed = true;
            }
		}
	}
    
    if (selection_changed) {
    	highlight_state->SetEventType(HLStateInt::delta);
    	highlight_state->notifyObservers();
    }
}

/** Only wxGrid should call this */
void TableBase::FromGridSelectRowRange(int first_row, int last_row)
{
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
    
	for (int i=0; i<hl_size; ++i) {
		if (i < first_row || i > last_row) {
            if (hs[row_order[i]])  {
                hs[row_order[i]] = false;
                selection_changed = true;
            }
		} else {
            if (!hs[row_order[i]])  {
                hs[row_order[i]] = true;
                selection_changed = true;
            }
		}
	}
    
    if (selection_changed) {
    	highlight_state->SetEventType(HLStateInt::delta);
    	highlight_state->notifyObservers();
    }
}

/** Only wxGrid should call this, others should use Select(int row) */
void TableBase::FromGridSelectRow(int row)
{
	//LOG_MSG(wxString::Format("selecting %d", (int) row_order[row]));
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    hs[ row_order[row] ]  = true;
    
	highlight_state->SetEventType(HLStateInt::delta);
	highlight_state->notifyObservers();
}

/** Only wxGrid should call this, others should use Deselect(int row) */
void TableBase::FromGridDeselectRow(int row)
{
	//LOG_MSG(wxString::Format("deselecting %d", (int) row_order[row]));
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
   
    hs[ row_order[row] ]  = false;
    
	highlight_state->SetEventType(HLStateInt::delta);
	highlight_state->notifyObservers();
}

void TableBase::DeselectAllRows()
{
	highlight_state->SetEventType(HLStateInt::unhighlight_all);
	highlight_state->notifyObservers();
}

void TableBase::SortByDefaultDecending()
{
	for (int i=0; i<rows; i++) {
		row_order[i] = i;
	}
	sorting_ascending = false;
	sorting_col = -1;
}

void TableBase::SortByDefaultAscending()
{
	int last_ind = rows-1;
	for (int i=0; i<rows; i++) {
		row_order[i] = last_ind - i;
	}
	sorting_ascending = true;
	sorting_col = -1;
}


template <class T>
class index_pair
{
public:
	int index;
	T val;
	static bool less_than(const index_pair& i,
						  const index_pair& j) {
		return (i.val<j.val);
	}
	static bool greater_than (const index_pair& i,
							  const index_pair& j) {
		return (i.val>j.val);
	}
};

std::vector<int> TableBase::GetRowOrder()
{
    return row_order;
}

template <class T>
void TableBase::SortColumn(int col, int tm, bool ascending)
{
    std::vector<T> temp;
    table_int->GetColData(col, tm, temp);
    std::vector<bool> undefs;
    table_int->GetColUndefined(col, tm, undefs);
    std::vector<int> undef_ids;
    for (int i=0; i<rows; i++) {
        if (undefs[i]) undef_ids.push_back(i);
    }
    std::vector< index_pair<T> > sort_col(rows - undef_ids.size());
    int j = 0;
    for (int i=0; i<rows; i++) {
        if (undefs[i]) continue;
        sort_col[j].index = i;
        sort_col[j].val = temp[i];
        j++;
    }
    if (ascending) {
        sort(sort_col.begin(), sort_col.end(),
             index_pair<T>::less_than);
        for (int i=0; i<undef_ids.size(); i++) {
            row_order[i] = undef_ids[i];
        }
        j = undef_ids.size();
        for (int i=0; i<sort_col.size(); i++) {
            row_order[j++] = sort_col[i].index;
        }
    } else {
        sort(sort_col.begin(), sort_col.end(),
             index_pair<T>::greater_than);
        for (int i=0; i<sort_col.size(); i++) {
            row_order[i] = sort_col[i].index;
        }
        j = 0;
        for (int i=sort_col.size(); i<rows; i++) {
            row_order[i] = undef_ids[j++];
        }
    }
}

void TableBase::SortByCol(int col, bool ascending)
{
	if (col == -1) {
		if (ascending) {
			SortByDefaultAscending();
		} else {
			SortByDefaultDecending();
		}
		return;
	}
	sorting_ascending = ascending;
	sorting_col = col;
	int rows = GetNumberRows();
	
	int tm=time_state->GetCurrTime();
	switch (table_int->GetColType(col)) {
		case GdaConst::date_type:
		case GdaConst::time_type:
		case GdaConst::datetime_type:
		case GdaConst::long64_type:
		{
            SortColumn<wxInt64>(col, tm, ascending);
		}
			break;
		case GdaConst::double_type:
		{
            SortColumn<double>(col, tm, ascending);
		}
			break;
		case GdaConst::string_type:
		{
            SortColumn<wxString>(col, tm, ascending);
		}
			break;
		default:
			break;
	}
}

void TableBase::MoveSelectedToTop()
{
	std::set<int> sel_set;
	for (int i=0, iend=rows; i<iend; i++) {
		if (hs[row_order[i]]) sel_set.insert(row_order[i]);
	}
	int sel_row_offset = 0;
	int unsel_row_offset = sel_set.size();
	for (int i=0; i<rows; i++) {
		if (sel_set.find(i) != sel_set.end()) {
			row_order[sel_row_offset++] = i;
		} else {
			row_order[unsel_row_offset++] = i;
		}
	}
	sorting_col = -1;
	if (GetView()) GetView()->Refresh();
}

bool TableBase::FromGridIsSelectedCol(int col)
{
	if (hs_col.size() -1 <col) return false;
	return hs_col[col];
}

void TableBase::FromGridSelectCol(int col)
{
	if (hs_col.size() -1 <col) 
    hs_col[col] = true;
}

void TableBase::FromGridDeselectCol(int col)
{
	if (hs_col.size() -1 <col)
    hs_col[col] = false;
}

void TableBase::DeselectAllCols()
{
	for(int i=0;i<hs_col.size();i++) hs_col[i] = false;
}

void TableBase::GetSelectedCols(std::vector<int>& sel_cols)
{
	sel_cols.clear();
    for (int i=0; i<hs_col.size(); i++) {
        if (hs_col[i]) sel_cols.push_back(i);
    }
}

// pure virtual method implementation for wxGridTableBase
int TableBase::GetNumberRows()
{
	return table_int->GetNumberRows();
}

int TableBase::GetNumberCols()
{ 
	return table_int->GetNumberCols();
}

wxString TableBase::GetValue(int row, int col)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	if (col<0 || col>GetNumberCols()) return wxEmptyString;
	
	int curr_ts = (table_int->IsColTimeVariant(col) ?
				   time_state->GetCurrTime() : 0);
	return table_int->GetCellString(row_order[row], col, curr_ts);
}

// Note: when writing to raw_data, we must be careful not to overwrite
//       the buffer and also to respect the DBF formating requirements,
//       especially for floats.  Aditionally, must check that all numbers
//       are valid and set undefined flag appropriately.  Also, this
//       method should only be called by wxGrid since we automatically
//       compute the correct row.
void TableBase::SetValue(int row, int col, const wxString &value)
{
	int curr_ts = (table_int->IsColTimeVariant(col) ?
				   time_state->GetCurrTime() : 0);
	table_int->SetCellFromString(row_order[row], col, curr_ts, value);
    if (project->GetSaveButtonManager()) {
		project->GetSaveButtonManager()->SetMetaDataSaveNeeded(true);
	}
    //GetView()->SetCellAlignment(row, col, wxALIGN_RIGHT, wxALIGN_RIGHT);
    GetView()->Refresh();
}

wxString TableBase::GetRowLabelValue(int row)
{
	if (row<0 || row>=GetNumberRows()) return wxEmptyString;
	return wxString::Format("%d", row_order[row]+1);
}

wxString TableBase::GetColLabelValue(int col)
{
	// \uXXXX are unicode characters for up and down arrows
	// \u0668 Arabic-Indic digit eight (up pointing arrow)
	// \u0667 Arabic-Indic digit seven (down pointing arrow)
	// \u25B5 white small up-pointing triangle (too big on OSX)
	// \u25BF white small down-pointing triangle
	// \u25B4 black small up-pointing triangle  (too big on OSX)
	// \u25BE black small down-pointing triangle
	// \u02C4, \u02C5 are up and down-pointing arrows
	// \u27F0 upward quadruple arrow
	// \u27F1 downward quadruble arrow
	
	if (col<0 || col>table_int->GetNumberCols()) return wxEmptyString;
	
	wxString label(table_int->GetColName(col));
	
	if (table_int->IsColTimeVariant(col)) {
		label << " (" << time_state->GetCurrTimeString();
		label << ")";
	}
	
	if (col == sorting_col) {
        label << (sorting_ascending ? " >" : " <");
	}
	return label;
}

void TableBase::update(HLStateInt* o)
{
    if (GetView()) {
        GetView()->Refresh();
    }
    UpdateStatusBar();
}

void TableBase::update(TableState* o)
{
	using namespace std;
	if (!GetView()) return;
	
	if (o->GetEventType() == TableState::cols_delta) {
		BOOST_FOREACH(const TableDeltaEntry& e, o->GetTableDeltaListRef()) {
			if (e.insert) {
				if (e.pos_at_op <= sorting_col) sorting_col++;
				wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_INSERTED,
									   e.pos_at_op, 1);
				GetView()->ProcessTableMessage(msg);
			} else {
				if (e.pos_at_op == sorting_col) { 
					sorting_col = -1;
                    sorting_ascending = true;
                }
				if (e.pos_at_op < sorting_col) sorting_col--;
				
				wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED,
									   e.pos_at_op, 1);
				GetView()->ProcessTableMessage( msg );
			}
		}
		BOOST_FOREACH(const TableDeltaEntry& e, o->GetTableDeltaListRef()) {
			if (e.insert) {
				if (e.type == GdaConst::long64_type) {
                    // leave as a string: for more than 10 digts 64-bit number
                    GetView()->RegisterDataType("Long64Type",
                                                new wxGridCellInt64Renderer(),
                                                new wxGridCellInt64Editor());
                    GetView()->SetColFormatCustom(e.pos_final, "Long64Type");
				} else if (e.type == GdaConst::double_type) {
					int dd = e.displayed_decimals;
					if (dd == -1) dd = e.decimals;
                    dd = std::min(e.decimals, dd);
                    if (dd <=0) dd = GdaConst::default_dbf_double_decimals;
					GetView()->SetColFormatFloat(e.pos_final, -1, dd);
				} else {
					// leave as a string
				}
				if (GetNumberRows() <= 500) {
					// this operation is too slow when a large number of
					// cells are present.
					GetView()->AutoSizeColumn(e.pos_final, false);
				}
			} // no formatting needed for removing columns
		}
	} else if (o->GetEventType() == TableState::col_disp_decimals_change) {
		int pos = o->GetModifiedColPos();
		if (table_int->GetColType(pos) == GdaConst::double_type) {
			int dd = table_int->GetColDispDecimals(pos);
			GetView()->SetColFormatFloat(pos, -1, dd);
		}
    } 
	
	GetView()->Refresh();
}

void TableBase::update(TimeState* o)
{
	if (GetView()) GetView()->Refresh();
}

/** Grid Base does not need to be notifed of TableViewer column
 movements since moving a column in the TableViewer does not
 change the state of Grid Base.  This method is a workaround
 to notify interested listeners that the visual column order has
 changed.  It is called by TableFrame::OnColMoveEvent */
void TableBase::notifyColMove()
{
	table_state->SetColOrderChangeEvtTyp();
	table_state->notifyObservers();
}

TableInterface* TableBase::GetTableInt()
{
	return table_int;
}
