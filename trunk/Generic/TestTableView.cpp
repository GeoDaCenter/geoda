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

#include "TestTableView.h"
#include "../logger.h"
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/grid.h>
#include "../GeoDa.h"
#include "../Project.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"


IMPLEMENT_CLASS(TestTableFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(TestTableFrame, TemplateFrame)
	EVT_ACTIVATE(TestTableFrame::OnActivate)
	EVT_CLOSE(TestTableFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), TestTableFrame::OnMenuClose)
END_EVENT_TABLE()


TestTableFrame::TestTableFrame(wxFrame *parent, Project* project,
						   const wxString& title,
						   const wxPoint& pos, const wxSize& size,
						   const long style)
: TemplateFrame(parent, project, title, pos, size, style), my_grid(0)
{
	LOG_MSG("Entering TestTableFrame::TestTableFrame");
	
	my_grid = new wxGrid( this, wxID_ANY,
								 wxDefaultPosition, GetClientSize() );
	
    // Then we call CreateGrid to set the dimensions of the grid
    // (100 rows and 10 columns in this example)
    my_grid->CreateGrid( project->GetNumRecords(),
						project->dbf_reader->getNumFields() );
	
	my_grid->SetSelectionMode(wxGrid::wxGridSelectRows);
	
    // We can set the sizes of individual rows and columns
    // in pixels
    my_grid->SetRowSize( 0, 60 );
    my_grid->SetColSize( 0, 120 );
	
    // And set grid cell contents as strings
    my_grid->SetCellValue( 0, 0, "wxGrid is good" );
	
    // We can specify that some cells are read->only
    my_grid->SetCellValue( 0, 3, "This is read->only" );
    my_grid->SetReadOnly( 0, 3 );
	
    // Colours can be specified for grid cell contents
    my_grid->SetCellValue(3, 3, "green on grey");
    my_grid->SetCellTextColour(3, 3, *wxGREEN);
    my_grid->SetCellBackgroundColour(3, 3, *wxLIGHT_GREY);
	
    // We can specify the some cells will store numeric
    // values rather than strings. Here we set grid column 5
    // to hold floating point values displayed with width of 6
    // and precision of 2
    my_grid->SetColFormatFloat(5, 6, 2);
    my_grid->SetCellValue(0, 6, "3.1415");
	
	my_grid->SetColLabelValue(1, "Yes");
	my_grid->SetRowLabelValue(2, "OK Now");
	
	std::vector<DbfFieldDesc> dbf_fields = project->dbf_reader->getFieldDescs();
	DbfFileHeader dbf_header = project->dbf_reader->getFileHeader();
	for (int j=0, jend=dbf_fields.size(); j<jend; j++) {
		my_grid->SetColLabelValue(j, dbf_fields[j].name);
	}
	for (int i=0, iend=dbf_header.num_records; i<iend; i++) {
		for (int j=0, jend=dbf_fields.size(); j<jend; j++) {
			wxString val;
			val << "(" << i << ", " << j << ")";
			my_grid->SetCellValue(i,  j, val);
		}
	}
	
	my_grid->SelectRow(4, true);
	my_grid->SelectRow(5, true);
	my_grid->SelectRow(0, true);
	
	//template_canvas = new TestTableCanvas(this, wxDefaultPosition,
	//									  GetClientSize());
	//template_canvas->template_frame = this;
	//template_canvas->SetScrollRate(1,1);
	
	highlight_state = &(project->highlight_state);
	highlight_state->registerObserver(this);
	
	Show(true);
	LOG_MSG("Exiting TestTableFrame::TestTableFrame");
}

TestTableFrame::~TestTableFrame()
{
	LOG_MSG("Entering TestMapFrame::~TestMapFrame");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting TestMapFrame::~TestMapFrame");	
}

void TestTableFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In TestTableFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("TestTableFrame", GetTitle());
	}
    if ( event.GetActive() && my_grid ) my_grid->SetFocus();
}

void TestTableFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In TestTableFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void TestTableFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In TestTableFrame::OnMenuClose");
	Close();
}

void TestTableFrame::MapMenus()
{
	LOG_MSG("In TestTableFrame::MapMenus");
	wxMenuBar* mb = MyFrame::theFrame->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_TEST_TABLE_VIEW_MENU_CONTEXT");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateOptionMenuItems();
}

/**
 Impelmentation of HighlightStateObservable interface function.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void TestTableFrame::update(HighlightState* o)
{
	LOG_MSG("Entering TestTableFrame::update");
	
	//wxClientDC dc(this);
	//DoPrepareDC(dc);
	
	// Draw the updated objects.
	//LOG_MSG("Painting Updated Shapes");
	
	// int total = highlight_state->GetTotalNewlyUnhighlighted();
	//std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	if (highlight_state->GetTotalNewlyUnhighlighted() > 0) {
		my_grid->ClearSelection();
		std::vector<bool>highlight = highlight_state->GetHighlight();
		for (int i=0, iend=highlight.size(); i<iend; i++) {
			if (highlight[i]) my_grid->SelectRow(i, true);
		}
	} else {
		int total = highlight_state->GetTotalNewlyHighlighted();
		std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
		for (int i=0, iend = total; i<total; i++) {
			my_grid->SelectRow(nh[i], true);
		}
	}

	//for (int i=0; i<total; i++) {
		//selectable_shps[nuh[i]]->highlight = false;
		//DrawMySelShape(nuh[i], dc);
	//}
	
	//total = highlight_state->GetTotalNewlyHighlighted();
	//std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	//for (int i=0; i<total; i++) {
		//selectable_shps[nh[i]]->highlight = true;
		//DrawMySelShape(nh[i], dc);
	//}
	
	// Redraw the foreground
	//BOOST_FOREACH(MyShape* shp, foreground_shps) {
	//	shp->paintSelf(dc);
	//}
	
	// Draw the the selection region if needed
	//PaintSelectionOutline(dc);
	
	// Draw and scroll/scale-invarant controls such as zoom/pan buttons
	//PaintControls(dc);
	LOG_MSG("Exiting TestTableFrame::update");
}

