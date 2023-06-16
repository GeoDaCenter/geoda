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
#include <boost/unordered_map.hpp>
#include <wx/wx.h>

#include "CatClassification.h"
#include "CatClassifStateObserver.h"

#include "Basemap.h"
#include "../DataViewer/DataSource.h"
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"
#include "../TemplateFrame.h"
#include "../VarTools.h"
#include "../GdaShape.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../ShapeOperations/GalWeight.h"
#include "../MapLayerStateObserver.h"
#include "MapLayer.hpp"
#include "MapViewHelper.h"

class CatClassifState;
class MapTreeFrame;
class MapFrame;
class MapCanvas;
class MapNewLegend;
class TableInterface;
class WeightsManState;
class ExportDataDlg;
class OGRLayerProxy;

typedef boost::multi_array<bool, 2> b_array_type;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<wxString, 2> s_array_type;

class MapCanvas : public TemplateCanvas, public CatClassifStateObserver,
                  public MapLayerStateObserver, public AssociateLayerInt
{
	DECLARE_CLASS(MapCanvas)
public:
	
    enum SmoothingType { no_smoothing, raw_rate,
        excess_risk, empirical_bayes,
        spatial_rate, spatial_empirical_bayes };
    
    MapCanvas(wxWindow *parent, TemplateFrame* t_frame,
              Project* project,
              const std::vector<GdaVarTools::VarInfo>& var_info,
              const std::vector<int>& col_ids,
              CatClassification::CatClassifType theme_type = CatClassification::no_theme,
              SmoothingType smoothing_type = no_smoothing,
              int num_categories = 1,
              boost::uuids::uuid weights_id = boost::uuids::nil_uuid(),
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize);
    
	virtual ~MapCanvas();

    virtual void RedrawMap();

    // functions for heat map
    virtual void OnHeatMap(int menu_id);

    // function for MST map
    virtual void OnMSTMap(int menu_id);
    
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void AddTimeVariantOptionsToMenu(wxMenu* menu);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual wxString GetNameWithTime(int var);
    virtual void UpdateSelection(bool shiftdown = false,
                                 bool pointsel = false);
    virtual void UpdateSelectionPoints(bool shiftdown = false,
                                       bool pointsel = false);
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
    virtual void update(MapLayerState* o);
	virtual void SaveRates();
	virtual void OnSaveCategories();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
    virtual void OnSize(wxSizeEvent& event);
    virtual void SetWeightsId(boost::uuids::uuid id);
    virtual void deleteLayerBms();
	virtual void DrawLayerBase();
	virtual void DrawLayers();
    virtual void resizeLayerBms(int width, int height);
    virtual void DrawLayer0();
	virtual void DrawLayer1();
	virtual void DrawLayer2();
    virtual void SetHighlight(int idx);
    virtual void DrawHighlighted(wxMemoryDC &dc, bool revert);
    virtual void DrawHighlightedShapes(wxMemoryDC &dc, bool revert);
    virtual void DrawSelectableShapes_dc(wxMemoryDC &_dc, bool hl_only=false,
                                         bool revert=false,
                                         bool use_crosshatch=false);
    virtual void ResetShapes();
	virtual void ZoomShapes(bool is_zoomin = true);
	virtual void PanShapes();
    virtual void ExtentMap();
    virtual void ExtentTo(double minx, double miny, double maxx, double maxy);
    virtual OGRSpatialReference* GetSpatialReference();
    virtual bool IsExtentChanged();
    virtual void ResizeSelectableShps(int virtual_scrn_w = 0,
                                      int virtual_scrn_h = 0);
	virtual void PopulateCanvas();
	virtual void VarInfoAttributeChange();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
	virtual void DisplayMeanCenters();
	virtual void DisplayCentroids();
    virtual void DisplayWeightsGraph();
    virtual void DisplayNeighbors();
    virtual void DisplayMapWithGraph();
    virtual void DisplayMapBoundray(bool flag=false);
    virtual void ChangeGraphThickness(int val);
    virtual void ChangeGraphColor();
    virtual void ChangeConnSelectedColor();
    virtual void ChangeConnSelectedFillColor();
    virtual void ChangeNeighborFillColor();
    virtual void ChangeHeatMapFillColor();
    virtual void ChangeHeatMapOutlineColor();
	virtual void DisplayVoronoiDiagram();
	virtual int GetNumVars();
	virtual int GetNumCats();
	virtual boost::uuids::uuid GetWeightsId() { return weights_id; }
    virtual void DetermineMouseHoverObjects(wxPoint pt);
    virtual void RenderToDC(wxDC &dc, int w, int h);
    virtual void UpdateStatusBar();
    virtual wxBitmap* GetPrintLayer();
    
    void AddMapLayer(wxString name, BackgroundMapLayer* map_layer,
                     bool is_hide = false);
    void CleanBasemapCache();
    bool DrawBasemap(bool flag, Gda::BasemapItem& bm_item);
    void SetNoBasemap();
    void OnIdle(wxIdleEvent& event);
    void TranslucentLayer0(wxMemoryDC& dc);
    void RenderToSVG(wxDC& dc, int svg_w, int svg_h, int map_w, int map_h,
                     int offset_x, int offset_y);
    void SetupColor();
    void SetPredefinedColor(const wxString& lbl, const wxColor& new_color);
    void UpdatePredefinedColor(const wxString& lbl, const wxColor& new_color);
    std::vector<bool> AddNeighborsToSelection(GalWeight* gal_weights, wxMemoryDC &dc);
    void SetLegendLabel(int cat, wxString label);
   
    // multi-layers:
    std::vector<BackgroundMapLayer*> GetBackgroundMayLayers();
    std::vector<BackgroundMapLayer*> GetForegroundMayLayers();
    void SetForegroundMayLayers(std::vector<BackgroundMapLayer*>& val);
    void SetBackgroundMayLayers(std::vector<BackgroundMapLayer*>& val);
    std::vector<wxString> GetLayerNames();
    void RemoveLayer(wxString name);
    virtual bool IsCurrentMap();
    virtual wxString GetName();
    virtual std::vector<wxString> GetKeyNames();
    virtual int  GetNumRecords();
    virtual bool GetKeyColumnData(wxString col_name, std::vector<wxString>& data);
    virtual void ResetHighlight();
    virtual void DrawHighlight(wxMemoryDC& dc, MapCanvas* map_canvas);
    virtual void SetLayerAssociation(wxString my_key, AssociateLayerInt* layer,
                                     wxString key, bool show_connline=true);
    virtual bool IsAssociatedWith(AssociateLayerInt* layer);
    virtual GdaShape* GetShape(int idx);
    virtual int GetHighlightRecords();
    virtual void GetExtent(double &minx, double &miny, double &maxx,
                           double &maxy);
    virtual void GetExtentOfSelected(double &minx, double &miny, double &maxx,
                                     double &maxy);
    void UpdateMapTree();
    
    Shapefile::Main& GetGeometryData();
    OGRLayerProxy*   GetOGRLayerProxy();
    CatClassification::CatClassifType GetCcType();
    static void ResetThumbnail() {
        MapCanvas::has_thumbnail_saved = false;
    }
    Project* GetProject() { return project; }

    int num_obs;
    bool isDrawBasemap;
    int tran_unhighlighted;
    bool print_detailed_basemap;
	bool is_rate_smoother;
	bool display_mean_centers;
	bool display_centroids;
	bool display_voronoi_diagram;
	bool voronoi_diagram_duplicates_exist;
    bool display_map_boundary;
    bool display_weights_graph;
    bool display_neighbors;
    bool display_neighbor_color;
    bool display_map_with_graph;
    int  weights_graph_thickness;
    bool draw_highlight_in_multilayers;
    wxColour graph_color;
    wxColour conn_selected_color;
    wxColour conn_selected_fill_color;
    wxColour neighbor_fill_color;

    // heat map
    bool display_heat_map;
    bool show_heat_map_on_top;
    HeatMapHelper heat_map;

    // mst map
    bool display_mst;
    MSTMapHelper mst_map;

    // connectivity
    int conn_selected_size;
    std::set<int> ids_of_nbrs;
    std::vector<int> ids_wo_nbrs;
	std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<GdaPoint*> heat_map_pts;
    CatClassifDef cat_classif_def;
    SmoothingType smoothing_type;

    static std::vector<int> empty_shps_ids;
    static boost::unordered_map<int, bool> empty_dict;
    static bool has_shown_empty_shps_msg;
    static int GetEmptyNumber();
    static void ResetEmptyFlag();
    
    Gda::BasemapItem basemap_item;
    
protected:
    std::vector<BackgroundMapLayer*> bg_maps;
    std::vector<BackgroundMapLayer*> fg_maps;
    std::list<GdaShape*>  background_maps;
    std::list<GdaShape*>  foreground_maps;
    std::vector<int> select_with_neighbor; // only works w/ graph/connectivity

    bool layerbase_valid; // if false, then needs to be redrawn
    bool is_updating; // true: if triggered by other window
    std::vector<GdaPolyLine*> w_graph;
    IDataSource* p_datasource;
    static bool has_thumbnail_saved;
    wxString layer_name;
    wxString ds_name;
    
	TableInterface* table_int;
	CatClassifState* custom_classif_state;
    MapLayerState* maplayer_state;
	
    bool IS_VAR_STRING;
	int num_time_vals;
	std::vector<d_array_type> data;
    std::vector<s_array_type> s_data;
	std::vector<b_array_type> data_undef;
    
	std::vector<Gda::dbl_int_pair_vec_type> cat_var_sorted;
    std::vector<Gda::str_int_pair_vec_type> cat_str_var_sorted;
	int num_categories; // used for Quantile, Equal Interval and Natural Breaks
	
	int ref_var_index;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	bool full_map_redraw_needed;
	boost::uuids::uuid weights_id;
   
    // predefined/user-specified color, each label can be assigned with a color
    // user can specified using:
    // SetPredefinedColor(), UpdatePredifinedColor()
    std::map<wxString, wxColour> lbl_color_dict;
    
	wxBitmap* basemap_bm;
	Gda::Basemap* basemap;
    
    void show_empty_shps_msgbox();
    void SaveThumbnail();
    bool InitBasemap();
    virtual void DrawConnectivityGraph(wxMemoryDC &dc);
    virtual void CreateConnectivityGraph();
    
	DECLARE_EVENT_TABLE()
};

