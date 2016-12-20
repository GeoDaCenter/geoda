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

#include <algorithm> // std::sort
#include <cfloat>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include "../DialogTools/NumCategoriesDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../logger.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "BoxNewPlotView.h"

IMPLEMENT_CLASS(BoxPlotCanvas, TemplateCanvas)
BEGIN_EVENT_TABLE(BoxPlotCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int BoxPlotCanvas::MAX_BOX_PLOTS = 100;
const double BoxPlotCanvas::plot_height_const = 100;
const double BoxPlotCanvas::left_pad_const = 0;
const double BoxPlotCanvas::right_pad_const = 0;
const double BoxPlotCanvas::plot_width_const = 10;
const double BoxPlotCanvas::plot_gap_const = 20;

/** Box Plot view is somewhat different than other views because support
 the display of multiple box plots on the screen at the same time.  For
 this reason, data_sorted and hinge_stats will store data for all
 time periods when possible and will not resize when global time
 synch is turned off.  Any class than inherits from this class, such
 as Lisa Box Plot, will have to override this behaviour if they want
 to support multiple variables */

BoxPlotCanvas::BoxPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project_s,
                             const std::vector<GdaVarTools::VarInfo>& v_info,
                             const std::vector<int>& col_ids,
                             const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project_s, project_s->GetHighlightState(),
								 pos, size, false, true),
var_info(v_info), num_obs(project_s->GetNumRecords()),
num_time_vals(1), data(v_info.size()), data_undef(v_info.size()),
vert_axis(0), display_stats(false), show_axes(true),
hinge_15(true)
{
	using namespace Shapefile;
	wxLogMessage("Open BoxPlotCanvas.");
    
	TableInterface* table_int = project->GetTableInt();
    
	table_int->GetColData(col_ids[0], data[0]);
	bool has_undef = table_int->GetColUndefined(col_ids[0], data_undef[0]);
	int data0_times = data[0].shape()[0];
    
	sel_scratch.resize(num_obs);
	hinge_stats.resize(data0_times);
	data_stats.resize(data0_times);
	data_sorted.resize(data0_times);
    
	for (int t=0; t<data0_times; t++) {
        data_sorted[t].resize(num_obs);
        std::vector<Gda::dbl_int_pair_type> data_valid;
        std::vector<bool> undefs(num_obs, false);
        
		for (int i=0; i<num_obs; i++) {
            double val = data[0][t][i];
            data_sorted[t][i] = std::make_pair(val,i);
            if (has_undef && data_undef[0][t][i] == false) {
                data_valid.push_back(std::make_pair(val, i));
            }
            undefs[i] = undefs[i] || data_undef[0][t][i];
		}
		std::sort(data_sorted[t].begin(),
                  data_sorted[t].end(),
				  Gda::dbl_int_pair_cmp_less);
        
        if (has_undef) {
            std::sort(data_valid.begin(),
                      data_valid.end(),
                      Gda::dbl_int_pair_cmp_less);
            hinge_stats[t].CalculateHingeStats(data_sorted[t], undefs);
            data_stats[t].CalculateFromSample(data_valid, undefs);
            
        } else {
    		hinge_stats[t].CalculateHingeStats(data_sorted[t]);
    		data_stats[t].CalculateFromSample(data_sorted[t], undefs);
        }
	}

	template_frame->ClearAllGroupDependencies();
	for (int i=0, sz=var_info.size(); i<sz; ++i) {
		template_frame->AddGroupDependancy(var_info[i].name);
	}	

    // no more than 100 plots
	max_plots = GenUtils::min<int>(MAX_BOX_PLOTS, var_info[0].is_time_variant ?
								   project->GetTableInt()->GetTimeSteps() : 1);
	cur_num_plots = max_plots;
	cur_first_ind = var_info[0].time_min;
	cur_last_ind = var_info[0].time_max;
	
	// NOTE: define Box Plot defaults
	selectable_fill_color = GdaConst::boxplot_point_color;
	highlight_color = GdaConst::highlight_color;

    last_scale_trans.SetFixedAspectRatio(false);
	use_category_brushes = false;
	selectable_shps_type = mixed;
	
	ref_var_index = var_info[0].is_time_variant ? 0 : -1;
	VarInfoAttributeChange();
	PopulateCanvas();
	DisplayStatistics(true);
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
}

