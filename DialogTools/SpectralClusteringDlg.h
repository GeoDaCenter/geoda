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

#ifndef __GEODA_CENTER_SPECTRAL_DLG_H___
#define __GEODA_CENTER_SPECTRAL_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>
#include <wx/combobox.h>
#include <Eigen/Dense>

using namespace Eigen;

#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class Project;
class TableInterface;
	
class SpectralClusteringDlg : public AbstractClusterDlg
{
public:
    SpectralClusteringDlg(wxFrame *parent, Project* project);
    virtual ~SpectralClusteringDlg();
    
    void CreateControls();
    virtual bool Init();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnWeightsCheck(wxCommandEvent& event);
    void OnKernelCheck(wxCommandEvent& event);
    void OnKNNCheck(wxCommandEvent& event);
    void OnMutualKNNCheck(wxCommandEvent& event);
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    
    virtual void InitVariableCombobox(wxListBox* var_box);
   
    virtual wxString _printConfiguration();

protected:
    virtual bool Run(std::vector<wxInt64>& clusters);
    virtual bool CheckAllInputs();
    void UpdateGaussian(wxCommandEvent& event);

protected:
    int transform;
    int n_cluster;
    double value_sigma;
    int knn;
    int mutual_knn;
    char method;
    int npass;
    int n_maxiter;
    char dist;
    int affinity_type;
    std::vector<wxInt64> clusters;

    wxCheckBox* chk_seed;
    wxChoice* combo_method;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_iterations;
    wxTextCtrl* m_pass;
    
    wxComboBox* m_sigma;
    wxChoice* combo_kernel;
    wxChoice* m_method;
    wxChoice* m_distance;
    wxCheckBox* chk_kernel;
    wxStaticText* lbl_kernel;
    wxStaticText* lbl_sigma;
    
    wxCheckBox* chk_knn;
    wxCheckBox* chk_mknn;
    wxStaticText* lbl_knn;
    wxStaticText* lbl_neighbors;
    wxStaticText* lbl_m_neighbors;
    wxComboBox* m_knn;
    wxComboBox* m_mknn;
    
    wxStaticText* lbl_weights;
    wxCheckBox* chk_weights;
    wxChoice* combo_weights;
    
    wxButton* seedButton;
    
    DECLARE_EVENT_TABLE()
};

#endif
