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

#include "TestMapView.h"
#include "../logger.h"
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../GeoDa.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"
#include <boost/foreach.hpp>
#include "../ShapeOperations/ShapeUtils.h"
#include "../GdaConst.h"

IMPLEMENT_CLASS(TestMapFrame, TemplateFrame)
BEGIN_EVENT_TABLE(TestMapFrame, TemplateFrame)
	EVT_ACTIVATE(TestMapFrame::OnActivate)
	EVT_CLOSE(TestMapFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), TestMapFrame::OnMenuClose)
END_EVENT_TABLE()

IMPLEMENT_CLASS(TestMapCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(TestMapCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

TestMapCanvas::TestMapCanvas(wxWindow *parent,
							 const wxPoint& pos,
							 const wxSize& size )
	: TemplateCanvas(parent, pos, size, true, true)
{
	using namespace Shapefile;
	
	LOG_MSG("Entering TestMapCanvas::TestMapCanvas");
	Project* project = GdaFrame::GetProject();
	highlight_state = &(project->GetHighlightState());
	shps_orig_xmin = project->main_data.header.bbox_x_min;
	shps_orig_ymin = project->main_data.header.bbox_y_min;
	shps_orig_xmax = project->main_data.header.bbox_x_max;
	shps_orig_ymax = project->main_data.header.bbox_y_max;
	
	double scale_x, scale_y, trans_x, trans_y;
	GdaScaleTrans::calcAffineParams(shps_orig_xmin, shps_orig_ymin,
								   shps_orig_xmax, shps_orig_ymax,
								   virtual_screen_marg_top,
								   virtual_screen_marg_bottom,
								   virtual_screen_marg_left,
								   virtual_screen_marg_right,
								   size.GetWidth(), size.GetHeight(),
								   fixed_aspect_ratio_mode,
								   fit_to_window_mode,
								   &scale_x, &scale_y, &trans_x, &trans_y,
								   0, 0,
								   &current_shps_width, &current_shps_height);
	LOG(current_shps_width);
	LOG(current_shps_height);
	fixed_aspect_ratio_val = current_shps_width / current_shps_height;
	LOG(fixed_aspect_ratio_val);
	
	// Populate TemplateCanvas::selectable_shps with some shapes
	
	CreateSelShpsFromProj(selectable_shps, project);
	ResizeSelectableShps();
	wxBrush t_brush(GdaConst::map_default_fill_colour);
	wxPen t_pen(GdaConst::map_default_outline_colour,
				GdaConst::map_default_outline_width);
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) {
		shp->pen = t_pen;
		shp->brush = t_brush;
	}

	highlight_state->registerObserver(this);

	SetBackgroundColour(*wxWHITE);  // default color
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting TestMapCanvas::TestMapCanvas");
}

TestMapCanvas::~TestMapCanvas()
{
	LOG_MSG("Entering TestMapCanvas::~TestMapCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting TestMapCanvas::~TestMapCanvas");
}

void TestMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering TestMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((TestMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu =
		wxXmlResource::Get()->LoadMenu("ID_TESTMAP_VIEW_MENU_CONTEXT");
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos);
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting TestMapCanvas::DisplayRightClickMenu");
}

TestMapFrame::TestMapFrame(wxFrame *parent, Project* project,
						   const wxString& title,
						   const wxPoint& pos, const wxSize& size,
						   const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering TestMapFrame::TestMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	LOG(width);
	LOG(height);
	template_canvas = new TestMapCanvas(this, wxDefaultPosition,
										wxSize(width,height));
	template_canvas->template_frame = this;
	template_canvas->SetScrollRate(1,1);
	
	Show(true);
	LOG_MSG("Exiting TestMapFrame::TestMapFrame");
}

void TestMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In TestMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("TestMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void TestMapFrame::OnClose(wxCloseEvent& event)
{
	LOG_MSG("In TestMapFrame::OnClose");
	DeregisterAsActive();
	Destroy();
}

void TestMapFrame::OnMenuClose(wxCommandEvent& event)
{
	LOG_MSG("In TestMapFrame::OnMenuClose");
	Close();
}

void TestMapFrame::MapMenus()
{
	LOG_MSG("In TestMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_TESTMAP_VIEW_MENU_CONTEXT");
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);
	UpdateOptionMenuItems();
}
