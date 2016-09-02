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
#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>
#include <wx/textfile.h>

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
    wxStringTokenizer tokenizer(str, ",");
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        col_names.push_back(token);
    }
    
    int n_rows = col_names.size();
    int n_cols = 2; // 1 Var name 2 type
    
    
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
    
    wxStaticText* lbl = new wxStaticText(panel, wxID_ANY, "Please Specify Data Type for Each Data Column.");
    
    wxBoxSizer* lbl_box = new wxBoxSizer(wxVERTICAL);
    lbl_box->AddSpacer(5);
    lbl_box->Add(lbl, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    fieldGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300,-1));
    fieldGrid->CreateGrid(n_rows, n_cols, wxGrid::wxGridSelectRows);
    fieldGrid->SetColLabelValue(0, "Column Name");
    fieldGrid->SetColLabelValue(1, "Data Type");
    
    wxBoxSizer* grid_box = new wxBoxSizer(wxVERTICAL);
    grid_box->AddSpacer(5);
    grid_box->Add(fieldGrid, 1, wxALIGN_CENTER | wxEXPAND |wxALL, 10);
    
    wxButton* btn_cancel= new wxButton(panel, wxID_ANY, "Cancel", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    wxButton* btn_update= new wxButton(panel, wxID_ANY, "OK", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_update, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(lbl_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(grid_box, 0, wxALIGN_CENTER| wxEXPAND| wxRIGHT | wxTOP, 0);
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
    
    
    vector<wxString> types;
    wxString csvt_path = filepath + "t";
    
    if (wxFileExists(csvt_path)) {
        // load data type from csvt file
        wxTextFile csvt_file;
        csvt_file.Open(csvt_path);
        
        // read the first line
        wxString str = csvt_file.GetFirstLine();
        wxStringTokenizer tokenizer(str, ",");
        
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken().Upper();
            if (token.Contains("INTEGER")) {
                types.push_back("Integer");
            } else if (token.Contains("REAL")) {
                types.push_back("Real");
            } else {
                types.push_back("String");
            }
        }
        
    } else {
        // read second line, guess the type
        str = tfile.GetNextLine();
        wxStringTokenizer tokenizer1(str, ",");
        while ( tokenizer1.HasMoreTokens() )
        {
            wxString token = tokenizer1.GetNextToken();
            
            wxString val = token.Trim(true).Trim(false);
            double d_val = 0;
           
            if (val.IsNumber()) {
                types.push_back("Integer");
            } else if (val.ToDouble(&d_val)) {
                types.push_back("Real");
            } else {
                types.push_back("String");
            }
        }
    }
    
    for (int i=0; i<n_rows; i++) {
        wxString strChoices[4] = {"Real", "Integer", "String"};
        int COL_T = 1;
        fieldGrid->SetCellEditor(i, COL_T, new wxGridCellChoiceEditor(4, strChoices, false));
        
        fieldGrid->SetCellValue(i, 0, col_names[i]);
        fieldGrid->SetCellValue(i, COL_T, types[i]);
    }
    
    
    LOG_MSG("Exiting CsvFieldConfDlg::CsvFieldConfDlg(..)");
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

