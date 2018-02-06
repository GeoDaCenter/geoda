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
#include <math.h>
#include <map>
#include <stdlib.h>

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/clipbrd.h>
#include <wx/splitter.h>
#include <wx/overlay.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/menu.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcgraph.h>

#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
//#include <boost/geometry/index/rtree.hpp>

#include "DialogTools/SaveToTableDlg.h"
#include "Explore/CatClassifManager.h"


#include "GdaShape.h"
#include "ShpFile.h"
#include "GeoDa.h"
#include "Project.h"
#include "GdaConst.h"
#include "GenUtils.h"
#include "GenGeomAlgs.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"
#include "GdaConst.h"
#include "logger.h"


using namespace std;
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(boost::geometry::cs::cartesian)

IMPLEMENT_CLASS(TemplateCanvas, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateCanvas, wxScrolledWindow)
	EVT_SIZE(TemplateCanvas::OnSize)
    EVT_IDLE(TemplateCanvas::OnIdle)
	EVT_PAINT(TemplateCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	EVT_KEY_DOWN(TemplateCanvas::OnKeyEvent)
	EVT_KEY_UP(TemplateCanvas::OnKeyEvent)
	EVT_SCROLLWIN(TemplateCanvas::OnScrollChanged)
    EVT_TIMER(-1, TemplateCanvas::OnHighlightTimerEvent)
#ifdef __WIN32__
	EVT_SCROLLWIN_LINEUP(TemplateCanvas::OnScrollUp)
	EVT_SCROLLWIN_LINEDOWN(TemplateCanvas::OnScrollDown)
#endif
END_EVENT_TABLE()

TemplateCanvas::TemplateCanvas(wxWindow* parent,
                               TemplateFrame* template_frame_,
                               Project* project_s,
                               HLStateInt* hl_state_int_,
                               const wxPoint& pos,
                               const wxSize& size,
                               bool fixed_aspect_ratio_mode_s,
                               bool fit_to_window_mode_s,
                               bool enable_high_dpi_support_)
: wxScrolledWindow(parent, -1, pos, size,
                   wxNO_FULL_REPAINT_ON_RESIZE | wxCLIP_CHILDREN),
mousemode(select), selectstate(start), brushtype(rectangle), is_brushing(false),
scrollbarmode(none), remember_shiftdown(false), project(project_s),
highlight_state(hl_state_int_), template_frame(template_frame_),
selectable_outline_visible(true), user_canvas_background_color(false),
selectable_outline_color(GdaConst::selectable_outline_color),
selectable_fill_color(GdaConst::selectable_fill_color),
highlight_color(GdaConst::highlight_color),
canvas_background_color(GdaConst::canvas_background_color),
selectable_shps_type(mixed), use_category_brushes(false),
draw_sel_shps_by_z_val(false),
isResize(false), 
layer0_bm(0), layer1_bm(0), layer2_bm(0), faded_layer_bm(0),
layer0_valid(false), layer1_valid(false), layer2_valid(false),
total_hover_obs(0), max_hover_obs(11), hover_obs(11),
is_pan_zoom(false), prev_scroll_pos_x(0), prev_scroll_pos_y(0),
useScientificNotation(false), is_showing_brush(false),
axis_display_precision(2), enable_high_dpi_support(enable_high_dpi_support_),
MASK_R(183), MASK_G(184), MASK_B(185)
{
    highlight_timer = new wxTimer(this);
    
    // default is one time slice
	cat_data.CreateEmptyCategories(1, highlight_state->GetHighlightSize());
    
    // will set the correct cursor for current mode
	SetMouseMode(mousemode);
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

/**
 The destructor.  Note that all destructors in C++ should be declared
 "virtual".  Also note that super-class destructors are called automatically,
 and it is a mistake to call them explicitly (unlike for consturctors or
 other virtual methods).  All of the GdaShape objects in
 #selectable_shps are deleted in this destructor.
 */
TemplateCanvas::~TemplateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) delete shp;
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) delete shp;
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) delete shp;

    if (HasCapture()) {
        ReleaseMouse();
    }
    deleteLayerBms();
}

void TemplateCanvas::OnHighlightTimerEvent(wxTimerEvent &event)
{
    highlight_state->SetEventType(HLStateInt::delta);
    highlight_state->notifyObservers(this);
    highlight_timer->Stop();
}

// We will handle drawing our background in a paint event
// handler.  So, do nothing in this handler.
void TemplateCanvas::OnEraseBackground(wxEraseEvent& event)
{
}

void TemplateCanvas::SetScientificNotation(bool flag)
{
    useScientificNotation = flag;
}

