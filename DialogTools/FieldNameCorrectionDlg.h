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

#ifndef __GEODA_CENTER_FIELDNAMECORRECTION_DLG_H__
#define __GEODA_CENTER_FIELDNAMECORRECTION_DLG_H__

#include <set>
#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include "../DataViewer/TableInterface.h"

using namespace std;

class ScrolledWidgetsPane : public wxScrolledWindow
{
private:
	int  n_fields;
	GdaConst::DataSourceType ds_type;
    // old_name : new_name (after correction)
	map<wxString, wxString> field_names_dict;
	map<wxString, bool> field_dict;
	vector<wxString> merged_field_names;
    
    vector<wxString> old_field_names;
    vector<wxString> new_field_names;
	
public:
	// Variable number of controls
	size_t num_controls;
	std::vector<wxStaticText*> txt_fname; // ID_FNAME_STAT_TXT_BASE
	std::vector<wxTextCtrl*> txt_input; // ID_INPUT_TXT_CTRL_BASE
	std::vector<wxStaticText*> txt_info; // ID_INFO_STAT_TXT_BASE
	
	wxButton* ok_btn; // wxID_OK
	wxButton* exit_btn; // wxID_CANCEL
	
    bool need_correction;

    
public:
	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id);
	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id,
											GdaConst::DataSourceType ds_type,
											vector<wxString>& all_fname);	
	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id,
											GdaConst::DataSourceType ds_type,
											map<wxString, wxString>& fnames_dict,
											vector<wxString>& merged_field_names,
											set<wxString>& dup_fname,
											set<wxString>& bad_fname);		
	virtual ~ScrolledWidgetsPane();
	
	wxString GetSuggestFieldName(const wxString& old_name);
	wxString GetSuggestFieldName(int field_idx);
	wxString RenameDupFieldName(const wxString& old_name);
	wxString RemoveIllegalChars(const wxString& old_name);
	wxString TruncateFieldName(const wxString& old_name, int max_len=0);
	bool IsFieldNameValid(const wxString& col_name);
	
	map<wxString, wxString> GetMergedFieldNameDict();
    vector<wxString> GetNewFieldNames();

	void Init(vector<wxString>& merged_field_names,
						set<wxString>& dup_fname, set<wxString>& bad_fname);
	void Init(vector<int>& dup_fname_idx_s, vector<int>& bad_fname_idx_s);
	
	bool CheckUserInput();
		
};

class FieldNameCorrectionDlg: public wxDialog
{
private:
	ScrolledWidgetsPane* fieldPane;
    bool need_correction;
    
public:
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           vector<wxString>& all_fname,
                           wxString title="Field Name Correction");
    
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           map<wxString, wxString>& fnames_dict,
                           vector<wxString>& merged_field_names,
                           set<wxString>& dup_fname,
                           set<wxString>& bad_fname,
                           wxString title="Field Name Correction");
	virtual ~FieldNameCorrectionDlg();

	bool NeedCorrection() { return need_correction;}
    
	map<wxString, wxString> GetMergedFieldNameDict(){ 
        return fieldPane->GetMergedFieldNameDict();
    }
    
    vector<wxString> GetNewFieldNames() {
        return fieldPane->GetNewFieldNames();
    }
	
	void OnOkClick(wxCommandEvent& event);
	void OnCancelClick(wxCommandEvent& event);
	void OnClose(wxCloseEvent& ev);
	
	DECLARE_EVENT_TABLE()
};

#endif