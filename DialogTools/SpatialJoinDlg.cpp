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
#include <boost/foreach.hpp>

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
    duplicate_count = false;
    ml = _ml;
    project = _project;
    int n_joins = project->GetNumRecords();
    spatial_counts.resize(n_joins);
    //spatial_joins.resize(n_joins);
    join_ids.resize(n_joins);
    // always use points to create a rtree, since in normal case
    // the number of points are larger than the number of polygons
}

SpatialJoinWorker::~SpatialJoinWorker()
{

}

bool SpatialJoinWorker::IsDuplicateCount()
{
    return duplicate_count;
}

bool SpatialJoinWorker::JoinVariable()
{
    return join_variable;
}

std::vector<std::vector<double> > SpatialJoinWorker::GetJoinResults()
{
    return spatial_joins;
}

vector<wxInt64> SpatialJoinWorker::GetResults()
{
     return spatial_counts;
}

void SpatialJoinWorker::Run()
{
    int initial = num_polygons;
    int nCPUs = boost::thread::hardware_concurrency();
    if (GdaConst::gda_set_cpu_cores) nCPUs = GdaConst::gda_cpu_cores;
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

    // check if duplicated counting
    int n_joins = project->GetNumRecords();
    if (is_spatial_assign == false) {
        wxInt64 sum = 0;
        for (size_t i=0; i<spatial_counts.size(); i++) {
            sum += spatial_counts[i];
        }
        if (sum > ml->GetNumRecords()) {
            duplicate_count = true;
        }
    } 

    // join variable if needed
    if (join_variable) {
        int n_vars = (int)join_values.size();
        spatial_joins.resize(n_vars);
        for (size_t k=0; k<n_vars; ++k) {
            spatial_joins[k].resize(n_joins);
        }
        for (size_t i=0; i<n_joins; ++i) {
            size_t cnt = join_ids[i].size();
            
            for (size_t k=0; k<n_vars; ++k) {
                std::vector<double> vals(cnt, 0);
                
                for (size_t j=0; j<cnt; ++j) {
                    int idx = (int)join_ids[i][j];
                    vals[j] = join_values[k][idx];
                }
                
                if (join_operation[k] == STD) {
                    double variance = GenUtils::GetVariance(vals);
                    spatial_joins[k][i] = sqrt(variance);
                } else if (join_operation[k] == SUM) {
                    double sum = GenUtils::Sum(vals);
                    spatial_joins[k][i] = sum;
                } else if (join_operation[k] == MEAN) {
                    double sum = GenUtils::Sum(vals);
                    spatial_joins[k][i] = cnt == 0? 0 : sum / cnt;
                } else if (join_operation[k] == MEDIAN) {
                    double median = GenUtils::Median(vals);
                    spatial_joins[k][i] = median;
                }
            }
        }
    }
}


CountPointsInPolygon::CountPointsInPolygon(BackgroundMapLayer* _ml,
                                           Project* _project,
                                           std::vector<wxString> join_variable_nm,
                                           std::vector<Operation> _op)
: SpatialJoinWorker(_ml, _project)
{
    is_spatial_assign = false;
    join_operation = _op;
    int n_vars = (int)join_operation.size();
    join_values.resize(n_vars);
    join_variable = false; // default false: no need to join variables
    for (int i=0; i<join_variable_nm.size(); ++i) {
        join_variable = _ml->GetDoubleColumnData(join_variable_nm[i], join_values[i]);
    }

    num_polygons = project->GetNumRecords();

    // using selected layer (points) to create rtree
    int n = (int)ml->shapes.size();
    double x, y;
    for (int i=0; i<n; i++) {
        if (ml->shapes[i]) {
            x = ml->shapes[i]->center_o.x;
            y = ml->shapes[i]->center_o.y;
            rtree.insert(std::make_pair(pt_2d(x,y), i));
        }
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
            int pt_idx = v.second;
            double x = v.first.get<0>();
            double y = v.first.get<1>();
            OGRPoint ogr_pt(x, y);
            if (ogr_pt.Within(ogr_poly)) {
                if (i == 12) {
                    std::cout << pt_idx << std::endl;
                }
                spatial_counts[i] += 1;
                if (join_variable) {
                    mutex.lock();
                    join_ids[i].push_back(pt_idx);
                    mutex.unlock();
                }
            }
        }
    }
}