void TemplateCanvas::deleteLayerBms()
{
	if (layer0_bm) delete layer0_bm; layer0_bm = 0;
	if (layer1_bm) delete layer1_bm; layer1_bm = 0;
	if (layer2_bm) delete layer2_bm; layer2_bm = 0;
    if (faded_layer_bm) delete faded_layer_bm; faded_layer_bm = 0;
    
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

void TemplateCanvas::resizeLayerBms(int width, int height)
{
	deleteLayerBms();

    int vs_w, vs_h;
    GetClientSize(&vs_w, &vs_h);
    
    if (vs_w <= 0) vs_w = 1;
    if (vs_h <=0 ) vs_h = 1;
    
    if (enable_high_dpi_support) {
        double scale_factor = GetContentScaleFactor();

        layer0_bm = new wxBitmap;
        layer1_bm = new wxBitmap;
        layer2_bm = new wxBitmap;
        layer0_bm->CreateScaled(vs_w, vs_h, 32, scale_factor);
        layer1_bm->CreateScaled(vs_w, vs_h, 32, scale_factor);
        layer2_bm->CreateScaled(vs_w, vs_h, 32, scale_factor);
    } else {
    	layer0_bm = new wxBitmap(vs_w, vs_h, 32);
    	layer1_bm = new wxBitmap(vs_w, vs_h, 32);
    	layer2_bm = new wxBitmap(vs_w, vs_h, 32);
    }

	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

// redraw everything
void TemplateCanvas::invalidateBms()
{
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
    ResetFadedLayer();
}

bool TemplateCanvas::GetFixedAspectRatioMode()
{
	return last_scale_trans.fixed_aspect_ratio;
}

void TemplateCanvas::SetFixedAspectRatioMode(bool mode)
{
    last_scale_trans.SetFixedAspectRatio(mode);

	ResizeSelectableShps();
}

void TemplateCanvas::SetDisplayPrecision(int n)
{
    axis_display_precision = n;
    PopulateCanvas();    
}

bool TemplateCanvas::GetFitToWindowMode()
{
	return fit_to_window_mode;
}

void TemplateCanvas::SetFitToWindowMode(bool mode)
{
	fit_to_window_mode = mode;
	scrollbarmode = none;
    
	if (fit_to_window_mode) {
		is_pan_zoom = false;

		prev_scroll_pos_x = 0;
		prev_scroll_pos_y = 0;
		
        ResetShapes();
	}
}

void TemplateCanvas::OnKeyEvent(wxKeyEvent& event)
{
	if (mousemode == zoom) {
		if (event.ShiftDown()) {

			SetCursor(GdaConst::zoomOutCursor);
		} else {
			SetCursor(GdaConst::zoomInCursor);
		}
	}
	event.Skip();
}

#ifdef __WIN32__
void TemplateCanvas::OnScrollUp(wxScrollWinEvent& event)
{
	/*
	int shp_h = ext_shps_orig_ymax - ext_shps_orig_ymin;
	int shp_w = ext_shps_orig_xmax - ext_shps_orig_xmin;
	int vs_w = 0, vs_h = 0;
	GetClientSize(&vs_w, &vs_h);
	int offset = -1;
	int orient = event.GetOrientation();
	if (orient == wxVERTICAL) {
		double delta = -offset / (double)vs_h * shp_h;
		current_map_y_min += delta;
		current_map_y_max += delta;
		prev_scroll_pos_y -= 1;
	} else if (orient == wxHORIZONTAL) {
		double delta = offset / (double)vs_w * shp_w;
		current_map_x_min += delta;
		current_map_x_max += delta;
		prev_scroll_pos_x -= 1;
	}

	ResizeSelectableShps();
	event.Skip();
	*/
}
void TemplateCanvas::OnScrollDown(wxScrollWinEvent& event)
{
	/*
	int shp_h = ext_shps_orig_ymax - ext_shps_orig_ymin;
	int shp_w = ext_shps_orig_xmax - ext_shps_orig_xmin;
	int vs_w = 0, vs_h = 0;
	GetClientSize(&vs_w, &vs_h);
	int offset = 1;
	int orient = event.GetOrientation();
	if (orient == wxVERTICAL) {
		double delta = -offset / (double)vs_h * shp_h;
		current_map_y_min += delta;
		current_map_y_max += delta;
		prev_scroll_pos_y += 1;
	} else if (orient == wxHORIZONTAL) {
		double delta = offset / (double)vs_w * shp_w;
		current_map_x_min += delta;
		current_map_x_max += delta;
		prev_scroll_pos_x += 1;
	}

	ResizeSelectableShps();
	event.Skip();
	*/
}
#endif

void TemplateCanvas::OnScrollChanged(wxScrollWinEvent& event)
{
	//layer0_valid = false;
	
	int orient = event.GetOrientation();
	int pos_e = event.GetPosition(); // this was often returning 0 on Windows
	int pos_w = GetScrollPos(orient);
    
	// There must be a bug in wxWidgets 3.0 for Windows because often
	// pos_e == 0 when it shouldn't be.  When pos_e == 0, we use the
	// value from pos_w instead.  However, pos_w is often the same as
	// the previous value when it shouldn't be.  This at least prevents
	// the window from jumping around.  Actually dragging the scroll
	// bars by the thumb slider works.
	int pos = (pos_e == 0) ? pos_w : pos_e; 

	if (pos!=0 && is_pan_zoom) {
        int v_offset = 0;
        int h_offset = 0;
        
		if (orient == wxHORIZONTAL) {
			h_offset = pos - prev_scroll_pos_x ;
			prev_scroll_pos_x = pos;
		} else if (orient == wxVERTICAL) {
			v_offset = -pos + prev_scroll_pos_y ;
			prev_scroll_pos_y = pos;
		}

        last_scale_trans.ScrollView(h_offset, v_offset);
		ResizeSelectableShps();
	}
	event.Skip();
}

wxString TemplateCanvas::GetCanvasStateString()
{
	return "";
}


void TemplateCanvas::ResizeSelectableShps(int virtual_scrn_w,
										  int virtual_scrn_h)
{
    int vs_w = virtual_scrn_w;
    int vs_h = virtual_scrn_h;
    
    if (vs_w <= 0 && vs_h <=0 ) {
        GetClientSize(&vs_w, &vs_h);
    }
    
    // view: extent, margins, width, height
    last_scale_trans.SetView(vs_w, vs_h);

    if (last_scale_trans.IsValid()) {
		BOOST_FOREACH( GdaShape* ms, background_shps ) {
			if (ms) ms->applyScaleTrans(last_scale_trans);
		}
		BOOST_FOREACH( GdaShape* ms, selectable_shps ) {
			if (ms) ms->applyScaleTrans(last_scale_trans);
		}
    	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
    		if (ms) ms->applyScaleTrans(last_scale_trans);
    	}
	}
    layer0_valid = false;
    layer1_valid = false;
    layer2_valid = false;
    ResetFadedLayer();
}

void TemplateCanvas::ResetBrushing()
{
    is_showing_brush = false;
    sel1.x = 0;
    sel1.y = 0;
    sel2.x = 0;
    sel2.y = 0;
}

void TemplateCanvas::ResetShapes()
{
    last_scale_trans.Reset();
	is_pan_zoom = false;
    
    ResetFadedLayer();
    
    ResetBrushing();
	SetMouseMode(select);
	ResizeSelectableShps();
}

void TemplateCanvas::ResetFadedLayer()
{
    if (faded_layer_bm) {
        delete faded_layer_bm;
        faded_layer_bm = NULL;
    }
    
}

void TemplateCanvas::ZoomShapes(bool is_zoomin)
{
	if (sel2.x == 0 && sel2.y==0)
        return;
    
	if (sel1.x == sel2.x)
        sel2.x = sel1.x + 2;
    
	if (sel1.y == sel2.y)
        sel2.y = sel1.y + 2;
    
    last_scale_trans.Zoom(is_zoomin, sel1, sel2);

	is_pan_zoom = true;
    is_showing_brush = false;

    ResetBrushing();
	ResizeSelectableShps();
}

void TemplateCanvas::PanShapes()
{
	if (sel2.x == 0 && sel2.y==0) 
        return;
   
    last_scale_trans.PanView(sel1, sel2);
   
	is_pan_zoom = true;
    is_showing_brush = false;

    ResetBrushing();
	ResizeSelectableShps();
}

void TemplateCanvas::SetMouseMode(MouseMode mode)
{
	mousemode = mode;
	if (mousemode == select) {
		SetCursor(*wxSTANDARD_CURSOR);
        
	} else if (mousemode == pan) {
		SetCursor(wxCursor(wxCURSOR_HAND));
        
	} else if (mousemode == zoom) {
		if (remember_shiftdown) 
			SetCursor(GdaConst::zoomOutCursor);
		else
			SetCursor(GdaConst::zoomInCursor);
        
	} else if (mousemode == zoomout) {
        SetCursor(GdaConst::zoomOutCursor);
        
	} else { // default
		SetCursor(*wxSTANDARD_CURSOR);
	}
}

std::vector<int> TemplateCanvas::CreateSelShpsFromProj(vector<GdaShape*>& selectable_shps,
                                           Project* project)
{
	using namespace Shapefile;
    std::vector<int> empty_shps_ids;
    
    if (selectable_shps.size() > 0) {
        return empty_shps_ids;
    }
	int num_recs = project->GetNumRecords();
	selectable_shps.resize(num_recs);
    
	vector<MainRecord>& records = project->main_data.records;
	Header& hdr = project->main_data.header;
	
	if (hdr.shape_type == Shapefile::POINT_TYP) {
		PointContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PointContents*) records[i].contents_p;
			if (pc->shape_type == 0) {
                empty_shps_ids.push_back(i);
				selectable_shps[i] = new GdaPoint();
            } else {
				selectable_shps[i] = new GdaPoint(wxRealPoint(pc->x,pc->y));
			}

		}
	} else if (hdr.shape_type == Shapefile::POLYGON) {

		PolygonContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PolygonContents*) records[i].contents_p;
            if (pc->shape_type == 0) {
                empty_shps_ids.push_back(i);
                selectable_shps[i] = new GdaPolygon();
            } else {
                selectable_shps[i] = new GdaPolygon(pc);
            }
		}
        
	} else if (hdr.shape_type == Shapefile::POLY_LINE) {
		PolyLineContents* pc = 0;
		//wxPen pen(GdaConst::selectable_fill_color, 1, wxSOLID);
		for (int i=0; i<num_recs; i++) {
			pc = (PolyLineContents*) records[i].contents_p;
            if (pc->shape_type == 0) {
                empty_shps_ids.push_back(i);
                selectable_shps[i] = new GdaPolyLine();
            } else {
                selectable_shps[i] = new GdaPolyLine(pc);
            }
		}
	}
   
    return empty_shps_ids;
}

wxRealPoint TemplateCanvas::MousePntToObsPnt(const wxPoint &pt)
{
    wxPoint new_pt = pt;
    wxRealPoint data_pt = last_scale_trans.View2Data(new_pt);
    return data_pt;
}

void TemplateCanvas::SetSelectableOutlineVisible(bool visible)
{
	selectable_outline_visible = visible;
    
	layer0_valid = false;

	UpdateSelectableOutlineColors();
}

bool TemplateCanvas::IsSelectableOutlineVisible()
{
	return selectable_outline_visible;
}

