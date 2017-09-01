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

#ifndef __GEODA_CENTER_MAXP_DLG_H___
#define __GEODA_CENTER_MAXP_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>


#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class Project;
class TableInterface;
	
class MaxpDlg : public AbstractClusterDlg
{
public:
    MaxpDlg(wxFrame *parent, Project* project);
    virtual ~MaxpDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnLISACheck(wxCommandEvent& event);
    void OnFloorCheck(wxCommandEvent& event);
    
    void InitVariableCombobox(wxListBox* var_box);
    
private:
    wxCheckBox* chk_seed;
    wxCheckBox* chk_lisa;
    
    wxListBox* combo_var;
    wxChoice* combo_weights;
    wxChoice* combo_tranform;
    wxChoice* combo_lisa;
    
    wxChoice* m_distance;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_iterations;

    wxButton* seedButton;
    
    DECLARE_EVENT_TABLE()
};

#endif
