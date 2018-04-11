
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
#include "VariableSettingsDlg.h"

////////////////////////////////////////////////////////////////////////////
//
//
// Belows are codes for DiffVarSettingDlg
//
////////////////////////////////////////////////////////////////////////////
DiffMoranVarSettingDlg::DiffMoranVarSettingDlg(Project* project_s)
    : wxDialog(NULL, -1, _("Differential Moran Variable Settings"), wxDefaultPosition, wxSize(590, 230))
{
    wxLogMessage("Open DiffMoranVarSettingDlg.");
    
    project = project_s;
    
    bool init_success = Init();
    
    if (init_success == false) {
        EndDialog(wxID_CANCEL);
    } else {
        CreateControls();
    }
}

DiffMoranVarSettingDlg::~DiffMoranVarSettingDlg()
{
}

bool DiffMoranVarSettingDlg::Init()
{
    if (project == NULL)
        return false;
    
    table_int = project->GetTableInt();
    if (table_int == NULL)
        return false;
    
    
    table_int->GetTimeStrings(tm_strs);
    
    return true;
}

void DiffMoranVarSettingDlg::CreateControls()
{
    wxPanel *panel = new wxPanel(this);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
     wxBoxSizer *vbox1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    
    wxSize var_size(100, -1);
    wxSize  time_size(100,-1);
    
    wxStaticText  *st = new wxStaticText (panel, wxID_ANY, _("Select variable "),
                                          wxDefaultPosition, wxDefaultSize);
    
    wxComboBox* box = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                     var_size, 0, NULL, wxCB_READONLY);
    
    wxStaticText  *st1 = new wxStaticText (panel, wxID_ANY,
                                           _(" and two time periods: "),
                                           wxDefaultPosition, wxDefaultSize);
    
    wxComboBox* box1 = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                      time_size, 0, NULL, wxCB_READONLY);
    
    wxStaticText  *st2 = new wxStaticText (panel, wxID_ANY, _(" and "),
                                           wxDefaultPosition, wxDefaultSize);
    
    wxComboBox* box2 = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                      time_size, 0, NULL, wxCB_READONLY);
    
    hbox->Add(st, 1, wxALIGN_CENTER | wxLEFT| wxTOP | wxBOTTOM, 10);
    hbox->Add(box, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    hbox->Add(st1, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    hbox->Add(box1, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    hbox->Add(st2, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    hbox->Add(box2, 1, wxALIGN_CENTER | wxTOP | wxBOTTOM |wxRIGHT, 10);
    
    
    wxStaticText  *st3 = new wxStaticText (panel, wxID_ANY, wxT("Weights"),
                                           wxDefaultPosition, wxSize(70,-1));
    
    wxComboBox* box3 = new wxComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition,
                                      wxSize(160,-1), 0, NULL, wxCB_READONLY);
    
    hbox1->Add(st3, 0, wxALIGN_CENTER | wxLEFT| wxTOP | wxBOTTOM, 10);
    hbox1->Add(box3, 0, wxALIGN_LEFT | wxTOP | wxBOTTOM, 10);
    
    vbox->Add(hbox, 1);
    vbox->Add(hbox1, 1, wxALIGN_LEFT | wxTOP , 30);
    
    panel->SetSizer(vbox);
    
    wxButton *okButton = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    
    hbox2->Add(okButton, 1);
    hbox2->Add(closeButton, 1, wxLEFT, 5);
    
    vbox1->Add(panel, 1);
    vbox1->Add(hbox2, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    
    SetSizer(vbox1);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    InitTimeComboboxes(box1, box2);
    InitWeightsCombobox(box3);
    
    combo_var = box;
    combo_time1 = box1;
    combo_time2 = box2;
    combo_weights = box3;
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &DiffMoranVarSettingDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &DiffMoranVarSettingDlg::OnClose, this);
}

void DiffMoranVarSettingDlg::InitVariableCombobox(wxComboBox* var_box)
{
    std::vector<wxString> grp_names = table_int->GetGroupNames();
    for (size_t i=0, n=grp_names.size(); i < n; i++ ) {
        var_box->Append(grp_names[i]);
    }
    var_box->SetSelection(0);
}

void DiffMoranVarSettingDlg::InitTimeComboboxes(wxComboBox* time1, wxComboBox* time2)
{
    for (size_t i=0, n=tm_strs.size(); i < n; i++ ) {
        time1->Append(tm_strs[i]);
        time2->Append(tm_strs[i]);
    }
    time1->SetSelection(1);
    time2->SetSelection(0);

}

void DiffMoranVarSettingDlg::InitWeightsCombobox(wxComboBox* weights_ch)
{
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

void DiffMoranVarSettingDlg::OnClose(wxCommandEvent& event )
{
    wxLogMessage("Close DiffMoranVarSettingDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void DiffMoranVarSettingDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click DiffMoranVarSettingDlg::OnOK");
    
    wxString col_name = combo_var->GetStringSelection();
    if (col_name.IsEmpty()) {
        wxMessageDialog dlg (this,
                             "Please select a variable first.",
                             _("Warning"),
                             wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    int time1 = combo_time1->GetSelection();
    int time2 = combo_time2->GetSelection();
    if (time1 < 0 || time2 < 0 || time1 == time2) {
        wxMessageDialog dlg (this,
                             "Please choose two different time periods.",
                             _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }
    
    int num_var = 2;
    
    col_ids.resize(num_var);
    var_info.resize(num_var);
    
    int col_idx = table_int->FindColId(col_name);
    
    col_ids[0] = col_idx;
    col_ids[1] = col_idx;
    
    for (int i=0; i<num_var; i++) {
        var_info[i].name = col_name;
        var_info[i].is_time_variant = true;
        
        table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = false;
        var_info[i].fixed_scale = true;
    }
    var_info[0].time = time1;
    var_info[1].time = time2;

    // Call function to set all Secondary Attributes based on Primary Attributes
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
    
    bool check_group_var = true;
    try {
        for (int i=0; i<col_ids.size(); i++) {
            project->GetTableInt()->GetColTypes(col_ids[i]);
        }
    } catch(GdaException& ex) {
        // place holder found
        wxString str_tmplt = _T("The selected group variable should contains %d items. Please modify the group variable in Time Editor, or select another variable.");
        wxString msg = wxString::Format(str_tmplt, project->GetTableInt()->GetTimeSteps());
        wxMessageDialog dlg (this, msg.mb_str(), "Incomplete Group Variable ", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        check_group_var = false;
    }
    
    if (check_group_var == true)
        EndDialog(wxID_OK);
    
}

boost::uuids::uuid DiffMoranVarSettingDlg::GetWeightsId()
{
   
    int sel = combo_weights->GetSelection();
    if (sel < 0) sel = 0;
    if (sel >= weights_ids.size()) sel = weights_ids.size()-1;

    return weights_ids[sel];
}



////////////////////////////////////////////////////////////////////////////
//
// MultiVariableSettingsDlg
//
////////////////////////////////////////////////////////////////////////////

MultiVariableSettingsDlg::MultiVariableSettingsDlg(Project* project_s)
    : wxDialog(NULL, -1, _("Multi-Variable Settings"), wxDefaultPosition, wxSize(320, 430))
{
    wxLogMessage("Entering MultiVariableSettingsDlg::MultiVariableSettingsDlg().");
    
    combo_time1 = NULL;
    
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
    wxStaticText* st = new wxStaticText (panel, wxID_ANY, _("Select Variables (Multi-Selection)"),
                                          wxDefaultPosition, wxDefaultSize);
    
    wxListBox* box = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                                   wxSize(320,200), 0, NULL,
                                   wxLB_MULTIPLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    
    // weights
    wxStaticText  *st3 = new wxStaticText(panel, wxID_ANY, wxT("Weights:"),
                                           wxDefaultPosition, wxSize(60,-1));
    wxChoice* box3 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                      wxSize(160,-1), 0, NULL);
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(st3, 0, wxALIGN_CENTER_VERTICAL);
    hbox1->Add(box3, 1, wxALIGN_CENTER_VERTICAL);

    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("OK"), wxDefaultPosition,
                                      wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    vbox->Add(st, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(box, 1,  wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // time
    if (has_time) {
        wxStaticText* st1 = new wxStaticText(panel, wxID_ANY, _("Time:"),
                                             wxDefaultPosition, wxSize(40,-1));
        wxChoice* box1 = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                      wxSize(160,-1), 0, NULL);
        wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
        hbox->Add(st1, 0, wxRIGHT | wxLEFT, 10);
        hbox->Add(box1, 1, wxEXPAND);
        combo_time1 = box1;
        combo_time1->SetSelection(0);
        vbox->Add(hbox, 1, wxALIGN_CENTER | wxALL, 10);
    }
    vbox->Add(hbox1, 1, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 1, wxALIGN_CENTER | wxALL, 10);
    

    panel->SetSizer(vbox);
    
    Centre();
    
    // Content
    InitVariableCombobox(box);
    if (has_time) {
        InitTimeComboboxes(combo_time1);
    }
    InitWeightsCombobox(box3);
    
    combo_var = box;
    
    combo_weights = box3;
    
    // Events
    okButton->Bind(wxEVT_BUTTON, &MultiVariableSettingsDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MultiVariableSettingsDlg::OnClose, this);
    if (combo_time1) {
        combo_time1->Bind(wxEVT_CHOICE, &MultiVariableSettingsDlg::OnTimeSelect, this);
    }
}

void MultiVariableSettingsDlg::OnTimeSelect( wxCommandEvent& event )
{
    wxLogMessage("In MultiVariableSettingsDlg::OnTimeSelect()");
    combo_var->Clear();
    
    InitVariableCombobox(combo_var);
}

void MultiVariableSettingsDlg::InitVariableCombobox(wxListBox* var_box)
{
    wxLogMessage("In MultiVariableSettingsDlg::InitVariableCombobox().");
    var_box->Clear();
    
    wxArrayString items;
    
	std::vector<int> col_id_map;
	table_int->FillNumericColIdMap(col_id_map);
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            int t = combo_time1->GetSelection();
            if (t< 0) t = 0;
            
            wxString nm = name;
            nm << " (" << project->GetTimeState()->GetTimeString(t) << ")";
            name_to_nm[nm] = name;
            name_to_tm_id[nm] = t;
            items.Add(nm);
        } else {
            name_to_nm[name] = name;
            name_to_tm_id[name] = 0;
            items.Add(name);
        }
    }
   
    if (!items.IsEmpty())
        var_box->InsertItems(items,0);
}

void MultiVariableSettingsDlg::InitTimeComboboxes(wxChoice* time1)
{
    wxLogMessage("In MultiVariableSettingsDlg::InitTimeComboboxes().");
    for (size_t i=0, n=tm_strs.size(); i < n; i++ ) {
        time1->Append(tm_strs[i]);
    }
    time1->SetSelection(0);
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
        wxString list_item = combo_var->GetString(idx);
        wxString nm = name_to_nm[list_item];
        wxLogMessage(nm);
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), nm); wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        }
        
        int tm = name_to_tm_id[combo_var->GetString(idx)];
        
        col_ids[i] = col;
        var_info[i].time = tm;
        
        // Set Primary GdaVarTools::VarInfo attributes
        var_info[i].name = nm;
        var_info[i].is_time_variant = table_int->IsColTimeVariant(col);
        
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


////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////
/**
 * Belows are codes for VariableSettingsDlg
 *
 */

BEGIN_EVENT_TABLE(VariableSettingsDlg, wxDialog)
	EVT_CHOICE(XRCID("ID_TIME1"), VariableSettingsDlg::OnTime1)
	EVT_CHOICE(XRCID("ID_TIME2"), VariableSettingsDlg::OnTime2)
	EVT_CHOICE(XRCID("ID_TIME3"), VariableSettingsDlg::OnTime3)
	EVT_CHOICE(XRCID("ID_TIME4"), VariableSettingsDlg::OnTime4)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE1"),
					   VariableSettingsDlg::OnListVariable1DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE2"),
					   VariableSettingsDlg::OnListVariable2DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE3"),
					   VariableSettingsDlg::OnListVariable3DoubleClicked)
	EVT_LISTBOX_DCLICK(XRCID("ID_VARIABLE4"),
					   VariableSettingsDlg::OnListVariable4DoubleClicked)
	EVT_LISTBOX(XRCID("ID_VARIABLE1"), VariableSettingsDlg::OnVar1Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE2"), VariableSettingsDlg::OnVar2Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE3"), VariableSettingsDlg::OnVar3Change)
	EVT_LISTBOX(XRCID("ID_VARIABLE4"), VariableSettingsDlg::OnVar4Change)
	EVT_SPINCTRL(XRCID("ID_NUM_CATEGORIES_SPIN"),
				 VariableSettingsDlg::OnSpinCtrl)
	EVT_BUTTON(XRCID("wxID_OKBUTTON"), VariableSettingsDlg::OnOkClick)
	EVT_BUTTON(XRCID("wxID_CANCEL"), VariableSettingsDlg::OnCancelClick)
END_EVENT_TABLE()

VariableSettingsDlg::VariableSettingsDlg(Project* project_s,
										 VarType v_type_s,
										 bool show_weights_s,
                                         bool show_distance_s,
										 const wxString& title_s,
										 const wxString& var1_title_s,
										 const wxString& var2_title_s,
										 const wxString& var3_title_s,
										 const wxString& var4_title_s,
										 bool _set_second_from_first_mode,
										 bool _set_fourth_from_third_mode,
                                         bool hide_time,
                                         bool _var1_str,
                                         bool _var2_str,
                                         bool _var3_str,
                                         bool _var4_str)
: project(project_s),
table_int(project_s->GetTableInt()),
show_weights(show_weights_s),
no_weights_found_fail(false),
show_distance(show_distance_s),
is_time(project_s->GetTableInt()->IsTimeVariant()),
time_steps(project_s->GetTableInt()->GetTimeSteps()),
title(title_s),
var1_title(var1_title_s),
var2_title(var2_title_s),
var3_title(var3_title_s),
var4_title(var4_title_s),
set_second_from_first_mode(_set_second_from_first_mode),
set_fourth_from_third_mode(_set_fourth_from_third_mode),
num_cats_spin(0),
num_categories(4),
hide_time(hide_time),
all_init(false),
var1_str(_var1_str),
var2_str(_var2_str),
var3_str(_var3_str),
var4_str(_var4_str)
{
    wxLogMessage("Open VariableSettingsDlg");
   
    default_var_name1 = project->GetDefaultVarName(0);
    default_var_name2 = project->GetDefaultVarName(1);
    default_var_name3 = project->GetDefaultVarName(2);
    default_var_name4 = project->GetDefaultVarName(3);
    
	if (show_weights && project->GetWManInt()->GetIds().size() == 0) {
		no_weights_found_fail = true;
		wxXmlResource::Get()->LoadDialog(this, GetParent(),
										 "ID_VAR_SETTINGS_NO_W_FAIL_DLG");
		SetTitle("No Weights Found");
	} else {
		Init(v_type_s);
	}
	SetParent(0);
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);	
	Centre();
	all_init = true;
}