BoxPlotCanvas::~BoxPlotCanvas()
{
	wxLogMessage("Close BoxPlotCanvas.");
	highlight_state->removeObserver(this);
}

void BoxPlotCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((BoxPlotFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_BOX_NEW_PLOT_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
}

void BoxPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!var_info[0].is_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	{
		wxString s;
		s << "Synchronize " << var_info[0].name << " with Time Control";
		wxMenuItem* mi =
		menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+0, s, s);
		mi->Check(var_info[0].sync_with_global_time);
	}
	
	wxMenu* menu2 = new wxMenu(wxEmptyString);
	{
		wxString s;
		s << "Fixed scale over time";
		wxMenuItem* mi =
		menu2->AppendCheckItem(GdaConst::ID_FIX_SCALE_OVER_TIME_VAR1, s, s);
		mi->Check(var_info[0].fixed_scale);
	}
	
	wxMenu* menu3 = new wxMenu(wxEmptyString);
	{
		int mppv = GdaConst::max_plots_per_view_menu_items;
		int mp = GenUtils::min<int>(max_plots, mppv);
		for (int i=0; i<mp-1; i++) {
			wxString s;
			s << i+1;
			wxMenuItem* mi =
			menu3->AppendCheckItem(GdaConst::ID_PLOTS_PER_VIEW_1+i, s, s);
			mi->Check(i+1 == cur_num_plots);
		}
		if (max_plots > 10) {
			wxString s;
			s << "Other...";
			wxMenuItem* mi =
			menu3->AppendCheckItem(GdaConst::ID_PLOTS_PER_VIEW_OTHER, s, s);
		}
		wxString s;
		if (project->GetTableInt()->GetTimeSteps() <= MAX_BOX_PLOTS) {
			s << "All";
		} else {
			s << MAX_BOX_PLOTS << " (max plots allowed)";
		}
		wxMenuItem* mi =
		menu3->AppendCheckItem(GdaConst::ID_PLOTS_PER_VIEW_ALL, s, s);
		mi->Check(cur_num_plots == max_plots);
	}
	
	menu->Prepend(wxID_ANY, "Number of Box Plots", menu3,
				  "Number of Box Plots");
	menu->Prepend(wxID_ANY, "Scale Options", menu2, "Scale Options");
    
    
    menu->AppendSeparator();
    menu->Append(wxID_ANY, "Time Variable Options", menu1,
				  "Time Variable Options");
}

void BoxPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATISTICS"),
								  IsDisplayStats());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SHOW_AXES"),
								  IsShowAxes());
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_15"),
								  hinge_15);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_OPTIONS_HINGE_30"),
								  !hinge_15);
	
	if (var_info[0].is_time_variant) {
		GeneralWxUtils::CheckMenuItem(menu,
									  GdaConst::ID_TIME_SYNC_VAR1,
									  var_info[0].sync_with_global_time);
		GeneralWxUtils::CheckMenuItem(menu,
									  GdaConst::ID_FIX_SCALE_OVER_TIME_VAR1,
									  var_info[0].fixed_scale);
		int mppv = GdaConst::max_plots_per_view_menu_items;
		int mp = GenUtils::min<int>(max_plots, mppv);
		for (int i=0; i<mp-1; i++) {
			GeneralWxUtils::CheckMenuItem(menu,
										  GdaConst::ID_PLOTS_PER_VIEW_1+i,
										  i+1 == cur_num_plots);
		}
		GeneralWxUtils::CheckMenuItem(menu,
									  GdaConst::ID_PLOTS_PER_VIEW_ALL,
									  cur_num_plots == max_plots);
	}
}

void BoxPlotCanvas::DetermineMouseHoverObjects(wxPoint pt)
{
	total_hover_obs = 0;
	const double r2 = GdaConst::my_point_click_radius;
	
	for (int i=0; i<num_obs; i++)
        sel_scratch[i] = false;
    
	for (int t=0; t<cur_num_plots; t++) {
		for (int i=0; i<num_obs; i++) {
            wxPoint& pt0 = selectable_shps[t*num_obs + i]->center;
            wxPoint& pt1 = pt;
			sel_scratch[i] = sel_scratch[i] ||
				GenUtils::distance_sqrd(pt0, pt1) <= 16.5;
		}
	}
	for (int i=0; i<num_obs && total_hover_obs<max_hover_obs; i++) {
		if (sel_scratch[i])
            hover_obs[total_hover_obs++] = i;
	}
}

