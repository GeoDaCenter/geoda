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

#ifndef __GEODA_CENTER_GETIS_ORD_CHOICE_DLG_H__
#define __GEODA_CENTER_GETIS_ORD_CHOICE_DLG_H__

#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>

class GetisOrdChoiceDlg: public wxDialog
{    
public:
    GetisOrdChoiceDlg( wxWindow* parent, wxWindowID id = wxID_ANY,
					  const wxString& caption = "Maps To Open",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxCAPTION | wxSYSTEM_MENU);

    void CreateControls();

    void OnOkClick( wxCommandEvent& event );
	void OnRadioWStand( wxCommandEvent& event );
	void OnRadioWBinary( wxCommandEvent& event );

	wxCheckBox* gi_clus_map_perm_check;
	wxCheckBox* gi_star_clus_map_perm_check;	
	wxCheckBox* show_sig_map_check;
	wxCheckBox* show_norm_pval_check;
	wxRadioButton* w_row_standardize;
	wxRadioButton* w_binary;
	
	bool Gi_ClustMap_norm;
	bool Gi_SigMap_norm;
	bool GiStar_ClustMap_norm;
	bool GiStar_SigMap_norm;
	bool Gi_ClustMap_perm;
	bool Gi_SigMap_perm;
	bool GiStar_ClustMap_perm;
	bool GiStar_SigMap_perm;
	
	bool row_standardize_weights;
	
	DECLARE_EVENT_TABLE()
};

#endif
