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

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/regex.h>

#include "FieldNameCorrectionDlg.h"
#include "../GdaConst.h"


BEGIN_EVENT_TABLE( FieldNameCorrectionDlg, wxDialog )
//EVT_BUTTON( XRCID("IDC_OPEN_IASC"), FieldNameCorrectionDlg::OnBrowseDSfileBtn)
EVT_BUTTON( wxID_OK, FieldNameCorrectionDlg::OnOkClick )
EVT_BUTTON( wxID_CANCEL, FieldNameCorrectionDlg::OnCancelClick )
END_EVENT_TABLE()

FieldNameCorrectionDlg::
FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                       vector<wxString>& all_fname,
                       wxString title)
: wxDialog(NULL, -1, title), ds_type(ds_type), need_correction(false)
{
    vector<wxString> merged_field_names;
    set<wxString> bad_fnames, dup_fname;
    for (int i=0; i < all_fname.size(); i++) {
        wxString field_name = all_fname[i];
        field_names_dict[field_name] = field_name;
        if (!IsFieldNameValid(field_name) ) {
            bad_fnames.insert(field_name);
            merged_field_names.push_back(field_name);
        }
    }
    if (!bad_fnames.empty()) need_correction = true;
    Init(merged_field_names, dup_fname, bad_fnames);
}

FieldNameCorrectionDlg::
FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                       map<wxString, wxString>& fnames_dict,
                       vector<wxString>& merged_field_names,
                       set<wxString>& dup_fname,
                       set<wxString>& bad_fname,
                       wxString title)
: wxDialog(NULL, -1, title), field_names_dict(fnames_dict), ds_type(ds_type),
merged_field_names(merged_field_names), need_correction(true)
{
    Init(merged_field_names, dup_fname, bad_fname);
}

map<wxString, wxString> FieldNameCorrectionDlg::GetMergedFieldNameDict()
{
    return field_names_dict;
}

void FieldNameCorrectionDlg::Init(vector<wxString>& merged_field_names,
                                  set<wxString>& dup_fname,
                                  set<wxString>& bad_fname)
{
    n_fields = dup_fname.size() + bad_fname.size();
    int nrow = n_fields + 2;
    int ncol = 3;
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(nrow, ncol, 0, 0);
    SetSizer( gridSizer );
    // titile
    wxStaticText* txt_oname=new wxStaticText(this, wxID_STATIC, "Field name:");
    gridSizer->Add(txt_oname, 0, wxALIGN_LEFT|wxALL, 15);
    wxStaticText* txt_newname=new wxStaticText(this,wxID_STATIC,"New field name:");
    gridSizer->Add(txt_newname, 0,
                   wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 15);
    wxStaticText* txt_orest=new wxStaticText(this, wxID_STATIC,
											 "Field restrictions:");
    gridSizer->Add(txt_orest, 0,
                   wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 15);
    // add content
    wxString warn_msg;
    wxString DUP_WARN = "Duplicated field name.";
    wxString INV_WARN = "Field name is not valid.";
    for (size_t i=0; i < merged_field_names.size(); i++) {
        wxString field_name = merged_field_names[i];
        if (dup_fname.find(field_name)!=dup_fname.end()) warn_msg=DUP_WARN;
        else if (bad_fname.find(field_name)!=bad_fname.end()) warn_msg=INV_WARN;
        else continue;
        wxString user_field_name = GetSuggestFieldName(field_name);
        wxStaticText* txt_fname = new wxStaticText(this, wxID_STATIC,field_name);
        gridSizer->Add(txt_fname, 0, wxALIGN_LEFT|wxALL, 5);
        wxTextCtrl* txt_input = new wxTextCtrl(this, 1000+i,
											   user_field_name,
											   wxDefaultPosition,wxSize(240,-1));
        gridSizer->Add(txt_input, 0, wxALIGN_LEFT|wxALL, 5);
        wxStaticText* txt_info =
            new wxStaticText(this, wxID_STATIC, warn_msg);
        gridSizer->Add(txt_info, 0, wxALIGN_LEFT|wxALL, 5);
    }
   
    // buttons
    wxStaticText* txt_dummy = new wxStaticText(this, wxID_STATIC, "");
    gridSizer->Add(txt_dummy, 0, wxALIGN_LEFT | wxALL, 5);
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* ok_btn = new wxButton(this, wxID_OK, "OK");
    btnSizer->Add(ok_btn, 0,
                  wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL,15);
    wxButton* exit_btn = new wxButton(this, wxID_CANCEL, "Cancel");
    btnSizer->Add(exit_btn, 0,
                  wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL,15);
    gridSizer->Add(btnSizer, 0, wxALIGN_LEFT | wxALL, 15);
    
    gridSizer->Fit(this);
    gridSizer->SetSizeHints(this);
}