// The following function assumes that the set of selectable objects
// being selected against are all points.  Since all GdaShape objects
// define a center point, this is also the default function for
// all GdaShape selectable objects.
void BoxPlotCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	for (int i=0; i<num_obs; i++)
        sel_scratch[i] = false;
	
	if (pointsel) { // a point selection
		for (int t=0; t<cur_num_plots; t++) {
			for (int i=0; i<num_obs; i++) {
				sel_scratch[i] = sel_scratch[i] ||
				selectable_shps[t*num_obs + i]->pointWithin(sel1);
			}
		}
	} else {
		// assume brush is a rectangle
		wxRegion rect(wxRect(sel1, sel2));
		for (int t=0; t<cur_num_plots; t++) {
			for (int i=0; i<num_obs; i++) {
				sel_scratch[i] = sel_scratch[i] ||
					(rect.Contains(selectable_shps[t*num_obs + i]->center) !=
					 wxOutRegion);
			}
		}
	}
	for (int i=0; i<num_obs; i++) {
		if (!shiftdown) {
			if (sel_scratch[i]) {
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
			if (sel_scratch[i] && !hs[i]) {
                hs[i] = true;
                selection_changed = true;
			}
		}
	}
   
	if ( selection_changed ) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers(this);
        
        // re-paint highlight layer (layer1_bm)
        layer1_valid = false;
        DrawLayers();
        Refresh();
        
        UpdateStatusBar();
    }
}

void BoxPlotCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	int radius = GdaConst::my_point_click_radius;
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		int min_IQR = hinge_stats[t].min_IQR_ind;
		int max_IQR = hinge_stats[t].max_IQR_ind;
		int ind_base = (t-cur_first_ind)*num_obs;
		dc.SetPen(selectable_fill_color);
		dc.SetBrush(*wxWHITE_BRUSH);
        
		for (int i=0; i<min_IQR; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
			int ind = ind_base + idx;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		for (int i=max_IQR+1; i<num_obs; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
            int ind = ind_base + idx;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		int iqr_s = GenUtils::max<double>(min_IQR, 0);
		int iqr_t = GenUtils::min<double>(max_IQR, num_obs-1);
		dc.SetPen(GdaConst::boxplot_q1q2q3_color);
		dc.SetBrush(GdaConst::boxplot_q1q2q3_color);
		for (int i=iqr_s; i<=iqr_t; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
            int ind = ind_base + idx;
			GdaRectangle* rec = (GdaRectangle*) selectable_shps[ind];
			dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
							 rec->upper_right.x - rec->lower_left.x,
							 rec->upper_right.y - rec->lower_left.y);
		}
	}	
}

void BoxPlotCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	int radius = 3;
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	dc.SetBrush(highlight_color);
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		int min_IQR = hinge_stats[t].min_IQR_ind;
		int max_IQR = hinge_stats[t].max_IQR_ind;
		int ind_base = (t-cur_first_ind)*num_obs;
		dc.SetPen(selectable_fill_color);
		dc.SetBrush(*wxWHITE_BRUSH);
		for (int i=0; i<min_IQR; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
            int ind = ind_base + idx;
            
			if (!hs[idx]) continue;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		for (int i=max_IQR+1; i<num_obs; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
            int ind = ind_base + idx;
            
            if (!hs[idx]) continue;
			dc.DrawCircle(selectable_shps[ind]->center, radius);
		}
		int iqr_s = GenUtils::max<double>(min_IQR, 0);
		int iqr_t = GenUtils::min<double>(max_IQR, num_obs-1);
		dc.SetPen(GdaConst::boxplot_q1q2q3_color);
		dc.SetBrush(GdaConst::boxplot_q1q2q3_color);
		for (int i=iqr_s; i<=iqr_t; i++) {
            int idx = data_sorted[t][i].second;
            if (data_undef[0][t][idx])
                continue;
            int ind = ind_base + idx;
            
            if (!hs[idx]) continue;
            
			GdaRectangle* rec = (GdaRectangle*) selectable_shps[ind];
			dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
							 rec->upper_right.x - rec->lower_left.x,
							 rec->upper_right.y - rec->lower_left.y);
		}
	}
}

