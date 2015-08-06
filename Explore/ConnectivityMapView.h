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

#ifndef __GEODA_CENTER_CONNECTIVITY_MAP_VIEW_H__
#define __GEODA_CENTER_CONNECTIVITY_MAP_VIEW_H__

#include <set>
#include "MapNewView.h"
#include "../ShapeOperations/WeightsManStateObserver.h"

class ConnectivityMapFrame;
class ConnectivityMapCanvas;
class WeightsManInterface;

class ConnectivityMapCanvas : public MapCanvas
{
	DECLARE_CLASS(ConnectivityMapCanvas)
public:	
	ConnectivityMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
						  Project* project, boost::uuids::uuid weights_id,
						  const wxPoint& pos = wxDefaultPosition,
						  const wxSize& size = wxDefaultSize);
	virtual ~ConnectivityMapCanvas();
	
	virtual void OnMouseEvent(wxMouseEvent& event);
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	void UpdateFromSharedCore();
	//virtual void PaintSelectionOutline(wxDC& dc);
	
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	virtual void CreateAndUpdateCategories();

	void ChangeWeights(boost::uuids::uuid new_id);
	virtual void update(HLStateInt* o);
protected:
	WeightsManInterface* w_man_int;
	
	/** The normal, shared Project::highlight_state.  This class is
	 going to set TemplateCanvas::highlight_state to a private version */
	HighlightState* proj_hs;
	HighlightState* shared_core_hs;
	
	std::set<long> sel_cores; // set of cores under current selection
	std::set<long> core_nbrs; // set of nbrs of cores, excl cores
	virtual void UpdateStatusBar();
	
	DECLARE_EVENT_TABLE()
};

class ConnectivityMapFrame : public MapFrame
{
	DECLARE_CLASS(ConnectivityMapFrame)
public:
    ConnectivityMapFrame(wxFrame *parent, Project* project,
					boost::uuids::uuid weights_id,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::map_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ConnectivityMapFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
		
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	
	void ChangeWeights(boost::uuids::uuid new_id);
	
protected:
	void CoreSelectHelper(const std::vector<bool>& elem);
	
    DECLARE_EVENT_TABLE()
};

#endif
