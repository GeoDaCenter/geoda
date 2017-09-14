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
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/PermutationCounterDlg.h"
#include "../DialogTools/SaveToTableDlg.h"
#include "../DialogTools/VariableSettingsDlg.h"
#include "../DialogTools/RandomizationDlg.h"
#include "../DialogTools/AbstractClusterDlg.h"

#include "ConditionalClusterMapView.h"
#include "MapNewView.h"
#include "ColocationMapView.h"

using namespace std;

BEGIN_EVENT_TABLE( ColocationSelectDlg, wxDialog )
EVT_CLOSE( ColocationSelectDlg::OnClose )
END_EVENT_TABLE()

ColocationSelectDlg::ColocationSelectDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Co-location Variable Settings"))
{
    wxLogMessage("Open ColocationSelectDlg.");
    
    CreateControls();
}

ColocationSelectDlg::~ColocationSelectDlg()
{
}

void ColocationSelectDlg::CreateControls()
{
    panel = new wxPanel(this);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    AddSimpleInputCtrls(panel, &combo_var, vbox);
  
    // Parameters
    //wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);
    gbox = new wxGridSizer(1,2,5,0);
    wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Co-location value:"),
                                         wxDefaultPosition, wxSize(128,-1));
    combo_co_value = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(200,-1), 0, NULL);
    gbox->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(combo_co_value, 1, wxEXPAND);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Parameters:");
    hbox->Add(gbox, 1, wxEXPAND);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("OK"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    //hbox2->Add(saveButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    //vbox->Add(hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
   
    panel->SetAutoLayout(true);
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    // Events
	combo_var->Bind(wxEVT_LISTBOX, &ColocationSelectDlg::OnVarSelect, this);
    okButton->Bind(wxEVT_BUTTON, &ColocationSelectDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &ColocationSelectDlg::OnClickClose, this);
}

void ColocationSelectDlg::OnVarSelect( wxCommandEvent& event)
{
    var_selections.Clear();
    combo_var->GetSelections(var_selections);
    int num_var = var_selections.size();
    if (num_var > 0) {
        // check selected variables for any co-located values, and add them to choice box
        
    }
}
void ColocationSelectDlg::OnOK( wxCommandEvent& event)
{
    int n_rows = gbox->GetRows() + 1;
    gbox->SetRows(n_rows);
    gbox->Add(new wxTextCtrl(panel, wxID_ANY, "value"), 0, wxALL | wxEXPAND, 0);
    gbox->Add(new wxButton(panel, wxID_ANY, "Remove"), 0, wxALL | wxEXPAND, 0);
    container->Layout();
    /*
    // check variable selection
    int num_var = var_selections.size();
    if (num_var < 2) {
        wxString m = _("Please select at least 2 varibles.");
        wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    // check co-locate value selection
    int idx = combo_co_value->GetSelection();
    
    if (idx <0 ) {
        wxString m = _("Please select a co-located value.");
        wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    wxString str_co_val = co_values[idx];
    long co_val;
    if (!str_co_val.ToLong(&co_val)) {
        wxString m = _("Please select a valid co-located value.");
        wxMessageDialog dlg(NULL, m, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
     */
}

void ColocationSelectDlg::OnClose( wxCloseEvent& event)
{
    wxLogMessage("ColocationSelectDlg::OnClose()");
    Destroy();
}

