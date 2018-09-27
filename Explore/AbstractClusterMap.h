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

#ifndef __GEODA_CENTER_ABSTRACT_CLUSTER_MAP_H__
#define __GEODA_CENTER_ABSTRACT_CLUSTER_MAP_H__

#include "MapNewView.h"
#include "AbstractCoordinator.h"
#include "../GdaConst.h"

class AbstractMapCanvas : public MapCanvas
{
	DECLARE_CLASS(AbstractMapCanvas)
public:	
    AbstractMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                      Project* project,
                      AbstractCoordinator* a_coordinator,
                      CatClassification::CatClassifType theme_type,
                      bool is_clust,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize);
	virtual ~AbstractMapCanvas();
    
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();
    virtual void SetWeightsId(boost::uuids::uuid id) { weights_id = id; }
    
    virtual void SetLabelsAndColorForClusters(int& num_cats, int t,
                                              CatClassifData& cat_data) = 0;
    
protected:
	AbstractCoordinator* a_coord;
	bool is_clust; // true = Cluster Map, false = Significance Map
    wxString menu_xrcid;
    wxString str_undefined;
    wxString str_neighborless;
    wxString str_not_sig;
    wxString str_p005;
    wxString str_p001;
    wxString str_p0001;
    wxString str_p00001;
    
    wxColour clr_not_sig_point;
    wxColour clr_not_sig_polygon;
    
	DECLARE_EVENT_TABLE()
};

class AbstractMapFrame : public MapFrame, public AbstractCoordinatorObserver
{
	DECLARE_CLASS(AbstractMapFrame)
public:
    AbstractMapFrame(wxFrame *parent, Project* project,
					AbstractCoordinator* a_coordinator,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::map_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~AbstractMapFrame();

    void Init();
   
    virtual CatClassification::CatClassifType GetThemeType() = 0;
    virtual TemplateCanvas* CreateMapCanvas(wxPanel* rpanel) = 0;
    virtual void OnSaveResult(wxCommandEvent& event) = 0;
    virtual void OnShowAsConditionalMap(wxCommandEvent& event) = 0;
    
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    virtual void update(WeightsManState* o){}
	
	void RanXPer(int permutation);
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);
	
	void OnUseSpecifiedSeed(wxCommandEvent& event);
	void OnSpecifySeedDlg(wxCommandEvent& event);
	
	void SetSigFilterX(int filter);
	void OnSigFilter05(wxCommandEvent& event);
	void OnSigFilter01(wxCommandEvent& event);
	void OnSigFilter001(wxCommandEvent& event);
	void OnSigFilter0001(wxCommandEvent& event);
	void OnSigFilterSetup(wxCommandEvent& event);

	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);

	virtual void update(AbstractCoordinator* o);
	virtual void closeObserver(AbstractCoordinator* o);
	
	virtual void GetVizInfo(std::vector<int>& clusters);
    void CoreSelectHelper(const std::vector<bool>& elem);
    
protected:
    CatClassification::CatClassifType theme_type;
	AbstractCoordinator* a_coord;
    wxString menu_xrcid;
    
    DECLARE_EVENT_TABLE()
};

#endif
