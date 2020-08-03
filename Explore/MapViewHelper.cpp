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
#include <wx/wx.h>
#include <wx/textfile.h>

#include "../DialogTools/VariableSettingsDlg.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../ShapeOperations/WeightUtils.h"
#include "MapNewView.h"
#include "MapViewHelper.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MapTransparencyDlg
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
MapTransparencyDlg::MapTransparencyDlg(wxWindow * parent,
                                       MapCanvas* _canvas,
                                       wxWindowID id,
                                       const wxString & caption,
                                       const wxPoint & position,
                                       const wxSize & size,
                                       long style )
: wxDialog( parent, id, caption, position, size, style)
{

    wxLogMessage("Open MapTransparencyDlg");

    canvas = _canvas;

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    // A text control for the user’s name
    double trasp = (double)canvas->tran_unhighlighted / 255.0;
    int trasp_scale = 100 * trasp;

    wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
    slider = new wxSlider(this, wxID_ANY, trasp_scale, 0, 100,
                          wxDefaultPosition, wxSize(200, -1),
                          wxSL_HORIZONTAL);
    subSizer->Add(new wxStaticText(this, wxID_ANY,"1.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(slider, 0, wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(new wxStaticText(this, wxID_ANY,"0.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);

    boxSizer->Add(subSizer);
    wxString txt_transparency = wxString::Format(_("Current Transparency: %.2f"), 1.0 - trasp);

    slider_text = new wxStaticText(this,
                                   wxID_ANY,
                                   txt_transparency,
                                   wxDefaultPosition,
                                   wxSize(100, -1));
    boxSizer->Add(slider_text, 0, wxGROW|wxALL, 5);
    boxSizer->Add(new wxButton(this, wxID_CANCEL, _("Close")), 0, wxALIGN_CENTER|wxALL, 10);

    topSizer->Fit(this);

    slider->Bind(wxEVT_SLIDER, &MapTransparencyDlg::OnSliderChange, this);
}

MapTransparencyDlg::~MapTransparencyDlg()
{
}

void MapTransparencyDlg::OnSliderChange( wxCommandEvent & event )
{
    int val = event.GetInt();
    double trasp = 1.0 - val / 100.0;
    slider_text->SetLabel(wxString::Format(_("Current Transparency: %.2f"), trasp));
    canvas->tran_unhighlighted = (1-trasp) * 255;
    canvas->ReDraw();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeatMapTransparencyDlg
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
HeatMapTransparencyDlg::HeatMapTransparencyDlg(HeatMapHelper* _heatmap, MapCanvas* canvas)
: MapTransparencyDlg((wxWindow*)canvas, canvas), heatmap(_heatmap)
{
    int trans = heatmap->GetTransparency();
    int tick = (trans / 255.0) * 100.0;
    slider->SetValue(tick);
}

HeatMapTransparencyDlg::~HeatMapTransparencyDlg()
{
}

void HeatMapTransparencyDlg::OnSliderChange(wxCommandEvent& event)
{
    int val = event.GetInt();
    double trasp = 1.0 - val / 100.0;
    slider_text->SetLabel(wxString::Format(_("Current Transparency: %.2f"), trasp));
    int transparency = (1-trasp) * 255;
    if (canvas && heatmap) {
        heatmap->UpdateTransparency(transparency);
        canvas->RedrawMap();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeatMapBandwidthDlg
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
HeatMapBandwidthDlg::HeatMapBandwidthDlg(HeatMapHelper* _heatmap,
                                         MapCanvas* _canvas,
                                         double sel_band, double max_band,
                                         wxWindowID id,
                                         const wxString & caption,
                                         const wxPoint & position,
                                         const wxSize & size,
                                         long style)
: wxDialog(_canvas, id, caption, position, size, style)
{
    wxLogMessage("Open HeatMapBandwidthDlg");

    this->canvas = _canvas;
    this->heatmap = _heatmap;
    this->sel_band = sel_band;
    this->max_band = max_band;

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    int ticket = (int)(sel_band / max_band * 100);
    slider = new wxSlider(this, wxID_ANY, ticket, 0, 100,
                          wxDefaultPosition, wxSize(300, -1),
                          wxSL_HORIZONTAL);

    boxSizer->Add(slider);
    wxString bandwidth_lbl = wxString::Format(_("Bandwidth: %f"), sel_band);

    slider_text = new wxStaticText(this, wxID_ANY, bandwidth_lbl,
                                   wxDefaultPosition, wxSize(100, -1));
    boxSizer->Add(slider_text, 0, wxGROW|wxALL, 5);
    boxSizer->Add(new wxButton(this, wxID_CANCEL, _("Close")), 0, wxALIGN_CENTER|wxALL, 10);

    SetSizer(topSizer);
    topSizer->Fit(this);

    slider->Bind(wxEVT_SLIDER, &HeatMapBandwidthDlg::OnSliderChange, this);

    wxCommandEvent ev;
    OnSliderChange(ev);
}

HeatMapBandwidthDlg::~HeatMapBandwidthDlg()
{
}

void HeatMapBandwidthDlg::OnSliderChange( wxCommandEvent & event )
{
    int val = slider->GetValue();
    double band = (val / 100.0) * max_band;
    slider_text->SetLabel(wxString::Format(_("Bandwidth: %f"), band));
    if (canvas && heatmap) {
        heatmap->UpdateBandwidth(band);
        canvas->RedrawMap();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeatMapHelper
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
bool HeatMapHelper::check_spatial_ref = true;

HeatMapHelper::HeatMapHelper()
{
    use_fill_color = false;
    use_outline_color = false;
    use_bandwidth = true;
    use_radius_variable = false;
    transparency = 20;
    bandwidth = 0;
}

HeatMapHelper::~HeatMapHelper()
{
}

void HeatMapHelper::Reset()
{
    use_fill_color = false;
    use_outline_color = false;
    transparency = 20;
    bandwidth = 0;
}

int HeatMapHelper::GetTransparency()
{
    return transparency;
}

void HeatMapHelper::SetBandwidth(MapCanvas* canvas, Project* project)
{
    if (check_spatial_ref) {
        bool cont = project->CheckSpatialProjection(check_spatial_ref);
        if (cont == false) {
            return;
        }
    }
    use_bandwidth = true;
    use_radius_variable = !use_bandwidth;
    // prompt user to select a bandwidth from a slider
    double min_band = project->GetMax1nnDistEuc();
    double max_band = project->GetMaxDistEuc();
    if (bandwidth == 0) {
        bandwidth = min_band;
    }
    HeatMapBandwidthDlg bandDlg(this, canvas, bandwidth, max_band);
    bandDlg.ShowModal();
}

void HeatMapHelper::UpdateBandwidth(double bandwidth)
{
    this->bandwidth = bandwidth;
}

void HeatMapHelper::SetRadiusVariable(MapCanvas* canvas, Project* project)
{
    // prompt user to select a variable for the radius
    VariableSettingsDlg dlg(project, VariableSettingsDlg::univariate);
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    std::vector<d_array_type> data(1);
    TableInterface *table_int = project->GetTableInt();
    table_int->GetColData(dlg.col_ids[0], data[0]);

    int t = 0; // assume the radius is not a time-variable
    int n = data[0][0].size();
    radius_arr.clear();
    radius_arr.resize(n);
    for (int i=0; i< n; ++i) {
        radius_arr[i] = data[0][0][i];
    }
    use_radius_variable = true;
    use_bandwidth = !use_radius_variable;
    canvas->RedrawMap();
}

void HeatMapHelper::ChangeFillColor(MapCanvas* canvas)
{
    // prompt user color pick dialog
    fill_color = GeneralWxUtils::PickColor(canvas, fill_color);
    use_fill_color = true;
    canvas->RedrawMap();
}

void HeatMapHelper::ChangeOutlineColor(MapCanvas* canvas)
{
    // prompt user color pick dialog
    outline_color = GeneralWxUtils::PickColor(canvas, outline_color);
    use_outline_color = true;
    canvas->RedrawMap();
}

void HeatMapHelper::ChangeTransparency(MapCanvas* canvas)
{
    // prompt user color input dialog
    HeatMapTransparencyDlg sliderDlg(this, canvas);
    sliderDlg.ShowModal();
}

void HeatMapHelper::UpdateTransparency(int tran)
{
    this->transparency = tran;
}

void HeatMapHelper::Draw(const std::vector<GdaShape*>& selectable_shps,
                         std::list<GdaShape*>& background_shps,
                         CatClassifData& cat_data)
{
    wxPen pen(outline_color);
    if (use_fill_color) {
        fill_color.Set(fill_color.Red(), fill_color.Green(), fill_color.Blue(), transparency);
        wxBrush brush(fill_color);
        for (int i=0; i<selectable_shps.size(); ++i) {
            GdaPoint *pt = (GdaPoint*)selectable_shps[i];
            double r = use_bandwidth ? bandwidth : radius_arr[i];
            GdaPoint* p = new GdaPoint(pt->GetX(), pt->GetY(), r);
            p->setPen(use_outline_color ? pen : *wxTRANSPARENT_PEN);
            p->setBrush(brush);
            background_shps.push_back(p); // memory will be freed by background_shps
        }
    } else {
        int cc_ts = cat_data.curr_canvas_tm_step;
        int num_cats = cat_data.GetNumCategories(cc_ts);
        for (int cat=0; cat<num_cats; cat++) {
            wxColour clr = cat_data.GetCategoryBrushColor(cc_ts, cat);
            clr.Set(clr.Red(), clr.Green(), clr.Blue(), transparency);
            wxBrush brush(clr);
            std::vector<int>& ids = cat_data.GetIdsRef(cc_ts, cat);
            for (size_t i=0, iend=ids.size(); i<iend; i++) {
                GdaPoint *pt = (GdaPoint*)selectable_shps[ids[i]];
                double r = use_bandwidth ? bandwidth : radius_arr[ids[i]];
                GdaPoint* p = new GdaPoint(pt->GetX(), pt->GetY(), r);
                p->setBrush(brush);
                p->setPen(use_outline_color ? pen : *wxTRANSPARENT_PEN);
                background_shps.push_back(p); // memory will be freed by background_shps
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MSTMapHelper
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
bool MSTMapHelper::check_spatial_ref = true;

MSTMapHelper::MSTMapHelper()
{
    use_custom_pen = false;
    outline_color = *wxBLACK;
    outline_thickness = 1;
}


MSTMapHelper::~MSTMapHelper()
{
}

void MSTMapHelper::CreateDistMatrix(const std::vector<GdaPoint*>& points)
{
    if (dist_matrix.empty() == false) return;

    int n = points.size();
    dist_matrix.resize(n);
    for (int i=0; i<n; i++) {
        dist_matrix[i].resize(n-i-1);
        for (int j=i+1, k=0; j<n; j++, k++) {
            // euclidean distance between i and j
            double x1 = points[i]->GetX();
            double y1 = points[i]->GetY();
            double x2 = points[j]->GetX();
            double y2 = points[j]->GetY();
            double d = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
            dist_matrix[i][k] = sqrt(d);
        }
    }
}

bool MSTMapHelper::Create(Project* project)
{
    if (check_spatial_ref) {
        bool cont = project->CheckSpatialProjection(check_spatial_ref);
        if (cont == false) {
            return false;
        }
    }
    if (project == 0) return false;
    if (mst_edges.empty() == false) return true; // already has a MST

    // if no MST, create a MST
    // use centroids to create MST
    WeightsManInterface* w_man_int = project->GetWManInt();
    nodes = project->GetCentroids();
    CreateDistMatrix(nodes);

    int num_obs = project->GetNumRecords();
    Graph g(num_obs);
    for (int i=0; i<num_obs; i++) {
        for (int j=i+1, k=0; j<num_obs; j++, k++) {
            boost::add_edge(i, j, dist_matrix[i][k], g);
        }
    }

    //https://github.com/vinecopulib/vinecopulib/issues/22
    std::vector<int> p(num_vertices(g));
    prim_minimum_spanning_tree(g, p.data());

    for (int source = 0; source < p.size(); ++source) {
        int target = p[source];
        if (source != target) {
            //boost::add_edge(source, p[source], mst);
            mst_edges.push_back(std::make_pair(source, target));
        }
    }
    return true;
}

void MSTMapHelper::SaveToWeightsFile(Project* project)
{
    // create MST first if needed
    if (this->Create(project) == false) {
        return;
    }
    
    // Get ID variable for spatial weights file
    SelectWeightsIdDialog selid_dlg(NULL, project);
    if (selid_dlg.ShowModal() != wxID_OK) {
        return;
    }
    TableInterface* table_int = project->GetTableInt();
    wxString id = selid_dlg.GetIDVariable();
    if (id.IsEmpty()) return;

    int col = table_int->FindColId(id);
    if (col < 0) return;

    std::vector<wxString> ids;
    table_int->GetColData(col, 0, ids);

    // prompt user to select output file
    wxString filter = "GWT|*.gwt";
    wxFileDialog dialog(NULL, _("Save Spanning Tree to a Weights File"),
                        wxEmptyString, wxEmptyString, filter,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    // create Spatial weights file
    wxFileName fname = wxFileName(dialog.GetPath());
    wxString new_main_dir = fname.GetPathWithSep();
    wxString new_main_name = fname.GetName();
    wxString new_txt = new_main_dir + new_main_name+".gwt";
    wxTextFile file(new_txt);
    file.Create(new_txt);
    file.Open(new_txt);
    file.Clear();

    wxString header;
    header << "0 " << project->GetNumRecords() << " ";
    header << "\"" << project->GetProjectTitle() << "\" ";
    header <<  id;
    file.AddLine(header);

    for (int i=0; i<mst_edges.size(); i++) {
        int f_idx = mst_edges[i].first;
        int t_idx = mst_edges[i].second;

        if (f_idx == t_idx) {
            continue;
        }

        double cost = t_idx > f_idx ? dist_matrix[f_idx][t_idx - f_idx-1] :
        dist_matrix[t_idx][f_idx - t_idx-1];
        wxString line1;
        line1 << ids[f_idx] << " " << ids[t_idx] << " " <<  cost;
        file.AddLine(line1);
        wxString line2;
        line2 << ids[t_idx] << " " << ids[f_idx] << " " <<  cost;
        file.AddLine(line2);
    }
    file.Write();
    file.Close();

    // Load the weights file into Weights Manager
    WeightsManInterface* w_man_int = project->GetWManInt();
    WeightUtils::LoadGwtInMan(w_man_int, new_txt, table_int, id, WeightsMetaInfo::WT_tree);
}

void MSTMapHelper::Draw(std::list<GdaShape*>& foreground_shps,
                        CatClassifData& cat_data)
{
    int n_edges = mst_edges.size();
    if (n_edges <= 0) return;

    GdaPolyLine* edge;
    wxPen pen(outline_color, outline_thickness);
    int cc_ts = cat_data.curr_canvas_tm_step;

    for (int i=0; i < n_edges; ++i) {
        const std::pair<int, int>& s_t = mst_edges[i];
        int source = s_t.first;
        int target = s_t.second;
        edge = new GdaPolyLine(nodes[source]->GetX(), nodes[source]->GetY(),
                               nodes[target]->GetX(), nodes[target]->GetY());
        edge->from = source;
        edge->to = target;
        if (!use_custom_pen && cat_data.GetNumCategories(cc_ts) >  1) {
            pen = cat_data.GetCategoryPenById(cc_ts, source);
            pen.SetWidth(outline_thickness);
        }
        edge->setPen(pen);
        foreground_shps.push_back(edge);
    }
}

void MSTMapHelper::ChangeColor(MapCanvas* canvas)
{
    // prompt user color pick dialog
    outline_color = GeneralWxUtils::PickColor(canvas, outline_color);
    use_custom_pen = true;
    canvas->RedrawMap();
}

void MSTMapHelper::ChangeThickness(MapCanvas* canvas, int thickness)
{
    use_custom_pen = true;
    outline_thickness = thickness;
    canvas->RedrawMap();
}

int MSTMapHelper::GetThickness()
{
    return outline_thickness;
}
