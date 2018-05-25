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
#include "../VarCalc/WeightsManInterface.h"

#include "ConditionalClusterMapView.h"
#include "AbstractCoordinator.h"
#include "AbstractClusterMap.h"


IMPLEMENT_CLASS(AbstractMapCanvas, MapCanvas)
BEGIN_EVENT_TABLE(AbstractMapCanvas, MapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

using namespace std;

AbstractMapCanvas::AbstractMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project,
                             AbstractCoordinator* a_coordinator,
                             CatClassification::CatClassifType theme_type_s,
                                     bool is_clust_s,
                             const wxPoint& pos, const wxSize& size)
:MapCanvas(parent, t_frame, project,
           vector<GdaVarTools::VarInfo>(0), vector<int>(0),
           CatClassification::no_theme,
           no_smoothing, 1, boost::uuids::nil_uuid(), pos, size),
a_coord(a_coordinator),
is_clust(is_clust_s),
str_not_sig(_("Not Significant")),
str_undefined(_("Undefined")),
str_neighborless(_("Neighborless")),
str_p005("p = 0.05"),
str_p001("p = 0.01"),
str_p0001("p = 0.001"),
str_p00001("p = 0.00001"),
clr_not_sig_point(wxColour(190, 190, 190)),
clr_not_sig_polygon(wxColour(240, 240, 240))
{
	wxLogMessage("Entering AbstractMapCanvas::AbstractMapCanvas");

    SetPredefinedColor(str_not_sig, wxColour(240, 240, 240));
    SetPredefinedColor(str_undefined, wxColour(70, 70, 70));
    SetPredefinedColor(str_neighborless, wxColour(140, 140, 140));
    SetPredefinedColor(str_p005, wxColour(75, 255, 80));
    SetPredefinedColor(str_p001, wxColour(6, 196, 11));
    SetPredefinedColor(str_p0001, wxColour(3, 116, 6));
    SetPredefinedColor(str_p00001, wxColour(1, 70, 3));
    
	cat_classif_def.cat_classif_type = theme_type_s;
	// must set var_info times from AbstractCoordinator initially
	var_info = a_coordinator->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0, sz=var_info.size(); t<sz; ++t) {
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	
    UpdateStatusBar();
	wxLogMessage("Exiting AbstractMapCanvas::AbstractMapCanvas");
}

AbstractMapCanvas::~AbstractMapCanvas()
{
	wxLogMessage("In AbstractMapCanvas::~AbstractMapCanvas");
}

void AbstractMapCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	wxLogMessage("Entering AbstractMapCanvas::DisplayRightClickMenu");
	// Workaround for right-click not changing window focus in OSX / wxW 3.0
	wxActivateEvent ae(wxEVT_NULL, true, 0, wxActivateEvent::Reason_Mouse);
	((AbstractMapFrame*) template_frame)->OnActivate(ae);
	
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu(menu_xrcid);
	AddTimeVariantOptionsToMenu(optMenu);
	SetCheckMarks(optMenu);
	
	template_frame->UpdateContextMenuItems(optMenu);
	template_frame->PopupMenu(optMenu, pos + GetPosition());
	template_frame->UpdateOptionMenuItems();
	wxLogMessage("Exiting AbstractMapCanvas::DisplayRightClickMenu");
}

wxString AbstractMapCanvas::GetCanvasTitle()
{
	return "";
}

/** This method definition is empty.  It is here to override any call
 to the parent-class method since smoothing and theme changes are not
 supported by Abstract maps */
bool
AbstractMapCanvas::ChangeMapType(CatClassification::CatClassifType new_map_theme,
                             SmoothingType new_map_smoothing)
{
	wxLogMessage("In AbstractMapCanvas::ChangeMapType");
	return false;
}

void AbstractMapCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	MapCanvas::SetCheckMarks(menu);
	
	int sig_filter = a_coord->GetSignificanceFilter();
	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_05"),
								  sig_filter == 1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_01"),
								  sig_filter == 2);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_001"),
								  sig_filter == 3);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_0001"),
								  sig_filter == 4);
    GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SIGNIFICANCE_FILTER_SETUP"),
                                  sig_filter == -1);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_USE_SPECIFIED_SEED"),
								  a_coord->IsReuseLastSeed());
}

