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

#include <utility> // std::pair
#include <list>
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include "../GenGeomAlgs.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../GdaConst.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/TimeChooserDlg.h"
#include "../DialogTools/VarGroupingEditorDlg.h"

#include "../FramesManager.h"
#include "../FramesManagerObserver.h"
#include "CatClassification.h"
#include "LineChartCanvas.h"


IMPLEMENT_CLASS(LineChartCanvas, TemplateCanvas)

BEGIN_EVENT_TABLE(LineChartCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const double LineChartCanvas::circ_rad = 2.5;
const double LineChartCanvas::ss_circ_rad = 6.0;
const double LineChartCanvas::ray_len = 10.0;

LineChartCanvas::LineChartCanvas(wxWindow *parent, TemplateFrame* t_frame,
                                 Project* project, const LineChartStats& lcs_,
                                 LineChartCanvasCallbackInt* lc_canv_cb_,
                                 const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, t_frame, project, project->GetHighlightState(), pos,
                 size, false, true),
lcs(lcs_), lc_canv_cb(lc_canv_cb_), summ_avg_circs(4, (GdaCircle*) 0),
y_axis_precision(1)
{
	LOG_MSG("Entering LineChartCanvas::LineChartCanvas");
    last_scale_trans.SetFixedAspectRatio(false);
    last_scale_trans.SetData(0, 0, 100, 100);
	UpdateMargins();
	
	use_category_brushes = false;
	
	PopulateCanvas();
	ResizeSelectableShps();
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
    
    Bind(wxEVT_LEFT_DCLICK, &LineChartCanvas::OnDblClick, this);
    
	LOG_MSG("Exiting LineChartCanvas::LineChartCanvas");
}

LineChartCanvas::~LineChartCanvas()
{
	LOG_MSG("In LineChartCanvas::LineChartCanvas");
}

void LineChartCanvas::OnDblClick(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    
    wxSize size(GetVirtualSize());
    int win_width = size.GetWidth();
    int win_height = size.GetHeight();
    size_t tms = tm_rects.size();
    for (size_t t=0; t<tms; ++t) {
        GdaRectangle r(*((GdaRectangle*) tm_rects[t]));
        if (r.lower_left.y - pos.y < 40 && r.lower_left.y - pos.y > -40) {
            bool opened = false;
            wxPoint pt;
            FramesManager* fm = project->GetFramesManager();
            std::list<FramesManagerObserver*> observers(fm->getCopyObservers());
            std::list<FramesManagerObserver*>::iterator it;
            for (it=observers.begin(); it != observers.end(); ++it) {
                if (TimeChooserDlg* w = dynamic_cast<TimeChooserDlg*>(*it)) {
                    w->Show(true);
                    w->Maximize(false);
                    w->Raise();
                    pt = w->GetPosition();
                    opened = true;
                }
            }
            if (!opened) {
                TimeChooserDlg* dlg = new TimeChooserDlg(0, project->GetFramesManager(),
                                                         project->GetTimeState(),
                                                         project->GetTableState(),
                                                         project->GetTableInt());
                dlg->Show(true);
                pt = dlg->GetPosition();
            }
            
            opened = false;
            for (it=observers.begin(); it != observers.end(); ++it) {
                if (VarGroupingEditorDlg* w = dynamic_cast<VarGroupingEditorDlg*>(*it))
                {
                    w->Show(true);
                    w->Maximize(false);
                    w->Raise();
                    w->SetPosition(wxPoint(pt.x, pt.y + 130));
                    opened =true;
                    break;
                }
            }
            if (!opened) {
                VarGroupingEditorDlg* dlg = new VarGroupingEditorDlg(project, this);
                dlg->Show(true);
                int start_x = pt.x - 200;
                if (start_x) start_x = 0;
                dlg->SetPosition(wxPoint(pt.x, pt.y + 130));
            }
            return;
        }
    }
}

/** Note: some of these right-click popup items need to be called with
 a delayed callback.  See notes on LineChartEventDelay class. */
void LineChartCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering LineChartCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	template_frame->OnActivate(ae);
	
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->LoadMenu("ID_LINE_CHART_MENU_OPTIONS");
	
	template_frame->UpdateContextMenuItems(optMenu);
    template_frame->PopupMenu(optMenu);//, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting LineChartCanvas::DisplayRightClickMenu");
}

