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

#ifndef __GEODA_CENTER_NEIGHBORMATCH_DLG_H__
#define __GEODA_CENTER_NEIGHBORMATCH_DLG_H__

#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/string.h>

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../DataViewer/TableInterface.h"
#include "../VarTools.h"
#include "../Explore/MapNewView.h"
#include "AbstractClusterDlg.h"

class NbrMatchDlg : public AbstractClusterDlg
{
public:
    NbrMatchDlg(wxFrame *parent, Project* project);
    virtual ~NbrMatchDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );

    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnSeedCheck(wxCommandEvent& event);
    void OnChangeSeed(wxCommandEvent& event);
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:

    wxChoice* m_distance;
    wxChoice* m_geo_dist_metric;
    wxTextCtrl* txt_knn;

    wxCheckBox* chk_seed;
    wxButton* seedButton;
    
    DECLARE_EVENT_TABLE()
};

class NbrMatchSaveWeightsDialog : public wxDialog
{
    wxChoice* m_id_field;
    std::vector<int> col_id_map;
    TableInterface* table_int;
public:
    NbrMatchSaveWeightsDialog(TableInterface* table_int, const wxString& title);
    void OnIdVariableSelected( wxCommandEvent& event );
    bool CheckID(const wxString& id);
    wxString GetSelectID();
};

class LocalMatchMapCanvas : public MapCanvas
{
    DECLARE_CLASS(LocalMatchMapCanvas)
public:
    LocalMatchMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                        Project* project,
                        const std::vector<std::vector<int> >& groups,
                        const std::vector<double>& pval,
                        boost::uuids::uuid weights_id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
    virtual ~LocalMatchMapCanvas();
    
    virtual void DisplayRightClickMenu(const wxPoint& pos);
    virtual wxString GetCanvasTitle();
    virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
                               SmoothingType new_map_smoothing);
    virtual void SetCheckMarks(wxMenu* menu);
    virtual void TimeChange();
    virtual void CreateAndUpdateCategories();
    virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();
    
    std::vector<std::vector<int> > groups;
    std::vector<double> pval;
    
    DECLARE_EVENT_TABLE()
};

class LocalMatchMapFrame : public MapFrame
{
    DECLARE_CLASS(LocalMatchMapFrame)
public:
    LocalMatchMapFrame(wxFrame *parent, Project* project,
                       const std::vector<std::vector<int> >& groups,
                       const std::vector<double>& pval,
                       boost::uuids::uuid weights_id = boost::uuids::nil_uuid(),
                       const wxString& title = wxEmptyString,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = GdaConst::map_default_size);
    virtual ~LocalMatchMapFrame();
    
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    virtual void update(WeightsManState* o);
    void OnSave(wxCommandEvent& event);
    
    boost::uuids::uuid weights_id;
    
    DECLARE_EVENT_TABLE()
};



class LocalMatchCoordinator
{
public:
    LocalMatchCoordinator(GwtWeight* spatial_w, GalWeight* variable_w,
                          const std::vector<wxInt64>& cadinality,
                          int permutations,
                          uint64_t last_seed_used,
                          bool reuse_last_seed=false);
    virtual ~LocalMatchCoordinator();

    bool IsOk() { return true; }
    wxString GetErrorMessage() { return "Error Message"; }

    void run();
    void job(size_t nbr_sz, size_t idx, uint64_t seed_start);

    int num_obs;
    GwtWeight* spatial_w;
    GalWeight* variable_w;
    std::vector<double> sigVal;
    std::vector<int> sigCat;
    std::vector<wxInt64> cadinality;

    int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
    double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
    void SetSignificanceFilter(int filter_id);
    int GetSignificanceFilter() { return significance_filter; }
    int permutations; // any number from 9 to 99999, 99 will be default
    double bo; //Bonferroni bound
    double fdr; //False Discovery Rate
    double user_sig_cutoff; // user defined cutoff
    uint64_t last_seed_used;
    bool reuse_last_seed;

    uint64_t GetLastUsedSeed() { return last_seed_used;}

