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
#ifndef __GEODA_CENTER_CSVFIELDCONF_DLG_H__
#define __GEODA_CENTER_CSVFIELDCONF_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <ogrsf_frmts.h>




class CsvFieldConfDlg: public wxDialog
{
public:
    CsvFieldConfDlg(wxWindow* parent, wxString filepath,
                    wxWindowID id = wxID_ANY,
                    const wxString& title = _("GeoDa Csv Filed Configuration Dialog"),
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxSize(580,580));
    ~CsvFieldConfDlg();
    
    
private:
    int n_prev_cols;
    int n_prev_rows;
    std::vector<wxString> col_names;
    std::vector<wxString> prev_lines;
    std::vector<wxString> types;
    std::vector<OGRFeature*> prev_data;
    
    wxString filepath;
    wxGrid* fieldGrid;
    wxGrid* previewGrid;
    wxComboBox* lat_box;
    wxComboBox* lng_box;
   
    void ReadCSVT();
    void WriteCSVT();
    
    void PrereadCSV();
    
    void UpdateFieldGrid();
    void UpdatePreviewGrid();
    void UpdateXYcombox();
    void OnSetupLocale( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnFieldSelected(wxCommandEvent& event);
    
};

#endif
