/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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
#include "../logger.h"
#include "GetisOrdChoiceDlg.h"


BEGIN_EVENT_TABLE( GetisOrdChoiceDlg, wxDialog )
    EVT_BUTTON( wxID_OK, GetisOrdChoiceDlg::OnOkClick )
	EVT_RADIOBUTTON( XRCID("IDC_W_ROW_STAND"),
					GetisOrdChoiceDlg::OnRadioWStand )
	EVT_RADIOBUTTON( XRCID("IDC_W_BINARY"),
					GetisOrdChoiceDlg::OnRadioWBinary )
END_EVENT_TABLE()


GetisOrdChoiceDlg::GetisOrdChoiceDlg( wxWindow* parent, wxWindowID id,
									 const wxString& caption,
									 const wxPoint& pos,
									 const wxSize& size, long style )
{
	SetParent(parent);
    CreateControls();
    Centre();
	row_standardize_weights = true;
}

void GetisOrdChoiceDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_GETIS_ORD_CHOICE");

	gi_clus_map_perm_check =
		wxDynamicCast(FindWindow(XRCID("IDC_GI_CHECK")), wxCheckBox);
	gi_star_clus_map_perm_check =
		wxDynamicCast(FindWindow(XRCID("IDC_GI_STAR_CHECK")), wxCheckBox);
	show_sig_map_check =
		wxDynamicCast(FindWindow(XRCID("IDC_SIG_MAPS_CHECK")), wxCheckBox);
	show_norm_pval_check =
		wxDynamicCast(FindWindow(XRCID("IDC_NORM_P_VAL_CHECK")), wxCheckBox);
	w_row_standardize =
		wxDynamicCast(FindWindow(XRCID("IDC_W_ROW_STAND")), wxRadioButton);
	w_binary =
		wxDynamicCast(FindWindow(XRCID("IDC_W_BINARY")), wxRadioButton);
}

void GetisOrdChoiceDlg::OnOkClick( wxCommandEvent& event )
{
	bool gi = gi_clus_map_perm_check->GetValue() == 1;
	bool gi_star = gi_star_clus_map_perm_check->GetValue() == 1;
	bool sig_map = show_sig_map_check->GetValue() == 1;
	bool norm_pval = show_norm_pval_check->GetValue() == 1;

	Gi_ClustMap_norm = norm_pval && gi;
	Gi_SigMap_norm = norm_pval && gi && sig_map;
	GiStar_ClustMap_norm = norm_pval && gi_star;
	GiStar_SigMap_norm = norm_pval && gi_star && sig_map;
	
	Gi_ClustMap_perm = gi;
	Gi_SigMap_perm = gi && sig_map;
	GiStar_ClustMap_perm = gi_star;
	GiStar_SigMap_perm = gi_star && sig_map;

	row_standardize_weights = w_row_standardize->GetValue() == 1;
	
	event.Skip();
	EndDialog(wxID_OK);	
}

void GetisOrdChoiceDlg::OnRadioWStand( wxCommandEvent& event )
{
}

void GetisOrdChoiceDlg::OnRadioWBinary( wxCommandEvent& event )
{
}