void TemplateCanvas::SetBackgroundColorVisible(bool visible)
{
	user_canvas_background_color = visible;
    
	layer0_valid = false;
    ReDraw();
}

bool TemplateCanvas::IsUserBackgroundColorVisible()
{
	return user_canvas_background_color;
}

void TemplateCanvas::SetSelectableOutlineColor(wxColour color)
{
	selectable_outline_color = color;
    
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
    
	UpdateSelectableOutlineColors();
}

void TemplateCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	UpdateSelectableOutlineColors();
    
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
    ReDraw();
}

void TemplateCanvas::SetHighlightColor(wxColour color)
{
	highlight_color = color;
	layer1_valid = false;
    ReDraw();
}

void TemplateCanvas::SetCanvasBackgroundColor(wxColour color)
{
	canvas_background_color = color;
	layer0_valid = false;
    ReDraw();
}

void TemplateCanvas::UpdateSelectableOutlineColors()
{
}

/**
 Impelmentation of HighlightStateObservable interface.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void TemplateCanvas::update(HLStateInt* o)
{
	if (layer2_bm) {
        ResetBrushing();
    
        if (draw_sel_shps_by_z_val) {
            // force a full redraw
            layer0_valid = false;
            return;
        }

        HLStateInt::EventType type = o->GetEventType();
        if (type == HLStateInt::transparency) {
            ResetFadedLayer();
        }
        // re-paint highlight layer (layer1_bm)
        layer1_valid = false;
        DrawLayers();
    
        UpdateStatusBar();
	}
}

void TemplateCanvas::RenderToDC(wxDC &dc, int w, int h)
{

	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	//dc.DrawRectangle(wxPoint(0,0), sz);
    
    resizeLayerBms(w, h);
    ResizeSelectableShps(w, h);
    
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}

    vector<bool>& hs = highlight_state->GetHighlight();
    helper_DrawSelectableShapes_dc(dc, hs, false, false, false, true);

	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}
    
	wxSize sz = GetClientSize();
    
    w = sz.GetWidth();
    h = sz.GetHeight();
    resizeLayerBms(w, h);
    ResizeSelectableShps(w, h);
    isResize = true;
}

void TemplateCanvas::DrawLayers()
{
	if (layer2_valid && layer1_valid && layer0_valid)
		return;
   
    if (!layer0_valid) {
        DrawLayer0();
    }

    if (!layer1_valid) {
        DrawLayer1();
    }
    
    if (!layer2_valid) {
        DrawLayer2();
    }
   
    //wxWakeUpIdle();
    
    Refresh(false);
}


// Draw all solid background, background decorations and unhighlighted
// shapes.
void TemplateCanvas::DrawLayer0()
{
    if (layer0_bm == NULL)
        return;

    wxSize sz = GetClientSize();
    wxMemoryDC dc(*layer0_bm);

    dc.SetPen(canvas_background_color);
    dc.SetBrush(canvas_background_color);
    dc.DrawRectangle(wxPoint(0,0), sz);

    BOOST_FOREACH( GdaShape* shp, background_shps ) {
        shp->paintSelf(dc);
    }
    
    DrawSelectableShapes(dc);


    dc.SelectObject(wxNullBitmap);
    layer0_valid = true;
    layer1_valid = false;
}


// draw highlighted shapes.
void TemplateCanvas::DrawLayer1()
{
    if (layer1_bm == NULL)
        return;
    wxMemoryDC dc(*layer1_bm);
    dc.Clear();
    wxSize sz = GetClientSize();
    dc.SetPen(canvas_background_color);
    dc.SetBrush(canvas_background_color);
    dc.DrawRectangle(wxPoint(0,0), sz);
    
    // faded the background half transparency
    if (highlight_state->GetTotalHighlighted()>0) {
        if (faded_layer_bm == NULL) {
            wxImage image = layer0_bm->ConvertToImage();
            if (enable_high_dpi_support) {
                image.Rescale(sz.GetWidth(), sz.GetHeight());
            }

            if (!image.HasAlpha()) {
                image.InitAlpha();
            }
            unsigned char *alpha=image.GetAlpha();
            memset(alpha, GdaConst::plot_transparency_unhighlighted,
                   image.GetWidth()*image.GetHeight());

            faded_layer_bm = new wxBitmap(image);
        }
        dc.DrawBitmap(*faded_layer_bm,0,0);
    } else {
        dc.DrawBitmap(*layer0_bm, 0, 0);
    }
    
    DrawHighlightedShapes(dc);
    
    dc.SelectObject(wxNullBitmap);
    layer1_valid = true;
    layer2_valid = false;
}

void TemplateCanvas::DrawLayer2()
{
    if (layer2_bm == NULL)
        return;
    wxMemoryDC dc(*layer2_bm);
    dc.Clear();
    dc.DrawBitmap(*layer1_bm, 0, 0);
    BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
        shp->paintSelf(dc);
    }

    dc.SelectObject(wxNullBitmap);
    layer2_valid = true;
}

void TemplateCanvas::OnPaint(wxPaintEvent& event)
{
    if (layer2_bm) {
        wxSize sz = GetClientSize();
        wxMemoryDC dc(*layer2_bm);
        
        wxPaintDC paint_dc(this);
        paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
        
        // Draw optional control objects if needed
        PaintControls(paint_dc);
        
        helper_PaintSelectionOutline(paint_dc);
        
        //wxBufferedPaintDC paint_dc(this, *layer2_bm);
        //PaintSelectionOutline(paint_dc);
    }
    event.Skip();
}

void TemplateCanvas::ReDraw()
{
    isResize = true;
}

void TemplateCanvas::OnIdle(wxIdleEvent& event)
{
    if (isResize) {
        isResize = false;
       
        
        int vs_w, vs_h;
        
        GetClientSize(&vs_w, &vs_h);
       
        vs_w = vs_w;
        vs_h = vs_h;
        
        double scale_factor = 1.0;
        
        last_scale_trans.SetView(vs_w, vs_h, scale_factor);
        
        resizeLayerBms(vs_w, vs_h);

        ResizeSelectableShps(vs_w, vs_h);
        
        event.RequestMore(); // render continuously, not only once on idle
    }
    
    if (!layer2_valid || !layer1_valid || !layer0_valid) {
        DrawLayers();
        event.RequestMore(); // render continuously, not only once on idle
    }
}

void TemplateCanvas::OnSize(wxSizeEvent& event)
{
    ResetBrushing();
    isResize = true;
    event.Skip();
}


bool TemplateCanvas::_IsShpValid(int idx)
{
    if (selectable_shps[idx] == NULL || selectable_shps[idx]->isNull())  {
        return false;
    }
    
    if (!selectable_shps_undefs.empty() && selectable_shps_undefs[idx] == true) {
        return false;
    }
    return true;
}


// draw unhighlighted selectable shapes
// using wxDC only since windows platform has poor wxGC support
void TemplateCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	if (selectable_shps.size() == 0)
        return;

	if (use_category_brushes) {
        DrawSelectableShapes_dc(dc);
        
	} else {
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
            if (_IsShpValid(i)) {
                selectable_shps[i]->paintSelf(dc);
            }
		}
	}
}

// draw highlighted selectable shapes
void TemplateCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	if (selectable_shps.size() == 0)
        return;
   
    if (use_category_brushes) {
        bool highlight_only = true;
        DrawSelectableShapes_dc(dc, highlight_only);
        
    } else {
        vector<bool>& hs = GetSelBitVec();
        for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
            if (hs[i] && _IsShpValid(i)) {
                selectable_shps[i]->paintSelf(dc);
            }
        }
    }
}

void TemplateCanvas::DrawSelectableShapes_dc(wxMemoryDC &_dc, bool hl_only,
                                             bool revert)
{
    vector<bool>& hs = highlight_state->GetHighlight();
#ifdef __WXOSX__
    wxGCDC dc(_dc);
    helper_DrawSelectableShapes_dc(dc, hs, hl_only, revert);
#else
	helper_DrawSelectableShapes_dc(_dc, hs, hl_only, revert);
#endif
}

void TemplateCanvas::helper_DrawSelectableShapes_dc(wxDC &dc,
                                                    vector<bool>& hs,
                                                    bool hl_only,
                                                    bool revert,
                                                    bool crosshatch,
                                                    bool is_print,
                                                    const wxColour& fixed_pen_color)
{
    
    //vector<bool>& hs = GetSelBitVec();
    
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats = cat_data.GetNumCategories(cc_ts);
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
    
    if (selectable_shps_type == points) {
		int bnd = w*h;
		vector<bool> dirty(bnd, false);

		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		wxDouble r = GdaConst::my_point_click_radius;
        if (w < 150 || h < 150) {
            r *= 0.66;
        }
        if (selectable_shps.size() > 100 && (w < 80 || h < 80)) {
            r = 0.2;
        }
        if (is_print)
            r = 15;
		GdaPoint* p;
		for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch ){
                dc.SetPen(wxPen(highlight_color));
            } else {
                wxColour clr = cat_data.GetCategoryColor(cc_ts, cat);
                dc.SetPen(wxPen(clr));
            }
            if (fixed_pen_color != *wxWHITE) {
                dc.SetPen(wxPen(fixed_pen_color));
            }
			vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]] == revert)) {
                    continue;
                }
				p = (GdaPoint*) selectable_shps[ids[i]];
                if (p->isNull()) {
                    continue;
                }
				int bnd_idx = p->center.x + p->center.y*w;
				if (is_print) {
					dc.DrawCircle(p->center.x, p->center.y, r);
				} else if (bnd_idx >= 0 && bnd_idx < bnd && !dirty[bnd_idx]) {
					dc.DrawCircle(p->center.x, p->center.y, r);
					dirty[bnd_idx] = true;
				}
			}
		}
        
	} else if (selectable_shps_type == polygons) {

		GdaPolygon* p;
		for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch) {
                dc.SetPen(wxPen(highlight_color));
                dc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
            } else {
                if (selectable_outline_visible) {
                    wxPen pen = cat_data.GetCategoryPen(cc_ts, cat);
                    dc.SetPen(pen);
                } else {
                    dc.SetPen(*wxTRANSPARENT_PEN);
                }
                if (fixed_pen_color != *wxWHITE) {
                    dc.SetPen(wxPen(fixed_pen_color));
                }
                dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
            }
			vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
            
			for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]] == revert))
                    continue;
				p = (GdaPolygon*) selectable_shps[ids[i]];
                if (p->isNull())
                    continue;
				if (p->all_points_same) {
					dc.DrawPoint(p->center.x, p->center.y);
				} else {
					if (p->n_count > 1) {
						dc.DrawPolyPolygon(p->n_count, p->count, p->points);
					} else {
						dc.DrawPolygon(p->n, p->points);
					}
				}
			}
		}
        
	} else if (selectable_shps_type == circles) {
		// Only Bubble Chart uses circles currently, but Bubble Chart uses
		// DrawSelectableShapesByZVal.  This will be useful for Cartogram map
		GdaCircle* c;
		for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch) {
                dc.SetPen(wxPen(highlight_color));
                dc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
            } else {
                if (selectable_outline_visible) {
                    wxPen pen = cat_data.GetCategoryPen(cc_ts, cat);
                    dc.SetPen(pen);
                } else {
                    dc.SetPen(*wxTRANSPARENT_PEN);
                }
                if (fixed_pen_color != *wxWHITE) {
                    dc.SetPen(wxPen(fixed_pen_color));
                }
                wxBrush brush =  cat_data.GetCategoryBrush(cc_ts, cat);
                dc.SetBrush(brush);
            }
			vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]] == revert))
                    continue;
				c = (GdaCircle*) selectable_shps[ids[i]];
				if (c->isNull())
                    continue;
				dc.DrawCircle(c->center.x, c->center.y, c->radius);
			}
		}
        
	} else if (selectable_shps_type == polylines) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		// only PCP uses PolyLines currently. So, we assume that there
		// is only one group of line segments connected together.
		// If we support Shapefile polyline map objects, then this will
		// have to change.
		GdaPolyLine* s = 0;
		for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch) {
                dc.SetPen(wxPen(highlight_color));
            } else {
                wxColour clr = cat_data.GetCategoryColor(cc_ts, cat);
                dc.SetPen(wxPen(clr));
            }
            if (fixed_pen_color != *wxWHITE) {
                dc.SetPen(wxPen(fixed_pen_color));
            }
			vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]] == revert)) {
                    continue;
                }
				s = (GdaPolyLine*) selectable_shps[ids[i]];
				if (s->isNull()) continue;
				for (int v=0; v<s->n-1; v++) {
					dc.DrawLine(s->points[v].x, s->points[v].y,
								s->points[v+1].x, s->points[v+1].y);
				}
			}
		}
	}
}

// draw unhighlighted selectable shapes with wxGraphicsContext
void TemplateCanvas::helper_DrawSelectableShapes_gc(wxGraphicsContext &gc,
                                                    vector<bool>& hs,
                                                    bool hl_only,
                                                    bool revert, 
													bool crosshatch,
													int alpha)
{
    //vector<bool>& hs = GetSelBitVec();
    
    gc.SetAntialiasMode(wxANTIALIAS_NONE);
    gc.SetInterpolationQuality( wxINTERPOLATION_NONE );
    
    int cc_ts = cat_data.curr_canvas_tm_step;
    int num_cats=cat_data.GetNumCategories(cc_ts);
    int w = layer0_bm->GetWidth();
    int h = layer0_bm->GetHeight();
    
    if (selectable_shps_type == points) {
		int bnd = w*h;
		vector<bool> dirty(bnd, false);
        
        wxDouble r = GdaConst::my_point_click_radius;
        if (w < 150 || h < 150) {
            r *= 0.66;
        }
        if (selectable_shps.size() > 100 && (w < 80 || h < 80)) {
            r = 0.2;
        }
        GdaPoint* p;
        for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch ){
                gc.SetPen(wxPen(highlight_color));
            } else {
                wxColour penClr = cat_data.GetCategoryColor(cc_ts, cat);
                char r = penClr.Red();
                char b = penClr.Blue();
                char g = penClr.Green();
                wxColour newClr(r, g, b, alpha);
                gc.SetPen(wxPen(newClr));
            }
            std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
            wxGraphicsPath path = gc.CreatePath();
            
            for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]]== revert)) {
                    continue;
                }
                p = (GdaPoint*) selectable_shps[ids[i]];
                if (p->isNull())
                    continue;
				int bnd_idx = p->center.x + p->center.y*w;
				if (bnd_idx >= 0 && bnd_idx < bnd && !dirty[bnd_idx]) {
                    path.AddCircle(p->center.x, p->center.y, r);
					dirty[bnd_idx] = true;
                }
            }
            gc.StrokePath(path);
        }
        
    } else if (selectable_shps_type == polygons) {
        GdaPolygon* p;
        for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch) {
                gc.SetPen(wxPen(highlight_color));
                gc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
            } else {
                if (selectable_outline_visible) {
                    gc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
                }
                wxBrush br = cat_data.GetCategoryBrush(cc_ts, cat);
                wxColour brushClr = br.GetColour();
                char r = brushClr.Red();
                char b = brushClr.Blue();
                char g = brushClr.Green();
                wxColour newClr(r, g, b, alpha);
                wxBrush newBrush(newClr);
                gc.SetBrush(newBrush);
            }
            
            vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
            
            for (int i=0, iend=ids.size(); i<iend; i++) {
                wxGraphicsPath path = gc.CreatePath();
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]]== revert) ) {
                    continue;
                }
                p = (GdaPolygon*) selectable_shps[ids[i]];
                if (p->isNull())
                    continue;
                if (p->all_points_same) {
                    path.AddCircle(p->center.x, p->center.y, 0.2);
                } else {
                    for (int c=0, s=0, t=p->count[0]; c<p->n_count; c++) {
                        path.MoveToPoint(p->points[s]);
                        for (int pt=s+1; pt<t && pt<p->n; pt++) {
                            path.AddLineToPoint(p->points[pt]);
                        }
                        path.CloseSubpath();
                        s = t;
                        if (c+1 < p->n_count) {
                            t += p->count[c+1];
                        }
                    }
                }
                gc.FillPath(path, wxWINDING_RULE);
                if (selectable_outline_visible)	{
                    gc.StrokePath(path);
                }
            }
        }
    } else if (selectable_shps_type == circles) {
        // Only Cartogram map uses this currently
        GdaCircle* c;
        for (int cat=0; cat<num_cats; cat++) {
            if (hl_only && crosshatch) {
                gc.SetPen(wxPen(highlight_color));
                gc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
            } else {
                wxColour penClr = cat_data.GetCategoryPen(cc_ts, cat).GetColour();
                wxColour brushClr = cat_data.GetCategoryBrush(cc_ts, cat).GetColour();
                wxColour newPenClr(penClr.Red(), penClr.Green(), penClr.Blue(), alpha);
                wxColour newBrushClr(brushClr.Red(), brushClr.Green(), brushClr.Blue(), alpha);
                gc.SetPen(wxPen(newPenClr));
                gc.SetBrush(wxBrush(newBrushClr));
            }
            
            std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
            if (selectable_outline_visible) {
                for (int i=0, iend=ids.size(); i<iend; i++) {
                    if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]]== revert) ) {
                        continue;
                    }
                    c = (GdaCircle*) selectable_shps[ids[i]];
                    wxGraphicsPath path = gc.CreatePath();
                    path.AddCircle(c->center.x, c->center.y, c->radius);
                    gc.FillPath(path, wxWINDING_RULE);
                    gc.StrokePath(path);
                }
            } else {
                // Note: in the case of circles, it is much slower
                // to batch render all of the circles together rather
                // than filling them one at a time.  This does not appear
                // to be true for polygons.
                for (int i=0, iend=ids.size(); i<iend; i++) {
                    if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]]== revert) ) {
                        continue;
                    }
                    c = (GdaCircle*) selectable_shps[ids[i]];
                    wxGraphicsPath path = gc.CreatePath();
                    path.AddCircle(c->center.x, c->center.y, c->radius);
                    gc.FillPath(path, wxWINDING_RULE);
                }
            }
        }
        
    } else if (selectable_shps_type == polylines) {
        // only PCP uses PolyLines currently. So, we assume that there
        // is only one group of line segments connected together.
        // If we support Shapefile polyline map objects, then this will
        // have to change.
        //gc->SetAntialiasMode(wxANTIALIAS_NONE);
        GdaPolyLine* s = 0;
        for (int cat=0; cat<num_cats; cat++) {
            wxColour penClr = cat_data.GetCategoryPen(cc_ts, cat).GetColour();
            wxColour newPenClr(penClr.Red(), penClr.Green(), penClr.Blue(),alpha);
            
            gc.SetPen(wxPen(newPenClr));
            
            std::vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
            wxGraphicsPath path = gc.CreatePath();
            for (int i=0, iend=ids.size(); i<iend; i++) {
                if (!_IsShpValid(ids[i]) || (hl_only && hs[ids[i]]== revert) ) {
                    continue;
                }
                s = (GdaPolyLine*) selectable_shps[ids[i]];
                
                path.MoveToPoint(s->points[0]);
                for (int v=0; v < s->n-1; v++) {
                    path.AddLineToPoint(s->points[v+1]);
                }
            }
            gc.StrokePath(path);
        }
    }
}

void TemplateCanvas::OnMouseEvent(wxMouseEvent& event)
{
    // Capture the mouse when left mouse button is down.
    if (event.LeftIsDown() && !HasCapture())
        CaptureMouse();
    
	if (event.LeftUp() && HasCapture())
        ReleaseMouse();
	
	if (mousemode == select) {
        is_showing_brush = true;
		if (selectstate == start) {
			if (event.LeftDown()) {
                prev = GetActualPos(event);
              
                if (sel1.x > 0 && sel1.y > 0 && sel2.x > 0 && sel2.y >0) {
                    // already has selection then
                    // detect if click inside brush_shape
                    GdaShape* brush_shape = NULL;
                    if (brushtype == rectangle) {
                        brush_shape = new GdaRectangle(sel1, sel2);
                    } else if (brushtype == circle) {
                        brush_shape = new GdaCircle(sel1, sel2);
                    } else if (brushtype == line) {
                        brush_shape = new GdaPolyLine(sel1, sel2);
                    }
                    if (brush_shape->Contains(prev)) {
                        // brushing
                        is_brushing = true;
                        remember_shiftdown = false;  // brush will cancel shift
                        selectstate = brushing;
                    } else {
                        // cancel brushing since click outside, restore leftdown
                        ResetBrushing();
                        sel1 = prev;
                        selectstate = leftdown;
                        //UpdateSelection();
                    }
                    delete brush_shape;
                    
                } else {
                    sel1 = prev;
                    selectstate = leftdown;
                }

			} else if (event.RightDown()) {
                ResetBrushing();
				DisplayRightClickMenu(event.GetPosition());

			} else {
                // hover
				if (template_frame && template_frame->IsStatusBarVisible()) {
					prev  = GetActualPos(event);
					DetermineMouseHoverObjects(prev);
					UpdateStatusBar();
				}
			}
            
		} else if (selectstate == leftdown) {
			if (event.Moving() || event.Dragging()) {
				wxPoint act_pos = GetActualPos(event);
				if (fabs((double) (sel1.x - act_pos.x)) +
					fabs((double) (sel1.y - act_pos.y)) > 2) {
					sel2 = GetActualPos(event);
					selectstate = dragging;
					remember_shiftdown = event.ShiftDown();
					UpdateSelection(remember_shiftdown);
					//UpdateStatusBar();
					//Refresh(false);
				}
			} else if (event.LeftUp()) {
				wxPoint act_pos = GetActualPos(event);
                if (act_pos == sel1 ) {
                    sel2 = sel1;
                }
				UpdateSelection(event.ShiftDown(), true);
				selectstate = start;
                ResetBrushing();
                
			} else if (event.RightDown()) {
				selectstate = start;
			}
            
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);

				UpdateSelection(remember_shiftdown);
				//UpdateStatusBar();
				//Refresh(false);
                
			} else if (event.LeftUp()) {
				sel2 = GetActualPos(event);

				UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
                //Refresh(false);
                
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}
            
		} else if (selectstate == brushing) {
			if (event.LeftUp()) {
                is_brushing = false; // will check again if brushing when mouse down again
				selectstate = start;
                
			} else if (event.RightDown()) {
                is_brushing = false;
				selectstate = start;
				Refresh();
                
			} else if (is_brushing && (event.Moving() || event.Dragging())) {
                wxPoint cur = GetActualPos(event);
				wxPoint diff = cur - prev;
				sel1 += diff;
				sel2 += diff;
				//UpdateStatusBar();

				UpdateSelection();
				//Refresh(false); // keep painting the select rect
                prev = cur;
			}
		}
		
	} else if (mousemode == zoom || mousemode == zoomout) {

		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
                is_showing_brush = true;
                
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
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
					Refresh(false);
				}
			} else if (event.LeftUp()) {
				if (event.ShiftDown() || event.CmdDown() || mousemode == zoomout) {
					// zoom out by a factor of two
                    is_showing_brush = false;
					sel2 = GetActualPos(event);
					int c_w, c_h;
					GetClientSize(&c_w, &c_h);
					if (c_w <=1) c_w = 8;
					if (c_h <=1) c_h = 8;
					sel1.x = sel2.x - (c_w/8);
					sel1.y = sel2.y - (c_h/8);
					sel2.x = sel2.x + (c_w/8);
					sel2.y = sel2.y + (c_h/8);
					ZoomShapes(false);
				}
				selectstate = start;
				Refresh(false);
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				Refresh(false);
			} else if (event.LeftUp() ) {
				sel2 = GetActualPos(event);
				remember_shiftdown = event.ShiftDown() || event.CmdDown() || mousemode == zoomout;
				ZoomShapes(!remember_shiftdown);
				remember_shiftdown = false;
				
                selectstate = start;
                ResetBrushing();
				Refresh(false);
                
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		}
        
	} else if (mousemode == pan) {
		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			} 
		} else if (selectstate == leftdown) {
			if (event.Moving() || event.Dragging()) {
				wxPoint act_pos = GetActualPos(event);
				if (fabs((double) (prev.x - act_pos.x)) +
					fabs((double) (prev.y - act_pos.y)) > 2) {
					sel1 = prev;
					sel2 = GetActualPos(event);
					selectstate = dragging;
				}
			} else if (event.LeftUp()) {
				selectstate = start;
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
			} else if (event.LeftUp() ) {
				sel2 = GetActualPos(event);

				remember_shiftdown = false;
				selectstate = start;
				PanShapes();
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		}
	}
}

void TemplateCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) 
        ReleaseMouse();
}

void TemplateCanvas::PaintSelectionOutline(wxMemoryDC& _dc)
{
#ifndef __WIN32__
    wxGCDC dc(_dc);
    helper_PaintSelectionOutline(dc);
#else
    helper_PaintSelectionOutline(_dc);
#endif
}

void TemplateCanvas::helper_PaintSelectionOutline(wxDC& dc)
{
	if (is_showing_brush && (mousemode == select || mousemode == zoom || mousemode == zoomout))
    {
        if (sel1 != sel2 ) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*wxBLACK_PEN);
		if (brushtype == rectangle) {
			dc.DrawRectangle(wxRect(sel1, sel2));
		} else if (brushtype == line) {
			dc.DrawLine(sel1, sel2);
		} else if (brushtype == circle) {
			dc.DrawCircle(sel1, GenUtils::distance(sel1, sel2));
		}
        }
	}
}

void TemplateCanvas::PaintControls(wxDC& dc)
{
}

// The following five methods enable the use of a custom
// HLStateInt object
// Returns bit vector of selection values according
// to selectable objects
vector<bool>& TemplateCanvas::GetSelBitVec()
{
	return highlight_state->GetHighlight();
}

// Returns number of newly selected objects
int TemplateCanvas::GetNumNewlySel()
{
	return highlight_state->GetTotalNewlyHighlighted();
}

// Sets number of newly selected objects
void TemplateCanvas::SetNumNewlySel(int n)
{
	highlight_state->SetTotalNewlyHighlighted(n);
}

// Returns list of newly selected objects.  Only indexes
// 0 through GetNumNewlySel()-1 are valid.
vector<int>& TemplateCanvas::GetNewlySelList()
{
	return highlight_state->GetNewlyHighlighted();
}

// Returns number of newly unselected objects
int TemplateCanvas::GetNumNewlyUnsel()
{
	return highlight_state->GetTotalNewlyUnhighlighted();
}

void TemplateCanvas::SetNumNewlyUnsel(int n)
{
	highlight_state->SetTotalNewlyUnhighlighted(n);
}

// Returns list of newly unselected objects.  Only indexes
// 0 through GetNumNewlyUnsel()-1 are valid.
vector<int>& TemplateCanvas::GetNewlyUnselList()
{
	return highlight_state->GetNewlyUnhighlighted();
}

wxPoint TemplateCanvas::GetActualPos(const wxMouseEvent& event)
{
	return wxPoint(event.GetX(), event.GetY());
}

void TemplateCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
    ResetBrushing();
}

void TemplateCanvas::AppendCustomCategories(wxMenu* menu,
											CatClassifManager* ccm)
{
	// search for ID_CAT_CLASSIF_A(B,C)_MENU submenus
	const int num_sub_menus=3;
	vector<int> menu_id(num_sub_menus);
	vector<int> sub_menu_id(num_sub_menus);
	vector<int> base_id(num_sub_menus);
	menu_id[0] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A");
	menu_id[1] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_B"); // conditional horizontal menu
	menu_id[2] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_C"); // conditional verticle menu
	sub_menu_id[0] = XRCID("ID_CAT_CLASSIF_A_MENU");
	sub_menu_id[1] = XRCID("ID_CAT_CLASSIF_B_MENU");
	sub_menu_id[2] = XRCID("ID_CAT_CLASSIF_C_MENU");
	base_id[0] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0;
	base_id[1] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0;
	base_id[2] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0;
	
	for (int i=0; i<num_sub_menus; i++) {
		wxMenuItem* smii = menu->FindItem(sub_menu_id[i]);
		if (!smii) continue;
		wxMenu* smi = smii->GetSubMenu();
		if (!smi) continue;
        int m_id = smi->FindItem("Custom Breaks");
        wxMenuItem* mi = smi->FindItem(m_id);
        if (!mi) continue;
       
        wxMenu* sm = mi->GetSubMenu();
        // clean
        wxMenuItemList items = sm->GetMenuItems();
        for (int i=0; i<items.size(); i++) {
            sm->Delete(items[i]);
        }
        
		sm->Append(menu_id[i], "Create New Custom", "Create new custom categories classification.");
		sm->AppendSeparator();
        
		vector<wxString> titles;
		ccm->GetTitles(titles);
		for (size_t j=0; j<titles.size(); j++) {
			wxMenuItem* mi = sm->Append(base_id[i]+j, titles[j]);
		}
        if (i==0) {
            // regular map menu
            GdaFrame::GetGdaFrame()->Bind(wxEVT_COMMAND_MENU_SELECTED, &GdaFrame::OnCustomCategoryClick, GdaFrame::GetGdaFrame(), GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0, GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0 + titles.size());
        } else if (i==1) {
            // conditional horizontal map menu
            GdaFrame::GetGdaFrame()->Bind(wxEVT_COMMAND_MENU_SELECTED, &GdaFrame::OnCustomCategoryClick_B, GdaFrame::GetGdaFrame(), GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0, GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0 + titles.size());
        } else if (i==2) {
            // conditional verticle map menu
            GdaFrame::GetGdaFrame()->Bind(wxEVT_COMMAND_MENU_SELECTED, &GdaFrame::OnCustomCategoryClick_C, GdaFrame::GetGdaFrame(), GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0, GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0 + titles.size());
        }
	}
}

void TemplateCanvas::UpdateSelectRegion(bool translate, wxPoint diff)
{
	
}

// This is a good candidate for parallelization in the future.  Could
// also use an r-tree to greatly reduce number of comparisons needed.
// For efficency sake, will make this default solution assume that
// selectable shapes and highlight state are in a one-to-one
// correspondence.  Special views such as histogram, or perhaps
// even map legends will have to override UpdateSelection and
// NotifyObservables.
void TemplateCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	if (selectable_shps_type == circles) {
		UpdateSelectionCircles(shiftdown, pointsel);
	} else if (selectable_shps_type == polylines) {
		UpdateSelectionPolylines(shiftdown, pointsel);
	} else {
		UpdateSelectionPoints(shiftdown, pointsel);
	}
    
    // re-paint highlight layer (layer1_bm)
    layer1_valid = false;
    DrawLayers();
    
    UpdateStatusBar();
}

// The following function assumes that the set of selectable objects
// being selected against are all points.  Since all GdaShape objects
// define a center point, this is also the default function for
// all GdaShape selectable objects.
void TemplateCanvas::UpdateSelectionPoints(bool shiftdown, bool pointsel)
{
	int hl_size = GetSelBitVec().size();
	if (hl_size != selectable_shps.size()) return;
    
	vector<bool>& hs = GetSelBitVec();
    bool selection_changed = false;
    
	if (pointsel) { // a point selection
		for (int i=0; i<hl_size; i++) {
            if ( !_IsShpValid(i))
                continue;
			if (selectable_shps[i]->pointWithin(sel1)) {
				if (hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				} else {
                    hs[i] = true;
                    selection_changed = true;
				}
			} else {
				if (!shiftdown && hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				}
			}			
		}
	} else { // determine which obs intersect the selection region.
		if (brushtype == rectangle) {
			wxRegion rect(wxRect(sel1, sel2));
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
				bool contains = (rect.Contains(selectable_shps[i]->center) !=
								 wxOutRegion);
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i]) {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
			
		} else if (brushtype == circle) {
			// using quad-tree to do pre-selection
			
			
			double radius = GenUtils::distance(sel1, sel2);
			// determine if each center is within radius of sel1
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i) )
                    continue;
				bool contains = (GenUtils::distance(sel1, selectable_shps[i]->center)
								 <= radius);
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i]) {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
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
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i) )
                    continue;
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
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i]) {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		}
	}
    if (selection_changed) {
        int total_highlighted = 0; // used for MapCanvas::Drawlayer1
        for (int i=0; i<hl_size; i++) if (hs[i]) total_highlighted += 1;
        highlight_state->SetTotalHighlighted(total_highlighted);
        highlight_timer->Start(50);
    }
}

// The following function assumes that the set of selectable objects
// being selected against are all GdaCircle objects.
void TemplateCanvas::UpdateSelectionCircles(bool shiftdown, bool pointsel)
{
	int hl_size = GetSelBitVec().size();
	if (hl_size != selectable_shps.size()) return;
    
	vector<bool>& hs = GetSelBitVec();
    bool selection_changed = false;
	
	if (pointsel) { // a point selection
		for (int i=0; i<hl_size; i++) {
            if ( !_IsShpValid(i))
                continue;
			GdaCircle* s = (GdaCircle*) selectable_shps[i];
			if (s->isNull()) continue;
			if (GenUtils::distance(s->center, sel1) <= s->radius) {
				if (hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				} else {
                    hs[i] = true;
                    selection_changed = true;
				}
			} else {
				if (!shiftdown && hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				}
			}			
		}
	} else {
		if (brushtype == rectangle) {
			wxRect rect(sel1, sel2);
			double rect_x = rect.GetPosition().x;
			double rect_y = rect.GetPosition().y;
			double half_rect_w = fabs((double) (sel1.x - sel2.x))/2.0;
			double half_rect_h = fabs((double) (sel1.y - sel2.y))/2.0;
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
                
				GdaCircle* s = (GdaCircle*) selectable_shps[i];
				if (s->isNull()) continue;
				double cdx = fabs((s->center.x - rect_x) - half_rect_w);
				double cdy = fabs((s->center.y - rect_y) - half_rect_h);
				bool contains = true;
				if (cdx > (half_rect_w + s->radius) ||
					cdy > (half_rect_h + s->radius)) {
					contains = false;
				} else if (cdx <= half_rect_w ||
						   cdy <= half_rect_h) {
					contains = true;
				} else {
					double t1 = cdx - half_rect_w;
					double t2 = cdy - half_rect_h;
					double corner_dist_sq = t1*t1 + t2*t2;
					contains = corner_dist_sq <= (s->radius)*(s->radius); 
				}
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i])  {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i]) {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		} else if (brushtype == circle) {
			double radius = GenUtils::distance(sel1, sel2);
			// determine if circles overlap
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
				GdaCircle* s = (GdaCircle*) selectable_shps[i];
				if (s->isNull()) continue;
				bool contains = (radius + s->radius >=
								 GenUtils::distance(sel1, s->center));
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i])  {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		} else if (brushtype == line) {
			wxRealPoint hp((sel1.x+sel2.x)/2.0, (sel1.y+sel2.y)/2.0);
			double hp_rad = GenUtils::distance(sel1, sel2)/2.0;
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
				GdaCircle* s = (GdaCircle*) selectable_shps[i];
				if (s->isNull()) continue;
				bool contains = ((GenUtils::pointToLineDist(s->center,
														   sel1, sel2) <=
								 s->radius) &&
								 (GenUtils::distance(hp, s->center) <=
								  hp_rad + s->radius));
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i])  {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		}
	}
    if (selection_changed) {
        int total_highlighted = 1; // used for MapCanvas::Drawlayer1
        highlight_state->SetTotalHighlighted(total_highlighted);
        highlight_timer->Start(50);
    }
}

// The following function assumes that the set of selectable objects
// being selected against are all GdaPolyLine objects.
void TemplateCanvas::UpdateSelectionPolylines(bool shiftdown, bool pointsel)
{
	int hl_size = GetSelBitVec().size();
	if (hl_size != selectable_shps.size()) return;
    
	vector<bool>& hs = GetSelBitVec();
    bool selection_changed = false;
	
	GdaPolyLine* p;
	if (pointsel) { // a point selection
		double radius = 3.0;
		wxRealPoint hp;
		double hp_rad;
		for (int i=0; i<hl_size; i++) {
            if ( !_IsShpValid(i))
                continue;
			p = (GdaPolyLine*) selectable_shps[i];
			if (p->isNull()) continue;
			bool contains = false;
			for (int j=0, its=p->n-1; j<its; j++) {
				hp.x = (p->points[j].x + p->points[j+1].x)/2.0;
				hp.y = (p->points[j].y + p->points[j+1].y)/2.0;
				hp_rad = GenUtils::distance(p->points[j],
											p->points[j+1])/2.0;
				
				if ((GenUtils::pointToLineDist(sel1,
											   p->points[j],
											   p->points[j+1]) <=
					 radius) &&
					(GenUtils::distance(hp, sel1) <= hp_rad + radius))
				{
					contains = true;
					break;
				}
			}
			if (contains) {
				if (hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				} else {
                    hs[i] = true;
                    selection_changed = true;
				}
			} else {
				if (!shiftdown && hs[i]) {
                    hs[i] = false;
                    selection_changed = true;
				}
			}
		}
	} else { // determine which obs intersect the selection region.
		if (brushtype == rectangle) {
			wxPoint lleft; // lower left corner of rect
			wxPoint uright; // upper right corner of rect
			wxPoint uleft; // upper right corner
			wxPoint lright;  // lower right corner
			GenGeomAlgs::StandardizeRect(sel1, sel2, lleft, uright);
			uleft.x = lleft.x;
			uleft.y = uright.y;
			lright.x = uright.x;
			lright.y = lleft.y;
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
				p = (GdaPolyLine*) selectable_shps[i];
				if (p->isNull()) continue;
				bool contains = false;
				for (int j=0, its=p->n-1; j<its; j++) {
                    wxPoint& pt = p->points[j];
                    wxPoint& next_pt = p->points[j+1];
					if (GenGeomAlgs::LineSegsIntersect(pt, next_pt,lleft, uleft) ||
						GenGeomAlgs::LineSegsIntersect(pt, next_pt, uleft, uright) ||
						GenGeomAlgs::LineSegsIntersect(pt, next_pt, uright, lright) ||
						GenGeomAlgs::LineSegsIntersect(pt, next_pt, lright, lleft))
					{
						contains = true;
						break;
					}
				}
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i])  {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i]) {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		} else if (brushtype == line) {
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
                
				p = (GdaPolyLine*) selectable_shps[i];
				if (p->isNull()) continue;
				bool contains = false;
				for (int j=0, its=p->n-1; j<its; j++) {
                    wxPoint& pt = p->points[j];
                    wxPoint& next_pt = p->points[j+1];
					if (GenGeomAlgs::LineSegsIntersect(pt, next_pt, sel1, sel2))
					{
						contains = true;
						break;
					}
				}
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i]) {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i])  {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}	
		} else if (brushtype == circle) {
			double radius = GenUtils::distance(sel1, sel2);
			wxRealPoint hp;
			double hp_rad;
			for (int i=0; i<hl_size; i++) {
                if ( !_IsShpValid(i))
                    continue;
                
				p = (GdaPolyLine*) selectable_shps[i];
				if (p->isNull()) continue;
				bool contains = false;
				for (int j=0, its=p->n-1; j<its; j++) {
                    wxPoint& pt = p->points[j];
                    wxPoint& next_pt = p->points[j+1];
					hp.x = (pt.x + next_pt.x)/2.0;
					hp.y = (pt.y + next_pt.y)/2.0;
					hp_rad = GenUtils::distance(pt, next_pt)/2.0;
					
					if ((GenUtils::pointToLineDist(sel1, pt, next_pt) <= radius) &&
						(GenUtils::distance(hp, sel1) <= hp_rad + radius))
					{
						contains = true;
						break;
					}
				}
				if (!shiftdown) {
					if (contains) {
                        if (!hs[i])  {
                            hs[i] = true;
                            selection_changed = true;
                        }
					} else {
                        if (hs[i])  {
                            hs[i] = false;
                            selection_changed = true;
                        }
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
                        hs[i] = true;
                        selection_changed = true;
					}
				}
			}
		}
	}
    if (selection_changed) {
        int total_highlighted = 1; // used for MapCanvas::Drawlayer1
        highlight_state->SetTotalHighlighted(total_highlighted);
        highlight_timer->Start(50);
    }
}

void TemplateCanvas::SelectAllInCategory(int category,
										 bool add_to_selection)
{
	int cc_ts = cat_data.curr_canvas_tm_step;
	if (category < 0 && category >= cat_data.GetNumCategories(cc_ts)) {
		return;
	}	
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
    
	vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	vector<bool> obs_in_cat(hl_size, false);
	vector<int>& ids = cat_data.GetIdsRef(cc_ts, category);

	for (int i=0, iend=ids.size(); i<iend; i++) obs_in_cat[ids[i]] = true;
	
	for (int i=0; i<hl_size; i++) {
		if (!add_to_selection && hs[i] && !obs_in_cat[i]) {
            hs[i] = false;
            selection_changed = true;
		}
		if (!hs[i] && obs_in_cat[i]) {
            hs[i] = true;
            selection_changed = true;
		}
	}
	
	if ( selection_changed ) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}


/** In this default implementation of NotifyObservables, we assume
 that the selectable_shps are in one-to-one correspondence
 with the shps in the highlight_state observable vector.  If this is
 not true, then NotifyObservables() needs to be redefined in the
 child class.  This method looks at the vectors of newly highlighted
 and unhighlighted observations as set by the calling UpdateSelection
 method, and determines the best notification to broadcast to all
 other HighlightStateObserver instances.
 */
