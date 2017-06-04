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

#ifndef __GEODA_CENTER_KMEAN_DLG_H___
#define __GEODA_CENTER_KMEAN_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>


#include "../FramesManager.h"
#include "../VarTools.h"

class Project;
class TableInterface;
	
class KMeansDlg : public wxDialog, public FramesManagerObserver
{
public:
    KMeansDlg(wxFrame *parent, Project* project);
    virtual ~KMeansDlg();
    
    void CreateControls();
    bool Init();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnDistanceChoice(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
        
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
private:
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    std::vector<wxString> tm_strs;
    
    FramesManager* frames_manager;
    
    wxCheckBox* chk_seed;
    wxListBox* combo_var;
    wxChoice* combo_method;
    wxChoice* combo_tranform;
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxCheckBox* m_use_centroids;
    wxTextCtrl* m_iterations;
    wxTextCtrl* m_pass;

    wxChoice* m_method;
    wxChoice* m_distance;
    
    wxButton* seedButton;
    
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    
    unsigned int row_lim;
    unsigned int col_lim;
    std::vector<float> scores;
    double thresh95;
    
    DECLARE_EVENT_TABLE()
};

#endif
