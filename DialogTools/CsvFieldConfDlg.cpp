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



#include <string>
#include <vector>
#include <queue>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>
#include <wx/tokenzr.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>

#include <ogrsf_frmts.h>

#include "stdio.h"
#include <iostream>
#include <sstream>


#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../GdaException.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "LocaleSetupDlg.h"
#include "CsvFieldConfDlg.h"

using namespace std;


CsvFieldConfDlg::CsvFieldConfDlg(wxWindow* parent,
                                 wxString _filepath,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size )
: wxDialog(parent, id, title, pos, size)
{
    
    wxLogMessage("Open CsvFieldConfDlg.");
    
    n_max_rows = 10;
    filepath = _filepath;
    
    wxString prmop_txt = _("(Optional) You can change the data type for a field:");
    wxString csvt_path = filepath + "t";
    
    PrereadCSV();
    
    // Create controls UI
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, prmop_txt);
    wxBoxSizer* lbl_box = new wxBoxSizer(wxHORIZONTAL);
    lbl_box->Add(lbl, 1, wxEXPAND |  wxTOP , 0);
    
    // field grid selection control
    fieldGrid = new wxGrid(panel, wxID_ANY, wxDefaultPosition, wxSize(250,150));
    fieldGrid->CreateGrid(n_prev_cols, 2, wxGrid::wxGridSelectRows);
    UpdateFieldGrid();
	fieldGrid->EnableEditing(true);
    wxBoxSizer* grid_box = new wxBoxSizer(wxVERTICAL);
    grid_box->AddSpacer(5);
    grid_box->Add(fieldGrid, 1, wxEXPAND |wxLEFT, 10);
    
    // Preview label controls
    wxStaticText* prev_lbl = new wxStaticText(panel, wxID_ANY, _("Data Preview - number of preview records:"));
    prev_spin = new wxSpinCtrl(panel, wxID_ANY, "");
    n_max_rows = 10;
    prev_spin->SetRange(0, 1000);
    prev_spin->SetValue(n_max_rows);
    prev_spin->Connect(wxEVT_SPINCTRL,
                       wxCommandEventHandler(CsvFieldConfDlg::OnSampleSpinClick),
                       NULL,
                       this);
    
    wxBoxSizer* prev_lbl_box = new wxBoxSizer(wxHORIZONTAL);
    prev_lbl_box->Add(prev_lbl, 0, wxEXPAND |wxTOP |wxLEFT , 10);
    prev_lbl_box->Add(prev_spin, 0, wxEXPAND |wxTOP, 10);
   
    // Preview Grid controls
    previewGrid = new wxGrid(panel, wxID_ANY, wxDefaultPosition, wxSize(300, 150));
    previewGrid->CreateGrid(n_prev_rows, n_prev_cols, wxGrid::wxGridSelectRows);
    previewGrid->EnableEditing(false);
    previewGrid->SetDefaultCellAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    
    wxBoxSizer* preview_box = new wxBoxSizer(wxVERTICAL);
    preview_box->AddSpacer(5);
    preview_box->Add(previewGrid, 1, wxEXPAND | wxLEFT, 10);
   
    // lat/lon
    wxStaticText* lat_lbl = new wxStaticText(panel, wxID_ANY, _("(Optional) Longitude/X:"));
    wxStaticText* lng_lbl = new wxStaticText(panel, wxID_ANY, _("Latitude/Y:"));
    lat_box = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                     wxSize(80,-1), 0, NULL, wxCB_READONLY);
    lng_box = new wxComboBox(panel, wxID_ANY, _(""), wxDefaultPosition,
                                     wxSize(80,-1), 0, NULL, wxCB_READONLY);
    wxBoxSizer* latlng_box = new wxBoxSizer(wxHORIZONTAL);
    latlng_box->Add(lat_lbl, 0, wxALIGN_CENTER_VERTICAL);
    latlng_box->AddSpacer(5);
    latlng_box->Add(lat_box, 0, wxALIGN_CENTER_VERTICAL);
    latlng_box->AddSpacer(5);
    latlng_box->Add(lng_lbl, 0, wxALIGN_CENTER_VERTICAL);
    latlng_box->AddSpacer(5);
    latlng_box->Add(lng_box, 0, wxALIGN_CENTER_VERTICAL);
    
    // first row
    wxStaticText* header_lbl = new wxStaticText(panel, wxID_ANY, _("(Optional) First record has field names? "));
    wxComboBox* header_cmb = new wxComboBox(panel, wxID_ANY, _(""),
                                            wxDefaultPosition,
                                            wxDefaultSize, 0, NULL, wxCB_READONLY);
    header_cmb->Append("NO");
    header_cmb->Append("YES");
    header_cmb->Append("AUTO DETECT");
    header_cmb->SetSelection(2);
    header_cmb->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
                       wxCommandEventHandler(CsvFieldConfDlg::OnHeaderCmbClick),
                       NULL,
                       this);
    header_cmb->SetFocus();
    wxBoxSizer* header_box = new wxBoxSizer(wxHORIZONTAL);
    header_box->Add(header_lbl, 0, wxALIGN_CENTER_VERTICAL);
    header_box->Add(header_cmb, 0, wxALIGN_CENTER_VERTICAL);
    
    
    // buttons
    wxButton* btn_locale= new wxButton(panel, wxID_ANY, _("Set Number Separators"),
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, _("Cancel"),
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxButton* btn_update= new wxButton(panel, wxID_ANY, _("OK"),
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_locale, 1, wxALIGN_CENTER | wxALL, 10);
    btn_box->AddSpacer(10);
    btn_box->Add(btn_update, 1, wxALIGN_CENTER | wxALL, 10);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER | wxALL, 10);
    
    // main container
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(header_box, 0, wxEXPAND | wxLEFT | wxRIGHT |wxTOP , 10);
    box->Add(latlng_box, 0, wxEXPAND | wxLEFT | wxRIGHT |wxTOP , 10);
    box->Add(lbl_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(grid_box, 0, wxEXPAND| wxRIGHT, 10);
    box->Add(prev_lbl_box, 0, wxEXPAND |  wxTOP, 30);
    box->Add(preview_box, 0, wxEXPAND| wxRIGHT, 10);
    box->Add(btn_box, 0, wxALIGN_CENTER| wxLEFT | wxRIGHT | wxTOP, 20);
    
    panel->SetSizerAndFit(box);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    
    SetParent(parent);
    SetPosition(pos);
    Centre();
    
    btn_locale->Connect(wxEVT_BUTTON,
                        wxCommandEventHandler(CsvFieldConfDlg::OnSetupLocale),
                        NULL, this);
    btn_update->Connect(wxEVT_BUTTON,
                        wxCommandEventHandler(CsvFieldConfDlg::OnOkClick),
                        NULL, this);
    btn_cancel->Connect(wxEVT_BUTTON,
                        wxCommandEventHandler(CsvFieldConfDlg::OnCancelClick),
                        NULL, this);
    
    ReadCSVT();
    UpdatePreviewGrid();
    UpdateXYcombox();
    
    fieldGrid->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
                       wxCommandEventHandler(CsvFieldConfDlg::OnFieldSelected),
                       NULL,
                       this);
}