/** Override of TemplateCanvas method. */
void BoxPlotCanvas::update(HLStateInt* o)
{
    TemplateCanvas::update(o);
}

wxString BoxPlotCanvas::GetCanvasTitle()
{
	wxString s("Box Plot (Hinge=");
	if (hinge_15) s << "1.5): ";
	else s << "3.0): ";
	if (cur_first_ind == cur_last_ind) {
		s << GetNameWithTime(0);
	} else {
		s << var_info[0].name << " (";
		s << project->GetTableInt()->GetTimeString(cur_first_ind) << "-";
		s << project->GetTableInt()->GetTimeString(cur_last_ind) << ")";
	}
	return s;
}

wxString BoxPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

wxString  BoxPlotCanvas::GetNameWithTime(int var, int time)
{
	if (var < 0 || var >= var_info.size() || 
		time < 0 || time >= project->GetTableInt()->GetTimeSteps()) {
		return wxEmptyString;
	}
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(time);
		s << ")";
	}
	return s;
}

wxString  BoxPlotCanvas::GetTimeString(int var, int time)
{
	if (var < 0 || var >= var_info.size() || 
		time < 0 || time >= project->GetTableInt()->GetTimeSteps() ||
		!var_info[var].is_time_variant) {
		return wxEmptyString;
	}
	return project->GetTableInt()->GetTimeString(time);
}

