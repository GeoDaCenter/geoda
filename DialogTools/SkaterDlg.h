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

#ifndef __GEODA_CENTER_SKATER_DLG_H___
#define __GEODA_CENTER_SKATER_DLG_H___

#include <vector>
#include <map>
#include <wx/wx.h>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"
#include "../Algorithms/redcap.h"

class Project;
class TableInterface;
	
class SkaterDlg : public AbstractClusterDlg
{
public:
    SkaterDlg(wxFrame *parent, Project* project);
    virtual ~SkaterDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnSaveTree(wxCommandEvent& event );
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    virtual void OnCheckMinBound(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();
    
private:
    wxCheckBox* chk_seed;
    wxCheckBox* chk_lisa;
    
    wxChoice* combo_weights;
    wxChoice* combo_lisa;
    
    wxChoice* m_distance;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_max_region;
    
    wxStaticText* st_minregions;
    wxTextCtrl* txt_minregions;

    wxButton* seedButton;
    wxButton* saveButton;

    wxCheckBox* chk_save_mst;
    
    SpanningTreeClustering::Skater* skater;
    
    DECLARE_EVENT_TABLE()
};

#endif