void AbstractMapCanvas::TimeChange()
{
	wxLogMessage("Entering AbstractMapCanvas::TimeChange");
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
	wxLogMessage("Exiting AbstractMapCanvas::TimeChange");
}

/** Update Categories based on info in AbstractCoordinator */
void AbstractMapCanvas::CreateAndUpdateCategories()
{
	SyncVarInfoFromCoordinator();
	cat_data.CreateEmptyCategories(num_time_vals, num_obs);

    
    double sig_cutoff = a_coord->GetSignificanceCutoff();
    int    s_f = a_coord->GetSignificanceFilter();
    int    set_perm = a_coord->GetNumPermutations();
    int    num_obs = a_coord->num_obs;
    double stop_sig = 1.0 / (1.0 + set_perm);
    std::vector<wxString> def_cats = a_coord->GetDefaultCategories();
    std::vector<double> def_cutoffs = a_coord->GetDefaultCutoffs();
   
    bool is_cust_cutoff = true;
    for (int i=0; i<4; i++) {
        if (sig_cutoff == def_cutoffs[i]) {
            is_cust_cutoff = false;
            break;
        }
    }
    
    if ( is_cust_cutoff ) {
        // if set customized cutoff value
        wxString lbl = wxString::Format("p = %g", sig_cutoff);
        if ( sig_cutoff > 0.05 ) {
            def_cutoffs[0] = sig_cutoff;
            lbl_color_dict[lbl] = lbl_color_dict[def_cats[0]];
            def_cats[0] = lbl;
        } else {
            for (int i = 1; i < 4; i++) {
                if (def_cutoffs[i-1] + def_cutoffs[i] < 2 * sig_cutoff){
                    lbl_color_dict[lbl] = lbl_color_dict[def_cats[i-1]];
                    def_cutoffs[i-1] = sig_cutoff;
                    def_cats[i-1] = lbl;
                    break;
                } else {
                    lbl_color_dict[lbl] = lbl_color_dict[def_cats[i]];
                    def_cutoffs[i] = sig_cutoff;
                    def_cats[i] = lbl;
                    break;
                }
            }
        }
    }
    
	for (int t=0; t<num_time_vals; t++) {
		if (!map_valid[t]) break;
		
        bool has_isolates = a_coord->GetHasIsolates(t);
        bool has_undefined = a_coord->GetHasUndefined(t);
        double* p = a_coord->GetLocalSignificanceValues(t);
        int* cluster = a_coord->GetClusterIndicators(t);

		int undefined_cat = -1;
		int isolates_cat = -1;
		int num_cats = 0;

		if ( has_isolates )
            num_cats++;
        
		if ( has_undefined )
            num_cats++;
        
		if (is_clust) {
            // NotSig LL HH LH HL
            SetLabelsAndColorForClusters(num_cats, t, cat_data);

		} else {
            // significance map
            // 0: >0.05 (Not sig) 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
            num_cats += 5;
            for (int j=0; j < def_cats.size(); j++) {
                if (sig_cutoff < def_cutoffs[j])
                    num_cats -= 1;
            }
            // issue #474 only show significance levels that can
            // be mapped for the given number of permutations,
            // e.g., for 99 it would stop at 0.01, for 999 at 0.001, etc.
            if ( sig_cutoff >= def_cutoffs[3] && stop_sig > def_cutoffs[3] ){ //0.0001
                num_cats -= 1;
            }
            if ( sig_cutoff >= def_cutoffs[2] && stop_sig > def_cutoffs[2] ){ //0.001
                num_cats -= 1;
            }
            if ( sig_cutoff >= def_cutoffs[1] && stop_sig > def_cutoffs[1] ){ //0.01
                num_cats -= 1;
            }
            cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
            
            wxColour not_sig_clr = clr_not_sig_polygon;
            if (project->IsPointTypeData()) not_sig_clr = clr_not_sig_point;
            cat_data.SetCategoryColor(t, 0, not_sig_clr);
            cat_data.SetCategoryLabel(t, 0, str_not_sig);

            int cat_idx = 1;
            std::map<int, int> level_cat_dict;
            for (int j=0; j < def_cats.size(); j++) {
                if (sig_cutoff >= def_cutoffs[j] && def_cutoffs[j] >= stop_sig) {
                    cat_data.SetCategoryColor(t, cat_idx, lbl_color_dict[def_cats[j]]);
                    cat_data.SetCategoryLabel(t, cat_idx, def_cats[j]);
                    level_cat_dict[j] = cat_idx;
                    cat_idx += 1;
                }
            }
            
            if (has_isolates && has_undefined) {
                isolates_cat = cat_idx++;
                undefined_cat = cat_idx++;
                
            } else if (has_undefined) {
                undefined_cat = cat_idx++;
                
            } else if (has_isolates) {
                isolates_cat = cat_idx++;
            }
            
            if (undefined_cat != -1) {
                cat_data.SetCategoryLabel(t, undefined_cat, str_undefined);
                cat_data.SetCategoryColor(t, undefined_cat, lbl_color_dict[str_undefined]);
            }
            if (isolates_cat != -1) {
                cat_data.SetCategoryLabel(t, isolates_cat, str_neighborless);
                cat_data.SetCategoryColor(t, isolates_cat, lbl_color_dict[str_neighborless]);
            }
            for (int i=0; i<num_obs; i++) {
                if (p[i] > sig_cutoff && cluster[i] != 5 && cluster[i] != 6) {
                    cat_data.AppendIdToCategory(t, 0, i); // not significant
                } else if (cluster[i] == 5) {
                    cat_data.AppendIdToCategory(t, isolates_cat, i);
                } else if (cluster[i] == 6) {
                    cat_data.AppendIdToCategory(t, undefined_cat, i);
                } else {
                    //cat_data.AppendIdToCategory(t, (sigCat[i]-s_f)+1, i);
                    for ( int c = def_cats.size()-1; c >= 0; c-- ) {
                        if ( p[i] <= def_cutoffs[c] ) {
                            cat_data.AppendIdToCategory(t, level_cat_dict[c], i);
                            break;
                        }
                    }
                }
            }
		}
		
		for (int cat=0; cat<num_cats; cat++) {
            int cnt = cat_data.GetNumObsInCategory(t, cat);
            cat_data.SetCategoryCount(t, cat, cnt);
		}
	}
	
	if (ref_var_index != -1) {
        int stps = var_info[ref_var_index].time - var_info[ref_var_index].time_min;
		cat_data.SetCurrentCanvasTmStep(stps);
	}
	PopulateCanvas();
}

