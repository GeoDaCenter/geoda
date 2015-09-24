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

#ifndef __GEODA_CENTER_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_MAP_NEW_VIEW_H__

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/window.h>
#include "CatClassification.h"
#include "CatClassifStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"
#include "../ShapeOperations/WeightsManStateObserver.h"

class CatClassifState;
class MapFrame;
class MapCanvas;
class MapNewLegend;
class TableInterface;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;


// Transparency SliderBar dialog for Basemap
class SliderDialog: public wxDialog
{
    DECLARE_CLASS( SliderDialog )
    DECLARE_EVENT_TABLE()
public:
    SliderDialog ();
	SliderDialog (wxWindow * parent, TemplateCanvas* _canvas, wxWindowID id=wxID_ANY,
                  const wxString & caption="Slider Dialog",
                 const wxPoint & pos = wxDefaultPosition,
                 const wxSize & size = wxDefaultSize,
                 long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~SliderDialog ();
    
private:
    TemplateCanvas* canvas;
    wxSlider* slider;
    wxStaticText* slider_text;
	void OnSliderChange(wxScrollEvent& event );
    
};

class MapCanvas : public TemplateCanvas, public CatClassifStateObserver
{
	DECLARE_CLASS(MapCanvas)
public:
	
	enum SmoothingType { no_smoothing, raw_rate, excess_risk, empirical_bayes,
		spatial_rate, spatial_empirical_bayes };
	
	MapCanvas(wxWindow *parent, TemplateFrame* t_frame,
				 Project* project,
				 const std::vector<GdaVarTools::VarInfo>& var_info,
				 const std::vector<int>& col_ids,
				 CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				 SmoothingType smoothing_type = no_smoothing,
				 int num_categories = 1,
				 boost::uuids::uuid weights_id = boost::uuids::nil_uuid(),
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize);
	virtual ~MapCanvas();

	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	
	virtual void NewCustomCatClassif();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
						SmoothingType new_map_smoothing,
						int num_categories,
						boost::uuids::uuid weights_id,
						bool use_new_var_info_and_col_ids,
						const std::vector<GdaVarTools::VarInfo>& new_var_info,
						const std::vector<int>& new_col_ids,
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(HLStateInt* o);
	virtual void update(CatClassifState* o);
	virtual void SaveRates();
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	
    int GetBasemapType();
    void CleanBasemapCache();
    
public:
	virtual void PopulateCanvas();
	virtual void VarInfoAttributeChange();
	virtual void CreateAndUpdateCategories();

	virtual void TimeSyncVariableToggle(int var_index);
	virtual void DisplayMeanCenters();
	virtual void DisplayCentroids();
	virtual void DisplayVoronoiDiagram();
	virtual int GetNumVars();
	virtual int GetNumCats();
	virtual boost::uuids::uuid GetWeightsId() { return weights_id; }
	virtual void SetWeightsId(boost::uuids::uuid id) { weights_id = id; }
	
	CatClassifDef cat_classif_def;
	CatClassification::CatClassifType GetCcType();
	SmoothingType smoothing_type;
	bool is_rate_smoother;
	bool display_mean_centers;
	bool display_centroids;
	bool display_voronoi_diagram;
	bool voronoi_diagram_duplicates_exist;
	
protected:
    
	TableInterface* table_int;
	CatClassifState* custom_classif_state;
	
	int num_obs;
	int num_time_vals;
	std::vector<d_array_type> data;
	std::vector<Gda::dbl_int_pair_vec_type> cat_var_sorted;
	int num_categories; // used for Quantile, Equal Interval and Natural Breaks
	
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	bool full_map_redraw_needed;
	
	boost::uuids::uuid weights_id;
	
	virtual void UpdateStatusBar();
		
	DECLARE_EVENT_TABLE()
};

class MapNewLegend : public TemplateLegend {
public:
	MapNewLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~MapNewLegend();
};

class MapFrame : public TemplateFrame, public WeightsManStateObserver
{
   DECLARE_CLASS(MapFrame)
public:
    MapFrame(wxFrame *parent, Project* project,
				const std::vector<GdaVarTools::VarInfo>& var_info,
				const std::vector<int>& col_ids,
				CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				MapCanvas::SmoothingType smoothing_type
				  = MapCanvas::no_smoothing,
				int num_categories = 1,
				boost::uuids::uuid weights_id = boost::uuids::nil_uuid(),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				const long style = wxDEFAULT_FRAME_STYLE);
	/** This constructor should only be called by derived classes */
	MapFrame(wxFrame *parent, Project* project,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~MapFrame();

    void SetupToolbar();
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const;
	virtual void closeObserver(boost::uuids::uuid id);
	
	virtual void OnNewCustomCatClassifA();
	virtual void OnCustomCatClassifA(const wxString& cc_title);
	virtual void OnThemelessMap();
	virtual void OnQuantile(int num_cats);
	virtual void OnPercentile();
	virtual void OnHinge15();
	virtual void OnHinge30();
	virtual void OnStdDevMap();
	virtual void OnUniqueValues();
	virtual void OnNaturalBreaks(int num_cats);
	virtual void OnEqualIntervals(int num_cats);
	virtual void OnRawrate();
	virtual void OnExcessRisk();
	virtual void OnEmpiricalBayes();
	virtual void OnSpatialRate();
	virtual void OnSpatialEmpiricalBayes();
	virtual void OnSaveRates();
	virtual void OnSaveCategories();
	virtual void OnDisplayMeanCenters();
	virtual void OnDisplayCentroids();
	virtual void OnDisplayVoronoiDiagram();
	virtual void OnExportVoronoi();
	virtual void OnExportMeanCntrs();
	virtual void OnExportCentroids();
	virtual void OnSaveVoronoiDupsToTable();
    virtual void OnChangeMapTransparency();
    
    void CleanBasemap();
    
protected:
    void OnMapSelect(wxCommandEvent& e);
    void OnMapInvertSelect(wxCommandEvent& e);
    void OnMapPan(wxCommandEvent& e);
    void OnMapZoom(wxCommandEvent& e);
    void OnMapZoomOut(wxCommandEvent& e);
    void OnMapExtent(wxCommandEvent& e);
    void OnMapRefresh(wxCommandEvent& e);
    //void OnMapBrush(wxCommandEvent& e);
    void OnMapBasemap(wxCommandEvent& e);
    
	
protected:
	bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
					   MapCanvas::SmoothingType new_map_smoothing,
					   int num_categories,
					   boost::uuids::uuid weights_id,
					   bool use_new_var_info_and_col_ids,
					   const std::vector<GdaVarTools::VarInfo>& new_var_info,
					   const std::vector<int>& new_col_ids,
					   const wxString& custom_classif_title = wxEmptyString);
	
	WeightsManState* w_man_state;
	
    DECLARE_EVENT_TABLE()
};


#endif
