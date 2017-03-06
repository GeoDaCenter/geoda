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

#ifndef __GEODA_CENTER_LINE_CHART_CANVAS_H__
#define __GEODA_CENTER_LINE_CHART_CANVAS_H__

#include <vector>
#include <wx/gbsizer.h>
#include <wx/menu.h>
#include <wx/webview.h>
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../GdaShape.h"
#include "LineChartStats.h"

class Project;
typedef std::vector<double> vec_dbl_type;
typedef std::vector<vec_dbl_type> vec_vec_dbl_type;

class LineChartCanvas : public TemplateCanvas
{
	DECLARE_CLASS(LineChartCanvas)
    LineChartCanvas(wxWindow *parent, TemplateFrame* t_frame,
                    Project* project,
                    const LineChartStats& lcs,
                    LineChartCanvasCallbackInt* lc_canv_cb = 0,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);
	virtual ~LineChartCanvas();
	
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void UpdateSelection(bool shiftdown = false,
                                 bool pointsel = false);
	virtual void UpdateStatusBar();
	
	virtual void UpdateAll();
    
    void UpdateYAxis(wxString y_min="", wxString y_max="");
    
    void UpdateYAxisPrecision(int precision_s);
    
    double GetYAxisMinVal() {return axis_scale_y.scale_min;}
    double GetYAxisMaxVal() {return axis_scale_y.scale_max;}
    
protected:
    void OnDblClick(wxMouseEvent& event);
    
	virtual void PopulateCanvas();
	void UpdateMargins();
	GdaCircle* MakeSummAvgHelper(double y_avg,
                                 const wxColour& fg_clr,
                                 const wxColour& bg_clr = wxTransparentColor);
	
	const LineChartStats& lcs;
	LineChartCanvasCallbackInt* lc_canv_cb;
	
	AxisScale axis_scale_y;
	double scaleY;
    int y_axis_precision;
    
    double y_axis_min;
    double y_axis_max;
    
    wxString def_y_min;
    wxString def_y_max;
	
	std::vector<GdaCircle*> comb_circs;
	std::vector<GdaCircle*> sel_circs;
	std::vector<GdaCircle*> excl_circs;
	std::vector<GdaRectangle*> tm_rects;
	std::vector<GdaCircle*> summ_avg_circs; // size 4
	
	static const double circ_rad;
	static const double ss_circ_rad;
	static const double ray_len;
	
	DECLARE_EVENT_TABLE()
};


#endif
