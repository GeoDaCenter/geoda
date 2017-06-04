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

#ifndef __GEODA_CENTER_LOCALGEARY_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_LOCALGEARY_MAP_NEW_VIEW_H__

#include "MapNewView.h"
#include "LocalGearyCoordinatorObserver.h"
#include "../GdaConst.h"

class LocalGearyMapFrame;
class LocalGearyMapCanvas;
class LocalGearyCoordinator;

class LocalGearyMapCanvas : public MapCanvas
{
	DECLARE_CLASS(LocalGearyMapCanvas)
public:	
	LocalGearyMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
					 Project* project,
					 LocalGearyCoordinator* local_geary_coordinator,
					 CatClassification::CatClassifType theme_type,
					 bool isBivariate, bool isEBRate,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize);
	virtual ~LocalGearyMapCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
	
    bool is_diff;
    
protected:
	LocalGearyCoordinator* local_geary_coord;
	bool is_clust; // true = Cluster Map, false = Significance Map
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Empirical Bayes Rate Smoothing
	
	DECLARE_EVENT_TABLE()
};

class LocalGearyMapFrame : public MapFrame, public LocalGearyCoordinatorObserver
{
	DECLARE_CLASS(LocalGearyMapFrame)
public:
    LocalGearyMapFrame(wxFrame *parent, Project* project,
					LocalGearyCoordinator* local_geary_coordinator,
					bool isClusterMap, bool isBivariate,
					bool isEBRate,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::map_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LocalGearyMapFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
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
	
	void OnSaveLocalGeary(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	void OnAddNeighborToSelection(wxCommandEvent& event);
    
    void OnShowAsConditionalMap(wxCommandEvent& event);
	
	virtual void update(LocalGearyCoordinator* o);
	virtual void closeObserver(LocalGearyCoordinator* o);
	
	void GetVizInfo(std::vector<int>& clusters);
protected:
	void CoreSelectHelper(const std::vector<bool>& elem);
	LocalGearyCoordinator* local_geary_coord;
	
    DECLARE_EVENT_TABLE()
};

#endif