void BoxPlotCanvas::PopulateCanvas()
{
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
		+ plot_width_const * cur_num_plots + 
		+ plot_gap_const * (cur_num_plots-1);

    last_scale_trans.SetData(x_min, 0, x_max, 100);
	
	GdaShape* s = 0;
	int table_w=0, table_h=0;
	if (display_stats) {
		int cols = 1;
		int rows = 8;
		std::vector<wxString> vals(rows);
		vals[0] << "min";
		vals[1] << "max";
		vals[2] << "Q1";
		vals[3] << "median";
		vals[4] << "Q3";
		vals[5] << "IQR";
		vals[6] << "mean";
		vals[7] << "s.d.";
		std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
		s = new GdaShapeTable(vals, attribs, rows, cols, *GdaConst::small_font,
                              wxRealPoint(0, 0), GdaShapeText::h_center,
                              GdaShapeText::top, GdaShapeText::right,
                              GdaShapeText::v_center, 3, 10, -45, 30);
		foreground_shps.push_back(s);
		wxClientDC dc(this);
		((GdaShapeTable*) s)->GetSize(dc, table_w, table_h);
	}
	

    last_scale_trans.SetMargin(25,
                               display_stats ? 42+table_h : 35,
                               (display_stats || show_axes) ? 25+50 : 25,
                               25);
	wxSize size(GetVirtualSize());
    last_scale_trans.SetView(size.GetWidth(), size.GetHeight());
    
	
	double y_min = hinge_stats[cur_first_ind].extreme_lower_val_15;
	double y_max = hinge_stats[cur_first_ind].extreme_upper_val_15;
	int tstart = cur_first_ind;
	int tfinish = cur_last_ind;
	if (var_info[0].is_time_variant && var_info[0].sync_with_global_time
		&& var_info[0].fixed_scale) {
		tstart = 0;
		tfinish = hinge_stats.size()-1;
	}
	for (int t=tstart; t<=tfinish; t++) {
		double ext_upper = (hinge_15 ? hinge_stats[t].extreme_upper_val_15 :
							hinge_stats[t].extreme_upper_val_30);
		double ext_lower = (hinge_15 ? hinge_stats[t].extreme_lower_val_15 :
							hinge_stats[t].extreme_lower_val_30);
		if (ext_upper > y_max) y_max = ext_upper;
		if (ext_lower < y_min) y_min = ext_lower;
		if (var_info[0].max[t] > y_max) y_max = var_info[0].max[t];
		if (var_info[0].min[t] < y_min) y_min = var_info[0].min[t];
	}
	if (show_axes) {
		axis_scale = AxisScale(y_min, y_max, 5, axis_display_precision);
		y_min = axis_scale.scale_min;
		y_max = axis_scale.scale_max;
		vert_axis = new GdaAxis(var_info[0].name, axis_scale,
							   wxRealPoint(0,0), wxRealPoint(0, 100), -20, 0);
		foreground_shps.push_back(vert_axis);
	}
	// need to scale height data so that y_min and y_max are between 0 and 100
	double scaleY = 100.0 / (y_max-y_min);
	
	std::vector<double> orig_x_pos(cur_num_plots);
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		orig_x_pos[t-cur_first_ind] = left_pad_const + plot_width_const/2.0
		+ (t-cur_first_ind) * (plot_width_const + plot_gap_const);
	}
	
	selectable_shps.resize(num_obs * cur_num_plots);
    selectable_shps_undefs.resize(num_obs * cur_num_plots);
    
	for (int t=cur_first_ind; t<=cur_last_ind; t++) {
		double xM = orig_x_pos[t-cur_first_ind];
		double x0r = xM - plot_width_const/2.2;
		double x1r = xM + plot_width_const/2.2;
		double x0 = xM - plot_width_const/2.0;
		double y0 = (hinge_15 ? hinge_stats[t].extreme_lower_val_15 :
					 hinge_stats[t].extreme_lower_val_30);
		double x1 = xM + plot_width_const/2.0;
		double y1 = (hinge_15 ? hinge_stats[t].extreme_upper_val_15 :
					 hinge_stats[t].extreme_upper_val_30);
		
		if (display_stats) {
			// min max Q1 Q2 Q3 IQR mean sd skewness? kurtosis?
			// idea: could show a parallel real-time boxplot of
			// subset selection?  And could also show exluded as well then
			// show real-time statistics too.
			// want to show curve fitting.
			int cols = 1;
			int rows = 8;
			std::vector<wxString> vals(rows);
			vals[0] << GenUtils::DblToStr(hinge_stats[t].min_val, 4);
			vals[1] << GenUtils::DblToStr(hinge_stats[t].max_val, 4);
			vals[2] << GenUtils::DblToStr(hinge_stats[t].Q1, 4);
			vals[3] << GenUtils::DblToStr(hinge_stats[t].Q2, 4);
			vals[4] << GenUtils::DblToStr(hinge_stats[t].Q3, 4);
			vals[5] << GenUtils::DblToStr(hinge_stats[t].IQR, 4);
			vals[6] << GenUtils::DblToStr(data_stats[t].mean, 4);
			vals[7] << GenUtils::DblToStr(data_stats[t].sd_with_bessel, 4);

			std::vector<GdaShapeTable::CellAttrib> attribs(0); // undefined
            s = new GdaShapeTable(vals, attribs, rows, cols,
                                  *GdaConst::small_font, wxRealPoint(xM, 0),
                                  GdaShapeText::h_center, GdaShapeText::top,
                                  GdaShapeText::h_center, GdaShapeText::v_center,
                                  3, 10, 0, 30);
			foreground_shps.push_back(s);
		}
		
		s = new GdaPolyLine(xM-plot_width_const/3.0, (y0-y_min)*scaleY,
						   xM+plot_width_const/3.0, (y0-y_min)*scaleY);
		foreground_shps.push_back(s);
		s = new GdaPolyLine(xM-plot_width_const/3.0, (y1-y_min)*scaleY,
						   xM+plot_width_const/3.0, (y1-y_min)*scaleY);
		foreground_shps.push_back(s);
		s = new GdaPolyLine(orig_x_pos[t-cur_first_ind], (y0-y_min)*scaleY,
						   orig_x_pos[t-cur_first_ind], (y1-y_min)*scaleY);
		foreground_shps.push_back(s);
        
		s = new GdaCircle(wxRealPoint(xM, (data_stats[t].mean-y_min)*scaleY),
						 5.0);
		s->setPen(selectable_fill_color);
		s->setBrush(GdaConst::boxplot_mean_point_color);
		foreground_shps.push_back(s);
		double y0m = (hinge_stats[t].Q2-y_min)*scaleY - 0.2;
		double y1m = (hinge_stats[t].Q2-y_min)*scaleY + 0.2;
		s = new GdaRectangle(wxRealPoint(x0, y0m), wxRealPoint(x1, y1m));
		s->setPen(GdaConst::boxplot_median_color);
		s->setBrush(GdaConst::boxplot_median_color);
		foreground_shps.push_back(s);
		
		wxString plot_label(var_info[0].is_time_variant ?
							GetTimeString(0, t) : GetNameWithTime(0));
		s = new GdaShapeText(plot_label, *GdaConst::small_font,
					   wxRealPoint(orig_x_pos[t-cur_first_ind], 0), 0,
					   GdaShapeText::h_center, GdaShapeText::v_center, 0, 18);
		foreground_shps.push_back(s);
		
		for (int i=0; i<hinge_stats[t].min_IQR_ind; i++) {
			double val = data_sorted[t][i].first;
            int idx = data_sorted[t][i].second;
            int ind = (t-cur_first_ind)*num_obs + idx;
            
			selectable_shps[ind] =
				new GdaPoint(orig_x_pos[t-cur_first_ind], (val-y_min) * scaleY);
			selectable_shps[ind]->setPen(selectable_fill_color);
			selectable_shps[ind]->setBrush(*wxWHITE_BRUSH);
            
            selectable_shps_undefs[ind] = data_undef[0][t][idx];
		}
		for (int i=hinge_stats[t].max_IQR_ind+1; i<num_obs; i++) {
            double val = data_sorted[t][i].first;
            int idx = data_sorted[t][i].second;
            int ind = (t-cur_first_ind)*num_obs + idx;
            
			selectable_shps[ind] =
				new GdaPoint(orig_x_pos[t-cur_first_ind], (val-y_min) * scaleY);
			selectable_shps[ind]->setPen(selectable_fill_color);
			selectable_shps[ind]->setBrush(*wxWHITE_BRUSH);
            
            selectable_shps_undefs[ind] = data_undef[0][t][idx];
		}
		if (hinge_stats[t].min_IQR_ind == hinge_stats[t].max_IQR_ind) {
            int idx = data_sorted[t][hinge_stats[t].min_IQR_ind].second;
            int ind = (t-cur_first_ind)*num_obs + idx;
                       
			double y0 = (hinge_stats[t].Q1 - y_min) * scaleY;
			double y1 = (hinge_stats[t].Q3 - y_min) * scaleY;
			selectable_shps[ind] = new GdaRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			selectable_shps[ind]->setPen(GdaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GdaConst::boxplot_q1q2q3_color);
            
            selectable_shps_undefs[ind] = data_undef[0][t][idx];
            
		} else {
			int minIQR = hinge_stats[t].min_IQR_ind;
			int maxIQR = hinge_stats[t].max_IQR_ind;
            int idx = data_sorted[t][minIQR].second;
            
            int ind = (t-cur_first_ind)*num_obs + idx;
			double y0 = (hinge_stats[t].Q1 - y_min) * scaleY;
			double y1;
			if (minIQR > -1) {
				y1 = (((data_sorted[t][minIQR].first +
						data_sorted[t][minIQR+1].first)/2.0) - y_min)*scaleY;
			} else {
				y1 = (data_sorted[t][minIQR+1].first - y_min)*scaleY;
			}
			selectable_shps[ind] = new GdaRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			selectable_shps[ind]->setPen(GdaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GdaConst::boxplot_q1q2q3_color);
            selectable_shps_undefs[ind] = data_undef[0][t][idx];
           
            idx = data_sorted[t][maxIQR].second;
			ind = (t-cur_first_ind)*num_obs + idx;
            
			if (maxIQR < num_obs) {
				y0 = (((data_sorted[t][maxIQR].first +
						data_sorted[t][maxIQR-1].first)/2.0) - y_min)*scaleY;
			} else {
				y0 = (data_sorted[t][maxIQR-1].first - y_min)*scaleY;
			}
			y1 = (hinge_stats[t].Q3 - y_min) * scaleY;
			selectable_shps[ind] = new GdaRectangle(wxRealPoint(x0r, y0),
												   wxRealPoint(x1r, y1));
			selectable_shps[ind]->setPen(GdaConst::boxplot_q1q2q3_color);
			selectable_shps[ind]->setBrush(GdaConst::boxplot_q1q2q3_color);
            selectable_shps_undefs[ind] = data_undef[0][t][idx];
			
			for (int i=minIQR+1; i<maxIQR; i++) {
                idx = data_sorted[t][i].second;
				ind = (t-cur_first_ind)*num_obs + idx;
                
				y0 = (((data_sorted[t][i].first +
						data_sorted[t][i-1].first)/2.0) - y_min)*scaleY;
				y1 = (((data_sorted[t][i].first +
						data_sorted[t][i+1].first)/2.0) - y_min)*scaleY;
				selectable_shps[ind] = new GdaRectangle(wxRealPoint(x0r, y0),
													   wxRealPoint(x1r, y1));
				selectable_shps[ind]->setPen(GdaConst::boxplot_q1q2q3_color);
				selectable_shps[ind]->setBrush(GdaConst::boxplot_q1q2q3_color);
                selectable_shps_undefs[ind] = data_undef[0][t][idx];
			}
		}
	}
	
	ResizeSelectableShps();
}