/** Copy everything in var_info except for current time field for each
 variable.  Also copy over is_any_time_variant, is_any_sync_with_global_time,
 ref_var_index, num_time_vales, map_valid and map_error_message */
void AbstractMapCanvas::SyncVarInfoFromCoordinator()
{
	std::vector<int>my_times(var_info.size());
	for (int t=0; t<var_info.size(); t++) my_times[t] = var_info[t].time;
	var_info = a_coord->var_info;
	template_frame->ClearAllGroupDependencies();
	for (int t=0; t<var_info.size(); t++) {
		var_info[t].time = my_times[t];
		template_frame->AddGroupDependancy(var_info[t].name);
	}
	is_any_time_variant = a_coord->is_any_time_variant;
	is_any_sync_with_global_time = a_coord->is_any_sync_with_global_time;
	ref_var_index = a_coord->ref_var_index;
	num_time_vals = a_coord->num_time_vals;
	map_valid = a_coord->map_valid;
	map_error_message = a_coord->map_error_message;
}

void AbstractMapCanvas::TimeSyncVariableToggle(int var_index)
{
	LOG_MSG("In AbstractMapCanvas::TimeSyncVariableToggle");
	a_coord->var_info[var_index].sync_with_global_time =
		!a_coord->var_info[var_index].sync_with_global_time;
	for (int i=0; i<var_info.size(); i++) {
		a_coord->var_info[i].time = var_info[i].time;
	}
	a_coord->VarInfoAttributeChange();
	a_coord->InitFromVarInfo();
	a_coord->notifyObservers();
}

