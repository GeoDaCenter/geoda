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
#include <wx/wx.h>
#include <wx/notebook.h>

#include "../GeneralWxUtils.h"
#include "../FramesManager.h"
#include "../VarTools.h"
#include "../DataViewer/TableStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../ShapeOperations/GalWeight.h"

class Project;
class TableInterface;

// Abstract class for Cluster Dialog
class AbstractClusterDlg : public wxDialog, public FramesManagerObserver,
    public TableStateObserver, public WeightsManStateObserver
{
public:
    AbstractClusterDlg(wxFrame *parent, Project* project, wxString title);
    virtual ~AbstractClusterDlg();

    // Clean the table data used in Cluster Dialog; Used for refresh the dialog
    void CleanData();
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    
    /** Implementation of TableStateObserver interface */
    virtual void update(TableState* o);
    virtual bool AllowTimelineChanges() { return true; }
    virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
    virtual bool AllowObservationAddDelete() { return false; }

    /** Implementation of WeightsManStateObserver interface */
    virtual void update(WeightsManState* o);
    virtual int numMustCloseToRemove(boost::uuids::uuid id) const { return 0; }
    virtual void closeObserver(boost::uuids::uuid id) {};

protected:
    static bool check_spatial_ref;
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    FramesManager* frames_manager;
    TableState* table_state;
    WeightsManState* w_man_state;
    GalElement* gal;
    
    std::vector<std::vector<double> > z;
    std::vector<bool> undefs;

    // number of selected variables in table
    int num_vars;

    // number of observations
    int rows;

    // number of columns
    int columns;

    // column names
    std::vector<wxString> col_names;

    // time/group names
    std::vector<wxString> tm_strs;
    //
    std::map<wxString, wxString> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    std::map<int, double> idx_sum;

    std::vector<double> cent_xs;
    std::vector<double> cent_ys;

    wxTextValidator validator;
    wxArrayString var_items;

    virtual bool Init();

    virtual bool CheckAllInputs();
    
    virtual bool Run(std::vector<wxInt64>& clusters) { return false;}
    
    virtual double* GetWeights(int columns);
    
    virtual double GetMinBound();
   
    virtual double* GetBoundVals();
   
    // Utils
    bool IsUseCentroids();
    bool CheckConnectivity(GalWeight* gw);
    bool CheckConnectivity(GalElement* W);
    
    // Input related
    bool has_x_cent;
    bool has_y_cent;
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    std::vector<wxString> select_vars;
    double* weight;
    double** input_data;
    int** mask;
    // -- controls
    wxListBox* combo_var;
    wxCheckBox* m_use_centroids;
    wxButton* auto_btn;
    wxSlider* m_weight_centroids;
    wxTextCtrl* m_wc_txt;
    wxStaticText* st_spatial_w;
    wxChoice* m_spatial_weights;
    wxBitmapButton* weights_btn;
    // -- functions
    virtual void AddInputCtrls(wxPanel *panel, wxBoxSizer* vbox,
                               bool show_auto_button = false,
                               bool integer_only = false,
                               bool show_spatial_weights = true,
                               bool single_variable = false,
                               bool add_centroids = true);
    virtual void AddSimpleInputCtrls(wxPanel *panel, wxBoxSizer* vbox,
                                     bool integer_only = false,
                                     bool show_spatial_weights = false,
                                     bool add_centroids = true,
                                     bool multiple_selection = true,
                                     wxString header = _("Select Variables"));
    virtual void OnUseCentroids(wxCommandEvent& event);
    virtual void OnSlideWeight(wxCommandEvent& event);
    virtual void InitVariableCombobox(wxListBox* var_box,
                                      bool integer_only=false,
                                      bool add_centroids=true);
    virtual bool GetInputData(int transform, int min_num_var=2);
    virtual bool CheckEmptyColumn(int col_id, int time);
    virtual void OnInputWeights(wxCommandEvent& event);

    virtual bool CheckContiguity(GalWeight* weights, double w, double& ssd);
    virtual bool CheckContiguity(GalElement* gal, std::vector<wxInt64>& clusters);
    virtual double BinarySearch(GalWeight* weights, double left, double right);
    virtual void OnAutoWeightCentroids(wxCommandEvent& event);
    virtual void InitSpatialWeights(wxChoice* combo_weights);
    virtual GalWeight* GetInputSpatialWeights();
    virtual GalWeight* CheckSpatialWeights();
    virtual void OnSpatialWeights(wxCommandEvent& event);

    // Transformation control
    // -- variables
    wxChoice* combo_tranform;
    // -- functions;
    virtual void AddTransformation(wxPanel* panel, wxFlexGridSizer* gbox);
    
    // Minimum Bound related
	// -- variables
    wxCheckBox* chk_floor;
    wxChoice* combo_floor;
    wxTextCtrl* txt_floor;
    wxTextCtrl* txt_floor_pct;
    wxSlider* slider_floor;
	// -- functions
    virtual void AddMinBound(wxPanel *panel, wxFlexGridSizer* gbox, bool show_checkbox=true);
    virtual void OnCheckMinBound(wxCommandEvent& event);
    virtual void OnSelMinBound(wxCommandEvent& event);
    virtual void OnTypeMinBound(wxKeyEvent& event);
    virtual void OnTypeMinPctBound(wxKeyEvent& event);
    virtual void OnSlideMinBound(wxCommandEvent& event);
    virtual bool CheckMinBound();

    // output controls
    wxComboBox* combo_n;
    int max_n_clusters;
    virtual void AddNumberOfClusterCtrl(wxPanel *panel, wxFlexGridSizer* gbox,
                                        bool allow_dropdown = true);

    // Summary related
    // The main statistics should be:
    // - mean centers or centroids of each cluster in terms of the variables involved
    // - the total sum of squares
    // - the within sum of squares
    // - the between sum of squares
    // - the ratio of between to total sum of squares
	// -- variables
    wxString mean_center_type;
    SimpleReportTextCtrl* m_reportbox;
	wxNotebook* AddSimpleReportCtrls(wxPanel *panel);
    bool return_additional_summary;
	// -- functions
    virtual double _getTotalSumOfSquares(const std::vector<bool>& noises);
    virtual double _calcSumOfSquares(const std::vector<int>& cluster_ids);
    virtual std::vector<std::vector<double> > _getMeanCenters(const std::vector<std::vector<int> >& solution);
    virtual std::vector<double> _getWithinSumOfSquares(const std::vector<std::vector<int> >& solution);
    virtual wxString _printMeanCenters(const std::vector<std::vector<double> >& mean_centers);
    virtual wxString _printWithinSS(const std::vector<double>& within_ss,
                            const wxString& title = _("Within-cluster sum of squares:\n"),
                            const wxString& header = _("Within cluster S.S."));
    virtual wxString _printWithinSS(const std::vector<double>& within_ss,
                            const std::vector<double>& avgs,
                            const wxString& title = _("Within-cluster sum of squares:\n"),
                            const wxString& header1 = _("Within cluster S.S."),
                            const wxString& header2 = _("Averages"));
    virtual wxString _printConfiguration()=0;
    virtual double CreateSummary(const std::vector<wxInt64>& clusters,
                         bool show_print = true,
                         bool return_additional_summary = false);
    virtual double CreateSummary(const std::vector<std::vector<int> >& solution,
                         const std::vector<int>& isolated = std::vector<int>(),
                         bool show_print = true,
                         bool return_additional_summary = false);
    
    // get addtional content for summary,e.g. medoids (within distance to median)
    virtual wxString _additionalSummary(const std::vector<std::vector<int> >& solution,
                                        double& additional_ratio) { return wxEmptyString;}
};

#endif
