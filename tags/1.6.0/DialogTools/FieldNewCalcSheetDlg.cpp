/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#include <wx/grid.h>
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "FieldNewCalcSheetDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcSheetDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_APPLY"), FieldNewCalcSheetDlg::OnApplyClick )
END_EVENT_TABLE()

FieldNewCalcSheetDlg::FieldNewCalcSheetDlg(Project* project_s,
										   wxWindow* parent, wxWindowID id,
										   const wxString& caption, 
										   const wxPoint& pos,
										   const wxSize& size, long style )
: project(project_s)
{
	Create(parent, id, caption, pos, size, style);

	pSpecial = new FieldNewCalcSpecialDlg(project, m_note);
	pUni = new FieldNewCalcUniDlg(project, m_note);
	pBin = new FieldNewCalcBinDlg(project, m_note);
	pLag = new FieldNewCalcLagDlg(project, m_note);
	pRate = new FieldNewCalcRateDlg(project, m_note);
	pSpecial->SetOtherPanelPointers(pUni, pBin, pLag, pRate);
	pUni->SetOtherPanelPointers(pSpecial, pBin, pLag, pRate);
	pBin->SetOtherPanelPointers(pSpecial, pUni, pLag, pRate);
	pLag->SetOtherPanelPointers(pSpecial, pUni, pBin, pRate);
	pRate->SetOtherPanelPointers(pSpecial, pUni, pBin, pLag);

	m_note->AddPage(pSpecial, "Special");
	m_note->AddPage(pUni, "Univariate");
	m_note->AddPage(pBin, "Bivariate");
	m_note->AddPage(pLag, "Spatial Lag");
	m_note->AddPage(pRate, "Rates");
	this->SetSize(-1,-1,-1,-1);
}

bool FieldNewCalcSheetDlg::Create( wxWindow* parent, wxWindowID id,
	const wxString& caption, const wxPoint& pos,
	const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}


void FieldNewCalcSheetDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_FIELDCALC_SHEET");
    m_note = XRCCTRL(*this, "ID_NOTEBOOK", wxNotebook);
}


void FieldNewCalcSheetDlg::OnApplyClick( wxCommandEvent& event )
{
	switch(m_note->GetSelection())
	{
		case 0:
			pSpecial->Apply();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			break;			
		case 1:
			pUni->Apply();
			pSpecial->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			break;
		case 2:
			pBin->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			break;
		case 3:
			pLag->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pRate->InitFieldChoices();
			break;
		case 4:
			pRate->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			break;
		default:
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();			
			break;
	}
}

