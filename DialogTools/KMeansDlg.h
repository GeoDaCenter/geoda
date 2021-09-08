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
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/combobox.h>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"

using namespace std;

class Project;
class TableInterface;

class MakeSpatialDlg : public AbstractClusterDlg
{
public:
    MakeSpatialDlg(wxFrame *parent, Project* project);
    virtual ~MakeSpatialDlg();
    
    virtual void CreateControls();
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    virtual wxString _printConfiguration();
    
private:
    wxTextCtrl* m_textbox;
    wxTextCtrl* m_max_region;
    

    wxButton* seedButton;
    wxButton* saveButton;
        
    DECLARE_EVENT_TABLE()
};

class KClusterDlg : public AbstractClusterDlg
{
public:
    KClusterDlg(wxFrame *parent, Project* project, wxString title="");
    virtual ~KClusterDlg();
    
    virtual void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void OnDistanceChoice(wxCommandEvent& event);
    void OnInitMethodChoice(wxCommandEvent& event);

    virtual void ComputeDistMatrix(int dist_sel);
    virtual wxString _printConfiguration();
    virtual vector<vector<double> > _getMeanCenters(const vector<vector<int> >& solution);
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals)=0;
    
    //std::vector<GdaVarTools::VarInfo> var_info;
    //std::vector<int> col_ids;

protected:
    virtual bool Run(vector<wxInt64>& clusters);
    virtual bool CheckAllInputs();

    int n_cluster;
    int transform;
    int n_pass;
    int n_maxiter;
    int meth_sel;
    int dist_sel;
    
    bool show_initmethod;
    bool show_distance;
    bool show_iteration;
    
    wxCheckBox* chk_seed;
    wxChoice* combo_method;

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
    
    DECLARE_EVENT_TABLE()
};

////////////////////////////////////////////////////////////////////////
//
// SpatialKMeansDlg
////////////////////////////////////////////////////////////////////////

class SpatialKMeansDlg : public KClusterDlg
{
public:
    SpatialKMeansDlg(wxFrame *parent, Project* project);
    virtual ~SpatialKMeansDlg();
    
    virtual void CreateControls();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
    
    virtual wxString _printConfiguration();
    
    void OnOK( wxCommandEvent& event );
};

////////////////////////////////////////////////////////////////////////
//
// KMeansDlg
////////////////////////////////////////////////////////////////////////

class KMeansDlg : public KClusterDlg
{
public:
    KMeansDlg(wxFrame *parent, Project* project);
    virtual ~KMeansDlg();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
};

////////////////////////////////////////////////////////////////////////
//
// KMediansDlg
////////////////////////////////////////////////////////////////////////
class KMediansDlg : public KClusterDlg
{
public:
    KMediansDlg(wxFrame *parent, Project* project);
    virtual ~KMediansDlg();
    
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
    virtual vector<vector<double> > _getMeanCenters(const vector<vector<int> >& solution);
    
protected:
    // get addtional content for summary,e.g. medoids (within distance to median)
    virtual wxString _additionalSummary(const vector<vector<int> >& solution,
                                        double& additional_ratio);

    double _calcSumOfSquaresMedian(const vector<int>& cluster_ids);
    
    double _calcSumOfManhattanMedian(const vector<int>& cluster_ids);
};

////////////////////////////////////////////////////////////////////////
//
// KMedoids
////////////////////////////////////////////////////////////////////////
class KMedoidsDlg : public KClusterDlg
{
public:
    KMedoidsDlg(wxFrame *parent, Project* project);
    virtual ~KMedoidsDlg();

    virtual void CreateControls();

    virtual void ComputeDistMatrix(int dist_sel);
    virtual void doRun(int s1, int ncluster, int npass, int n_maxiter, int meth_sel, int dist_sel, double min_bound, double* bound_vals);
    virtual vector<vector<double> > _getMeanCenters(const vector<vector<int> >& solution);
    virtual wxString _printConfiguration();
    void OnMethodChoice(wxCommandEvent& evt);

protected:
    virtual bool Run(vector<wxInt64>& clusters);
    virtual bool CheckAllInputs();
    
    // get addtional content for summary,e.g. medoids (within distance to median)
    virtual wxString _additionalSummary(const vector<vector<int> >& solution,
                                        double& additional_ratio);

    int GetFirstMedoid(double** distmatrix);
    
    double _calcSumOfSquaresMedoid(const vector<int>& cluster_ids, int medoid_idx);
    
    double _calcSumOfManhattanMedoid(const vector<int>& cluster_ids, int medoid_idx);
        
    wxStaticText* txt_iterations;
    wxStaticText* txt_initmethod;
    wxChoice* combo_initmethod;
    wxCheckBox* m_fastswap;
    wxStaticText* txt_numsamples;
    wxTextCtrl* m_numsamples;
    wxStaticText* txt_sampling;
    wxTextCtrl* m_sampling;
    wxCheckBox* m_keepmed;

    std::vector<int> medoid_ids;
    // first medoid is the medoid when cluster number is specified to 1
    int first_medoid;
};
#endif
