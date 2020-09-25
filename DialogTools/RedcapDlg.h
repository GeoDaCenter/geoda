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

#ifndef __GEODA_CENTER_REDCAP_DLG_H___
#define __GEODA_CENTER_REDCAP_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"
#include "../Algorithms/redcap.h"

using namespace std;

class Project;
class TableInterface;
	
class RedcapDlg : public AbstractClusterDlg
{
public:
    RedcapDlg(wxFrame *parent, Project* project);
    virtual ~RedcapDlg();
    
    void CreateControls();
    bool Init();
    
    void OnOK( wxCommandEvent& event );
    void OnSaveTree( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnCheckMinBound(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();
    
protected:
    wxChoice* combo_method;

    wxTextCtrl* m_max_region;
    
    wxCheckBox* chk_seed;
    
    wxTextCtrl* m_textbox;

    wxChoice* m_method;
	wxChoice* m_distance;

    wxStaticText* st_minregions;
    wxTextCtrl* txt_minregions;

    wxButton* seedButton;
    wxButton* saveButton;

    wxCheckBox* chk_save_mst;
    
    SpanningTreeClustering::AbstractClusterFactory* redcap;
    
    DECLARE_EVENT_TABLE()
};

#endif
