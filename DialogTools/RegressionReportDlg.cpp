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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>
#include <wx/fontdlg.h>
#include "../GeneralWxUtils.h"
#include "RegressionReportDlg.h"


BEGIN_EVENT_TABLE( RegressionReportDlg, wxFrame)
    EVT_CLOSE( RegressionReportDlg::OnClose )
	EVT_MOUSE_EVENTS(RegressionReportDlg::OnMouseEvent)
	EVT_MENU(XRCID("ID_FONT"), RegressionReportDlg::OnFontChanged)
END_EVENT_TABLE()

RegressionReportDlg::RegressionReportDlg( )
{
}

RegressionReportDlg::RegressionReportDlg( wxWindow* parent,
										   wxString showText,
										   wxWindowID id,
										   const wxString& caption,
										   const wxPoint& pos,
										   const wxSize& size, long style )
:wxFrame(parent, id, caption, pos, size, style)
{
    wxLogMessage("Open RegressionReportDlg.");
	results = showText;
    //Create(parent, id, caption, pos, size, style);
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    CreateControls();
    Centre();
	m_textbox->AppendText(results);
}

bool RegressionReportDlg::Create( wxWindow* parent, wxWindowID id,
								  const wxString& caption,
								  const wxPoint& pos,
								  const wxSize& size, long style )
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    SetParent(parent);
    CreateControls();
    Centre();
	m_textbox->AppendText(results);
    return true;
}

void RegressionReportDlg::CreateControls()
{

    //wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_REGRESSION_REPORT");
    //m_textbox = XRCCTRL(*this, "ID_TEXTCTRL1", wxTextCtrl);
    wxPanel *panel = new wxPanel(this, -1);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    m_textbox = new wxTextCtrl(panel, XRCID("ID_TEXTCTRL"), "", wxDefaultPosition, wxSize(620,560), wxTE_MULTILINE | wxTE_READONLY);
    
    if (GeneralWxUtils::isWindows()) {
        wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
    } else {
        wxFont font(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
        
    }
    
    vbox->Add(m_textbox, 1, wxEXPAND|wxALL);
    panel->SetSizer(vbox);
   
    

    wxBitmap save = wxArtProvider::GetBitmap(wxART_FILE_SAVE);
    wxToolBar *toolbar = CreateToolBar();

    toolbar->AddTool(wxID_SAVE, _("Save Regression Results"), save);
    toolbar->Realize();
    Connect(wxID_SAVE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(RegressionReportDlg::OnSaveToFile));
    
    Center();
}
void RegressionReportDlg::AddNewReport(const wxString report)
{
    results = report + results;
	m_textbox->SetValue(results);
}

void RegressionReportDlg::SetReport(const wxString report)
{
    results = report;
    m_textbox->SetValue(results);
}

void RegressionReportDlg::OnMouseEvent(wxMouseEvent& event)
{
	if (event.RightDown())
		PopupMenu(wxXmlResource::Get()->
			LoadMenu("ID_REPORT_VIEW_MENU_CONTEXT"),
			event.GetPosition().x, event.GetPosition().y);
}

void RegressionReportDlg::OnSaveToFile(wxCommandEvent& event)
{
    wxLogMessage("In RegressionReportDlg::OnSaveToFile()");
    wxFileDialog dlg( this, "Regression Output Text File", wxEmptyString,
                     wxEmptyString,
                     "TXT files (*.txt)|*.txt",
                     wxFD_SAVE );
    if (dlg.ShowModal() != wxID_OK) return;
    
    wxFileName new_txt_fname(dlg.GetPath());
    wxString new_main_dir = new_txt_fname.GetPathWithSep();
    wxString new_main_name = new_txt_fname.GetName();
    wxString new_txt = new_main_dir + new_main_name + ".txt";
    
    // Prompt for overwrite permission
    if (wxFileExists(new_txt)) {
        wxString msg;
        msg << new_txt << " already exists.  OK to overwrite?";
        wxMessageDialog dlg (this, msg, "Overwrite?",
                             wxYES_NO | wxCANCEL | wxNO_DEFAULT);
        if (dlg.ShowModal() != wxID_YES) return;
    }
    
    bool failed = false;
    // Automatically overwrite existing csv since we have
    // permission to overwrite.
    
    if (wxFileExists(new_txt) && !wxRemoveFile(new_txt)) failed = true;
    
    if (!failed) {
        // write logReport to a text file
        wxFFileOutputStream output(new_txt);
        if (output.IsOk()) {
            wxTextOutputStream txt_out( output );
            txt_out << m_textbox->GetValue();
            txt_out.Flush();
            output.Close();
        } else {
            failed = true;
        }
    }
    
    if (failed) {
        wxString msg;
        msg << "Unable to overwrite " << new_txt;
        wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }
}

void RegressionReportDlg::OnFontChanged(wxCommandEvent& event)
{
    wxLogMessage("In RegressionReportDlg::OnFontChanged()");
	wxFontData data;
	wxFontDialog dlg(NULL, data);
	wxTextAttr attr;

	if (dlg.ShowModal() == wxID_OK)
	{
		data = dlg.GetFontData();
		m_textbox->GetStyle(0, attr);
		attr.SetFont(data.GetChosenFont());
		m_textbox->SetStyle(0, results.Length(), attr);
	}
}

void RegressionReportDlg::OnClose( wxCloseEvent& event )
{
    wxLogMessage("In RegressionReportDlg::OnClose()");
    Destroy();
    event.Skip();
}
