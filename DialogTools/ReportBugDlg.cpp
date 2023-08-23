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
#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
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
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/notebook.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include "curl/curl.h"
#include "ogrsf_frmts.h"
#include "cpl_conv.h"

#include "../HLStateInt.h"
#include "../HighlightStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "../GdaJson.h"
#include "../logger.h"
#include "ReportBugDlg.h"

using namespace GdaJson;


IMPLEMENT_CLASS(WelcomeSelectionStyleDlg, wxDialog)

BEGIN_EVENT_TABLE(WelcomeSelectionStyleDlg, wxDialog)
END_EVENT_TABLE()

WelcomeSelectionStyleDlg::WelcomeSelectionStyleDlg(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
{
	SetParent(parent);
	SetPosition(pos);

	wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_WELCOME_SELECTION_STYLE");

	wxStaticBitmap* m_sel_style1 = XRCCTRL(*this, "IDC_SELECTION_STYLE1", wxStaticBitmap);
	wxStaticBitmap* m_sel_style2 = XRCCTRL(*this, "IDC_SELECTION_STYLE2", wxStaticBitmap);

	m_sel_style1->Bind(wxEVT_LEFT_DOWN, &WelcomeSelectionStyleDlg::OnStyle1, this);
	m_sel_style2->Bind(wxEVT_LEFT_DOWN, &WelcomeSelectionStyleDlg::OnStyle2, this);

	Centre();
	Move(pos);

	ShowModal();

	Destroy();
}

void WelcomeSelectionStyleDlg::OnStyle1(wxMouseEvent& ev)
{
	GdaConst::use_cross_hatching = false;
	OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "0");
	EndDialog(wxID_OK);
}

void WelcomeSelectionStyleDlg::OnStyle2(wxMouseEvent& ev)
{
	GdaConst::use_cross_hatching = true;
	OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "1");
	EndDialog(wxID_OK);
}


////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
ReportResultDlg::ReportResultDlg(wxWindow* parent, wxString issue_url,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size)
{
	wxPanel* panel = new wxPanel(this);
    
    if (!wxSystemSettings::GetAppearance().IsDark()) {
        panel->SetBackgroundColour(*wxWHITE);
    }
    
	wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);

	wxString result_tip = _("Thank you for helping us improve GeoDa with your bug report! \n\nYou can track our response and add screenshots or details here (or email us at spatial@uchicago.edu):");

	wxStaticText* lbl_tip = new wxStaticText(panel, wxID_ANY, result_tip);
	m_hyperlink1 = new wxHyperlinkCtrl(panel, wxID_ANY, issue_url,
		issue_url);
	bSizer->Add(lbl_tip, 1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	bSizer->Add(m_hyperlink1, 1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT, 10);

	panel->SetSizerAndFit(bSizer);

	wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
	sizerAll->Add(panel, 1, wxEXPAND | wxALL);
	SetSizer(sizerAll);
	SetAutoLayout(true);
	Centre(wxBOTH);
}

ReportResultDlg::~ReportResultDlg()
{
}