// The following function assumes that the set of selectable objects
// are all rectangles.
void LineChartCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
    is_showing_brush = false;
	bool rect_sel = (!pointsel && (brushtype == rectangle));
		
    /*
	wxPoint lower_left;
	wxPoint upper_right;
	if (rect_sel) {
		GenGeomAlgs::StandardizeRect(sel1, sel2, lower_left, upper_right);
	}
	
	size_t tms = tm_rects.size();
	std::vector<bool> new_sel_tms(tms, false);
	
    for (size_t t=0; t<tms; ++t) {
		// slightly increase the target area for selection by a fixed
		// number of pixels.
		GdaRectangle r(*((GdaRectangle*) tm_rects[t]));
		r.lower_left += wxPoint(t==0 ? -30 : 0,   40);
		r.upper_right += wxPoint(t+1==tms ? 30 : 0,   -30);
		bool selected = ((pointsel && r.pointWithin(sel1)) || (rect_sel && GenGeomAlgs::RectsIntersect(r.lower_left,	r.upper_right, lower_left, upper_right)));
		new_sel_tms[t] = selected;
	}
	
	if (lc_canv_cb) {
		lc_canv_cb->notifyNewSelection(new_sel_tms, shiftdown, pointsel);
	}
     */
}

void LineChartCanvas::UpdateStatusBar()
{
	if (mousemode == select && selectstate == start) {
		wxString s;
		if (summ_avg_circs[0]) {
			GdaCircle c(*summ_avg_circs[0]);
			c.center.x += c.getXNudge();
			c.radius = 6;
			if (c.pointWithin(prev)) {
				s << "Sample 1 mean=";
				if (lcs.compare_regimes || lcs.compare_r_and_t) {
					s << lcs.Y_sel_tm0_avg;
				} else {
					s << lcs.Y_avg_tm0;
				}
			}
		}
		if (summ_avg_circs[1]) {
			GdaCircle c(*summ_avg_circs[1]);
			c.radius = 6;
			c.center.x += c.getXNudge();
			if (c.pointWithin(prev)) {
				if (!s.IsEmpty()) s << ", ";
				s << "Sample 2 mean=";
				if (lcs.compare_regimes || lcs.compare_r_and_t) {
					s << lcs.Y_excl_tm0_avg;
				} else {
					s << lcs.Y_avg_tm1;
				}
			}
		}
		if (summ_avg_circs[2]) {
			GdaCircle c(*summ_avg_circs[2]);
			c.radius = 6;
			c.center.x += c.getXNudge();
			if (c.pointWithin(prev)) {
				if (!s.IsEmpty()) s << ", ";
				s << "Sample 3 mean="<< lcs.Y_sel_tm1_avg;
			}
		}
		if (summ_avg_circs[3]) {
			GdaCircle c(*summ_avg_circs[3]);
			c.radius = 6;
			c.center.x += c.getXNudge();
			if (c.pointWithin(prev)) {
				if (!s.IsEmpty()) s << ", ";
				s << "Sample 4 mean=" << lcs.Y_excl_tm1_avg;
			}
		}
		if (!s.IsEmpty()) {
			wxString m;
			m << lcs.Yname << ", " << s;
			if (lc_canv_cb) lc_canv_cb->notifyNewHoverMsg(m);
			return;
		}
		
		TableInterface* table_int = project->GetTableInt();
		// s is empty
		bool time_inv = lcs.Y.size() <= 1;
		for (size_t t=0, tms=comb_circs.size(); t<tms; ++t) {
			GdaCircle c(*comb_circs[t]);
			c.radius = 6;
			if (c.pointWithin(prev)) {
				if (!s.IsEmpty()) s << ", ";
				if (!time_inv) s << table_int->GetTimeString(t) << " ";
				s << "all obs mean=" << lcs.Y_avg[t];
			}
		}
		for (size_t t=0, tms=sel_circs.size(); t<tms; ++t) {
			GdaCircle c(*sel_circs[t]);
			c.radius = 6;
			if (c.pointWithin(prev)) {
                if (lcs.sel_sz_i >=0 ) {
                    if (!s.IsEmpty()) s << ", ";
                    if (!time_inv) s << table_int->GetTimeString(t) << " ";
                
                    s << "selected obs mean= " << lcs.Y_sel_avg[t];
                }
			}
		}
		for (size_t t=0, tms=excl_circs.size(); t<tms; ++t) {
			GdaCircle c(*excl_circs[t]);
			c.radius = 6;
			if (c.pointWithin(prev)) {
                if (lcs.sel_sz_i > 0) {
                    if (!s.IsEmpty()) s << ", ";
                    if (!time_inv) s << table_int->GetTimeString(t) << " ";
                
                    s << "excluded obs mean=" << lcs.Y_excl_avg[t];
                }
			}
		}
		if (!s.IsEmpty()) {
			wxString m;
			m << lcs.Yname << ", " << s;
			if (lc_canv_cb) lc_canv_cb->notifyNewHoverMsg(m);
			return;
		}
		
	}
	if (lc_canv_cb) lc_canv_cb->notifyNewHoverMsg("");
}

