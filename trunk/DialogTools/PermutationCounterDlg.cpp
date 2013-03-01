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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/image.h>
#include <wx/xrc/xmlres.h>              // XRC XML resouces
#include <wx/dialog.h>
#include <wx/valtext.h>
#include <wx/sizer.h>

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "PermutationCounterDlg.h"


IMPLEMENT_CLASS( PermutationCounterDlg, wxDialog )

BEGIN_EVENT_TABLE( PermutationCounterDlg, wxDialog )
    EVT_BUTTON( wxID_OK, PermutationCounterDlg::OnOkClick )
END_EVENT_TABLE()


PermutationCounterDlg::PermutationCounterDlg( )
{
}

PermutationCounterDlg::PermutationCounterDlg( wxWindow* parent,
											   wxWindowID id,
											   const wxString& caption,
											   const wxPoint& pos,
											   const wxSize& size,
											   long style )
{
    Create(parent, id, caption, pos, size, style);
	s_int = "999";
}

bool PermutationCounterDlg::Create( wxWindow* parent,
									wxWindowID id,
									const wxString& caption,
									const wxPoint& pos,
									const wxSize& size,
									long style )
{
    m_number = NULL;
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}


void PermutationCounterDlg::CreateControls()
{    
    wxXmlResource::Get()->
		LoadDialog(this, GetParent(), "IDD_PERMUTATION_COUNT");
    m_number = XRCCTRL(*this, "IDC_EDIT_ORDEROFCONTIGUITY", wxTextCtrl);
    // Set validators
    if (FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY")))
        FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY"))->
			SetValidator(wxTextValidator(wxFILTER_NUMERIC, & s_int) );
}

void PermutationCounterDlg::OnOkClick( wxCommandEvent& event )
{
    event.Skip();
	EndDialog(wxID_OK);
}