VariableSettingsDlg::~VariableSettingsDlg()
{
}

void VariableSettingsDlg::Init(VarType var_type)
{
	v_type = var_type;
	if (var_type == univariate) {
		num_var = 1;
	} else if (var_type == bivariate || var_type == rate_smoothed) {
		num_var = 2;
	} else if (var_type == trivariate) {
		num_var = 3;
	} else { // (var_type == quadvariate)
		num_var = 4;
	}
    
	if (num_var > 2) show_weights = false;
	
	int num_obs = project->GetNumRecords();
	m_theme = 0;
	map_theme_ch = 0;
	weights_ch = 0;
	distance_ch = 0;
	lb1 = 0;
	lb2 = 0;
	lb3 = 0;
	lb4 = 0;
	time_lb1 = 0;
	time_lb2 = 0;
	time_lb3 = 0;
	time_lb4 = 0;
	CreateControls();
	v1_time = 0;
	v2_time = 0;
	v3_time = 0;
	v4_time = 0;
	InitTimeChoices();
	lb1_cur_sel = 0;
	lb2_cur_sel = 0;
	lb3_cur_sel = 0;
	lb4_cur_sel = 0;
    
	table_int->FillColIdMap(col_id_map);
	
	if (col_id_map.size() == 0) {
		wxString msg("No numeric variables found.");
		wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
		dlg.ShowModal();
		return;
	}
	
	InitFieldChoices();
	
	if (map_theme_ch) {
		map_theme_ch->Clear();
		map_theme_ch->Append("Quantile Map");
		map_theme_ch->Append("Percentile Map");
		map_theme_ch->Append("Box Map (Hinge=1.5)");
		map_theme_ch->Append("Box Map (Hinge=3.0)");
		map_theme_ch->Append("Standard Deviation Map");
		map_theme_ch->Append("Natural Breaks");
		map_theme_ch->Append("Equal Intervals");
		map_theme_ch->SetSelection(0);
	}
}

