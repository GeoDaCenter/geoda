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

#ifndef __GEODA_CENTER_TEMPLATE_CANVAS_H__
#define __GEODA_CENTER_TEMPLATE_CANVAS_H__

#include <list>
#include <set> // for std::multiset template
#include <vector>
#include <map>
#include <boost/multi_array.hpp>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/overlay.h>
#include <wx/scrolwin.h>
#include <wx/string.h>
#include <wx/dcgraph.h>

#include "Explore/CatClassification.h"
#include "HLStateInt.h"
#include "HighlightStateObserver.h"
#include "GdaShape.h"
#include "GdaConst.h"

typedef boost::multi_array<GdaShape*, 2> shp_array_type;
typedef boost::multi_array<int, 2> i_array_type;

class CatClassifManager;
class Project;
class TemplateFrame;

/** TemplateCanvas is a base class that implements most of the
 functionality associated with selecting polygons.  It is the base
 class of all "views" in GeoDa such as Scatter Plots, Box Plots, etc.
 */

class TemplateCanvas : public wxScrolledWindow, public HighlightStateObserver
{
    DECLARE_CLASS(TemplateCanvas)
public:
    TemplateCanvas(wxWindow *parent,
                   TemplateFrame* template_frame,
                   Project* project,
                   HLStateInt* hl_state_int,
                   const wxPoint& pos,
                   const wxSize& size,
                   bool fixed_aspect_ratio_mode = false,
                   bool fit_to_window_mode = true,
                   bool enable_high_dpi_support = GdaConst::enable_high_dpi_support);
	virtual ~TemplateCanvas();

public:
	/** The mouse can be in one of three operational modes: select,
	 pan and zoom. */
	enum MouseMode { select, pan, zoom, zoomout };
	
	/** When in mouse is in the 'select' operational mode, the SelectState
	 enum describes the types of states it can be in.  Initially it is in the
	 start state. */
	enum SelectState { start, leftdown, dragging, brushing };
	
	/** The selection/brushing tool can be either a rectangle, line or a
	 circle. */
	enum BrushType { rectangle, line, circle, custom_select };

	/** The selection/brushing tool can be either a rectangle, line or a
	 circle. */
	enum ScrollBarMode { none, horiz_only, vert_only, horiz_and_vert };
	
	enum SelectableShpType { mixed, circles, points, rectangles, polygons,
        polylines };

	/** Colors */
	bool selectable_outline_visible;
	bool user_canvas_background_color;
	wxColour selectable_outline_color;
	wxColour selectable_fill_color;
	wxColour highlight_color;
	wxColour canvas_background_color;
    
	CatClassifData cat_data;
    bool useScientificNotation;
    int  category_disp_precision;
    bool category_disp_fixed_point;
    int  axis_display_precision;
    bool axis_display_fixed_point;

	virtual void SetSelectableOutlineVisible(bool visible);
	virtual bool IsSelectableOutlineVisible();
    
	virtual void SetBackgroundColorVisible(bool visible);
	virtual bool IsUserBackgroundColorVisible();
    
	virtual void SetSelectableOutlineColor(wxColour color);
	virtual void SetSelectableFillColor(wxColour color);
	virtual void SetHighlightColor(wxColour color);
	virtual void SetCanvasBackgroundColor(wxColour color);

	/** This is the implementation of the Observer interface update function.
	 It is called whenever the Observable's state has changed.  In this case,
	 the Observable is an instance of the HighlightState, which keeps track
	 of the hightlighted/selected state for every SHP file observation.
	 */
	virtual void update(HLStateInt* o);
    
	/** Returns a human-readable string of the values of many of the
	 internal state variables for the TemplateCanvas class instance.  This
	 is for debugging purposes. */
	virtual wxString GetCanvasStateString();

	void OnKeyEvent(wxKeyEvent& event);

	virtual void OnScrollChanged(wxScrollWinEvent& event);
    
	virtual void OnSize(wxSizeEvent& event);
    virtual void OnIdle(wxIdleEvent& event);
	
	/** Where all the drawing action happens.  Should do something similar
	 to the update() method. */
	virtual void OnPaint(wxPaintEvent& event);

