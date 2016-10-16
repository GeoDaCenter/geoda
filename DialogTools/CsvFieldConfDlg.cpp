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
    
    LOG_MSG("Entering CsvFieldConfDlg::CsvFieldConfDlg(..)");
    
    filepath = _filepath;
    
    wxString prmop_txt = "Please Specify Data Type for Each Data Column.";
    wxString csvt_path = filepath + "t";
    
    if (wxFileExists(csvt_path)) {
        prmop_txt += "\n(Note: Data types are loaded from .csvt file)";
    }
    
    PrereadCSV();
    
    // Create controls UI
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, prmop_txt);
    
    wxBoxSizer* lbl_box = new wxBoxSizer(wxVERTICAL);
    lbl_box->AddSpacer(5);
    lbl_box->Add(lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    /*
    // separator selection ctonrol
    wxString separator_txt = _T("Select separator used in the csv file:");
    wxStaticText* sep_lbl = new wxStaticText(panel, wxID_ANY, separator_txt);
    
    wxComboBox* separator_cmbox = new wxComboBox(panel, wxID_ANY,
                                                 separator_txt,
                                                 wxDefaultPosition,
                                                 wxSize(30,-1), 0, NULL,
                                                 wxCB_READONLY);
    separator_cmbox->Append("COMMA");
    separator_cmbox->Append("SEMICOLON");
    separator_cmbox->Append("TAB");
    separator_cmbox->Append("SPACE");
    separator_cmbox->SetSelection(0);
   
    wxBoxSizer* separator_box = new wxBoxSizer(wxHORIZONTAL);
    separator_box->AddSpacer(10);
    separator_box->Add(sep_lbl, 1, wxALIGN_CENTER | wxALIGN_LEFT, 1);
    separator_box->Add(separator_cmbox, 1, wxALIGN_CENTER, 1);
     */
    
    // field grid selection control
    fieldGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(250,200));
    fieldGrid->CreateGrid(n_prev_cols, 2, wxGrid::wxGridSelectRows);
    
    UpdateFieldGrid();
    
    wxBoxSizer* grid_box = new wxBoxSizer(wxVERTICAL);
    grid_box->AddSpacer(5);
    grid_box->Add(fieldGrid, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    // Preview label controls
    wxStaticText* prev_lbl = new wxStaticText(panel, wxID_ANY, "Data Preview");
    
    wxBoxSizer* prev_lbl_box = new wxBoxSizer(wxVERTICAL);
    prev_lbl_box->AddSpacer(5);
    prev_lbl_box->Add(prev_lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
   
    // Preview Grid controls
    previewGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 100));
    previewGrid->CreateGrid(n_prev_rows, n_prev_cols, wxGrid::wxGridSelectRows);
    previewGrid->SetEditable(false);
    previewGrid->SetDefaultCellAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    
    wxBoxSizer* preview_box = new wxBoxSizer(wxVERTICAL);
    preview_box->AddSpacer(5);
    preview_box->Add(previewGrid, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    // buttons
    wxButton* btn_locale= new wxButton(panel, wxID_ANY, "Set Number Separators",
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, "Cancel",
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxButton* btn_update= new wxButton(panel, wxID_ANY, "OK",
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_locale, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->AddSpacer(10);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_update, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    // main container
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(lbl_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    //box->Add(separator_box, 0, wxALIGN_CENTER| wxRIGHT | wxLEFT | wxTOP, 10);
    box->Add(grid_box, 0, wxALIGN_CENTER| wxEXPAND| wxRIGHT | wxTOP, 0);
    box->Add(prev_lbl_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(preview_box, 0, wxALIGN_CENTER| wxEXPAND| wxRIGHT | wxTOP, 0);
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
    
    fieldGrid->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
                       wxCommandEventHandler(CsvFieldConfDlg::OnFieldSelected),
                       NULL,
                       this);
    
    LOG_MSG("Exiting CsvFieldConfDlg::CsvFieldConfDlg(..)");
}

CsvFieldConfDlg::~CsvFieldConfDlg()
{
    for (size_t i=0; i<prev_data.size(); i++) {
        OGRFeature::DestroyFeature(prev_data[i]);
    }
}

void CsvFieldConfDlg::PrereadCSV()
{
    const char* pszDsPath = GET_ENCODED_FILENAME(filepath);
    const char *papszOpenOptions[255] = {"AUTODETECT_TYPE=YES"};
    GDALDataset* poDS = (GDALDataset*) GDALOpenEx(pszDsPath,
                                                  GDAL_OF_VECTOR,
                                                  NULL,
                                                  papszOpenOptions,
                                                  NULL);
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
            types.push_back("Integer");
        } else if( poFieldDefn->GetType() == OFTReal ) {
            types.push_back("Real");
        } else {
            types.push_back("String");
        }
    }
    
    prev_lines.clear();
    int n_max_line = 10;
    int cnt = 0;
    
    for (size_t i=0; i<prev_data.size(); i++) {
        OGRFeature::DestroyFeature(prev_data[i]);
    }
    prev_data.clear();
    
    OGRFeature *poFeature;
    poLayer->ResetReading();
    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        if (cnt > n_max_line)
            break;
        
        for(int iField = 0; iField < nFields; iField++)
        {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
            
            if( poFieldDefn->GetType() == OFTInteger ) {
                poFeature->GetFieldAsInteger( iField );
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
    fieldGrid->SaveEditControlValue();
    fieldGrid->EnableCellEditControl(false);
    
    int n_cols = col_names.size();
    for (int r=0; r < n_cols; r++ ) {
        wxString type = fieldGrid->GetCellValue(r, 1);
        types[r] = type;
    }
   
    WriteCSVT();
    PrereadCSV();
    
    UpdatePreviewGrid();
    event.Skip();
}


void CsvFieldConfDlg::UpdateFieldGrid( )
{
    fieldGrid->BeginBatch();
    fieldGrid->ClearGrid();
    
    fieldGrid->SetColLabelValue(0, "Column Name");
    fieldGrid->SetColLabelValue(1, "Data Type");
    
    for (int i=0; i<col_names.size(); i++) {
        fieldGrid->SetCellValue(i, 0, col_names[i]);
        
        wxString strChoices[4] = {"Real", "Integer", "String"};
        int COL_T = 1;
        wxGridCellChoiceEditor* m_editor = new wxGridCellChoiceEditor(4, strChoices, false);
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

void CsvFieldConfDlg::UpdatePreviewGrid( )
{
    previewGrid->BeginBatch();
    previewGrid->ClearGrid();
    
    for (int i=0; i<col_names.size(); i++) {
        previewGrid->SetColLabelValue(i, col_names[i]);
    }
    
    for (int i=0; i<prev_data.size(); i++) {
        OGRFeature* poFeature = prev_data[i];
        for (int j=0; j<col_names.size(); j++) {
            bool undef = !poFeature->IsFieldSet(j);
            if (undef) {
                previewGrid->SetCellValue(i, j, "");
                continue;
            }
            
            if (types[j] == "Integer") {
                wxInt64 val = poFeature->GetFieldAsInteger64(j);
                wxString str = wxString::Format("%lld", val);
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
            if (token.Contains("INTEGER")) {
                types[idx++] = "Integer";
            } else if (token.Contains("REAL")) {
                types[idx++] = "Real";
            } else {
                types[idx++] = "String";
            }
        }
    }
}

void CsvFieldConfDlg::WriteCSVT()
{
    wxString csvt;
    
    int n_rows = col_names.size();
    for (int r=0; r < n_rows; r++ ) {
        wxString type = fieldGrid->GetCellValue(r, 1);
        csvt << type;
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
   
    WriteCSVT();
    
    EndDialog(wxID_OK);
}

void CsvFieldConfDlg::OnCancelClick( wxCommandEvent& event )
{
    EndDialog(wxID_CANCEL);
}

void CsvFieldConfDlg::OnSetupLocale( wxCommandEvent& event )
{
    bool need_reopen = false;
    LocaleSetupDlg localeDlg(this, need_reopen);
    localeDlg.ShowModal();
    
    PrereadCSV();
    UpdatePreviewGrid();
}