void TemplateCanvas::NotifyObservables()
{
	// Goal: assuming that all views have the ability to draw
	// deltas, try to determine set of operations that will minimize
	// number of shapes to draw/erease.  Remember that all classes
	// have the ability to erase all shapes for free.  But, we will also
	// assume that when a delta update is given, that the class can
	// also determine how best to do the update.
	
	int total_newly_selected = highlight_state->GetTotalNewlyHighlighted();
	int total_newly_unselected = highlight_state->GetTotalNewlyUnhighlighted();
	
	if (total_newly_selected == 0 &&
		total_newly_unselected == highlight_state->GetTotalHighlighted()) {
		highlight_state->SetEventType(HLStateInt::unhighlight_all);
		highlight_state->notifyObservers(this);
	} else {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		highlight_state->notifyObservers(this);
	}
}

void TemplateCanvas::DetermineMouseHoverObjects(wxPoint pt)
{
	total_hover_obs = 0;
    hover_obs.clear();
	int total_obs = selectable_shps.size();
	if (selectable_shps_type == circles) {
		// slightly faster than GdaCircle::pointWithin
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
            if ( !_IsShpValid(i))
                continue;
			GdaCircle* s = (GdaCircle*) selectable_shps[i];
			if (s==NULL || s->isNull()) continue;
			if (GenUtils::distance_sqrd(s->center, pt) <=
				s->radius*s->radius) {
                hover_obs.push_back(i);
                total_hover_obs++;
			}			
		}
	} else if (selectable_shps_type == polygons ||
			   selectable_shps_type == polylines ||
               selectable_shps_type == rectangles)
	{
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
            if ( !_IsShpValid(i))
                continue;
			if (selectable_shps[i]->pointWithin(pt)) {
                hover_obs.push_back(i);
                total_hover_obs++;
			}
		}
	} else { // selectable_shps_type == points or anything without pointWithin
		const double r2 = GdaConst::my_point_click_radius;
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
            if ( !_IsShpValid(i))
                continue;
			if (GenUtils::distance_sqrd(selectable_shps[i]->center, pt)
				<= 16.5) {
                hover_obs.push_back(i);
                total_hover_obs++;
			}
		}
	}
}

void TemplateCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = 0;
	if (template_frame) sb = template_frame->GetStatusBar();
	if (!sb) return;
	wxString s;
	if (mousemode == select && selectstate == start) {
		s << "mouse position = (" << sel1.x << "," << sel1.y << ")";
		if (total_hover_obs >= 1) {
			s << ", sel obs id: " << hover_obs[0]+1;
		}
	}
	sb->SetStatusText(s);
}

wxString TemplateCanvas::GetCanvasTitle()
{
	return wxEmptyString;
}

/** Global title or time step has changed.  Update canvas and title
 as necessary */
void TemplateCanvas::TimeChange()
{
}

void TemplateCanvas::CreateZValArrays(int num_canvas_tms, int num_obs)
{
	z_val_order.resize(num_canvas_tms);
	for (int t=0; t<num_canvas_tms; t++) {
		if (z_val_order[t].shape()[0] != num_obs) {
			z_val_order[t].resize(boost::extents[num_obs][2]);
		}
	}
}

wxString TemplateCanvas::GetCategoriesTitle()
{
	return GetCanvasTitle();
}

/** Mark each observation according to its
 category with 1, 2, ...,#categories. */
void TemplateCanvas::SaveCategories(const wxString& title,
									const wxString& label,
									const wxString& field_default,
                                    vector<bool>& undefs)
{
	if (project->GetNumRecords() != selectable_shps.size()) return;
	vector<SaveToTableEntry> data(1);
	
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats = cat_data.GetNumCategories(cc_ts);
	vector<wxInt64> dt(selectable_shps.size());
	
	data[0].type = GdaConst::long64_type;
	data[0].l_val = &dt;
	data[0].label = label;
	data[0].field_default = field_default;
    data[0].undefined = &undefs;
	
	for (int cat=0; cat<num_cats; cat++) {
		vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
        for (int i=0, iend=ids.size(); i<iend; i++) {
            dt[ids[i]] = cat+1;
        }
	}
	
	SaveToTableDlg dlg(project, this, data,
                       title, wxDefaultPosition, wxSize(500,400));
	dlg.ShowModal();
}


