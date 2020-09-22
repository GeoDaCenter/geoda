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

#include <set>
#include <limits>
#include <algorithm>
#include <cstddef>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/dialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/tokenzr.h>
#include <boost/foreach.hpp>

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../Explore/MLJCCoordinator.h"
#include "../Explore/MLJCMapNewView.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "MultiQuantileLisaDlg.h"

BEGIN_EVENT_TABLE( MultiQuantileLisaDlg, wxDialog )
EVT_CLOSE( MultiQuantileLisaDlg::OnClose )
END_EVENT_TABLE()

MultiQuantileLisaDlg::MultiQuantileLisaDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Multivariate Quantile LISA Dialog"))
{
    wxLogMessage("Open MultiQuantileLisaDlg Dialog.");
   
    CreateControls();
}

MultiQuantileLisaDlg::~MultiQuantileLisaDlg()
{
}

void MultiQuantileLisaDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(800,560), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate(5, 5);
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    wxStaticBoxSizer *hbox_quantile = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Add a Variable for Quantile LISA:"));
    wxBoxSizer* left_box = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* middle_box = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* right_box = new wxBoxSizer(wxVERTICAL);


    // variable list
    combo_var = new wxListBox(panel, wxID_ANY, wxDefaultPosition,
                              wxSize(280,250), 0, NULL,
                              wxLB_SINGLE | wxLB_HSCROLL| wxLB_NEEDED_SB);
    InitVariableCombobox(combo_var, false, false);
    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(15,2,10,0);

    // Quantiles
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Number of Quantiles:"));
    txt_quantiles = new wxTextCtrl(panel, wxID_ANY, "5",wxDefaultPosition, wxSize(70,-1));
    txt_quantiles->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_quantiles, 1, wxEXPAND);

    // Select Quantile
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Select a Quantile for LISA:"));
    wxString choices13[] = {"1", "2", "3", "4", "5"};
    cho_quantile = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(70,-1), 5, choices13);
    cho_quantile->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(cho_quantile, 1, wxEXPAND);

    // Quantiles
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Save Quantile Selection in Field:"));
    txt_output_field = new wxTextCtrl(panel, wxID_ANY, "QT1",wxDefaultPosition, wxSize(70,-1));

    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_output_field, 1, wxEXPAND);

    wxBoxSizer* var_box = new wxBoxSizer(wxVERTICAL);
    var_box->Add(combo_var, 1,  wxEXPAND | wxALL, 10);
    var_box->Add(gbox, 0,  wxEXPAND);

    // list contrl
    lst_quantile = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(400, 180), wxLC_REPORT);
    lst_quantile->AppendColumn(_("Variable"));
    lst_quantile->SetColumnWidth(0, 80);
    lst_quantile->AppendColumn(_("Number of Quantiles"), wxLIST_FORMAT_RIGHT);
    lst_quantile->SetColumnWidth(1, 120);
    lst_quantile->AppendColumn(_("Select Quantile"), wxLIST_FORMAT_RIGHT);
    lst_quantile->SetColumnWidth(2, 120);
    lst_quantile->AppendColumn(_("New Field"));
    lst_quantile->SetColumnWidth(3, 80);
    

    // move buttons
    move_left = new wxButton(panel, wxID_ANY, "<", wxDefaultPosition, wxSize(25,25));
    move_right = new wxButton(panel, wxID_ANY, ">", wxDefaultPosition, wxSize(25,25));
    middle_box->Add(move_right, 0, wxTOP, 100);
    middle_box->Add(move_left, 0, wxTOP, 10);

    left_box->Add(var_box);
    right_box->Add(lst_quantile, 1, wxALL|wxEXPAND, 5);

    hbox_quantile->Add(left_box);
    hbox_quantile->Add(middle_box);
    hbox_quantile->Add(right_box, 1, wxALL|wxEXPAND);

    // add spatial weights selection control
    wxBitmap w_bitmap(wxXmlResource::Get()->LoadBitmap("SpatialWeights_Bmp"));
    weights_btn = new wxBitmapButton(panel, wxID_ANY, w_bitmap, wxDefaultPosition,
                                     w_bitmap.GetSize(), wxTRANSPARENT_WINDOW | wxBORDER_NONE);
    st_spatial_w = new wxStaticText(panel, wxID_ANY, _("Select Spatial Weights:"));
    m_spatial_weights = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer *hbox_spatial_w = new wxBoxSizer(wxHORIZONTAL);
    hbox_spatial_w->Add(st_spatial_w, 0, wxALIGN_CENTER_VERTICAL | wxLEFT| wxRIGHT, 10);
    hbox_spatial_w->Add(m_spatial_weights, 0, wxALIGN_CENTER_VERTICAL| wxRIGHT, 10);
    hbox_spatial_w->Add(weights_btn,  0, wxALIGN_CENTER_VERTICAL);
    // init the spatial weights control
    InitSpatialWeights(m_spatial_weights);

    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox_quantile, 1, wxEXPAND | wxALL, 10);
    vbox->Add(hbox_spatial_w, 0, wxALIGN_LEFT | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox, 1, wxEXPAND);
    
    panel->SetSizer(container);
   
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(panel, 1, wxEXPAND|wxALL, 0);
    
    scrl->SetSizer(panelSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(scrl, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);

    Centre();
    
    // Events
    txt_quantiles->Bind(wxEVT_KEY_UP, &MultiQuantileLisaDlg::OnChangeQuantiles, this);
    move_left->Bind(wxEVT_BUTTON, &MultiQuantileLisaDlg::OnRemoveRow, this);
    move_right->Bind(wxEVT_BUTTON, &MultiQuantileLisaDlg::OnAddRow, this);

    weights_btn->Bind(wxEVT_BUTTON, &MultiQuantileLisaDlg::OnSpatialWeights, this);
    okButton->Bind(wxEVT_BUTTON, &MultiQuantileLisaDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &MultiQuantileLisaDlg::OnCloseClick, this);
}

