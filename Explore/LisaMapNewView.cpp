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
#include "LisaCoordinator.h"
#include "LisaMapNewView.h"


IMPLEMENT_CLASS(LisaMapCanvas, AbstractMapCanvas)
BEGIN_EVENT_TABLE(LisaMapCanvas, AbstractMapCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

using namespace std;

LisaMapCanvas::LisaMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                             Project* project,
                             LisaCoordinator* lisa_coordinator,
                             CatClassification::CatClassifType theme_type_s,
                             bool isClust,
                             bool isBivariate,
                             bool isEBRate,
                             wxString menu_xrcid_s,
                             const wxPoint& pos, const wxSize& size)
: AbstractMapCanvas(parent, t_frame, project, lisa_coordinator, theme_type_s, isClust),
lisa_coord(lisa_coordinator),
is_bi(isBivariate),
is_rate(isEBRate),
is_diff(lisa_coordinator->lisa_type == LisaCoordinator::differential)
{
	wxLogMessage("Entering LisaMapCanvas::LisaMapCanvas");

    menu_xrcid = menu_xrcid_s;
    str_highhigh = _("High-High");
    str_highlow = _("High-Low");
    str_lowlow = _("Low-Low");
    str_lowhigh= _("Low-High");
    
    SetPredefinedColor(str_highhigh, wxColour(255, 0, 0));
    SetPredefinedColor(str_highlow, wxColour(255, 150, 150));
    SetPredefinedColor(str_lowlow, wxColour(0, 0, 255));
    SetPredefinedColor(str_lowhigh, wxColour(150, 150, 255));

	CreateAndUpdateCategories();

	wxLogMessage("Exiting LisaMapCanvas::LisaMapCanvas");
}

LisaMapCanvas::~LisaMapCanvas()
{
	wxLogMessage("In LisaMapCanvas::~LisaMapCanvas");
}

wxString LisaMapCanvas::GetCanvasTitle()
{
	wxString lisa_t;
	if (is_clust && !is_bi) lisa_t = _(" LISA Cluster Map");
	if (is_clust && is_bi) lisa_t = _(" BiLISA Cluster Map");
    if (is_clust && is_diff) lisa_t = _(" Differential LISA Cluster Map");
    
	if (!is_clust && !is_bi) lisa_t = _(" LISA Significance Map");
	if (!is_clust && is_bi) lisa_t = _(" BiLISA Significance Map");
    if (!is_clust && is_diff) lisa_t = _(" Differential Significance Map");
	
	wxString field_t;
	if (is_bi) {
		field_t << GetNameWithTime(0) << " w/ " << GetNameWithTime(1);
    } else if (is_diff) {
        field_t << GetNameWithTime(0) << " - " << GetNameWithTime(1);
    }else {
		field_t << "I_" << GetNameWithTime(0);
	}
	if (is_rate) {
		field_t << "EB Rate: " << GetNameWithTime(0);
		field_t << " / " << GetNameWithTime(1);
	}
	
	wxString ret;
	ret << lisa_t << ": " << lisa_coord->GetWeightsName() << ", ";
	ret << field_t << " (" << lisa_coord->GetNumPermutations() << " perm)";
	return ret;
}

wxString LisaMapCanvas::GetVariableNames()
{
    wxString field_t;
    if (is_bi) {
        field_t << GetNameWithTime(0) << "," << GetNameWithTime(1);
    } else if (is_diff) {
        field_t << GetNameWithTime(0) << " - " << GetNameWithTime(1);
    }else {
        field_t << GetNameWithTime(0);
    }
    if (is_rate) {
        field_t << GetNameWithTime(0);
        field_t << " / " << GetNameWithTime(1);
    }

    return field_t;
}