class MapNewLegend : public TemplateLegend {
public:
	MapNewLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				 const wxPoint& pos, const wxSize& size);
	virtual ~MapNewLegend();
    
    // override
    virtual void OnCategoryFillColor(wxCommandEvent& event);
	virtual void OnCategoryFillOpacity(wxCommandEvent& event);
    virtual void OnCategoryOutlineColor(wxCommandEvent& event);
};

class MapFrame : public TemplateFrame, public WeightsManStateObserver
{
   DECLARE_CLASS(MapFrame)
public:
    MapFrame(wxFrame *parent, Project* project,
             const std::vector<GdaVarTools::VarInfo>& var_info,
             const std::vector<int>& col_ids,
             CatClassification::CatClassifType theme_type = CatClassification::no_theme,
             MapCanvas::SmoothingType smoothing_type = MapCanvas::no_smoothing,
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

    void UpdateMapLayer();
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
    virtual void OnCustomCategoryClick(wxCommandEvent& event);
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
    virtual void OnSelectableOutlineVisible(wxCommandEvent& event);    
    virtual void OnChangeMapTransparency();
    virtual void OnDrawBasemap(bool flag, Gda::BasemapItem& bm_item);
    void SetNoBasemap();
    void OnBasemapSelect(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void CleanBasemap();
	void GetVizInfo(std::map<wxString, std::vector<int> >& colors);
    void GetVizInfo(wxString& shape_type,
                    wxString& field_name,
                    std::vector<wxString>& clrs,
                    std::vector<double>& bins);
    void OnAddNeighborToSelection(wxCommandEvent& event);
    void OnDisplayWeightsGraph(wxCommandEvent& event);
    void OnDisplayMapWithGraph(wxCommandEvent& event);
    void OnChangeGraphThickness(wxCommandEvent& event);
    void OnChangeGraphColor(wxCommandEvent& event);
    void OnChangeConnSelectedColor(wxCommandEvent& event);
    void OnChangeConnSelectedFillColor(wxCommandEvent& event);
    void OnChangeNeighborFillColor(wxCommandEvent& event);
    void OnChangeHeatMapFillColor(wxCommandEvent& event);
    void OnChangeHeatMapOutlineColor(wxCommandEvent& event);
    void OnMapSelect(wxCommandEvent& e);
    void OnMapInvertSelect(wxCommandEvent& e);
    void OnMapPan(wxCommandEvent& e);
    void OnMapZoom(wxCommandEvent& e);
    void OnMapZoomOut(wxCommandEvent& e);
    void OnMapExtent(wxCommandEvent& e);
    void OnMapRefresh(wxCommandEvent& e);
    void OnMapBasemap(wxCommandEvent& e);
    void OnMapAddLayer(wxCommandEvent& e);
    void OnMapEditLayer(wxCommandEvent& e);
    void OnMapTreeClose(wxWindowDestroyEvent& event);
    void OnShowMapBoundary(wxCommandEvent& event);

    void OnHeatMap(wxCommandEvent& event);
    void OnMapMST(wxCommandEvent& event);
    
    void UpdateMapTree();
	bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
					   MapCanvas::SmoothingType new_map_smoothing,
					   int num_categories,
					   boost::uuids::uuid weights_id,
					   bool use_new_var_info_and_col_ids,
					   const std::vector<GdaVarTools::VarInfo>& new_var_info,
					   const std::vector<int>& new_col_ids,
					   const wxString& custom_classif_title = wxEmptyString);
    void SetLegendLabel(int cat, wxString label) {
        if (!template_canvas) return;
        MapCanvas* map_canvs_ref = (MapCanvas*) template_canvas;
        map_canvs_ref->SetLegendLabel(cat, label);
        if (!template_legend) return;
        template_legend->Recreate();
    }
    void AppendCustomCategories(wxMenu* menu, CatClassifManager* ccm);
	
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:
    wxBoxSizer* rbox;

    MapTreeFrame* map_tree;
	WeightsManState* w_man_state;
    ExportDataDlg*   export_dlg;
	
    GalWeight* checkWeights();
    bool no_update_weights;
    
    DECLARE_EVENT_TABLE()
};


#endif
