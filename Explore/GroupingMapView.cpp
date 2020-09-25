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

#include <limits>
#include <vector>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <wx/statbmp.h>
#include <wx/checklst.h>

#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../DialogTools/AbstractClusterDlg.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "GroupingMapView.h"

using namespace std;

HierachicalMapSelectDlg::HierachicalMapSelectDlg(wxFrame* parent_s, Project* project_s)
: wxDialog(parent_s, wxID_ANY, _("Hierarchical Cluster Map"), wxDefaultPosition,
           wxSize(350, 250))
{
    wxLogMessage("Open GroupingSelectDlg.");
    
    project = project_s;
    wxPanel* panel = new wxPanel(this, -1);

    wxFlexGridSizer* gbox = new wxFlexGridSizer(2, 2, 5, 5);
    wxStaticText* root_var_st = new wxStaticText(panel, wxID_ANY,
                                                 _("Root Variable:"));
    root_var_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                 wxSize(160,-1));
    gbox->Add(root_var_st, 0, wxALIGN_LEFT | wxALL, 0);
    gbox->Add(root_var_list, 0, wxALIGN_LEFT | wxLEFT, 12);

    wxStaticText* group_var_st = new wxStaticText(panel, wxID_ANY,
                                                  _("Group Variable:"));
    group_var_list = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                  wxSize(160,-1));
    gbox->Add(group_var_st, 0, wxALIGN_LEFT | wxALL, 0);
    gbox->Add(group_var_list, 0, wxALIGN_LEFT | wxLEFT, 12);
    
    wxButton* ok_btn = new wxButton(this, wxID_ANY, _("OK"), wxDefaultPosition,
                                    wxDefaultSize, wxBU_EXACTFIT);
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, _("Close"),
                                        wxDefaultPosition, wxDefaultSize,
                                        wxBU_EXACTFIT);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(ok_btn, 0, wxALIGN_CENTER | wxALL, 5);
    hbox->Add(cancel_btn, 0, wxALIGN_CENTER | wxALL, 5);
    
    wxBoxSizer* cbox = new wxBoxSizer(wxVERTICAL);
    cbox->Add(gbox, 0, wxALIGN_CENTER | wxALL, 10);
    panel->SetSizerAndFit(cbox);
    
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxALL, 15);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(vbox);
    vbox->Fit(this);
    
    Center();
    ok_btn->Bind(wxEVT_BUTTON, &HierachicalMapSelectDlg::OnOK, this);
    
    std::vector<int> col_id_map;
    table_int = project->GetTableInt();
    table_int->FillIntegerColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                group_var_list->Append(nm);
                root_var_list->Append(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            group_var_list->Append(name);
            root_var_list->Append(name);
        }
    }
}

HierachicalMapSelectDlg::~HierachicalMapSelectDlg()
{
    
}

std::vector<GdaVarTools::VarInfo> HierachicalMapSelectDlg::GetVarInfo()
{
    return vars;
}

std::vector<int> HierachicalMapSelectDlg::GetColIds()
{
    return col_ids;
}