std::list<int> MultiQuantileLisaDlg::GetListSel(wxListCtrl* lc)
{
    std::list<int> l;
    if (!lc) return l;
    long item = -1;
    for ( ;; ) {
        item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if ( item == -1 ) break;
        l.push_back(item);
    }
    return l;
}

void MultiQuantileLisaDlg::OnRemoveRow(wxCommandEvent& event)
{
    std::list<int> sels = GetListSel(lst_quantile);
    sels.sort();
    sels.reverse();
    if (!sels.empty()) {
        BOOST_FOREACH(int i, sels) {
            wxString field_name = lst_quantile->GetItemText(i, 3);
            new_fields.erase(field_name);
            lst_quantile->DeleteItem(i);
        }
    }
}

void MultiQuantileLisaDlg::OnAddRow(wxCommandEvent& event)
{
    // check if inputs are valid
    if (project == NULL) return;

    // get selected variable
    int sel_var = combo_var->GetSelection();
    if (sel_var < 0) {
        wxString err_msg = _("Please select a variable for Quantile LISA.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    wxString var_name = combo_var->GetString(sel_var);
    wxString col_name = name_to_nm[var_name];

    int col = table_int->FindColId(col_name);
    if (col == wxNOT_FOUND) {
        wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen this dialog to synchronize with Table data."), col_name);
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    // get num quantiles
    long l_quantiles = 0;
    wxString tmp_quantiles = txt_quantiles->GetValue();
    if (tmp_quantiles.ToLong(&l_quantiles) == false) {
        wxString err_msg = _("The input value for the number of quantiles is not valid.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }

    // get select quantile
    int sel_quantile = cho_quantile->GetSelection() + 1;

    // save the binary data in table
    wxString field_name = txt_output_field->GetValue();
    if (field_name.IsEmpty() || new_fields.find(field_name) != new_fields.end()) {
        wxString err_msg = _("Please enter a valid and non-duplicated field name for saving the quantile selection as binary data in table.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
    
    // add as a new row to listctrol
    int new_row = lst_quantile->GetItemCount();
    lst_quantile->InsertItem(new_row, var_name);
    lst_quantile->SetItem(new_row, 1, tmp_quantiles);
    lst_quantile->SetItem(new_row, 2, wxString::Format("%d", sel_quantile));
    lst_quantile->SetItem(new_row, 3, field_name);

    // save for detect duplicates
    new_fields.insert(field_name);

    // update the suggested field name
    wxString suggest_field_nm = "QT";
    suggest_field_nm << lst_quantile->GetItemCount()+1;
    txt_output_field->SetValue(suggest_field_nm);
}


void MultiQuantileLisaDlg::update(TableState* o)
{
    InitVariableCombobox(combo_var, /*integer + real*/false, /* no centroids*/false);
}

void MultiQuantileLisaDlg::OnChangeQuantiles(wxKeyEvent& event)
{
    wxString tmp_val = txt_quantiles->GetValue();
    tmp_val.Trim(false);
    tmp_val.Trim(true);

    if (tmp_val.IsEmpty()) return;

    long quantiles = 0;
    if (tmp_val.ToLong(&quantiles) == false || quantiles < 2) {
        wxString err_msg = _("The input value for the number of quantiles is not valid. Please enter an integer number greater than 1.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        txt_quantiles->SetValue("5");
        return;
    }
    // change dropdown list items in cho_quantiles
    cho_quantile->Clear();
    for (int i=0; i<quantiles; ++i) {
        cho_quantile->Append(wxString::Format("%d", i+1));
    }
    cho_quantile->SetSelection(0);
    event.Skip();
}

void MultiQuantileLisaDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close MultiQuantileLisaDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void MultiQuantileLisaDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close MultiQuantileLisaDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void MultiQuantileLisaDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click MultiQuantileLisaDlg::OnOK");

    if (project == NULL) return;

    // Weights selection
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) {
        return;
    }

    // get selected variable
    int num_vars = lst_quantile->GetItemCount();

    if (num_vars < 2) {
        wxString err_msg = _("Please use the > button to specify more than one variable for Multivarite Quantile LISA.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
        dlg.ShowModal();
        return;
    }

    std::vector<int> col_ids(num_vars);
    std::vector<GdaVarTools::VarInfo> var_info(num_vars);
    wxString var_details;

    for (int i=0; i<num_vars; ++i) {
        wxString var_name = lst_quantile->GetItemText(i, 0);
        wxString col_name = name_to_nm[var_name];
        int tm = name_to_tm_id[var_name];
        int col = table_int->FindColId(col_name);
        std::vector<double> data;
        table_int->GetColData(col, tm, data);

        wxString tmp_quantiles = lst_quantile->GetItemText(i, 1);
        long l_quantiles = 0;
        tmp_quantiles.ToLong(&l_quantiles);
        int n_quantiles = l_quantiles;

        wxString tmp_selquantile = lst_quantile->GetItemText(i, 2);
        long l_selquantile = 0;
        tmp_selquantile.ToLong(&l_selquantile);
        int sel_quantile = l_selquantile;

        wxString field_name = lst_quantile->GetItemText(i, 3);
        if (field_name.IsEmpty()) {
            wxString err_msg = _("Please enter a field name for saving the quantile selection as binary data in table.");
            wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_WARNING);
            dlg.ShowModal();
            return;
        }

        var_details << var_name << "/" << sel_quantile << "/" << n_quantiles;
        if (i < num_vars-1) var_details << ",";

        // compute quantile of selected variable
        std::vector<double> breaks(n_quantiles - 1);
        std::vector<double> sort_data = data;
        std::sort(sort_data.begin(), sort_data.end());
        for (int i=0; i < breaks.size(); ++i) {
            breaks[i] = Gda::percentile(((i+1.0)*100.0)/((double) n_quantiles), sort_data);
        }
        breaks.insert(breaks.begin(), std::numeric_limits<double>::min());
        breaks.push_back(std::numeric_limits<double>::max());

        // create a binary data for selected quantile
        std::vector<wxInt64> bin_data(rows, 0);
        double lower_bound = breaks[sel_quantile-1];
        double upper_bound = breaks[sel_quantile];

        for (int i=0; i < data.size(); ++i) {
            if (lower_bound < data[i] && data[i] <= upper_bound) {
                bin_data[i] = 1;
            }
        }
        int time=0;
        int new_col = table_int->FindColId(field_name);
        if ( new_col == wxNOT_FOUND) {
            int col_insert_pos = table_int->GetNumberCols();
            int time_steps = 1;
            int m_length_val = GdaConst::default_dbf_long_len;
            int m_decimals_val = 0;
            new_col = table_int->InsertCol(GdaConst::long64_type, field_name,
                                           col_insert_pos, time_steps,
                                           m_length_val, m_decimals_val);
        } else {
            // detect if column is integer field, if not raise a warning
            if (table_int->GetColType(new_col) != GdaConst::long64_type ) {
                wxString msg = _("This field name already exists (non-integer type). Please input a unique name.");
                wxMessageDialog dlg(this, msg, _("Warning"), wxOK | wxICON_WARNING );
                dlg.ShowModal();
                return;
            }
        }

        if (new_col > 0) {
            std::vector<bool> clusters_undef(rows, false);
            table_int->SetColData(new_col, time, bin_data);
            table_int->SetColUndefined(col, time, clusters_undef);
        }

        col_ids[i] = new_col;
        var_info[i].time = 0;
        var_info[i].name = field_name;
        var_info[i].is_time_variant = false;
        table_int->GetMinMaxVals(new_col, var_info[i].min, var_info[i].max);
        var_info[i].sync_with_global_time = false;
        var_info[i].fixed_scale = true;
    }

    // compute binary local Join count
    int sel = m_spatial_weights->GetSelection();
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (sel >= weights_ids.size()) {
        sel = (int)weights_ids.size() - 1;
    }
    boost::uuids::uuid w_id = weights_ids[sel];

    JCCoordinator* lc = new JCCoordinator(w_id, project, var_info, col_ids);
    MLJCMapFrame *sf = new MLJCMapFrame(parent, project, lc, false);

    wxString ttl = _("Multivariate Quantile LISA Map (%s)");
    ttl = wxString::Format(ttl, var_details);
    sf->SetTitle(ttl);
}
