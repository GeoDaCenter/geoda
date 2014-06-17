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


#include "Generic/macro_cleaner.h"
#include <limits>
#include <math.h>
#include <map>
#include <stdlib.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/clipbrd.h>
#include <wx/splitter.h>
#include <wx/overlay.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/bimap.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
//#include <boost/geometry/index/rtree.hpp>

#include "DialogTools/SaveToTableDlg.h"
#include "Explore/CatClassifManager.h"
#include "Generic/GdaShape.h"
#include "ShapeOperations/ShpFile.h"
#include "GeoDa.h"
#include "Project.h"
#include "GdaConst.h"
#include "GenUtils.h"
#include "logger.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(boost::geometry::cs::cartesian)

IMPLEMENT_CLASS(TemplateCanvas, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateCanvas, wxScrolledWindow)
	EVT_SIZE(TemplateCanvas::OnSize)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
	EVT_KEY_DOWN(TemplateCanvas::OnKeyEvent)
	//EVT_SCROLL_CHANGED(TemplateCanvas::OnScrollChanged)
	EVT_SCROLLWIN(TemplateCanvas::OnScrollChanged)
END_EVENT_TABLE()

TemplateCanvas::TemplateCanvas(wxWindow *parent, const wxPoint& pos,
							   const wxSize& size,
							   bool fixed_aspect_ratio_mode_s,
							   bool fit_to_window_mode_s)
: wxScrolledWindow(parent, -1, pos, size,
				   wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE),
	mousemode(select), selectstate(start), brushtype(rectangle),
	scrollbarmode(none),
	fixed_aspect_ratio_mode(fixed_aspect_ratio_mode_s),
	fit_to_window_mode(fit_to_window_mode_s),
	remember_shiftdown(false),
	highlight_state(GdaFrame::GetProject()->GetHighlightState()),
	num_obs(GdaFrame::GetProject()->GetNumRecords()),
	template_frame(0),
	fixed_aspect_ratio_val(1.0),
	current_shps_width(0.0), current_shps_height(0.0),
	virtual_screen_marg_left(GdaConst::default_virtual_screen_marg_left),
	virtual_screen_marg_right(GdaConst::default_virtual_screen_marg_right),
	virtual_screen_marg_top(GdaConst::default_virtual_screen_marg_top),
	virtual_screen_marg_bottom(GdaConst::default_virtual_screen_marg_bottom),
	shps_orig_xmin(0), shps_orig_ymin(0),
	shps_orig_xmax(0), shps_orig_ymax(0),
	selectable_outline_visible(true),
	selectable_outline_color(GdaConst::selectable_outline_color),
	selectable_fill_color(GdaConst::selectable_fill_color),
	highlight_color(GdaConst::highlight_color),
	canvas_background_color(GdaConst::canvas_background_color),
	selectable_shps_type(mixed), use_category_brushes(false),
	draw_sel_shps_by_z_val(false),
	layer0_bm(0), layer1_bm(0), layer2_bm(0),
	layer0_valid(false), layer1_valid(false), layer2_valid(false),
	total_hover_obs(0), max_hover_obs(11), hover_obs(11),
	is_pan_zoom(false), is_scrolled(false), prev_scroll_pos_x(0),
	prev_scroll_pos_y(0)
{
	cat_data.CreateEmptyCategories(1, num_obs); // default is one time slice
	SetMouseMode(mousemode); // will set the correct cursor for current mode
	SetBackgroundStyle(wxBG_STYLE_ERASE);
	LOG_MSG("Entering TemplateCanvas::TemplateCanvas");
	LOG_MSG("Exiting TemplateCanvas::TemplateCanvas");
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
	LOG_MSG("Entering TemplateCanvas::~TemplateCanvas()");
	BOOST_FOREACH( GdaShape* shp, background_shps ) delete shp;
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) delete shp;
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) delete shp;
	if (HasCapture()) ReleaseMouse();
	deleteLayerBms();
	/*
	if (qtree != NULL) {
		delete qtree;
		qtree = NULL;
	}
	*/
	LOG_MSG("Exiting TemplateCanvas::~TemplateCanvas()");
}

void TemplateCanvas::deleteLayerBms()
{
	if (layer0_bm) delete layer0_bm; layer0_bm = 0;
	if (layer1_bm) delete layer1_bm; layer1_bm = 0;
	if (layer2_bm) delete layer2_bm; layer2_bm = 0;
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

void TemplateCanvas::resizeLayerBms(int width, int height)
{
	deleteLayerBms();
	layer0_bm = new wxBitmap(width, height);
	layer1_bm = new wxBitmap(width, height);
	layer2_bm = new wxBitmap(width, height);
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
}

void TemplateCanvas::invalidateBms()
{
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;	
}

bool TemplateCanvas::GetFixedAspectRatioMode()
{
	return fixed_aspect_ratio_mode;
}

void TemplateCanvas::SetFixedAspectRatioMode(bool mode)
{
	wxString msg("TemplateCanvas::SetFixedAspectRatioMode(");
	msg << mode << ")";
	LOG_MSG(msg);
	fixed_aspect_ratio_mode = mode;
	if (fixed_aspect_ratio_mode) {
		if (current_shps_width < current_shps_height) {
			current_shps_height = current_shps_width / fixed_aspect_ratio_val;
		} else {
			current_shps_width = current_shps_height * fixed_aspect_ratio_val;
		}
	}
	ResizeSelectableShps();
	Refresh();
}


bool TemplateCanvas::GetFitToWindowMode()
{
	return fit_to_window_mode;
}

void TemplateCanvas::SetFitToWindowMode(bool mode)
{
	wxString msg("TemplateCanvas::SetFitToWindowMode(");
	msg << mode << ")";
	LOG_MSG(msg);
	fit_to_window_mode = mode;
	scrollbarmode = none;
	if (fit_to_window_mode) {
		is_pan_zoom = false;
		is_scrolled = false;
		prev_scroll_pos_x = 0;
		prev_scroll_pos_y = 0;
		
        ResetShapes();
        /*
		int cs_w=0, cs_h=0;
		GetClientSize(&cs_w, &cs_h);
		int vs_w, vs_h;
		GetVirtualSize(&vs_w, &vs_h);
		// SetVirtualSize will automatically generate an OnSize event when
		// the current virtual screen size does not equal the current
		// client screen size.  So, if an OnSize event will not be generated,
		// then we should manually call ResizeSelectableShps
		if (vs_w == cs_w && vs_h == cs_h ) {
			// this call will automatically generate an OnSize event when
			// the current virtual screen size does not equal the current
			// client screen size
			ResizeSelectableShps(cs_w, cs_h);
			Refresh();
		} else {
			SetVirtualSize(cs_w, cs_h);
		}*/
	}
}

void TemplateCanvas::OnKeyEvent(wxKeyEvent& event)
{
	LOG_MSG("In TemplateCanvas::OnKeyEvent");
	event.Skip();
}

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
	event.Skip();
	if (is_pan_zoom) {
		int shp_h = ext_shps_orig_ymax - ext_shps_orig_ymin;
		int shp_w = ext_shps_orig_xmax - ext_shps_orig_xmin;
		int vs_w = 0, vs_h = 0;
		GetVirtualSize(&vs_w, &vs_h);
		if (orient == wxHORIZONTAL) {
			int v_offset = pos - prev_scroll_pos_x ;
			double offset = v_offset / (double)vs_w * shp_w;
			current_map_x_min += offset;
			current_map_x_max += offset;
			prev_scroll_pos_x = pos;
		} else if (orient == wxVERTICAL) {
			int v_offset = -pos + prev_scroll_pos_y ;
			double offset = v_offset / (double)vs_h * shp_h;
			current_map_y_min += offset;
			current_map_y_max += offset;
			prev_scroll_pos_y = pos;
		}
		is_scrolled = true;
		ResizeSelectableShps();
	}
	Refresh();
}

wxString TemplateCanvas::GetCanvasStateString()
{
	wxString str("TemplateCanvas state data:\n");
	str << "  fit_to_window_mode = " << fit_to_window_mode;
	str << "\n  fixed_aspect_ratio_mode = " << fixed_aspect_ratio_mode;
	str << "\n  fixed_aspect_ratio_val = " << fixed_aspect_ratio_val;
	str << "\n  current_shps_width = " << current_shps_width;
	str << "\n  current_shps_height = " << current_shps_height;
	str << "\n  GetClientSize().GetWidth() = "
									<< GetClientSize().GetWidth();
	str << "\n  GetClientSize().GetHeight() = "
									<< GetClientSize().GetHeight();
	str << "\n  GetVirtualSize().GetWidth() = "
									<< GetVirtualSize().GetWidth();
	str << "\n  GetVirtualSize().GetHeight() = "
									<< GetVirtualSize().GetHeight();
	str << "\n";
	//str << "\n   = " << ;
	return str;
}