CsvFieldConfDlg::~CsvFieldConfDlg()
{
    for (size_t i=0; i<prev_data.size(); i++) {
        OGRFeature::DestroyFeature(prev_data[i]);
    }
}

void CsvFieldConfDlg::PrereadCSV(int HEADERS)
{
	wxLogMessage("CsvFieldConfDlg::PrereadCSV()");
    const char* pszDsPath = GET_ENCODED_FILENAME(filepath);
    
    GDALDataset* poDS;
    if (HEADERS == 1) {
        const char *papszOpenOptions[255] = {"AUTODETECT_TYPE=YES", "HEADERS=YES"};
        poDS = (GDALDataset*) GDALOpenEx(pszDsPath,
                                         GDAL_OF_VECTOR,
                                         NULL,
                                         papszOpenOptions,
                                         NULL);
    } else if (HEADERS == 0) {
        const char *papszOpenOptions[255] = {"AUTODETECT_TYPE=YES", "HEADERS=NO"};
        poDS = (GDALDataset*) GDALOpenEx(pszDsPath,
                                         GDAL_OF_VECTOR,
                                         NULL,
                                         papszOpenOptions,
                                         NULL);
        
    } else {
        const char *papszOpenOptions[255] = {"AUTODETECT_TYPE=YES"};
        poDS = (GDALDataset*) GDALOpenEx(pszDsPath,
                                         GDAL_OF_VECTOR,
                                         NULL,
                                         papszOpenOptions,
                                         NULL);
    }
    if( poDS == NULL ) {
        return;
    }
    
    OGRLayer  *poLayer = poDS->GetLayer(0);
    OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
    
    int nFields = poFDefn->GetFieldCount();
    n_prev_cols = nFields;
    col_names.clear();
    types.clear();
    
    for(int iField = 0; iField < nFields; iField++)
    {
        OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
        wxString fieldName = poFieldDefn->GetNameRef();
        col_names.push_back(fieldName);
        
        if( poFieldDefn->GetType() == OFTInteger ) {
            types.push_back("Integer");
        } else if( poFieldDefn->GetType() == OFTInteger64 ) {
            types.push_back("Integer64");
        } else if( poFieldDefn->GetType() == OFTReal ) {
            types.push_back("Real");
        } else {
            types.push_back("String");
        }
    }
    
    prev_lines.clear();
    int cnt = 0;
    
    for (size_t i=0; i<prev_data.size(); i++) {
        OGRFeature::DestroyFeature(prev_data[i]);
    }
    prev_data.clear();
   
    
    OGRFeature *poFeature;
    poLayer->ResetReading();
    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        if (cnt > n_max_rows)
            break;
        
        for(int iField = 0; iField < nFields; iField++)
        {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
            
            if( poFieldDefn->GetType() == OFTInteger ) {
                poFeature->GetFieldAsInteger64( iField );
            } else if( poFieldDefn->GetType() == OFTInteger64 ) {
                poFeature->GetFieldAsInteger64( iField );
            } else if( poFieldDefn->GetType() == OFTReal ) {
                poFeature->GetFieldAsDouble(iField);
            } else if( poFieldDefn->GetType() == OFTString ) {
                poFeature->GetFieldAsString(iField);
            } else {
                poFeature->GetFieldAsString(iField);
            }
        }
        prev_data.push_back(poFeature);
        cnt += 1;
    }
    
    n_prev_rows = cnt;
   
    GDALClose(poDS);
}



