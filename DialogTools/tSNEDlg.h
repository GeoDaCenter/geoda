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

#ifndef __GEODA_CENTER_TSNE_DLG_H__
#define __GEODA_CENTER_TSNE_DLG_H__

#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>

#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class TSNEDlg : public AbstractClusterDlg
{
public:
    TSNEDlg(wxFrame *parent, Project* project);
    virtual ~TSNEDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );

    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();

    double _calculateRankCorr(char dist, int rows, double **ragged_distances,
                              const std::vector<std::vector<double> >& result);
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:

    wxChoice* m_distance;

    wxTextCtrl* txt_iteration;
    wxTextCtrl* txt_perplexity;
    wxTextCtrl* txt_theta;
    wxTextCtrl* txt_momentum;
    wxTextCtrl* txt_finalmomentum;
    wxTextCtrl* txt_mom_switch_iter;
    wxTextCtrl* txt_learningrate;
    wxTextCtrl* txt_outdim;

    wxCheckBox* chk_seed;
    wxButton* seedButton;

    wxStaticText* lbl_poweriteration;

    SimpleReportTextCtrl *m_textbox;
    
    DECLARE_EVENT_TABLE()
};

#endif