//bool fixed_aspect_ratio_mode;
//bool fit_to_window_mode;
//int virtual_screen_marg_left;  // virtual screen fixed margins
//int virtual_screen_marg_right;
//int virtual_screen_marg_top;
//int virtual_screen_marg_bottom;
//double current_shps_width;  // these will be set by the zoom in/out code
//double current_shps_height;
// the following four parameters should usually be obtained from
// the shp file bounding box info in the header file.  They are used
// to calculate the affine transformation when the window is resized.
//double shps_orig_xmin;
//double shps_orig_ymin;
//double shps_orig_xmax;
//double shps_orig_ymax;
//
// We assume that the above parameters are all set correctly.
//
// virtual_scrn_w and virtual_scrn_h are optional parameters.  When
// they are > 0, they are used, otherwise we call GetVirtualSize
// to get the current virtual screen size.
//
void TemplateCanvas::ResizeSelectableShps(int virtual_scrn_w,
										  int virtual_scrn_h)
{
	// NOTE: we do not support both fixed_aspect_ratio_mode
	//    and fit_to_window_mode being false currently.
	//LOG_MSG("Entering TemplateCanvas::ResizeSelectableShps");
	wxStopWatch sw;
	int vs_w=virtual_scrn_w, vs_h=virtual_scrn_h;

	double image_width, image_height;
	bool ftwm = true;//GetFitToWindowMode();
	if (ftwm == false) {
		if (vs_w <= 0 && vs_h <= 0) {
			GetVirtualSize(&vs_w, &vs_h);
		}
	} else {
		if (vs_w <= 0 && vs_h <= 0) {
			GetClientSize(&vs_w, &vs_h);
		}
	}
	double resize_xmin, resize_ymin, resize_xmax, resize_ymax;
	if (is_pan_zoom ) {
		resize_xmin = current_map_x_min;
		resize_ymin = current_map_y_min;
		resize_xmax = current_map_x_max;
		resize_ymax = current_map_y_max;
	} else {
		resize_xmin = shps_orig_xmin;
		resize_ymin = shps_orig_ymin;
		resize_xmax = shps_orig_xmax;
		resize_ymax = shps_orig_ymax;
	}
	GdaScaleTrans::calcAffineParams(resize_xmin, resize_ymin,
								   resize_xmax, resize_ymax,
								   virtual_screen_marg_top,
								   virtual_screen_marg_bottom,
								   virtual_screen_marg_left,
								   virtual_screen_marg_right,
								   vs_w, vs_h, fixed_aspect_ratio_mode,
								   ftwm,
								   &last_scale_trans.scale_x,
								   &last_scale_trans.scale_y,
								   &last_scale_trans.trans_x,
								   &last_scale_trans.trans_y,
								   ftwm ? 0 : current_shps_width,
								   ftwm ? 0 : current_shps_height,
								   &image_width, &image_height);
	
	last_scale_trans.max_scale =
		GenUtils::max<double>(last_scale_trans.scale_x,
							  last_scale_trans.scale_y);
	/*
	//LOG_MSG(last_scale_trans.GetString());
	//wxString msg;
	//msg << "    image_width = " << image_width;
	//msg << ", image_height = " << image_height;
	//msg << "\n    current_shps_width = " << current_shps_width;
	//msg << ", current_shps_height = " << current_shps_height;
	//LOG_MSG(msg);
	*/
	if (current_shps_width > 0 && current_shps_height > 0) {
		BOOST_FOREACH( GdaShape* ms, background_shps ) {
			ms->applyScaleTrans(last_scale_trans);
		}
		BOOST_FOREACH( GdaShape* ms, selectable_shps ) {
			ms->applyScaleTrans(last_scale_trans);
		}
	}
	LOG_MSG(wxString::Format("ResizeSelectableShps scale shapes: %ld ms",
							 sw.Time()));
	/*
	//if (selectable_shps_type == polygons) {
		//int proj_to_pnt_cnt = 0;
		//for (int i=0; i<num_obs; i++) {
		//  if (selectable_shps[i]->isNull()) continue;
		//	if (((GdaPolygon*) selectable_shps[i])->all_points_same) {
		//		proj_to_pnt_cnt++;
		//	}
		//}
		//double perc = proj_to_pnt_cnt*100;
		//perc /= (double) num_obs;
		//wxString s;
		//s << "ResizeSelectableShps: " << proj_to_pnt_cnt << "/" << num_obs;
		//s << ", " << perc << "% project to single point";
		//LOG_MSG(s);
		// MMM: should keep track of this percentage and use it as
		// a cuttoff to decide if it is worth it to keep track
		// of dirty/non-dirty in during rendering.
	//}
	 */
	BOOST_FOREACH( GdaShape* ms, foreground_shps ) {
		ms->applyScaleTrans(last_scale_trans);
	}
	layer0_valid = false;
	if ( resize_xmax == shps_orig_xmax && resize_ymin == shps_orig_ymin){
		wxRealPoint map_topleft, map_bottomright;
		last_scale_trans.transform_back(wxPoint(0,0), map_topleft);
		last_scale_trans.transform_back(wxPoint(vs_w-1,vs_h-1), map_bottomright);
		ext_shps_orig_xmin = map_topleft.x;
		ext_shps_orig_xmax = map_bottomright.x;
		ext_shps_orig_ymin = map_bottomright.y;
		ext_shps_orig_ymax = map_topleft.y;
	}
	//Scrollbars setting
	if (is_pan_zoom && !is_scrolled) {
		wxRealPoint map_topleft, map_bottomright;
		last_scale_trans.transform_back(wxPoint(0,0), map_topleft);
		last_scale_trans.transform_back(wxPoint(vs_w-1,vs_h-1), map_bottomright);
		double current_w = map_bottomright.x - map_topleft.x;
		double current_h = -map_bottomright.y + map_topleft.y;
		double shp_h = ext_shps_orig_ymax - ext_shps_orig_ymin;
		double shp_w = ext_shps_orig_xmax - ext_shps_orig_xmin;
		int whole_vs_h =(int)(vs_h * shp_h / current_h); 
		int whole_vs_w =(int)(vs_w * shp_w / current_w); 
		int offset_vs_x = (int)(whole_vs_w * (map_topleft.x-ext_shps_orig_xmin) / shp_w);
		int offset_vs_y = (int)(whole_vs_h * (ext_shps_orig_ymax-map_topleft.y) / shp_h);

		SetVirtualSize(whole_vs_w, whole_vs_h);
		SetScrollRate(1,1);
		prev_scroll_pos_x =  offset_vs_x > 0 ? offset_vs_x:0;
		prev_scroll_pos_y =  offset_vs_y > 0 ? offset_vs_y:0;
		SetScrollPos(wxHORIZONTAL, prev_scroll_pos_x);
		SetScrollPos(wxVERTICAL, prev_scroll_pos_y);
	}
	is_scrolled = false;
	Refresh();
	LOG_MSG(wxString::Format("ResizeSelectableShps run time: %ld ms",sw.Time()));
	//LOG_MSG("Exiting TemplateCanvas::ResizeSelectableShps");
}

void TemplateCanvas::ResetShapes()
{
	current_map_x_min = shps_orig_xmin;
	current_map_y_min = shps_orig_ymin;
	current_map_x_max = shps_orig_xmax;
	current_map_y_max = shps_orig_ymax;
	is_pan_zoom = false;

	int vs_w=0, vs_h=0;
	GetClientSize(&vs_w, &vs_h);
	SetVirtualSize(vs_w, vs_h);
	//SetScrollbars(1, 1, vs_w, vs_h, 0, 0, true);
	
	SetMouseMode(select);
	ResizeSelectableShps();
	Refresh();
}

void TemplateCanvas::ZoomShapes(bool is_zoomin)
{
	if (sel2.x == 0 && sel2.y==0) return;
	if (sel1.x == sel2.x) sel2.x = sel1.x + 2;
	if (sel1.y == sel2.y) sel2.y = sel1.y + 2;
	SetFitToWindowMode(false);
	// get current selected extent/view in map coordinates
	wxRealPoint map_sel1, map_sel2;
	last_scale_trans.transform_back(sel1, map_sel1);
	last_scale_trans.transform_back(sel2, map_sel2);
	
	// topLeft, bottomRight
	double resize_xmin, resize_ymin, resize_xmax, resize_ymax;
	if (!is_pan_zoom ) {
		current_map_x_min = shps_orig_xmin;
		current_map_y_min = shps_orig_ymin;
		current_map_x_max = shps_orig_xmax;
		current_map_y_max = shps_orig_ymax;
	}

	if (!is_zoomin) {
		double current_map_w = current_map_x_max - current_map_x_min;
		double current_map_h = current_map_y_max - current_map_y_min;
		double w_ratio = current_map_w / fabs( map_sel1.x - map_sel2.x);
		double h_ratio = current_map_h / fabs( map_sel1.y - map_sel2.y);
		double ratio = w_ratio > h_ratio ? h_ratio : w_ratio;
		
		double x_expand = current_map_w * (ratio - 1) / 2.0;
		double y_expand = current_map_h * (ratio - 1) / 2.0;
		resize_xmin = current_map_x_min - x_expand;
		resize_xmax = current_map_x_max + x_expand;
		resize_ymin = current_map_y_min - y_expand;
		resize_ymax = current_map_y_max + y_expand;
	} else {
		resize_xmin = std::min( map_sel1.x, map_sel2.x);
		resize_xmax = std::max( map_sel1.x, map_sel2.x);
		resize_ymin = std::min( map_sel1.y, map_sel2.y);
		resize_ymax = std::max( map_sel1.y, map_sel2.y);
	}

	is_pan_zoom = true;
	current_map_x_min = resize_xmin;
	current_map_y_min = resize_ymin;
	current_map_x_max = resize_xmax;
	current_map_y_max = resize_ymax;

	ResizeSelectableShps();
}

void TemplateCanvas::PanShapes()
{
	if (sel2.x == 0 && sel2.y==0) return;
    SetFitToWindowMode(false);
	// update map boundary of current view, here we can only update
	// trans_x and trans_y; (scale_x and scale_y wont change since it's only pan
	wxRealPoint map_sel1, map_sel2;
	last_scale_trans.transform_back(sel1, map_sel1);
	last_scale_trans.transform_back(sel2, map_sel2);
	double delta_x_o = map_sel2.x - map_sel1.x;
	double delta_y_o = map_sel2.y - map_sel1.y;
	
	double resize_xmin, resize_ymin, resize_xmax, resize_ymax;
	if (is_pan_zoom ) {
		resize_xmin = current_map_x_min - delta_x_o;
		resize_ymin = current_map_y_min - delta_y_o;
		resize_xmax = current_map_x_max - delta_x_o;
		resize_ymax = current_map_y_max - delta_y_o;
	} else {
		resize_xmin = shps_orig_xmin - delta_x_o;
		resize_ymin = shps_orig_ymin - delta_y_o;
		resize_xmax = shps_orig_xmax - delta_x_o;
		resize_ymax = shps_orig_ymax - delta_y_o;
	}

	is_pan_zoom = true;
	current_map_x_min = resize_xmin;
	current_map_y_min = resize_ymin;
	current_map_x_max = resize_xmax;
	current_map_y_max = resize_ymax;
	
	ResizeSelectableShps();
}

void TemplateCanvas::SetMouseMode(MouseMode mode)
{
	mousemode = mode;
	if (mousemode == select) {
		//SetCursor(*wxCROSS_CURSOR);
		SetCursor(*wxSTANDARD_CURSOR);
	} else if (mousemode == pan) {
		SetCursor(wxCursor(wxCURSOR_HAND));
	} else if (mousemode == zoom) {
		SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
	} else { // default 
		SetCursor(*wxSTANDARD_CURSOR);
	}
}

void TemplateCanvas::CreateSelShpsFromProj(
							std::vector<GdaShape*>& selectable_shps,
							Project* project)
{
	using namespace Shapefile;
	
	if (selectable_shps.size() > 0) return;
	int num_recs = project->GetNumRecords();
	selectable_shps.resize(num_recs);
	std::vector<MainRecord>& records = project->main_data.records;
	Header& hdr = project->main_data.header;
	
	if (hdr.shape_type == Shapefile::POINT) {
		PointContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PointContents*) records[i].contents_p;
			if (pc->shape_type == 0) {
				selectable_shps[i] = new GdaPoint();
			} else {
				selectable_shps[i] = new GdaPoint(wxRealPoint(pc->x,pc->y));
			}
		}
	} else if (hdr.shape_type == Shapefile::POLYGON) {
		//namespace bg = boost::geometry;
		//namespace bgi = boost::geometry::index;
		//typedef bg::model::point<float, 2, bg::cs::cartesian> point;
		//typedef bg::model::box<point> box;
		//typedef std::pair<box, int> value;
		// create the rtree using default constructor
		//bgi::rtree< value, bgi::rstar<16, 4> > rtree;
		PolygonContents* pc = 0;
		for (int i=0; i<num_recs; i++) {
			pc = (PolygonContents*) records[i].contents_p;
			selectable_shps[i] = new GdaPolygon(pc);
			//box b(point(pc->box[0],pc->box[1]), point(pc->box[2],pc->box[3]));
			//rtree.insert(std::make_pair(b, i));
		}
	} else if (hdr.shape_type == Shapefile::POLY_LINE) {
		PolyLineContents* pc = 0;
		wxPen pen(GdaConst::selectable_fill_color, 1, wxSOLID);
		for (int i=0; i<num_recs; i++) {
			pc = (PolyLineContents*) records[i].contents_p;
			selectable_shps[i] = new GdaPolyLine(pc);
		}
	}
	/*
	//todo: test QuadTree using centroids from selectable_shps
	double x,y, center_min_x, center_min_y, center_max_x, center_max_y;
	for ( int i=0; i<num_recs; i++) {
		x = selectable_shps[i]->center_o.x;
		y = selectable_shps[i]->center_o.y;
		if (i == 0) {
			center_min_x = x;center_min_y = y;center_max_x=x;center_max_y=y;
		} else {
			if ( center_min_x > x ) center_min_x = x;
			else if (center_max_x < x) center_max_x = x;
			if ( center_min_y > y ) center_min_y = y;
			else if (center_max_y < y) center_max_y = y;
		}
	}
	GdaRealRect rect(center_min_x,center_min_y,
					 (center_max_x-center_min_x) * 1.1,
					 (center_max_y - center_min_y) * 1.1);
	if (qtree != NULL) {
		delete qtree;
		qtree = NULL;
	}
	qtree = new QuadTree(rect);
	for( int i=0; i< selectable_shps.size(); i++ ) {
		qtree->Insert(selectable_shps[i],i);
	}
	*/
}

