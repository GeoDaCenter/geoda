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

#ifndef __GEODA_CENTER_NEIGHBORMATCH_DLG_H__
#define __GEODA_CENTER_NEIGHBORMATCH_DLG_H__

#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>

#include "../DataViewer/TableInterface.h"
#include "../VarTools.h"
#include "../Explore/MapNewView.h"
#include "AbstractClusterDlg.h"

class NbrMatchDlg : public AbstractClusterDlg
{
public:
    NbrMatchDlg(wxFrame *parent, Project* project);
    virtual ~NbrMatchDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );

    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnDistanceChoice( wxCommandEvent& event );
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:

    wxChoice* m_distance;
    wxChoice* m_geo_dist_metric;
    wxTextCtrl* txt_knn;

    wxCheckBox* chk_seed;
    wxButton* seedButton;
    
    DECLARE_EVENT_TABLE()
};

class NbrMatchSaveWeightsDialog : public wxDialog
{
    wxChoice* m_id_field;
    std::vector<int> col_id_map;
    TableInterface* table_int;
public:
    NbrMatchSaveWeightsDialog(TableInterface* table_int, const wxString& title);
    void OnIdVariableSelected( wxCommandEvent& event );
    bool CheckID(const wxString& id);
    wxString GetSelectID();
};

class LocalMatchMapCanvas : public MapCanvas
{
    DECLARE_CLASS(LocalMatchMapCanvas)
public:
    LocalMatchMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                        Project* project,
                        const std::vector<std::vector<int> >& groups,
                        boost::uuids::uuid weights_id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
    virtual ~LocalMatchMapCanvas();
    
    virtual void DisplayRightClickMenu(const wxPoint& pos);
    virtual wxString GetCanvasTitle();
    virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
                               SmoothingType new_map_smoothing);
    virtual void SetCheckMarks(wxMenu* menu);
    virtual void TimeChange();
    virtual void CreateAndUpdateCategories();
    virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();
    
    const std::vector<std::vector<int> >& groups;
    
    DECLARE_EVENT_TABLE()
};


class LocalMatchMapFrame : public MapFrame
{
    DECLARE_CLASS(LocalMatchMapFrame)
public:
    LocalMatchMapFrame(wxFrame *parent, Project* project,
                       const std::vector<std::vector<int> >& groups,
                       boost::uuids::uuid weights_id = boost::uuids::nil_uuid(),
                       const wxString& title = wxEmptyString,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = GdaConst::map_default_size);
    virtual ~LocalMatchMapFrame();
    
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    
    void OnSave(wxCommandEvent& event);
    
    boost::uuids::uuid weights_id;
    
    DECLARE_EVENT_TABLE()
};


#endif
