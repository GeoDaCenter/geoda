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
    virtual ~ColocationSelectDlg();
   
    void CreateControls();
    
    virtual void update(TableState* o);
    
    void OnVarSelect( wxCommandEvent& event );
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnClickColor(wxMouseEvent& ev);
    void OnRightUp(wxMouseEvent& ev);
    void OnPopupClick( wxCommandEvent& event );
    void OnSchemeSelect( wxCommandEvent& event );
 
    virtual wxString _printConfiguration();
    void clear_colo_control();
    void add_colo_control(bool is_new=false);
    wxColour get_a_color(wxString label);
    wxColour get_a_color(int idx);
    wxString get_a_label(wxString label);
    bool check_colocations();
   
    void update_grid();
    
protected:
    wxPanel *panel;
    wxFlexGridSizer *gbox;
    wxBoxSizer *container;
    wxScrolledWindow* scrl;
    wxChoice* clrscheme_choice;
    
    std::vector<wxStaticBitmap*> co_bitmaps;
    std::vector<wxCheckBox*> co_boxes;
    
    wxArrayInt var_selections;
    std::vector<wxString> co_values;
   
    int base_remove_id;
    int base_color_id;
    int base_choice_id;
    
    std::vector<wxColour> m_colors;
    std::vector<wxColour> m_labels;
    std::vector<wxColour> m_predef_colors;
    std::vector<wxString> m_predef_labels;
    
    std::map<wxInt64, std::vector<int> > co_val_dict;
    
    DECLARE_EVENT_TABLE()
};

class ColocationMapCanvas : public MapCanvas
{
	DECLARE_CLASS(ColocationMapCanvas)
public:	
	ColocationMapCanvas(wxWindow *parent,
                        TemplateFrame* t_frame,
                        Project* project,
                        std::vector<wxString>& select_vars,
                        std::vector<wxString>& co_vals,
                        std::vector<wxColour>& co_clrs,
                        std::vector<wxString>& co_lbls,
                        std::vector<std::vector<int> >& co_ids,
                        CatClassification::CatClassifType theme_type,
                        boost::uuids::uuid weights_id_s,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
	virtual ~ColocationMapCanvas();
    
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
	virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
							   SmoothingType new_map_smoothing);
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void TimeChange();
	virtual void CreateAndUpdateCategories();
	virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();

    std::vector<wxString> select_vars;
    std::vector<wxString> co_vals;
    std::vector<wxColour> co_clrs;
    std::vector<wxString> co_lbls;
    std::vector<std::vector<int> > co_ids;
    
	DECLARE_EVENT_TABLE()
};


class ColocationMapFrame : public MapFrame
{
	DECLARE_CLASS(ColocationMapFrame)
public:
    ColocationMapFrame(wxFrame *parent,
                       Project* project,
                       std::vector<wxString>& select_vars,
                       std::vector<wxString>& co_vals,
                       std::vector<wxColour>& co_clrs,
                       std::vector<wxString>& co_lbls,
                       std::vector<std::vector<int> >& co_ids,
                       boost::uuids::uuid weights_id_s,
                       const wxString title,
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

#endif
