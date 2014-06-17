/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_FIELDNAMECORRECTION_DLG_H__
#define __GEODA_CENTER_FIELDNAMECORRECTION_DLG_H__

#include <set>
#include <map>
#include <vector>
#include <wx/dialog.h>
#include "../DataViewer/TableInterface.h"

using namespace std;

class FieldNameCorrectionDlg: public wxDialog
{
private:
    int  n_fields;
    bool need_correction;
    GdaConst::DataSourceType ds_type;
    map<wxString, wxString> field_names_dict;
    vector<wxString> merged_field_names;
    
public:
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           vector<wxString>& all_fname,
                           wxString title="Field name correction dialog.");
    
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           map<wxString, wxString>& fnames_dict,
                           vector<wxString>& merged_field_names,
                           set<wxString>& dup_fname,
                           set<wxString>& bad_fname,
                           wxString title="Field name correction dialog.");
   
    void OnOkClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);
    bool NeedCorrection(){return need_correction;}
    map<wxString, wxString> GetMergedFieldNameDict();

protected:
    void Init(vector<wxString>& merged_field_names,
              set<wxString>& dup_fname, set<wxString>& bad_fname);
    wxString GetSuggestFieldName(const wxString& old_name);
    wxString RenameDupFieldName(const wxString& old_name);
    wxString RemoveIllegalChars(const wxString& old_name);
    wxString TruncateFieldName(const wxString& old_name, int max_len=0);
    bool IsFieldNameValid(const wxString& col_name);
    
DECLARE_EVENT_TABLE()
};

#endif