void LineChartCanvas::UpdateAll()
{
	invalidateBms();
	UpdateMargins();
	PopulateCanvas();
	Refresh();
}

void LineChartCanvas::UpdateYAxis(wxString y_min, wxString y_max)
{
    def_y_min = y_min;
    def_y_max = y_max;
}

void LineChartCanvas::UpdateYAxisPrecision(int precision_s)
{
    y_axis_precision = precision_s;
}

void LineChartCanvas::PopulateCanvas()
{
	LOG_MSG("Entering LineChartCanvas::PopulateCanvas");
	BOOST_FOREACH( GdaShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( GdaShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( GdaShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	tm_rects.clear(); // NOTE: tm_rects are owned by background_shps
	summ_avg_circs[0] = 0; // NOTE: summ_avg_circs contents are owned
	summ_avg_circs[1] = 0; //       by background_shps.
	summ_avg_circs[2] = 0;
	summ_avg_circs[3] = 0;
	comb_circs.clear();
	sel_circs.clear();
	excl_circs.clear();
	
	wxSize size(GetVirtualSize());
	int win_width = size.GetWidth();
	int win_height = size.GetHeight();
    last_scale_trans.SetView(win_width, win_height);
	
	
	double y_min = 0;
	double y_max = 100;
	if (lcs.Y_avg_valid) {
		y_min = lcs.Y_avg_min;
		y_max = lcs.Y_avg_max;
	}
	if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_sel_avg_valid) {
        if (lcs.Y_sel_avg_min < y_min) {
            y_min = lcs.Y_sel_avg_min;
        }
        if (lcs.Y_sel_avg_max > y_max) {
            y_max = lcs.Y_sel_avg_max;
        }
	}
	if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_excl_avg_valid) {
        if (lcs.Y_excl_avg_min < y_min) {
            y_min = lcs.Y_excl_avg_min;
        }
        if (lcs.Y_excl_avg_max > y_max) {
            y_max = lcs.Y_excl_avg_max;
        }
	}
	double y_pad = 0.1 * (y_max - y_min);
	double axis_min = y_min - y_pad;
	double axis_max = y_max + y_pad;
    
	if (y_min >= 0 && axis_min < 0)
        axis_min = 0;
    
    if (!def_y_min.IsEmpty())
          def_y_min.ToDouble(&axis_min);
    
    if (!def_y_max.IsEmpty())
          def_y_max.ToDouble(&axis_max);

	axis_scale_y = AxisScale(axis_min, axis_max, 4, y_axis_precision);
	
	//LOG_MSG(wxString(axis_scale_y.ToString().c_str(), wxConvUTF8));
	scaleY = 100.0 / (axis_scale_y.scale_range);
	
	TableInterface* table_int = project->GetTableInt();
	// create axes
	std::vector<wxString> tm_strs;
	table_int->GetTimeStrings(tm_strs);
	bool time_variant = lcs.Y.size() > 1;
	
	wxRealPoint* y_pts = 0;
	size_t num_points = lcs.Y_avg.size();
	
	if (time_variant && num_points > 0) {
		// draw summary average circs
		// Will define avg point 1 and avg point 2 depending on
		// state of lcs.  Will ultimately need these points to
		// be detectable objects so that we can report summaries.
		// NULL indicates avg is not currently defined.
		
		if (lcs.compare_regimes) {
			if (lcs.Y_sel_tm0_avg_valid) {
				summ_avg_circs[0] =  MakeSummAvgHelper(lcs.Y_sel_tm0_avg,GdaConst::ln_cht_clr_sel_dark);
			}
			if (lcs.Y_excl_tm0_avg_valid) {
				summ_avg_circs[1] = MakeSummAvgHelper(lcs.Y_excl_tm0_avg,GdaConst::ln_cht_clr_exl_dark);
			}
		} else if (lcs.compare_time_periods) {
			if (lcs.Y_avg_tm0_valid) {
				summ_avg_circs[0] = MakeSummAvgHelper(lcs.Y_avg_tm0,GdaConst::ln_cht_clr_tm1_dark);
			}
			if (lcs.Y_avg_tm1_valid) {
				summ_avg_circs[1] = MakeSummAvgHelper(lcs.Y_avg_tm1,GdaConst::ln_cht_clr_tm2_dark);
			}
		} else if (lcs.compare_r_and_t) {
			if (lcs.Y_sel_tm0_avg_valid) {
				summ_avg_circs[0] =  MakeSummAvgHelper(lcs.Y_sel_tm0_avg,GdaConst::ln_cht_clr_sel_dark,GdaConst::ln_cht_clr_tm1_light);
			}
			if (lcs.Y_excl_tm0_avg_valid) {
				summ_avg_circs[1] = MakeSummAvgHelper(lcs.Y_excl_tm0_avg,GdaConst::ln_cht_clr_exl_dark,GdaConst::ln_cht_clr_tm1_light);
			}
			if (lcs.Y_sel_tm1_avg_valid) {
				summ_avg_circs[2] =  MakeSummAvgHelper(lcs.Y_sel_tm1_avg,GdaConst::ln_cht_clr_sel_dark,GdaConst::ln_cht_clr_tm2_light);
			}
			if (lcs.Y_excl_tm1_avg_valid) {
				summ_avg_circs[3] = MakeSummAvgHelper(lcs.Y_excl_tm1_avg,GdaConst::ln_cht_clr_exl_dark,GdaConst::ln_cht_clr_tm2_light);
			}
		}
	}
	
	if (time_variant && num_points > 0) {
		size_t tms = lcs.Y_avg.size();
		y_pts = new wxRealPoint[num_points];
		
		
		// Draw subset highlight line and invisible selection
		// rectangles first
		if (lcs.Y_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;
				double y = (lcs.Y_avg[t] - axis_scale_y.scale_min) * scaleY;
				
				// draw a rectangle behind x-axis line to indicate
				// time period 0 or 1.
				double x0 = 0.0;
				double x1 = 100.0;
				double x_p1 = ((double) (t+1))/((double) (tms-1)) * 100.0;
				double x_m1 = ((double) (t-1))/((double) (tms-1)) * 100.0;
				if (t == 0) {
					x1 = (x+x_p1)/2.0;
				} else if (t+1 == tms) {
					x0 = (x_m1+x)/2.0;
				} else { // not an end-point
					x0 = (x_m1+x)/2.0;
					x1 = (x+x_p1)/2.0;
				}
				if (lcs.tms_subset0[t]) {
					GdaPolyLine* p = new GdaPolyLine(x0, 0, x1, 0);
					if (lcs.compare_time_periods || lcs.compare_r_and_t) {
						p->setPen(wxPen(GdaConst::ln_cht_clr_tm1_light, 9));
					} else {
						p->setPen(wxPen(GdaConst::ln_cht_clr_regimes_hl, 9));
					}
					p->setNudge(0, 5);
					foreground_shps.push_back(p);
				}
				if ((lcs.compare_time_periods || lcs.compare_r_and_t)
						&& lcs.tms_subset1[t]) {
					GdaPolyLine* p = new GdaPolyLine(x0, 0, x1, 0);
					p->setPen(wxPen(GdaConst::ln_cht_clr_tm2_light, 9));
					p->setNudge(0, 5);
					foreground_shps.push_back(p);
				}
      
				// Create invisible selection rectangles
				{
					double x0_nudge = 0;
					double x1_nudge = 0;
					if (t == 0) {
						x0_nudge = -5;
					} else if (t+1 == tms) {
						x1_nudge = 5;
					}
					GdaRectangle* r = new GdaRectangle(wxRealPoint(x0, 0), wxRealPoint(x1, 100));
					tm_rects.push_back(r);
					r->setPen(*wxTRANSPARENT_PEN);
					r->setBrush(*wxTRANSPARENT_BRUSH);
					foreground_shps.push_back(r);
				}
			}
		}
		
		
		// Push subset circle highlights to the background first
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_excl_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;
				double y = (lcs.Y_excl_avg[t] - axis_scale_y.scale_min) * scaleY;
				if (lcs.tms_subset0[t] && lcs.Y_excl_tm0_avg_valid) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					if (lcs.compare_r_and_t) {
						c->setPen(GdaConst::ln_cht_clr_tm1_light);
						c->setBrush(GdaConst::ln_cht_clr_tm1_light);
					} else {
						c->setPen(GdaConst::ln_cht_clr_regimes_hl);
						c->setBrush(GdaConst::ln_cht_clr_regimes_hl);
					}
					foreground_shps.push_back(c);
				}
				if (lcs.compare_r_and_t && lcs.tms_subset1[t] &&lcs.Y_excl_tm1_avg_valid ) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					c->setPen(GdaConst::ln_cht_clr_tm2_light);
					c->setBrush(GdaConst::ln_cht_clr_tm2_light);
					foreground_shps.push_back(c);
				}
			}
		}
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_sel_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;	
				double y = (lcs.Y_sel_avg[t] - axis_scale_y.scale_min) * scaleY;
				if (lcs.tms_subset0[t] && lcs.Y_sel_tm0_avg_valid) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					if (lcs.compare_r_and_t) {
						c->setPen(GdaConst::ln_cht_clr_tm1_light);
						c->setBrush(GdaConst::ln_cht_clr_tm1_light);
					} else {
						c->setPen(GdaConst::ln_cht_clr_regimes_hl);
						c->setBrush(GdaConst::ln_cht_clr_regimes_hl);
					}
					foreground_shps.push_back(c);
				}
				if (lcs.compare_r_and_t && lcs.tms_subset1[t] && lcs.Y_sel_tm1_avg_valid) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					c->setPen(GdaConst::ln_cht_clr_tm2_light);
					c->setBrush(GdaConst::ln_cht_clr_tm2_light);
					foreground_shps.push_back(c);
				}
			}
		}
        /*
		if (lcs.Y_avg_valid && lcs.Y_excl_avg_valid == lcs.Y_sel_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;
				double y = (lcs.Y_avg[t] - axis_scale_y.scale_min) * scaleY;
				if (lcs.tms_subset0[t]) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					if (lcs.compare_time_periods || lcs.compare_r_and_t) {
						c->setPen(GdaConst::ln_cht_clr_tm1_light);
						c->setBrush(GdaConst::ln_cht_clr_tm1_light);
					} else {
						c->setPen(GdaConst::ln_cht_clr_regimes_hl);
						c->setBrush(GdaConst::ln_cht_clr_regimes_hl);
					}
					foreground_shps.push_back(c);
				}
				if ((lcs.compare_time_periods || lcs.compare_r_and_t)
						&& lcs.tms_subset1[t]) {
					GdaCircle* c = new GdaCircle(wxRealPoint(x, y), ss_circ_rad);
					c->setPen(GdaConst::ln_cht_clr_tm2_light);
					c->setBrush(GdaConst::ln_cht_clr_tm2_light);
					foreground_shps.push_back(c);
				}
			}
		}
         */
		// Draw everything else
        if (lcs.Y_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;
				double y = (lcs.Y_avg[t] - axis_scale_y.scale_min) * scaleY;
				y_pts[t].x = x;
				y_pts[t].y = y;
			}
			GdaPolyLine* p = new GdaPolyLine(num_points, y_pts);
            p->setPen(wxPen(*wxBLACK, 1, wxSHORT_DASH));
			foreground_shps.push_back(p);
			for (size_t t=0; t<tms; ++t) {
				GdaCircle* c = new GdaCircle(wxRealPoint(y_pts[t].x, y_pts[t].y), circ_rad);
				wxColour lc = *wxBLACK;
				wxColour dc = GdaColorUtils::ChangeBrightness(lc);
				c->setPen(lc);
				c->setBrush(dc);
				foreground_shps.push_back(c);
				comb_circs.push_back(c);
			}
		}
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_excl_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;
				double y = (lcs.Y_excl_avg[t] - axis_scale_y.scale_min) * scaleY;
				y_pts[t].x = x;
				y_pts[t].y = y;
			}
			GdaPolyLine* p = new GdaPolyLine(num_points, y_pts);
			p->setPen(GdaConst::ln_cht_clr_exl_dark);
			foreground_shps.push_back(p);
			for (size_t t=0; t<tms; ++t) {
				GdaCircle* c = new GdaCircle(wxRealPoint(y_pts[t].x, y_pts[t].y),
																		 circ_rad);
				wxColour lc = GdaConst::ln_cht_clr_exl_dark;
				wxColour dc = GdaColorUtils::ChangeBrightness(lc);
				c->setPen(lc);
				c->setBrush(dc);
				foreground_shps.push_back(c);
				excl_circs.push_back(c);
			}
		}
		
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_sel_avg_valid) {
			for (size_t t=0; t<tms; ++t) {
				double fracX = ((double) t)/((double) (tms-1));
				double x = fracX * 100.0;	
				double y = (lcs.Y_sel_avg[t] - axis_scale_y.scale_min) * scaleY;
				y_pts[t].x = x;
				y_pts[t].y = y;
			}
			GdaPolyLine* p = new GdaPolyLine(num_points, y_pts);
			p->setPen(GdaConst::ln_cht_clr_sel_dark);
			foreground_shps.push_back(p);
			for (size_t t=0; t<tms; ++t) {
				GdaCircle* c = new GdaCircle(wxRealPoint(y_pts[t].x, y_pts[t].y), circ_rad);
				wxColour lc = GdaConst::ln_cht_clr_sel_dark;
				wxColour dc = GdaColorUtils::ChangeBrightness(lc);
				c->setPen(lc);
				c->setBrush(dc);
				foreground_shps.push_back(c);
				sel_circs.push_back(c);
			}
		}
	}
	
	if (!time_variant) {
		size_t t = 0;
		const double d=5.0;
		const double x = 50.0;
        
		if (lcs.Y_avg_valid && lcs.Y_excl_avg_valid == lcs.Y_sel_avg_valid) {
			double y = (lcs.Y_avg[t] - axis_scale_y.scale_min) * scaleY;
			GdaPolyLine* p = new GdaPolyLine(x-d, y, x+d, y);
			p->setPen(*wxBLACK_PEN);
			foreground_shps.push_back(p);
			GdaCircle* c = new GdaCircle(wxRealPoint(x,y), circ_rad);
			wxColour lc = *wxBLACK;
			wxColour dc = GdaColorUtils::ChangeBrightness(lc);
			c->setPen(lc);
			c->setBrush(dc);
			foreground_shps.push_back(c);
			comb_circs.push_back(c);
		}
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_excl_avg_valid) {
			double y = (lcs.Y_excl_avg[t] - axis_scale_y.scale_min) * scaleY;
			GdaPolyLine* p = new GdaPolyLine(x-d, y, x+d, y);
			p->setPen(GdaConst::ln_cht_clr_exl_dark);
			foreground_shps.push_back(p);
			GdaCircle* c = new GdaCircle(wxRealPoint(x,y), circ_rad);
			wxColour lc = GdaConst::ln_cht_clr_exl_dark;
			wxColour dc = GdaColorUtils::ChangeBrightness(lc);
			c->setPen(lc);
			c->setBrush(dc);
			foreground_shps.push_back(c);
			excl_circs.push_back(c);
		}
		if ((lcs.compare_regimes || lcs.compare_r_and_t) && lcs.Y_sel_avg_valid) {
			double y = (lcs.Y_sel_avg[t] - axis_scale_y.scale_min) * scaleY;
			GdaPolyLine* p = new GdaPolyLine(x-d, y, x+d, y);
			p->setPen(GdaConst::ln_cht_clr_sel_dark);
			foreground_shps.push_back(p);
			GdaCircle* c = new GdaCircle(wxRealPoint(x,y), circ_rad);
			wxColour lc = GdaConst::ln_cht_clr_sel_dark;
			wxColour dc = GdaColorUtils::ChangeBrightness(lc);
			c->setPen(lc);
			c->setBrush(dc);
			foreground_shps.push_back(c);
			sel_circs.push_back(c);
		}
	}

	GdaAxis* x_baseline = 0;
	if (time_variant) {
		x_baseline = new GdaAxis("", tm_strs, wxRealPoint(0,0), wxRealPoint(100, 0), 0, 5);
		x_baseline->hideCaption(true);
		x_baseline->setPen(*GdaConst::scatterplot_scale_pen);
		x_baseline->autoDropScaleValues(true);
		x_baseline->moveOuterValTextInwards(false);
		foreground_shps.push_back(x_baseline);
	}
	GdaAxis* y_baseline = new GdaAxis(lcs.Yname, axis_scale_y, wxRealPoint(0,0), wxRealPoint(0, 100), -5, 0);
	y_baseline->autoDropScaleValues(true);
	y_baseline->moveOuterValTextInwards(true);
	y_baseline->setPen(*GdaConst::scatterplot_scale_pen);
	foreground_shps.push_back(y_baseline);	
	
	if (y_pts) delete [] y_pts;
	
	ResizeSelectableShps();
    Refresh(false);
	LOG_MSG("Exiting LineChartCanvas::PopulateCanvas");
}

