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

#ifndef __GEODA_CENTER_GETIS_ORD_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_GETIS_ORD_MAP_NEW_VIEW_H__

#include <map>
#include "../GdaConst.h"
#include "MapNewView.h"

class GetisOrdMapFrame;
class GetisOrdMapCanvas;
class GStatCoordinator;

class GetisOrdMapCanvas : public MapCanvas
{
	DECLARE_CLASS(GetisOrdMapCanvas)
public:
	GetisOrdMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
						 Project* project, GStatCoordinator* gs_coordinator,
						 bool is_gi, bool is_clust, bool is_perm,
						 bool row_standardize_weights,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	virtual ~GetisOrdMapCanvas();
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
    
    double bo;
    double fdr;
	
protected:
	GStatCoordinator* gs_coord;
	bool is_gi; // true = Gi, false = GiStar
	bool is_clust; // true = cluster map, false = significance map
	bool is_perm; // true = pseudo-p-val, false = normal distribution p-val
	bool row_standardize; // true = row standardize, false = binary
	
    wxString str_not_sig;
    wxString str_high;
    wxString str_low;
    wxString str_undefined;
    wxString str_neighborless;
    wxString str_p005;
    wxString str_p001;
    wxString str_p0001;
    wxString str_p00001;
    
	DECLARE_EVENT_TABLE()
};

class GetisOrdMapFrame : public MapFrame
{
	DECLARE_CLASS(GetisOrdMapFrame)
public:
	enum GMapType {
		Gi_clus_perm = 0,		// G_i, cluster, permutation
		Gi_clus_norm = 1,		// G_i, cluster, normal_dist
		Gi_sig_perm = 2,		// G_i, significance, permutation
		Gi_sig_norm = 3,		// G_i, significance, normal_dist
		GiStar_clus_perm = 4,	// G_i_star, cluster, permutation
		GiStar_clus_norm = 5,	// G_i_star, cluster, normal_dist
		GiStar_sig_perm = 6,	// G_i_star, significance, permutation
		GiStar_sig_norm = 7,	// G_i_star, significance, normal_dist
	};
	
	GetisOrdMapFrame(wxFrame *parent, Project* project,
						GStatCoordinator* gs_coordinator,
						GMapType map_type,
						bool row_standardize_weights,
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = GdaConst::map_default_size,
						const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~GetisOrdMapFrame();
	
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
	
	void OnSaveGetisOrd(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);

    void OnShowAsConditionalMap(wxCommandEvent& event);
    
	virtual void update(GStatCoordinator* o);
	virtual void closeObserver(GStatCoordinator* o);
	GStatCoordinator* GetGStatCoordinator() { return gs_coord; }
	
	GMapType map_type;
	
protected:
	void CoreSelectHelper(const std::vector<bool>& elem);
	GStatCoordinator* gs_coord;
	bool is_gi; // true = Gi, false = GiStar
	bool is_clust; // true = cluster map, false = significance map
	bool is_perm; // true = pseudo-p-val, false = normal distribution p-val
	bool row_standardize; // true = row standardize, false = binary
	
	DECLARE_EVENT_TABLE()
};


#endif
