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

AdjustYAxisDlg::AdjustYAxisDlg( double min_val_s,
								 double max_val_s,
								 wxWindow* parent,
								 wxWindowID id,
								 const wxString& caption,
								 const wxPoint& pos, const wxSize& size,
								 long style )
: o_min_val(min_val_s), o_max_val(max_val_s)
{
    
    LOG_MSG("Entering AdjustYAxisDlg::AdjustYAxisDlg(..)");
    s_min_val << min_val_s;
    s_max_val << max_val_s;
    
	SetParent(parent);
    CreateControls();
    Centre();
    
    LOG_MSG("Exiting AdjustYAxisDlg::AdjustYAxisDlg(..)");
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
  
    /*
    if (min_val > o_min_val) {
        wxString msg;
        msg << "Please make sure the input Min value <= " << o_min_val;
        wxMessageBox(msg);
        return;
        
    }
    if (max_val < o_max_val - 0.0000001) {
        wxString msg;
        msg << "Please make sure the input Max value >= " << o_max_val;
        wxMessageBox(msg);

        return;
        
    }
    */
    
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

IMPLEMENT_CLASS( AxisLabelPrecisionDlg, wxDialog )

BEGIN_EVENT_TABLE( AxisLabelPrecisionDlg, wxDialog )
EVT_BUTTON( wxID_OK, AxisLabelPrecisionDlg::OnOkClick )
EVT_BUTTON( wxID_CANCEL, AxisLabelPrecisionDlg::OnCancelClick )
END_EVENT_TABLE()

AxisLabelPrecisionDlg::AxisLabelPrecisionDlg(int precision_s,
                                             wxWindow* parent,
                                             wxWindowID id,
                                             const wxString& caption,
                                             const wxPoint& pos,
                                             const wxSize& size,
                                             long style)
{
    precision = precision_s;
    
    SetParent(parent);
    CreateControls();
    Centre();
}

void AxisLabelPrecisionDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_AXIS_LABEL_PRECISION_DLG");
    m_precision_spin = wxDynamicCast(FindWindow(XRCID("ID_AXIS_LABEL_PRECISION_SPIN")), wxSpinCtrl);
    m_precision_spin->SetRange(1, 6);
    

}
void AxisLabelPrecisionDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
    EndDialog(wxID_CANCEL);
    
}

void AxisLabelPrecisionDlg::OnOkClick( wxCommandEvent& event )
{
    precision = m_precision_spin->GetValue();
    if (precision < 0 || precision > 6) {
        precision = 1;
    }
    EndDialog(wxID_OK);
}