void HierachicalMapSelectDlg::OnOK( wxCommandEvent& event)
{
    wxLogMessage("GroupingSelectDlg::OnOK()");
    vars.resize(2);
    col_ids.resize(2);
    
    wxString group_nm = group_var_list->GetString(group_var_list->GetSelection());
    wxString group_field = name_to_nm[group_nm];
    int time1 = name_to_tm_id[group_nm];
    int col1 = table_int->GetColIdx(group_field);
    if (col1 < 0) return;
    
    vars[0].name = group_field;
    vars[0].is_time_variant = table_int->IsColTimeVariant(col1);
    vars[0].time = time1;
    col_ids[0] = col1;
    
    wxString root_nm = root_var_list->GetString(root_var_list->GetSelection());
    wxString root_field = name_to_nm[root_nm];
    int time2 = name_to_tm_id[root_nm];
    int col2 = table_int->GetColIdx(root_field);
    if (col2 < 0) return;
    
    vars[1].name = root_field;
    vars[1].is_time_variant = table_int->IsColTimeVariant(col2);
    vars[1].time = time2;
    col_ids[1] = col2;
    
    // create a weights
    vector<wxInt64> vals, root_ids;
    table_int->GetColData(col1, time1, vals);
    if (vals.empty()) return;
    table_int->GetColData(col2, time2, root_ids);
    if (root_ids.empty()) return;
    
    std::map<wxInt64, int> grp_root;
    std::map<wxInt64, std::vector<int> > grp_vals;
    int n = vals.size();
    for (int i=0; i<n; i++) {
        grp_vals[ vals[i] ].push_back(i);
        if (root_ids[i] == 1) {
            grp_root[ vals[i] ] = i;
        }
    }
    
    GalWeight* Wp = new GalWeight;
    Wp->num_obs = n;
    Wp->is_symmetric = true;
    Wp->symmetry_checked = true;
    Wp->gal = new GalElement[n];
    Wp->is_internal_use = true;

    std::map<wxInt64, int>::iterator it;
    for (it=grp_root.begin(); it != grp_root.end(); ++it) {
        wxInt64 grp_id = it->first;
        int r_id = it->second;
        if (grp_vals.find(grp_id) != grp_vals.end()) {
            std::vector<int>& ids = grp_vals[grp_id];
            int n_brs = 0;
            for (size_t i=0; i<ids.size(); ++i) {
                if (ids[i] != r_id) n_brs ++;
            }
            Wp->gal[r_id].SetSizeNbrs(n_brs);
            int cnt=0;
            for (size_t i=0; i<ids.size(); ++i) {
                if (ids[i] != r_id) {
                    Wp->gal[r_id].SetNbr(cnt++, ids[i]);
                }
            }
        }
    }
    WeightsMetaInfo wmi;
    WeightsMetaInfo e(wmi);
    e.filename = group_nm + "/" + root_nm;
    e.weights_type = WeightsMetaInfo::WT_internal;
    WeightsManInterface* w_man_int = project->GetWManInt();
    uid = w_man_int->RequestWeights(e);
    bool success = ((WeightsNewManager*) w_man_int)->AssociateGal(uid, Wp);
    if (success == false) return;
    title = _("Hierarchical Map: ");
    title << e.filename;
    EndDialog(wxID_OK);
}

wxString HierachicalMapSelectDlg::GetTitle()
{
    return title;
}

boost::uuids::uuid HierachicalMapSelectDlg::GetWUID()
{
    return uid;
}
////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(HierachicalMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(HierachicalMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

HierachicalMapCanvas::HierachicalMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
    Project* project, std::vector<GdaVarTools::VarInfo> vars,
    std::vector<int> col_ids, boost::uuids::uuid w_uuid,
    const wxPoint& pos, const wxSize& size)
: MapCanvas(parent, t_frame, project, vars, col_ids,
            CatClassification::unique_values, no_smoothing, 1,
            w_uuid, pos, size)
{
	wxLogMessage("Entering GroupingMapCanvas::GroupingMapCanvas");
    root_radius = 4;
    group_field_nm = vars[0].name;
    root_field_nm = vars[1].name;
    root_field_id = table_int->GetColIdx(vars[1].name);
    // Get Root values and related row id
    std::vector<wxInt64> vals;
    table_int->GetColData(root_field_id, vars[1].time, vals);
    for (size_t i=0; i<vals.size(); ++i) {
        if (vals[i] == 1) {
            grp_root[i] = true;
        }
    }
    display_weights_graph = true;
	CreateAndUpdateCategories();
    UpdateStatusBar();
    
    full_map_redraw_needed = true;
    PopulateCanvas();
	wxLogMessage("Exiting GroupingMapCanvas::GroupingMapCanvas");
}

HierachicalMapCanvas::~HierachicalMapCanvas()
{
	wxLogMessage("In GroupingMapCanvas::~GroupingMapCanvas");
}