void LineChartCanvas::UpdateMargins()
{
    last_scale_trans.SetMargin(10, 40, 50, 50);
}

/** bg_clr is optional and is transparent by default */
GdaCircle* LineChartCanvas::MakeSummAvgHelper(double y_avg, const wxColour& fg_clr, const wxColour& bg_clr)
{
	const double x = 100;
	const int x_nudge = 40;
	double y = (y_avg - axis_scale_y.scale_min) * scaleY;
	GdaRay* r0 = new GdaRay(wxRealPoint(x, y), 0, ray_len);
	GdaRay* r1 = new GdaRay(wxRealPoint(x, y), 180, ray_len);
	r0->setPen(fg_clr);
	r1->setPen(fg_clr);
	r0->setNudge(x_nudge, 0);
	r1->setNudge(x_nudge, 0);
	foreground_shps.push_back(r0);
	foreground_shps.push_back(r1);
	if (bg_clr != wxTransparentColor) {
		GdaCircle* bc = new GdaCircle(wxRealPoint(x,y), ss_circ_rad);
		bc->setNudge(x_nudge, 0);
		bc->setPen(bg_clr);
		bc->setBrush(bg_clr);
		foreground_shps.push_back(bc);
	}
	GdaCircle* c = new GdaCircle(wxRealPoint(x,y), circ_rad);
	c->setNudge(x_nudge, 0);
	c->setPen(fg_clr);
	c->setBrush(GdaColorUtils::ChangeBrightness(fg_clr));
	foreground_shps.push_back(c);
	return c;
}