AssignPolygonToPoint::AssignPolygonToPoint(BackgroundMapLayer* _ml,
                                Project* _project, vector<wxInt64>& _poly_ids)
: SpatialJoinWorker(_ml, _project)
{
    join_variable = false;
    is_spatial_assign = true;
    poly_ids = _poly_ids;
    num_polygons = ml->GetNumRecords();
    // using current map(points) to create rtree
    Shapefile::Main& main_data = project->main_data;
    OGRLayerProxy* ogr_layer = project->layer_proxy;
    Shapefile::PointContents* pc;
    int n = project->GetNumRecords();
    double x, y;
    for (int i=0; i<n; i++) {
        OGRGeometry* ogr_pt = ogr_layer->GetGeometry(i);
        if (ogr_pt) {
            pc = (Shapefile::PointContents*)main_data.records[i].contents_p;
            x = pc->x;
            y = pc->y;
            rtree.insert(std::make_pair(pt_2d(x,y), i));
        }
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


CountLinesInPolygon::CountLinesInPolygon(BackgroundMapLayer* _ml,
                                         Project* _project,
                                         std::vector<wxString> join_variable_nm,
                                         std::vector<Operation> _op)
: SpatialJoinWorker(_ml, _project)
{
    is_spatial_assign = false;
    join_operation = _op;
    int n_vars = (int)join_operation.size();
    join_values.resize(n_vars);
    join_variable = false; // default false: no need to join variables
    for (int i=0; i<join_variable_nm.size(); ++i) {
        join_variable = _ml->GetDoubleColumnData(join_variable_nm[i], join_values[i]);
    }
    num_polygons = project->GetNumRecords();

    // using selected layer (lines) to create rtree
    int n = (int)ml->shapes.size();
    for (int i=0; i<n; i++) {
        OGRGeometry* geom = ml->geoms[i];
        OGREnvelope bbox;
        geom->getEnvelope(&bbox);
        pt_2d ll(bbox.MinX, bbox.MinY);
        pt_2d ur(bbox.MaxX, bbox.MaxY);
        box_2d b(ll, ur);
        rtree.insert(std::make_pair(b, i));
    }
}

void CountLinesInPolygon::sub_run(int start, int end)
{
    Shapefile::Main& main_data = project->main_data;
    OGRLayerProxy* ogr_layer = project->layer_proxy;
    Shapefile::PolygonContents* pc;
    for (int i=start; i<=end; i++) {
        pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
        // create a box, tl, br
        box_2d b(pt_2d(pc->box[0], pc->box[1]),
                 pt_2d(pc->box[2], pc->box[3]));
        // query boxes in this box
        std::vector<box_2d_val> q;
        rtree.query(bgi::within(b), std::back_inserter(q));
        OGRGeometry* ogr_poly = ogr_layer->GetGeometry(i);
        for (int j=0; j<q.size(); j++) {
            const box_2d_val& v = q[j];
            int row_idx = v.second;
            OGRGeometry* geom = ml->geoms[row_idx];
            if (geom->Intersects(ogr_poly)) {
                spatial_counts[i] += 1;
                if (join_variable) {
                    mutex.lock();
                    join_ids[i].push_back(row_idx);
                    mutex.unlock();
                }
            }
        }
    }
}

AssignPolygonToLine::AssignPolygonToLine(BackgroundMapLayer* _ml,
                                         Project* _project,
                                         vector<wxInt64>& _poly_ids)
: SpatialJoinWorker(_ml, _project)
{
    join_variable = false;
    is_spatial_assign = true;
    poly_ids = _poly_ids;
    num_polygons = ml->GetNumRecords();
    // using current map(lines) to create rtree
    Shapefile::Main& main_data = project->main_data;
    Shapefile::PolyLineContents* pc;
    int n = project->GetNumRecords();
    std::vector<wxFloat64> box;
    for (int i=0; i<n; i++) {
        pc = (Shapefile::PolyLineContents*)main_data.records[i].contents_p;
        box = pc->box;
        pt_2d ll(box[0], box[1]);
        pt_2d ur(box[2], box[3]);
        box_2d b(ll, ur);
        rtree.insert(std::make_pair(b, i));
        spatial_counts[i] = -1;
    }
}

void AssignPolygonToLine::sub_run(int start, int end)
{
    // for every polygon in sub-layer
    for (int i=start; i<=end; i++) {
        OGRGeometry* ogr_poly = ml->geoms[i];
        // create a box, tl, br
        OGREnvelope box;
        ogr_poly->getEnvelope(&box);
        box_2d b(pt_2d(box.MinX, box.MinY), pt_2d(box.MaxX, box.MaxY));
        // query lines in this box
        std::vector<box_2d_val> q;
        rtree.query(bgi::intersects(b), std::back_inserter(q));
        for (int j=0; j<q.size(); j++) {
            const box_2d_val& v = q[j];
            int row_idx = v.second;
            OGRGeometry* geom = project->layer_proxy->GetGeometry(row_idx);
            if (geom->Intersects(ogr_poly)) {
                spatial_counts[row_idx] = poly_ids[i];
            }
        }
    }
}



CountPolygonInPolygon::CountPolygonInPolygon(BackgroundMapLayer* _ml,
                                             Project* _project,
                                             std::vector<wxString> join_variable_nm,
                                             std::vector<Operation> _op)
: SpatialJoinWorker(_ml, _project)
{
    is_spatial_assign = false;
    join_operation = _op;
    int n_vars = (int)join_operation.size();
    join_values.resize(n_vars);
    join_variable = false; // default false: no need to join variables
    for (int i=0; i<join_variable_nm.size(); ++i) {
        join_variable = _ml->GetDoubleColumnData(join_variable_nm[i], join_values[i]);
    }
    num_polygons = project->GetNumRecords();

    // using selected layer (polygons) to create rtree
    int n = (int)ml->shapes.size();
    for (int i=0; i<n; i++) {
        OGRGeometry* geom = ml->geoms[i];
        OGREnvelope bbox;
        geom->getEnvelope(&bbox);
        pt_2d ll(bbox.MinX, bbox.MinY);
        pt_2d ur(bbox.MaxX, bbox.MaxY);
        box_2d b(ll, ur);
        rtree.insert(std::make_pair(b, i));
    }
}

void CountPolygonInPolygon::sub_run(int start, int end)
{
    Shapefile::Main& main_data = project->main_data;
    OGRLayerProxy* ogr_layer = project->layer_proxy;
    Shapefile::PolygonContents* pc;
    for (int i=start; i<=end; i++) {
        pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
        // create a box, tl, br
        box_2d b(pt_2d(pc->box[0], pc->box[1]),
                 pt_2d(pc->box[2], pc->box[3]));
        // query boxes in this box
        std::vector<box_2d_val> q;
        rtree.query(bgi::intersects(b), std::back_inserter(q));
        OGRGeometry* ogr_poly = ogr_layer->GetGeometry(i);
        for (int j=0; j<q.size(); j++) {
            const box_2d_val& v = q[j];
            int row_idx = v.second;
            OGRGeometry* geom = ml->geoms[row_idx];
            if (geom->Intersects(ogr_poly)) {
                spatial_counts[i] += 1;
                if (join_variable) {
                    mutex.lock();
                    join_ids[i].push_back(row_idx);
                    mutex.unlock();
                }
            }
        }
    }
}

SpatialJoinDlg::SpatialJoinDlg(wxWindow* parent, Project* _project)
: wxDialog(parent, wxID_ANY, "Spatial Join", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    project = _project;

    panel = new wxPanel(this, -1);

    wxString info = _("Please select a map layer to apply "
                      "spatial join to current map (%s):");
    info = wxString::Format(info, project->GetProjectTitle());
    wxStaticText* st = new wxStaticText(panel, wxID_ANY, info);

    map_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    field_st = new wxStaticText(panel, wxID_ANY, _("Select ID Variable (Optional)"));
    field_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(100,-1));
    wxBoxSizer* mbox = new wxBoxSizer(wxHORIZONTAL);
    mbox->Add(map_list, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_st, 0, wxALIGN_CENTER | wxALL, 5);
    mbox->Add(field_list, 0, wxALIGN_CENTER | wxALL, 5);

    rb_spatial_count = new wxRadioButton(panel, -1, _("Spatial Count"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rb_spatial_join = new wxRadioButton(panel, -1, _("Spatial Join"), wxDefaultPosition, wxDefaultSize);
    wxBoxSizer* rb_box = new wxBoxSizer(wxHORIZONTAL);
    rb_box->Add(rb_spatial_count, 0,  wxLEFT, 200);
    rb_box->Add(rb_spatial_join, 0,  wxLEFT, 10);
    rb_spatial_count->SetValue(true);
    
    join_var_st = new wxStaticText(panel, wxID_ANY, _("Join Variable:"));
    join_var_list = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(160, 110), 0, NULL,
                                  wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    wxBoxSizer* join_box = new wxBoxSizer(wxHORIZONTAL);
    join_box->Add(join_var_st, 0, wxALIGN_LEFT | wxALL, 0);
    join_box->Add(join_var_list, 0, wxALIGN_LEFT | wxLEFT, 12);

    join_op_st = new wxStaticText(panel, wxID_ANY, _("Join Operation:"));
    join_op_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(160,-1));
    wxBoxSizer* join_op_box = new wxBoxSizer(wxHORIZONTAL);
    join_op_box->Add(join_op_st, 0, wxALIGN_LEFT | wxTOP, 5);
    join_op_box->Add(join_op_list, 0, wxALIGN_LEFT | wxTOP, 5);

    wxBoxSizer* left_box = new wxBoxSizer(wxVERTICAL);
    left_box->Add(join_box, 0, wxALIGN_LEFT | wxLEFT, 10);
    left_box->Add(join_op_box, 0, wxALIGN_LEFT | wxLEFT, 10);
    
    // list contrl
    lst_join = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(320, 140), wxLC_REPORT);
    lst_join->AppendColumn(_("Variable"), wxLIST_FORMAT_RIGHT);
    lst_join->SetColumnWidth(0, 120);
    lst_join->AppendColumn(_("Join Operation"), wxLIST_FORMAT_RIGHT);
    lst_join->SetColumnWidth(1, 120);
    
    // move buttons
    move_left = new wxButton(panel, wxID_ANY, "<", wxDefaultPosition, wxSize(25,25));
    move_right = new wxButton(panel, wxID_ANY, ">", wxDefaultPosition, wxSize(25,25));
    wxBoxSizer* middle_box = new wxBoxSizer(wxVERTICAL);
    middle_box->Add(move_right, 0, wxTOP, 10);
    middle_box->Add(move_left, 0, wxTOP, 10);
    
    wxBoxSizer* main_box = new wxBoxSizer(wxHORIZONTAL);
    main_box->Add(left_box);
    main_box->Add(middle_box, 0, wxALL, 5);
    main_box->Add(lst_join, 1, wxALL, 0);
    
    cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(st, 0, wxALIGN_CENTER | wxALL, 15);
    cbox->Add(mbox, 0, wxALIGN_CENTER | wxALL, 10);
    cbox->AddSpacer(10);
    cbox->Add(rb_box, 0, wxALL, 10);
    cbox->Add(main_box, wxALL, 10);

    wxButton* ok_btn = new wxButton(panel, wxID_ANY, _("OK"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(panel, wxID_CANCEL, _("Close"),
                                        wxDefaultPosition, wxDefaultSize,
                                        wxBU_EXACTFIT);

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);

    wxBoxSizer *container = new wxBoxSizer(wxVERTICAL);
    container->Add(cbox, 0, wxALIGN_CENTER | wxALL, 10);
    container->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);

    panel->SetSizer(container);
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
     

    map_list->Bind(wxEVT_CHOICE, &SpatialJoinDlg::OnLayerSelect, this);
    ok_btn->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnOK, this);
    join_var_list->Bind(wxEVT_CHOICE, &SpatialJoinDlg::OnJoinVariableSel, this);
    move_left->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnRemoveRow, this);
    move_right->Bind(wxEVT_BUTTON, &SpatialJoinDlg::OnAddRow, this);
    rb_spatial_count->Bind(wxEVT_RADIOBUTTON, &SpatialJoinDlg::OnRadioButton, this);
    rb_spatial_join->Bind(wxEVT_RADIOBUTTON, &SpatialJoinDlg::OnRadioButton, this);

    InitMapList();
    
    wxCommandEvent e;
    OnLayerSelect(e);
    OnRadioButton(e);

    SetSizer(panelSizer);
    SetAutoLayout(true);
    panelSizer->Fit(this);

    Center();
}

