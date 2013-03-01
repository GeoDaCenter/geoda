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
#include <wx/xrc/xmlres.h>
#include "GeoDa.h"
#include "GenUtils.h"
#include "GeneralWxUtils.h"
#include "GeoDaConst.h"
#include "logger.h"
#include "Project.h"
#include "ShapeOperations/GalWeight.h"
#include "DialogTools/FieldNewCalcSheetDlg.h"
#include "DataViewer/DbfGridTableBase.h"
#include "DataViewer/DataViewerAddColDlg.h"
#include "DataViewer/DataViewerDeleteColDlg.h"
#include "DataViewer/DataViewerEditFieldPropertiesDlg.h"
#include "DataViewer/MergeTableDlg.h"
#include "DialogTools/RangeSelectionDlg.h"
#include "NewTableViewer.h"

BEGIN_EVENT_TABLE(NewTableViewerFrame, TemplateFrame)
	EVT_ACTIVATE( NewTableViewerFrame::OnActivate )
	EVT_CLOSE( NewTableViewerFrame::OnClose )
	EVT_MENU( XRCID("wxID_CLOSE"), NewTableViewerFrame::OnMenuClose )

	EVT_GRID_CELL_RIGHT_CLICK( NewTableViewerFrame::OnRightClickEvent )
	EVT_GRID_LABEL_RIGHT_CLICK( NewTableViewerFrame::OnRightClickEvent )
	EVT_GRID_LABEL_LEFT_CLICK( NewTableViewerFrame::OnLabelLeftClickEvent )
	EVT_GRID_LABEL_LEFT_DCLICK( NewTableViewerFrame::OnLabelLeftDClickEvent )
	EVT_GRID_COL_SIZE( NewTableViewerFrame::OnColSizeEvent )
	EVT_GRID_COL_MOVE( NewTableViewerFrame::OnColMoveEvent )
	EVT_GRID_CELL_CHANGED( NewTableViewerFrame::OnCellChanged )
END_EVENT_TABLE()

NewTableViewerFrame::NewTableViewerFrame(wxFrame *parent, Project* project,
										 const wxString& title,
										 const wxPoint& pos, const wxSize& size,
										 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering NewTableViewerFrame::NewTableViewerFrame");
	
	cur_dbf_fname = project->GetMainDir() + project->GetMainName() +".dbf";
	
	grid_base = project->GetGridBase();
	grid = new wxGrid( this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
	grid->SetDefaultColSize((grid->GetDefaultColSize() * 4)/3);
	// false to not take ownership, but this uncovers a bug in wxWidgets.
	// therefore, we'll have to let the wxGrid take ownership and make
	// sure that the NewTableViewerFrame is only hidden until the
	// project is finally closed.
	grid->SetTable(grid_base, true); 
	grid->EnableDragColMove(true);
	grid->EnableDragCell(false);
	// This line causes the row to disappear on Windows and Linux, but not OSX
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
		
	LOG_MSG("Exiting NewTableViewerFrame::NewTableViewerFrame");
}

NewTableViewerFrame::~NewTableViewerFrame()
{
	LOG_MSG("In NewTableViewerFrame::~NewTableViewerFrame");
	DeregisterAsActive();
}

void NewTableViewerFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In NewTableViewerFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("NewTableViewerFrame", GetTitle());
	}
	event.Skip(false);
}

void NewTableViewerFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("Entering NewTableViewerFrame::OnClose");
	if (!MyFrame::theFrame) {
		// This is an exit event, so allow close to proceed
		event.Skip();
		LOG_MSG("Exiting NewTableViewerFrame::OnClose");
		return;
	}
	if (!MyFrame::IsProjectOpen()) {
		LOG_MSG("In NewTableViewFrame::OnClose and actually closing.");
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
		LOG_MSG("In NewTableViewFrame::OnClose, but just hiding.");
		Hide();
	}
	LOG_MSG("Exiting NewTableViewerFrame::OnClose");
}

void NewTableViewerFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In NewTableViewerFrame::OnMenuClose");
	Hide();
}

void NewTableViewerFrame::MapMenus()
{
	LOG_MSG("In NewTableViewerFrame::MapMenus");
	// Map Default Options Menus
    wxMenu* optMenu=wxXmlResource::Get()->LoadMenu("ID_DEFAULT_MENU_OPTIONS");
	GeneralWxUtils::ReplaceMenu(MyFrame::theFrame->GetMenuBar(),
								"Options", optMenu);
}

void NewTableViewerFrame::OnRightClickEvent( wxGridEvent& ev)
{
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_NEW_TABLE_VIEW_MENU_CONTEXT");
	GeneralWxUtils::EnableMenuItem(optMenu, XRCID("ID_SAVE_PROJECT"),
							project->GetGridBase()->ChangedSinceLastSave() &&
								   project->IsAllowEnableSave());
	optMenu->SetLabel(XRCID("ID_SPACE_TIME_TOOL"),
					  (project->GetGridBase()->IsTimeVariant() ?
					   "Space-Time Variable Creation Tool" :
					   "Convert to Space-Time Project"));
	PopupMenu(optMenu, ev.GetPosition());
}

void NewTableViewerFrame::OnColSizeEvent( wxGridSizeEvent& ev )
{
	LOG_MSG("Entering NewTableViewerFrame::OnColSizeEvent");
	DataViewerFrame::ColSizeEvent( ev, grid, grid_base );
	LOG_MSG("Exiting NewTableViewerFrame::OnColSizeEvent");
}

void NewTableViewerFrame::OnColMoveEvent( wxGridEvent& ev )
{
	LOG_MSG("Entering NewTableViewerFrame::OnColMoveEvent");
	DataViewerFrame::ColMoveEvent( ev, grid, grid_base );
	grid_base->notifyColMove();
	LOG_MSG("Exiting NewTableViewerFrame::OnColMoveEvent");
}

void NewTableViewerFrame::OnLabelLeftClickEvent( wxGridEvent& ev)
{
	LOG_MSG("Entering NewTableViewerFrame::OnLabelLeftClickEvent");
	LOG(ev.GetCol());
	LOG(ev.GetRow());
	LOG(ev.ShiftDown());
	int row = ev.GetRow();
	int col = ev.GetCol();
	if (col < 0 && row >= 0) {
		if (!ev.ShiftDown()) {
			grid_base->DeselectAll();
		}
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
			
	LOG_MSG("Exiting NewTableViewerFrame::OnLabelLeftClickEvent");
}

void NewTableViewerFrame::OnLabelLeftDClickEvent( wxGridEvent& ev)
{
	LOG_MSG("Entering NewTableViewerFrame::OnLabelLeftDClickEvent");
	DataViewerFrame::LabelLeftDClickEvent(ev, grid, grid_base);
	LOG_MSG("Exiting NewTableViewerFrame::OnLabelLeftDClickEvent");
}

void NewTableViewerFrame::OnCellChanged( wxGridEvent& ev )
{
	GeneralWxUtils::EnableMenuItem(MyFrame::theFrame->GetMenuBar(),
								   XRCID("ID_SAVE_PROJECT"),
							project->GetGridBase()->ChangedSinceLastSave() &&
								   project->IsAllowEnableSave());
	ev.Skip();
}

void NewTableViewerFrame::update(FramesManager* o)
{
	// A possible title change or time step change.
	Refresh();
}


