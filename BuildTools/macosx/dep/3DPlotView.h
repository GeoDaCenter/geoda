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

#ifndef __GEODA_CENTER_3D_PLOT_VIEW_H__
#define __GEODA_CENTER_3D_PLOT_VIEW_H__

#include <wx/glcanvas.h>
#include "../FramesManagerObserver.h"
#include "../HLStateInt.h"
#include "../HighlightStateObserver.h"
#include "../VarTools.h"
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"

class Arcball;
class C3DControlPan;
class C3DPlotFrame;
class TableInterface;

typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class C3DPlotCanvas: public wxGLCanvas, public HighlightStateObserver
{
public:
	C3DPlotCanvas(Project* project, C3DPlotFrame* t_frame,
				  HLStateInt* highlight_state,
				  const std::vector<GdaVarTools::VarInfo>& var_info,
				  const std::vector<int>& col_ids,
				  wxWindow *parent,
				  const wxWindowID id = wxID_ANY,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = 0);
	virtual ~C3DPlotCanvas();
	
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	virtual void TimeChange();
	void VarInfoAttributeChange();
	void UpdateScaledData();
	virtual void TimeSyncVariableToggle(int var_index);
	
	void SetCurrentTmStep(int t) { local_time_step = t; }
	int GetCurrentTmStep() { return local_time_step; }
	int local_time_step; // valid range always starts at 0?
	
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
	void OnKeyEvent(wxKeyEvent& ev);
    void OnMouse(wxMouseEvent& event);
	void UpdateSelect();
	void SelectByRect();
    void InitGL(void);
   
	bool isInit;

	float m_ClearColorBlue;
	float m_ClearColorGreen;
	float m_ClearColorRed;
	
	int selection_mode;
	int last[2];
	bool m_bRButton;
	bool m_bLButton;
	int height, width;
	double bb_min[3], bb_max[3];

	wxColour selectable_fill_color;
	wxColour highlight_color;
	wxColour canvas_background_color;
	virtual void SetSelectableFillColor(wxColour color);
	virtual void SetHighlightColor(wxColour color);
	virtual void SetCanvasBackgroundColor(wxColour color);
	
	Arcball* ball;
	double xs, xp, ys, yp, zs, zp;

	bool ShowFirst;
	bool m_d;
	bool m_z;
	bool m_y;
	bool m_x;
	bool b_select;
	bool m_brush;
	void RenderScene();
	void apply_camera();
	void end_redraw();
	void begin_redraw();
	
	/** Implementation of the HighlightStateObserver interface
	 update function. */
	virtual void update(HLStateInt* o);
	TableInterface* table_int;
	Project* project;
	HLStateInt* highlight_state;
	int num_obs;
	int num_vars;
	int num_time_vals;
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	std::vector<d_array_type> data;
	std::vector<b_array_type> data_undef;
    std::vector<bool> all_undefs;
    
	std::vector<d_array_type> scaled_d;
	std::vector< std::vector<SampleStatistics> > data_stats;
	std::vector<double> var_min; // min over time
	std::vector<double> var_max; // max over time
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	
	wxPoint select_start;
	wxPoint select_end;
	bool bSelect;
	C3DControlPan *m_dlg;
	C3DPlotFrame* c3d_plot_frame;
	wxGLContext*	m_context;
	
	DECLARE_EVENT_TABLE()
};


class C3DPlotFrame: public TemplateFrame
{
public:
	C3DPlotFrame(wxFrame *parent, Project* project,
				 const std::vector<GdaVarTools::VarInfo>& var_info,
				 const std::vector<int>& col_ids,
				 const wxString& title, const wxPoint& pos,
				 const wxSize& size, const long style);
	virtual ~C3DPlotFrame();	
    
	wxSplitterWindow* m_splitter;
	C3DPlotCanvas* canvas;
	C3DControlPan* control;

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
	virtual void OnCanvasBackgroundColor(wxCommandEvent& event);
	virtual void OnSelectableFillColor(wxCommandEvent& event);
	virtual void OnHighlightColor(wxCommandEvent& event);
	virtual void OnTimeSyncVariable(int var_index);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	virtual void UpdateTitle();
	
	DECLARE_EVENT_TABLE()
};




#endif