wxRealPoint TemplateCanvas::MousePntToObsPnt(const wxPoint &pt)
{
	//LOG_MSG("Entering TemplateCanvas::MousePntToObsPnt");
	wxSize size(GetVirtualSize());
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
	double x = pt.x;
	double y = pt.y;
	x = (x-trans_x)/scale_x;
	y = (y-trans_y)/scale_y;
	double my_x_factor =
		(data_scale_xmax-data_scale_xmin)/(shps_orig_xmax-shps_orig_xmin);
	double my_y_factor =
		(data_scale_ymax-data_scale_ymin)/(shps_orig_ymax-shps_orig_ymin);
	x = x*my_x_factor + data_scale_xmin;
	y = y*my_y_factor + data_scale_ymin;	
	//LOG_MSG("Exiting TemplateCanvas::MousePntToObsPnt");
	return wxRealPoint(x,y);
}

void TemplateCanvas::SetSelectableOutlineVisible(bool visible)
{
	wxString msg("Called TemplateCanvas::SetSelectableOutlineVisible(");
	if (visible) { msg << "true)"; } else { msg << "false)"; }
	LOG_MSG(msg);
	selectable_outline_visible = visible;
	layer0_valid = false;
	UpdateSelectableOutlineColors();
	Refresh();
}

bool TemplateCanvas::IsSelectableOutlineVisible()
{
	return selectable_outline_visible;
}

void TemplateCanvas::SetSelectableOutlineColor(wxColour color)
{
	LOG_MSG("Called TemplateCanvas::SetSelectableOutlineColor");
	selectable_outline_color = color;
	layer0_valid = false;
	UpdateSelectableOutlineColors();
	Refresh();
}

void TemplateCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	UpdateSelectableOutlineColors();
	layer0_valid = false;
	Refresh();
}

void TemplateCanvas::SetHighlightColor(wxColour color)
{
	highlight_color = color;
	layer1_valid = false;
	Refresh();
}

void TemplateCanvas::SetCanvasBackgroundColor(wxColour color)
{
	canvas_background_color = color;
	layer0_valid = false;
	Refresh();
}

void TemplateCanvas::UpdateSelectableOutlineColors()
{
	LOG_MSG("Called TemplateCanvas::UpdateSelectableOutlineColors");
	wxPen pen(selectable_outline_visible ? selectable_outline_color :
			  selectable_fill_color, 1, wxSOLID);
	wxBrush brush(GdaConst::selectable_fill_color, wxSOLID);
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (GdaPolyLine* p = dynamic_cast<GdaPolyLine*>(selectable_shps[i])) {
			//wxPen pen(GdaConst::selectable_fill_color, 1, wxSOLID);
			//selectable_shps[i]->pen = pen;
		} else {  // same for all other GdaShapes
			//selectable_shps[i]->pen = pen;
		}
		//selectable_shps[i]->brush = brush;
	}
}

void TemplateCanvas::OnSize(wxSizeEvent& event)
{
	//LOG_MSG("Entering TemplateCanvas::OnSize");
	// we know there has been a change in the client size
	int cs_w=0, cs_h=0;
	GetClientSize(&cs_w, &cs_h);
	int vs_w, vs_h;
	GetVirtualSize(&vs_w, &vs_h);
	
	double new_w = (cs_w-(virtual_screen_marg_left +
                          virtual_screen_marg_right));
    double new_h = (cs_h-(virtual_screen_marg_top +
                          virtual_screen_marg_bottom));
    double new_ar = (double) new_w / (double) new_h;
    //LOG(new_w);
    //LOG(new_h);
    //LOG(new_ar);
    //LOG(fixed_aspect_ratio_mode);
    //LOG(fixed_aspect_ratio_val);
    if (fixed_aspect_ratio_mode) {
        if (fixed_aspect_ratio_val >= new_ar) {
            current_shps_width = new_w;
            current_shps_height = new_w / fixed_aspect_ratio_val;
        } else {
            current_shps_height = new_h;
            current_shps_width = new_h * fixed_aspect_ratio_val;
        }
    } else {
        current_shps_width = new_w;
        current_shps_height = new_h;
    }
    //LOG(current_shps_width);
    //LOG(current_shps_height);
    resizeLayerBms(cs_w, cs_h);
    //SetVirtualSize(cs_w, cs_h);
    ResizeSelectableShps();

	event.Skip();
	//LOG_MSG("Exiting TemplateCanvas::OnSize");
}

/**
 Impelmentation of HighlightStateObservable interface.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void TemplateCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering TemplateCanvas::update");

	if (draw_sel_shps_by_z_val) {
		// force a full redraw
		layer0_valid = false;
		Refresh();
		LOG_MSG("Exiting TemplateCanvas::update");
		return;
	}
	
	int nh_cnt = highlight_state->GetTotalNewlyHighlighted();
	int nuh_cnt = highlight_state->GetTotalNewlyUnhighlighted();
	//std::vector<int>& nh = highlight_state->GetNewlyHighlighted();

	HighlightState::EventType type = highlight_state->GetEventType();
	if (type == HighlightState::delta) {
		LOG_MSG("processing HighlightState::delta");
		wxMemoryDC dc(*layer1_bm);
		if (!layer0_valid) {
			DrawLayer0();
			dc.DrawBitmap(*layer0_bm, 0, 0);
		}
		if (nuh_cnt > 0) EraseNewUnSelShapes(dc);
		if (nh_cnt > 0) DrawNewSelShapes(dc);
		layer1_valid = true;
		layer2_valid = false;
	 
		Refresh();
	} else {
		LOG_MSG("processing  HighlightState::unhighlight_all or invert");
		// type == HighlightState::unhighlight_all
		// type == HighlightState::invert
		//layer0_valid = false;
		layer1_valid = false;
		layer2_valid = false;
		
		Refresh();
	}

	LOG_MSG("Exiting TemplateCanvas::update");
}

// Paint events are generated when user interaction
// causes regions to need repainting, or when wxWindow::Refresh
// wxWindow::RefreshRect is called.  wxWindow::Update can be
// called immediately after Refresh or RefreshRect to force the
// paint event to be called immediately.  Use the
// wxFULL_REPAINT_ON_RESIZE window style to have the entire
// window included in the update region.  This is important in the
// case where resizing the window changes the position of all of
// the window graphics.
void TemplateCanvas::OnPaint(wxPaintEvent& event)
{
	//LOG_MSG("Entering TemplateCanvas::OnPaint");
	DrawLayers();

	wxMemoryDC dc(*layer2_bm);
	wxPaintDC paint_dc(this);
	wxSize sz = GetClientSize();
	paint_dc.Blit(0, 0, sz.x, sz.y, &dc, 0, 0);
	//int xx, yy;
	//CalcUnscrolledPosition(0, 0, &xx, &yy);
	//LOG(xx);
	//LOG(yy);
	//LOG(sz.x);
	//paint_dc.Blit(0, 0, sz.x, sz.y, &dc, xx, yy);
	
	// Draw the the selection region if needed
	PaintSelectionOutline(paint_dc);
	
	// Draw optional control objects if needed
	PaintControls(paint_dc);
	
	// The resize event will ruin the position of scroll bars, so we reset the
	// position of scroll bars again.
	if (prev_scroll_pos_x > 0) SetScrollPos(wxHORIZONTAL, prev_scroll_pos_x);
	if (prev_scroll_pos_y > 0) SetScrollPos(wxVERTICAL, prev_scroll_pos_y);
	//LOG_MSG("Exiting TemplateCanvas::OnPaint");
    event.Skip();
}

void TemplateCanvas::RenderToDC(wxDC &dc, bool disable_crosshatch_brush)
{
	wxSize sz = GetVirtualSize();
	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	dc.DrawRectangle(wxPoint(0,0), sz);
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	if (draw_sel_shps_by_z_val) {
		DrawSelectableShapesByZVal(dc, disable_crosshatch_brush);
	} else {
		DrawSelectableShapes_gen_dc(dc);
	}
	
	if (!draw_sel_shps_by_z_val) {
		DrawHighlightedShapes_gen_dc(dc, disable_crosshatch_brush);
	}
	
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}
	
}

void TemplateCanvas::DrawLayers()
{
	if (layer2_valid && layer1_valid && layer0_valid) return;
	if (!layer0_valid) {
		layer1_valid = false;
		layer2_valid = false;
		DrawLayer0();
		DrawLayer1();
	} else if (!layer1_valid) {
		layer2_valid = false;
		DrawLayer1();
	}
	DrawLayer2();
}

// Draw all solid background, background decorations and unhighlighted
// shapes.
void TemplateCanvas::DrawLayer0()
{
	//LOG_MSG("In TemplateCanvas::DrawLayer0");
	wxSize sz = GetVirtualSize();
	if (!layer0_bm) resizeLayerBms(sz.GetWidth(), sz.GetHeight());
	wxMemoryDC dc(*layer0_bm);
	dc.SetPen(canvas_background_color);
	dc.SetBrush(canvas_background_color);
	dc.DrawRectangle(wxPoint(0,0), sz);
	
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	if (draw_sel_shps_by_z_val) {
		DrawSelectableShapesByZVal(dc);
	} else {
		DrawSelectableShapes(dc);
	}
	
	layer0_valid = true;
	layer1_valid = false;
	layer2_valid = false;
}

// Copy in layer0_bm and draw highlighted shapes.
void TemplateCanvas::DrawLayer1()
{
	//LOG_MSG("In TemplateCanvas::DrawLayer1");
	if (!layer0_valid) DrawLayer0();
	wxMemoryDC dc(*layer1_bm);
	dc.DrawBitmap(*layer0_bm, 0, 0);
	if (!draw_sel_shps_by_z_val) DrawHighlightedShapes(dc);
	
	layer1_valid = true;
	layer2_valid = false;
}

void TemplateCanvas::DrawLayer2()
{
	//LOG_MSG("In TemplateCanvas::DrawLayer2");
	if (!layer1_valid) DrawLayer1();
	wxMemoryDC dc(*layer2_bm);
	dc.DrawBitmap(*layer1_bm, 0, 0);
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}
	
	layer2_valid = true;
}

/** Called only when draw_sel_shps_by_z_val is true.  This method renders
 all selectable shapes according to the order in the height permutation
 vector called z_val_order.  It draws highlights as needed and sets colors
 according to color specified in actual shape object.
 Since it is only used by Bubble Chart, we only define the drawing behavior
 for cicles at present.  Also, since Bubble Chart should not be used for
 large numbers of observations, we only provide a wxDC version. */
