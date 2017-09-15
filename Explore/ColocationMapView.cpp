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

#include "ConditionalClusterMapView.h"
#include "MapNewView.h"
#include "ColocationMapView.h"

using namespace std;

BEGIN_EVENT_TABLE( ColocationSelectDlg, wxDialog )
EVT_CLOSE( ColocationSelectDlg::OnClose )
END_EVENT_TABLE()

ColocationSelectDlg::ColocationSelectDlg(wxFrame* parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Co-location Settings"))
{
    wxLogMessage("Open ColocationSelectDlg.");
    
    base_remove_id = XRCID("COL_REMOVE_BTN");
    base_color_id = XRCID("COL_COLOR_BTN");
    base_choice_id = XRCID("COL_CHOICE_BTN");
    
    GdaColorUtils::GetUnique20Colors(m_20colors);
    m_20colors[0] = wxColour(255,0,0);
    m_20colors[1] = wxColour(0,0,255);
    
    CreateControls();
}

ColocationSelectDlg::~ColocationSelectDlg()
{
}

wxColour ColocationSelectDlg::get_a_color(int idx)
{
    return m_20colors[idx % 20];
}

void ColocationSelectDlg::CreateControls()
{
    panel = new wxPanel(this);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Input
    AddSimpleInputCtrls(panel, &combo_var, vbox, true);
  
    // Parameters
    //wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);
    gbox = new wxFlexGridSizer(0,3,5,5);
    add_colo_control(true);

    wxBoxSizer *vvbox = new wxBoxSizer(wxVERTICAL);
    wxButton *addButton = new wxButton(panel, wxID_ANY, _("Add co-location"), wxDefaultPosition, wxDefaultSize);
    addButton->Bind(wxEVT_BUTTON, &ColocationSelectDlg::OnClickAdd, this);
    vvbox->Add(gbox, 1, wxEXPAND);
    vvbox->Add(addButton, 0, wxEXPAND | wxTOP, 20);
    
    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Setup co-locations:"));
    hbox->Add(vvbox, 1, wxEXPAND);
    
    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
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
    co_val_dict.clear();
    var_selections.Clear();
    combo_var->GetSelections(var_selections);
    int num_var = var_selections.size();
    if (num_var >= 2) {
        // check selected variables for any co-located values, and add them to choice box
        col_ids.resize(num_var);
        std::vector<wxString> col_names;
        col_names.resize(num_var);
        for (int i=0; i<num_var; i++) {
            int idx = var_selections[i];
            wxString nm = name_to_nm[combo_var->GetString(idx)];
            col_names.push_back(nm);
            int col = table_int->FindColId(nm);
            if (col == wxNOT_FOUND) {
                wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
                wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return;
            }
            col_ids[i] = col;
        }
        rows = project->GetNumRecords();
        columns =  0;
        
        std::vector<l_array_type> data;
        data.resize(col_ids.size()); // data[variable][time][obs]
        for (int i=0; i<col_ids.size(); i++) {
            table_int->GetColData(col_ids[i], data[i]);
        }
        
        vector<set<wxInt64> > same_val_counts(rows);
        
        for (int i=0; i<data.size(); i++ ){ // col
            for (int j=0; j<data[i].size(); j++) { // time
                columns += 1;
                for (int k=0; k< rows;k++) { // row
                    same_val_counts[k].insert(data[i][j][k]);
                }
            }
        }
        
        wxInt64 val;
        for (int i=0; i<rows; i++) {
            if (same_val_counts[i].size() == 1) {
                val = *(same_val_counts[i].begin());
                co_val_dict[val].push_back(i);
            }
        }
       
        if (check_colocations()==false)
            return;
        
        std::map<wxInt64, std::vector<int> >::iterator co_it;
        // insert to existing wxChoices
        for (int i=0; i<co_choices.size(); i++) {
            if (co_choices[i] ) {
                co_choices[i]->Clear();
                for (co_it = co_val_dict.begin(); co_it!=co_val_dict.end(); co_it++) {
                    wxString tmp;
                    tmp << co_it->first;
                    co_choices[i]->Append(tmp);
                }
                co_choices[i]->SetSelection(-1);
            }
        }
    }
    
}

bool ColocationSelectDlg::check_colocations()
{
    if (co_val_dict.empty()) {
        wxString err_msg =_("There is no co-location found in selected variable. Please select another variables with co-locations.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return false;
    }
    return true;
}

void ColocationSelectDlg::add_colo_control(bool is_new)
{
    int n_rows = gbox->GetRows() + 1;
    gbox->SetRows(n_rows);
    
    wxChoice* choice = new wxChoice(panel, base_choice_id+n_rows, wxDefaultPosition, wxSize(200,-1), 0, NULL);
    
    wxBitmap clr;
    wxColour sel_clr = get_a_color(n_rows-1);
    wxStaticBitmap* color_btn = new wxStaticBitmap(panel, base_color_id+n_rows, clr, wxDefaultPosition, wxSize(16,16));
    color_btn->SetBackgroundColour(sel_clr);
    
    wxButton* remove_btn = new wxButton(panel, base_remove_id+n_rows, "Remove");
    
    choice->Bind(wxEVT_CHOICE, &ColocationSelectDlg::OnClickCoVar, this);
    color_btn->Bind(wxEVT_LEFT_UP, &ColocationSelectDlg::OnClickColor, this);
    remove_btn->Bind(wxEVT_BUTTON, &ColocationSelectDlg::OnClickRemove, this);
  
    co_choices.push_back(choice);
    co_bitmaps.push_back(color_btn);
    co_removes.push_back(remove_btn);
    
    m_colors.push_back(sel_clr);
    
    gbox->Add(choice, 1, wxEXPAND);
    gbox->Add(color_btn, 0, wxALIGN_CENTER | wxRIGHT, 5);
    gbox->Add(remove_btn, 0);
    
    if (!is_new)    container->Layout();
    
    std::map<wxInt64, std::vector<int> >::iterator co_it;
    choice->Clear();
    for (co_it = co_val_dict.begin(); co_it!=co_val_dict.end(); co_it++) {
        wxString tmp;
        tmp << co_it->first;
        choice->Append(tmp);
    }
    choice->SetSelection(-1);
}

void ColocationSelectDlg::OnClickCoVar( wxCommandEvent& event)
{
    int obj_id = -1;
    wxChoice* obj = (wxChoice*) event.GetEventObject();
    for (int i=0, iend=co_choices.size(); i<iend && obj_id==-1; i++) {
        if (co_choices[i] && obj == co_choices[i])
            obj_id = i;
    }
    if (obj_id < 0) return;
    
    int cur_sel = obj->GetSelection();
    
    for (int i=0; i<co_choices.size(); i++) {
        if (co_choices[i] && co_choices[i] != obj) {
            if (co_choices[i]->GetSelection() == cur_sel) {
                wxString err_msg = _("Please select another value for co-location setup.");
                wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
                dlg.ShowModal();
                obj->SetSelection(-1);
                return;
            }
        }
    }
}

void ColocationSelectDlg::OnClickAdd( wxCommandEvent& event)
{
    //if (!co_val_dict.empty() && co_va
    add_colo_control(false);
}

void ColocationSelectDlg::OnClickColor( wxMouseEvent& event)
{
    int obj_id = -1;
    wxStaticBitmap* obj = (wxStaticBitmap*) event.GetEventObject();
    for (int i=0, iend=co_bitmaps.size(); i<iend && obj_id==-1; i++) {
        if (obj == co_bitmaps[i])
            obj_id = i;
    }
    
    if (obj_id < 0) return;
    
    wxColour col = m_colors[obj_id];
    
    wxColourData clr_data;
    clr_data.SetColour(col);
    clr_data.SetChooseFull(true);
    for (int ki = 0; ki < 16; ki++) {
        wxColour colour(ki * 16, ki * 16, ki * 16);
        clr_data.SetCustomColour(ki, colour);
    }
    
    wxColourDialog dialog(this, &clr_data);
    dialog.SetTitle("Choose Color");
    if (dialog.ShowModal() != wxID_OK)
        return;
    
    wxColourData retData = dialog.GetColourData();
    wxColour sel_clr = retData.GetColour();
    co_bitmaps[obj_id]->SetBackgroundColour(sel_clr);
    co_bitmaps[obj_id]->Refresh();
    m_colors[obj_id] = sel_clr;
}

int ColocationSelectDlg::count_rows()
{
    // check if there is only one row left
    int existing_rows = 0;
    for (int i=0; i<co_choices.size(); i++) {
        if (co_choices[i] != NULL) {
            existing_rows += 1;
        }
    }
    return existing_rows;
}

void ColocationSelectDlg::OnClickRemove( wxCommandEvent& event)
{
    // check if there is only one row left
    int existing_rows = count_rows();
    if (existing_rows == 1)
        return;
    
    int id = event.GetId();
    int idx = id - base_remove_id - 1;
    
    co_removes[idx]->Destroy();
    co_bitmaps[idx]->Destroy();
    co_choices[idx]->Destroy();
    
    co_removes[idx] = NULL;
    co_bitmaps[idx] = NULL;
    co_choices[idx] = NULL;
    
    container->Layout();
}

void ColocationSelectDlg::OnOK( wxCommandEvent& event)
{
    if (check_colocations()==false)
        return;
    
    int n_co = co_choices.size();
    
    vector<wxString> sel_vals;
    vector<wxColour> sel_clrs;
    vector<vector<int> > sel_ids;
    
    for (int i=0; i<n_co; i++) {
        if (co_choices[i] && co_bitmaps[i]) {
            int idx1 = co_choices[i]->GetSelection();
            if (idx1 >=0) {
                wxString sel_val = co_choices[i]->GetString(idx1);
                wxColour sel_clr = m_colors[i];
                long l_sel_val;
                if (sel_val.ToLong(&l_sel_val)) {
                    sel_ids.push_back( co_val_dict[l_sel_val] );
                    sel_vals.push_back(sel_val);
                    sel_clrs.push_back(sel_clr);
                }
            }
        }
    }
    
    if (sel_vals.empty()) {
        wxString err_msg = _("Please setup co-locations first.");
        wxMessageDialog dlg(NULL, err_msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    ColocationMapFrame* nf = new ColocationMapFrame(parent, project, sel_vals, sel_clrs, sel_ids,wxDefaultPosition, GdaConst::map_default_size);
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

IMPLEMENT_CLASS(ColocationMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(ColocationMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

ColocationMapCanvas::ColocationMapCanvas(wxWindow *parent, TemplateFrame* t_frame, Project* project, vector<wxString>& _co_vals, vector<wxColour>& _co_clrs, vector<vector<int> >& _co_ids, CatClassification::CatClassifType theme_type_s, const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project, vector<GdaVarTools::VarInfo>(0), vector<int>(0), CatClassification::no_theme, no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
co_vals(_co_vals), co_clrs(_co_clrs), co_ids(_co_ids)
{
	wxLogMessage("Entering ColocationMapCanvas::ColocationMapCanvas");

	cat_classif_def.cat_classif_type = theme_type_s;
   
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
		LoadMenu("ID_COLOCATION_VIEW_MENU_OPTIONS");
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
	wxLogMessage("Exiting ColocationMapCanvas::TimeChange");
}

void ColocationMapCanvas::CreateAndUpdateCategories()
{
	cat_data.CreateEmptyCategories(1, num_obs);
	
    int t = 0;
    int undefined_cat = -1;
    int num_cats = co_vals.size() + 1;
    
    cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
    
    map<int, bool> sig_dict;
    for (int i=0; i<num_obs; i++) {
        sig_dict[i] = false;
    }
    for (int i=0; i<co_vals.size(); i++) {
        cat_data.SetCategoryLabel(t, i+1, co_vals[i]);
        cat_data.SetCategoryColor(t, i+1, co_clrs[i]);
        for (int j=0; j<co_ids[i].size();j++) {
            cat_data.AppendIdToCategory(t, i+1, co_ids[i][j]);
            sig_dict[ co_ids[i][j] ] = true;
        }
    }
    
    // not significant
    cat_data.SetCategoryLabel(t, 0, "Others");
    cat_data.SetCategoryColor(t, 0, wxColour(240, 240, 240));
    for (int i=0; i<num_obs; i++) {
        if (sig_dict[i]==false) {
            cat_data.AppendIdToCategory(t, 0, i);
        }
    }
    
    // SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    
    for (int cat=0; cat<num_cats; cat++) {
        cat_data.SetCategoryCount(t, cat, cat_data.GetNumObsInCategory(t, cat));
    }
    
	PopulateCanvas();
}

void ColocationMapCanvas::TimeSyncVariableToggle(int var_index)
{
	wxLogMessage("In ColocationMapCanvas::TimeSyncVariableToggle");
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
    sb->SetStatusText(s);
}

IMPLEMENT_CLASS(ColocationMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(ColocationMapFrame, MapFrame)
	EVT_ACTIVATE(ColocationMapFrame::OnActivate)
END_EVENT_TABLE()

ColocationMapFrame::ColocationMapFrame(wxFrame *parent, Project* project, vector<wxString>& co_vals, vector<wxColour>& co_clrs, vector<vector<int> >& co_ids,const wxPoint& pos, const wxSize& size, const long style)
: MapFrame(parent, project, pos, size, style)
{
	wxLogMessage("Entering ColocationMapFrame::ColocationMapFrame");
	
	int width, height;
	GetClientSize(&width, &height);
    
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    CatClassification::CatClassifType theme_type_s = CatClassification::colocation;
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = new ColocationMapCanvas(rpanel, this, project, co_vals, co_clrs, co_ids, theme_type_s);
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
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_COLOCATION_VIEW_MENU_OPTIONS");
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
    int num_obs = lc->num_obs;
    
    std::vector<SaveToTableEntry> data(1);
    std::vector<bool> undefs(num_obs, true);
    std::vector<wxInt64> dt(num_obs);
    
    for (int i=0; i<lc->co_vals.size(); i++) {
        wxString tmp = lc->co_vals[i];
        long l_tmp;
        if (tmp.ToLong(&l_tmp)) {
            for (int j=0; j<lc->co_ids[i].size(); j++) {
                undefs[ lc->co_ids[i][j] ] = false;
                dt[ lc->co_ids[i][j] ] = l_tmp;
            }
        }
    }
    
	data[0].l_val = &dt;
	data[0].label = "Coloc Indices";
	data[0].field_default = "CO_I";
	data[0].type = GdaConst::long64_type;
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
