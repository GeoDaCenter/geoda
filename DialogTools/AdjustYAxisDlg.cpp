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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>
#include "AdjustYAxisDlg.h"
#include "../logger.h"

IMPLEMENT_CLASS( AdjustYAxisDlg, wxDialog )

BEGIN_EVENT_TABLE( AdjustYAxisDlg, wxDialog )
    EVT_BUTTON( wxID_OK, AdjustYAxisDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, AdjustYAxisDlg::OnCancelClick )
END_EVENT_TABLE()

AdjustYAxisDlg::AdjustYAxisDlg(double min_val_s,
                               double max_val_s,
                               wxWindow* parent,
                               wxWindowID id,
                               const wxString& caption,
                               const wxPoint& pos, const wxSize& size,
                               long style )
: o_min_val(min_val_s), o_max_val(max_val_s)
{
    
    wxLogMessage(wxString::Format("AdjustYAxisDlg with new min_val %f and max_val %f", min_val_s, max_val_s));
    
    s_min_val << min_val_s;
    s_max_val << max_val_s;
    
	SetParent(parent);
    CreateControls();
    Centre();
}

void AdjustYAxisDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_ADJUST_YAXIS");
	m_min_val = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_YAXIS_MIN")), wxTextCtrl);
    m_max_val = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_YAXIS_MAX")), wxTextCtrl);
    
    m_min_val->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
    m_max_val->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
    
    m_min_val->SetValue(s_min_val);
    m_max_val->SetValue(s_max_val);
}

void AdjustYAxisDlg::OnOkClick( wxCommandEvent& event )
{
	if (!m_min_val->GetValue().ToDouble(&min_val)) {
		wxMessageBox("Please enter a valid Min value for Y axis");
		return;
	}
    if (!m_max_val->GetValue().ToDouble(&max_val)) {
        wxMessageBox("Please enter a valid Max value for Y axis");
        return;
    }
	wxString _min_val = m_min_val->GetValue();
    wxString _max_val = m_max_val->GetValue();
    if (max_val <= min_val) {
        wxMessageBox("Please make sure input Max value is larger than input Min value");
        return;
    }
    
    s_min_val = _min_val;
    s_max_val = _max_val;
    
    event.Skip();
	EndDialog(wxID_OK);
}

void AdjustYAxisDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
	EndDialog(wxID_CANCEL);

}

IMPLEMENT_CLASS( SetDisplayPrecisionDlg, wxDialog )

BEGIN_EVENT_TABLE( SetDisplayPrecisionDlg, wxDialog )
EVT_BUTTON( wxID_OK, SetDisplayPrecisionDlg::OnOkClick )
EVT_BUTTON( wxID_CANCEL, SetDisplayPrecisionDlg::OnCancelClick )
END_EVENT_TABLE()

SetDisplayPrecisionDlg::SetDisplayPrecisionDlg(int precision_s,
                                               bool fixed_point_s,
                                             wxWindow* parent,
                                             wxWindowID id,
                                             const wxString& caption,
                                             const wxPoint& pos,
                                             const wxSize& size,
                                             long style)
{
    wxLogMessage(wxString::Format("SetDisplayPrecisionDlg with precision = %d.",
                                  precision_s));
    precision = precision_s;
    fixed_point = fixed_point_s;
    SetParent(parent);
    CreateControls();
    Centre();
}

void SetDisplayPrecisionDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
                                     "ID_LABEL_PRECISION_DLG");
    m_precision_spin = wxDynamicCast(FindWindow(XRCID("ID_LABEL_PRECISION_SPIN")),
                                     wxSpinCtrl);
    m_precision_spin->SetRange(0, 6);
    m_precision_spin->SetValue(precision);
    m_precision_spin->Bind(wxEVT_KEY_DOWN, &SetDisplayPrecisionDlg::OnKeyUp, this);

    m_fixed_point = wxDynamicCast(FindWindow(XRCID("IDC_FIX_POINT_CHECK")),
                                  wxCheckBox);
    m_fixed_point->SetValue(fixed_point);
}

void SetDisplayPrecisionDlg::OnKeyUp( wxEvent& event )
{
    if (((wxKeyEvent&)event).GetKeyCode() == WXK_RETURN) {
        wxCommandEvent ev;
        OnOkClick(ev);
    }
}

void SetDisplayPrecisionDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
    EndDialog(wxID_CANCEL);
}

void SetDisplayPrecisionDlg::OnOkClick( wxCommandEvent& event )
{
    precision = m_precision_spin->GetValue();
    if (precision < 0 || precision > 12) {
        precision = 1;
    }
    fixed_point = m_fixed_point->GetValue();
    EndDialog(wxID_OK);
}
