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

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/regex.h>
#include <wx/xrc/xmlres.h>

#include "../GdaConst.h"
#include "FieldNameCorrectionDlg.h"


BEGIN_EVENT_TABLE( FieldNameCorrectionDlg, wxDialog )
	//EVT_BUTTON( XRCID("IDC_OPEN_IASC"), FieldNameCorrectionDlg::OnBrowseDSfileBtn)
	//EVT_CLOSE( FieldNameCorrectionDlg::OnClose )
	EVT_BUTTON( wxID_OK, FieldNameCorrectionDlg::OnOkClick )
	EVT_BUTTON( wxID_CANCEL, FieldNameCorrectionDlg::OnCancelClick )
END_EVENT_TABLE()


ScrolledWidgetsPane::ScrolledWidgetsPane(wxWindow* parent, wxWindowID id,
                                         GdaConst::DataSourceType ds_type,
                                         vector<wxString>& all_fname)
: wxScrolledWindow(parent, id, wxDefaultPosition, wxSize(700,300)), ds_type(ds_type), need_correction(false)
{
	vector<wxString> merged_field_names;
	set<wxString> bad_fnames, dup_fname, uniq_upper_fname;
   
    vector<int> bad_fname_idx_s, dup_fname_idx_s;
    
	for (int i=0; i < all_fname.size(); i++) {
		wxString field_name = all_fname[i];
        
        old_field_names.push_back(field_name);
        new_field_names.push_back(field_name);
        
        field_names_dict[field_name] = field_name;
        
        bool isValid = true;
		if (!IsFieldNameValid(field_name) ) {
			bad_fnames.insert(field_name);
            bad_fname_idx_s.push_back(i);
            isValid = false;
            
        } else if (uniq_upper_fname.find(field_name.Upper()) !=
                   uniq_upper_fname.end()) {
            dup_fname.insert(field_name);
            dup_fname_idx_s.push_back(i);
            isValid = false;
        }
        
        uniq_upper_fname.insert(field_name.Upper());
        if (isValid)
            merged_field_names.push_back(field_name);
	}
	if (!dup_fname_idx_s.empty() || !bad_fname_idx_s.empty()) {
		need_correction = true;
        //Init(merged_field_names, dup_fname, bad_fnames);
        Init(dup_fname_idx_s, bad_fname_idx_s);
    }
}

ScrolledWidgetsPane::ScrolledWidgetsPane(wxWindow* parent, wxWindowID id,
                                        GdaConst::DataSourceType ds_type,
                                        map<wxString, wxString>& fnames_dict,
                                        vector<wxString>& merged_field_names,
                                        set<wxString>& dup_fname,
                                        set<wxString>& bad_fname)
: wxScrolledWindow(parent, id, wxDefaultPosition, wxSize(700,300)),
field_names_dict(fnames_dict),
ds_type(ds_type),
merged_field_names(merged_field_names),
need_correction(true)
{
	Init(merged_field_names, dup_fname, bad_fname);
}


ScrolledWidgetsPane::~ScrolledWidgetsPane()
{
    for (size_t i=0, sz=txt_fname.size(); i<sz; i++) {
        delete txt_fname[i];
        delete txt_input[i];
        delete txt_info[i];
        delete input_info[i];
    }
}