void TemplateCanvas::GetVizInfo(map<wxString, vector<int> >& colors)
{
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	
	for (int cat=0; cat<num_cats; cat++) {
		wxColour brushClr = cat_data.GetCategoryBrush(cc_ts, cat).GetColour();		
		wxString clr = GdaColorUtils::ToHexColorStr(brushClr);
			
		vector<int> ids_copy;
		colors[clr] = ids_copy;
		
		vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
		
		for (int i=0, iend=ids.size(); i<iend; i++) {
			colors[clr].push_back(ids[i]);
		}
	}	
}

void TemplateCanvas::GetVizInfo(wxString& shape_type,
                                vector<wxString>& clrs,
                                vector<double>& bins)
{
    
	if (selectable_shps_type == points) {
        shape_type = "POINT";
	} else if (selectable_shps_type == polygons) {
        shape_type = "POLYGON";
	}
   
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	
    if (num_cats >1) {
	for (int cat=0; cat<num_cats; cat++) {
		wxColour brushClr = cat_data.GetCategoryBrush(cc_ts, cat).GetColour();		
		wxString clr = GdaColorUtils::ToHexColorStr(brushClr);
			
        clrs.push_back(clr);
        bins.push_back(cat_data.GetCategoryMax(cc_ts, cat));
    }
    }
}
