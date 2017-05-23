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

#include <limits>
#include <vector>
#include <boost/foreach.hpp>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../VarCalc/WeightsManInterface.h"
#include "ConnectivityMapView.h"

IMPLEMENT_CLASS(ConnectivityMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(ConnectivityMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

ConnectivityMapCanvas::ConnectivityMapCanvas(wxWindow *parent,
											 TemplateFrame* t_frame,
											 Project* project,
											 boost::uuids::uuid weights_id,
											 const wxPoint& pos,
											 const wxSize& size)
: MapCanvas(parent, t_frame, project,
			   std::vector<GdaVarTools::VarInfo>(0), std::vector<int>(0),
			   CatClassification::no_theme,
			   no_smoothing, 1, weights_id, pos, size),
	w_man_int(project->GetWManInt())
{
	SetWeightsId(weights_id);
	
	wxString w_title = project->GetWManInt()->GetShortDispName(GetWeightsId());
	
	cat_classif_def.color_scheme = CatClassification::custom_color_scheme;
	CatClassification::ChangeNumCats(1, cat_classif_def);
	cat_classif_def.colors[0] = GdaConst::conn_map_default_fill_colour;
	cat_data.CreateCategoriesAllCanvasTms(1, 1, num_obs);
	cat_data.SetCategoryColor(0 ,0, GdaConst::conn_map_default_fill_colour);
	cat_data.SetCategoryLabel(0, 0, "");
	cat_data.SetCategoryCount(0, 0, num_obs);
	for (int i=0; i<num_obs; i++) cat_data.AppendIdToCategory(0, 0, i);
	
	selectable_fill_color = GdaConst::conn_map_default_fill_colour;
	highlight_color = GdaConst::conn_map_default_highlight_colour;
	
	// Will ignore these messages
	proj_hs = project->GetHighlightState();
	proj_hs->registerObserver(this);
	
	// Used by TemplateCanvas for each instance of ConnectivityMapCanvas
	highlight_state->removeObserver(this);
	highlight_state = new HighlightState();
	highlight_state->SetSize(project->GetNumRecords());
	highlight_state->registerObserver(this);
	
	// Used to synchronize core selection amongst all ConnectivityMaps
	shared_core_hs = project->GetConMapHlightState();
	shared_core_hs->registerObserver(this);
}

ConnectivityMapCanvas::~ConnectivityMapCanvas()
{
	proj_hs->removeObserver(this);
	highlight_state->removeObserver(this);
	// ensure child won't try to removeObserver as well.  Since this is
	// the only observer, ok to simply set to null since we know it
	// was just deleted.
	highlight_state = 0;
	shared_core_hs->removeObserver(this);
}

void ConnectivityMapCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();
	
	if (mousemode == select) {
		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			} else {
				if (template_frame && template_frame->IsStatusBarVisible()) {
					prev = GetActualPos(event);
					sel1 = prev; // sel1 now has current mouse position
					
					// The following two lines will make this operate in
					// continuous selection mode
					UpdateSelectRegion();
					UpdateSelection(event.ShiftDown(), true);
					
					selectstate = start;
					DetermineMouseHoverObjects(prev);
					UpdateStatusBar();
				}
			}
		} else if (selectstate == leftdown) {
			if (event.Moving() || event.Dragging()) {
				wxPoint act_pos = GetActualPos(event);
				if (fabs((double) (prev.x - act_pos.x)) +
					fabs((double) (prev.y - act_pos.y)) > 2) {
					sel1 = prev;
					sel2 = GetActualPos(event);
					selectstate = dragging;
					remember_shiftdown = event.ShiftDown();
					UpdateSelectRegion();
					UpdateSelection(remember_shiftdown);
					UpdateStatusBar();
					Refresh();
				}
			} else if (event.LeftUp()) {
				UpdateSelectRegion();
				UpdateSelection(event.ShiftDown(), true);
				selectstate = start;
				Refresh();
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				UpdateStatusBar();
				Refresh();
			} else if (event.LeftUp() && !event.CmdDown()) {
				sel2 = GetActualPos(event);
				UpdateSelectRegion();
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
				Refresh();
			} else if (event.LeftUp() && event.CmdDown()) {
				selectstate = brushing;
				sel2 = GetActualPos(event);
				wxPoint diff = wxPoint(0,0);
				UpdateSelectRegion(false, diff);
				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				Refresh();
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		} else if (selectstate == brushing) {
			if (event.LeftIsDown()) {
			} else if (event.LeftUp()) {
				selectstate = start;
				Refresh();
			}
			else if (event.RightDown()) {
				selectstate = start;
				Refresh();
			} else if (event.Moving()) {
				wxPoint diff = GetActualPos(event) - sel2;
				sel1 += diff;
				sel2 = GetActualPos(event);
				UpdateStatusBar();
				UpdateSelectRegion(true, diff);
				UpdateSelection();
				Refresh();
			}
		} else { // unknown state
		}
	} else {
		TemplateCanvas::OnMouseEvent(event);
	}
}