	/** The function handles all mouse events. */
	virtual void OnMouseEvent(wxMouseEvent& event);
		
	/** Draw the outline of the current selection tool. */
	virtual void PaintSelectionOutline(wxMemoryDC& dc);
	
	/** This might go away since we have foreground_shps. */
	virtual void PaintControls(wxDC& dc);
	
	virtual void DisplayRightClickMenu(const wxPoint& pos);

	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void UpdateSelectionPoints(bool shiftdown = false,
									   bool pointsel = false);
	virtual void UpdateSelectionCircles(bool shiftdown = false,
										bool pointsel = false);
	virtual void UpdateSelectionPolylines(bool shiftdown = false,
										  bool pointsel = false);
	virtual void UpdateSelectRegion(bool translate = false,
									wxPoint diff = wxPoint(0,0) );
	/** Assumes selectable_shps.size() == num obs **/
	virtual void DetermineMouseHoverObjects(wxPoint pt);
	virtual void UpdateStatusBar();
	virtual wxString GetCanvasTitle();
	virtual void TimeChange();
	virtual void TimeSyncVariableToggle(int var_index) {}
	virtual void FixedScaleVariableToggle(int var_index) {}
	virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
									  int virtual_scrn_h = 0);
    virtual void ResetBrushing();
    virtual void ResetFadedLayer();
	virtual void ZoomShapes(bool is_zoomin = true);
	virtual void PanShapes();
	virtual void ResetShapes();
	virtual void ApplyLastResizeToShp(GdaShape* s) {
		s->applyScaleTrans(last_scale_trans); }
	virtual void SetBrushType(BrushType brush) { brushtype = brush; }
	virtual BrushType GetBrushType() { return brushtype; }
	
	virtual MouseMode GetMouseMode() { return mousemode; }
	/** Will set the mouse mode and change the mouse cursor */
	virtual void SetMouseMode(MouseMode mode);
	virtual bool GetFitToWindowMode();
	virtual void SetFitToWindowMode(bool mode);
	virtual bool GetFixedAspectRatioMode();
	virtual void SetFixedAspectRatioMode(bool mode);
	virtual void SetAxisDisplayPrecision(int n, bool fixed_point=false);

	/** convert mouse coordiante point to original observation-coordinate
	 points.  This is an inverse of the affine transformation that converts
	 data points to screen points. */
	virtual wxRealPoint MousePntToObsPnt(const wxPoint &pt);

	virtual wxString GetCategoriesTitle();
    virtual std::vector<wxString> SaveCategories(const wxString& title,
                                const wxString& label,
								const wxString& field_default,
                                std::vector<bool>& undefs);
	virtual void RenderToDC(wxDC &dc, int w, int h);
    virtual void RenderToSVG(wxDC &dc, int w, int h);

	virtual void DrawLayer0();
	virtual void DrawLayer1();
	virtual void DrawLayer2();
	virtual void DrawLayers();

    virtual wxBitmap* GetPrintLayer() { return layer2_bm; }
    // should be implemented by inherited classes for drawing on this canvas
	virtual void PopulateCanvas() = 0;
	// draw highlighted sel shapes
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	// draw unhighlighted sel shapes
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
    virtual void DrawSelectableShapes_dc(wxMemoryDC &dc,
                                         bool hl_only=false,
                                         bool revert=false);

    
    virtual wxString GetVariableNames() = 0;
    virtual SelectableShpType GetShapeType() {return selectable_shps_type;}
    virtual double GetPointRadius() { return point_radius; }
    virtual void SetPointRadius(double r);
    virtual int GetDisplayPrecision() { return display_precision;}
    virtual void SetDisplayPrecision(int prec, bool fixed_point=false);
    virtual bool GetDisplayFixedPoint() { return display_precision_fixed_point;}

    // The following 3 functions should be enought to create another
    // inherited class (maybe TemplateMultiCanvas : TemplateCanvas
    virtual void PlotsPerView(int plots_per_view) {}
    virtual void PlotsPerViewOther() {}
    virtual void PlotsPerViewAll() {}
    
    // helper functions
    void SetScientificNotation(bool flag);
    void SetCategoryDisplayPrecision(int prec, bool fixed_point=false);
    const wxBitmap* GetLayer0() { return layer0_bm; }
    const wxBitmap* GetLayer1() { return layer1_bm; }
    const wxBitmap* GetLayer2() { return layer2_bm; }
    void deleteLayerBms();
    void resizeLayerBms(int width, int height);
    void invalidateBms();
    void ReDraw();
    void CreateZValArrays(int num_canvas_tms, int num_obs);

    /** generic function to create and initialized th*e selectable_shps vector
     based on a passed-in Project pointer and given an initial canvas
     screen size. */
    static std::vector<int> CreateSelShpsFromProj(std::vector<GdaShape*>& selectable_shps,
                                                  Project* project);
    /** Select all observations in a given category for current
     canvas time step. Assumes selectable_shps.size() == num obs */
    void SelectAllInCategory(int category, bool add_to_selection);
    static void AppendCustomCategories(wxMenu* menu, CatClassifManager* ccm);
    void helper_PaintSelectionOutline(wxDC& dc);

    /** This function is needed to handle the Erase Background WX event.  It
     does nothing, since we handle the Paint event ourselves and draw the
     background and foreground ourselves. */
    void OnEraseBackground(wxEraseEvent& event);
    /** This function handles possible WX Mouse Capture Lost events. */
    void OnMouseCaptureLostEvent(wxMouseCaptureLostEvent& event);

    void helper_DrawSelectableShapes_dc(wxDC &dc,
                                        vector<bool>& hs,
                                        bool hl_only=false,
                                        bool revert=false,
                                        bool crosshatch= false,
                                        bool is_print = false,
                                        const wxColour& fixed_pen_color = *wxWHITE);
    void helper_DrawSelectableShapes_gc(wxGraphicsContext &gc,
                                        vector<bool>& hs,
                                        bool hl_only=false,
                                        bool revert=false,
                                        bool crosshatch= false,
                                        int alpha=255);
    void DrawPoints(wxGCDC& dc, CatClassifData& cat_data, vector<bool>& hs,
                    double radius,
                    int alpha = 255,
                    wxColour fixed_pen_color = *wxWHITE,
                    bool cross_hatch = false);
    void DrawPolygons(wxGCDC& dc, CatClassifData& cat_data, vector<bool>& hs,
                      int alpha = 255,
                      wxColour fixed_pen_color = *wxWHITE,
                      bool cross_hatch = false);
    void DrawCircles(wxGCDC& dc, CatClassifData& cat_data, vector<bool>& hs,
                     int alpha = 255,
                     wxColour fixed_pen_color = *wxWHITE,
                     bool cross_hatch = false);
    void DrawLines(wxGCDC& dc, CatClassifData& cat_data, vector<bool>& hs,
                   int alpha = 255,
                   wxColour fixed_pen_color = *wxWHITE,
                   bool cross_hatch = false);

    // following 2 functions are deprecated
	void GetVizInfo(std::map<wxString, std::vector<int> >& colors);
    void GetVizInfo(wxString& shape_type, std::vector<wxString>& clrs, std::vector<double>& bins);
    int GetMarginLeft() { return last_scale_trans.slack_x;}
    int GetMarginRight() { return last_scale_trans.slack_x;}
    int GetMarginTop() { return last_scale_trans.slack_y;}
    int GetMarginBottom() { return last_scale_trans.slack_y;}
    wxSize GetDrawingSize() {
        wxSize sz(last_scale_trans.screen_width -  last_scale_trans.slack_x * 2,
                  last_scale_trans.screen_height -  last_scale_trans.slack_y * 2);
        return sz;
    }

