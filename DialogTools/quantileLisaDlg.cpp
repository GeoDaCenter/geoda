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

#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../Explore/MLJCCoordinator.h"
#include "../Explore/MLJCMapNewView.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "quantileLisaDlg.h"

BEGIN_EVENT_TABLE( QuantileLisaDlg, wxDialog )
EVT_CLOSE( QuantileLisaDlg::OnClose )
END_EVENT_TABLE()

QuantileLisaDlg::QuantileLisaDlg(wxFrame *parent_s, Project* project_s)
: AbstractClusterDlg(parent_s, project_s, _("Quantile LISA Dialog"))
{
    wxLogMessage("Open QuantileLisaDlg Dialog.");
   
    CreateControls();
}

QuantileLisaDlg::~QuantileLisaDlg()
{
}

void QuantileLisaDlg::CreateControls()
{
    wxScrolledWindow* scrl = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(420,560), wxHSCROLL|wxVSCROLL );
    scrl->SetScrollRate(5, 5);
    
    wxPanel *panel = new wxPanel(scrl);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
   
    // Input
    AddInputCtrls(panel, vbox, false/*no auto button*/, false/*integer + real*/,
                  true/*spatial weights*/,
                  /*single variable*/true, /*add centroids*/true);

    // parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(15,2,10,0);

    // Quantiles
    wxStaticText* st17 = new wxStaticText(panel, wxID_ANY, _("Number of Quantiles:"));
    txt_quantiles = new wxTextCtrl(panel, wxID_ANY, "5",wxDefaultPosition, wxSize(150,-1));
    txt_quantiles->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

    gbox->Add(st17, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_quantiles, 1, wxEXPAND);
    
    // Select Quantile
    wxStaticText* st13 = new wxStaticText(panel, wxID_ANY, _("Select a Quantile for LISA:"));
    wxString choices13[] = {"1", "2", "3", "4", "5"};
    cho_quantile = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(150,-1), 5, choices13);
    cho_quantile->SetSelection(0);
    gbox->Add(st13, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(cho_quantile, 1, wxEXPAND);

    // Quantiles
    wxStaticText* st14 = new wxStaticText(panel, wxID_ANY, _("Save Quantile Selection in Field:"));
    txt_output_field = new wxTextCtrl(panel, wxID_ANY, "QT",wxDefaultPosition, wxSize(150,-1));

    gbox->Add(st14, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(txt_output_field, 1, wxEXPAND);

    wxStaticBoxSizer *hbox = new wxStaticBoxSizer(wxHORIZONTAL, panel, _("Parameters:"));
    hbox->Add(gbox, 1, wxEXPAND);
    
    // buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, _("Run"), wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_EXIT, _("Close"), wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
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
    txt_quantiles->Bind(wxEVT_KEY_UP, &QuantileLisaDlg::OnChangeQuantiles, this);
    okButton->Bind(wxEVT_BUTTON, &QuantileLisaDlg::OnOK, this);
    closeButton->Bind(wxEVT_BUTTON, &QuantileLisaDlg::OnCloseClick, this);
}

void QuantileLisaDlg::update(TableState* o)
{
    InitVariableCombobox(combo_var, /*integer + real*/false, /* no centroids*/false);
}

void QuantileLisaDlg::OnChangeQuantiles(wxKeyEvent& event)
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

void QuantileLisaDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Close QuantileLisaDlg");
    // Note: it seems that if we don't explictly capture the close event
    //       and call Destory, then the destructor is not called.
    Destroy();
}

void QuantileLisaDlg::OnCloseClick(wxCommandEvent& event )
{
    wxLogMessage("Close QuantileLisaDlg.");
    
    event.Skip();
    EndDialog(wxID_CANCEL);
    Destroy();
}

void QuantileLisaDlg::OnOK(wxCommandEvent& event )
{
    wxLogMessage("Click QuantileLisaDlg::OnOK");

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
    int tm = name_to_tm_id[var_name];
    std::vector<double> data;
    table_int->GetColData(col, tm, data);

    // Weights selection
    GalWeight* gw = CheckSpatialWeights();
    if (gw == NULL) {
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
    int n_quantiles = l_quantiles;

    // get select quantile
    int sel_quantile = cho_quantile->GetSelection() + 1;

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

    // save the binary data in table
    wxString field_name = txt_output_field->GetValue();
    if (field_name.IsEmpty()) {
        wxString err_msg = _("Please enter a field name for saving the quantile selection as binary data in table.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
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
        vector<bool> clusters_undef(rows, false);
        table_int->SetColData(new_col, time, bin_data);
        table_int->SetColUndefined(col, time, clusters_undef);
    }

    // compute binary local Join count
    int sel = m_spatial_weights->GetSelection();
    std::vector<boost::uuids::uuid> weights_ids;
    WeightsManInterface* w_man_int = project->GetWManInt();
    w_man_int->GetIds(weights_ids);
    if (sel >= weights_ids.size()) {
        sel = weights_ids.size() - 1;
    }
    boost::uuids::uuid w_id = weights_ids[sel];

    std::vector<int> col_ids;
    col_ids.push_back(new_col);

    std::vector<GdaVarTools::VarInfo> var_info;
    var_info.resize(1);
    var_info[0].time = 0;
    var_info[0].name = field_name;
    var_info[0].is_time_variant = false;
    table_int->GetMinMaxVals(new_col, var_info[0].min, var_info[0].max);
    var_info[0].sync_with_global_time = false;
    var_info[0].fixed_scale = true;

    JCCoordinator* lc = new JCCoordinator(w_id, project, var_info, col_ids);
    MLJCMapFrame *sf = new MLJCMapFrame(parent, project, lc, false);

    wxString ttl = _("Quantile LISA Map (%s, # of quantiles=%d, select quantile=%d)");
    ttl = wxString::Format(ttl, var_name, n_quantiles, sel_quantile);
    sf->SetTitle(ttl);
}
