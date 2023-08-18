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

class ScrolledWidgetsPane : public wxScrolledWindow
{
private:
	int  n_fields;
	GdaConst::DataSourceType ds_type;
    bool is_case_sensitive;
    // old_name : new_name (after correction)
    std::map<wxString, wxString> field_names_dict;
    std::map<wxString, bool> field_dict;
    std::vector<wxString> merged_field_names;
    
    std::map<wxString, int> user_input_dict;
    
    std::vector<wxString> old_field_names;
    std::vector<wxString> new_field_names;
    std::set<wxString> table_fnames;
    
public:
	// Variable number of controls
	size_t num_controls;
	std::vector<wxStaticText*> txt_fname; // ID_FNAME_STAT_TXT_BASE
	std::vector<wxTextCtrl*> txt_input; // ID_INPUT_TXT_CTRL_BASE
	std::vector<wxStaticText*> txt_info; // ID_INFO_STAT_TXT_BASE
	std::vector<wxStaticText*> input_info; // ID_INPUT_INFO_STAT_TXT_BASE
	
	wxButton* ok_btn; // wxID_OK
	wxButton* exit_btn; // wxID_CANCEL
	
    bool need_correction;

	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id);
	ScrolledWidgetsPane(wxWindow* parent,
                        wxWindowID id,
                        GdaConst::DataSourceType ds_type,
                        std::vector<wxString>& all_fname);
	ScrolledWidgetsPane(wxWindow* parent, wxWindowID id,
                        GdaConst::DataSourceType ds_type,
                        std::set<wxString> table_fnames,
                        std::map<wxString, wxString>& fnames_dict,
                        std::vector<wxString>& merged_field_names,
                        std::set<wxString>& dup_fname,
                        std::set<wxString>& bad_fname);
	virtual ~ScrolledWidgetsPane();

    void OnUserInput(wxCommandEvent& ev);
	wxString GetSuggestFieldName(const wxString& old_name);
	wxString GetSuggestFieldName(int field_idx);
	wxString RenameDupFieldName(const wxString& old_name);
	wxString RemoveIllegalChars(const wxString& old_name);
	wxString TruncateFieldName(const wxString& old_name, int max_len=0);
	bool IsFieldNameValid(const wxString& col_name);
	
    std::map<wxString, wxString> GetMergedFieldNameDict();
    std::vector<wxString> GetNewFieldNames();

	void Init(std::vector<wxString>& merged_field_names,
              std::set<wxString>& dup_fname,
              std::set<wxString>& bad_fname);
    
	void Init(std::vector<int>& dup_fname_idx_s, std::vector<int>& bad_fname_idx_s);
	
	bool CheckUserInput();
		
};

class FieldNameCorrectionDlg: public wxDialog
{
private:
	ScrolledWidgetsPane* fieldPane;
    bool need_correction;
    
public:
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           std::vector<wxString>& all_fname,
                           wxString title="Update Field Name");
    
    FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                           std::set<wxString> table_fnames,
                           std::map<wxString, wxString>& fnames_dict,
                           std::vector<wxString>& merged_field_names,
                           std::set<wxString>& dup_fname,
                           std::set<wxString>& bad_fname,
                           wxString title="Update Field Name");
	virtual ~FieldNameCorrectionDlg();

	bool NeedCorrection() { return need_correction;}
    
    std::map<wxString, wxString> GetMergedFieldNameDict(){ 
        return fieldPane->GetMergedFieldNameDict();
    }
    
    std::vector<wxString> GetNewFieldNames() {
        return fieldPane->GetNewFieldNames();
    }
	
	void OnOkClick(wxCommandEvent& event);
	void OnCancelClick(wxCommandEvent& event);
	void OnClose(wxCloseEvent& ev);
	
	DECLARE_EVENT_TABLE()
};

#endif
