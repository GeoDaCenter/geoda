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

#ifndef __GEODA_CENTER_SHP_2_ASC_DLG_H__
#define __GEODA_CENTER_SHP_2_ASC_DLG_H__

#include "../ShapeOperations/OGRDatasourceProxy.h"
#include "../ShapeOperations/OGRLayerProxy.h"

class SHP2ASCDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    SHP2ASCDlg( );
    SHP2ASCDlg( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Exporting Shape to Boundary"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
    ~SHP2ASCDlg();

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Exporting Shape to Boundary"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    bool CreateASCBoundary(wxString oasc, wxString orasc, int field,
                           int type, bool isR);
    void CreateControls();

    void OnOkAddClick( wxCommandEvent& event );
    void OnCOpenOascClick( wxCommandEvent& event );
    void OnOkdoneClick( wxCommandEvent& event );
    void OnCRadio1Selected( wxCommandEvent& event );
    void OnCRadio2Selected( wxCommandEvent& event );
    void OnCRadio3Selected( wxCommandEvent& event );
    void OnCRadio4Selected( wxCommandEvent& event );
    void OnCOpenIshpClick( wxCommandEvent& event );

private:
    wxTextCtrl* m_inputfile;
    wxTextCtrl* m_outputfile;
    wxChoice* m_X;
    wxCheckBox* m_check;
    wxRadioButton* m_ra1;
    wxRadioButton* m_ra1a;
    wxRadioButton* m_ra2;
    wxRadioButton* m_ra2a;

	int type;
	wxString fn;
    
    OGRDatasourceProxy* ogr_ds;
    OGRLayerProxy* ogr_layer;
};

#endif