void SpatialJoinDlg::OnRadioButton(wxCommandEvent& e)
{
    bool flag = !rb_spatial_count->GetValue();
    join_var_st->Enable(flag);
    join_op_st->Enable(flag);
    join_var_list->Enable(flag);
    join_op_list->Enable(flag);
    move_left->Enable(flag);
    move_right->Enable(flag);
    lst_join->Enable(flag);
}

void SpatialJoinDlg::OnAddRow(wxCommandEvent& e)
{
    // get join operation
    wxString join_op;
    int op_sel = join_op_list->GetSelection();
    if (op_sel == 0) {
        join_op = "Sum";
    } else if (op_sel == 1) {
        join_op = "Mean";
    } else if (op_sel == 2) {
        join_op = "Median";
    } else if (op_sel == 3) {
        join_op = "Standard Deviation";
    }
    
    bool show_warning = false;
    // get selected variables
    wxArrayInt selections;
    join_var_list->GetSelections(selections);
    int num_var = (int)selections.size();
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString var_name = join_var_list->GetString(idx);
        std::pair<wxString, wxString> element;
        element.first = var_name;
        element.second = join_op;
        if (var_set.find(element) != var_set.end() && !show_warning) {
            // warning
            wxString err_msg = _("%s and %s have already been added for spatial join.");
            wxMessageDialog dlg(NULL, wxString::Format(err_msg, var_name, join_op),
                                _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            show_warning = true;
        } else {
            var_set.insert(element);
            int new_row = lst_join->GetItemCount();
            lst_join->InsertItem(new_row, element.first);
            lst_join->SetItem(new_row, 1, element.second);
        }
    }
}

