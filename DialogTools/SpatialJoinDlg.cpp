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

SpatialJoinWorker::SpatialJoinWorker(BackgroundMapLayer* _ml, Project* _project)
{
    ml = _ml;
    project = _project;
    spatial_counts.resize(project->GetNumRecords());
    
    // always use points to create a rtree, since in normal case
    // the number of points are larger than the number of polygons
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
    int initial = num_polygons;
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

CountPointsInPolygon::CountPointsInPolygon(BackgroundMapLayer* _ml, Project* _project)
: SpatialJoinWorker(_ml, _project)
{
    num_polygons = project->GetNumRecords();
    
    // using selected layer (points) to create rtree
    int n = ml->shapes.size();
    double x, y;
    for (int i=0; i<n; i++) {
        x = ml->shapes[i]->center_o.x;
        y = ml->shapes[i]->center_o.y;
        rtree.insert(std::make_pair(pt_2d(x,y), i));
    }
}

void CountPointsInPolygon::sub_run(int start, int end)
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
AssignPolygonToPoint::AssignPolygonToPoint(BackgroundMapLayer* _ml, Project* _project, vector<wxInt64>& _poly_ids)
: SpatialJoinWorker(_ml, _project)
{
    poly_ids = _poly_ids;
    num_polygons = ml->GetNumRecords();
    // using current map(points) to create rtree
    Shapefile::Main& main_data = project->main_data;
    Shapefile::PointContents* pc;
    int n = project->GetNumRecords();
    double x, y;
    for (int i=0; i<n; i++) {
        pc = (Shapefile::PointContents*)main_data.records[i].contents_p;
        x = pc->x;
        y = pc->y;
        rtree.insert(std::make_pair(pt_2d(x,y), i));
        spatial_counts[i] = -1;
    }
}

void AssignPolygonToPoint::sub_run(int start, int end)
{
    // for every polygon in sub-layer
    for (int i=start; i<=end; i++) {
        OGRGeometry* ogr_poly = ml->geoms[i];
        // create a box, tl, br
        OGREnvelope box;
        ogr_poly->getEnvelope(&box);
        
        box_2d b(pt_2d(box.MinX, box.MinY), pt_2d(box.MaxX, box.MaxY));
        // query points in this box
        std::vector<pt_2d_val> q;
        rtree.query(bgi::within(b), std::back_inserter(q));
        for (int j=0; j<q.size(); j++) {
            const pt_2d_val& v = q[j];
            int pt_idx = v.second;
            double x = v.first.get<0>();
            double y = v.first.get<1>();
            OGRPoint ogr_pt(x, y);
            if (ogr_pt.Within(ogr_poly)) {
                spatial_counts[pt_idx] = poly_ids[i];
            }
        }
    }
}


SpatialJoinDlg::SpatialJoinDlg(wxWindow* parent, Project* _project)
: wxDialog(parent, -1, "Spatial Join", wxDefaultPosition, wxSize(350, 250))
{
    project = _project;
    panel = new wxPanel(this, -1);
    
    wxString info = _("Please select a map layer to apply spatial join to current map (%s):");
    info = wxString::Format(info, project->GetProjectTitle());
    wxStaticText* st = new wxStaticText(panel, -1, info);
    
    map_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    field_st = new wxStaticText(panel, -1, "Select ID Variable (Optional)");
    field_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxBoxSizer* mbox = new wxBoxSizer(wxHORIZONTAL);
    mbox->Add(map_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_st, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_list, 0, wxALIGN_CENTER | wxALL, 5);
    
    cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(st, 0, wxALIGN_CENTER | wxALL, 15);
    cbox->Add(mbox, 0, wxALIGN_CENTER | wxALL, 10);
    panel->SetSizerAndFit(cbox);
    
    wxButton* add_btn = new wxButton(this, XRCID("IDC_SPATIALJOIN_ADD_LAYER"), _("Add Map Layer"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,  wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(add_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxALL, 15);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    
    map_list->Bind(wxEVT_CHOICE, &SpatialJoinDlg::OnLayerSelect, this);
    add_btn->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnAddMapLayer, this);
    ok_btn->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnOK, this);
    
    InitMapList();
    field_st->Disable();
    field_list->Disable();
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
    if (map_list->GetCount() > 0) {
        // check the first item in list
        wxString name = map_list->GetString(0);
        UpdateFieldList(name);
    }
}

void SpatialJoinDlg::UpdateFieldList(wxString name)
{
    BackgroundMapLayer* ml = project->GetMapLayer(name);
    if (ml) {
        if (Shapefile::POLYGON == ml->GetShapeType() &&
            project->IsPointTypeData()) {
            // assign polygon to point
            field_list->Clear();
            vector<wxString> field_names = ml->GetIntegerFieldNames();
            field_list->Append("");
            for (int i=0; i<field_names.size(); i++) {
                field_list->Append(field_names[i]);
            }
            field_list->Enable();
            field_st->Enable();
            
        } else {
            field_list->Clear();
            field_list->Disable();
            field_st->Disable();
        }
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
        UpdateFieldList(layer_name);
        MapLayerState* ml_state = project->GetMapLayerState();
        ml_state->notifyObservers();
    }
}

void SpatialJoinDlg::OnLayerSelect(wxCommandEvent& e)
{
    int layer_idx = map_list->GetSelection();
    if ( layer_idx < 0) {
        return;
    }
    wxString layer_name = map_list->GetString(layer_idx);
    UpdateFieldList(layer_name);
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
        int n = ml->GetNumRecords();
        if (project->IsPointTypeData() &&
            ml->GetShapeType() == Shapefile::POINT_TYP) {
            wxMessageDialog dlg (this, _("Spatial Join can not be applied on two points layers. Please select another layer."), _("Warning"), wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
            return;
        }
        wxString label = "Spatial Count";
        wxString field_name = "SC";
        
        SpatialJoinWorker* sj;
        if (project->IsPointTypeData()) {
            vector<wxInt64> poly_ids;
            for (int i=0; i<n; i++) {
                poly_ids.push_back(i);
            }
            
            int field_idx = field_list->GetSelection();
            if (field_idx > 0) {
                wxString field_name = field_list->GetString(field_idx);
                bool success = ml->GetIntegerColumnData(field_name, poly_ids);
                if ( !success || poly_ids.size() != n) {
                    wxMessageDialog dlg (this, _("Select field is not integer type. Default record order will be used instead."), _("Warning"), wxOK | wxICON_INFORMATION);
                    dlg.ShowModal();
                    poly_ids.clear();
                }
            }
            sj = new AssignPolygonToPoint(ml, project, poly_ids);
            label = "Spatial Assign";
            field_name = "SA";
        } else {
            sj = new CountPointsInPolygon(ml, project);
        }
        sj->Run();
        
        wxString dlg_title = _("Save Results: ") + label;
        vector<wxInt64> spatial_counts = sj->GetResults();
        // save results
        int new_col = 1;
        std::vector<SaveToTableEntry> new_data(new_col);
        vector<bool> undefs(project->GetNumRecords(), false);
        new_data[0].l_val = &spatial_counts;
        new_data[0].label = label;
        new_data[0].field_default = field_name;
        new_data[0].type = GdaConst::long64_type;
        new_data[0].undefined = &undefs;
        SaveToTableDlg dlg(project, this, new_data, dlg_title,
                           wxDefaultPosition, wxSize(400,400));
        dlg.ShowModal();
        
        delete sj;
    }
}