// The following function assumes that the set of selectable objects
// being selected against are all points.  Since all GdaShape objects
// define a center point, this is also the default function for
// all GdaShape selectable objects.
void ConnectivityMapCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	size_t sel_shps_sz = selectable_shps.size();
	
	sel_cores.clear();
	
	if (pointsel) { // a point selection
		for (int i=0; i<sel_shps_sz; i++) {
			if (selectable_shps[i]->pointWithin(sel1)) {
				sel_cores.insert(i);
			}			
		}
	} else { // determine which obs intersect the selection region.
		if (brushtype == rectangle) {
			wxRegion rect(wxRect(sel1, sel2));
			for (int i=0; i<sel_shps_sz; i++) {
				bool contains = (rect.Contains(selectable_shps[i]->center) !=
								 wxOutRegion);
				if (contains) sel_cores.insert(i);
			}
			
		} else if (brushtype == circle) {
			double radius = GenUtils::distance(sel1, sel2);
			// determine if each center is within radius of sel1
			for (int i=0; i<sel_shps_sz; i++) {
				bool contains = (GenUtils::distance(sel1,
													selectable_shps[i]->center)
								 <= radius);
				if (contains) sel_cores.insert(i);
			}
		} else if (brushtype == line) {
			wxRegion rect(wxRect(sel1, sel2));
			// determine if each center is within rect and also within distance
			// 3.0 of line passing through sel1 and sel2
			// Note: we can speed up calculations for GenUtils::pointToLineDist
			// by reusing parts of the calculation.  See
			// GenUtils::pointToLineDist for algorithm that the following
			// is based upon.
			double p1x = sel1.x;
			double p1y = sel1.y;
			double p2x = sel2.x;
			double p2y = sel2.y;
			double p2xMp1x = p2x - p1x;
			double p2yMp1y = p2y - p1y;
			double dp1p2 = GenUtils::distance(sel1, sel2);
			double delta = 3.0 * dp1p2;
			for (int i=0; i<sel_shps_sz; i++) {
				bool contains = (rect.Contains(selectable_shps[i]->center) !=
								 wxOutRegion);
				if (contains) {
					double p0x = selectable_shps[i]->center.x;
					double p0y = selectable_shps[i]->center.y;
					// determine if selectable_shps[i]->center is within
					// distance 3.0 of line passing through sel1 and sel2
					if (abs(p2xMp1x * (p1y-p0y) - (p1x-p0x) * p2yMp1y) >
						delta ) contains = false;
				}
				if (contains) sel_cores.insert(i);
			}
		}
	}
	
	// update shared core
	std::vector<bool> temp_sel_cores(project->GetNumRecords(), false);
	BOOST_FOREACH(long i, sel_cores) {
		temp_sel_cores[i] = true;
	}
	
	std::vector<bool>& hs = shared_core_hs->GetHighlight();
    bool selection_changed = false;
    
	for (size_t	i=0, sz=project->GetNumRecords(); i<sz; i++) {
		if (!hs[i] && temp_sel_cores[i]) {
            hs[i] = true;
            selection_changed = true;
		} else if (hs[i] && !temp_sel_cores[i]) {
            hs[i] = false;
            selection_changed = true;
		}
	}
    if (selection_changed) {
		shared_core_hs->SetEventType(HLStateInt::delta);
		shared_core_hs->notifyObservers();
	}
}

