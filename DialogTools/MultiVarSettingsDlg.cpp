
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
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/panel.h>
#include <wx/tokenzr.h>
#include <wx/choice.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../Algorithms/pca.h"
#include "../logger.h"
#include "../FramesManager.h"
#include "../FramesManagerObserver.h"
#include "SaveToTableDlg.h"
#include "MultiVarSettingsDlg.h"

////////////////////////////////////////////////////////////////////////////
//
// MultiVariableSettingsDlg
//
////////////////////////////////////////////////////////////////////////////

MultiVariableSettingsDlg::MultiVariableSettingsDlg(Project* project_s)
    : wxDialog(NULL, wxID_ANY, _("Multi-Variable Settings"), wxDefaultPosition,
               wxSize(320, 430))
{
    wxLogMessage("Entering MultiVariableSettingsDlg::MultiVariableSettingsDlg().");
        
    project = project_s;
    
    has_time = project->GetTimeState()->GetTimeSteps() > 1 ;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
    wxLogMessage("Exiting MultiVariableSettingsDlg::MultiVariableSettingsDlg().");
}

MultiVariableSettingsDlg::~MultiVariableSettingsDlg()
{
    wxLogMessage("In ~MultiVariableSettingsDlg.");
}

bool MultiVariableSettingsDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void MultiVariableSettingsDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

    // label & listbox
    wxStaticText* st = new wxStaticText (panel, wxID_ANY,
                                        _("Select Variables (Multi-Selection)"),
                                        wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(320,200), 0, NULL,
                                wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    
    // weights
    wxStaticText  *st3 = new wxStaticText(panel, wxID_ANY, _("Weights:"),
                                wxDefaultPosition, wxSize(60,-1));
    wxChoice* box3 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                wxSize(160,-1), 0, NULL);
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);

    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("OK"), wxDefaultPosition,
                                wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"),
                                wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    vbox->Add(st, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    vbox->Add(hbox1, 1, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);
    

    panel->SetSizer(vbox);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    InitWeightsCombobox(box3);
    
    combo_var = box;
    combo_weights = box3;
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MultiVariableSettingsDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MultiVariableSettingsDlg::OnClose, this);
}

void MultiVariableSettingsDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("In MultiVariableSettingsDlg::InitVariableCombobox().");
    var_box->Clear();
    var_items.Clear();

	std::vector<int> col_id_map;
	table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = name;
                name_to_tm_id[nm] = t;
                var_items.Add(nm);
            }
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            var_items.Add(name);
        }
    }
    if (!var_items.IsEmpty()) {
        var_box->InsertItems(var_items,0);
    }
}

void MultiVariableSettingsDlg::InitWeightsCombobox(wxChoice* weights_ch)
{
    wxLogMessage("In MultiVariableSettingsDlg::InitWeightsCombobox().");
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);

    size_t sel_pos=0;
    for (size_t i=0; i<weights_ids.size(); ++i) {
        weights_ch->Append(w_man_int->GetShortDispName(weights_ids[i]));
        if (w_man_int->GetDefault() == weights_ids[i])
            sel_pos = i;
    }
    if (weights_ids.size() > 0) weights_ch->SetSelection(sel_pos);
}

void MultiVariableSettingsDlg::OnClose(wxCommandEvent& event )
{
    wxLogMessage("In MultiVariableSettingsDlg::OnClose");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void MultiVariableSettingsDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Entering MultiVariableSettingsDlg::OnOK");
  
    wxArrayInt selections;
    combo_var->GetSelections(selections);
    
    int num_var = selections.size();
    if (num_var < 2) {
        // show message box
        wxString err_msg = _("Please select at least 2 variables.");
        wxMessageDialog dlg(NULL, err_msg, _("Info"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    for (int i=0; i<num_var; i++) {
        int idx = selections[i];
        wxString sel_str = combo_var->GetString(idx);
        //select_vars.push_back(sel_str);
        wxString nm = name_to_nm[sel_str];
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, _("Error"),
                                wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        //col_names.push_back(sel_str);
        col_ids[i] = col;
        var_info[i].time = tm;
        // Set Primary GdaVarTools::VarInfo attributes
        var_info[i].name = nm;
        var_info[i].is_time_variant = table_int->IsColTimeVariant(nm);
        // var_info[i].time already set above
        table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = var_info[i].is_time_variant;
        var_info[i].fixed_scale = true;
    }
    
    // Call function to set all Secondary Attributes based on Primary Attributes
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    EndDialog(wxID_OK);

    wxLogMessage("Exiting MultiVariableSettingsDlg::OnOK");
}

boost::uuids::uuid MultiVariableSettingsDlg::GetWeightsId()
{
    wxLogMessage("In MultiVariableSettingsDlg::GetWeightsId()");
    int sel = combo_weights->GetSelection();
    if (sel < 0) sel = 0;
    if (sel >= weights_ids.size()) sel = weights_ids.size()-1;

    return weights_ids[sel];
}