void ScrolledWidgetsPane::Init(vector<int>& dup_fname_idx_s,
                               vector<int>& bad_fname_idx_s)
{
	// the sizer will take care of determining the needed scroll size
	// (if you don't use sizers you will need to manually set the viewport size)
   
    // build a dict for searching duplicated field
    for (size_t i=0; i<old_field_names.size(); i++)
    {
        wxString old_name = old_field_names[i];
        field_dict[old_name] = true;
    }
    
	n_fields = dup_fname_idx_s.size() + bad_fname_idx_s.size();
	int nrow = n_fields + 2; // for buttons
	int ncol = 4;
    
	wxString warn_msg;
	wxString DUP_WARN = _("(Duplicate field name)");
	wxString INV_WARN = _("(Field name is not valid)");
	
	wxFlexGridSizer* sizer = new wxFlexGridSizer(nrow, ncol, 10, 0);
	
	// add a series of widgets
    SetBackgroundColour(*wxWHITE);
	
	txt_fname.clear(); // ID_FNAME_STAT_TXT_BASE
	txt_input.clear(); // ID_INPUT_TXT_CTRL_BASE
	txt_info.clear(); // ID_INFO_STAT_TXT_BASE
    input_info.clear();
	
	// titile
	wxStaticText* txt_oname=new wxStaticText(this, wxID_ANY, _("Field name:"));
	sizer->Add(txt_oname, 0, wxALIGN_LEFT);
    
	wxStaticText* txt_orest=new wxStaticText(this, wxID_ANY, wxEmptyString);
	sizer->Add(txt_orest, 0);
    
	wxStaticText* txt_newname=new wxStaticText(this,wxID_ANY, _("Suggest field name:"));
	sizer->Add(txt_newname, 0, wxALIGN_LEFT|wxLEFT, 10);
    
	wxStaticText* txt_input_info =new wxStaticText(this, wxID_ANY, wxEmptyString);
	sizer->Add(txt_input_info, 0);

    size_t ctrl_cnt = 0;

    for (size_t i=0; i < dup_fname_idx_s.size(); i++)
    {
        int fname_idx = dup_fname_idx_s[i];
        wxString field_name = old_field_names[fname_idx];
        warn_msg=DUP_WARN;
        
        wxString id_name1, id_name2, id_name3, id_name4;
        id_name1 << "ID_FNAME_STAT_TXT_BASE" << ctrl_cnt;
        id_name2 << "ID_INPUT_TXT_CTRL_BASE" << ctrl_cnt;
        id_name3 << "ID_INFO_STAT_TXT_BASE" << ctrl_cnt;
        id_name4 << "ID_INPUT_INFO_STAT_TXT_BASE" << ctrl_cnt;
        
        wxString user_field_name = GetSuggestFieldName(fname_idx);
        txt_fname.push_back(new wxStaticText(this, XRCID(id_name1), field_name));
        sizer->Add(txt_fname[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
        
        txt_info.push_back(new wxStaticText(this, XRCID(id_name3), warn_msg));
        sizer->Add(txt_info[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
       
        wxTextCtrl* user_input = new wxTextCtrl(this, XRCID(id_name2), user_field_name, wxDefaultPosition,wxSize(240,-1),wxTE_PROCESS_ENTER);
        user_input->Bind(wxEVT_TEXT_ENTER, &ScrolledWidgetsPane::OnUserInput, this);
        txt_input.push_back(user_input);
        sizer->Add(txt_input[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 10);
        
        input_info.push_back(new wxStaticText(this, XRCID(id_name4), wxEmptyString));
        sizer->Add(input_info[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 2);
        
        ++ctrl_cnt;
    }
    
    for (size_t i=0; i < bad_fname_idx_s.size(); i++)
    {
        int fname_idx = bad_fname_idx_s[i];
        wxString field_name = old_field_names[fname_idx];
        warn_msg=INV_WARN;
        
        wxString id_name1, id_name2, id_name3, id_name4;
        id_name1 << "ID_FNAME_STAT_TXT_BASE" << ctrl_cnt;
        id_name2 << "ID_INPUT_TXT_CTRL_BASE" << ctrl_cnt;
        id_name3 << "ID_INFO_STAT_TXT_BASE" << ctrl_cnt;
        id_name4 << "ID_INPUT_INFO_STAT_TXT_BASE" << ctrl_cnt;
        
        wxString user_field_name = GetSuggestFieldName(fname_idx);
        txt_fname.push_back(new wxStaticText(this, XRCID(id_name1), field_name));
        sizer->Add(txt_fname[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
        
        txt_info.push_back(new wxStaticText(this, XRCID(id_name3), warn_msg));
        sizer->Add(txt_info[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
       
        wxTextCtrl* user_input = new wxTextCtrl(this, XRCID(id_name2), user_field_name, wxDefaultPosition,wxSize(240,-1),wxTE_PROCESS_ENTER);
        user_input->Bind(wxEVT_TEXT_ENTER, &ScrolledWidgetsPane::OnUserInput, this);
        txt_input.push_back(user_input);
        sizer->Add(txt_input[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 10);
        
        input_info.push_back(new wxStaticText(this, XRCID(id_name4), wxEmptyString));
        sizer->Add(input_info[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 2);
        
        ++ctrl_cnt;
    }
	
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(sizer, 1, wxEXPAND|wxALL, 15);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
	// this part makes the scrollbars show up
	this->FitInside(); // ask the sizer about the needed size
	this->SetScrollRate(5, 5);

    Centre();
}

void ScrolledWidgetsPane::Init(vector<wxString>& merged_field_names,
                                 set<wxString>& dup_fname, 
                                 set<wxString>& bad_fname)
{
	// the sizer will take care of determining the needed scroll size
	// (if you don't use sizers you will need to manually set the viewport size)
	
   
    // build a dict for searching duplicated field
    map<wxString,wxString>::iterator iter;
    // update changed fields
    for (iter = field_names_dict.begin();
         iter != field_names_dict.end(); ++iter )
    {
        wxString old_name = iter->first;
        field_dict[old_name] = true;
    }
    
	n_fields = dup_fname.size() + bad_fname.size();
	int nrow = n_fields + 2;
	int ncol = 4;
	wxString warn_msg;
	wxString DUP_WARN = _("(Duplicate field name)");
	wxString INV_WARN = _("(Field name is not valid)");
	
	wxFlexGridSizer* sizer = new wxFlexGridSizer(nrow, ncol, 10, 0);
	
	// add a series of widgets
	num_controls = merged_field_names.size();
	
	txt_fname.clear(); // ID_FNAME_STAT_TXT_BASE
	txt_input.clear(); // ID_INPUT_TXT_CTRL_BASE
	txt_info.clear(); // ID_INFO_STAT_TXT_BASE
    input_info.clear();
	
	// titile
	wxStaticText* txt_oname=new wxStaticText(this, wxID_ANY, "Field name:");
	sizer->Add(txt_oname, 0, wxALIGN_LEFT);
    
	wxStaticText* txt_orest=new wxStaticText(this, wxID_ANY, "");
	sizer->Add(txt_orest, 0);

	wxStaticText* txt_newname=new wxStaticText(this,wxID_ANY,"Suggest field name:");
	sizer->Add(txt_newname, 0, wxALIGN_LEFT, 15);
   
    wxStaticText* txt_input_info =new wxStaticText(this, wxID_ANY, wxEmptyString);
    sizer->Add(txt_input_info, 0);
    
    size_t ctrl_cnt = 0;

    for (set<wxString>::iterator it=dup_fname.begin();
         it != dup_fname.end(); ++it ) {
        wxString field_name = *it;
        warn_msg=DUP_WARN;
        
        wxString id_name1, id_name2, id_name3, id_name4;
        id_name1 << "ID_FNAME_STAT_TXT_BASE" << ctrl_cnt;
        id_name2 << "ID_INPUT_TXT_CTRL_BASE" << ctrl_cnt;
        id_name3 << "ID_INFO_STAT_TXT_BASE" << ctrl_cnt;
        id_name4 << "ID_INPUT_INFO_STAT_TXT_BASE" << ctrl_cnt;
        
        wxString user_field_name = GetSuggestFieldName(field_name);
        txt_fname.push_back(new wxStaticText(this, XRCID(id_name1), field_name));
        sizer->Add(txt_fname[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
        
        txt_info.push_back(new wxStaticText(this, XRCID(id_name3), warn_msg));
        sizer->Add(txt_info[ctrl_cnt], 0, wxALIGN_LEFT|wxALL);
        
        wxTextCtrl* user_input = new wxTextCtrl(this, XRCID(id_name2), user_field_name, wxDefaultPosition,wxSize(240,-1),wxTE_PROCESS_ENTER);
        user_input->Bind(wxEVT_TEXT_ENTER, &ScrolledWidgetsPane::OnUserInput, this);
        txt_input.push_back(user_input);
        sizer->Add(txt_input[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 10);
        
        input_info.push_back(new wxStaticText(this, XRCID(id_name4), wxEmptyString));
        sizer->Add(input_info[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 2);

        ++ctrl_cnt;
    }
    
    for (set<wxString>::iterator it=bad_fname.begin();
         it != bad_fname.end(); ++it ) {
        wxString field_name = *it;
        warn_msg=INV_WARN;
        
        wxString id_name1, id_name2, id_name3, id_name4;
        id_name1 << "ID_FNAME_STAT_TXT_BASE" << ctrl_cnt;
        id_name2 << "ID_INPUT_TXT_CTRL_BASE" << ctrl_cnt;
        id_name3 << "ID_INFO_STAT_TXT_BASE" << ctrl_cnt;
        id_name4 << "ID_INPUT_INFO_STAT_TXT_BASE" << ctrl_cnt;
        
        wxString user_field_name = GetSuggestFieldName(field_name);
        txt_fname.push_back(new wxStaticText(this, XRCID(id_name1), field_name));
        sizer->Add(txt_fname[ctrl_cnt], 0, wxALIGN_LEFT|wxALL, 0);
        
        txt_info.push_back(new wxStaticText(this, XRCID(id_name3), warn_msg));
        sizer->Add(txt_info[ctrl_cnt], 0, wxALIGN_LEFT|wxALL, 0);
        
        wxTextCtrl* user_input = new wxTextCtrl(this, XRCID(id_name2), user_field_name, wxDefaultPosition,wxSize(240,-1),wxTE_PROCESS_ENTER);
        user_input->Bind(wxEVT_TEXT_ENTER, &ScrolledWidgetsPane::OnUserInput, this);
        txt_input.push_back(user_input);
        sizer->Add(txt_input[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 10);
        
        input_info.push_back(new wxStaticText(this, XRCID(id_name4), wxEmptyString));
        sizer->Add(input_info[ctrl_cnt], 0, wxALIGN_LEFT|wxLEFT, 2);
        
        ++ctrl_cnt;
    }
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(sizer, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    // this part makes the scrollbars show up
    this->FitInside(); // ask the sizer about the needed size
    this->SetScrollRate(5, 5);
    
    Centre();
}

// give suggested field name based on GdaConst::DataSourceType
wxString ScrolledWidgetsPane::GetSuggestFieldName(int field_idx)
{
    wxString old_name = old_field_names[field_idx];
	wxString new_name(old_name);
	
	// if illegal name
	new_name = RemoveIllegalChars(new_name);
	
	// if name too long
	new_name = TruncateFieldName(new_name);
	
	// check duplicate name
    new_name = RenameDupFieldName(new_name);

    // put it to new field name list
    new_field_names[field_idx] = new_name;
	
    // add any new suggest name to search dict
    field_dict[new_name] = true;
    
	return new_name;
}

wxString ScrolledWidgetsPane::GetSuggestFieldName(const wxString& old_name)
{
	wxString new_name(old_name);
	
	// if illegal name
	new_name = RemoveIllegalChars(new_name);
	
	// if name too long
	new_name = TruncateFieldName(new_name);
	
	// check duplicate name
    new_name = RenameDupFieldName(new_name);
	
	field_names_dict[old_name] = new_name;
	
    // add any new suggest name to search dict
    field_dict[new_name] = true;
    
	return new_name;
}

wxString ScrolledWidgetsPane::TruncateFieldName(const wxString& old_name,
                                                   int max_len)
{
    wxLogMessage("ScrolledWidgetsPane::TruncateFieldName");
    wxLogMessage(old_name);
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
	
    wxLogMessage(new_name);
	return new_name;
}

wxString ScrolledWidgetsPane::RemoveIllegalChars(const wxString& old_name)
{
    wxLogMessage("ScrolledWidgetsPane::RemoveIllegalChars");
    wxLogMessage(old_name);
	if (GdaConst::datasrc_field_illegal_regex.find(ds_type) ==
			GdaConst::datasrc_field_illegal_regex.end())
	{
		return old_name;
	}
	wxString illegal_regex = GdaConst::datasrc_field_illegal_regex[ds_type];
	if (illegal_regex.empty()) return old_name;
	
	wxString new_name = old_name;
	wxRegEx regex;
	regex.Compile(illegal_regex);
	regex.ReplaceAll(&new_name, "");
	
	if (new_name.size()==0) new_name = "NONAME";
    
    wxLogMessage(new_name);
	return new_name;
}


wxString ScrolledWidgetsPane::RenameDupFieldName(const wxString& old_name)
{
    wxLogMessage("ScrolledWidgetsPane::RenameDupFieldName");
    wxLogMessage(old_name);
    
	wxString new_name(old_name);
    while (field_dict.find(new_name) != field_dict.end() ||
           field_dict.find(new_name.Upper()) != field_dict.end() ||
           field_dict.find(new_name.Lower()) != field_dict.end()) {
        
    	// duplicated name, try to append suffix in the form "_x"
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
    }
    wxLogMessage(new_name);
    return new_name;
}

bool ScrolledWidgetsPane::IsFieldNameValid(const wxString& col_name)
{
    wxLogMessage("ScrolledWidgetsPane::IsFieldNameValid");
    wxLogMessage(col_name);
    
	if ( GdaConst::datasrc_field_lens.find(ds_type) ==
			GdaConst::datasrc_field_lens.end() )
	{
        wxLogMessage("Error:no valid entry in datasrc_field_lens, could be a unwritable ds");
		return false;
	}
	
	int field_len = GdaConst::datasrc_field_lens[ds_type];
	if ( field_len == 0 || field_len < col_name.length() )
        return false;
	
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

void ScrolledWidgetsPane::OnUserInput(wxCommandEvent& ev)
{
    CheckUserInput();
}

bool ScrolledWidgetsPane::CheckUserInput()
{
    wxString str_dup_field = _("Input is duplicated.");
    wxString str_invalid_field = _("Input is not valid.");
    
    // reset
    map<wxString, int> current_inputs;
	for ( size_t i=0, sz=txt_input.size(); i<sz; ++i) {
		if (txt_input[i]) {
            txt_input[i]->SetForegroundColour(*wxBLACK);
            input_info[i]->SetLabel(wxEmptyString);
            wxString user_field_name = txt_input[i]->GetValue();
            user_field_name.Trim();
            current_inputs[user_field_name]++;;
        }
    }
    
    bool _success = true;
   
    for ( size_t i=0, sz=txt_input.size(); i<sz; ++i) {
        bool success = true;
        if (txt_input[i]) {
            wxString user_field_name = txt_input[i]->GetValue();
            user_field_name.Trim();
            if (current_inputs[user_field_name]>1) {
                // re-check if any input itself is duplicated
                txt_input[i]->SetForegroundColour(*wxRED);
                input_info[i]->SetLabel(str_dup_field);
                success = false;
            } else if ( !IsFieldNameValid(user_field_name) ) {
                 // re-check if input is valid
                txt_input[i]->SetForegroundColour(*wxRED);
                input_info[i]->SetLabel(str_invalid_field);
                success = false;
            } else {
                // re-check if any input is duplicated with what's in table
                if (field_names_dict.find(user_field_name) != field_names_dict.end()) {
                    txt_input[i]->SetForegroundColour(*wxRED);
                    input_info[i]->SetLabel(str_dup_field);
                    success = false;
                }
            }
            if (success) {
                wxString old_name = txt_fname[i]->GetLabel();
                if (field_names_dict.find(old_name) != field_names_dict.end()) {
                    field_names_dict[old_name] = user_field_name;
                    new_field_names[i] = user_field_name;
                }
            }
        } else {
            // input is empty
            txt_input[i]->SetForegroundColour(*wxRED);
            input_info[i]->SetLabel(str_invalid_field);
            success = false;
        }
        if (!success) _success = success;
    }
    
    return _success;
}


map<wxString, wxString> ScrolledWidgetsPane::GetMergedFieldNameDict()
{
	return field_names_dict;
}

vector<wxString> ScrolledWidgetsPane::GetNewFieldNames()
{
    return new_field_names;
}

////////////////////////////////////////////////////////////////////////////////
//
// Main FieldNameCorrectionDlg
//
////////////////////////////////////////////////////////////////////////////////

FieldNameCorrectionDlg::
FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                       vector<wxString>& all_fname,
                       wxString title)
: wxDialog(NULL, -1, title, wxDefaultPosition, wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxPanel *panel = new wxPanel(this);
    
    // panel
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    fieldPane = new ScrolledWidgetsPane(panel, wxID_ANY,
                                        ds_type,
                                        all_fname);
    need_correction = fieldPane->need_correction;
    
    // buttons
    wxButton* ok_btn = new wxButton(panel, wxID_OK, _("OK"));
    wxButton* exit_btn = new wxButton(panel, wxID_CANCEL, _("Cancel"));
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(ok_btn, 1, wxALIGN_CENTER|wxALL,15);
    btnSizer->Add(exit_btn, 1, wxALIGN_CENTER|wxALL,15);
    
    vbox->Add(fieldPane, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox,1, wxEXPAND|wxALL, 0);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    
    Centre();
}

FieldNameCorrectionDlg::
FieldNameCorrectionDlg(GdaConst::DataSourceType ds_type,
                       map<wxString, wxString>& fnames_dict,
                       vector<wxString>& merged_field_names,
                       set<wxString>& dup_fname,
                       set<wxString>& bad_fname,
                       wxString title)
: wxDialog(NULL, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open FieldNameCorrectionDlg:");
    
    wxPanel *panel = new wxPanel(this);
    
    // panel
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	fieldPane = new ScrolledWidgetsPane(panel, wxID_ANY,
                                        ds_type,
                                        fnames_dict,
                                        merged_field_names,
                                        dup_fname,
                                        bad_fname);
    
    // buttons
    wxButton* ok_btn = new wxButton(panel, wxID_OK, _("OK"));
    wxButton* exit_btn = new wxButton(panel, wxID_CANCEL, _("Cancel"));
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(ok_btn, 1, wxALIGN_CENTER|wxALL,15);
    btnSizer->Add(exit_btn, 1, wxALIGN_CENTER|wxALL,15);
    
    vbox->Add(fieldPane, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 10);

    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox,1, wxEXPAND|wxALL, 0);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    
    Centre();
}

FieldNameCorrectionDlg::~FieldNameCorrectionDlg()
{
    wxLogMessage("Exit FieldNameCorrectionDlg");
    if (fieldPane) {
        delete fieldPane;
        fieldPane = 0;
    }
}


void FieldNameCorrectionDlg::OnClose(wxCloseEvent& ev)
{
    wxLogMessage("Click FieldNameCorrectionDlg::OnClose");
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
	ev.Skip();
	
}

void FieldNameCorrectionDlg::OnOkClick(wxCommandEvent& event)
{
	wxLogMessage("Click FieldNameCorrectionDlg::OnOkClick");
	// check user input
    if (fieldPane->CheckUserInput()) {
        EndDialog(wxID_OK);
        event.Skip();
    }
}

void FieldNameCorrectionDlg::OnCancelClick( wxCommandEvent& event )
{
    wxLogMessage("Click FieldNameCorrectionDlg::OnCancelClick");
	event.Skip();
	EndDialog(wxID_CANCEL);	
}







