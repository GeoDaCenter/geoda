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

#ifndef __GEODA_CENTER_COLOCATION_MAP_VIEW_H__
#define __GEODA_CENTER_COLOCATION_MAP_VIEW_H__

#include <vector>
#include "MapNewView.h"
#include "../GdaConst.h"

class ColocationSelectDlg : public AbstractClusterDlg
{
public:
    ColocationSelectDlg(wxFrame *parent, Project* project);
    ~ColocationSelectDlg();
   
    void CreateControls();
    
    void OnVarSelect( wxCommandEvent& event );
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
protected:
    wxPanel *panel;
    wxChoice* combo_co_value;
    wxGridSizer *gbox;
    wxBoxSizer *container;
    
    wxArrayInt var_selections;
    std::vector<wxString> co_values;
    
    DECLARE_EVENT_TABLE()
};

/*
class ColocationMapCanvas : public MapCanvas
{
	DECLARE_CLASS(ColocationMapCanvas)
public:	
	ColocationMapCanvas(wxWindow *parent,
                        TemplateFrame* t_frame,
                        Project* project,
                        CatClassification::CatClassifType theme_type,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
	virtual ~ColocationMapCanvas();
    
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
	
    bool is_diff;
    
protected:
    wxString str_not_sig;
    wxString str_highhigh;
    wxString str_highlow;
    wxString str_lowlow;
    wxString str_lowhigh;
    wxString str_undefined;
    wxString str_neighborless;
    wxString str_p005;
    wxString str_p001;
    wxString str_p0001;
    wxString str_p00001;
    
	DECLARE_EVENT_TABLE()
};


class ColocationMapFrame : public MapFrame
{
	DECLARE_CLASS(ColocationMapFrame)
public:
    ColocationMapFrame(wxFrame *parent,
                       Project* project,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = GdaConst::map_default_size,
                       const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ColocationMapFrame();
	
    void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
	
	void OnSave(wxCommandEvent& event);
	
    void OnShowAsConditionalMap(wxCommandEvent& event);
	
	virtual void closeObserver(LisaCoordinator* o);
	
    DECLARE_EVENT_TABLE()
};
 */

#endif