void ConnectivityMapCanvas::UpdateFromSharedCore()
{
	sel_cores.clear();
	std::vector<bool>& sc_hs = shared_core_hs->GetHighlight();
	for (int i=0, sz=sc_hs.size(); i<sz; ++i) {
		if (sc_hs[i]) sel_cores.insert(i);
	}
	
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	// find set of all neighbors of cores
	core_nbrs.clear();
	w_man_int->GetNbrsExclCores(weights_id, sel_cores, core_nbrs);
	
	for (size_t	i=0, sz=project->GetNumRecords(); i<sz; i++) {
		bool is_sel = core_nbrs.find(i) != core_nbrs.end();
		if (!hs[i] && is_sel) {
            hs[i] = true;
            selection_changed = true;
		} else if (hs[i] && !is_sel) {
            hs[i] = false;
            selection_changed = true;
		}
	}
    
    
	if ( selection_changed ) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
	
	UpdateStatusBar();
}

/** Don't draw selection outline */
//void ConnectivityMapCanvas::PaintSelectionOutline(wxDC& dc)
//{
//}

void ConnectivityMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	if (ConnectivityMapFrame* f =
		dynamic_cast<ConnectivityMapFrame*>(template_frame)) {
		f->OnActivate(ae);
	}
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CONNECTIVITY_MAP_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	if (template_frame) {
		template_frame->UpdateContextMenuItems(optMenu);
		template_frame->PopupMenu(optMenu, pos + GetPosition());
		template_frame->UpdateOptionMenuItems();
	}
}

wxString ConnectivityMapCanvas::GetCanvasTitle()
{
	wxString s;
	s << "Connectivity Map - " << w_man_int->GetLongDispName(weights_id);
	return s;
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by connectivity maps */
bool ConnectivityMapCanvas::ChangeMapType(
							CatClassification::CatClassifType new_map_theme,
							SmoothingType new_map_smoothing)
{
	return false;
}

void ConnectivityMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapCanvas::SetCheckMarks(menu);
}

/** Time changes have no effect */
void ConnectivityMapCanvas::TimeChange()
{
}

/** Nothing to do */
void ConnectivityMapCanvas::CreateAndUpdateCategories()
{
}

void ConnectivityMapCanvas::ChangeWeights(boost::uuids::uuid new_id)
{
	if (new_id == GetWeightsId() || new_id.is_nil()) return;
	SetWeightsId(new_id);
	
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	// find set of all neighbors of cores
	core_nbrs.clear();
	w_man_int->GetNbrsExclCores(GetWeightsId(), sel_cores, core_nbrs);
	
	if (core_nbrs.size() == 1) 
		
		int num_obs = project->GetNumRecords();
	for (int i=0; i<num_obs; i++) {
		bool is_sel = core_nbrs.find(i) != core_nbrs.end();
		if (!hs[i] && is_sel) {
            hs[i] = true;
            selection_changed = true;
		} else if (hs[i] && !is_sel) {
            hs[i] = false;
            selection_changed = true;
		}
	}
	
	if ( selection_changed ) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}

void ConnectivityMapCanvas::update(HLStateInt* o)
{
	if (o == proj_hs) {
	} else if (o == highlight_state) {
		TemplateCanvas::update(o);
	} else { // o == shared_core_hs
		UpdateFromSharedCore();
	}
}

void ConnectivityMapCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = 0;
	if (ConnectivityMapFrame* f =
		dynamic_cast<ConnectivityMapFrame*>(template_frame)) {
		sb = f->GetStatusBar();
	}
	if (!sb) return;
	wxString s;
	if (mousemode == select) {
		if (sel_cores.size() == 1) {
			long cid = *sel_cores.begin();
			s << "obs " << w_man_int->RecNumToId(GetWeightsId(), cid);
			s << " has " << core_nbrs.size() << " neighbor";
			if (core_nbrs.size() != 1) s << "s";
			if (core_nbrs.size() > 0) {
				s << ": ";
				int n_cnt = 0;
				for (std::set<long>::const_iterator it = core_nbrs.begin();
					 it != core_nbrs.end() && n_cnt <= 20; ++it) {
					s << w_man_int->RecNumToId(GetWeightsId(), (*it));
					if (n_cnt+1 < core_nbrs.size()) s << ", ";
					++n_cnt;
				}
				if (core_nbrs.size() > 20) s << "...";
			} else {
				s << ".";
			}
		}
	}
	sb->SetStatusText(s);
}



IMPLEMENT_CLASS(ConnectivityMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(ConnectivityMapFrame, MapFrame)
	EVT_ACTIVATE(ConnectivityMapFrame::OnActivate)
END_EVENT_TABLE()

ConnectivityMapFrame::ConnectivityMapFrame(wxFrame *parent, Project* project,
										   boost::uuids::uuid weights_id_s,
										   const wxPoint& pos,
										   const wxSize& size,
										   const long style)
: MapFrame(parent, project, pos, size, style)
{
	wxLogMessage("Open ConnectivityMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
	
	template_legend = 0;
    
    wxPanel* rpanel = new wxPanel(this);
    
	template_canvas = new ConnectivityMapCanvas(rpanel, this, project,
												weights_id_s,
												wxDefaultPosition,
												wxDefaultSize);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);

    wxPanel* toolbar_panel = new wxPanel(this,-1, wxDefaultPosition);
    wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    wxToolBar* tb = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    tb->EnableTool(XRCID("ID_SELECT_INVERT"), false);
    SetupToolbar();
    toolbar_sizer->Add(tb, 0, wxEXPAND|wxALL);
    toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL);
    sizer->Add(rpanel, 1, wxEXPAND|wxALL);
    SetSizer(sizer);

	SetAutoLayout(true);
    
    DisplayStatusBar(true);
    SetTitle(template_canvas->GetCanvasTitle());
    
	Show(true);
}

ConnectivityMapFrame::~ConnectivityMapFrame()
{
}

void ConnectivityMapFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
        wxLogMessage("In ConnectivityMapFrame::OnActivate");
		RegisterAsActive("ConnectivityMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void ConnectivityMapFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
	LoadMenu("ID_CONNECTIVITY_MAP_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ConnectivityMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((ConnectivityMapCanvas*) template_canvas)->
			SetCheckMarks(mb->GetMenu(menu));
	}
}

void ConnectivityMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}


void ConnectivityMapFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->GetHighlightState();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	int num_obs = project->GetNumRecords();
	for (int i=0; i<num_obs; i++) {
		if (!hs[i] && elem[i]) {
            hs[i] = true;
            selection_changed = true;
		} else if (hs[i] && !elem[i]) {
            hs[i] = false;
            selection_changed = true;
		}
	}
    
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}

void ConnectivityMapFrame::OnSelectCores(wxCommandEvent& event)
{
	wxLogMessage("Entering ConnectivityMapFrame::OnSelectCores");
	
	std::vector<bool> elem(project->GetNumRecords(), false);
	CoreSelectHelper(elem);
}

void ConnectivityMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	wxLogMessage("Entering ConnectivityMapFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(project->GetNumRecords(), false);
	CoreSelectHelper(elem);
}

void ConnectivityMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	wxLogMessage("Entering ConnectivityMapFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(project->GetNumRecords(), false);
	CoreSelectHelper(elem);
}

void ConnectivityMapFrame::ChangeWeights(boost::uuids::uuid new_id)
{
	if (new_id == ((MapCanvas*) template_canvas)->GetWeightsId() ||
		new_id.is_nil()) return;
	((ConnectivityMapCanvas*) template_canvas)->ChangeWeights(new_id);
	UpdateTitle();
}