    void SetLastUsedSeed(uint64_t seed) {
        reuse_last_seed = true;
        last_seed_used = seed;
        // update global one
        GdaConst::use_gda_user_seed = true;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
        GdaConst::gda_user_seed =  last_seed_used;
        wxString val;
        val << last_seed_used;
        OGRDataAdapter::GetInstance().AddEntry("gda_user_seed", val);
    }

    bool IsReuseLastSeed() { return reuse_last_seed; }
    void SetReuseLastSeed(bool reuse) {
        reuse_last_seed = reuse;
        // update global one
        GdaConst::use_gda_user_seed = reuse;
        if (reuse) {
            last_seed_used = GdaConst::gda_user_seed;
            OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
        } else {
            OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "0");
        }
    }

    virtual std::vector<wxString> GetDefaultCategories();
    virtual std::vector<double> GetDefaultCutoffs();
};


class LocalMatchSignificanceCanvas : public MapCanvas
{
    DECLARE_CLASS(LocalMatchSignificanceCanvas)
public:
    LocalMatchSignificanceCanvas(wxWindow *parent,
                  TemplateFrame* t_frame,
                  Project* project,
                  LocalMatchCoordinator* lm_coord,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);
    virtual ~LocalMatchSignificanceCanvas();
    virtual void DisplayRightClickMenu(const wxPoint& pos);
    virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
    virtual bool ChangeMapType(CatClassification::CatClassifType new_map_theme,
                               SmoothingType new_map_smoothing);
    virtual void SetCheckMarks(wxMenu* menu);
    virtual void TimeChange();
    void SyncVarInfoFromCoordinator();
    virtual void CreateAndUpdateCategories();
    virtual void TimeSyncVariableToggle(int var_index);
    virtual void UpdateStatusBar();
    virtual void SetWeightsId(boost::uuids::uuid id) { weights_id = id; }

    double bo;
    double fdr;
    
protected:
    LocalMatchCoordinator* gs_coord;
    bool is_clust; // true = cluster map, false = significance map
    
    wxString str_sig;
    wxString str_high;
    wxString str_med;
    wxString str_low;
    wxString str_undefined;
    wxString str_neighborless;
    wxString str_p005;
    wxString str_p001;
    wxString str_p0001;
    wxString str_p00001;
    
    DECLARE_EVENT_TABLE()
};

class LocalMatchSignificanceFrame : public MapFrame
{
    DECLARE_CLASS(LocalMatchSignificanceFrame)
public:
    
    LocalMatchSignificanceFrame(wxFrame *parent, Project* project,
                 LocalMatchCoordinator* gs_coordinator,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = GdaConst::map_default_size,
                 const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LocalMatchSignificanceFrame();
    
    void OnActivate(wxActivateEvent& event);
    virtual void MapMenus();
    virtual void UpdateOptionMenuItems();
    virtual void UpdateContextMenuItems(wxMenu* menu);
    virtual void update(WeightsManState* o){}
    
    void RanXPer(int permutation);
    void OnRan99Per(wxCommandEvent& event);
    void OnRan199Per(wxCommandEvent& event);
    void OnRan499Per(wxCommandEvent& event);
    void OnRan999Per(wxCommandEvent& event);
    void OnRanOtherPer(wxCommandEvent& event);
    
    void OnUseSpecifiedSeed(wxCommandEvent& event);
    void OnSpecifySeedDlg(wxCommandEvent& event);
    
    void SetSigFilterX(int filter);
    void OnSigFilter05(wxCommandEvent& event);
    void OnSigFilter01(wxCommandEvent& event);
    void OnSigFilter001(wxCommandEvent& event);
    void OnSigFilter0001(wxCommandEvent& event);
    void OnSigFilterSetup(wxCommandEvent& event);
    
    void OnSaveMLJC(wxCommandEvent& event);
    
    void OnSelectCores(wxCommandEvent& event);
    void OnSelectNeighborsOfCores(wxCommandEvent& event);
    void OnSelectCoresAndNeighbors(wxCommandEvent& event);

    void OnShowAsConditionalMap(wxCommandEvent& event);
        
    LocalMatchCoordinator* GetLocalMatchCoordinator() { return gs_coord; }
    
protected:
    void CoreSelectHelper(const std::vector<bool>& elem);
    LocalMatchCoordinator* gs_coord;
    
    DECLARE_EVENT_TABLE()
};



#endif
