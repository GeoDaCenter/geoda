//
//  SpatialJoinDlg.hpp
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//

#ifndef SpatialJoinDlg_hpp
#define SpatialJoinDlg_hpp

#include <boost/thread/mutex.hpp>
#include <wx/dialog.h>
#include <wx/choice.h>
#include "../SpatialIndTypes.h"

class Project;
class BackgroundMapLayer;
class MapLayerState;
class MapLayerStateObserver;

class SpatialJoinWorker
{
public:
    enum Operation {NONE, MEAN, MEDIAN, STD, SUM};
    SpatialJoinWorker(BackgroundMapLayer* ml, Project* project);
    virtual ~SpatialJoinWorker();

    bool IsDuplicateCount();
    void Run();
    void points_in_polygons(int start, int end);
    void polygon_at_point(int start, int end);
    bool JoinVariable();
    std::vector<wxInt64> GetResults();
    std::vector<std::vector<double> > GetJoinResults();
    virtual void sub_run(int start, int end) = 0;

protected:
    Project* project;
    BackgroundMapLayer* ml;
    int num_polygons;
    boost::mutex mutex;

    bool is_spatial_assign;
    bool duplicate_count;

    // results
    std::vector<wxInt64> spatial_counts;
    std::vector<std::vector<double> > spatial_joins;

    // for join variable
    bool join_variable;
    
    std::vector<std::vector<double>  > join_values;
    std::vector<Operation> join_operation;
    
    std::vector<std::vector<wxInt64> > join_ids;
};

class CountPointsInPolygon : public SpatialJoinWorker
{
public:
    CountPointsInPolygon(BackgroundMapLayer* ml, Project* project,
                         std::vector<wxString> join_variable_nm,
                         std::vector<Operation> op);
    virtual void sub_run(int start, int end);
protected:
    rtree_pt_2d_t rtree;
};

class CountLinesInPolygon : public SpatialJoinWorker
{
public:
    CountLinesInPolygon(BackgroundMapLayer* ml, Project* project,
                        std::vector<wxString> join_variable_nm,
                        std::vector<Operation> op);
    virtual void sub_run(int start, int end);
protected:
    rtree_box_2d_t rtree;
};

class CountPolygonInPolygon : public SpatialJoinWorker
{
protected:
    rtree_box_2d_t rtree;
public:
    CountPolygonInPolygon(BackgroundMapLayer* ml, Project* project,
                          std::vector<wxString> join_variable_nm,
                          std::vector<Operation> op);
    virtual void sub_run(int start, int end);
};

class AssignPolygonToPoint : public SpatialJoinWorker
{
protected:
    std::vector<wxInt64> poly_ids;
    rtree_pt_2d_t rtree;
public:
    AssignPolygonToPoint(BackgroundMapLayer* ml, Project* project, std::vector<wxInt64>& poly_ids);
    virtual void sub_run(int start, int end);
};

class AssignPolygonToLine : public SpatialJoinWorker
{
protected:
    std::vector<wxInt64> poly_ids;
    rtree_box_2d_t rtree;
public:
    AssignPolygonToLine(BackgroundMapLayer* ml, Project* project, std::vector<wxInt64>& poly_ids);
    virtual void sub_run(int start, int end);
};



class SpatialJoinDlg : public wxDialog
{
    Project* project;
    wxChoice* map_list;
    wxListBox* join_var_list;
    wxChoice* join_op_list;
    wxChoice* field_list;
    wxStaticText* field_st;
    wxStaticText* join_var_st;
    wxStaticText* join_op_st;
    wxBoxSizer* vbox;
    wxBoxSizer* cbox;
    wxPanel* panel;
    wxListCtrl* lst_join;
    wxButton* move_left;
    wxButton* move_right;
    wxRadioButton* rb_spatial_count;
    wxRadioButton* rb_spatial_join;
    
    std::set<std::pair<wxString, wxString> > var_set;
    
    void UpdateFieldList(wxString name);
    std::list<int> GetListSel(wxListCtrl* lc);

public:
    SpatialJoinDlg(wxWindow* parent, Project* project);
    
    void OnOK(wxCommandEvent& e);
    void OnLayerSelect(wxCommandEvent& e);
    void OnJoinVariableSel(wxCommandEvent& e);
    void OnRemoveRow(wxCommandEvent& e);
    void OnAddRow(wxCommandEvent& e);
    void OnRadioButton(wxCommandEvent& e);
    void InitMapList();
};

#endif /* SpatialJoinDlg_h */