// give suggested field name based on GdaConst::DataSourceType
wxString FieldNameCorrectionDlg::GetSuggestFieldName(const wxString& old_name)
{
    wxString new_name = old_name;
    
    // if illegal name
    new_name = RemoveIllegalChars(new_name);

    // if name too long
    new_name = TruncateFieldName(new_name);
    
    // check duplicate name
    if ( field_names_dict.find(new_name) != field_names_dict.end() ) {
        new_name = RenameDupFieldName(new_name);
    }
    
    field_names_dict[old_name] = new_name;
    
    return new_name;
}

wxString FieldNameCorrectionDlg::RenameDupFieldName(const wxString& old_name)
{
    // duplicated name, try to append suffix in the form "_x"
    wxString new_name = old_name;
    wxRegEx regex;
    long suffix_number = 1;
    regex.Compile("_[0-9]+$");
    if ( regex.Matches(new_name) ) {
        wxString tmp_suffix = new_name.AfterLast('_');
        if (tmp_suffix.ToLong(&suffix_number)) {
            new_name = new_name.BeforeLast('_');
            suffix_number += 1;
        }
    }
    if (new_name.EndsWith("_")) new_name << suffix_number;
    else new_name << "_" << suffix_number;
   
    int max_len = GdaConst::datasrc_field_lens[ds_type];
    if ( new_name.length() > max_len ) {
        int offset = new_name.length() - max_len;
        wxString first_part = new_name.BeforeLast('_');
        wxString last_part = new_name.AfterLast('_');
        first_part = TruncateFieldName(first_part, first_part.length()-offset);
        new_name = first_part + "_" + last_part;
    }
    
    if ( field_names_dict.find(new_name) != field_names_dict.end() )
        return RenameDupFieldName(new_name);
    
    return new_name;
}

wxString FieldNameCorrectionDlg::TruncateFieldName(const wxString& old_name,
                                                   int max_len)
{
    if (GdaConst::datasrc_field_lens.find(ds_type) ==
        GdaConst::datasrc_field_lens.end())
    {
        return old_name;
    }
    if (max_len == 0) {
        max_len = GdaConst::datasrc_field_lens[ds_type];
    }
    int str_len = old_name.length();
    if ( str_len <= max_len ) return old_name;
   
    wxString separator = "_";
    int sep_len = separator.length();
    int chars_to_show = max_len - sep_len;
    int front_chars = ceil(chars_to_show / 2.0 );
    int back_chars = floor(chars_to_show / 2.0 );
    
    wxString new_name;
    new_name << old_name.substr(0, front_chars) << separator
    << old_name.substr(str_len - back_chars);
    
    return new_name;
}

wxString FieldNameCorrectionDlg::RemoveIllegalChars(const wxString& old_name)
{
    if (GdaConst::datasrc_field_illegal_regex.find(ds_type) ==
        GdaConst::datasrc_field_illegal_regex.end())
    {
        return old_name;
    }
    wxString illegal_regex = GdaConst::datasrc_field_illegal_regex[ds_type];
    if (illegal_regex.empty()) return wxEmptyString;
    
    wxString new_name = old_name;
    wxRegEx regex;
    regex.Compile(illegal_regex);
    regex.ReplaceAll(&new_name, "");
    
    if (new_name.size()==0) new_name = "NONAME";
    return new_name;
}

bool FieldNameCorrectionDlg::IsFieldNameValid(const wxString& col_name)
{
    if ( GdaConst::datasrc_field_lens.find(ds_type) ==
        GdaConst::datasrc_field_lens.end() )
    {
        // no valid entry in datasrc_field_lens, could be a unwritable ds
        return false;
    }
    
    int field_len = GdaConst::datasrc_field_lens[ds_type];
    if ( field_len < col_name.length() ) return false;
    
    if (GdaConst::datasrc_field_regex.find(ds_type) ==
        GdaConst::datasrc_field_regex.end() )
    {
        return true;
    }
    wxString field_regex = GdaConst::datasrc_field_regex[ds_type];
    if (field_regex.empty()) return true;
    
    wxRegEx regex;
    regex.Compile(field_regex);
    if ( regex.Matches(col_name) ) {
        return true;
    }
    return false;
}

void FieldNameCorrectionDlg::OnOkClick(wxCommandEvent& event)
{
    // check user input
    for ( int i=0; i<n_fields; i++) {
        wxTextCtrl* input = (wxTextCtrl*)wxWindow::FindWindowById(1000+i);
        wxString user_field_name = input->GetValue();
        if ( !IsFieldNameValid(user_field_name) ) {
            input->SetForegroundColour(*wxRED);
            return;
        } else {
            input->SetForegroundColour(*wxBLACK);
        }
    }
    event.Skip();
    EndDialog(wxID_OK);
}

void FieldNameCorrectionDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);	
}