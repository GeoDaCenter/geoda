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

#include <wx/aboutdlg.h>
#include <wx/choicdlg.h>
#include <wx/colour.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/wxprec.h>
#include <wx/xrc/xmlres.h>
#include <time.h>
#include <boost/math/special_functions/fpclassify.hpp>
#include "../FramesManager.h"
#include "../Generic/HighlightState.h"
#include "../GeoDaConst.h"
#include "../logger.h"
#include "TableState.h"
#include "../DialogTools/TimeChooserDlg.h"
#include "DbfGridTableBase.h"
#include "DataViewerResizeColDlg.h"
#include "DataViewerAddColDlg.h"
#include "DataViewerDeleteColDlg.h"
#include "DataViewerEditFieldPropertiesDlg.h"
#include "MergeTableDlg.h"
#include "DataViewer.h"

int id_serial = wxID_HIGHEST+1;
const int ID_PRINT_TABLE = id_serial++;
const int ID_CHOOSE_TIME_STEP = id_serial++;
const int ID_SHOW_TIME_CHOOSER = id_serial++;
const int ID_MOVE_SELECTED_TO_TOP = id_serial++;
const int ID_DELETE_COL = id_serial++;
const int ID_ADD_COL = id_serial++;
const int ID_COL_RESIZE = id_serial++;
const int ID_SAVE_DBF = id_serial++;
const int ID_SAVE_DBF_AS = id_serial++;
const int ID_SHOW_DBF_INFO = id_serial++;
const int ID_RESET_COL_ORDER = id_serial++;
const int ID_RANDOM_COL_ORDER = id_serial++;
const int ID_PRINT_GRID_INFO = id_serial++;
const int ID_MOVE_COL = id_serial++;
const int ID_EDIT_FIELD_PROPERTIES = id_serial++;
const int ID_MERGE_TABLE = id_serial++;

BEGIN_EVENT_TABLE( DataViewerFrame, wxFrame )
	EVT_GRID_LABEL_LEFT_CLICK( DataViewerFrame::OnLabelLeftClickEvent )
	EVT_GRID_LABEL_LEFT_DCLICK( DataViewerFrame::OnLabelLeftDClickEvent )
	//EVT_GRID_COL_SORT( DataViewerFrame::OnSortEvent )
	EVT_GRID_COL_SIZE( DataViewerFrame::OnColSizeEvent )
	EVT_GRID_COL_MOVE( DataViewerFrame::OnColMoveEvent )
	EVT_GRID_CELL_CHANGED(DataViewerFrame::OnCellChanged)
	EVT_MENU( ID_CHOOSE_TIME_STEP, DataViewerFrame::OnChooseTimeStep )
	EVT_MENU( ID_SHOW_TIME_CHOOSER, DataViewerFrame::OnShowTimeChooser )
	EVT_MENU( ID_MOVE_COL, DataViewerFrame::OnMoveCol )
	EVT_MENU( ID_COL_RESIZE, DataViewerFrame::OnColResize )
	EVT_MENU( wxID_ABOUT, DataViewerFrame::About )
	EVT_MENU( wxID_EXIT, DataViewerFrame::OnQuit )
	EVT_MENU( wxID_OPEN, DataViewerFrame::OnOpenFile )
	EVT_MENU( ID_PRINT_TABLE, DataViewerFrame::OnPrintTable )
	EVT_MENU( ID_MOVE_SELECTED_TO_TOP, DataViewerFrame::OnMoveSelectedToTop )
	EVT_MENU( ID_ADD_COL, DataViewerFrame::OnAddCol )
	EVT_MENU( ID_DELETE_COL, DataViewerFrame::OnDeleteCol )
	EVT_MENU( ID_RANDOM_COL_ORDER, DataViewerFrame::OnRandomColOrder )
	EVT_MENU( ID_PRINT_GRID_INFO, DataViewerFrame::OnPrintGridInfo )
	EVT_MENU( ID_SAVE_DBF, DataViewerFrame::OnSaveDbf )
	EVT_MENU( ID_SAVE_DBF_AS, DataViewerFrame::OnSaveDbfAs )
	EVT_MENU( ID_SHOW_DBF_INFO, DataViewerFrame::OnShowDbfInfo )
	EVT_MENU( ID_EDIT_FIELD_PROPERTIES, DataViewerFrame::OnEditFieldProperties )
	EVT_MENU( ID_MERGE_TABLE, DataViewerFrame::OnMergeTable )
