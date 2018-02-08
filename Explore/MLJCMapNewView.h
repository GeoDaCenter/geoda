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

#ifndef __GEODA_CENTER_MLJC_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_MLJC_MAP_NEW_VIEW_H__

#include <map>
#include "../GdaConst.h"
#include "MapNewView.h"
#include "MLJCCoordinatorObserver.h"

class MapCanvas;
class MapFrame;
class JCCoordinator;
class MLJCMapFrame;
class MLJCMapCanvas;

class MLJCMapCanvas : public MapCanvas
{
	DECLARE_CLASS(MLJCMapCanvas)
public:
    MLJCMapCanvas(wxWindow *parent,
                  TemplateFrame* t_frame,
                  bool is_clust,
                  Project* project,
                  JCCoordinator* gs_coordinator,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);
	virtual ~MLJCMapCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	void SyncVarInfoFromCoordinator();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();
    virtual void SetWeightsId(boost::uuids::uuid id) { weights_id = id; }

    double bo;
    double fdr;
	
protected:
	JCCoordinator* gs_coord;
	bool is_clust; // true = cluster map, false = significance map
	
    wxString str_sig;
    wxString str_high;
    wxString str_med;
    wxString str_low;
    wxString str_undefined;
    wxString str_neighborless;
    wxString str_p005;
    wxString str_p001;
    wxString str_p0001;
    wxString str_p00001;
    
	DECLARE_EVENT_TABLE()
};

class MLJCMapFrame : public MapFrame, public JCCoordinatorObserver
{
	DECLARE_CLASS(MLJCMapFrame)
public:
	
    MLJCMapFrame(wxFrame *parent, Project* project,
                 JCCoordinator* gs_coordinator,
                 bool isClusterMap,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = GdaConst::map_default_size,
                 const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~MLJCMapFrame();
	
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
    void OnSigFilterSetup(wxCommandEvent& event);
	
	void OnSaveMLJC(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);

    void OnShowAsConditionalMap(wxCommandEvent& event);
    
    virtual void update(JCCoordinator* o);
    /** Request for the Observer to close itself */
    virtual void closeObserver(JCCoordinator* o);
    
    
	JCCoordinator* GetJCCoordinator() { return gs_coord; }
	
protected:
	void CoreSelectHelper(const std::vector<bool>& elem);
	JCCoordinator* gs_coord;
	bool is_clust; // true = cluster map, false = significance map
	
	DECLARE_EVENT_TABLE()
};


#endif
