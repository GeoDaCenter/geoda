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

#ifndef __GEODA_CENTER_GROUPING_MAP_VIEW_H__
#define __GEODA_CENTER_GROUPING_MAP_VIEW_H__

#include <map>
#include <vector>
#include "MapNewView.h"
#include "../GdaConst.h"

class HierachicalMapSelectDlg : public wxDialog
{
    std::vector<GdaVarTools::VarInfo> vars;
    std::vector<int> col_ids;
    boost::uuids::uuid uid;
    wxString title;
    Project* project;
    TableInterface* table_int;
    wxChoice* group_var_list;
    wxChoice* root_var_list;
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    
public:
    HierachicalMapSelectDlg(wxFrame *parent, Project* project);
    virtual ~HierachicalMapSelectDlg();
    
    void OnOK( wxCommandEvent& event );
    std::vector<GdaVarTools::VarInfo> GetVarInfo();
    std::vector<int> GetColIds();
    wxString GetTitle();
    boost::uuids::uuid GetWUID();
};

class HierachicalMapCanvas : public MapCanvas
{
	DECLARE_CLASS(GroupingMapCanvas)

    wxString group_field_nm;
    wxString root_field_nm;
    int root_field_id;

    std::map<int, bool> grp_root;
    int root_radius;
    wxColour root_color;

    virtual void DrawConnectivityGraph(wxMemoryDC &dc);
    virtual void CreateConnectivityGraph();

public:	
    HierachicalMapCanvas(wxWindow *parent,
                      TemplateFrame* t_frame,
                      Project* project,
                      std::vector<GdaVarTools::VarInfo> vars,
                      std::vector<int> col_ids,
                      boost::uuids::uuid w_uuid,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize);
	virtual ~HierachicalMapCanvas();
    
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();

    void ChangeRootSize(int root_size);
    void ChangeRootColor(wxColour root_color);
    int GetRootSize();
    wxColour GetRootColor();

	DECLARE_EVENT_TABLE()
};


class HierachicalMapFrame : public MapFrame
{
	DECLARE_CLASS(GroupingMapFrame)
public:
    HierachicalMapFrame(wxFrame *parent,
                     Project* project,
                     std::vector<GdaVarTools::VarInfo> vars,
                     std::vector<int> col_ids,
                     boost::uuids::uuid w_uuid,
                     const wxString title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = GdaConst::map_default_size,
                     const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~HierachicalMapFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    virtual void update(WeightsManState* o);
    
    void OnChangeConnRootSize(wxCommandEvent& event);
    void OnChangeConnRootColor(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

#endif