void CsvFieldConfDlg::OnFieldSelected(wxCommandEvent& event)
{
	wxLogMessage("CsvFieldConfDlg::OnFieldSelected()");
    fieldGrid->SaveEditControlValue();
    fieldGrid->EnableCellEditControl(false);
   
    int n_types = types.size();
    int n_cols = col_names.size();
    for (int r=0; r < n_cols; r++ ) {
        wxString type = fieldGrid->GetCellValue(r, 1);
        if (r < n_types)
            types[r] = type;
    }
   
    WriteCSVT();
    PrereadCSV(HEADERS);
    
    UpdatePreviewGrid();
    UpdateXYcombox();
    event.Skip();
}


void CsvFieldConfDlg::UpdateFieldGrid( )
{
	wxLogMessage("CsvFieldConfDlg::UpdateFieldGrid()");
    fieldGrid->BeginBatch();
    fieldGrid->ClearGrid();
    
    fieldGrid->SetColLabelValue(0, _("Column Name"));
    fieldGrid->SetColLabelValue(1, _("Data Type"));
    
    for (int i=0; i<col_names.size(); i++) {
        wxString col_name = col_names[i];
        fieldGrid->SetCellValue(i, 0, col_name);
        
        wxString strChoices[5] = {"Real", "Integer", "Integer64","String"};
        int COL_T = 1;
        wxGridCellChoiceEditor* m_editor = new wxGridCellChoiceEditor(5, strChoices, false);
        fieldGrid->SetCellEditor(i, COL_T, m_editor);
        
        if (types.size() == 0 || i >= types.size() ) {
            fieldGrid->SetCellValue(i, COL_T, "String");
        } else {
            fieldGrid->SetCellValue(i, COL_T, types[i]);
        }
       
    }
    
    fieldGrid->ForceRefresh();
    fieldGrid->EndBatch();
}

void CsvFieldConfDlg::UpdateXYcombox( )
{
	wxLogMessage("CsvFieldConfDlg::UpdateXYcombox()");
    lat_box->Clear();
    lng_box->Clear();
  
    bool first_item = true;
    for (int i=0; i<col_names.size(); i++) {
        if (types[i] == "Real") {
            if (first_item ) {
                lat_box->Append("");
                lng_box->Append("");
                first_item = false;
            }
            lat_box->Append(col_names[i]);
            lng_box->Append(col_names[i]);
        }
    }
    
    wxString csvt_path = filepath + "t";
    
    if (wxFileExists(csvt_path)) {
        // load data type from csvt file
        wxTextFile csvt_file;
        csvt_file.Open(csvt_path);
        
        // read the first line
        wxString str = csvt_file.GetFirstLine();
        wxStringTokenizer tokenizer(str, ",");
        
        int idx = 0;
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken().Upper();
            if (token.Contains("COORDX")) {
                wxString col_name = col_names[idx];
                int pos = lng_box->FindString(col_name);
                lat_box->SetSelection(pos);
            } else if (token.Contains("COORDY")) {
                wxString col_name = col_names[idx];
                int pos = lng_box->FindString(col_name);
                lng_box->SetSelection(pos);
            }
            idx += 1;
        }
    }

}

