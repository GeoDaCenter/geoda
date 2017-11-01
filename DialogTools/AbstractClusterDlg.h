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

#ifndef __GEODA_CENTER_ABSTRACTCLUSTER_DLG_H___
#define __GEODA_CENTER_ABSTRACTCLUSTER_DLG_H___

#include <vector>
#include <map>
#include <wx/choice.h>
#include <wx/checklst.h>
#include <wx/notebook.h>

#include "../GeneralWxUtils.h"
#include "../FramesManager.h"
#include "../VarTools.h"

using namespace std;

class Project;
class TableInterface;
	
class AbstractClusterDlg : public wxDialog, public FramesManagerObserver
{
public:
    AbstractClusterDlg(wxFrame *parent, Project* project, wxString title);
    virtual ~AbstractClusterDlg();
   
    void CleanData();
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    
protected:
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    FramesManager* frames_manager;
    
    vector<vector<double> > z;
    vector<bool> undefs;
    int num_vars;
   
    int rows;
    int columns;
    int num_obs;
    
    std::vector<wxString> tm_strs;
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    std::map<int, double> idx_sum;
    
    wxTextValidator validator;
    wxArrayString var_items;
   
    virtual bool Init();
    
    virtual double* GetWeights(int columns);
    
    virtual double GetMinBound();
   
    virtual double* GetBoundVals();
    
    // Input related
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    double* weight;
    double** input_data;
    int** mask;
    
    wxListBox* combo_var;
    wxCheckBox* m_use_centroids;
    wxSlider* m_weight_centroids;
    wxTextCtrl* m_wc_txt;
    
    virtual void AddInputCtrls(
       wxPanel *panel,
       wxListBox** combo_var,
       wxCheckBox** m_use_centroids,
       wxSlider** m_weight_centroids,
       wxTextCtrl** m_wc_txt,
       wxBoxSizer* vbox);
    
    virtual void AddSimpleInputCtrls(
        wxPanel *panel,
        wxListBox** combo_var,
        wxBoxSizer* vbox,
        bool integer_only = false);
    
    void OnUseCentroids(wxCommandEvent& event);
    void OnSlideWeight(wxCommandEvent& event);
    virtual void InitVariableCombobox(wxListBox* var_box, bool integer_only=false);
    bool GetInputData(int transform, int min_num_var=2);
    void OnInputWeights(wxCommandEvent& event);
   
    // Minimum Bound related
    wxCheckBox* chk_floor;
    wxChoice* combo_floor;
    wxTextCtrl* txt_floor;
    wxTextCtrl* txt_floor_pct;
    wxSlider* slider_floor;
    
    virtual void AddMinBound(
        wxPanel *panel,
        wxCheckBox** chk_floor,
        wxChoice** combo_floor,
        wxTextCtrl** txt_floor,
        wxSlider** slider_floor,
        wxTextCtrl** txt_floor_pct,
        wxFlexGridSizer* gbox,
        bool show_checkbox=true);
    
    virtual void  OnCheckMinBound(wxCommandEvent& event);
    virtual void  OnSelMinBound(wxCommandEvent& event);
    virtual void  OnTypeMinBound(wxCommandEvent& event);
    virtual void  OnSlideMinBound(wxCommandEvent& event);
    
    // Summary related
    SimpleReportTextCtrl* m_reportbox;
    
    void AddSimpleReportCtrls(wxNotebook* myNotebook, SimpleReportTextCtrl** m_textbox);
    
    void GetClusterSummary(const vector<wxInt64>& clusters);
    
    void GetClusterSummary(const vector<vector<int> >& solution);
    
    double calcHeterogeneity(const vector<int>& cluster_ids);
    
    wxString CreateSummary(
        const vector<vector<int> >& solution,
        const vector<double>& wss);
    
    void get_centroids(
        const vector<vector<int> >& solution,
                       vector<GdaPoint*>& centroids);
    
    void get_mean_centers(const vector<vector<int> >& solution,
                          vector<GdaPoint*>& centers);
};

#endif
