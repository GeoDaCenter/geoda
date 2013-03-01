/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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
#include "../Generic/MyShape.h"

class CatClassifState;
class MapNewFrame;
class MapNewCanvas;
class MapNewLegend;
class DbfGridTableBase;
class GalWeight;
typedef boost::multi_array<double, 2> d_array_type;

class MapNewCanvas : public TemplateCanvas, public CatClassifStateObserver {
	DECLARE_CLASS(MapNewCanvas)
public:
	
	enum SmoothingType { no_smoothing, raw_rate, excess_risk, empirical_bayes,
		spatial_rate, spatial_empirical_bayes };
	
	MapNewCanvas(wxWindow *parent, TemplateFrame* t_frame,
				 Project* project,
				 CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				 SmoothingType smoothing_type = no_smoothing,
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
						const wxString& custom_classif_title = wxEmptyString);
	virtual void update(CatClassifState* o);
	virtual void SaveRates();
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TitleOrTimeChange();
	
protected:
	virtual void PopulateCanvas();
	virtual void VarInfoAttributeChange();
	virtual void CreateAndUpdateCategories();

public:
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void DisplayMeanCenters();
	virtual void DisplayCentroids();
	virtual void DisplayVoronoiDiagram();
	
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
	DbfGridTableBase* grid_base;
	HighlightState* highlight_state;
	CatClassifState* custom_classif_state;
	
	int num_obs;
	int num_time_vals;
	std::vector<d_array_type> data;
	std::vector<GeoDa::dbl_int_pair_vec_type> cat_var_sorted;
	
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
				CatClassification::CatClassifType theme_type =
					CatClassification::no_theme,
				MapNewCanvas::SmoothingType smoothing_type
				  = MapNewCanvas::no_smoothing,
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
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);

	virtual void UpdateTitle();
	
	virtual void OnNewCustomCatClassifA();
	virtual void OnCustomCatClassifA(const wxString& cc_title);
	void OnThemelessMap(wxCommandEvent& event);
	void OnQuantile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnStdDevMap(wxCommandEvent& event);
	void OnUniqueValues(wxCommandEvent& event);
	void OnNaturalBreaks(wxCommandEvent& event);
	void OnEqualIntervals(wxCommandEvent& event);
	void OnRawrate(wxCommandEvent& event);
	void OnExcessRisk(wxCommandEvent& event);
	void OnEmpiricalBayes(wxCommandEvent& event);
	void OnSpatialRate(wxCommandEvent& event);
	void OnSpatialEmpiricalBayes(wxCommandEvent& event);
	void OnSaveRates(wxCommandEvent& event);
	void OnSaveCategories(wxCommandEvent& event);
	virtual void OnDisplayMeanCenters();
	virtual void OnDisplayCentroids();
	virtual void OnDisplayVoronoiDiagram();
	virtual void OnSaveVoronoiToShapefile();
	virtual void OnSaveMeanCntrsToShapefile();
	virtual void OnSaveCentroidsToShapefile();
	virtual void OnSaveVoronoiDupsToTable();
	
protected:
	bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
					   MapNewCanvas::SmoothingType new_map_smoothing,
					   const wxString& custom_classif_title = wxEmptyString);
	
    DECLARE_EVENT_TABLE()
};


#endif