void CsvFieldConfDlg::UpdatePreviewGrid( )
{
	wxLogMessage("CsvFieldConfDlg::UpdatePreviewGrid()");
    previewGrid->BeginBatch();
    previewGrid->ClearGrid();
    
    int n_grid_row =previewGrid->GetNumberRows();
    
    int n_new_row = n_prev_rows;
    
    if (n_max_rows < n_new_row) n_new_row = n_max_rows;
        
    if (n_grid_row < n_new_row) {
        previewGrid->InsertRows(0, n_new_row - n_grid_row);
    }
    if (n_grid_row > n_new_row) {
        previewGrid->DeleteRows(0,  n_grid_row - n_new_row);
    }
    
    for (int i=0; i<col_names.size(); i++) {
        previewGrid->SetColLabelValue(i, col_names[i]);
    }
    
    for (int i=0; i<n_new_row; i++) {
        OGRFeature* poFeature = prev_data[i];
        for (int j=0; j<col_names.size(); j++) {
            bool undef = !poFeature->IsFieldSet(j);
            if (undef) {
                previewGrid->SetCellValue(i, j, "");
                continue;
            }
            
            if (types[j] == "Integer" || types[j] == "Integer64") {
                wxInt64 val = poFeature->GetFieldAsInteger64(j);
                wxString str = wxString::Format(wxT("%") wxT(wxLongLongFmtSpec) wxT("d"), val);
                previewGrid->SetCellValue(i, j, str);
                
            } else if (types[j] == "Real") {
                double val = poFeature->GetFieldAsDouble(j);
                wxString str = wxString::Format("%f", val);
                previewGrid->SetCellValue(i, j, str);
                
            } else {
                wxString str = poFeature->GetFieldAsString(j);
                previewGrid->SetCellValue(i, j, str);
            }
        }
    }
    previewGrid->ForceRefresh();
    previewGrid->EndBatch();
}

void CsvFieldConfDlg::ReadCSVT()
{
	wxLogMessage("CsvFieldConfDlg::ReadCSVT()");
    wxString csvt_path = filepath + "t";
    
    if (wxFileExists(csvt_path)) {
        // load data type from csvt file
        wxTextFile csvt_file;
        csvt_file.Open(csvt_path);
        
        // read the first line
        wxString str = csvt_file.GetFirstLine();
        wxStringTokenizer tokenizer(str, ",");
       
        int max_n_types = types.size();
        int idx = 0;
        while ( tokenizer.HasMoreTokens() && idx < max_n_types)
        {
            wxString token = tokenizer.GetNextToken().Upper();
            if (token.Contains("INTEGER64")) {
                types[idx] = "Integer64";
            } else if (token.Contains("INTEGER")) {
                types[idx] = "Integer";
            } else if (token.Contains("REAL")) {
                types[idx] = "Real";
            } else {
                types[idx] = "String";
            } 
            idx += 1;
        }
    }
}

void CsvFieldConfDlg::WriteCSVT()
{
	wxLogMessage("CsvFieldConfDlg::WriteCSVT()");
    wxString lat_col_name = lat_box->GetValue();
    wxString lng_col_name = lng_box->GetValue();
    
    wxString csvt;
    
    int n_rows = col_names.size();
    for (int r=0; r < n_rows; r++ ) {
        wxString col_name = fieldGrid->GetCellValue(r, 0);
        if (col_name == lat_col_name) {
            csvt << "CoordX";
        } else if (col_name == lng_col_name ) {
            csvt << "CoordY";
        } else {
            wxString type = fieldGrid->GetCellValue(r, 1);
            csvt << type;
        }
        if (r < n_rows-1)
            csvt << ",";
    }
    
    // write back to a CSVT file
    wxString csvt_path = filepath + "t";
    wxTextFile file(csvt_path);
    file.Open();
    file.Clear();
    
    file.AddLine( csvt );
    
    file.Write();
    file.Close();
}

void CsvFieldConfDlg::OnOkClick( wxCommandEvent& event )
{
	wxLogMessage("CsvFieldConfDlg::OnOkClick()");
   
    WriteCSVT();
    GdaConst::gda_ogr_csv_header = HEADERS;
    EndDialog(wxID_OK);
}

void CsvFieldConfDlg::OnCancelClick( wxCommandEvent& event )
{
	wxLogMessage("CsvFieldConfDlg::OnCancelClick()");
    EndDialog(wxID_CANCEL);
}

void CsvFieldConfDlg::OnSetupLocale( wxCommandEvent& event )
{
	wxLogMessage("CsvFieldConfDlg::OnSetupLocale()");
    bool need_reopen = false;
    LocaleSetupDlg localeDlg(this, need_reopen);
    localeDlg.ShowModal();
   
    PrereadCSV(HEADERS);
    UpdatePreviewGrid();
}

void CsvFieldConfDlg::OnHeaderCmbClick( wxCommandEvent& event )
{
	wxLogMessage("CsvFieldConfDlg::OnHeaderCmbClick()");
    HEADERS = (int)(event.GetSelection());
    
    PrereadCSV(HEADERS);
   
    UpdateFieldGrid();
    UpdatePreviewGrid();
    UpdateXYcombox();

}

void CsvFieldConfDlg::OnSampleSpinClick( wxCommandEvent& event )
{
	wxLogMessage("CsvFieldConfDlg::OnSampleSpinClick()");
    n_max_rows = prev_spin->GetValue();
    UpdatePreviewGrid();
}