void AbstractMapCanvas::UpdateStatusBar()
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
    if (is_clust && a_coord) {
        double p_val = a_coord->GetSignificanceCutoff();
        wxString inf_str = wxString::Format(" p <= %g", p_val);
        s << inf_str;
    }
    sb->SetStatusText(s);
}

IMPLEMENT_CLASS(AbstractMapFrame, MapFrame)
	BEGIN_EVENT_TABLE(AbstractMapFrame, MapFrame)
	EVT_ACTIVATE(AbstractMapFrame::OnActivate)
END_EVENT_TABLE()

AbstractMapFrame::AbstractMapFrame(wxFrame *parent, Project* project,
                                   AbstractCoordinator* a_coordinator,
                                   const wxPoint& pos, const wxSize& size,
                                   const long style)
: MapFrame(parent, project, pos, size, style),
a_coord(a_coordinator)
{
	LOG_MSG("Entering AbstractMapFrame::AbstractMapFrame");
}

void AbstractMapFrame::Init()
{
	wxSplitterWindow* splitter_win = new wxSplitterWindow(this,-1,
        wxDefaultPosition, wxDefaultSize,
        wxSP_3D|wxSP_LIVE_UPDATE|wxCLIP_CHILDREN);
	splitter_win->SetMinimumPaneSize(10);
	
    CatClassification::CatClassifType theme_type = GetThemeType();
    
    wxPanel* rpanel = new wxPanel(splitter_win);
    template_canvas = CreateMapCanvas(rpanel);
	template_canvas->SetScrollRate(1,1);
    wxBoxSizer* rbox = new wxBoxSizer(wxVERTICAL);
    rbox->Add(template_canvas, 1, wxEXPAND);
    rpanel->SetSizer(rbox);
    
    WeightsManInterface* w_man_int = project->GetWManInt();
    boost::uuids::uuid w_id = w_man_int->GetDefault();
    ((AbstractMapCanvas*) template_canvas)->SetWeightsId(w_id);
    
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
    SetAutoLayout(true);

	SetTitle(template_canvas->GetCanvasTitle());
    
	a_coord->registerObserver(this);
    
	Show(true);
    DisplayStatusBar(true);
    
	LOG_MSG("Exiting AbstractMapFrame::AbstractMapFrame");
}

AbstractMapFrame::~AbstractMapFrame()
{
	LOG_MSG("In AbstractMapFrame::~AbstractMapFrame");
	if (a_coord) {
		a_coord->removeObserver(this);
		a_coord = NULL;
	}
}

void AbstractMapFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In AbstractMapFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("AbstractMapFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}

void AbstractMapFrame::MapMenus()
{
	LOG_MSG("In AbstractMapFrame::MapMenus");
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->LoadMenu(menu_xrcid);
	((MapCanvas*) template_canvas)->
		AddTimeVariantOptionsToMenu(optMenu);
	((MapCanvas*) template_canvas)->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, _("Options"), optMenu);	
	UpdateOptionMenuItems();
}

void AbstractMapFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu(_("Options"));
    if (menu == wxNOT_FOUND) {
        LOG_MSG("AbstractMapFrame::UpdateOptionMenuItems: "
				"Options menu not found");
	} else {
		((AbstractMapCanvas*) template_canvas)->SetCheckMarks(mb->GetMenu(menu));
	}
}

void AbstractMapFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	
	TemplateFrame::UpdateContextMenuItems(menu); // set common items
}

void AbstractMapFrame::RanXPer(int permutation)
{
	if (permutation < 9) permutation = 9;
	if (permutation > 99999) permutation = 99999;
	a_coord->SetNumPermutations(permutation);
	a_coord->CalcPseudoP();
	a_coord->notifyObservers();
}

void AbstractMapFrame::OnRan99Per(wxCommandEvent& event)
{
	RanXPer(99);
}

void AbstractMapFrame::OnRan199Per(wxCommandEvent& event)
{
	RanXPer(199);
}

void AbstractMapFrame::OnRan499Per(wxCommandEvent& event)
{
	RanXPer(499);
}

void AbstractMapFrame::OnRan999Per(wxCommandEvent& event)
{
	RanXPer(999);  
}

