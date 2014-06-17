/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#include "TestScrollWinView.h"
#include "TestMapView.h"
#include "../logger.h"
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/settings.h> // to get wxSystemSettings
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include "../GeoDa.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"

IMPLEMENT_CLASS(TestScrollWinFrame, TemplateFrame)
BEGIN_EVENT_TABLE(TestScrollWinFrame, TemplateFrame)
	EVT_ACTIVATE(TestScrollWinFrame::OnActivate)
	EVT_CLOSE(TestScrollWinFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), TestScrollWinFrame::OnMenuClose)
END_EVENT_TABLE()

IMPLEMENT_CLASS(TestScrollWinCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(TestScrollWinCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

TestScrollWinCanvas::TestScrollWinCanvas(wxWindow *parent,
							 const wxPoint& pos,
							 const wxSize& size )
	: TemplateCanvas(parent, pos, size, true, true)
{
  //LOG_MSG("Entering TestScrollWinCanvas::TestScrollWinCanvas");

	SetBackgroundColour(*wxWHITE);  // default color
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	//LOG_MSG("Exiting TestScrollWinCanvas::TestScrollWinCanvas");
	cb = new wxComboBox(this, wxID_ANY, wxEmptyString,
									wxDefaultPosition, wxDefaultSize,
									0, NULL, 0);
	cb->Clear();
	cb->Append("Cove");
	cb->Append("Bikes");
	cb->Append("rock");
}

TestScrollWinCanvas::~TestScrollWinCanvas()
{
  //LOG_MSG("Entering TestScrollWinCanvas::~TestScrollWinCanvas");
  //LOG_MSG("Exiting TestScrollWinCanvas::~TestScrollWinCanvas");
}

void TestScrollWinCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
  //LOG_MSG("Entering TestScrollWinCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((TestScrollWinFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_TEST_SCROLL_WIN_VIEW_MENU_CONTEXT");
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	//LOG_MSG("Exiting TestScrollWinCanvas::DisplayRightClickMenu");
	cb->Clear();
}

void TestScrollWinCanvas::PaintShapes(wxDC& dc)
{
	TemplateCanvas::PaintShapes(dc);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxRED_BRUSH);
	dc.SetTextForeground(*wxBLACK);
	dc.SetTextBackground(wxTransparentColor); // new const in wxWidgets 2.9.1
	dc.SetFont(*wxSMALL_FONT);
	int y=10;
	int dy=20;
	int i=0;
	dc.DrawLine(30,30, 250,15);
	dc.DrawText("*wxSMALL_FONT", 10, y+(dy*i++));
	dc.SetFont(*wxNORMAL_FONT);
	dc.DrawText("*wxNORMAL_FONT", 10, y+(dy*i++));
	dc.SetFont(wxSystemSettings::GetFont(wxSYS_ANSI_VAR_FONT));
	dc.DrawText("GetFont(wxSYS_ANSI_VAR_FONT)", 10, y+(dy*i++));
	dc.SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
	dc.DrawText("GetFont(wxSYS_SYSTEM_FONT)", 10, y+(dy*i++));
	wxFont f1(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f1);
	dc.DrawText("8, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	wxFont f1_1(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f1_1);
	dc.DrawText("9, wxFONTFAMILY_SWISS", 10, y+(dy*i++));	
	wxFont f2(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f2);
	dc.DrawText("10, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	wxFont f3(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f3);
	dc.DrawText("11, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	wxFont f4(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f4);
	dc.DrawText("12, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	wxFont f5(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(f5);
	dc.DrawText("14, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	f5.SetPointSize(16);
	dc.SetFont(f5);
	dc.DrawText("16, wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	f5.SetPixelSize(wxSize(0,15));
	dc.SetFont(f5);
	dc.DrawText("SetPixelSize(0,15), wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	f5.SetPixelSize(wxSize(0,10));
	dc.SetFont(f5);
	dc.DrawText("SetPixelSize(0,10), wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	f5.SetPixelSize(wxSize(0,20));
	dc.SetFont(f5);
	dc.DrawText("SetPixelSize(0,20), wxFONTFAMILY_SWISS", 10, y+(dy*i++));
	dc.SetFont(*GdaConst::small_font);
	dc.DrawText("*GdaConst::small_font", 10, y+(dy*i++));
	dc.SetFont(*GdaConst::medium_font);
	dc.DrawText("*GdaConst::medium_font", 10, y+(dy*i++));
	dc.SetTextForeground(GdaConst::selectable_fill_color);
	dc.SetFont(*GdaConst::large_font);
	dc.DrawText("*GdaConst::large_font", 10, y+(dy*i++));
}

TestScrollWinFrame::TestScrollWinFrame(wxFrame *parent,
						   const wxString& title,
						   const wxPoint& pos, const wxSize& size,
						   const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering TestScrollWinFrame::TestScrollWinFrame");
	
	top_sizer = new wxFlexGridSizer( 2, 10, 10 );

	template_canvas =
	new TestScrollWinCanvas(this, wxDefaultPosition, wxSize(100,100));
	template_canvas->template_frame = this;
	template_canvas->SetScrollRate(1,1);
	
	TestScrollWinCanvas* template_canvas2 =
	new TestScrollWinCanvas(this, wxDefaultPosition, wxSize(100,100));
	template_canvas2->template_frame = this;
	template_canvas2->SetScrollRate(1,1);
	
	TestScrollWinCanvas* template_canvas3 =
	new TestScrollWinCanvas(this, wxDefaultPosition, wxSize(100,100));
	template_canvas3->template_frame = this;
	template_canvas3->SetScrollRate(1,1);
	
	TestScrollWinCanvas* template_canvas4 =
	new TestScrollWinCanvas(this, wxDefaultPosition, wxSize(100,50));
	template_canvas4->template_frame = this;
	template_canvas4->SetScrollRate(1,1);

	TestScrollWinCanvas* template_canvas5 =
	new TestScrollWinCanvas(this, wxDefaultPosition, wxSize(100,50));
	template_canvas5->template_frame = this;
	template_canvas5->SetScrollRate(1,1);	
	
	// Create a wxGrid object
	
    wxGrid* grid = new wxGrid( this,
					  -1,
					  wxPoint( 0, 0 ),
					  wxSize( 400, 300 ) );
	
    // Then we call CreateGrid to set the dimensions of the grid
    // (100 rows and 10 columns in this example)
    grid->CreateGrid( 100, 10 );
	
    // We can set the sizes of individual rows and columns
    // in pixels
    grid->SetRowSize( 0, 60 );
    grid->SetColSize( 0, 120 );
	
    // And set grid cell contents as strings
    grid->SetCellValue( 0, 0, "wxGrid is good" );
	
    // We can specify that some cells are read->only
    grid->SetCellValue( 0, 3, "This is read->only" );
    grid->SetReadOnly( 0, 3 );
	
    // Colours can be specified for grid cell contents
    grid->SetCellValue(3, 3, "green on grey");
    grid->SetCellTextColour(3, 3, *wxGREEN);
    grid->SetCellBackgroundColour(3, 3, *wxLIGHT_GREY);
	
    // We can specify the some cells will store numeric
    // values rather than strings. Here we set grid column 5
    // to hold floating point values displayed with width of 6
    // and precision of 2
    grid->SetColFormatFloat(5, 6, 2);
    grid->SetCellValue(0, 6, "3.1415");
	
	grid->SetColLabelValue(1, "Yes");
	grid->SetRowLabelValue(2, "OK Now");
	
	wxSizerFlags flags(1);
	flags.Expand().Border(wxALL, 5);
	
	//top_sizer->Add(template_canvas,
	//			   1, // make vertically stretchable 
	//			   wxEXPAND | // make horizontally stretchable
	//			   wxALL, // make border all around
	//			   10);	// set border width to 10			   

	top_sizer->Add(template_canvas, flags);
	top_sizer->Add(grid, flags);
	top_sizer->Add(template_canvas3, flags);
	
	wxBoxSizer* box_sizer = new wxBoxSizer(wxVERTICAL);
	box_sizer->Add(template_canvas4, 1, wxEXPAND | wxALL, 2);
	box_sizer->Add(template_canvas5, 1, wxEXPAND | wxALL, 2);
	top_sizer->Add(box_sizer, flags);
	
	SetSizerAndFit(top_sizer);

	top_sizer->AddGrowableCol(0);
	top_sizer->AddGrowableRow(0);
	top_sizer->AddGrowableCol(1);
	top_sizer->AddGrowableRow(1);
	LOG(top_sizer->IsRowGrowable(1));
	LOG(top_sizer->IsColGrowable(1));
	
	Show(true);
	LOG_MSG("Exiting TestScrollWinFrame::TestScrollWinFrame");
}

void TestScrollWinFrame::OnActivate(wxActivateEvent& event)
{
  //LOG_MSG("In TestScrollWinFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("TestScrollWinFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void TestScrollWinFrame::OnClose(wxCloseEvent& event)
{
  //LOG_MSG("In TestScrollWinFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void TestScrollWinFrame::OnMenuClose(wxCommandEvent& event)
{
  //LOG_MSG("In TestScrollWinFrame::OnMenuClose");
	Close();
}

void TestScrollWinFrame::MapMenus()
{
  //LOG_MSG("In TestScrollWinFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_TEST_SCROLL_WIN_VIEW_MENU_CONTEXT");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateOptionMenuItems();
}