void TemplateCanvas::DrawSelectableShapesByZVal(wxDC &dc,
												bool disable_crosshatch_brush)
{
	if (selectable_shps_type != circles) return;
	
	wxBrush hc_brush(disable_crosshatch_brush ? wxBrush(highlight_color) :
					 wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	dc.SetPen(*wxTRANSPARENT_PEN);
	
	int cc_ts = cat_data.curr_canvas_tm_step;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (selectable_shps[i]->isNull()) continue;
		int obs = z_val_order[cc_ts][i][0];
		int cat = z_val_order[cc_ts][i][1];
		dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
		if (selectable_outline_visible) {
			dc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
		}
		dc.DrawCircle(((GdaCircle*) selectable_shps[obs])->center.x,
					  ((GdaCircle*) selectable_shps[obs])->center.y,
					  ((GdaCircle*) selectable_shps[obs])->radius);
		if (hs[obs]) {
			dc.SetBrush(hc_brush);
			if (selectable_outline_visible) {
				dc.SetPen(highlight_color);
			}
			dc.SetBrush(hc_brush);
			dc.DrawCircle(((GdaCircle*) selectable_shps[obs])->center.x,
						  ((GdaCircle*) selectable_shps[obs])->center.y,
						  ((GdaCircle*) selectable_shps[obs])->radius);
		}
	}
}

// draw unhighlighted selectable shapes
void TemplateCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	//LOG_MSG("In TemplateCanvas::DrawSelectableShapes");
	wxStopWatch sw;
	if (use_category_brushes) {
#ifdef __WXMAC__
		DrawSelectableShapes_gc(dc);
#else
		DrawSelectableShapes_dc(dc);
#endif
	} else {
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			selectable_shps[i]->paintSelf(dc);
		}
	}
	LOG_MSG(wxString::Format("DrawSelectableShapes render time: "
							 "%ld ms", sw.Time()));
}

// draw unhighlighted selectable shapes with wxGraphicsContext
void TemplateCanvas::DrawSelectableShapes_gc(wxMemoryDC &dc)
{
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;
	gc->SetPen(wxPen(selectable_outline_color));
	gc->SetBrush(wxBrush(selectable_fill_color));
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	if (selectable_shps_type == points) {
		std::vector<bool> dirty(w*h, false);
		int dirty_cnt = 0;
		gc->SetAntialiasMode(wxANTIALIAS_NONE);
		wxDouble r = GdaConst::my_point_click_radius;
		wxGraphicsPath path = gc->CreatePath();
		GdaPoint* p;
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			//gc->SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=ids.size(); i<iend; i++) {
				p = (GdaPoint*) selectable_shps[ids[i]];
				if (p->isNull()) continue;
				path.AddCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//path.AddCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
			//gc->FillPath(path, wxWINDING_RULE);
			//if (selectable_outline_visible)	gc->StrokePath(path);
			gc->StrokePath(path);
		}
	} else if (selectable_shps_type == polygons) {
		//std::vector<bool> dirty(w*h, false);
		int dirty_cnt = 0;
		int poly_pts_cnt = 0;
		GdaPolygon* p;
		if (!selectable_outline_visible) gc->SetAntialiasMode(wxANTIALIAS_NONE);
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			gc->SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=ids.size(); i<iend; i++) {
				p = (GdaPolygon*) selectable_shps[ids[i]];
				if (p->isNull()) continue;
				if (p->all_points_same) {
                    path.AddCircle(p->center.x, p->center.y, 0.2);
					/*
					if (!dirty[p->center.x + p->center.y*w]) {
						//path.AddCircle(p->center.x, p->center.y, 0.2);
						//dirty[p->center.x + p->center.y*w] = true;
					} else {
						//dirty_cnt++;
					}
					 */
				} else {
					for (int c=0, s=0, t=p->count[0]; c<p->n_count; c++) {
						path.MoveToPoint(p->points[s]);
						//dirty[p->points[s].x + p->points[s].y*w] = true;
						for (int pt=s+1; pt<t && pt<p->n; pt++) {
							path.AddLineToPoint(p->points[pt]);
							//dirty[p->points[pt].x + p->points[pt].y*w] = true;
							poly_pts_cnt++;
						}
						path.CloseSubpath();
						s = t;
						if (c+1 < p->n_count) t += p->count[c+1];
					}
				}
			}
			gc->FillPath(path, wxWINDING_RULE);
			if (selectable_outline_visible)	gc->StrokePath(path);
		}
		//double p_dirty_cnt = (double) dirty_cnt * 100 / (double) num_obs;
		//wxString s;
		//s << "dirty_cnt: " << dirty_cnt << ", " << p_dirty_cnt << "%";
		//LOG_MSG(s);
		//s = "Polygon points drawn: ";
		//s << poly_pts_cnt;
		//LOG_MSG(s);
	} else if (selectable_shps_type == circles) {
		// Only Cartogram map uses this currently
		GdaCircle* c;
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			gc->SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			
			if (selectable_outline_visible) {
				for (int i=0, iend=ids.size(); i<iend; i++) {
					c = (GdaCircle*) selectable_shps[ids[i]];
					if (c->isNull()) continue;
					wxGraphicsPath path = gc->CreatePath();
					path.AddCircle(c->center.x, c->center.y, c->radius);
					gc->FillPath(path, wxWINDING_RULE);
					gc->StrokePath(path);
				}
			} else {
				// Note: in the case of circles, it is much slower
				// to batch render all of the circles together rather
				// than filling them one at a time.  This does not appear
				// to be true for polygons.
				gc->SetAntialiasMode(wxANTIALIAS_NONE);
				for (int i=0, iend=ids.size(); i<iend; i++) {
					c = (GdaCircle*) selectable_shps[ids[i]];
					if (c->isNull()) continue;
					wxGraphicsPath path = gc->CreatePath();
					path.AddCircle(c->center.x, c->center.y, c->radius);
					gc->FillPath(path, wxWINDING_RULE);
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
			gc->SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			std::vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=ids.size(); i<iend; i++) {
				s = (GdaPolyLine*) selectable_shps[ids[i]];
				if (s->isNull()) continue;
				path.MoveToPoint(s->points[0]);
				for (int v=0; v < s->n-1; v++) {
					path.AddLineToPoint(s->points[v+1]);
				}
			}
			gc->StrokePath(path);
		}
	}
	
	delete gc;
}

void TemplateCanvas::DrawSelectableShapes_dc(wxMemoryDC &dc)
{
	DrawSelectableShapes_gen_dc(dc);
}

// draw unhighlighted selectable shapes with wxDC
void TemplateCanvas::DrawSelectableShapes_gen_dc(wxDC &dc)
{
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(selectable_fill_color));
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	if (selectable_shps_type == points) {
		std::vector<bool> dirty(w*h, false);
		int dirty_cnt = 0;
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		wxDouble r = GdaConst::my_point_click_radius;
		GdaPoint* p;
		for (int cat=0; cat<num_cats; cat++) {
			dc.SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			//dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
				p = (GdaPoint*) selectable_shps[ids[i]];
				if (p->isNull()) continue;
				dc.DrawCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//dc.DrawCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
		}
	} else if (selectable_shps_type == polygons) {
		std::vector<bool> dirty(w*h, false);
		GdaPolygon* p;
		for (int cat=0; cat<num_cats; cat++) {
			if (selectable_outline_visible) {
				dc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			}
			dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids =	cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
				p = (GdaPolygon*) selectable_shps[ids[i]];
				if (p->isNull()) continue;
				if (p->all_points_same) {
					dc.DrawPoint(p->center.x, p->center.y);
					//if (!dirty[p->center.x + p->center.y*w]) {
						//dc.DrawPoint(p->center.x, p->center.y);
						//dirty[p->center.x + p->center.y*w] = true;
					//}
				} else {
					//dirty[p->points[0].x + p->points[0].y*w] = true;
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
			if (selectable_outline_visible) {
				dc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			}
			dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
				c = (GdaCircle*) selectable_shps[ids[i]];
				if (c->isNull()) continue;
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
			dc.SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
			for (int i=0, iend=ids.size(); i<iend; i++) {
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

// draw highlighted selectable shapes
void TemplateCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	//LOG_MSG("In TemplateCanvas::DrawHighlightedShapes");
	if (use_category_brushes) {
#ifdef __WXMAC__
		DrawHighlightedShapes_gc(dc);
#else
		DrawHighlightedShapes_dc(dc);
#endif
		return;
	}
	std::vector<bool>& hs = highlight_state->GetHighlight();
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (hs[i]) {
			selectable_shps[i]->paintSelf(dc);
		}
	}
} 

// draw highlighted selectable shapes with wxGraphicsContext
void TemplateCanvas::DrawHighlightedShapes_gc(wxMemoryDC &dc)
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;
	gc->SetPen(wxPen(selectable_outline_color));
	gc->SetBrush(wxBrush(highlight_color));
	
	wxBrush hc_brush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	wxPen hc_pen(highlight_color);
	
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	if (selectable_shps_type == points) {
		//std::vector<bool> dirty(w*h, false);
		GdaPoint* p;
		gc->SetAntialiasMode(wxANTIALIAS_NONE);
		gc->SetPen(wxPen(highlight_color));
		wxGraphicsPath path = gc->CreatePath();
		wxDouble r = GdaConst::my_point_click_radius;
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (hs[i]) {
				p = (GdaPoint*) selectable_shps[i];
				if (p->isNull()) continue;
				path.AddCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//path.AddCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
		}
		gc->StrokePath(path);
	} else if (selectable_shps_type == polygons) {
		//std::vector<bool> dirty(w*h, false);
		int dirty_cnt = 0;
		GdaPolygon* p;
		if (!selectable_outline_visible) gc->SetAntialiasMode(wxANTIALIAS_NONE);
		wxGraphicsPath path = gc->CreatePath();
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (!hs[i]) continue;
			p = (GdaPolygon*) selectable_shps[i];
			if (p->isNull()) continue;
			if (p->all_points_same) {
				path.AddCircle(p->center.x, p->center.y, 0.2);
				//if (!dirty[p->center.x + p->center.y*w]) {
				//	path.AddCircle(p->center.x, p->center.y, 0.2);
				//	dirty[p->center.x + p->center.y*w] = true;
				//} else {
				//	dirty_cnt++;
				//}
			} else {
				for (int c=0, s=0, t=p->count[0]; c<p->n_count; c++) {
					path.MoveToPoint(p->points[s]);
					//dirty[p->points[s].x + p->points[s].y*w] = true;
					for (int pt=s+1; pt<t && pt<p->n; pt++) {
						path.AddLineToPoint(p->points[pt]);
						//dirty[p->points[pt].x + p->points[pt].y*w] = true;
					}
					path.CloseSubpath();
					s = t;
					if (c+1 < p->n_count) t += p->count[c+1];
				}
			}
		}
		gc->SetBrush(hc_brush);
		gc->FillPath(path, wxWINDING_RULE);
		if (selectable_outline_visible) {
			gc->SetPen(hc_pen);
			gc->StrokePath(path);
		}
	} else if (selectable_shps_type == circles) {
		wxBrush hc_brush(wxBrush(highlight_color,
								 wxBRUSHSTYLE_CROSSDIAG_HATCH));		
		if (selectable_outline_visible) {
			gc->SetPen(hc_pen);
			for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
				if (selectable_shps[i]->isNull()) continue;
				if (hs[i]) {
					wxGraphicsPath path = gc->CreatePath();
					path.AddCircle(((GdaCircle*) selectable_shps[i])->center.x,
								   ((GdaCircle*) selectable_shps[i])->center.y,
								   ((GdaCircle*) selectable_shps[i])->radius);
					gc->SetBrush(selectable_shps[i]->getBrush());
					gc->FillPath(path, wxWINDING_RULE);
					gc->SetBrush(hc_brush);
					gc->FillPath(path, wxWINDING_RULE);
					gc->StrokePath(path);
				}
			}
		} else {
			gc->SetAntialiasMode(wxANTIALIAS_NONE);
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
				if (selectable_shps[i]->isNull()) continue;
				if (hs[i]) {
					path.AddCircle(((GdaCircle*) selectable_shps[i])->center.x,
								   ((GdaCircle*) selectable_shps[i])->center.y,
								   ((GdaCircle*) selectable_shps[i])->radius);
					gc->SetBrush(selectable_shps[i]->getBrush());
					gc->FillPath(path, wxWINDING_RULE);
					gc->SetBrush(hc_brush);
					gc->FillPath(path, wxWINDING_RULE);
				}
			}
		}
	} else if (selectable_shps_type == polylines) {
		// only PCP uses PolyLines currently. So, we assume that there
		// is only one group of line segments connected together.
		// If we support Shapefile polyline map objects, then this will
		// have to change.
		gc->SetPen(highlight_color);
		//gc->SetAntialiasMode(wxANTIALIAS_NONE);
		GdaPolyLine* s = 0;
		wxGraphicsPath path = gc->CreatePath();
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (!hs[i]) continue;
			s = (GdaPolyLine*) selectable_shps[i];
			if (s->isNull()) continue;
			path.MoveToPoint(s->points[0]);
			for (int v=0; v < s->n-1; v++) {
				path.AddLineToPoint(s->points[v+1]);
			}
		}
		gc->StrokePath(path);
	}
	
	delete gc;
}

