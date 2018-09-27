//
//  SpatialJoinDlg.hpp
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//

#ifndef SpatialJoinDlg_hpp
#define SpatialJoinDlg_hpp

#include <wx/dialog.h>
#include <wx/choice.h>
#include "../SpatialIndTypes.h"

class Project;
class BackgroundMapLayer;
class MapLayerState;
class MapLayerStateObserver;

class SpatialJoinWorker
{
protected:
    rtree_pt_2d_t rtree;
    vector<wxInt64> spatial_counts;
    Project* project;
    BackgroundMapLayer* ml;
    int num_polygons;
    
public:
    SpatialJoinWorker(BackgroundMapLayer* ml, Project* project);
    virtual ~SpatialJoinWorker();
    
    void Run();
    void points_in_polygons(int start, int end);
    void polygon_at_point(int start, int end);
    vector<wxInt64> GetResults();
    
    virtual void sub_run(int start, int end) = 0;
};

class CountPointsInPolygon : public SpatialJoinWorker
{
public:
    CountPointsInPolygon(BackgroundMapLayer* ml, Project* project);
    virtual void sub_run(int start, int end);
};

class AssignPolygonToPoint : public SpatialJoinWorker
{
    vector<wxInt64> poly_ids;
public:
    AssignPolygonToPoint(BackgroundMapLayer* ml, Project* project, vector<wxInt64>& poly_ids);
    virtual void sub_run(int start, int end);
};

class SpatialJoinDlg : public wxDialog
{
    Project* project;
    wxChoice* map_list;
    wxChoice* field_list;
    wxStaticText* field_st;
    wxBoxSizer* vbox;
    wxBoxSizer* cbox;
    wxPanel* panel;
    
    void UpdateFieldList(wxString name);
    
public:
    SpatialJoinDlg(wxWindow* parent, Project* project);
    
    void OnOK(wxCommandEvent& e);
    void OnAddMapLayer(wxCommandEvent& e);
    void OnLayerSelect(wxCommandEvent& e);
    
    void InitMapList();
};

#endif /* SpatialJoinDlg_h */