END_EVENT_TABLE()


DataViewerFrame::DataViewerFrame(wxFrame *parent,
								 FramesManager* frames_manager_s,
								 const wxString& dbf_fname)
: wxFrame(parent, wxID_ANY,
		  "DBF Viewer - " +wxFileName::FileName(dbf_fname).GetFullName(),
		  wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE),
frames_manager(frames_manager_s)
{
	DbfFileReader dbf(dbf_fname);
	if (dbf.isDbfReadSuccess()) {
		cur_dbf_fname = dbf_fname;
		HighlightState* highlight_state = new HighlightState;
		TableState* table_state = new TableState;
		highlight_state->SetSize(dbf.getNumRecords());
		grid_base = new DbfGridTableBase(dbf, highlight_state, table_state);
		grid = new wxGrid( this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
		grid->SetDefaultColSize((grid->GetDefaultColSize() * 4)/3);
		grid->SetTable(grid_base, true); // false to not take ownership
		grid->EnableDragColMove(true);
		grid->EnableDragCell(false);
		// The following line caused the row to dissapear when clicked on Windows
		// and Linux, but not Mac.
		//grid->SetSelectionBackground(*wxWHITE);
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
			grid->DisableRowResize(i);
		}
		grid->SetSelectionMode(wxGrid::wxGridSelectRows);
		std::vector<DbfColContainer*>& col_data = grid_base->col_data;
		for (int i=0, iend=col_data.size(); i<iend; i++) {
			if (col_data[i]->type == GeoDaConst::long64_type) {
				grid->SetColFormatNumber(i);
			} else if (col_data[i]->type == GeoDaConst::double_type) {
				grid->SetColFormatFloat(i, -1, col_data[i]->displayed_decimals);
			} else if (col_data[i]->type == GeoDaConst::date_type) {
				// leave as a string
			}
		}
	} else {
		wxString msg("There was a problem reading in the DBF file. "
					 "The program will now exit.");
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		Close(true);
	}
	frames_manager->registerObserver(this);
}

DataViewerFrame::DataViewerFrame(wxFrame *parent,
								 FramesManager* frames_manager_s,
								 const wxString& dbf_sp_fname,
								 const wxString& dbf_tm_fname,
								 int sp_tbl_sp_col,
								 int tm_tbl_sp_col,
								 int tm_tbl_tm_col)
: wxFrame(parent, wxID_ANY,
		  "DBF Viewer - " +wxFileName::FileName(dbf_sp_fname).GetFullName(),
		  wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE),