void BoxPlotCanvas::TimeChange()
{
	if (!is_any_sync_with_global_time) return;
	
	var_info[0].time = project->GetTimeState()->GetCurrTime();
	
	int time_steps = project->GetTableInt()->GetTimeSteps();
	int start = var_info[0].time - cur_num_plots/2;
	if (cur_num_plots % 2 == 0) start++;
	start = GenUtils::max(start, 0);
	start = GenUtils::min(start, time_steps-cur_num_plots);
	
	if (cur_first_ind == start) return;
	
	cur_first_ind = start;
	cur_last_ind = cur_first_ind + cur_num_plots - 1;

	invalidateBms();
	PopulateCanvas();
	Refresh();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void BoxPlotCanvas::VarInfoAttributeChange()
{
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	if (var_info[0].is_time_variant) is_any_time_variant = true;
	if (var_info[0].sync_with_global_time) {
		is_any_sync_with_global_time = true;
	}
	template_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	ref_var_index = -1;
	num_time_vals = 1;
	if (var_info[0].is_ref_variable) ref_var_index = 0;
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	//GdaVarTools::PrintVarInfoVector(var_info);
}

void BoxPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	PopulateCanvas();
}

void BoxPlotCanvas::FixedScaleVariableToggle(int var_index)
{
	var_info[var_index].fixed_scale = !var_info[var_index].fixed_scale;
	VarInfoAttributeChange();
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::PlotsPerView(int plots_per_view)
{
	if (plots_per_view == cur_num_plots) return;
	cur_num_plots = plots_per_view;
	int time_steps = project->GetTableInt()->GetTimeSteps();
	int start = var_info[0].time - cur_num_plots/2;
	if (cur_num_plots % 2 == 0) start++;
	start = GenUtils::max(start, 0);
	start = GenUtils::min(start, time_steps-cur_num_plots);
	cur_first_ind = start;
	cur_last_ind = cur_first_ind + cur_num_plots - 1;
	
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::PlotsPerViewOther()
{
	// ask user for custom number of plots.  This dialog only appears
	// when GetTimeSteps() > 10
	wxString title("Plots Per View"); 
	NumCategoriesDlg dlg(this, 1, max_plots, 11, title, "Plots Per View");
	dlg.SetTitle(title);
	dlg.ShowModal();
	PlotsPerView(dlg.GetNumCategories());
}

void BoxPlotCanvas::PlotsPerViewAll()
{
	PlotsPerView(max_plots);
}

void BoxPlotCanvas::DisplayStatistics(bool display_stats_s)
{
	display_stats = display_stats_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::ShowAxes(bool show_axes_s)
{
	if (show_axes == show_axes_s) return;
	show_axes = show_axes_s;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::Hinge15()
{
	if (hinge_15) return;
	hinge_15 = true;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::Hinge30()
{
	if (!hinge_15) return;
	hinge_15 = false;
	invalidateBms();
	PopulateCanvas();
	Refresh();
}

void BoxPlotCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
   
    TableInterface* table_int = project->GetTableInt();
   
    const std::vector<bool>& hl = highlight_state->GetHighlight();
    
	wxString s;
    if (highlight_state->GetTotalHighlighted()> 0) {
        int n_total_hl = highlight_state->GetTotalHighlighted();
		s << "#selected=" << n_total_hl << "  ";
        
        if (num_time_vals == 1) {
            int t = 0;
            int n_undefs = 0;
            for (int i=0; i<num_obs; i++) {
                if (data_undef[0][t][i] && hl[i]) {
                    n_undefs += 1;
                }
            }
            if (n_undefs> 0) {
                s << "(undefined:" << n_undefs << ") ";
            }
        } else {
            wxString str;
            for (int t=0; t<num_time_vals; t++) {
                int n_undefs = 0;
                for (int i=0; i<num_obs; i++) {
                    if (data_undef[0][t][i] && hl[i])
                        n_undefs += 1;
                }
                if (n_undefs > 0) {
                    wxString t_str = table_int->GetTimeString(t);
                    str << n_undefs << "@" << t_str <<" ";
                }
            }
            if (!str.IsEmpty()) {
                s << "(undefined:" << str << ")";
            }
        }
	}
    
	if (mousemode == select && selectstate == start) {
		if (total_hover_obs >= 1) {
			s << "hover obs " << hover_obs[0]+1;
		}
		if (total_hover_obs >= 2) {
			s << ", ";
			s << "obs " << hover_obs[1]+1;
		}
		if (total_hover_obs >= 3) {
			s << ", ";
			s << "obs " << hover_obs[2]+1;
		}
		if (total_hover_obs >= 4) {
			s << ", ...";
		}
	}
	sb->SetStatusText(s);
}

IMPLEMENT_CLASS(BoxPlotFrame, TemplateFrame)
	BEGIN_EVENT_TABLE(BoxPlotFrame, TemplateFrame)
	EVT_ACTIVATE(BoxPlotFrame::OnActivate)
END_EVENT_TABLE()

BoxPlotFrame::BoxPlotFrame(wxFrame *parent, Project* project,
								 const std::vector<GdaVarTools::VarInfo>& var_info,
								 const std::vector<int>& col_ids,
								 const wxString& title,
								 const wxPoint& pos,
								 const wxSize& size,
								 const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	int width, height;
	GetClientSize(&width, &height);
	
    template_canvas = new BoxPlotCanvas(this, this, project,
                                        var_info, col_ids,
                                        wxDefaultPosition,
                                        wxSize(width,height));
    
	template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
		
	Show(true);
}

BoxPlotFrame::~BoxPlotFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
}

void BoxPlotFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("BoxPlotFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void BoxPlotFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_BOX_NEW_PLOT_VIEW_MENU_OPTIONS");
	((BoxPlotCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((BoxPlotCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void BoxPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		((BoxPlotCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void BoxPlotFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

/** Implementation of TimeStateObserver interface */
void BoxPlotFrame::update(TimeState* o)
{
	template_canvas->TimeChange();
	UpdateTitle();
}

void BoxPlotFrame::OnShowAxes(wxCommandEvent& event)
{
	wxLogMessage("In BoxPlotFrame::OnShowAxes");
	BoxPlotCanvas* t = (BoxPlotCanvas*) template_canvas;
	t->ShowAxes(!t->IsShowAxes());
	UpdateOptionMenuItems();
}

void BoxPlotFrame::OnDisplayStatistics(wxCommandEvent& event)
{
	wxLogMessage("In BoxPlotFrame::OnDisplayStatistics");
	BoxPlotCanvas* t = (BoxPlotCanvas*) template_canvas;
	t->DisplayStatistics(!t->IsDisplayStats());
	UpdateOptionMenuItems();
}

void BoxPlotFrame::OnHinge15(wxCommandEvent& event)
{
    wxLogMessage("In BoxPlotFrame::OnHinge15");

	BoxPlotCanvas* t = (BoxPlotCanvas*) template_canvas;
	t->Hinge15();
	UpdateOptionMenuItems();
	UpdateTitle();
}

void BoxPlotFrame::OnHinge30(wxCommandEvent& event)
{
    wxLogMessage("In BoxPlotFrame::OnHinge30");

	BoxPlotCanvas* t = (BoxPlotCanvas*) template_canvas;
	t->Hinge30();
	UpdateOptionMenuItems();
	UpdateTitle();
}