void TemplateCanvas::DrawHighlightedShapes_dc(wxMemoryDC &dc)
{
	DrawHighlightedShapes_gen_dc(dc, false);
}

// draw highlighted selectable shapes with wxDC
void TemplateCanvas::DrawHighlightedShapes_gen_dc(wxDC &dc,
												  bool disable_crosshatch_brush)
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	wxBrush hc_brush(disable_crosshatch_brush ? wxBrush(highlight_color) :
					 wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	wxPen hc_pen(highlight_color);
	
	if (selectable_outline_visible) {
		dc.SetPen(hc_pen);
	} else {
		dc.SetPen(*wxTRANSPARENT_PEN);
	}
	dc.SetBrush(hc_brush);

	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	if (selectable_shps_type == points) {
		std::vector<bool> dirty(w*h, false);
		GdaPoint* p;
		dc.SetPen(hc_pen);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		wxDouble r = GdaConst::my_point_click_radius;
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (hs[i]) {
				p = (GdaPoint*) selectable_shps[i];
				if (p->isNull()) continue;
				dc.DrawCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//dc.DrawCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
		}
	} else if (selectable_shps_type == polygons) {
		//std::vector<bool> dirty(w*h, false);
		GdaPolygon* p;
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (!hs[i]) continue;
			p = (GdaPolygon*) selectable_shps[i];
			if (p->isNull()) continue;
			if (p->all_points_same) {
				dc.DrawPoint(p->center.x, p->center.y);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//dc.DrawPoint(p->center.x, p->center.y);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			} else {
				//dirty[p->points[0].x + p->points[0].y*w] = true;
				if (p->n_count > 1) {
					dc.DrawPolyPolygon(p->n_count, p->count, p->points);
				} else {
					dc.DrawPolygon(p->n, p->points);
				}
			}
		}
	} else if (selectable_shps_type == circles) {
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (selectable_shps[i]->isNull()) continue;
			if (!hs[i]) continue;
			dc.DrawCircle(((GdaCircle*) selectable_shps[i])->center.x,
						  ((GdaCircle*) selectable_shps[i])->center.y,
						  ((GdaCircle*) selectable_shps[i])->radius);
		}
	}  else if (selectable_shps_type == polylines) {
		// only PCP uses PolyLines currently. So, we assume that there
		// is only one group of line segments connected together.
		// If we support Shapefile polyline map objects, then this will
		// have to change.
		dc.SetPen(hc_pen);
		GdaPolyLine* s = 0;
		for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
			if (!hs[i]) continue;
			s = (GdaPolyLine*) selectable_shps[i];
			if (s->isNull()) continue;
			for (int v=0; v<s->n-1; v++) {
				dc.DrawLine(s->points[v].x, s->points[v].y,
							s->points[v+1].x, s->points[v+1].y);
			}
		}
	}
}

void TemplateCanvas::DrawNewSelShapes(wxMemoryDC &dc)
{
	//LOG_MSG("In TemplateCanvas::DrawNewSelShapes");
	if (use_category_brushes) {
#ifdef __WXMAC__
		DrawNewSelShapes_gc(dc);
#else
		DrawNewSelShapes_dc(dc);
#endif
		return;
	}
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	for (int i=0; i<total; i++) {
		selectable_shps[nh[i]]->paintSelf(dc);
	}
}

void TemplateCanvas::DrawNewSelShapes_gc(wxMemoryDC &dc)
{
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;
	
	wxBrush hc_brush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	wxPen hc_pen(highlight_color);
	
	if (selectable_outline_visible) {
		dc.SetPen(hc_pen);
	} else {
		dc.SetPen(*wxTRANSPARENT_PEN);
	}
	dc.SetBrush(hc_brush);	
	
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	
	if (selectable_shps_type == points) {
		std::vector<bool> dirty(w*h, false);
		GdaPoint* p;
		gc->SetAntialiasMode(wxANTIALIAS_NONE);
		gc->SetPen(hc_pen);
		wxGraphicsPath path = gc->CreatePath();
		wxDouble r = GdaConst::my_point_click_radius;
		for (int i=0; i<total; i++) {
			p = (GdaPoint*) selectable_shps[nh[i]];
			if (p->isNull()) continue;
			path.AddCircle(p->center.x, p->center.y, r);
			//if (!dirty[p->center.x + p->center.y*w]) {
				//path.AddCircle(p->center.x, p->center.y, r);
				//dirty[p->center.x + p->center.y*w] = true;
			//}
		}
		gc->StrokePath(path);
	} else if (selectable_shps_type == polygons) {
		GdaPolygon* p;
		if (!selectable_outline_visible) gc->SetAntialiasMode(wxANTIALIAS_NONE);
		wxGraphicsPath path = gc->CreatePath();
		for (int i=0; i<total; i++) {
			p = (GdaPolygon*) selectable_shps[nh[i]];
			if (p->isNull()) continue;
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
					if (c+1 < p->n_count) t += p->count[c+1];
				}
			}
		}
		gc->SetBrush(hc_brush);
		gc->FillPath(path, wxWINDING_RULE);
		if (selectable_outline_visible) {
			gc->SetPen(hc_pen);
			gc->StrokePath(path);
		}
	} else if (selectable_shps_type == circles) {
		GdaCircle* c;
		gc->SetBrush(hc_brush);
		if (selectable_outline_visible) {
			gc->SetPen(hc_pen);
			for (int i=0; i<total; i++) {
				c = (GdaCircle*) selectable_shps[nh[i]];
				if (c->isNull()) continue;
				wxGraphicsPath path = gc->CreatePath();
				path.AddCircle(c->center.x, c->center.y, c->radius);
				gc->FillPath(path, wxWINDING_RULE);
				gc->StrokePath(path);
			}
		} else {
			// Note: in the case of circles, it is much slower
			// to batch render all of the circles together rather
			// than filling them one at a time.  This does not appear
			// to be true for polygons.
			gc->SetAntialiasMode(wxANTIALIAS_NONE);
			for (int i=0; i<total; i++) {
				c = (GdaCircle*) selectable_shps[nh[i]];
				if (c->isNull()) continue;
				wxGraphicsPath path = gc->CreatePath();
				path.AddCircle(c->center.x, c->center.y, c->radius);
				gc->FillPath(path, wxWINDING_RULE);
			}
		}
	} else if (selectable_shps_type == polylines) {
		gc->SetPen(highlight_color);
		//gc->SetAntialiasMode(wxANTIALIAS_NONE);
		GdaPolyLine* s = 0;
		wxGraphicsPath path = gc->CreatePath();
		for (int i=0; i<total; i++) {
			s = (GdaPolyLine*) selectable_shps[nh[i]];
			if (s->isNull()) continue;
			path.MoveToPoint(s->points[0]);
			for (int v=0; v < s->n-1; v++) {
				path.AddLineToPoint(s->points[v+1]);
			}
		}
		gc->StrokePath(path);
	}
	
	delete gc;	
}

void TemplateCanvas::DrawNewSelShapes_dc(wxMemoryDC &dc)
{
	wxBrush hc_brush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	wxPen hc_pen(highlight_color);
		
	if (selectable_outline_visible) {
		dc.SetPen(hc_pen);
	} else {
		dc.SetPen(*wxTRANSPARENT_PEN);
	}
	dc.SetBrush(hc_brush);
	
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	
	if (selectable_shps_type == points) {
		std::vector<bool> dirty(w*h, false);
		dc.SetPen(hc_pen);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		GdaPoint* p;
		wxDouble r = GdaConst::my_point_click_radius;
		for (int i=0; i<total; i++) {
			p = (GdaPoint*) selectable_shps[nh[i]];
			if (p->isNull()) continue;
			dc.DrawCircle(p->center.x, p->center.y, r);
			//if (!dirty[p->center.x + p->center.y*w]) {
				//dc.DrawCircle(p->center.x, p->center.y, r);
				//dirty[p->center.x + p->center.y*w] = true;
			//}
		}
	} else if (selectable_shps_type == polygons) {
		GdaPolygon* p;
		for (int i=0; i<total; i++) {
			p = (GdaPolygon*) selectable_shps[nh[i]];
			if (p->isNull()) continue;
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
	} else if (selectable_shps_type == circles) {
		GdaCircle* c;
		for (int i=0; i<total; i++) {
			c = (GdaCircle*) selectable_shps[nh[i]];
			if (c->isNull()) continue;
			dc.DrawCircle(c->center.x, c->center.y, c->radius);
		}
	} else if (selectable_shps_type == polylines) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		GdaPolyLine* s = 0;
		for (int i=0; i<total; i++) {
			s = (GdaPolyLine*) selectable_shps[nh[i]];
			if (s->isNull()) continue;
			for (int v=0; v<s->n-1; v++) {
				dc.DrawLine(s->points[v].x, s->points[v].y,
							s->points[v+1].x, s->points[v+1].y);
			}
		}
	}
}

void TemplateCanvas::EraseNewUnSelShapes(wxMemoryDC &dc)
{
	//LOG_MSG("In TemplateCanvas::EraseNewUnSelShapes");
	if (use_category_brushes) {
#ifdef __WXMAC__
		EraseNewUnSelShapes_gc(dc);
#else
		EraseNewUnSelShapes_dc(dc);
#endif
		return;
	}
	int total = highlight_state->GetTotalNewlyUnhighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	for (int i=0; i<total; i++) {
		selectable_shps[nuh[i]]->paintSelf(dc);
	}
}