void VariableSettingsDlg::CreateControls()
{
    wxString ctrl_xrcid;
    
	// show_distance is only supported for univariate
    if (num_var == 1 && is_time && show_distance) {
        ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_1_DIST";
        
    } else if (num_var == 1 && !is_time && show_distance) {
        ctrl_xrcid = "ID_VAR_SETTINGS_DLG_1_DIST";
        
    } else if (num_var == 1 && is_time && !show_weights) {
        ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_1";
        
    } else if (num_var == 1 && is_time && show_weights) {
        ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_1_W";
        
    } else if (num_var == 1 && !is_time && !show_weights) {
        ctrl_xrcid = "ID_VAR_SETTINGS_DLG_1";
        
    } else if (num_var == 1 && !is_time && show_weights) {
        ctrl_xrcid = "ID_VAR_SETTINGS_DLG_1_W";
        
    } else if (num_var == 2 && is_time && !show_weights) {
        if (v_type == rate_smoothed) {
            ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_RATE";
            
        } else {
            ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_2";
            
        }
    } else if (num_var == 2 && is_time && show_weights) {
        if (v_type == rate_smoothed) {
            ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_RATE_W";
            
        } else {
            ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_2_W";
            
        }
    } else if (num_var == 2 && !is_time && !show_weights) {
        if (v_type == rate_smoothed) {
            ctrl_xrcid = "ID_VAR_SETTINGS_DLG_RATE";
            
        } else {
            ctrl_xrcid = "ID_VAR_SETTINGS_DLG_2";
            
        }
    } else if (num_var == 2 && !is_time && show_weights) {
        if (v_type == rate_smoothed) {
            ctrl_xrcid = "ID_VAR_SETTINGS_DLG_RATE_W";
            
        } else {
            ctrl_xrcid = "ID_VAR_SETTINGS_DLG_2_W";
            
        }
    } else if (num_var == 3 && is_time) {
        ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_3";
        
    } else if (num_var == 3 && !is_time) {
        ctrl_xrcid = "ID_VAR_SETTINGS_DLG_3";
        
    } else if (num_var == 4 && is_time) {
        ctrl_xrcid = "ID_VAR_SETTINGS_TIME_DLG_4";
        
    } else if (num_var == 4 && !is_time) {
        ctrl_xrcid = "ID_VAR_SETTINGS_DLG_4";
        
    }
    wxXmlResource::Get()->LoadDialog(this, GetParent(), ctrl_xrcid);
    
	if (is_time) {
        if (hide_time) {
            wxStaticText* time_txt = XRCCTRL(*this, "ID_VARSEL_TIME", wxStaticText);
            time_txt->Hide();
        }
		time_lb1 = XRCCTRL(*this, "ID_TIME1", wxChoice);
        if (hide_time) time_lb1->Hide();
		if (num_var >= 2) {
			time_lb2 = XRCCTRL(*this, "ID_TIME2", wxChoice);
            if (hide_time) time_lb2->Hide();
		}
		if (num_var >= 3) {
			time_lb3 = XRCCTRL(*this, "ID_TIME3", wxChoice);
            if (hide_time) time_lb3->Hide();
		}
		if (num_var >= 4) {
			time_lb4 = XRCCTRL(*this, "ID_TIME4", wxChoice);
            if (hide_time) time_lb4->Hide();
		}
	}
	if (show_weights) {
		weights_ch = XRCCTRL(*this, "ID_WEIGHTS", wxChoice);
		WeightsManInterface* w_man_int = project->GetWManInt();
		w_man_int->GetIds(weights_ids);
		size_t sel_pos=0;
		for (size_t i=0; i<weights_ids.size(); ++i) {
			weights_ch->Append(w_man_int->GetShortDispName(weights_ids[i]));
			if (w_man_int->GetDefault() == weights_ids[i])
                sel_pos = i;
		}
		if (weights_ids.size() > 0)
            weights_ch->SetSelection(sel_pos);
	}
	if (show_distance && v_type == univariate) {
		distance_ch = XRCCTRL(*this, "ID_DISTANCE_METRIC", wxChoice);
		distance_ch->Append("Euclidean Distance");
		distance_ch->Append("Arc Distance (mi)");
		distance_ch->Append("Arc Distance (km)");
		if (project->GetDefaultDistMetric() == WeightsMetaInfo::DM_euclidean) {
			distance_ch->SetSelection(0);
		} else if (project->GetDefaultDistMetric() == WeightsMetaInfo::DM_arc) {
			if (project->GetDefaultDistUnits() == WeightsMetaInfo::DU_km) {
				distance_ch->SetSelection(2);
			} else {
				distance_ch->SetSelection(1);
			}
		} else {
			distance_ch->SetSelection(0);
		}
	}
	SetTitle(title);
	wxStaticText* st;
	if (FindWindow(XRCID("ID_VAR1_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR1_NAME", wxStaticText);
		st->SetLabelText(var1_title);
	}
	if (FindWindow(XRCID("ID_VAR2_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR2_NAME", wxStaticText);
		st->SetLabelText(var2_title);
	}
	if (FindWindow(XRCID("ID_VAR3_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR3_NAME", wxStaticText);
		st->SetLabelText(var3_title);
	}
	if (FindWindow(XRCID("ID_VAR4_NAME"))) {
        st = XRCCTRL(*this, "ID_VAR4_NAME", wxStaticText);
		st->SetLabelText(var4_title);
	}
	lb1 = XRCCTRL(*this, "ID_VARIABLE1", wxListBox);
	if (num_var >= 2) lb2 = XRCCTRL(*this, "ID_VARIABLE2", wxListBox);
	if (num_var >= 3) lb3 = XRCCTRL(*this, "ID_VARIABLE3", wxListBox);
	if (num_var >= 4) lb4 = XRCCTRL(*this, "ID_VARIABLE4", wxListBox);
	
	if (FindWindow(XRCID("ID_THEMATIC"))) {
        map_theme_ch = XRCCTRL(*this, "ID_THEMATIC", wxChoice);
        map_theme_ch->Bind(wxEVT_CHOICE, &VariableSettingsDlg::OnMapThemeChange, this);
	}
	if (FindWindow(XRCID("ID_NUM_CATEGORIES_SPIN"))) {
        num_cats_spin = XRCCTRL(*this, "ID_NUM_CATEGORIES_SPIN", wxSpinCtrl);
		num_categories = num_cats_spin->GetValue();
	}
}

void VariableSettingsDlg::OnMapThemeChange(wxCommandEvent& event)
{
    if (map_theme_ch) {
        int m_theme = map_theme_ch->GetSelection();
        //        map_theme_ch->Append("Percentile Map");
        //        map_theme_ch->Append("Box Map (Hinge=1.5)");
        //        map_theme_ch->Append("Box Map (Hinge=3.0)");
        //        map_theme_ch->Append("Standard Deviation Map");
        if (m_theme == 1 || m_theme == 2 ||
            m_theme == 3 || m_theme == 4 )
        {
            num_cats_spin->Disable();
        } else {
            num_cats_spin->Enable();
        }
    }
}

void VariableSettingsDlg::OnListVariable1DoubleClicked(wxCommandEvent& event)
{
	if (!all_init)
        return;
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		if (is_time) {
			time_lb2->SetSelection(v1_time);
			v2_time = v1_time;
		}
	}
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable2DoubleClicked(wxCommandEvent& event)
{
	if (!all_init)
        return;
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable3DoubleClicked(wxCommandEvent& event)
{
	if (!all_init)
        return;
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		if (is_time) {
			time_lb4->SetSelection(v3_time);
			v4_time = v3_time;
		}
	}
	OnOkClick(event);
}

void VariableSettingsDlg::OnListVariable4DoubleClicked(wxCommandEvent& event)
{
	if (!all_init)
        return;
	OnOkClick(event);
}

void VariableSettingsDlg::OnTime1(wxCommandEvent& event)
{
	if (!all_init)
        return;
	v1_time = time_lb1->GetSelection();
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		time_lb2->SetSelection(v1_time);
		v2_time = v1_time;
	}
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime2(wxCommandEvent& event)
{
	if (!all_init)
        return;
	v2_time = time_lb2->GetSelection();
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime3(wxCommandEvent& event)
{
	if (!all_init)
        return;
	v3_time = time_lb3->GetSelection();
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		time_lb4->SetSelection(v3_time);
		v4_time = v3_time;
	}
	InitFieldChoices();
}

void VariableSettingsDlg::OnTime4(wxCommandEvent& event)
{
	if (!all_init)
        return;
	v4_time = time_lb4->GetSelection();
	InitFieldChoices();
}

void VariableSettingsDlg::OnVar1Change(wxCommandEvent& event)
{
	if (!all_init)
        return;
	lb1_cur_sel = lb1->GetSelection();
    if (lb1_cur_sel >= 0) {
        int x_pos = sel1_idx_map[lb1_cur_sel];
        if (x_pos >= 0)
            default_var_name1 = table_int->GetColName(col_id_map[x_pos]);
    }
	if (num_var >= 2 && set_second_from_first_mode) {
		lb2->SetSelection(lb1_cur_sel);
		lb2_cur_sel = lb1_cur_sel;
		if (is_time) {
			time_lb2->SetSelection(v1_time);
			v2_time = v1_time;
		}
	}
}

void VariableSettingsDlg::OnVar2Change(wxCommandEvent& event)
{
	if (!all_init)
        return;
	lb2_cur_sel = lb2->GetSelection();
    if (lb2_cur_sel >= 0) {
        int x_pos = sel2_idx_map[lb2_cur_sel];
        if (x_pos >= 0)
            default_var_name2 = table_int->GetColName(col_id_map[x_pos]);
    }
}

void VariableSettingsDlg::OnVar3Change(wxCommandEvent& event)
{
	if (!all_init)
        return;
	lb3_cur_sel = lb3->GetSelection();
    if (lb3_cur_sel >= 0) {
        int x_pos = sel3_idx_map[lb3_cur_sel];
        if (x_pos >= 0)
            default_var_name3 = table_int->GetColName(col_id_map[x_pos]);
    }
    
	if (num_var >= 4 && set_fourth_from_third_mode) {
		lb4->SetSelection(lb3_cur_sel);
		lb4_cur_sel = lb3_cur_sel;
		if (is_time) {
			time_lb4->SetSelection(v3_time);
			v4_time = v3_time;
		}
	}
}

void VariableSettingsDlg::OnVar4Change(wxCommandEvent& event)
{
	if (!all_init)
        return;
	lb4_cur_sel = lb4->GetSelection();
    if (lb4_cur_sel >= 0) {
        int x_pos = sel4_idx_map[lb4_cur_sel];
        if (x_pos >= 0)
            default_var_name4 = table_int->GetColName(col_id_map[x_pos]);
    }
}

void VariableSettingsDlg::OnSpinCtrl( wxSpinEvent& event )
{
	if (!num_cats_spin)
        return;
	num_categories = num_cats_spin->GetValue();
	if (num_categories < num_cats_spin->GetMin()) {
		num_categories = num_cats_spin->GetMin();
	}
	if (num_categories > num_cats_spin->GetMax()) {
		num_categories = num_cats_spin->GetMax();
	}
}

void VariableSettingsDlg::OnCancelClick(wxCommandEvent& event)
{
    wxLogMessage("Click VariableSettingsDlg::OnOkClick");
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void VariableSettingsDlg::OnOkClick(wxCommandEvent& event)
{
    wxLogMessage("Click VariableSettingsDlg::OnOkClick:");
	if (no_weights_found_fail) {
		event.Skip();
		EndDialog(wxID_CANCEL);
		return;
	}
	
    if (map_theme_ch) {
        m_theme = map_theme_ch->GetSelection();
    }
	
	if (lb1->GetSelection() == wxNOT_FOUND) {
		wxString msg(_T("No field chosen for first variable."));
		wxMessageDialog dlg (this, msg, _T("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
    
	v1_col_id = col_id_map[sel1_idx_map[lb1->GetSelection()]];
    
	v1_name = table_int->GetColName(v1_col_id);
	project->SetDefaultVarName(0, v1_name);
	if (is_time) {
		v1_time = time_lb1->GetSelection();
		project->SetDefaultVarTime(0, v1_time);
		if (!table_int->IsColTimeVariant(v1_col_id))
            v1_time = 0;
	}
    wxLogMessage(v1_name);
    
	if (num_var >= 2) {
		if (lb2->GetSelection() == wxNOT_FOUND) {
			wxString msg(_T("No field chosen for second variable."));
			wxMessageDialog dlg (this, msg, _T("Error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v2_col_id = col_id_map[sel2_idx_map[lb2->GetSelection()]];
		v2_name = table_int->GetColName(v2_col_id);
		project->SetDefaultVarName(1, v2_name);
		if (is_time) {
			v2_time = time_lb2->GetSelection();
			project->SetDefaultVarTime(1, v2_time);
			if (!table_int->IsColTimeVariant(v2_col_id))
                v2_time = 0;
		}
        wxLogMessage(v2_name);
	}
	if (num_var >= 3) {
		if (lb3->GetSelection() == wxNOT_FOUND) {
			wxString msg(_T("No field chosen for third variable."));
			wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v3_col_id = col_id_map[sel3_idx_map[lb3->GetSelection()]];
		v3_name = table_int->GetColName(v3_col_id);
		project->SetDefaultVarName(2, v3_name);
		if (is_time) {
			v3_time = time_lb3->GetSelection();
			project->SetDefaultVarTime(2, v3_time);
			if (!table_int->IsColTimeVariant(v3_col_id))
                v3_time = 0;
		}
        wxLogMessage(v3_name);
	}
	if (num_var >= 4) {
		if (lb4->GetSelection() == wxNOT_FOUND) {
			wxString msg(_T("No field chosen for fourth variable."));
			wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		v4_col_id = col_id_map[sel4_idx_map[lb4->GetSelection()]];
		v4_name = table_int->GetColName(v4_col_id);
		project->SetDefaultVarName(3, v4_name);
		if (is_time) {
			v4_time = time_lb4->GetSelection();
			project->SetDefaultVarTime(3, v4_time);
			if (!table_int->IsColTimeVariant(v4_col_id))
                v4_time = 0;
		}
        wxLogMessage(v4_name);
	}
	
    wxString emptyVar = FillData();
    if (emptyVar.empty()== false) {
        wxString msg = wxString::Format(_("The selected variable %s is not valid. If it's a grouped variable, please modify it in Time->Time Editor. Or please select another variable."), emptyVar);
        wxMessageDialog dlg (this, msg.mb_str(), _("Invalid Variable"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        
    } else {
	
    	if (show_weights)
            project->GetWManInt()->MakeDefault(GetWeightsId());
    	
    	if (GetDistanceMetric() != WeightsMetaInfo::DM_unspecified) {
    		project->SetDefaultDistMetric(GetDistanceMetric());
    	}
    	if (GetDistanceUnits() != WeightsMetaInfo::DU_unspecified) {
    		project->SetDefaultDistUnits(GetDistanceUnits());
    	}
    	
        bool check_group_var = true;
        try {
            for (int i=0; i<col_ids.size(); i++) {
                project->GetTableInt()->GetColTypes(col_ids[i]);
            }
        } catch(GdaException& ex) {
            // place holder found
            wxString msg = wxString::Format(_T("The selected group variable should contains %d items. Please modify the group variable in Time->Time Editor, or select another variable."), project->GetTableInt()->GetTimeSteps());
            wxMessageDialog dlg (this, msg.mb_str(), _T("Incomplete Group Variable"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            check_group_var = false;
        }

        if (check_group_var == true)
            EndDialog(wxID_OK);
    }
}

// Theme choice for Rate Smoothed variable settings
CatClassification::CatClassifType VariableSettingsDlg::GetCatClassifType()
{
	if (no_weights_found_fail) return CatClassification::quantile; 
	if (m_theme == 0) return CatClassification::quantile;
	if (m_theme == 1) return CatClassification::percentile;
	if (m_theme == 2) return CatClassification::hinge_15;
	if (m_theme == 3) return CatClassification::hinge_30;
	if (m_theme == 4) return CatClassification::stddev;
	if (m_theme == 5) return CatClassification::natural_breaks;
	if (m_theme == 6) return CatClassification::equal_intervals;
	return CatClassification::quantile; 
}

// Number of categories for Rate Smoothed variable settings
int VariableSettingsDlg::GetNumCategories()
{
	if (no_weights_found_fail) return 6;
	CatClassification::CatClassifType cc_type = GetCatClassifType();
	if (cc_type == CatClassification::quantile ||
		cc_type == CatClassification::natural_breaks ||
		cc_type == CatClassification::equal_intervals) {
		return num_categories;
	} else {
		return 6;
	}
}

boost::uuids::uuid VariableSettingsDlg::GetWeightsId()
{
	if (no_weights_found_fail || !show_weights ||
		!weights_ch || weights_ids.size()==0) return boost::uuids::nil_uuid();
	
	int sel = weights_ch->GetSelection();
	if (sel < 0) sel = 0;
	if (sel >= weights_ids.size()) sel = weights_ids.size()-1;
    
	wxString s;
	s << "VariableSettingsDlg::GetWeightsId() weight: ";
	s << project->GetWManInt()->GetShortDispName(weights_ids[sel]);
    wxLogMessage(s);
    
	return weights_ids[sel];
}

WeightsMetaInfo::DistanceMetricEnum VariableSettingsDlg::GetDistanceMetric()
{
	if (distance_ch) {
		if (distance_ch->GetSelection() == 0) {
			return WeightsMetaInfo::DM_euclidean;
		} else if (distance_ch->GetSelection() == 1 ||
							 distance_ch->GetSelection() == 2) {
			return WeightsMetaInfo::DM_arc;
		}
	}
	return WeightsMetaInfo::DM_unspecified;
}

WeightsMetaInfo::DistanceUnitsEnum VariableSettingsDlg::GetDistanceUnits()
{
	if (distance_ch) {
		if (distance_ch->GetSelection() == 1) {
			return WeightsMetaInfo::DU_mile;
		} else if (distance_ch->GetSelection() == 2) {
			return WeightsMetaInfo::DU_km;
		}
	}
	return WeightsMetaInfo::DU_unspecified;
}

void VariableSettingsDlg::InitTimeChoices()
{
	if (!is_time) return;
	for (int i=0; i<time_steps; i++) {
		wxString s;
		s << table_int->GetTimeString(i);
		time_lb1->Append(s);
		if (num_var >= 2) time_lb2->Append(s);
		if (num_var >= 3) time_lb3->Append(s);
		if (num_var >= 4) time_lb4->Append(s);
	}
	v1_time = project->GetDefaultVarTime(0);
	time_lb1->SetSelection(v1_time);
	if (num_var >= 2) {
		v2_time = project->GetDefaultVarTime(1);
		time_lb2->SetSelection(v2_time);
	}
	if (num_var >= 3) {
		v3_time = project->GetDefaultVarTime(2);
		time_lb3->SetSelection(v3_time);
	}
	if (num_var >= 4) {
		v4_time = project->GetDefaultVarTime(3);
		time_lb4->SetSelection(v4_time);
	}
}

void VariableSettingsDlg::InitFieldChoices()
{
	wxString t1;
	wxString t2;
	wxString t3;
	wxString t4;
	if (is_time) {
        if (!hide_time) {
    		t1 << " (" << table_int->GetTimeString(v1_time) << ")";
    		t2 << " (" << table_int->GetTimeString(v2_time) << ")";
    		t3 << " (" << table_int->GetTimeString(v3_time) << ")";
    		t4 << " (" << table_int->GetTimeString(v4_time) << ")";
        } else {
            wxString first_time = table_int->GetTimeString(0);
            wxString second_time = table_int->GetTimeString(time_steps-1);
            
            t1 << " (" << first_time << "-" << second_time << ")";
            t2 << " (" << first_time << "-" << second_time << ")";
            t3 << " (" << first_time << "-" << second_time << ")";
            t4 << " (" << first_time << "-" << second_time << ")";
        }
	}
	
	lb1->Clear();
	if (num_var >= 2) lb2->Clear();
	if (num_var >= 3) lb3->Clear();
	if (num_var >= 4) lb4->Clear();

    sel1_idx_map.clear();
    sel2_idx_map.clear();
    sel3_idx_map.clear();
    sel4_idx_map.clear();
    idx_sel1_map.clear();
    idx_sel2_map.clear();
    idx_sel3_map.clear();
    idx_sel4_map.clear();
    
    int sel1_idx = 0;
    int sel2_idx = 0;
    int sel3_idx = 0;
    int sel4_idx = 0;
    
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        GdaConst::FieldType ftype = table_int->GetColType(col_id_map[i]);
		wxString name = table_int->GetColName(col_id_map[i]);
        
		if (table_int->IsColTimeVariant(col_id_map[i]))
            name << t1;
        
        if ((var1_str) ||
            (!var1_str && ftype == GdaConst::double_type) ||
            (!var1_str && ftype == GdaConst::long64_type))
        {
            lb1->Append(name);
            sel1_idx_map[sel1_idx] = i;
            idx_sel1_map[i] = sel1_idx;
            sel1_idx += 1;
        }
        
		if (num_var >= 2) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i]))
                name << t2;
            if ((var2_str) ||
                (!var2_str && ftype == GdaConst::double_type) ||
                (!var2_str && ftype == GdaConst::long64_type))
            {
                lb2->Append(name);
                sel2_idx_map[sel2_idx] = i;
                idx_sel2_map[i] = sel2_idx;
                sel2_idx += 1;
            }
		}
        
		if (num_var >= 3) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i]))
                name << t3;
            if ((var3_str) ||
                (!var3_str && ftype == GdaConst::double_type) ||
                (!var3_str && ftype == GdaConst::long64_type))
            {
                lb3->Append(name);
                sel3_idx_map[sel3_idx] = i;
                idx_sel3_map[i] = sel3_idx;
                sel3_idx += 1;
            }
		}
        
		if (num_var >= 4) {
			wxString name = table_int->GetColName(col_id_map[i]);
			if (table_int->IsColTimeVariant(col_id_map[i]))
                name << t4;
            if ((var4_str) ||
                (!var4_str && ftype == GdaConst::double_type) ||
                (!var4_str && ftype == GdaConst::long64_type))
            {
                lb4->Append(name);
                sel4_idx_map[sel4_idx] = i;
                idx_sel4_map[i] = sel4_idx;
                sel4_idx += 1;
            }
		}
        
	}
    
    for (int i=0, iend=col_id_map.size(); i<iend; i++) {
        wxString item_str = table_int->GetColName(col_id_map[i]);
        if (item_str == default_var_name1) {
            lb1_cur_sel = idx_sel1_map[i];
            if (set_second_from_first_mode && num_var >= 2) {
                lb2_cur_sel = idx_sel1_map[i];
            }
        }
        if (num_var >= 2 && item_str == default_var_name2) {
            if (!set_second_from_first_mode) {
                lb2_cur_sel = idx_sel2_map[i];
            }
        }
        if (num_var >= 3 && item_str == default_var_name3){
            lb3_cur_sel = idx_sel3_map[i];
            if (set_fourth_from_third_mode && num_var >= 4) {
                lb4_cur_sel = idx_sel3_map[i];
            }
        }
        if (num_var >= 4 && item_str == default_var_name4) {
            if (!set_fourth_from_third_mode) {
                lb4_cur_sel = idx_sel4_map[i];
            }
        }
    }
  
    if (sel1_idx > 0) {
    	int pos = lb1->GetScrollPos(wxVERTICAL);
    	lb1->SetSelection(lb1_cur_sel);
    	lb1->SetFirstItem(lb1->GetSelection());
    }
	if (sel2_idx > 0 && num_var >= 2) {
		lb2->SetSelection(lb2_cur_sel);
		lb2->SetFirstItem(lb2->GetSelection());
	}
	if (sel3_idx > 0 &&  num_var >= 3) {
		lb3->SetSelection(lb3_cur_sel);
		lb3->SetFirstItem(lb3->GetSelection());
	}
	if (sel4_idx > 0 && num_var >= 4) {
		lb4->SetSelection(lb4_cur_sel);
		lb4->SetFirstItem(lb4->GetSelection());
	}
    
    if (sel1_idx == 0 && sel2_idx == 0 && sel3_idx == 0 && sel4_idx == 0) {
        wxString msg("No numeric variables found.");
        wxMessageDialog dlg (this, msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
    }
}

bool VariableSettingsDlg::CheckEmptyColumn(int col_id, int time)
{
    std::vector<bool> undefs;
    table_int->GetColUndefined(col_id, time, undefs);
    for (int i=0; i<undefs.size(); i++) {
        if (undefs[i]==false)
            return false;
    }
    return true;
}

wxString VariableSettingsDlg::FillData()
{
    wxString emptyVar;
    
	col_ids.resize(num_var);
	var_info.resize(num_var);
	if (num_var >= 1) {
        int sel_idx = lb1->GetSelection();
        int col_idx = sel1_idx_map[sel_idx];
		v1_col_id = col_id_map[col_idx];
		v1_name = table_int->GetColName(v1_col_id);
		col_ids[0] = v1_col_id;
		var_info[0].time = v1_time;
        
        if (CheckEmptyColumn(v1_col_id, v1_time)) {
            emptyVar =  v1_name;
        }
	}
	if (num_var >= 2) {
		//v2_col_id = col_id_map[lb2->GetSelection()];
        int sel_idx = lb2->GetSelection();
        int col_idx = sel2_idx_map[sel_idx];
        v2_col_id = col_id_map[col_idx];
		v2_name = table_int->GetColName(v2_col_id);
		col_ids[1] = v2_col_id;
		var_info[1].time = v2_time;
        
        if (emptyVar.empty() && CheckEmptyColumn(v2_col_id, v2_time)) {
            emptyVar =  v2_name;
        }
	}
	if (num_var >= 3) {
		//v3_col_id = col_id_map[lb3->GetSelection()];
        int sel_idx = lb3->GetSelection();
        int col_idx = sel3_idx_map[sel_idx];
        v3_col_id = col_id_map[col_idx];
		v3_name = table_int->GetColName(v3_col_id);
		col_ids[2] = v3_col_id;
		var_info[2].time = v3_time;
       
        if (emptyVar.empty() && CheckEmptyColumn(v3_col_id, v3_time)) {
            emptyVar =  v3_name;
        }
	}
	if (num_var >= 4) {
		//v4_col_id = col_id_map[lb4->GetSelection()];
        int sel_idx = lb4->GetSelection();
        int col_idx = sel4_idx_map[sel_idx];
        v4_col_id = col_id_map[col_idx];
		v4_name = table_int->GetColName(v4_col_id);
		col_ids[3] = v4_col_id;
		var_info[3].time = v4_time;
        
        if (emptyVar.empty() && CheckEmptyColumn(v4_col_id, v4_time)) {
            emptyVar =  v4_name;
        }
	}
    

	for (int i=0; i<num_var; i++) {
		// Set Primary GdaVarTools::VarInfo attributes
		var_info[i].name = table_int->GetColName(col_ids[i]);
		var_info[i].is_time_variant = table_int->IsColTimeVariant(col_ids[i]);
       
        if (var_info[i].is_time_variant) {
            int n_timesteps = table_int->GetColTimeSteps(col_ids[i]);
            var_info[i].time_min = 0;
            var_info[i].time_max = n_timesteps>0 ? n_timesteps - 1 : 0;
        }
        
		// var_info[i].time already set above
		table_int->GetMinMaxVals(col_ids[i], var_info[i].min, var_info[i].max);
		var_info[i].sync_with_global_time = var_info[i].is_time_variant;
		var_info[i].fixed_scale = true;
	}
	// Call function to set all Secondary Attributes based on Primary Attributes
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	GdaVarTools::PrintVarInfoVector(var_info);
    
    return emptyVar;
}

