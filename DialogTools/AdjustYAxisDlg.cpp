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

IMPLEMENT_CLASS( AdjustYAxisDlg, wxDialog )

BEGIN_EVENT_TABLE( AdjustYAxisDlg, wxDialog )
    EVT_BUTTON( wxID_OK, AdjustYAxisDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, AdjustYAxisDlg::OnCancelClick )
END_EVENT_TABLE()

AdjustYAxisDlg::AdjustYAxisDlg( wxString min_val_s,
								 wxString max_val_s,
								 wxWindow* parent,
								 wxWindowID id,
								 const wxString& caption,
								 const wxPoint& pos, const wxSize& size,
								 long style )
: s_min_val(min_val_s), s_max_val(max_val_s)
{
    min_val_s.ToDouble(&o_min_val);
    max_val_s.ToDouble(&o_max_val);
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
		wxMessageBox("Please enter a valid MIN value for Y axis");
		return;
	}
    if (!m_max_val->GetValue().ToDouble(&max_val)) {
        wxMessageBox("Please enter a valid MAX value for Y axis");
        return;
    }
   
    if (min_val > o_min_val) {
        wxString msg;
        msg << "Please make sure the input MIN value <= " << o_min_val;
        wxMessageBox(msg);
        return;
        
    }
    if (max_val < o_max_val) {
        wxString msg;
        msg << "Please make sure the input MAX value >= " << o_max_val;
        wxMessageBox(msg);

        return;
        
    }
    
	wxString _min_val = m_min_val->GetValue();
    wxString _max_val = m_max_val->GetValue();

    
    if (max_val <= min_val) {
        wxMessageBox("Please make sure input MAX value is larger than input MIN value");
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