void TemplateCanvas::EraseNewUnSelShapes_gc(wxMemoryDC &dc)
{
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;
	gc->SetPen(wxPen(selectable_outline_color));
	gc->SetBrush(wxBrush(selectable_fill_color));
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	
	int total = highlight_state->GetTotalNewlyUnhighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();

	// scratch will contain a list of each obs to erease in each category
	// total_in_cat[cat] records the number of ids in each list.
	i_array_type& scratch =
		template_frame->GetProject()->GetSharedCategoryScratch();
	std::vector<int>& id_to_cat = cat_data.categories[cc_ts].id_to_cat;
	std::vector<int> total_in_cat(num_cats,0);
	int cat;
	for (int i=0; i<total; i++) {
		cat = id_to_cat[nuh[i]];
		scratch[cat][total_in_cat[cat]++] = nuh[i];
	}
	
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	if (selectable_shps_type == points) {
		//std::vector<bool> dirty(w*h, false);
		GdaPoint* p;
		gc->SetAntialiasMode(wxANTIALIAS_NONE);
		wxDouble r = GdaConst::my_point_click_radius;
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			//wxColour c(cat_data.GetCategoryColor(cc_ts, cat));
			//LOG_MSG(wxString::Format("GetCategoryColor(%d, %d): %d,%d,%d",
			//						 cc_ts, cat, (int) c.Red(),
			//						 (int) c.Green(), (int) c.Blue()));
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				p = (GdaPoint*) selectable_shps[scratch[cat][i]];
				if (p->isNull()) continue;
				path.AddCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//path.AddCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
			gc->StrokePath(path);
		}
	} else if (selectable_shps_type == polygons) {
		GdaPolygon* p;
		if (!selectable_outline_visible) gc->SetAntialiasMode(wxANTIALIAS_NONE);
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			gc->SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				p = (GdaPolygon*) selectable_shps[scratch[cat][i]];
				if (p->isNull()) continue;
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
						if (c+1 < p->n_count) t += p->count[c+1];
					}
				}
			}
			gc->FillPath(path, wxWINDING_RULE);
			if (selectable_outline_visible)	gc->StrokePath(path);
		}
	} else if (selectable_shps_type == circles) {
		GdaCircle* c;
		if (!selectable_outline_visible) gc->SetAntialiasMode(wxANTIALIAS_NONE);
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			gc->SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				wxGraphicsPath path = gc->CreatePath();
				c = (GdaCircle*) selectable_shps[scratch[cat][i]];
				if (c->isNull()) continue;
				path.AddCircle(c->center.x, c->center.y, c->radius);
				gc->FillPath(path, wxWINDING_RULE);
				if (selectable_outline_visible)	gc->StrokePath(path);
			}
		}
	} else if (selectable_shps_type == polylines) {
		GdaPolyLine* s = 0;
		//gc->SetAntialiasMode(wxANTIALIAS_NONE);
		for (int cat=0; cat<num_cats; cat++) {
			gc->SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			
			wxGraphicsPath path = gc->CreatePath();
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				s = (GdaPolyLine*) selectable_shps[scratch[cat][i]];
				if (s->isNull()) continue;
				path.MoveToPoint(s->points[0]);
				for (int v=0; v < s->n-1; v++) {
					path.AddLineToPoint(s->points[v+1]);
				}
			}
			gc->StrokePath(path);
		}
	}
	
	delete gc;	
}

void TemplateCanvas::EraseNewUnSelShapes_dc(wxMemoryDC &dc)
{	 
	dc.SetPen(*wxTRANSPARENT_PEN);
	
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats=cat_data.GetNumCategories(cc_ts);
	int total = highlight_state->GetTotalNewlyUnhighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	
	// scratch will contain a list of each obs to erease in each category
	// total_in_cat[cat] records the number of ids in each list.
	i_array_type& scratch =
	template_frame->GetProject()->GetSharedCategoryScratch();
	std::vector<int>& id_to_cat = cat_data.categories[cc_ts].id_to_cat;
	std::vector<int> total_in_cat(num_cats,0);
	int cat;
	for (int i=0; i<total; i++) {
		cat = id_to_cat[nuh[i]];
		scratch[cat][total_in_cat[cat]++] = nuh[i];
	}
	
	int w = layer0_bm->GetWidth();
	int h = layer0_bm->GetHeight();
	
	if (selectable_shps_type == points) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		//std::vector<bool> dirty(w*h, false);
		GdaPoint* p;
		wxDouble r = GdaConst::my_point_click_radius;
		for (int cat=0; cat<num_cats; cat++) {
			dc.SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				p = (GdaPoint*) selectable_shps[scratch[cat][i]];
				if (p->isNull()) continue;
				dc.DrawCircle(p->center.x, p->center.y, r);
				//if (!dirty[p->center.x + p->center.y*w]) {
					//dc.DrawCircle(p->center.x, p->center.y, r);
					//dirty[p->center.x + p->center.y*w] = true;
				//}
			}
		}
	} else if (selectable_shps_type == polygons) {
		GdaPolygon* p;
		for (int cat=0; cat<num_cats; cat++) {
			dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			if (selectable_outline_visible) {
				dc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			}
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				p = (GdaPolygon*) selectable_shps[scratch[cat][i]];
				if (p->isNull()) continue;
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
		GdaCircle* c;
		for (int cat=0; cat<num_cats; cat++) {
			dc.SetBrush(cat_data.GetCategoryBrush(cc_ts, cat));
			if (selectable_outline_visible) {
				dc.SetPen(cat_data.GetCategoryPen(cc_ts, cat));
			}
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				c = (GdaCircle*) selectable_shps[scratch[cat][i]];
				if (c->isNull()) continue;
				dc.DrawCircle(c->center.x, c->center.y, c->radius);
			}
		}
	} else if (selectable_shps_type == polylines) {
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		GdaPolyLine* s = 0;
		for (int cat=0; cat<num_cats; cat++) {
			dc.SetPen(cat_data.GetCategoryColor(cc_ts, cat));
			for (int i=0, iend=total_in_cat[cat]; i<iend; i++) {
				s = (GdaPolyLine*) selectable_shps[scratch[cat][i]];
				if (s->isNull()) continue;
				for (int v=0; v<s->n-1; v++) {
					dc.DrawLine(s->points[v].x, s->points[v].y,
								s->points[v+1].x, s->points[v+1].y);
				}
			}
		}
	}	
}


// We will handle drawing our background in a paint event
// handler.  So, do nothing in this handler.
void TemplateCanvas::OnEraseBackground(wxEraseEvent& event)
{	
	// LOG_MSG("TemplateCanvas::OnEraseBackground called, do nothing.");
}

// wxMouseEvent notes:
// LeftDown(): true when the left button is first pressed down
// LeftIsDown(): true while the left button is down. During a mouse dragging
//  operation, this will continue to return true, while LeftDown() is false.
// RightDown/RightIsDown: similar to Left.
// Moving(): returns true when mouse is moved, but no buttons are pressed.
// Dragging(): returns true when mouse is moved and at least one mouse button is
//   pressed.
// CmdDown(): Checks if MetaDown under Mac and ControlDown on other platforms.
// ButtonDClick(int but = wxMOUSE_BTN_ANY): checks for double click of any
//   button. Can also specify wxMOUSE_BTN_LEFT / RIGHT / MIDDLE.  Or
//   LeftDCLick(), etc.
// LeftUp(): returns true at the moment the button changed to up.

void TemplateCanvas::OnMouseEvent(wxMouseEvent& event)
{
	// Capture the mouse when left mouse button is down.
	if (event.LeftIsDown() && !HasCapture()) CaptureMouse();
	if (event.LeftUp() && HasCapture()) ReleaseMouse();
	
	// Scrollwheel data is highly platform specific at this point,
	// so it is unavoidable to have custom code for each different
	// platform.  As of wxWidgets 2.8.x there is no official support
	// for horizontal scrollwheel data, but it appears as though
	// Windows fakes this out by setting the wheel rotation value
	// +/-120 for vertical movement and +/-240 for horizontal movement.
	// We will have to check that this behaviour is consistent
	// across all versions of Windows.  Linux and Windows return +/-120
	// (and +/-240 in the case of Windows) while Mac returns any multiple
	// of 1.  This could possibly be to indicate amount of movement or
	// speed.  In wxWidgets 2.9.x, there is official support for horizontal
	// scrollwheel data on all platforms.
	
	// For non scroll events such as mouse movement, GetWheelDelta
	// sometimes returns 0 (Mac only).  To test if this is actually
	// a scroll event, we make sure event.GetLinesPerAction() returns at
	// least 1.  We might want to ignore scrolling when a mouse button is
	// being held down, or when in brushing mode.
	int wheel_rotation = event.GetWheelRotation();
	int wheel_delta = GenUtils::max<int>(event.GetLinesPerAction(), 1);
	int wheel_lines_per_action =GenUtils::max<int>(event.GetLinesPerAction(),1);
	if (abs(wheel_rotation) >= wheel_delta) {
		LOG(wheel_rotation);
		LOG(wheel_delta);
		LOG(wheel_lines_per_action);
	}
	
	if (mousemode == select) {
		if (selectstate == start) {
			if (event.LeftDown()) {
				prev = GetActualPos(event);
				sel1 = prev;
				selectstate = leftdown;
			} else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			} else {
				if (template_frame->IsStatusBarVisible()) {
					prev = GetActualPos(event);
					sel1 = prev; // sel1 now has current mouse position
					DetermineMouseHoverObjects();
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
			LOG_MSG("TemplateCanvas::OnMouseEvent: ERROR, unknown SelectState");
		}
		
	} else if (mousemode == zoom) {
		// we will allow zooming in up to a maximum virtual screen area
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
					remember_shiftdown = event.ShiftDown();
					Refresh();
				}
			} else if (event.LeftUp()) {
				if (event.ShiftDown() || event.CmdDown()) {
					// zoom out by a factor of two
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
				Refresh();
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				Refresh();
			} else if (event.LeftUp() ) {
				sel2 = GetActualPos(event);
				remember_shiftdown = event.ShiftDown() || event.CmdDown();
				ZoomShapes(!remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
				Refresh();
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
					//remember_shiftdown = event.ShiftDown();
					//UpdateSelectRegion();
					//UpdateSelection(remember_shiftdown);
					//UpdateStatusBar();
					Refresh();
				}
			} else if (event.LeftUp()) {
				//UpdateSelectRegion();
				//UpdateSelection(event.ShiftDown(), true);
				selectstate = start;
				Refresh();
			} else if (event.RightDown()) {
				selectstate = start;
			}
		} else if (selectstate == dragging) {
			if (event.Dragging()) { // mouse moved while buttons still down
				sel2 = GetActualPos(event);
				//UpdateSelectRegion();
				//UpdateSelection(remember_shiftdown);
				//UpdateStatusBar();
				Refresh();
			} else if (event.LeftUp() ) {
				sel2 = GetActualPos(event);
				//UpdateSelectRegion();
				//UpdateSelection(remember_shiftdown);
				remember_shiftdown = false;
				selectstate = start;
				PanShapes();
				Refresh();
			}  else if (event.RightDown()) {
				DisplayRightClickMenu(event.GetPosition());
			}			
		}
	}
}

