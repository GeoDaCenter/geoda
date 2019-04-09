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

using namespace std;

class GroupingSelectDlg : public wxDialog
{
    Project* project;
    TableInterface* table_int;
    wxChoice* group_var_list;
    wxChoice* root_var_list;
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    
public:
    GroupingSelectDlg(wxFrame *parent, Project* project);
    virtual ~GroupingSelectDlg();
    
    void OnOK( wxCommandEvent& event );
};

class GroupingMapCanvas : public MapCanvas
{
	DECLARE_CLASS(GroupingMapCanvas)
public:	
    GroupingMapCanvas(wxWindow *parent,
                      TemplateFrame* t_frame,
                      Project* project,
                      std::vector<GdaVarTools::VarInfo> vars,
                      std::vector<int> col_ids,
                      boost::uuids::uuid w_uuid,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize);
	virtual ~GroupingMapCanvas();
    
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();

    wxString group_field_nm;
    wxString root_field_nm;
    
	DECLARE_EVENT_TABLE()
};


class GroupingMapFrame : public MapFrame
{
	DECLARE_CLASS(GroupingMapFrame)
public:
    GroupingMapFrame(wxFrame *parent,
                     Project* project,
                     std::vector<GdaVarTools::VarInfo> vars,
                     std::vector<int> col_ids,
                     boost::uuids::uuid w_uuid,
                     const wxString title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = GdaConst::map_default_size,
                     const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~GroupingMapFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    
    DECLARE_EVENT_TABLE()
};

#endif
