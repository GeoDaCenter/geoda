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

#ifndef __GEODA_CENTER_CONDITIONAL_BOX_PLOT_VIEW_H__
#define __GEODA_CENTER_CONDITIONAL_BOX_PLOT_VIEW_H__

#include "../GenUtils.h"
#include "ConditionalNewView.h"

class ConditionalBoxPlotFrame;
class ConditionalBoxPlotCanvas;

class ConditionalBoxPlotCanvas : public ConditionalNewCanvas {
	DECLARE_CLASS(ConditionalBoxPlotCanvas)
public:
	
    ConditionalBoxPlotCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project,
                             const std::vector<GdaVarTools::VarInfo>& var_info,
                             const std::vector<int>& col_ids,
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize);
	virtual ~ConditionalBoxPlotCanvas();

	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	virtual wxString GetVariableNames();
	virtual void SetCheckMarks(wxMenu* menu);
    virtual void update(HLStateInt* o);
    virtual void TimeChange();
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
    //virtual void UpdateSelection(bool shiftdown = false,
    //                             bool pointsel = false);

	virtual void TimeSyncVariableToggle(int var_index);

	/// Override from TemplateCanvas
	virtual void SetSelectableFillColor(wxColour color);
	/// Override from TemplateCanvas
	virtual void SetSelectableOutlineColor(wxColour color);
	
	virtual void UpdateStatusBar();

    virtual void UserChangedCellCategories();

    void OnShowAxes(wxCommandEvent& evt);

    void OnHinge15(wxCommandEvent& evt);

    void OnHinge30(wxCommandEvent& evt);
    
protected:
    virtual void PopulateCanvas();
    
    void InitBoxPlot();

    static const int BOX_VAR; // box variable
    static const double left_pad_const;

    std::vector<HingeStats> hinge_stats;
    std::vector<SampleStatistics> data_stats;
    std::vector<std::vector<Gda::dbl_int_pair_type> > cell_data;
    std::vector<std::vector<Gda::dbl_int_pair_type> > data_valid;
    std::vector<std::vector<Gda::dbl_int_pair_type> > data_sorted;
    int max_plots; // min of time_steps and MAX_BOX_PLOTS
    // 1 <= cur_num_plots <= max_plots <= MAX_BOX_PLOTS
    int cur_num_plots; // number of plots actually shown

    bool hinge_15;
    bool show_axes;

    AxisScale axis_scale_y;
    GdaAxis* y_axis;

	DECLARE_EVENT_TABLE()
};


class ConditionalBoxPlotFrame : public ConditionalNewFrame
{
	DECLARE_CLASS(ConditionalBoxPlotFrame)
public:
    ConditionalBoxPlotFrame(wxFrame *parent, Project* project,
						const std::vector<GdaVarTools::VarInfo>& var_info,
						const std::vector<int>& col_ids,
						const wxString& title = _("Conditional Box Plot"),
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConditionalBoxPlotFrame();
	
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);

    virtual void OnSaveCanvasImageAs(wxCommandEvent& event);

    void OnShowAxes(wxCommandEvent& evt);
    void OnHinge15(wxCommandEvent& evt);
    void OnHinge30(wxCommandEvent& evt);

    DECLARE_EVENT_TABLE()
};

#endif