void TemplateCanvas::OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event)
{
	if (HasCapture()) ReleaseMouse();
}

void TemplateCanvas::PaintBackground(wxDC& dc)
{
	int xx, yy;
	CalcUnscrolledPosition(0,0,&xx,&yy);
	wxSize sz = GetClientSize();
	sz.SetWidth(sz.GetWidth()/1.0);  // zoom_factor = 1.0
	sz.SetHeight(sz.GetHeight()/1.0);
	xx = xx/1.0;
	yy = yy/1.0;
	wxRect windowRect(wxPoint(xx,yy), sz);
	dc.SetPen(wxPen(canvas_background_color));
	dc.SetBrush(wxBrush(canvas_background_color, wxSOLID));
	dc.DrawRectangle(wxPoint(0,0), GetVirtualSize());
		
	//Test shapes
	//dc.SetPen(*wxGREEN_PEN);
	//dc.SetBrush(*wxRED_BRUSH);
	//dc.DrawEllipse(wxPoint(180,30),wxSize(100,180));
	
	//int t=0;
	//wxPoint p[30];
	
	// skinny bowtie rectangle
	//p[t].x = -10; p[t++].y = 40;
	//p[t].x = 210; p[t++].y = 60;
	//p[t].x = 210; p[t++].y = 40;
	//p[t].x = -10; p[t++].y = 60;
	//p[t].x = -10; p[t++].y = 40;
	
	// square
	//p[t].x = 20; p[t++].y = 20;
	//p[t].x = 20; p[t++].y = 80;
	//p[t].x = 80; p[t++].y = 80;
	//p[t].x = 80; p[t++].y = 20;
	//p[t].x = 20; p[t++].y = 20;
	
	// clockwise rectangle
	//p[t].x = 0; p[t++].y= 0;
	//p[t].x = 200;  p[t++].y= 0;
	//p[t].x = 200; p[t++].y= 100;
	//p[t].x = 0; p[t++].y= 100;
	//p[t].x = 0; p[t++].y= 0;
	
	// counter-clockwise square
	//p[t].x = 10; p[t++].y = 10;
	//p[t].x = 10; p[t++].y = 90;
	//p[t].x = 90; p[t++].y = 90;
	//p[t].x = 90; p[t++].y = 10;
	//p[t].x = 10; p[t++].y = 10;

	// counter-clockwise square
	//p[t].x = 110; p[t++].y = 10;
	//p[t].x = 110; p[t++].y = 90;
	//p[t].x = 190; p[t++].y = 90;
	//p[t].x = 190; p[t++].y = 10;
	//p[t].x = 110; p[t++].y = 10;

	//p[t].x = 30; p[t++].y = 30;
	//p[t].x = 70; p[t++].y = 30;
	//p[t].x = 70; p[t++].y = 70;
	//p[t].x = 30; p[t++].y = 70;
	//p[t].x = 30; p[t++].y = 30;

	//int count[] = {5, 5, 5, 5, 5, 5};
	//dc.DrawPolyPolygon(6, count, p, 2, 160);
	
	//dc.DrawRectangle(20,20,260,160);
	
}

/** Paint background, selectable and foreground shps */
void TemplateCanvas::PaintShapes(wxDC& dc)
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) {
		shp->paintSelf(dc);
	}
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		DrawGdaSelShape(i, dc);
	}
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) {
		shp->paintSelf(dc);
	}	
}

void TemplateCanvas::PaintSelectionOutline(wxDC& dc)
{
	if ((mousemode == select || mousemode == zoom)&&
		(selectstate == dragging || selectstate == brushing) ) {
		int xx=0, yy=0;
		//CalcUnscrolledPosition(0, 0, &xx, &yy);
		// xx and yy now have the scrolled amount
		wxPoint ssel1(sel1.x-xx, sel1.y-yy);
		wxPoint ssel2(sel2.x-xx, sel2.y-yy);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*wxBLACK_PEN);
		if (brushtype == rectangle) {
			dc.DrawRectangle(wxRect(ssel1, ssel2));
		} else if (brushtype == line) {
			dc.DrawLine(ssel1, ssel2);
		} else if (brushtype == circle) {
			dc.DrawCircle(ssel1, GenUtils::distance(ssel1, ssel2));
			//dc.DrawPolygon(n_sel_poly_pts, sel_poly_pts);
		}
	}
}

void TemplateCanvas::PaintControls(wxDC& dc)
{
	/* // Test

	wxSize sz = GetClientSize();
	int w = 40;
	int h = 40;
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	int xx, yy;
	CalcUnscrolledPosition((sz.x-50),(sz.y-50),&xx,&yy);
	dc.DrawRectangle(xx,yy,w,h);
	dc.DrawCircle(xx,yy,w);
	
	wxPoint a[10];
	a[0] = wxPoint(0,0);
	a[1] = wxPoint(0,160);
	a[2] = wxPoint(160,160);
	a[3] = wxPoint(160,00);
	a[4] = wxPoint(0,0);
	a[5] = wxPoint(40,40);
	a[6] = wxPoint(120,40);
	a[7] = wxPoint(120,120);
	a[8] = wxPoint(40,120);
	a[9] = wxPoint(40,40);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxRED_BRUSH);
	dc.DrawPolygon(5, a, 200, 200);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawPolygon(5, (a+5), 200, 200);

	 */
}

wxPoint TemplateCanvas::GetActualPos(const wxMouseEvent& event) {
	//int xx;
	//int yy;
	return wxPoint(event.GetX(), event.GetY());
	//CalcUnscrolledPosition(event.GetX(), event.GetY(), &xx, &yy);
	//return wxPoint(xx/1.0,yy/1.0); // zoom_factor = 1.0
}

void TemplateCanvas::DrawGdaSelShape(int i, wxDC& dc)
{
	GdaShape* shape = selectable_shps[i];
	if (shape->isNull()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	dc.SetPen(shape->getPen());
	dc.SetBrush(shape->getBrush());
	wxBrush h_brush(highlight_color, wxCROSSDIAG_HATCH);
	wxPen h_pen(highlight_color);
	if (GdaPoint* p = dynamic_cast<GdaPoint*> (shape)) {
		dc.DrawCircle(p->center, GdaConst::my_point_click_radius);
		if (hs[i]) {
			dc.SetPen(h_pen);
			dc.DrawCircle(p->center, GdaConst::my_point_click_radius);
		}
	} else if (GdaCircle* p = dynamic_cast<GdaCircle*> (shape)) {
		dc.DrawCircle(p->center, p->radius);
		if (hs[i]) {
			dc.SetBrush(h_brush);
			dc.DrawCircle(p->center, p->radius);
		}
	} else if (GdaPolygon* p = dynamic_cast<GdaPolygon*> (shape)) {
		if (p->n_count > 1) {
			dc.DrawPolyPolygon(p->n_count, p->count, p->points);
		} else {
			dc.DrawPolygon(p->n, p->points);
		}
		if (hs[i]) {
			dc.SetBrush(h_brush);
			if (p->n_count > 1) {
				dc.DrawPolyPolygon(p->n_count, p->count, p->points);
			} else {
				dc.DrawPolygon(p->n, p->points);
			}
		}
	} else if (GdaPolyLine* p = dynamic_cast<GdaPolyLine*> (shape)) {
		int chunk_index = 0;  // will have the initial index of each part
		for (int h=0; h < p->n_count; h++) {
			if (p->count[h] > 1) {  // ensure this is a valid part
				dc.DrawLines(p->count[h],p->points+chunk_index);
				if (hs[i]) {
					dc.SetPen(h_pen);
					dc.DrawLines(p->count[h],p->points+chunk_index);
				}
			}
			chunk_index += p->count[h]; // increment to next part
		}
	}
}

void TemplateCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("TemplateCanvas::DisplayRightClickMenu() called");
}

void TemplateCanvas::AppendCustomCategories(wxMenu* menu,
											CatClassifManager* ccm)
{
	// search for ID_CAT_CLASSIF_A(B,C)_MENU submenus
	const int num_sub_menus=3;
	std::vector<int> menu_id(num_sub_menus);
	std::vector<int> sub_menu_id(num_sub_menus);
	std::vector<int> base_id(num_sub_menus);
	menu_id[0] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_A");
	menu_id[1] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_B");
	menu_id[2] = XRCID("ID_NEW_CUSTOM_CAT_CLASSIF_C");
	sub_menu_id[0] = XRCID("ID_CAT_CLASSIF_A_MENU");
	sub_menu_id[1] = XRCID("ID_CAT_CLASSIF_B_MENU");
	sub_menu_id[2] = XRCID("ID_CAT_CLASSIF_C_MENU");
	base_id[0] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_A0;
	base_id[1] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_B0;
	base_id[2] = GdaConst::ID_CUSTOM_CAT_CLASSIF_CHOICE_C0;
	
	for (int i=0; i<num_sub_menus; i++) {
		wxMenuItem* smi = menu->FindItem(sub_menu_id[i]);
		if (!smi) continue;
		wxMenu* sm = smi->GetSubMenu();
		if (!sm) continue;
		sm->AppendSeparator();
		sm->Append(menu_id[i], "Create New Custom",
				   "Create new custom categories classification.");
		std::vector<wxString> titles;
		ccm->GetTitles(titles);
		for (size_t j=0; j<titles.size(); j++) {
			wxMenuItem* mi = sm->Append(base_id[i]+j, titles[j]);
		}
	}
}

void TemplateCanvas::UpdateSelectRegion(bool translate, wxPoint diff)
{
	/*
	wxPoint* p = sel_poly_pts;
	//sel_region = wxRegion(wxRect(sel1, sel2));
	
	if (brushtype == rectangle) {
		//sel_region = wxRegion(wxRect(sel1, sel2));
	} else if (brushtype == line) {
		//sel_region = GdaShapeAlgs::createLineRegion(sel1, sel2);
	} else if (brushtype == circle) {
		if (!translate) {
			double radius = GenUtils::distance(sel1, sel2);	
			GdaShapeAlgs::createCirclePolygon(sel1, radius, 0,
											 sel_poly_pts, &n_sel_poly_pts);
		} else { // we are just translating a previously drawn circle
			for (int i=0; i<n_sel_poly_pts; i++) {
				p[i] += diff;
			}
	//		sel_region = wxRegion(n_sel_poly_pts, sel_poly_pts);
		}
		
	}
	 */
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
	LOG_MSG("Entering TemplateCanvas::UpdateSelection");
	if (selectable_shps_type == circles) {
		UpdateSelectionCircles(shiftdown, pointsel);
	} else if (selectable_shps_type == polylines) {
		UpdateSelectionPolylines(shiftdown, pointsel);
	} else {
		UpdateSelectionPoints(shiftdown, pointsel);
	}
	LOG_MSG("Exiting TemplateCanvas::UpdateSelection");
}