size_t write_to_string_(void *ptr, size_t size, size_t count, void *stream) {
	((std::string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

std::string CreateIssueOnGithub(wxString& post_data)
{
	std::vector<wxString> tester_ids = OGRDataAdapter::GetInstance().GetHistory("tester_id");
	if (tester_ids.empty()) return "";

	wxString tester_id = tester_ids[0];
	const char url[] = "https://api.github.com/repos/lixun910/geoda/issues";
	wxString header_auth = "Authorization: token " + tester_id;
	wxString header_user_agent = "User-Agent: GeoDaTester";

    std::string response;
	CURL* curl = curl_easy_init();
	CURLcode res;
	if (curl) {
		struct curl_slist *chunk = NULL;
		chunk = curl_slist_append(chunk, "Accept: application/vnd.github.v3+json");
		chunk = curl_slist_append(chunk, header_auth.c_str());
		chunk = curl_slist_append(chunk, header_user_agent.c_str());
		// set our custom set of headers
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                         (const char*)post_data.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string_);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		/* free the custom headers */
		curl_slist_free_all(chunk);
	}

	return response;
}



ReportBugDlg::ReportBugDlg(wxWindow* parent, wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size)
{
	wxLogMessage("Open ReportBugDlg.");
	//
	// Description: please briefly describe what went wrong
	// Steps you took before something went wrong (Optional):
	// Data you used (Optional): __________________________
	//
	// Create controls UI
	wxPanel* panel = new wxPanel(this);
	
    if (!wxSystemSettings::GetAppearance().IsDark()) {
        panel->SetBackgroundColour(*wxWHITE);
    }

	desc_tip = _("[Please briefly describe what went wrong]");
	steps_txt = _("[Steps you took before something went wrong]");

	title_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, desc_tip);
	steps_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, steps_txt,
		wxDefaultPosition,
		wxSize(500, 200),
		wxTE_MULTILINE);

	//wxString data_txt = _("Share part of your data for troubleshooting? (first 10 records)");
	//wxCheckBox* data_chk = new wxCheckBox(panel, wxID_ANY, data_txt);

	wxString user_tip = _("Your Github account (Optional):");
	wxStaticText* lbl_user = new wxStaticText(panel, wxID_ANY, user_tip);
	user_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
		wxDefaultPosition, wxSize(150, -1));
	wxBoxSizer* user_box = new wxBoxSizer(wxHORIZONTAL);
	user_box->Add(lbl_user);
	user_box->Add(user_txt_ctrl);

	wxString email_tip = _("Your Email address (Optional):");
	wxStaticText* lbl_email = new wxStaticText(panel, wxID_ANY, email_tip);
	email_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
		wxDefaultPosition, wxSize(150, -1));
	wxBoxSizer* email_box = new wxBoxSizer(wxHORIZONTAL);
	email_box->Add(lbl_email);
	email_box->AddSpacer(10);
	email_box->Add(email_txt_ctrl);

	// buttons
	wxButton* btn_cancel = new wxButton(panel, wxID_CANCEL, _("Cancel"),
		wxDefaultPosition,
		wxDefaultSize, wxBU_EXACTFIT);
	wxButton* btn_submit = new wxButton(panel, wxID_ANY, _("Submit Bug Report"),
		wxDefaultPosition,
		wxDefaultSize, wxBU_EXACTFIT);

	wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
	btn_box->Add(btn_cancel, 1, wxALIGN_CENTER | wxALL, 10);
	btn_box->Add(btn_submit, 1, wxALIGN_CENTER | wxALL, 10);

	wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
	box->Add(title_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(steps_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	//box->Add(data_chk, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
	box->Add(user_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(email_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	box->Add(btn_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);

	panel->SetSizerAndFit(box);

	wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
	sizerAll->Add(panel, 1, wxEXPAND | wxALL);
	SetSizer(sizerAll);
	SetAutoLayout(true);

	btn_submit->Bind(wxEVT_BUTTON, &ReportBugDlg::OnOkClick, this);


	SetParent(parent);
	SetPosition(pos);
	Centre();
}

ReportBugDlg::~ReportBugDlg()
{

}

void ReportBugDlg::OnOkClick(wxCommandEvent& event)
{
	wxLogMessage("In ReportBugDlg::OnOkClick()");
	//wxString rst = CreateIssueOnGithub("{\"title\": \"Test reporting bug from GeoDa software\", \"body\": \"We should have one\"}");

	wxString title = title_txt_ctrl->GetValue();
	wxString body = steps_txt_ctrl->GetValue();
	wxString user_github = user_txt_ctrl->GetValue();
	wxString email = email_txt_ctrl->GetValue();

	if (title.IsEmpty() || title == desc_tip) {
		wxMessageDialog msgDlg(this,
			_("Please briefly describe what went wrong."),
			_("Input is required"),
			wxOK | wxICON_INFORMATION);
		msgDlg.ShowModal();
		return;
	}
	if (body.IsEmpty() || body == steps_txt) {
		wxMessageDialog msgDlg(this,
			_("Please describe steps you took before something went wrong."),
			_("Input is required"),
			wxOK | wxICON_INFORMATION);
		msgDlg.ShowModal();
		return;
	}

	if (!user_github.IsEmpty()) {
		body << "\\n\\n@" << user_github << " " << email;
	}

	body << "\\n\\n";

	bool result = CreateIssue(title, body);

	if (result) {
		EndDialog(wxID_OK);
		return;
	}

	wxMessageDialog msgDlg(this,
		_("Oops. GeoDa was unable to submit a bug report. Please try again or create it here instead: https://github.com/GeoDaCenter/geoda/issues Thanks!"),
		_("Submit Bug Error"),
		wxOK | wxICON_INFORMATION);
	msgDlg.ShowModal();

	wxLogMessage("Submit Bug Report Error:");
	//wxLogMessage("title:");
	//wxLogMessage("%s", title);
}

void ReportBugDlg::OnCancelClick(wxCommandEvent& event)
{

}

bool ReportBugDlg::CreateIssue(wxString title, wxString body)
{
	body.Replace("\n", "\\n");
	// get log file to body
	wxString logger_path = GenUtils::GetLoggerPath();
    
	body << "\\n";
    
	wxTextFile tfile;
    if (tfile.Open(logger_path)) {
    	while (!tfile.Eof()) {
    		body << tfile.GetNextLine() << "\\n";
    	}
    }

	body.Replace("\"", "'");
	body.Replace("\t", "");
	body.Replace("\r", "");

	wxString labels = "[\"AutoBugReport\"]";
	//wxString assignees = "[\"GeoDaTester\"]";

	//wxString msg_templ = "{\"title\": \"%s\", \"body\": \"%s\", \"labels\": %s}";
	wxString json_msg = "{\"title\": \"";
	json_msg << title;
	json_msg << "\", \"body\": \"";
	json_msg << body;
	json_msg << "\", \"labels\": ";
	json_msg << labels;
	json_msg << "}";

    std::string result = CreateIssueOnGithub(json_msg);
	// parse results
	if (!result.empty()) {
		json_spirit::Value v;
		try {
			if (!json_spirit::read(result, v)) {
				throw std::runtime_error("Could not parse title as JSON");
			}
			json_spirit::Value url_val;
			if (!GdaJson::findValue(v, url_val, "html_url")) {
				throw std::runtime_error("could not find url");
			}
			wxString url_issue = url_val.get_str();
			ReportResultDlg dlg(NULL, url_issue);
			dlg.ShowModal();
			return true;
		} catch (std::runtime_error e) {
			wxString msg;
			msg << "JSON parsing failed: ";
			msg << e.what();
		}
	}
	return false;
}