frames_manager(frames_manager_s)
{
	DbfFileReader dbf_sp(dbf_sp_fname);
	DbfFileReader dbf_tm(dbf_tm_fname);
	if (dbf_sp.isDbfReadSuccess() && dbf_tm.isDbfReadSuccess()) {
		cur_dbf_fname = dbf_sp_fname;
		cur_dbf_tm_fname = dbf_tm_fname;
		HighlightState* highlight_state = new HighlightState();
		highlight_state->SetSize(dbf_sp.getNumRecords());
		TableState* table_state = new TableState();
		grid_base = new DbfGridTableBase(dbf_sp, dbf_tm, sp_tbl_sp_col,
										 tm_tbl_sp_col, tm_tbl_tm_col,
										 highlight_state, table_state);
		grid = new wxGrid( this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
		grid->SetDefaultColSize((grid->GetDefaultColSize() * 4)/3);
		grid->SetTable(grid_base, true); // false to not take ownership
		grid->EnableDragColMove(true);
		grid->EnableDragCell(false);
		// The following line caused the row to dissapear when clicked on Windows
		// and Linux, but not Mac.
		//grid->SetSelectionBackground(*wxWHITE);
		for (int i=0, iend=grid_base->GetNumberRows(); i<iend; i++) {
			grid->DisableRowResize(i);
		}
		grid->SetSelectionMode(wxGrid::wxGridSelectRows);
		std::vector<DbfColContainer*>& col_data = grid_base->col_data;
		for (int i=0, iend=col_data.size(); i<iend; i++) {
			if (col_data[i]->type == GeoDaConst::long64_type) {
				grid->SetColFormatNumber(i);
			} else if (col_data[i]->type == GeoDaConst::double_type) {
				grid->SetColFormatFloat(i, -1, col_data[i]->displayed_decimals);
			} else if (col_data[i]->type == GeoDaConst::date_type) {
				// leave as a string
			}
		}
	} else {
		wxString msg("There was a problem reading in the DBF file. "
					 "The program will now exit.");
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		Close(true);
	}
	frames_manager->registerObserver(this);
}

DataViewerFrame::~DataViewerFrame()
{
	frames_manager->removeObserver(this);
}

void DataViewerFrame::LoadDefaultMenus()
{
	wxMenu *fileMenu = new wxMenu;
	//fileMenu->Append( ID_PRINT_TABLE, "&Print Table\tCtrl-P" );
	fileMenu->Append( wxID_OPEN, "&Open\tCtrl-O" );
	if (grid_base->IsTimeVariant()) {
		fileMenu->Append( ID_CHOOSE_TIME_STEP, "Choose time step" );
		fileMenu->Append( ID_SHOW_TIME_CHOOSER, "Show Time Control" );
	}
	fileMenu->Append( ID_MOVE_SELECTED_TO_TOP, "&Move selected to top" );
	fileMenu->Append( ID_ADD_COL, "Add Variable" );
	fileMenu->Append( ID_DELETE_COL, "Delete Variable" );
	fileMenu->Append( ID_EDIT_FIELD_PROPERTIES, "Edit Variable Properties" );
	fileMenu->Append( ID_MERGE_TABLE, "Merge Table" );
	//fileMenu->Append( ID_MOVE_COL, "Move Column" );
	//fileMenu->Append( ID_COL_RESIZE, "Manually Resize Column" );
	//fileMenu->Append( ID_RESET_COL_ORDER, "Reset Column Order" );
	//fileMenu->Append( ID_RANDOM_COL_ORDER, "Random Column Order" );
	//fileMenu->Append( ID_PRINT_GRID_INFO, "Print Grid Info to logger");
	fileMenu->Append( ID_SAVE_DBF, "Save DBF" );
	fileMenu->Append( ID_SAVE_DBF_AS, "Save DBF as..." );
	fileMenu->Append( ID_SHOW_DBF_INFO, "Show Info" );
	fileMenu->AppendSeparator();
	fileMenu->Append( wxID_EXIT, "E&xit\tCtrl-Q" );
	
	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append( wxID_ABOUT, "&About GeoDa DBF Viewer" );
	
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( fileMenu, "&File" );
	menuBar->Append( helpMenu, "&Help" );
	
	SetMenuBar(menuBar);
	menuBar->Enable(ID_SAVE_DBF, false);	
}

void DataViewerFrame::About( wxCommandEvent& WXUNUSED(ev) )
{
	wxDialog dlg;
	wxXmlResource::Get()->LoadDialog(&dlg, this, "ID_DATA_VIEWER_ABOUT_DLG");
	dlg.ShowModal();
}

void DataViewerFrame::OnQuit( wxCommandEvent& WXUNUSED(ev) )
{
	Close(true);
}

void DataViewerFrame::OnOpenFile( wxCommandEvent& WXUNUSED(ev) )
{
	wxFileDialog dlg(this, "Choose a DBF file", "", "",
					 "DBF files (*.dbf)|*.dbf");	
	if (dlg.ShowModal() == wxID_OK) {
		DataViewerFrame *frame = new DataViewerFrame(0, frames_manager,
													 dlg.GetPath());
		frame->LoadDefaultMenus();
		frame->Show(true);
	}
}

void DataViewerFrame::OnPrintGridInfo( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnPrintGridInfo");
	wxGridSizesInfo col_sizes = grid->GetColSizes();
	LOG(col_sizes.m_sizeDefault);
	for (int i=0; i<grid->GetNumberCols(); i++) {
		LOG_MSG(wxString::Format("col %d width: %d", i, col_sizes.GetSize(i)));
	}
	LOG_MSG("Exiting DataViewerFrame::OnPrintGridInfo");
}

void DataViewerFrame::OnPrintTable( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Table Contents: ");
	wxString msg("col positions: ");
	for (int i=0, iend=grid->GetNumberCols(); i<iend; i++) {
		msg << grid->GetColAt(i) << " ";
	}
	LOG_MSG(msg);
	grid_base->PrintTable();
}

void DataViewerFrame::OnMoveSelectedToTop( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnMoveSelectedToTop");
	grid_base->MoveSelectedToTop();
	grid->Refresh();
	LOG_MSG("Exiting DataViewerFrame::OnMoveSelectedToTop");
}

void DataViewerFrame::SortEvent( wxGridEvent& ev,
								wxGrid* grid,
								DbfGridTableBase* grid_base )
{
	LOG_MSG("Entering DataViewerFrame::SortEvent");
	int col = ev.GetCol();
	int curr_sort_col = grid_base->GetSortingCol();
	LOG_MSG(wxString::Format("current sorting col: %d, new sorting col %d",
							 curr_sort_col, col));
	
	if (curr_sort_col != col) {
		LOG_MSG(wxString::Format("sorting by col %d in ascending order", col));
		grid_base->SortByCol(col, true);  // sort in ascending order
	} else if (grid_base->IsSortedAscending()) {
		LOG_MSG(wxString::Format("sorting by col %d in descending order", col));
		grid_base->SortByCol(col, false);  // sort in descending order
	} else {
		LOG_MSG("Returning to unsorted order");
		grid_base->SortByDefaultDecending(); // restore unsorted order
	}
	
	LOG_MSG("Exiting DataViewerFrame::SortEvent");
}

void DataViewerFrame::OnSortEvent( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerFrame::OnSortEvent");
	DataViewerFrame::SortEvent( ev, grid, grid_base );
	LOG_MSG("Exiting DataViewerFrame::OnSortEvent");
}

void DataViewerFrame::ColSizeEvent( wxGridSizeEvent& ev,
								   wxGrid* grid,
								   DbfGridTableBase* grid_base )
{
	LOG_MSG("Entering DataViewerFrame::ColSizeEvent");
	LOG(ev.GetRowOrCol());
	LOG(ev.GetPosition().x);
	LOG(ev.GetPosition().y);
	ev.Veto();
	LOG_MSG("Exiting DataViewerFrame::ColSizeEvent");
}

void DataViewerFrame::OnColSizeEvent( wxGridSizeEvent& ev )
{
	LOG_MSG("Entering DataViewerFrame::OnColSizeEvent");
	DataViewerFrame::ColSizeEvent( ev, grid, grid_base );
	LOG_MSG("Exiting DataViewerFrame::OnColSizeEvent");
}

void DataViewerFrame::ColMoveEvent( wxGridEvent& ev,
								   wxGrid* grid,
								   DbfGridTableBase* grid_base )
{
	LOG_MSG("Entering DataViewerFrame::ColMoveEvent");
	LOG(ev.GetCol());
	LOG(ev.GetPosition().x);
	LOG(ev.GetPosition().y);	
	LOG_MSG("Exiting DataViewerFrame::ColMoveEvent");
}

void DataViewerFrame::OnColMoveEvent( wxGridEvent& ev )
{
	LOG_MSG("Entering DataViewerFrame::OnColMoveEvent");
	DataViewerFrame::ColMoveEvent( ev, grid, grid_base );
	LOG_MSG("Exiting DataViewerFrame::OnColMoveEvent");
}

void DataViewerFrame::OnColResize( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnColResize");
	DataViewerResizeColDlg dlg(grid, grid_base, this);
	if (dlg.ShowModal() == wxID_OK) {
		LOG(dlg.id);
		LOG(dlg.width);
		grid->SetColSize(dlg.id, dlg.width);
	}
	LOG_MSG("Exiting DataViewerFrame::OnColResize");
}

void DataViewerFrame::LabelLeftClickEvent( wxGridEvent& ev,
										  wxGrid* grid,
										  DbfGridTableBase* grid_base )
{
	LOG_MSG("Entering DataViewerFrame::LabelLeftClickEvent");
	LOG(ev.GetCol());
	LOG(ev.GetRow());
	LOG(ev.ShiftDown());
	int row = ev.GetRow();
	int col = ev.GetCol();
	if (col < 0 && row >= 0) {
		if (!ev.ShiftDown()) grid_base->DeselectAll();
		if (grid_base->FromGridIsSelected(row)) {
			grid_base->FromGridDeselect(row);
		} else {
			grid_base->FromGridSelect(row);
		}
		// unselect whatever the wxGrid object selects since we are doing our
		// own selection.
		grid->ClearSelection();
		grid->Refresh();
	} else {
		ev.Skip(); // continue processing this event
	}
	
	LOG_MSG("Exiting DataViewerFrame::LabelLeftClickEvent");
}

void DataViewerFrame::OnLabelLeftClickEvent( wxGridEvent& ev)
{
	LOG_MSG("Entering DataViewerFrame::OnLabelLeftClickEvent");
	DataViewerFrame::LabelLeftClickEvent( ev, grid, grid_base );
	LOG_MSG("Exiting DataViewerFrame::OnLabelLeftClickEvent");
}

void DataViewerFrame::LabelLeftDClickEvent(wxGridEvent& ev,
										   wxGrid* grid,
										   DbfGridTableBase* grid_base)
{
	LOG_MSG("Entering DataViewerFrame::LabelLeftDClickEvent");
	LOG(ev.GetCol());
	LOG(ev.GetRow());
	LOG(ev.ShiftDown());
	int row = ev.GetRow();
	int col = ev.GetCol();
	if (row < 0 && col >= -1) {
		if (col == -1) {
			// upper-right-hand-corner click
			if (grid_base->GetSortingCol() != -1) {
				grid_base->SortByDefaultDecending();
			} else if (grid_base->IsSortedAscending()) {
				grid_base->SortByDefaultDecending();
			} else {
				grid_base->SortByDefaultAscending();
			}
			grid->ClearSelection();
		} else {
			// A column double click event
			SortEvent(ev, grid, grid_base);
		}
		grid->Refresh();
	} else {
		ev.Skip(); // continue processing this event
	}
	
	LOG_MSG("Exiting DataViewerFrame::LabelLeftDClickEvent");
}

void DataViewerFrame::OnLabelLeftDClickEvent( wxGridEvent& ev)
{
	LOG_MSG("Entering DataViewerFrame::OnLabelLeftDClickEvent");
	DataViewerFrame::LabelLeftDClickEvent(ev, grid, grid_base);
	LOG_MSG("Exiting DataViewerFrame::OnLabelLeftDClickEvent");
}

void DataViewerFrame::OnCellChanged( wxGridEvent& ev )
{
	GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
	ev.Skip();
}

void DataViewerFrame::OnMoveCol( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnMoveCol");
	GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
	LOG_MSG("Exiting DataViewerFrame::OnMoveCol");
}

void DataViewerFrame::OnAddCol( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnAddCol");
	DataViewerAddColDlg dlg(grid_base, this);
	if (dlg.ShowModal() == wxID_OK) {
		GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
	}
	LOG_MSG("Exiting DataViewerFrame::OnAddCol");
}

void DataViewerFrame::OnDeleteCol( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnDeleteCol");
	DataViewerDeleteColDlg dlg(grid_base, this);
	dlg.ShowModal();
	GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
	LOG_MSG("Exiting DataViewerFrame::OnDeleteCol");
}

void DataViewerFrame::OnRandomColOrder( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnRandomColOrder");
	int cols = grid->GetNumberCols();
	wxArrayInt order(cols);
	for (int i=0; i<cols; i++) order[i] = i;
	srand( time(NULL) );
	for (int i=0; i<cols; i++) {
		int j = rand() % cols; // random position;
		int t; // temp
		t = order[i];
		order[i] = order[j];
		order[j] = t;
	}
	wxString msg;
	for (int i=0; i<cols; i++) {
		msg << i << "->" << order[i] << " ";
	}
	LOG_MSG(msg);
	grid->SetColumnsOrder(order);
	GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
	
	LOG_MSG("Exiting DataViewerFrame::OnRandomColOrder");
}

void DataViewerFrame::OnSaveDbf( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnSaveDbf");
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	DbfFileHeader t_header = grid_base->orig_header;
	
	wxString err_msg;
	bool success = grid_base->WriteToDbf(cur_dbf_fname, err_msg);
	if (!success) {
		grid_base->orig_header = t_header;
		wxMessageBox(err_msg);
		LOG_MSG(err_msg);
	} else {
		GetMenuBar()->Enable(ID_SAVE_DBF, grid_base->ChangedSinceLastSave());
		wxMessageBox("DBF file saved successfully");
		LOG_MSG("DBF file saved successfully");
	}
	LOG_MSG("Exiting DataViewerFrame::OnSaveDbf");
}

void DataViewerFrame::OnSaveDbfAs( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnSaveDbfAs");
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	DbfFileHeader t_header = grid_base->orig_header;
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxFileDialog dlg( this, "Output DBF file", wxEmptyString,
					 wxEmptyString,
					 "DBF files (*.dbf)|*.dbf",
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	dlg.SetPath(cur_dbf_fname);
	if (dlg.ShowModal() == wxID_OK) {
		wxString err_msg;
		bool success = grid_base->WriteToDbf(dlg.GetPath(), err_msg);
		if (!success) {
			grid_base->orig_header = t_header;
			wxMessageBox(err_msg);
			LOG_MSG(err_msg);
		} else {
			GetMenuBar()->Enable(ID_SAVE_DBF,
								 grid_base->ChangedSinceLastSave());
			cur_dbf_fname = dlg.GetPath();
			SetTitle("DBF Viewer - "
					 + wxFileName::FileName(cur_dbf_fname).GetFullName());
			wxMessageBox("DBF file saved successfully");
			LOG_MSG("DBF file saved successfully");
		}
	}
	
	LOG_MSG("Exiting DataViewerFrame::OnSaveDbfAs");
}

void DataViewerFrame::OnShowDbfInfo( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnShowDbfInfo");
	
	wxString version_text;
	if (grid_base->orig_header.version == 3) {
		version_text = wxString("0x03 (dBASE III+)");
	} else {
		version_text = wxString::Format("0x%X (unknown)",
										grid_base->orig_header.version);
	}
	wxString year_text = wxString::Format("%d", grid_base->orig_header.year);
	wxString month_text = wxString::Format("%d", grid_base->orig_header.month);
	wxString day_text = wxString::Format("%d", grid_base->orig_header.day);
	wxString fields_text = wxString::Format("%d",
											grid_base->orig_header.num_fields);
	wxString records_text = wxString::Format("%d",
										grid_base->orig_header.num_records);
	int bytes = (grid_base->orig_header.num_fields + 1)*32 + 1;
	bytes += grid_base->orig_header.num_records *
		grid_base->orig_header.length_each_record;
	wxString size_text = wxString::Format("%d bytes", bytes);
	
	wxString msg;
	msg << "file: " << cur_dbf_fname;
	msg << "\n  version: " << version_text;
	msg << "\n  year: " << year_text;
	msg << "\n  month: " << month_text;
	msg << "\n  day: " << day_text;
	msg << "\n  fields: " << fields_text;
	msg << "\n  records: " << records_text;
	msg << "\n  file size: " << size_text;
	
	LOG_MSG(msg);

	wxDialog dlg;
	wxXmlResource::Get()->LoadDialog(&dlg, this, "ID_DATA_VIEWER_DBF_INFO_DLG");
	wxDynamicCast(FindWindow(XRCID("ID_VERSION_TEXT")),
				  wxStaticText)->SetLabelText(version_text);
	wxDynamicCast(FindWindow(XRCID("ID_YEAR_TEXT")),
				  wxStaticText)->SetLabelText(year_text);
	wxDynamicCast(FindWindow(XRCID("ID_MONTH_TEXT")),
				  wxStaticText)->SetLabelText(month_text);
	wxDynamicCast(FindWindow(XRCID("ID_DAY_TEXT")),
				  wxStaticText)->SetLabelText(day_text);
	wxDynamicCast(FindWindow(XRCID("ID_FIELDS_TEXT")),
				  wxStaticText)->SetLabelText(fields_text);
	wxDynamicCast(FindWindow(XRCID("ID_RECORDS_TEXT")),
				  wxStaticText)->SetLabelText(records_text);
	wxDynamicCast(FindWindow(XRCID("ID_FILE_SIZE_TEXT")),
				  wxStaticText)->SetLabelText(size_text);
	dlg.SetTitle(wxFileName::FileName(cur_dbf_fname).GetFullName());

	dlg.ShowModal();
	
	LOG_MSG("Exiting DataViewerFrame::OnShowDbfInfo");
}

void DataViewerFrame::OnEditFieldProperties( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnEditFieldProperties");
	DataViewerEditFieldPropertiesDlg dlg(grid_base,
										wxDefaultPosition, wxSize(600, 400));
	dlg.ShowModal();
	
	LOG_MSG("Exiting DataViewerFrame::OnEditFieldProperties");
}

void DataViewerFrame::OnMergeTable( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnMergeTable");
	MergeTableDlg dlg(grid_base, wxDefaultPosition);
	dlg.ShowModal();
	
	LOG_MSG("Exiting DataViewerFrame::OnMergeTable");
}

void DataViewerFrame::OnChooseTimeStep( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnChooseTimeStep");
	int cur_ts = grid_base->curr_time_step;
	
	wxString* choices = new wxString[grid_base->time_steps];
	for (int i=0; i<grid_base->time_steps; i++) {
		choices[i] << grid_base->time_ids[i];
	}
	
	wxSingleChoiceDialog dlg(this, "Choose Time Step", "Time Step",
							 grid_base->time_steps, choices, (char**) 0,
							 wxOK | wxCANCEL, wxDefaultPosition );
	
	if (dlg.ShowModal() == wxID_OK) {
		if (dlg.GetSelection() != cur_ts) {
			grid_base->curr_time_step = dlg.GetSelection();
			grid->Refresh();
		}
	}
	if (choices) delete [] choices;
	
	LOG_MSG("Exiting DataViewerFrame::OnChooseTimeStep");
}

void DataViewerFrame::OnShowTimeChooser( wxCommandEvent& WXUNUSED(ev) )
{
	LOG_MSG("Entering DataViewerFrame::OnShowTimeChooser");

	TimeChooserDlg* dlg = new TimeChooserDlg(0, frames_manager, grid_base);
	dlg->Show(true);
	
	LOG_MSG("Exiting DataViewerFrame::OnShowTimeChooser");
}


void DataViewerFrame::update(FramesManager* o)
{
	// A possible title change or time step change.
	Refresh();
}