void HierachicalMapCanvas::DrawConnectivityGraph(wxMemoryDC &dc)
{
    std::vector<bool>& hs = highlight_state->GetHighlight();
    if (highlight_state->GetTotalHighlighted() >0) {
        wxPen pen(graph_color, weights_graph_thickness);
        for (int i=0; i<w_graph.size(); i++) {
            GdaPolyLine* e = w_graph[i];
            if (hs[e->from] || hs[e->to]) {
                e->setPen(pen);
                e->paintSelf(dc);
            } else {
                e->setPen(*wxTRANSPARENT_PEN);
            }
        }
    }
}

void HierachicalMapCanvas::CreateConnectivityGraph()
{
    // use centroids to draw graph
    WeightsManInterface* w_man_int = project->GetWManInt();
    GalWeight* gal_weights = w_man_int->GetGal(weights_id);
    const vector<GdaPoint*>& c = project->GetCentroids();
    vector<bool>& hs = highlight_state->GetHighlight();
    GdaPolyLine* edge;
    std::set<int> w_nodes;
    wxPen pen(graph_color, weights_graph_thickness);
    for (int i=0; gal_weights && i<gal_weights->num_obs; i++) {
        GalElement& e = gal_weights->gal[i];
        for (int j=0, jend=e.Size(); j<jend; j++) {
            int nbr = e[j];
            if (i!=nbr) {
                // connect i<->nbr
                edge = new GdaPolyLine(c[i]->GetX(),c[i]->GetY(),
                                       c[nbr]->GetX(), c[nbr]->GetY());
                edge->from = i;
                edge->to = nbr;
                edge->setPen(pen);
                edge->setBrush(*wxTRANSPARENT_BRUSH);
                foreground_shps.push_back(edge);
                w_graph.push_back(edge);
                w_nodes.insert(i);
                w_nodes.insert(nbr);

            }
        }
    }

    std::map<int, bool>::iterator it;
    for (it = grp_root.begin(); it != grp_root.end(); ++it) {
        int row = it->first;
        GdaPoint* pt = c[row];
        GdaPoint* circle = new GdaPoint(pt->GetX(), pt->GetY());
        circle->SetRadius(root_radius);
        circle->setPen(wxPen(root_color));
        circle->setBrush(*wxTRANSPARENT_BRUSH);
        foreground_shps.push_back(circle);
    }
}

void HierachicalMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering GroupingMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((HierachicalMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_HIERARCHICAL_MAP_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting GroupingMapCanvas::DisplayRightClickMenu");
}

void HierachicalMapCanvas::ChangeRootSize(int root_sz)
{
    root_radius = root_sz;
    full_map_redraw_needed = true;
    PopulateCanvas();
}

int HierachicalMapCanvas::GetRootSize()
{
    return root_radius;
}

void HierachicalMapCanvas::ChangeRootColor(wxColour root_clr)
{
    root_color = root_clr;
    full_map_redraw_needed = true;
    PopulateCanvas();
}

wxColour HierachicalMapCanvas::GetRootColor()
{
    return root_color;
}

wxString HierachicalMapCanvas::GetCanvasTitle()
{
	wxString ttl;
    ttl << group_field_nm;
	return ttl;
}

wxString HierachicalMapCanvas::GetVariableNames()
{
    wxString ttl;
    ttl << group_field_nm << "/"  << root_field_nm;
    return ttl;
}

bool HierachicalMapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme, SmoothingType new_map_smoothing)
{
	wxLogMessage("In GroupingMapCanvas::ChangeMapType");
	return false;
}

void HierachicalMapCanvas::SetCheckMarks(wxMenu* menu)
{
	MapCanvas::SetCheckMarks(menu);
}

void HierachicalMapCanvas::TimeChange()
{
	wxLogMessage("Entering GroupingMapCanvas::TimeChange");
	wxLogMessage("Exiting GroupingMapCanvas::TimeChange");
}

void HierachicalMapCanvas::TimeSyncVariableToggle(int var_index)
{
	wxLogMessage("In GroupingMapCanvas::TimeSyncVariableToggle");
}

void HierachicalMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = 0;
    if (template_frame) {
        sb = template_frame->GetStatusBar();
    }
    if (!sb)
        return;
    wxString s;
    s << _("#obs=") << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        s << _("#selected=") << highlight_state->GetTotalHighlighted()<< "  ";
    }
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << _("#hover obs ") << hover_obs[0]+1;
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << _("obs ") << hover_obs[1]+1;
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << _("obs ") << hover_obs[2]+1;
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    sb->SetStatusText(s);
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(HierachicalMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(HierachicalMapFrame, MapFrame)
	EVT_ACTIVATE(HierachicalMapFrame::OnActivate)
END_EVENT_TABLE()

HierachicalMapFrame::HierachicalMapFrame(wxFrame *parent, Project* project,
                                   std::vector<GdaVarTools::VarInfo> vars,
                                   std::vector<int> col_ids,
                                   boost::uuids::uuid w_uuid,
                                   const wxString title,
                                   const wxPoint& pos,
                                   const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style)
{
	wxLogMessage("Entering GroupingMapFrame::GroupingMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new HierachicalMapCanvas(rpanel, this, project, vars,
                                            col_ids, w_uuid);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
	
	wxPanel* lpanel = new wxPanel(splitter_win);
    template_legend = new MapNewLegend(lpanel, template_canvas,
                                       wxPoint(0,0), wxSize(0,0));
	wxBoxSizer* lbox = new wxBoxSizer(wxVERTICAL);
    template_legend->GetContainingSizer()->Detach(template_legend);
    lbox->Add(template_legend, 1, wxEXPAND);
    lpanel->SetSizer(lbox);
    
	splitter_win->SplitVertically(lpanel, rpanel, GdaConst::map_default_legend_width);
    
    wxPanel* toolbar_panel = new wxPanel(this,wxID_ANY, wxDefaultPosition);
	wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    toolbar = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
	toolbar_sizer->Add(toolbar, 0, wxEXPAND|wxALL);
	toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL); 
	sizer->Add(splitter_win, 1, wxEXPAND|wxALL); 
    SetSizer(sizer);
    
    SetAutoLayout(true);
	SetTitle(title);
    DisplayStatusBar(true);
    
	Show(true);
	wxLogMessage("Exiting GroupingMapFrame::GroupingMapFrame");
}

HierachicalMapFrame::~HierachicalMapFrame()
{
	wxLogMessage("In GroupingMapFrame::~GroupingMapFrame");
}

void HierachicalMapFrame::OnActivate(wxActivateEvent& event)
{
	wxLogMessage("In GroupingMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("GroupingMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas )
        template_canvas->SetFocus();
}

void HierachicalMapFrame::MapMenus()
{
	wxLogMessage("In GroupingMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_HIERARCHICAL_MAP_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);	
	UpdateOptionMenuItems();
}

void HierachicalMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
        wxLogMessage("GroupingMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((HierachicalMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void HierachicalMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void HierachicalMapFrame::OnChangeConnRootSize(wxCommandEvent& event)
{
    int root_size = ((HierachicalMapCanvas*) template_canvas)->GetRootSize();
    wxString root_val;
    root_val << root_size;
    wxTextEntryDialog dlg(this, "Change Radius Size of Root Observations:",
                          "", root_val);
    if (dlg.ShowModal() == wxID_OK) {
        root_val = dlg.GetValue();
        long new_sz;
        if (root_val.ToLong(&new_sz)) {
            ((HierachicalMapCanvas*) template_canvas)->ChangeRootSize(new_sz);
        }
    }
}

void HierachicalMapFrame::OnChangeConnRootColor(wxCommandEvent& event)
{
    wxColour root_color = ((HierachicalMapCanvas*) template_canvas)->GetRootColor();
    root_color = GeneralWxUtils::PickColor(this, root_color);
    ((HierachicalMapCanvas*) template_canvas)->ChangeRootColor(root_color);
}

void HierachicalMapFrame::update(WeightsManState* o)
{
}
