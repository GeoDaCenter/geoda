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

#ifndef __GEODA_CENTER_3D_CONTROL_PAN_H__
#define __GEODA_CENTER_3D_CONTROL_PAN_H__

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/tooltip.h>

class C3DPlotFrame;

class C3DControlPan: public wxPanel
{    
    DECLARE_CLASS( C3DControlPan )
    DECLARE_EVENT_TABLE()

public:
    C3DControlPan( );
    C3DControlPan( wxWindow* parent, wxWindowID id = -1,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE,
				  const wxString& x3d_l = "X",
				  const wxString& y3d_l = "Y",
				  const wxString& z3d_l = "Z" );

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE,
				const wxString& x3d_l = "X",
				const wxString& y3d_l = "Y",
				const wxString& z3d_l = "Z" );

    void CreateControls();
	
	void UpdateAxesLabels(const wxString& x, const wxString& y,
						  const wxString& z);
    void OnCDatapointClick( wxCommandEvent& event );
    void OnCToxClick( wxCommandEvent& event );
    void OnCToyClick( wxCommandEvent& event );
    void OnCTozClick( wxCommandEvent& event );
    void OnCSelectClick( wxCommandEvent& event );
    void OnCSlxpUpdated( wxCommandEvent& event );
    void OnCSlxsUpdated( wxCommandEvent& event );
    void OnCSlypUpdated( wxCommandEvent& event );
    void OnCSlysUpdated( wxCommandEvent& event );
    void OnCSlzpUpdated( wxCommandEvent& event );
    void OnCSlzsUpdated( wxCommandEvent& event );

    wxCheckBox* m_data;
    wxCheckBox* m_prox;
    wxCheckBox* m_proy;
    wxCheckBox* m_proz;
    wxCheckBox* m_select;
	wxStaticText* m_static_text_x;
	wxStaticText* m_static_text_y;
	wxStaticText* m_static_text_z;
	wxString x3d_label;
	wxString y3d_label;
	wxString z3d_label;
    wxSlider* m_xp;
    wxSlider* m_xs;
    wxSlider* m_yp;
    wxSlider* m_ys;
    wxSlider* m_zp;
    wxSlider* m_zs;

	C3DPlotFrame* template_frame;
};

#endif

