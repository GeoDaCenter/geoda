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

#include "../DialogTools/VariableSettingsDlg.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
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