/** Update Categories based on info in LisaCoordinator */
void LisaMapCanvas::SetLabelsAndColorForClusters(int& num_cats, int t, CatClassifData& cat_data)
{
    // NotSig LL HH LH HL
    num_cats += 5;
    cat_data.CreateCategoriesAtCanvasTm(num_cats, t);
    
    int undefined_cat = -1;
    int isolates_cat = -1;
    bool has_isolates = a_coord->GetHasIsolates(t);
    bool has_undefined = a_coord->GetHasUndefined(t);
    double sig_cutoff = a_coord->GetSignificanceCutoff();
    double* p = a_coord->GetLocalSignificanceValues(t);
    int* cluster = a_coord->GetClusterIndicators(t);
    
    wxColour not_sig_clr = clr_not_sig_polygon;
    if (project->IsPointTypeData()) not_sig_clr = clr_not_sig_point;
    cat_data.SetCategoryColor(t, 0, not_sig_clr);
    cat_data.SetCategoryLabel(t, 0, str_not_sig);
    
    cat_data.SetCategoryLabel(t, 1, str_highhigh);
    cat_data.SetCategoryColor(t, 1, lbl_color_dict[str_highhigh]);
    cat_data.SetCategoryLabel(t, 2, str_lowlow);
    cat_data.SetCategoryColor(t, 2, lbl_color_dict[str_lowlow]);
    cat_data.SetCategoryLabel(t, 3, str_lowhigh);
    cat_data.SetCategoryColor(t, 3, lbl_color_dict[str_lowhigh]);
    cat_data.SetCategoryLabel(t, 4, str_highlow);
    cat_data.SetCategoryColor(t, 4, lbl_color_dict[str_highlow]);
    
    if (has_isolates && has_undefined) {
        isolates_cat = 5;
        undefined_cat = 6;
    } else if (has_undefined) {
        undefined_cat = 5;
    } else if (has_isolates) {
        isolates_cat = 5;
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
            cat_data.AppendIdToCategory(t, cluster[i], i);
        }
    }
}

IMPLEMENT_CLASS(LisaMapFrame, AbstractMapFrame)
	BEGIN_EVENT_TABLE(LisaMapFrame, AbstractMapFrame)
	EVT_ACTIVATE(LisaMapFrame::OnActivate)
END_EVENT_TABLE()

LisaMapFrame::LisaMapFrame(wxFrame *parent, Project* project,
                           LisaCoordinator* lisa_coordinator,
                           bool isClusterMap,
                           bool isBivariate,
                           bool isEBRate,
                           const wxPoint& pos, const wxSize& size,
                           const long style)
: AbstractMapFrame(parent, project, lisa_coordinator),
lisa_coord(lisa_coordinator),
is_clust(isClusterMap),
is_bivariate(isBivariate),
is_ebrate(isEBRate)
{
	wxLogMessage("Entering LisaMapFrame::LisaMapFrame");
    no_update_weights = true;
    Init();
	wxLogMessage("Exiting LisaMapFrame::LisaMapFrame");
}

LisaMapFrame::~LisaMapFrame()
{
	wxLogMessage("In LisaMapFrame::~LisaMapFrame");
    // the following code block will be exceuted in AbstractClusterMapFrame
	//if (lisa_coord) {
	//	lisa_coord->removeObserver(this);
	//	lisa_coord = 0;
	//}
}

CatClassification::CatClassifType LisaMapFrame::GetThemeType()
{
    theme_type = is_clust ? CatClassification::lisa_categories : CatClassification::lisa_significance;
    return theme_type;
}

TemplateCanvas* LisaMapFrame::CreateMapCanvas(wxPanel* panel)
{
    menu_xrcid = "ID_LISAMAP_NEW_VIEW_MENU_OPTIONS";
    return new LisaMapCanvas(panel, this, project,
                             lisa_coord,
                             theme_type,
                             is_clust,
                             is_bivariate,
                             is_ebrate,
                             menu_xrcid);
}

