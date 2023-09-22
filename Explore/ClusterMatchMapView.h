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

#ifndef __GEODA_CENTER_CLUSTERMATCH_MAP_VIEW_H__
#define __GEODA_CENTER_CLUSTERMATCH_MAP_VIEW_H__

#include <vector>
#include <map>
#include <wx/wx.h>

#include "MapNewView.h"
#include "../GdaConst.h"

class ClusterMatchSelectDlg : public AbstractClusterDlg
{
public:
    ClusterMatchSelectDlg(wxFrame *parent, Project* project);
    virtual ~ClusterMatchSelectDlg();
   
    void CreateControls();
    
    virtual void update(TableState* o);
    
    void OnTargetSelect(wxCommandEvent& event);
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    bool ShowOptionsOfVariable(const wxString& var_name,
                               std::vector<wxInt64> cat_vals);
    void OnCheckBoxChange(wxCommandEvent& event);
    void OnOriginSelect(wxCommandEvent& event);
    virtual wxString _printConfiguration();

    GeoDaWeight* CreateQueenWeights();
    bool CheckCategorical(const wxString& var_name, std::vector<wxInt64>& cat_vals);
    
protected:
    int base_choice_id;
    wxString selected_target;
    wxString select_variable_lbl;
    wxString selected_variable;
    std::map<wxString, std::map<wxInt64, bool> > input_conf;
    
    wxPanel *panel;
    wxBoxSizer *container;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_min_size;
    wxFlexGridSizer *gbox;
    wxScrolledWindow* scrl;
    wxStaticText* m_cluster_lbl;
    wxComboBox* list_var;
    wxComboBox* target_var;
    std::vector<wxCheckBox*> chk_list;
    
    DECLARE_EVENT_TABLE()
};

class ClusterMatchMapCanvas : public MapCanvas
{
    DECLARE_CLASS(ClusterMatchMapCanvas)
public:
    ClusterMatchMapCanvas(wxWindow *parent,
                          TemplateFrame* t_frame,
                          Project* project,
                          const std::vector<wxInt64>& cat_values,
                          const std::map<wxInt64, bool>& cat_value_flags,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize);
    virtual ~ClusterMatchMapCanvas();
    
    virtual void CreateAndUpdateCategories();
    
    DECLARE_EVENT_TABLE()
};

class ClusterMatchMapFrame : public MapFrame
{
    DECLARE_CLASS(ClusterMatchMapFrame)
public:
    ClusterMatchMapFrame(wxFrame *parent,
                         Project* project,
                         const std::vector<wxInt64>& cat_values,
                         const std::map<wxInt64, bool>& cat_value_flags,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = GdaConst::map_default_size,
                         const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~ClusterMatchMapFrame();
        
    DECLARE_EVENT_TABLE()
};

#endif