protected:
    int           MASK_R;
    int           MASK_G;
    int           MASK_B;
    int           display_precision;
    bool          display_precision_fixed_point;
    wxDouble      point_radius;
    bool          enable_high_dpi_support;
    bool          is_showing_brush;
	SelectState   selectstate;
	MouseMode     mousemode;
	BrushType     brushtype;
    bool          is_brushing;
	ScrollBarMode scrollbarmode;
	GdaScaleTrans last_scale_trans;
	bool          fit_to_window_mode;
    double        scale_factor;

    /** highlight_state is a pointer to the Observable HighlightState instance.
	 A HightlightState instance is a vector of booleans that keep track
	 of the highlight state for every observation in the currently opened SHP
	 file. This shared state object is the means by which the different
	 views in GeoDa are linked. */
	HLStateInt*           highlight_state;
	std::list<GdaShape*>  background_shps;
    
	/** This is an array of selectable objects.  In a map, these would
	 be the various observation regions or points, while in a histogram
	 these would be the bars of the histogram. This array of shapes is drawn
	 after the background_shps multi-set. */
	std::vector<GdaShape*> selectable_shps;
    std::vector<bool>      selectable_shps_undefs;
	SelectableShpType      selectable_shps_type;
	std::list<GdaShape*>   foreground_shps;
    
	// corresponds to the selectable color categories: generally between
	// 1 and 10 permitted.  Selectable shape drawing routines use brushes
	// from this list.
	bool use_category_brushes;
    
	// when true, draw all selectable shapes in order, with highlights,
	// and using category colors.  This is only used in Bubble Chart currently
	bool draw_sel_shps_by_z_val;
    
	// for each time period, the shape id is listed with its category
	// only used when draw_sel_shps_by_z_val is selected
	std::vector<i_array_type> z_val_order;
	
	wxPoint GetActualPos(const wxMouseEvent& event);
	wxPoint sel_poly_pts[100];  // for UpdateSelectRegion and UpdateSelection
	int n_sel_poly_pts;         // for UpdateSelectRegion and UpdateSelection
	GdaSelRegion sel_region;	// for UpdateSelectRegion and UpdateSelection
	bool remember_shiftdown;    // used by OnMouseEvent
	wxPoint diff;               // used by OnMouseEvent
	wxPoint prev;	            // used by OnMouseEvent
	wxPoint sel1;
	wxPoint sel2;
    
	std::vector<int> hover_obs; // list of obs mouse is hovering over
	int total_hover_obs; // total obs in list
	int max_hover_obs;
	// preserve current map bounding box for zoom/pan
	bool is_pan_zoom;
	int  prev_scroll_pos_x;
	int  prev_scroll_pos_y;
    
	wxBitmap* faded_layer_bm; // layer1_bm + foreground obs
	wxBitmap* layer0_bm; // background items + unhighlighted obs
	wxBitmap* layer1_bm; // layer0_bm + highlighted obs
	wxBitmap* layer2_bm; // layer1_bm + foreground obs
    
	bool layer0_valid; // if false, then needs to be redrawn
	bool layer1_valid; // if false, then needs to be redrawn
	bool layer2_valid; // if flase, then needs to be redrawn
	
	Project* project;
	TemplateFrame* template_frame;

    bool isResize;
    wxTimer* highlight_timer;
    
    void OnHighlightTimerEvent(wxTimerEvent &event);
    
	virtual void UpdateSelectableOutlineColors();
	// The following five methods enable the use of a custom
	// HLStateInt object
	// Returns bit vector of selection values according
	// to selectable objects
	virtual std::vector<bool>& GetSelBitVec();

	// Returns number of newly selected objects
	virtual int GetNumNewlySel();
	// Sets number of newly selected objects
	virtual void SetNumNewlySel(int n);
	// Returns list of newly selected objects.  Only indexes
	// 0 through GetNumNewlySel()-1 are valid.
	virtual std::vector<int>& GetNewlySelList();

	// Returns number of newly unselected objects
	virtual int GetNumNewlyUnsel();
	// Sets number of newly unselected objects
	virtual void SetNumNewlyUnsel(int n);
	// Returns list of newly unselected objects.  Only indexes
	// 0 through GetNumNewlyUnsel()-1 are valid.
	virtual std::vector<int>& GetNewlyUnselList();

    // helper functions
    bool _IsShpValid(int idx);
    
	DECLARE_EVENT_TABLE()
};


#endif

