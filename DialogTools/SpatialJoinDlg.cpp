//
//  SpatialJoinDlg.cpp
//  GeoDa
//
//  Created by Xun Li on 9/4/18.
//
#include <vector>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "../Project.h"
#include "../MapLayerStateObserver.h"
#include "../Explore/MapLayerTree.hpp"
#include "SaveToTableDlg.h"
#include "ProjectInfoDlg.h"
#include "ConnectDatasourceDlg.h"
#include "SpatialJoinDlg.h"

using namespace std;

SpatialJoinWorker::SpatialJoinWorker(BackgroundMapLayer* ml, Project* _project)
{
    project = _project;
    Shapefile::ShapeType shp_type = ml->GetShapeType();
    spatial_counts.resize(project->GetNumRecords());
    // not matter what shape type, just use centroids
    //if (shp_type == Shapefile::POINT_TYP) {
        int n = ml->shapes.size();
        double x, y;
        for (int i=0; i<n; i++) {
            x = ml->shapes[i]->center_o.x;
            y = ml->shapes[i]->center_o.y;
            rtree.insert(std::make_pair(pt_2d(x,y), i));
        }
    //}
}

SpatialJoinWorker::~SpatialJoinWorker()
{
    
}

vector<wxInt64> SpatialJoinWorker::GetResults()
{
    return spatial_counts;
}

void SpatialJoinWorker::Run()
{
    int initial = project->GetNumRecords();
    int nCPUs = boost::thread::hardware_concurrency();;
    int quotient = initial / nCPUs;
    int remainder = initial % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    boost::thread_group threadPool;
    for (int i=0; i<tot_threads; i++) {
        int a=0;
        int b=0;
        if (i < remainder) {
            a = i*(quotient+1);
            b = a+quotient;
        } else {
            a = remainder*(quotient+1) + (i-remainder)*quotient;
            b = a+quotient-1;
        }
        boost::thread* worker = new boost::thread(boost::bind(&SpatialJoinWorker::sub_run,this,a,b));
        threadPool.add_thread(worker);
    }
    
    threadPool.join_all();
}

void SpatialJoinWorker::sub_run(int start, int end)
{
    Shapefile::Main& main_data = project->main_data;
    OGRLayerProxy* ogr_layer = project->layer_proxy;
    Shapefile::PolygonContents* pc;
    for (int i=start; i<=end; i++) {
        pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
        // create a box, tl, br
        box_2d b(pt_2d(pc->box[0], pc->box[1]),
                 pt_2d(pc->box[2], pc->box[3]));
        // query points in this box
        std::vector<pt_2d_val> q;
        rtree.query(bgi::within(b), std::back_inserter(q));
        OGRGeometry* ogr_poly = ogr_layer->GetGeometry(i);
        for (int j=0; j<q.size(); j++) {
            const pt_2d_val& v = q[j];
            double x = v.first.get<0>();
            double y = v.first.get<1>();
            OGRPoint ogr_pt(x, y);
            if (ogr_pt.Within(ogr_poly)) {
                spatial_counts[i] += 1;
            }
        }
    }
}

SpatialJoinDlg::SpatialJoinDlg(wxWindow* parent, Project* _project)
: wxDialog(parent, -1, "Spatial Join", wxDefaultPosition, wxSize(350, 250))
{
    project = _project;
    wxPanel* panel = new wxPanel(this, -1);
    
    wxString info = _("Please select a map layer to apply spatial join to current map (%s):");
    info = wxString::Format(info, project->GetProjectTitle());
    wxStaticText* st = new wxStaticText(panel, -1, info);
    
    map_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1));
    InitMapList();
    
    wxBoxSizer* cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(st, 0, wxALIGN_CENTER | wxALL, 15);
    cbox->Add(map_list, 0, wxALIGN_CENTER | wxALL, 10);
    panel->SetSizerAndFit(cbox);
    
    wxButton* add_btn = new wxButton(this, XRCID("IDC_SPATIALJOIN_ADD_LAYER"), _("Add Map Layer"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(add_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxALL, 15);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    
    add_btn->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnAddMapLayer, this);
    ok_btn->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnOK, this);
}

void SpatialJoinDlg::InitMapList()
{
    map_list->Clear();
    map<wxString, BackgroundMapLayer*>::iterator it;
    for (it=project->bg_maps.begin(); it!=project->bg_maps.end(); it++) {
        wxString name = it->first;
        map_list->Append(name);
    }
    for (it=project->fg_maps.begin(); it!=project->fg_maps.end(); it++) {
        wxString name = it->first;
        map_list->Append(name);
    }
}

void SpatialJoinDlg::OnAddMapLayer(wxCommandEvent& e)
{
    ConnectDatasourceDlg connect_dlg(this, wxDefaultPosition, wxDefaultSize);
    if (connect_dlg.ShowModal() != wxID_OK) {
        return;
    }
    wxString proj_title = connect_dlg.GetProjectTitle();
    wxString layer_name = connect_dlg.GetLayerName();
    IDataSource* datasource = connect_dlg.GetDataSource();
    wxString datasource_name = datasource->GetOGRConnectStr();
    GdaConst::DataSourceType ds_type = datasource->GetType();
    
    BackgroundMapLayer* map_layer = project->AddMapLayer(datasource_name, ds_type, layer_name);
    if (map_layer) {
        map_list->Append(layer_name);
        MapLayerState* ml_state = project->GetMapLayerState();
        ml_state->notifyObservers();
    }
}

void SpatialJoinDlg::OnOK(wxCommandEvent& e)
{
    int layer_idx = map_list->GetSelection();
    if ( layer_idx < 0) {
        return;
    }
    
    wxString layer_name = map_list->GetString(layer_idx);
    BackgroundMapLayer* ml = NULL;
    ml = project->GetMapLayer(layer_name);
    if (ml) {
        SpatialJoinWorker sj(ml, project);
        sj.Run();
        
        vector<wxInt64> spatial_counts = sj.GetResults();
        // save results
        int new_col = 1;
        std::vector<SaveToTableEntry> new_data(new_col);
        vector<bool> undefs(project->GetNumRecords(), false);
        new_data[0].l_val = &spatial_counts;
        new_data[0].label = "Spatial Counts";
        new_data[0].field_default = "SC";
        new_data[0].type = GdaConst::long64_type;
        new_data[0].undefined = &undefs;
        SaveToTableDlg dlg(project, this, new_data,
                           "Save Results: Spatial Counts",
                           wxDefaultPosition, wxSize(400,400));
        dlg.ShowModal();
    }
}