void ColocationSelectDlg::OnClickClose( wxCommandEvent& event)
{
    wxLogMessage("ColocationSelectDlg::OnClose()");
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

/*
IMPLEMENT_CLASS(ColocationMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(ColocationMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

ColocationMapCanvas::ColocationMapCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project, CatClassification::CatClassifType theme_type_s, const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project, vector<GdaVarTools::VarInfo>(0), vector<int>(0), CatClassification::no_theme, no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
{
	wxLogMessage("Entering ColocationMapCanvas::ColocationMapCanvas");

    str_not_sig = _("Not Significant");
    str_highhigh = _("High-High");
    str_highlow = _("High-Low");
    str_lowlow = _("Low-Low");
    str_lowhigh= _("Low-High");
    str_undefined = _("Undefined");
    str_neighborless = _("Neighborless");
    str_p005 = _("p = 0.05");
    str_p001 = _("p = 0.01");
    str_p0001 = _("p = 0.001");
    str_p00001 = _("p = 0.00001");
    
    SetPredefinedColor(str_not_sig, wxColour(240, 240, 240));
    SetPredefinedColor(str_highhigh, wxColour(255, 0, 0));
    SetPredefinedColor(str_highlow, wxColour(255, 150, 150));
    SetPredefinedColor(str_lowlow, wxColour(0, 0, 255));
    SetPredefinedColor(str_lowhigh, wxColour(150, 150, 255));
    SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    SetPredefinedColor(str_neighborless, wxColour(140, 140, 140));
    SetPredefinedColor(str_p005, wxColour(75, 255, 80));
    SetPredefinedColor(str_p001, wxColour(6, 196, 11));
    SetPredefinedColor(str_p0001, wxColour(3, 116, 6));
    SetPredefinedColor(str_p00001, wxColour(1, 70, 3));
    
	cat_classif_def.cat_classif_type = theme_type_s;
    
	template_frame->ClearAllGroupDependencies();
	for (int t=0, sz=var_info.size(); t<sz; ++t) {
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	CreateAndUpdateCategories();
    UpdateStatusBar();
    
	wxLogMessage("Exiting ColocationMapCanvas::ColocationMapCanvas");
}

ColocationMapCanvas::~ColocationMapCanvas()
{
	wxLogMessage("In ColocationMapCanvas::~ColocationMapCanvas");
}

void ColocationMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering ColocationMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((ColocationMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting ColocationMapCanvas::DisplayRightClickMenu");
}

wxString ColocationMapCanvas::GetCanvasTitle()
{
	wxString title;
	title = "Co-location Map";
	return title;
}

bool ColocationMapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme, SmoothingType new_map_smoothing)
{
	wxLogMessage("In ColocationMapCanvas::ChangeMapType");
	return false;
}

void ColocationMapCanvas::SetCheckMarks(wxMenu* menu)
{
	MapCanvas::SetCheckMarks(menu);
}

void ColocationMapCanvas::TimeChange()
{
	wxLogMessage("Entering ColocationMapCanvas::TimeChange");
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min)) return;
	if (cts > ref_time_max) {
		ref_time = ref_time_max;
	} else if (cts < ref_time_min) {
		ref_time = ref_time_min;
	} else {
		ref_time = cts;
	}
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time = ref_time + var_info[i].ref_time_offset;
		}
	}
	cat_data.SetCurrentCanvasTmStep(ref_time - ref_time_min);
	invalidateBms();
	PopulateCanvas();
	Refresh();
	wxLogMessage("Exiting ColocationMapCanvas::TimeChange");
}

void ColocationMapCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);
	
    int undefined_cat = -1;
    int num_cats = 0;
    
    cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
    cat_data.SetCategoryLabel(t, 0, str_highhigh);
    cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
    
    cat_data.SetCategoryLabel(t, 1, str_highhigh);
    cat_data.SetCategoryColor(t, 1, lbl_color_dict[str_highhigh]);
    
    cat_data.SetCategoryLabel(t, 2, str_lowlow);
    cat_data.SetCategoryColor(t, 2, lbl_color_dict[str_lowlow]); //wxColour(0, 0, 255));
    
    cat_data.SetCategoryLabel(t, 3, str_lowhigh); //"Low-High");
    cat_data.SetCategoryColor(t, 3, lbl_color_dict[str_lowhigh]);//wxColour(150, 150, 255));
    
    for (int i=0; i<num_obs; i++) {
        cat_data.AppendIdToCategory(t, 0, i); // not significant
    }
    
    for (int cat=0; cat<num_cats; cat++) {
        cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
    }
    
	PopulateCanvas();
}

void ColocationMapCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = lisa_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0; t<var_info.size(); t++) {
		var_info[t].time = my_times[t];
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	is_any_time_variant = lisa_coord->is_any_time_variant;
	is_any_sync_with_global_time = lisa_coord->is_any_sync_with_global_time;
	ref_var_index = lisa_coord->ref_var_index;
	num_time_vals = lisa_coord->num_time_vals;
	map_valid = lisa_coord->map_valid;
	map_error_message = lisa_coord->map_error_message;
}

void ColocationMapCanvas::TimeSyncVariableToggle(int var_index)
{
	wxLogMessage("In ColocationMapCanvas::TimeSyncVariableToggle");
	lisa_coord->var_info[var_index].sync_with_global_time =
		!lisa_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		lisa_coord->var_info[i].time = var_info[i].time;
	}
	lisa_coord->VarInfoAttributeChange();
	lisa_coord->InitFromVarInfo();
	lisa_coord->notifyObservers();
}

void ColocationMapCanvas::UpdateStatusBar()
{
    wxStatusBar* sb = 0;
    if (template_frame) {
        sb = template_frame->GetStatusBar();
    }
    if (!sb)
        return;
    wxString s;
    s << "#obs=" << project->GetNumRecords() <<" ";
    
    if ( highlight_state->GetTotalHighlighted() > 0) {
        // for highlight from other windows
        s << "#selected=" << highlight_state->GetTotalHighlighted()<< "  ";
    }
    if (mousemode == select && selectstate == start) {
        if (total_hover_obs >= 1) {
            s << "hover obs " << hover_obs[0]+1;
        }
        if (total_hover_obs >= 2) {
            s << ", ";
            s << "obs " << hover_obs[1]+1;
        }
        if (total_hover_obs >= 3) {
            s << ", ";
            s << "obs " << hover_obs[2]+1;
        }
        if (total_hover_obs >= 4) {
            s << ", ...";
        }
    }
    if (is_clust && lisa_coord) {
        double p_val = lisa_coord->significance_cutoff;
        wxString inf_str = wxString::Format(" p <= %g", p_val);
        s << inf_str;
    }
    sb->SetStatusText(s);
}

IMPLEMENT_CLASS(ColocationMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(ColocationMapFrame, MapFrame)
	EVT_ACTIVATE(ColocationMapFrame::OnActivate)
END_EVENT_TABLE()

ColocationMapFrame::ColocationMapFrame(wxFrame *parent, Project* project, const wxPoint& pos, const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style)
{
	wxLogMessage("Entering ColocationMapFrame::ColocationMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    CatClassification::CatClassifType theme_type_s = CatClassification::colocation;
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new ColocationMapCanvas(rpanel, this, project, theme_type_s);
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
    
    wxPanel* toolbar_panel = new wxPanel(this,-1, wxDefaultPosition);
	wxBoxSizer* toolbar_sizer= new wxBoxSizer(wxVERTICAL);
    wxToolBar* tb = wxXmlResource::Get()->LoadToolBar(toolbar_panel, "ToolBar_MAP");
    SetupToolbar();
	toolbar_sizer->Add(tb, 0, wxEXPAND|wxALL);
	toolbar_panel->SetSizerAndFit(toolbar_sizer);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_panel, 0, wxEXPAND|wxALL); 
	sizer->Add(splitter_win, 1, wxEXPAND|wxALL); 
    SetSizer(sizer);
    //splitter_win->SetSize(wxSize(width,height));
    
    SetAutoLayout(true);
	SetTitle(template_canvas->GetCanvasTitle());
    DisplayStatusBar(true);
    
	Show(true);
	wxLogMessage("Exiting ColocationMapFrame::ColocationMapFrame");
}

ColocationMapFrame::~ColocationMapFrame()
{
	wxLogMessage("In ColocationMapFrame::~ColocationMapFrame");
}

void ColocationMapFrame::OnActivate(wxActivateEvent& event)
{
	wxLogMessage("In ColocationMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("ColocationMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void ColocationMapFrame::MapMenus()
{
	wxLogMessage("In ColocationMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_LISAMAP_NEW_VIEW_MENU_OPTIONS");
	((MapCanvas*) template_canvas)->AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void ColocationMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
        wxLogMessage("ColocationMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((ColocationMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void ColocationMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void ColocationMapFrame::OnSave(wxCommandEvent& event)
{
	int t = this->GetCurrentCanvasTimeStep();
    
    ColocationMapCanvas* lc = (ColocationMapCanvas*)template_canvas;
    
    std::vector<SaveToTableEntry> data(1);
    std::vector<bool> undefs;
    
	data[0].l_val = &tempLocalMoran;
	data[0].label = "Coloc Indices";
	data[0].field_default = "CO_I";
	data[0].type = GdaConst::integer_type;
    data[0].undefined = &undefs;
    
	SaveToTableDlg dlg(project, this, data,
					   "Save Results: Co-locations",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void ColocationMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional Co-location Map Variables"),
                            _("Horizontal Cells"), _("Vertical Cells"),
                            "", "", false, false, false, // default
                            true, true, false, false);
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
	ColocationMapCanvas* lc = (ColocationMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    //ConditionalClusterMapFrame* subframe = new ConditionalClusterMapFrame(this, project, dlg.var_info, dlg.col_ids, lisa_coord, title, wxDefaultPosition, GdaConst::cond_view_default_size);
}

void ColocationMapFrame::closeObserver(LisaCoordinator* o)
{
	wxLogMessage("In ColocationMapFrame::closeObserver(LisaCoordinator*)");
	Close(true);
}
*/