std::list<int> SpatialJoinDlg::GetListSel(wxListCtrl* lc)
{
    std::list<int> l;
    if (!lc) return l;
    long item = -1;
    for ( ;; ) {
        item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if ( item == -1 ) break;
        l.push_back(item);
    }
    return l;
}

void SpatialJoinDlg::OnRemoveRow(wxCommandEvent& e)
{
    std::list<int> sels = GetListSel(lst_join);
    sels.sort();
    sels.reverse();
    if (!sels.empty()) {
        BOOST_FOREACH(int i, sels) {
            wxString var_name = lst_join->GetItemText(i, 0);
            wxString join_op = lst_join->GetItemText(i, 1);
            var_set.erase(std::make_pair(var_name, join_op));
            lst_join->DeleteItem(i);
        }
    }
    e.Skip();
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
            project->GetShapeType() != Shapefile::POLYGON ) {
            // assign polygon to point
            field_list->Clear();
            vector<wxString> field_names = ml->GetIntegerFieldNames();
            field_list->Append("");
            for (int i=0; i<field_names.size(); i++) {
                field_list->Append(field_names[i]);
            }
            field_list->Show();
            field_st->Show();

            join_var_list->Hide();
            join_var_st->Hide();
            join_op_list->Hide();
            join_op_st->Hide();
            
            rb_spatial_count->Hide();
            rb_spatial_join->Hide();
            lst_join->Hide();

            move_left->Hide();
            move_right->Hide();
        } else {
            // spatial count
            field_list->Clear();
            field_list->Hide();
            field_st->Hide();
            
            join_var_list->Show();
            join_var_st->Show();
            join_op_list->Show();
            join_op_st->Show();
            // spatial join
            join_var_list->Clear();
            vector<wxString> field_names = ml->GetNumericFieldNames();
            for (int i=0; i<field_names.size(); i++) {
                join_var_list->Append(field_names[i]);
            }
            join_op_list->Clear();
            join_op_list->Append("Sum");
            join_op_list->Append("Mean");
            join_op_list->Append("Median");
            join_op_list->Append("Standard Deviation");
        }
    }
}