void LisaMapFrame::OnSaveResult(wxCommandEvent& event)
{
    
	int t = template_canvas->cat_data.GetCurrentCanvasTmStep();
    LisaMapCanvas* lc = (LisaMapCanvas*)template_canvas;
    
    std::vector<SaveToTableEntry> data;
    
    if (lc->is_diff || lc->is_rate) {
        data.resize(4);
    } else {
        data.resize(3);
    }
   
    std::vector<bool> undefs(lisa_coord->num_obs, false);
    for (int i=0; i<lisa_coord->undef_data[0][t].size(); i++){
        undefs[i] = undefs[i] || lisa_coord->undef_data[0][t][i];
    }
    
	std::vector<double> tempLocalMoran(lisa_coord->num_obs);
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		tempLocalMoran[i] = lisa_coord->local_moran_vecs[t][i];
	}
	data[0].d_val = &tempLocalMoran;
	data[0].label = "Lisa Indices";
	data[0].field_default = "LISA_I";
	data[0].type = GdaConst::double_type;
    data[0].undefined = &undefs;
	
	double cuttoff = lisa_coord->GetSignificanceCutoff();
	double* p = lisa_coord->GetLocalSignificanceValues(t);
	int* cluster = lisa_coord->GetClusterIndicators(t);
	std::vector<wxInt64> clust(lisa_coord->num_obs);
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		if (p[i] > cuttoff && cluster[i] != 5 && cluster[i] != 6) {
			clust[i] = 0; // not significant
		} else {
			clust[i] = cluster[i];
		}
	}
	data[1].l_val = &clust;
	data[1].label = "Clusters";
	data[1].field_default = "LISA_CL";
	data[1].type = GdaConst::long64_type;
    data[1].undefined = &undefs;
	
	std::vector<double> sig(lisa_coord->num_obs);
    std::vector<double> diff(lisa_coord->num_obs);
    
	for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
		sig[i] = p[i];
        if (lc->is_diff ) {
            int t0 =  lisa_coord->var_info[0].time;
            int t1 =  lisa_coord->var_info[1].time;
            diff[i] = lisa_coord->data[0][t0][i] - lisa_coord->data[0][t1][i];
        }
	}
	
	data[2].d_val = &sig;
	data[2].label = "Significance";
	data[2].field_default = "LISA_P";
	data[2].type = GdaConst::double_type;
    data[2].undefined = &undefs;
	
    if (lc->is_diff) {
        data[3].d_val = &diff;
        data[3].label = "Diff Values";
        data[3].field_default = "DIFF_VAL2";
        data[3].type = GdaConst::double_type;
        data[3].undefined = &undefs;
    }
    if (lc->is_rate) {
        std::vector<double> ebr(lisa_coord->num_obs);
        for (int i=0, iend=lisa_coord->num_obs; i<iend; i++) {
            ebr[i] = lisa_coord->smoothed_results[t][i];
        }
        data[3].d_val = &ebr;
        data[3].label = "EB Rates";
        data[3].field_default = "LISA_EB";
        data[3].type = GdaConst::double_type;
        data[3].undefined = &undefs;
    }
	SaveToTableDlg dlg(project, this, data,
					   "Save Results: LISA",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void LisaMapFrame::OnShowAsConditionalMap(wxCommandEvent& event)
{
    VariableSettingsDlg dlg(project, VariableSettingsDlg::bivariate,
                            false, false,
                            _("Conditional LISA Map Variables"),
                            _("Horizontal Cells"), _("Vertical Cells"),
                            "", "", false, false, false, // default
                            true, true, false, false);
    
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }
    
	LisaMapCanvas* lc = (LisaMapCanvas*) template_canvas;
    wxString title = lc->GetCanvasTitle();
    ConditionalClusterMapFrame* subframe =
    new ConditionalClusterMapFrame(this, project,
                                   dlg.var_info, dlg.col_ids, lisa_coord,
                                   title, wxDefaultPosition,
                                   GdaConst::cond_view_default_size);
}
