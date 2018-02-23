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
#include "AbstractClusterDlg.h"

using namespace std;

class Project;
class TableInterface;

class KClusterDlg : public AbstractClusterDlg
{
public:
    KClusterDlg(wxFrame *parent, Project* project, wxString title="");
    virtual ~KClusterDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnDistanceChoice(wxCommandEvent& event);
    void OnInitMethodChoice(wxCommandEvent& event);
   
    void BinarySearch(double left, double right, std::vector<std::pair<double, double> >& ssd_pairs);
    bool CheckContiguity(double w, double& ss);
    void Run(vector<wxInt64>& clusters);
    
    virtual void ComputeDistMatrix(int dist_sel);
    virtual wxString _printConfiguration();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals)=0;
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;

protected:
    virtual void OnAutoWeightCentroids(wxCommandEvent& event);
    
    bool show_initmethod;
    bool show_distance;
    bool show_iteration;
    
    wxCheckBox* chk_seed;
    wxChoice* combo_method;
    wxChoice* combo_n;
    wxChoice* combo_cov;
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_iterations;
    wxTextCtrl* m_pass;
    wxChoice* m_distance;
    wxButton* seedButton;
   
    wxString cluster_method;
    
    unsigned int row_lim;
    unsigned int col_lim;
    std::vector<float> scores;
    double thresh95;
    int max_n_clusters;
    double** distmatrix;
    
    map<double, vector<wxInt64> > sub_clusters;
    GalElement* gal;
    
    DECLARE_EVENT_TABLE()
};

class KMeansDlg : public KClusterDlg
{
public:
    KMeansDlg(wxFrame *parent, Project* project);
    virtual ~KMeansDlg();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
};

class KMediansDlg : public KClusterDlg
{
public:
    KMediansDlg(wxFrame *parent, Project* project);
    virtual ~KMediansDlg();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
};

class KMedoidsDlg : public KClusterDlg
{
public:
    KMedoidsDlg(wxFrame *parent, Project* project);
    virtual ~KMedoidsDlg();
   
    virtual void ComputeDistMatrix(int dist_sel);
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
};
#endif