// The following function assumes that the set of selectable objects
// being selected against are all points.  Since all GdaShape objects
// define a center point, this is also the default function for
// all GdaShape selectable objects.
void TemplateCanvas::UpdateSelectionPoints(bool shiftdown, bool pointsel)
{
	LOG_MSG("Entering TemplateCanvas::UpdateSelectionPoints");
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	if (pointsel) { // a point selection
		for (int i=0; i<hl_size; i++) {
			if (selectable_shps[i]->pointWithin(sel1)) {
				// A useful way to get polygon data by clicking
				// on a polygon
				//if (GdaPolygon* p =
				//	dynamic_cast<GdaPolygon*>(selectable_shps[i])) {
				//	if (p->pc) {
				//		LOG_MSG(wxString::Format("polygon %d bounding box:",i));
				//		LOG_MSG(wxString::Format("  %f, %f", p->pc->box[0],
				//								 p->pc->box[1]));
				//		LOG_MSG(wxString::Format("  %f, %f", p->pc->box[2],
				//								 p->pc->box[3]));
				//		LOG_MSG(wxString::Format("polygon %d:",i));
				//		for (int j=0; j<p->pc->num_points; j++) {
				//			LOG_MSG(wxString::Format("  %f, %f",
				//									 p->pc->points[j].x,
				//									 p->pc->points[j].y));
				//		}
				//	}
				//}
				if (hs[i]) {
					nuh[total_newly_unselected++] = i;
				} else {
					nh[total_newly_selected++] = i;
				}
			} else {
				if (!shiftdown && hs[i]) {
					nuh[total_newly_unselected++] = i;
				}
			}			
		}
	} else { // determine which obs intersect the selection region.
		if (brushtype == rectangle) {
			/*
			// using map coordinates to query points in quad-tree
			wxRealPoint map_sel1, map_sel2;
			last_scale_trans.transform_back(sel1, map_sel1);
			last_scale_trans.transform_back(sel2, map_sel2);
			// topLeft, bottomRight
			double min_map_x = std::min( map_sel1.x, map_sel2.x);
			double max_map_x = std::max( map_sel1.x, map_sel2.x);
			double min_map_y = std::min( map_sel1.y, map_sel2.y);
			double max_map_y = std::max( map_sel1.y, map_sel2.y);
			GdaRealRect queryBox(min_map_x, min_map_y, max_map_x - min_map_x,
								 max_map_y - min_map_y);
			
			std::map<int,bool> shapesInRange;
			qtree->QueryRange(queryBox, shapesInRange);
							
			for (int i=0; i<hl_size; i++) {
				bool contains = shapesInRange.count(i) > 0;
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
			*/
			
			wxRegion rect(wxRect(sel1, sel2));
			for (int i=0; i<hl_size; i++) {
				bool contains = (rect.Contains(selectable_shps[i]->center) !=
								 wxOutRegion);
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
			
		} else if (brushtype == circle) {
			// using quad-tree to do pre-selection
			
			
			double radius = GenUtils::distance(sel1, sel2);
			// determine if each center is within radius of sel1
			for (int i=0; i<hl_size; i++) {
				bool contains = (GenUtils::distance(sel1,
													selectable_shps[i]->center)
								 <= radius);
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
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
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	LOG_MSG("Exiting TemplateCanvas::UpdateSelectionPoints");
}

// The following function assumes that the set of selectable objects
// being selected against are all GdaCircle objects.
void TemplateCanvas::UpdateSelectionCircles(bool shiftdown, bool pointsel)
{
	LOG_MSG("Entering TemplateCanvas::UpdateSelectionCircles");
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	if (pointsel) { // a point selection
		for (int i=0; i<hl_size; i++) {
			GdaCircle* s = (GdaCircle*) selectable_shps[i];
			if (s->isNull()) continue;
			if (GenUtils::distance(s->center, sel1) <= s->radius) {
				if (hs[i]) {
					nuh[total_newly_unselected++] = i;
				} else {
					nh[total_newly_selected++] = i; 
				}
			} else {
				if (!shiftdown && hs[i]) {
					nuh[total_newly_unselected++] = i;
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
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		} else if (brushtype == circle) {
			double radius = GenUtils::distance(sel1, sel2);
			// determine if circles overlap
			for (int i=0; i<hl_size; i++) {
				GdaCircle* s = (GdaCircle*) selectable_shps[i];
				if (s->isNull()) continue;
				bool contains = (radius + s->radius >=
								 GenUtils::distance(sel1, s->center));
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		} else if (brushtype == line) {
			wxRealPoint hp((sel1.x+sel2.x)/2.0, (sel1.y+sel2.y)/2.0);
			double hp_rad = GenUtils::distance(sel1, sel2)/2.0;
			for (int i=0; i<hl_size; i++) {
				GdaCircle* s = (GdaCircle*) selectable_shps[i];
				if (s->isNull()) continue;
				bool contains = ((GenUtils::pointToLineDist(s->center,
														   sel1, sel2) <=
								 s->radius) &&
								 (GenUtils::distance(hp, s->center) <=
								  hp_rad + s->radius));
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	LOG_MSG("Exiting TemplateCanvas::UpdateSelectionCircles");	
}

// The following function assumes that the set of selectable objects
// being selected against are all GdaPolyLine objects.
void TemplateCanvas::UpdateSelectionPolylines(bool shiftdown, bool pointsel)
{
	LOG_MSG("Entering TemplateCanvas::UpdateSelectionPolylines");
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	GdaPolyLine* p;
	if (pointsel) { // a point selection
		double radius = 3.0;
		wxRealPoint hp;
		double hp_rad;
		for (int i=0; i<hl_size; i++) {
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
					nuh[total_newly_unselected++] = i;
				} else {
					nh[total_newly_selected++] = i; 
				}
			} else {
				if (!shiftdown && hs[i]) {
					nuh[total_newly_unselected++] = i;
				}
			}
		}
	} else { // determine which obs intersect the selection region.
		if (brushtype == rectangle) {
			wxPoint lleft; // lower left corner of rect
			wxPoint uright; // upper right corner of rect
			wxPoint uleft; // upper right corner
			wxPoint lright;  // lower right corner
			GenUtils::StandardizeRect(sel1, sel2, lleft, uright);
			uleft.x = lleft.x;
			uleft.y = uright.y;
			lright.x = uright.x;
			lright.y = lleft.y;
			for (int i=0; i<hl_size; i++) {
				p = (GdaPolyLine*) selectable_shps[i];
				if (p->isNull()) continue;
				bool contains = false;
				for (int j=0, its=p->n-1; j<its; j++) {
					if (GenUtils::LineSegsIntersect(p->points[j],
													p->points[j+1],
													lleft, uleft) ||
						GenUtils::LineSegsIntersect(p->points[j],
													p->points[j+1],
													uleft, uright) ||
						GenUtils::LineSegsIntersect(p->points[j],
													p->points[j+1],
													uright, lright) ||
						GenUtils::LineSegsIntersect(p->points[j],
													p->points[j+1],
													lright, lleft))
					{
						contains = true;
						break;
					}
				}
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		} else if (brushtype == line) {
			for (int i=0; i<hl_size; i++) {
				p = (GdaPolyLine*) selectable_shps[i];
				if (p->isNull()) continue;
				bool contains = false;
				for (int j=0, its=p->n-1; j<its; j++) {
					if (GenUtils::LineSegsIntersect(p->points[j],
													p->points[j+1],
													sel1, sel2))
					{
						contains = true;
						break;
					}
				}
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}	
		} else if (brushtype == circle) {
			double radius = GenUtils::distance(sel1, sel2);
			wxRealPoint hp;
			double hp_rad;
			for (int i=0; i<hl_size; i++) {
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
				if (!shiftdown) {
					if (contains) {
						if (!hs[i]) nh[total_newly_selected++] = i;
					} else {
						if (hs[i]) nuh[total_newly_unselected++] = i;
					}
				} else { // do not unhighlight if not in intersection region
					if (contains && !hs[i]) {
						nh[total_newly_selected++] = i;
					}
				}
			}
		}
	}
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	LOG_MSG("Exiting TemplateCanvas::UpdateSelectionPolylines");
}

void TemplateCanvas::SelectAllInCategory(int category,
										 bool add_to_selection)
{
	LOG_MSG("Entering TemplateCanvas::SelectAllInCategory");
	int cc_ts = cat_data.curr_canvas_tm_step;
	if (category < 0 && category >= cat_data.GetNumCategories(cc_ts)) {
		return;
	}	
	int hl_size = highlight_state->GetHighlightSize();
	if (hl_size != selectable_shps.size()) return;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	std::vector<bool> obs_in_cat(hl_size, false);
	std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, category);

	for (int i=0, iend=ids.size(); i<iend; i++) obs_in_cat[ids[i]] = true;
	
	for (int i=0; i<hl_size; i++) {
		if (!add_to_selection && hs[i] && !obs_in_cat[i]) {
			nuh[total_newly_unselected++] = i;
		}
		if (!hs[i] && obs_in_cat[i]) {
			nh[total_newly_selected++] = i;
		}
	}
	
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	LOG_MSG("Exiting TemplateCanvas::SelectAllInCategory");	
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
	LOG_MSG("Entering TemplateCanvas::NotifyObservables");
	
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
		highlight_state->SetEventType(HighlightState::unhighlight_all);
		highlight_state->notifyObservers();
	} else {
		highlight_state->SetEventType(HighlightState::delta);
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		highlight_state->notifyObservers();
	}
	LOG_MSG("Exiting TemplateCanvas::NotifyObservables");
}

void TemplateCanvas::DetermineMouseHoverObjects()
{
	total_hover_obs = 0;
	int total_obs = highlight_state->GetHighlightSize();
	if (selectable_shps.size() != total_obs) return;
	if (selectable_shps_type == circles) {
		// slightly faster than GdaCircle::pointWithin
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
			GdaCircle* s = (GdaCircle*) selectable_shps[i];
			if (s->isNull()) continue;
			if (GenUtils::distance_sqrd(s->center, sel1) <=
				s->radius*s->radius) {
				hover_obs[total_hover_obs++] = i;
			}			
		}
	} else if (selectable_shps_type == polygons ||
			   selectable_shps_type == polylines)
	{
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
			if (selectable_shps[i]->isNull()) continue;
			if (selectable_shps[i]->pointWithin(sel1)) {
				hover_obs[total_hover_obs++] = i;
			}
		}
	} else { // selectable_shps_type == points or anything without pointWithin
		const double r2 = GdaConst::my_point_click_radius;
		for (int i=0; i<total_obs && total_hover_obs<max_hover_obs; i++) {
			if (selectable_shps[i]->isNull()) continue;
			if (GenUtils::distance_sqrd(selectable_shps[i]->center, sel1)
				<= 16.5) {
				hover_obs[total_hover_obs++] = i;
			}
		}
	}
}

void TemplateCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
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
									const wxString& field_default)
{
	std::vector<SaveToTableEntry> data(1);
	
	int cc_ts = cat_data.curr_canvas_tm_step;
	int num_cats = cat_data.GetNumCategories(cc_ts);
	std::vector<wxInt64> dt(num_obs);
	
	data[0].type = GdaConst::long64_type;
	data[0].l_val = &dt;
	data[0].label = label;
	data[0].field_default = field_default;
	
	for (int cat=0; cat<num_cats; cat++) {
		std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
		for (int i=0, iend=ids.size(); i<iend; i++) dt[ids[i]] = cat+1;
	}
	
	SaveToTableDlg dlg(template_frame->GetProject(),
					   this, data, title, wxDefaultPosition, wxSize(400,400));
    dlg.ShowModal();
}