void AbstractMapFrame::OnRanOtherPer(wxCommandEvent& event)
{
	PermutationCounterDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		long num;
		wxString input = dlg.m_number->GetValue();
        
        wxLogMessage(input);
        
        input.ToLong(&num);
		RanXPer(num);
	}
}

void AbstractMapFrame::OnUseSpecifiedSeed(wxCommandEvent& event)
{
	a_coord->SetReuseLastSeed(!a_coord->IsReuseLastSeed());
}

void AbstractMapFrame::OnSpecifySeedDlg(wxCommandEvent& event)
{
	uint64_t last_seed = a_coord->GetLastUsedSeed();
	wxString m;
	m << "The last seed used by the pseudo random\nnumber ";
	m << "generator was " << last_seed << ".\n";
	m << "\nEnter a seed value to use between\n0 and ";
	m << std::numeric_limits<uint64_t>::max() << ".";
	long long unsigned int val;
	wxString dlg_val;
	wxString cur_val;
	cur_val << last_seed;
	
	wxTextEntryDialog dlg(NULL, m, _("Enter a seed value"), cur_val);
	if (dlg.ShowModal() != wxID_OK) return;
	dlg_val = dlg.GetValue();
	dlg_val.Trim(true);
	dlg_val.Trim(false);
	if (dlg_val.IsEmpty()) return;
	if (dlg_val.ToULongLong(&val)) {
		if (!a_coord->IsReuseLastSeed()) a_coord->SetLastUsedSeed(true);
		uint64_t new_seed_val = val;
		a_coord->SetLastUsedSeed(new_seed_val);
	} else {
        wxString m = _("\"%s\" is not a valid seed. Seed unchanged.");
        m = wxString::Format(m, dlg_val);
		wxMessageDialog dlg(NULL, m, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void AbstractMapFrame::SetSigFilterX(int filter)
{
	if (filter == a_coord->GetSignificanceFilter()) return;
	a_coord->SetSignificanceFilter(filter);
	a_coord->notifyObservers();
	UpdateOptionMenuItems();
}

void AbstractMapFrame::OnSigFilter05(wxCommandEvent& event)
{
	SetSigFilterX(1);
}

void AbstractMapFrame::OnSigFilter01(wxCommandEvent& event)
{
	SetSigFilterX(2);
}

void AbstractMapFrame::OnSigFilter001(wxCommandEvent& event)
{
	SetSigFilterX(3);
}

void AbstractMapFrame::OnSigFilter0001(wxCommandEvent& event)
{
	SetSigFilterX(4);
}

void AbstractMapFrame::OnSigFilterSetup(wxCommandEvent& event)
{
    AbstractMapCanvas* lc = (AbstractMapCanvas*)template_canvas;
    int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
    double* p = a_coord->GetLocalSignificanceValues(t);
    int n = a_coord->num_obs;
    wxString ttl = _("Inference Settings (%d perm)");
    ttl = wxString::Format(ttl, a_coord->GetNumPermutations());

    double user_sig = a_coord->GetSignificanceCutoff();
    int sig_filter = a_coord->GetSignificanceFilter();
    if (sig_filter < 0) user_sig = a_coord->GetUserCutoff();
    
    InferenceSettingsDlg dlg(this, user_sig, p, n, ttl);
    if (dlg.ShowModal() == wxID_OK) {
        a_coord->SetSignificanceFilter(-1);
        a_coord->SetSignificanceCutoff(dlg.GetAlphaLevel());
        a_coord->SetUserCutoff(dlg.GetUserInput());
        
        a_coord->notifyObservers();
        
        a_coord->SetBO(dlg.GetBO());
        a_coord->SetFDR(dlg.GetFDR());
        UpdateOptionMenuItems();
    }
}

void AbstractMapFrame::CoreSelectHelper(const std::vector<bool>& elem)
{
	HighlightState* highlight_state = project->GetHighlightState();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	for (int i=0; i<a_coord->num_obs; i++) {
		if (!hs[i] && elem[i]) {
            hs[i] = true;
            selection_changed  = true;
		} else if (hs[i] && !elem[i]) {
            hs[i] = false;
            selection_changed  = true;
		}
	}
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
}

void AbstractMapFrame::OnSelectCores(wxCommandEvent& event)
{
	wxLogMessage("Entering AbstractMapFrame::OnSelectCores");
	
	std::vector<bool> elem(a_coord->num_obs, false);
	int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int* clust = a_coord->GetClusterIndicators(ts);
	double* sig_val = a_coord->GetLocalSignificanceValues(ts);
    double user_sig = a_coord->GetSignificanceCutoff();
    
	// add all cores to elem list.
	for (int i=0; i<a_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4) {
            bool cont = true;
            if (sig_val[i] < user_sig) cont = false;
            if (cont)  continue;
			elem[i] = true;
		}
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting AbstractMapFrame::OnSelectCores");
}

void AbstractMapFrame::OnSelectNeighborsOfCores(wxCommandEvent& event)
{
	wxLogMessage("Entering AbstractMapFrame::OnSelectNeighborsOfCores");
	
	std::vector<bool> elem(a_coord->num_obs, false);
    int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int* clust = a_coord->GetClusterIndicators(ts);
    double* sig_val = a_coord->GetLocalSignificanceValues(ts);
    double user_sig = a_coord->GetSignificanceCutoff();
    const GalElement* W = a_coord->Gal_vecs_orig[ts]->gal;
    
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<a_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 ) {
            bool cont = true;
            if (sig_val[i] < user_sig) cont = false;
            if (cont)  continue;
            
			elem[i] = true;
			const GalElement& e = W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	// remove all cores
	for (int i=0; i<a_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 ) {
            bool cont = true;
            if (sig_val[i] < user_sig) cont = false;
            if (cont)  continue;
            
			elem[i] = false;
		}
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting AbstractMapFrame::OnSelectNeighborsOfCores");
}

void AbstractMapFrame::OnSelectCoresAndNeighbors(wxCommandEvent& event)
{
	wxLogMessage("Entering AbstractMapFrame::OnSelectCoresAndNeighbors");
	
	std::vector<bool> elem(a_coord->num_obs, false);
    int ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int* clust = a_coord->GetClusterIndicators(ts);
    double* sig_val = a_coord->GetLocalSignificanceValues(ts);
    double user_sig = a_coord->GetSignificanceCutoff();
    const GalElement* W = a_coord->Gal_vecs_orig[ts]->gal;
    
	// add all cores and neighbors of cores to elem list
	for (int i=0; i<a_coord->num_obs; i++) {
		if (clust[i] >= 1 && clust[i] <= 4 ) {
            bool cont = true;
            if (sig_val[i] < user_sig) cont = false;
            if (cont)  continue;

            
			elem[i] = true;
			const GalElement& e = W[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				elem[e[j]] = true;
			}
		}
	}
	CoreSelectHelper(elem);
	
	wxLogMessage("Exiting AbstractMapFrame::OnSelectCoresAndNeighbors");
}

/** Called by AbstractCoordinator to notify that state has changed.  State changes
 can include:
   - variable sync change and therefore all lisa categories have changed
   - significance level has changed and therefore categories have changed
   - new randomization for p-vals and therefore categories have changed */
void AbstractMapFrame::update(AbstractCoordinator* o)
{
	AbstractMapCanvas* lc = (AbstractMapCanvas*) template_canvas;
	lc->SyncVarInfoFromCoordinator();
	lc->CreateAndUpdateCategories();
	if (template_legend) template_legend->Recreate();
	SetTitle(lc->GetCanvasTitle());
	lc->Refresh();
    lc->UpdateStatusBar();
}

void AbstractMapFrame::closeObserver(AbstractCoordinator* o)
{
	wxLogMessage("In AbstractMapFrame::closeObserver(AbstractCoordinator*)");
	if (a_coord) {
		a_coord->removeObserver(this);
		a_coord = 0;
	}
	Close(true);
}

void AbstractMapFrame::GetVizInfo(std::vector<int>& clusters)
{
    // function called by PublishDlg, not used
	if (a_coord) {
        /*
		if(a_coord->sig_cat_vecs.size()>0) {
			for (int i=0; i<a_coord->num_obs;i++) {
				clusters.push_back(a_coord->sig_cat_vecs[0][i]);
			}
		}
         */
	}
}
