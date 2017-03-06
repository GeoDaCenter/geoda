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

#include <boost/foreach.hpp>
#include <wx/textdlg.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../GdaConst.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/WebViewHelpWin.h"
#include "LowessParamDlg.h"

LowessParamFrame::LowessParamFrame(double f, int iter, double delta_factor,
                                   Project* project_)
: wxFrame((wxWindow*) 0, wxID_ANY, "LOWESS Smoother Parameters",
          wxDefaultPosition, wxSize(400, -1), wxDEFAULT_FRAME_STYLE),
LowessParamObservable(f, iter, delta_factor), 
project(project_)
{
	LOG_MSG("Entering LowessParamFrame::LowessParamFrame");
	
	wxPanel* panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);

	wxButton* help_btn = new wxButton(panel, 
                                      XRCID("ID_HELP_BTN"), 
                                      "Help",
                                      wxDefaultPosition, 
                                      wxDefaultSize,
                                      wxBU_EXACTFIT);
	wxButton* apply_btn = new wxButton(panel, 
                                       XRCID("ID_APPLY_BTN"), 
                                       "Apply",
                                       wxDefaultPosition, 
                                       wxDefaultSize,
                                       wxBU_EXACTFIT);
	wxButton* reset_defaults_btn = new wxButton(panel,
                                                XRCID("ID_RESET_DEFAULTS_BTN"),
                                                "Reset",
                                                wxDefaultPosition, 
                                                wxDefaultSize,
                                                wxBU_EXACTFIT);

	wxStaticText* f_stat_t = new wxStaticText(panel, wxID_ANY, "Bandwidth:");
	f_text = new wxTextCtrl(panel, 
                            XRCID("ID_F_TEXT"),
                            wxString::Format("%.2f", GetF()),
                            wxDefaultPosition, 
                            wxSize(100, -1),
                            wxTE_PROCESS_ENTER);
	f_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	Connect(XRCID("ID_F_TEXT"), wxEVT_TEXT,
            wxCommandEventHandler(LowessParamFrame::OnFTextChange));
    Connect(XRCID("ID_F_TEXT"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(LowessParamFrame::OnApplyBtn));

	wxStaticText* iter_stat_t = new wxStaticText(panel, wxID_ANY, "Iterations:");
	iter_text = new wxTextCtrl(panel, 
                               XRCID("ID_ITER_TEXT"),
                               wxString::Format("%d", GetIter()),
                               wxDefaultPosition, 
                               wxSize(100, -1),
                               wxTE_PROCESS_ENTER);
	iter_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	Connect(XRCID("ID_ITER_TEXT"), 
            wxEVT_TEXT,
            wxCommandEventHandler(LowessParamFrame::OnIterTextChange));
    Connect(XRCID("ID_ITER_TEXT"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(LowessParamFrame::OnApplyBtn));
	
	wxStaticText* delta_factor_stat_t =
		new wxStaticText(panel, wxID_ANY, "Delta Factor:");
	delta_factor_text = new wxTextCtrl(panel, 
                                       XRCID("ID_DELTA_FACTOR_TEXT"),
                                       wxString::Format("%.4f", GetDeltaFactor()),
                                       wxDefaultPosition, 
                                       wxSize(100, -1),
                                       wxTE_PROCESS_ENTER);
	delta_factor_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    
	Connect(XRCID("ID_DELTA_FACTOR_TEXT"), 
            wxEVT_TEXT,
            wxCommandEventHandler(LowessParamFrame::OnDeltaFactorTextChange));
    Connect(XRCID("ID_DELTA_FACTOR_TEXT"), wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(LowessParamFrame::OnApplyBtn));
	Connect(XRCID("ID_HELP_BTN"), 
            wxEVT_BUTTON,
            wxCommandEventHandler(LowessParamFrame::OnHelpBtn));
	Connect(XRCID("ID_APPLY_BTN"), 
            wxEVT_BUTTON,
            wxCommandEventHandler(LowessParamFrame::OnApplyBtn));
	Connect(XRCID("ID_RESET_DEFAULTS_BTN"), 
            wxEVT_BUTTON,
            wxCommandEventHandler(LowessParamFrame::OnResetDefaultsBtn));
		
	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
	
	wxFlexGridSizer* fg_sizer = new wxFlexGridSizer(3, 2, 3, 3);
	fg_sizer->Add(f_stat_t, 0, wxALIGN_CENTRE_VERTICAL | wxALIGN_RIGHT);
	fg_sizer->Add(f_text, 0, wxALIGN_CENTRE_VERTICAL);
	fg_sizer->Add(iter_stat_t, 0, wxALIGN_CENTRE_VERTICAL | wxALIGN_RIGHT);
	fg_sizer->Add(iter_text, 0, wxALIGN_CENTRE_VERTICAL);
	fg_sizer->Add(delta_factor_stat_t, 0, wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);
	fg_sizer->Add(delta_factor_text, 0, wxALIGN_CENTRE_VERTICAL);
	
	wxBoxSizer* btns_row1_h_szr = new wxBoxSizer(wxHORIZONTAL);
	btns_row1_h_szr->Add(help_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_row1_h_szr->AddSpacer(8);
	btns_row1_h_szr->Add(reset_defaults_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_row1_h_szr->AddSpacer(8);
	btns_row1_h_szr->Add(apply_btn, 0, wxALIGN_CENTER_VERTICAL);
		
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(fg_sizer, 0,
									 wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(2);
	
	panel_v_szr->Add(btns_row1_h_szr, 0,
									 wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer for Frame
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	SetSizerAndFit(top_h_sizer);
	
	Show(true);
	
	LOG_MSG("Exiting LowessParamFrame::LowessParamFrame");
}

LowessParamFrame::~LowessParamFrame()
{
	LOG_MSG("In LowessParamFrame::~LowessParamFrame");
	notifyObserversOfClosing();
}

void LowessParamFrame::OnHelpBtn(wxCommandEvent& ev)
{
	LOG_MSG("In LowessParamFrame::OnHelpBtn");
	WebViewHelpWin* win = new WebViewHelpWin(project, GetHelpPageHtml(), NULL,
                                             wxID_ANY, _("LOWESS Smoother Help"));
}

void LowessParamFrame::OnApplyBtn(wxCommandEvent& ev)
{
	LOG_MSG("In LowessParamFrame::OnApplyBtn");
	UpdateParamsFromFields();
	notifyObservers();
}

void LowessParamFrame::OnResetDefaultsBtn(wxCommandEvent& ev)
{
	LOG_MSG("In LowessParamFrame::OnResetDefaultsBtn");
	f = Lowess::default_f;
	iter = Lowess::default_iter;
	delta_factor = Lowess::default_delta_factor;
	f_text->ChangeValue(wxString::Format("%.2f", GetF()));
	iter_text->ChangeValue(wxString::Format("%d", GetIter()));
	delta_factor_text->ChangeValue(wxString::Format("%.4f", GetDeltaFactor()));
    
    OnApplyBtn(ev);
}

void LowessParamFrame::OnFTextChange(wxCommandEvent& ev)
{	
}

void LowessParamFrame::OnIterTextChange(wxCommandEvent& ev)
{
}

void LowessParamFrame::OnDeltaFactorTextChange(wxCommandEvent& ev)
{
}

void LowessParamFrame::closeAndDeleteWhenEmpty()
{
	Close(true);
}

void LowessParamFrame::UpdateParamsFromFields()
{
	LOG_MSG("Entering LowessParamFrame::UpdateParamsFromFields");
	Lowess temp_l(GetF(), GetIter(), GetDeltaFactor());
	{
		wxString s = f_text->GetValue();
		double v;
		if (s.ToDouble(&v)) 
			temp_l.SetF(v);
		f = temp_l.GetF();
		wxString sf = wxString::Format("%.2f", GetF());
		f_text->ChangeValue(sf);
	}
	{
		wxString s = iter_text->GetValue();
		long v;
		if (s.ToLong(&v)) 
			temp_l.SetIter((long) v);
		iter = temp_l.GetIter();
		wxString sf = wxString::Format("%d", GetIter());
		iter_text->ChangeValue(sf);
	}
	{
		wxString s = delta_factor_text->GetValue();
		double v;
		if (s.ToDouble(&v)) 
			temp_l.SetDeltaFactor(v);
		delta_factor = temp_l.GetDeltaFactor();
		wxString sf = wxString::Format("%.4f", GetDeltaFactor());
		delta_factor_text->ChangeValue(sf);
	}
	LOG_MSG("Exiting LowessParamFrame::UpdateParamsFromFields");
}

wxString LowessParamFrame::GetHelpPageHtml() const
{
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <style type=\"text/css\">";
	s << "  body {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    font-size: small;";
	s << "  }";
	s << "  h1 h2 {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    color: blue;";
	s << "  }";
	s <<   "</style>";
	s << "</head>";
	s << "<body>";
	
	s << "<h2>LOWESS Smoother Overview</h2>";
	s << "<p>";
	s << "The LOWESS smoother algorithm uses locally-weighted polynomial ";
	s << "regression.  This program is based on the function called ";
	s << "&ldquo;lowess&rdquo; in the R stats package.  An internet search ";
	s << "for &ldquo;lowess stats R&rdquo; will bring up the relevent ";
	s << "R manual page.  The parameters documented below are respectively ";
	s << "refered to as &ldquo;f&rdquo;, &ldquo;iter&rdquo; and ";
	s << "&ldquo;delta&rdquo; in the R lowess manual page.";
	s << "</p>";
	
	s << "<h2>Bandwidth</h2>";
	s << "<p>";
	s << "The proportion of points in the plot that ";
	s << "influence smoothing at each value. Larger values give ";
	s << "more smoothness.";
	s << "</p>";
	
	s << "<h2>Iterations</h2>";
	s << "<p>";
	s << "The number of &ldquo;robustifying&rdquo; iterations which ";
	s << "should be performed. ";
	s << "Using smaller values of iter will speed up the smoothing.";
	s << "</p>";
	
	s << "<h2>Delta Factor</h2>";
	s << "<p>";
	s << "Delta Factor is multiplied by the range of values ";
	s << "in the x-coordinates to produce the internal parameter delta. ";
	s << "Small values of delta ";
	s << "speed up computation: instead of computing the local polynomial ";
	s << "fit at each data point it is not computed for points within ";
	s << "delta of the last computed point, and linear interpolation is ";
	s << "used to fill in the fitted values for the skipped points.";
	s << "</p>";
	
	s << "</body>";
	s << "</html>";
	return s;
}