void SpatialJoinDlg::OnJoinVariableSel(wxCommandEvent& e)
{
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
            wxMessageDialog dlg (this, _("Spatial Join can not be applied on "
                                         "two points layers. Please select "
                                         "another layer."),
                                 _("Warning"), wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
            return;
        }
        wxString label = "Spatial Count";
        wxString field_name = "SC";

        SpatialJoinWorker* sj = NULL;
        if (project->GetShapeType() == Shapefile::POINT_TYP ||
            project->GetShapeType() == Shapefile::POLY_LINE) {
            // working layer is Points/Lines
            vector<wxInt64> poly_ids;
            for (int i=0; i<n; i++) {
                poly_ids.push_back(i);
            }

            int field_idx = field_list->GetSelection();
            if (field_idx > 0) {
                wxString field_name = field_list->GetString(field_idx);
                bool success = ml->GetIntegerColumnData(field_name, poly_ids);
                if ( !success || poly_ids.size() != n) {
                    wxMessageDialog dlg (this, _("Select field is not integer "
                                                 "type. Default record order "
                                                 "will be used instead."),
                                         _("Warning"), wxOK | wxICON_INFORMATION);
                    dlg.ShowModal();
                    poly_ids.clear();
                }
            }
            if (project->GetShapeType() == Shapefile::POINT_TYP) {
                sj = new AssignPolygonToPoint(ml, project, poly_ids);
            } else if (project->GetShapeType() == Shapefile::POLY_LINE) {
                sj = new AssignPolygonToLine(ml, project, poly_ids);
            }
            label = "Spatial Assign";
            field_name = "SA";

        } else {
            // working layer is Polygon: spatial count
            label = "Spatial Join";
            field_name = rb_spatial_count->GetValue() ? "SC" : "SJ";

            int join_list_sel = lst_join->GetItemCount();
            if (rb_spatial_join->GetValue() && join_list_sel == 0) {
                wxMessageDialog dlg (this, _("Please select at least one Join Operation with Join Variable."),
                                     _("Warning"), wxOK | wxICON_INFORMATION);
                dlg.ShowModal();
                return;
            }
            
            std::vector<wxString> join_var_nms;
            std::vector<SpatialJoinWorker::Operation> join_ops;
            
            for (int i=0; i<join_list_sel; ++i) {
                wxString var_name = lst_join->GetItemText(i, 0);
                wxString op_name = lst_join->GetItemText(i, 1);
                SpatialJoinWorker::Operation join_op = SpatialJoinWorker::SUM;
                if (op_name == "Sum") {
                    join_op = SpatialJoinWorker::SUM;
                } else if (op_name == "Mean") {
                    join_op = SpatialJoinWorker::MEAN;
                } else if (op_name == "Median") {
                    join_op = SpatialJoinWorker::MEDIAN;
                } else if (op_name == "Standard Deviation") {
                    join_op = SpatialJoinWorker::STD;
                }
                join_var_nms.push_back(var_name);
                join_ops.push_back(join_op);
            }
        
            if (ml->GetShapeType() == Shapefile::POINT_TYP) {
                sj = new CountPointsInPolygon(ml, project, join_var_nms, join_ops);
            } else if (ml->GetShapeType() == Shapefile::POLY_LINE) {
                sj = new CountLinesInPolygon(ml, project, join_var_nms, join_ops);
            } else if (ml->GetShapeType() == Shapefile::POLYGON) {
                sj = new CountPolygonInPolygon(ml, project, join_var_nms, join_ops);
            } else {
                wxMessageDialog dlg (this, _("Spatial Join can not be applied on "
                                             "unknonwn layers. Please select "
                                             "another layer."),
                                     _("Warning"), wxOK | wxICON_INFORMATION);
                dlg.ShowModal();
                return;
            }
        }
        sj->Run();

        wxString dlg_title = _("Save Results to Table: ") + label;

        if (sj->IsDuplicateCount()) {
            wxMessageDialog dlg (this, _("There are spatial objects being counted more than once. Please check the results."),
                                 _("Warning"), wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
        }

        if (sj->JoinVariable()) {
            // Spatial Join with variable
            std::vector<std::vector<double> > spatial_joins = sj->GetJoinResults();
            int new_col = (int)spatial_joins.size();
            std::vector<SaveToTableEntry> new_data(new_col);
            vector<bool> undefs(project->GetNumRecords(), false);
            for (int i=0; i<new_col; ++i) {
                new_data[i].d_val = &spatial_joins[i];
                new_data[i].label = label + " " + field_name;
                new_data[i].field_default = field_name;
                new_data[i].field_default << "_" << i+1;
                new_data[i].type = GdaConst::double_type;
                new_data[i].undefined = &undefs;
            }
            SaveToTableDlg dlg(project, this, new_data, dlg_title,
                               wxDefaultPosition, wxSize(400,400));
            dlg.ShowModal();
        } else {
            // Spatial count or Spatial assigning
            vector<wxInt64> spatial_counts = sj->GetResults();
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
        }
        delete sj;
        EndDialog(wxID_OK);
    }
}

