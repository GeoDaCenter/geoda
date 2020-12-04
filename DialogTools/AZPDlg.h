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

#ifndef __GEODA_CENTER_AZP_DLG_H___
#define __GEODA_CENTER_AZP_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>


#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class Project;
class TableInterface;
	
class AZPDlg : public AbstractClusterDlg
{
public:
    AZPDlg(wxFrame *parent, Project* project);
    virtual ~AZPDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnLocalSearch(wxCommandEvent& event);
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnLISACheck(wxCommandEvent& event);
    void OnAriselCheck(wxCommandEvent& event);
    
    virtual void update(TableState* o);
    virtual void OnCheckMinBound(wxCommandEvent& event);
    virtual void InitLISACombobox();
    virtual wxString _printConfiguration();
    
private:
    wxCheckBox* chk_seed;
    wxCheckBox* chk_lisa;
    wxCheckBox* chk_arisel;
    
    wxChoice* combo_lisa;
   
    wxChoice* m_localsearch;
    wxChoice* m_distance;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_iterations;
    
    wxStaticText* st_minregions;
    wxTextCtrl* txt_minregions;
    wxTextCtrl* txt_regions;
    wxTextCtrl* m_tabulength;
    wxTextCtrl* m_convtabu;
    wxTextCtrl* m_coolrate;
    wxTextCtrl* m_maxit;
    wxTextCtrl* m_inits;
    wxButton* seedButton;
    
    wxString select_floor;
    wxString select_lisa;

    bool satisfy_min_bound;
    int conv_tabu;
    double initial_of;
    double final_of;
    DECLARE_EVENT_TABLE()
};

#endif
