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

#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GeneralWxUtils.h"

#include "../Explore/3DPlotView.h"
#include "3DControlPan.h"

IMPLEMENT_CLASS( C3DControlPan, wxPanel )

BEGIN_EVENT_TABLE( C3DControlPan, wxPanel )

    EVT_CHECKBOX( XRCID("IDC_DATAPOINT"), C3DControlPan::OnCDatapointClick )
    EVT_CHECKBOX( XRCID("IDC_TOX"), C3DControlPan::OnCToxClick )
    EVT_CHECKBOX( XRCID("IDC_TOY"), C3DControlPan::OnCToyClick )
    EVT_CHECKBOX( XRCID("IDC_TOZ"), C3DControlPan::OnCTozClick )
    EVT_CHECKBOX( XRCID("IDC_SELECT"), C3DControlPan::OnCSelectClick )

	EVT_SLIDER( XRCID("IDC_SLXP"), C3DControlPan::OnCSlxpUpdated )
    EVT_SLIDER( XRCID("IDC_SLXS"), C3DControlPan::OnCSlxsUpdated )

	EVT_SLIDER( XRCID("IDC_SLYP"), C3DControlPan::OnCSlypUpdated )
    EVT_SLIDER( XRCID("IDC_SLYS"), C3DControlPan::OnCSlysUpdated )

	EVT_SLIDER( XRCID("IDC_SLZP"), C3DControlPan::OnCSlzpUpdated )
    EVT_SLIDER( XRCID("IDC_SLZS"), C3DControlPan::OnCSlzsUpdated )

END_EVENT_TABLE()

C3DControlPan::C3DControlPan( )
{
}

C3DControlPan::C3DControlPan( wxWindow* parent,
							 wxWindowID id,
							 const wxPoint& pos,
							 const wxSize& size,
							 long style,
							 const wxString& x3d_l,
							 const wxString& y3d_l,
							 const wxString& z3d_l )
{
	Create(parent, id, pos, size, style, x3d_l, y3d_l, z3d_l);

	m_xp->SetRange(1,20000);
	m_xp->SetValue(10000);	
	m_xs->SetRange(1,10000);
	m_xs->SetValue(1000);	

	m_yp->SetRange(1,20000);
	m_yp->SetValue(10000);	
	m_ys->SetRange(1,10000);
	m_ys->SetValue(1000);	

	m_zp->SetRange(1,20000);
	m_zp->SetValue(10000);	
	m_zs->SetRange(1,10000);
	m_zs->SetValue(1000);	
}

bool C3DControlPan::Create( wxWindow* parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxString& x3d_l,
						   const wxString& y3d_l,
						   const wxString& z3d_l )
{
    m_data = NULL;
    m_prox = NULL;
    m_proy = NULL;
    m_proz = NULL;
    m_select = NULL;
	m_static_text_x = NULL;
	m_static_text_y = NULL;
	m_static_text_z = NULL;
	x3d_label = x3d_l;
	y3d_label = y3d_l;
	z3d_label = z3d_l;
    m_xp = NULL;
    m_xs = NULL;
    m_yp = NULL;
    m_ys = NULL;
    m_zp = NULL;
    m_zs = NULL;

    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    return true;
}


void C3DControlPan::CreateControls()
{
    wxXmlResource::Get()->LoadPanel(this, GetParent(), "IDD_3DCONTROL");
    m_data = XRCCTRL(*this, "IDC_DATAPOINT", wxCheckBox);
    m_prox = XRCCTRL(*this, "IDC_TOX", wxCheckBox);
    m_proy = XRCCTRL(*this, "IDC_TOY", wxCheckBox);
    m_proz = XRCCTRL(*this, "IDC_TOZ", wxCheckBox);
    m_select = XRCCTRL(*this, "IDC_SELECT", wxCheckBox);
	if ( GeneralWxUtils::isMac() ) {
		// change "CTRL" in label to "CMD" in label.
		m_select->SetLabel("Select, hold CMD for brushing");
	}
	m_static_text_x = XRCCTRL(*this, "ID_3D_STATICTEXT_X", wxStaticText);
	m_static_text_y = XRCCTRL(*this, "ID_3D_STATICTEXT_Y", wxStaticText);
	m_static_text_z = XRCCTRL(*this, "ID_3D_STATICTEXT_Z", wxStaticText);
	UpdateAxesLabels(x3d_label, y3d_label, z3d_label);
    m_xp = XRCCTRL(*this, "IDC_SLXP", wxSlider);
    m_xs = XRCCTRL(*this, "IDC_SLXS", wxSlider);
    m_yp = XRCCTRL(*this, "IDC_SLYP", wxSlider);
    m_ys = XRCCTRL(*this, "IDC_SLYS", wxSlider);
    m_zp = XRCCTRL(*this, "IDC_SLZP", wxSlider);
    m_zs = XRCCTRL(*this, "IDC_SLZS", wxSlider);
}

void C3DControlPan::UpdateAxesLabels(const wxString& x, const wxString& y,
									 const wxString& z)
{
	m_static_text_x->SetLabel("X: " + x);
	m_static_text_y->SetLabel("Y: " + y);
	m_static_text_z->SetLabel("Z: " + z);	
}

void C3DControlPan::OnCDatapointClick( wxCommandEvent& event )
{
	template_frame->canvas->m_d = m_data->GetValue();
	template_frame->canvas->Refresh();
}

void C3DControlPan::OnCToxClick( wxCommandEvent& event )
{
	template_frame->canvas->m_x = m_prox->GetValue();
	template_frame->canvas->Refresh();

}

void C3DControlPan::OnCToyClick( wxCommandEvent& event )
{
	template_frame->canvas->m_y = m_proy->GetValue();
	template_frame->canvas->Refresh();
}

void C3DControlPan::OnCTozClick( wxCommandEvent& event )
{
	template_frame->canvas->m_z = m_proz->GetValue();
	template_frame->canvas->Refresh();

}

void C3DControlPan::OnCSelectClick( wxCommandEvent& event )
{
	template_frame->canvas->b_select = m_select->GetValue();
	template_frame->canvas->Refresh();
}

/**  Called when X position slider moved*/
void C3DControlPan::OnCSlxpUpdated( wxCommandEvent& event )
{
	template_frame->canvas->xp = ((double) m_xp->GetValue())/10000.0 - 1.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();
}

/** Called when X selection size slider moved */
void C3DControlPan::OnCSlxsUpdated( wxCommandEvent& event )
{
	template_frame->canvas->xs = ((double) m_xs->GetValue())/10000.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();     
}

void C3DControlPan::OnCSlypUpdated( wxCommandEvent& event )
{
	template_frame->canvas->yp = ((double) m_yp->GetValue())/10000.0 - 1.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();
}

void C3DControlPan::OnCSlysUpdated( wxCommandEvent& event )
{
	template_frame->canvas->ys = ((double) m_ys->GetValue())/10000.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();
}

void C3DControlPan::OnCSlzpUpdated( wxCommandEvent& event )
{
	template_frame->canvas->zp = ((double) m_zp->GetValue())/10000.0 - 1.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();
}

void C3DControlPan::OnCSlzsUpdated( wxCommandEvent& event )
{
	template_frame->canvas->zs = ((double) m_zs->GetValue())/10000.0;
	if (this->m_select->GetValue()) template_frame->canvas->UpdateSelect();
	template_frame->canvas->Refresh();
}
