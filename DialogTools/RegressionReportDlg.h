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

#ifndef __GEODA_CENTER_REGRESSION_REPORT_DLG_H__
#define __GEODA_CENTER_REGRESSION_REPORT_DLG_H__

#include <wx/xrc/xmlres.h>

class RegressionReportDlg: public wxFrame
{    
    DECLARE_EVENT_TABLE()

public:
    RegressionReportDlg( );
    RegressionReportDlg( wxWindow* parent, wxString showText,
        wxWindowID id = wxID_ANY,
        const wxString& caption = _("Regression Report"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(680, 480),
        long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    bool Create( wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& caption = _("Regression Report"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(680, 480),
        long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    void CreateControls();

    void OnClose(wxCloseEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnFontChanged(wxCommandEvent& event);
	void OnSaveToFile(wxCommandEvent& event);
    
    void AddNewReport(const wxString report);
    void SetReport(const wxString report);

    wxTextCtrl* m_textbox;
    wxString results;
};

#endif
