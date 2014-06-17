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

#ifndef __GEODA_CENTER_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_MAP_NEW_VIEW_H__

#include <vector>
#include "CatClassification.h"
#include "CatClassifStateObserver.h"
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"
#include "../TemplateFrame.h"
#include "../GenUtils.h"
#include "../Generic/GdaShape.h"

class CatClassifState;
class MapNewFrame;
class MapNewCanvas;
class MapNewLegend;
class TableInterface;
class GalWeight;
typedef boost::multi_array<double, 2> d_array_type;

class MapNewCanvas : public TemplateCanvas, public CatClassifStateObserver {
	DECLARE_CLASS(MapNewCanvas)
public:
	
	enum SmoothingType { no_smoothing, raw_rate, excess_risk, empirical_bayes,
		spatial_rate, spatial_empirical_bayes };
	
	MapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
				 Project* project,
				 const std::vector<GeoDaVarInfo>& var_info,
				 const std::vector<int>& col_ids,
				 CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				 SmoothingType smoothing_type = no_smoothing,
				 int num_categories = 1,
				 const wxPoint& pos = wxDefaultPosition,
				 const wxSize& size = wxDefaultSize);
	virtual ~MapNewCanvas();

	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCanvasTitle();
	virtual wxString GetNameWithTime(int var);
	
	virtual void NewCustomCatClassif();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
						SmoothingType new_map_smoothing,
						int num_categories,
						bool use_new_var_info_and_col_ids,
						const std::vector<GeoDaVarInfo>& new_var_info,
						const std::vector<int>& new_col_ids,
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	virtual void SaveRates();
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	
protected:
	virtual void PopulateCanvas();
	virtual void VarInfoAttributeChange();
	virtual void CreateAndUpdateCategories();

public:
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void DisplayMeanCenters();
	virtual void DisplayCentroids();
	virtual void DisplayVoronoiDiagram();
	virtual int GetNumVars();
	virtual int GetNumCats();
	
	CatClassifDef cat_classif_def;
	CatClassification::CatClassifType GetCcType();
	SmoothingType smoothing_type;
	bool is_rate_smoother;
	bool display_mean_centers;
	bool display_centroids;
	bool display_voronoi_diagram;
	bool voronoi_diagram_duplicates_exist;
	
protected:
	Project* project;
	TableInterface* table_int;
	HighlightState* highlight_state;
	CatClassifState* custom_classif_state;
	
	int num_obs;
	int num_time_vals;
	std::vector<d_array_type> data;
	std::vector<Gda::dbl_int_pair_vec_type> cat_var_sorted;
	int num_categories; // used for Quantile, Equal Interval and Natural Breaks
	
	int ref_var_index;
	std::vector<GeoDaVarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	bool full_map_redraw_needed;
	
	GalWeight* gal_weight;
	
	virtual void UpdateStatusBar();
		
	DECLARE_EVENT_TABLE()
};

class MapNewLegend : public TemplateLegend {
public:
	MapNewLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~MapNewLegend();
};

class MapNewFrame : public TemplateFrame {
   DECLARE_CLASS(MapNewFrame)
public:
    MapNewFrame(wxFrame *parent, Project* project,
				const std::vector<GeoDaVarInfo>& var_info,
				const std::vector<int>& col_ids,
				CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				MapNewCanvas::SmoothingType smoothing_type
				  = MapNewCanvas::no_smoothing,
				int num_categories = 1,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				const long style = wxDEFAULT_FRAME_STYLE);
	/** This constructor should only be called by derived classes */
	MapNewFrame(wxFrame *parent, Project* project,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~MapNewFrame();

    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
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
	
protected:
	bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
					   MapNewCanvas::SmoothingType new_map_smoothing,
					   int num_categories,
					   bool use_new_var_info_and_col_ids,
					   const std::vector<GeoDaVarInfo>& new_var_info,
					   const std::vector<int>& new_col_ids,
					   const wxString& custom_classif_title = wxEmptyString);
	
    DECLARE_EVENT_TABLE()
};


#endif
