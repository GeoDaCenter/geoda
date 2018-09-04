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

class SpatialJoinWorker
{
    rtree_pt_2d_t rtree;
    vector<wxInt64> spatial_counts;
    Project* project;
    
public:
    SpatialJoinWorker(BackgroundMapLayer* ml, Project* project);
    ~SpatialJoinWorker();
    
    void Run();
    void sub_run(int start, int end);
    vector<wxInt64> GetResults();
};

class SpatialJoinDlg : public wxDialog
{
    Project* project;
    wxChoice* map_list;
    
public:
    SpatialJoinDlg(wxWindow* parent, Project* project);
    
    void OnOK(wxCommandEvent& e);
    void OnAddMapLayer(wxCommandEvent& e);
    
    void InitMapList();
};

#endif /* SpatialJoinDlg_h */
