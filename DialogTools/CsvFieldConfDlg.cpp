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

#include "stdio.h"
#include <iostream>
#include <sstream>


#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../GdaException.h"
#include "../ShapeOperations/OGRDataAdapter.h"
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
    wxTextFile tfile;
    tfile.Open(filepath);
    
    // read the first line
    wxString str = tfile.GetFirstLine();
    int start = 0;
    bool inQuotes = false;
    for (int current = 0; current < str.length(); current++) {
        if (str[current] == '\"')
            inQuotes = !inQuotes; // toggle state
        bool atLastChar = (current == str.length() - 1);
        if(atLastChar)
            col_names.push_back(str.SubString(start, str.length()-1));
        else if (str[current] == ',' && !inQuotes) {
            col_names.push_back(str.SubString(start, current-1));
            start = current + 1;
        }
    }
   
    int n_rows = col_names.size();
    int n_cols = 2; // 1 Var name 2 type
    
    for (int i=0; i<n_rows; i++)
        types.push_back("String");
    
    // read the preview content
    wxString second_line;
    int max_prev_rows = 10, cnt = 0;
    
    while(!tfile.Eof())
    {
        wxString str = tfile.GetNextLine();
        if (!str.IsEmpty()) {
            prev_lines.push_back(str);
            cnt++;
            if (cnt == 1)
                second_line = str;
            if (cnt >= max_prev_rows)
                break;
        }
    }
    n_prev_rows = prev_lines.size();
    n_prev_cols = n_rows;
    
    
    wxString prmop_txt = "Please Specify Data Type for Each Data Column.";
    wxString csvt_path = filepath + "t";
    
    if (wxFileExists(csvt_path)) {
        prmop_txt += "\n(Note: Data types are loaded from .csvt file)";
    }
    
    // Create controls UI
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, prmop_txt);
    
    wxBoxSizer* lbl_box = new wxBoxSizer(wxVERTICAL);
    lbl_box->AddSpacer(5);
    lbl_box->Add(lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    fieldGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(250,200));
    fieldGrid->CreateGrid(n_rows, n_cols, wxGrid::wxGridSelectRows);
    fieldGrid->SetColLabelValue(0, "Column Name");
    fieldGrid->SetColLabelValue(1, "Data Type");
    
    wxBoxSizer* grid_box = new wxBoxSizer(wxVERTICAL);
    grid_box->AddSpacer(5);
    grid_box->Add(fieldGrid, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);

    
    // Preview controls
    wxStaticText* prev_lbl = new wxStaticText(panel, wxID_ANY, "Data Preview");
    
    wxBoxSizer* prev_lbl_box = new wxBoxSizer(wxVERTICAL);
    prev_lbl_box->AddSpacer(5);
    prev_lbl_box->Add(prev_lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    previewGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 100));
    previewGrid->CreateGrid(n_prev_rows, n_prev_cols, wxGrid::wxGridSelectRows);
    previewGrid->SetEditable(false);
    //ApreviewGrid->SetBackgroundColour(wxColour(100,100,100));
    
    wxBoxSizer* preview_box = new wxBoxSizer(wxVERTICAL);
    preview_box->AddSpacer(5);
    preview_box->Add(previewGrid, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, "Cancel", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    wxButton* btn_update= new wxButton(panel, wxID_ANY, "OK", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_update, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(lbl_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
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
    
    
    btn_update->Connect(wxEVT_BUTTON, wxCommandEventHandler(CsvFieldConfDlg::OnOkClick), NULL, this);
    btn_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(CsvFieldConfDlg::OnCancelClick), NULL, this);
    
    
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
        
    } else {
    
        // read second line, guess the type
        int start = 0;
        bool inQuotes = false;
        wxString str = second_line;
        int idx = 0;
        
        for (int current = 0; current < str.length(); current++) {
            wxString val;
            bool flag = false;
            if (str[current] == '\"')
                inQuotes = !inQuotes; // toggle state
            bool atLastChar = (current == str.length() - 1);
            if(atLastChar) {
                val = str.SubString(start, str.length()-1);
                flag = true;
            } else if (str[current] == ',' && !inQuotes) {
                val = str.SubString(start, current-1);
                start = current + 1;
                flag = true;
            }
            if (flag ) {
                double d_val = 0;
               
                if (idx >= n_prev_cols)
                    continue;
                if (val.IsNumber()) {
                    types[idx++]  = "Integer";
                } else if (val.ToDouble(&d_val)) {
                    types[idx++]  = "Real";
                } else {
                    types[idx++]  = "String";
                }
            }
        }
    }
    
    for (int i=0; i<n_rows; i++) {
        wxString strChoices[4] = {"Real", "Integer", "String"};
        int COL_T = 1;
        wxGridCellChoiceEditor* m_editor = new wxGridCellChoiceEditor(4, strChoices, false);
        fieldGrid->SetCellEditor(i, COL_T, m_editor);
        
        
        fieldGrid->SetCellValue(i, 0, col_names[i]);
        if (types.size() == 0 || i >= types.size() ) {
            fieldGrid->SetCellValue(i, COL_T, "String");
        } else {
            fieldGrid->SetCellValue(i, COL_T, types[i]);
        }
    }
    

    fieldGrid->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED,
                       wxCommandEventHandler(CsvFieldConfDlg::OnFieldSelected),
                       NULL,
                       this);

    UpdatePreview();
    
    LOG_MSG("Exiting CsvFieldConfDlg::CsvFieldConfDlg(..)");
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
    
    UpdatePreview();
    
    event.Skip();
}


void CsvFieldConfDlg::UpdatePreview( )
{
    previewGrid->BeginBatch();

    previewGrid->ClearGrid();
    
    for (int i=0; i<n_prev_cols; i++) {
        previewGrid->SetColLabelValue(i, col_names[i]);
    }
    
    for (int i=0; i<n_prev_rows; i++) {
        wxString str = prev_lines[i];
        wxStringTokenizer tokenizer(str, ",");
        int j=0;
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken().Trim(true).Trim(false);
            bool flag = false;
            
            if (types[j] == "Integer") {
                long long val =0;
                double val1 = 0;
                if (token.ToLongLong(&val)) {
                    wxString str = wxString::Format("%lld", val);
                    previewGrid->SetCellValue(i, j, str);
                    flag = true;
                } else if (token.ToDouble(&val1)) {
                    val = (long long)val1;
                    wxString str = wxString::Format("%lld", val);
                    previewGrid->SetCellValue(i, j, str);
                    flag = true;
                }
            } else if (types[j] == "Real") {
                double val =0;
                if (token.ToDouble(&val)) {
                    wxString str = wxString::Format("%f", val);
                    previewGrid->SetCellValue(i, j, str);
                    flag = true;
                }
            }
            
            if (!flag)
                previewGrid->SetCellValue(i, j, token);
            j++;
        }
    }
    previewGrid->ForceRefresh();
    previewGrid->EndBatch();
}

void CsvFieldConfDlg::OnOkClick( wxCommandEvent& event )
{
    bool success = false;
  
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
    EndDialog(wxID_OK);
}

void CsvFieldConfDlg::OnCancelClick( wxCommandEvent& event )
{
    EndDialog(wxID_CANCEL);
}

