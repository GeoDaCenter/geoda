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

#include <wx/grid.h>
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../DataViewer/TableState.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "FieldNewCalcSheetDlg.h"

BEGIN_EVENT_TABLE( FieldNewCalcSheetDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_APPLY"), FieldNewCalcSheetDlg::OnApplyClick )
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID("ID_NOTEBOOK"), 
							  FieldNewCalcSheetDlg::OnPageChange )
	EVT_CLOSE( FieldNewCalcSheetDlg::OnClose )
END_EVENT_TABLE()

FieldNewCalcSheetDlg::FieldNewCalcSheetDlg(Project* project_s,
										   wxWindow* parent, wxWindowID id,
										   const wxString& caption, 
										   const wxPoint& pos,
										   const wxSize& size, long style )
: project(project_s), frames_manager(project_s->GetFramesManager()),
table_state(project_s->GetTableState()),
w_man_state(project_s->GetWManState())

{
    wxLogMessage("Open FieldNewCalcSheetDlg.");
    
	Create(parent, id, caption, pos, size, style);

	pSpecial = new FieldNewCalcSpecialDlg(project, m_note);
	pUni = new FieldNewCalcUniDlg(project, m_note);
	pBin = new FieldNewCalcBinDlg(project, m_note);
	pLag = new FieldNewCalcLagDlg(project, m_note);
	pRate = new FieldNewCalcRateDlg(project, m_note);
	pDT = new FieldNewCalcDateTimeDlg(project, m_note);
	pSpecial->SetOtherPanelPointers(pUni, pBin, pLag, pRate);
	pUni->SetOtherPanelPointers(pSpecial, pBin, pLag, pRate);
	pBin->SetOtherPanelPointers(pSpecial, pUni, pLag, pRate);
	pLag->SetOtherPanelPointers(pSpecial, pUni, pBin, pRate);
	pRate->SetOtherPanelPointers(pSpecial, pUni, pBin, pLag);
	pDT->SetOtherPanelPointers(pSpecial, pBin, pLag, pRate);

	m_note->AddPage(pSpecial, _("Special"));
	m_note->AddPage(pUni, _("Univariate"));
	m_note->AddPage(pBin, _("Bivariate"));
	m_note->AddPage(pLag, _("Spatial Lag"));
	m_note->AddPage(pRate, _("Rates"));
	m_note->AddPage(pDT, _("Date/Time"));
	pLag->InitWeightsList();
	pRate->InitWeightsList();
	this->SetSize(-1,-1,-1,-1);
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	w_man_state->registerObserver(this);
}

FieldNewCalcSheetDlg::~FieldNewCalcSheetDlg()
{
    frames_manager->removeObserver(this);
    table_state->removeObserver(this);
    w_man_state->removeObserver(this);
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

void FieldNewCalcSheetDlg::OnPageChange( wxBookCtrlEvent& event )
{
    wxLogMessage("In FieldNewCalcSheetDlg::OnPageChange()");
	int tab_idx = event.GetOldSelection();
	int var_sel_idx = -1;
	if (tab_idx == 0) 
		var_sel_idx = pSpecial->m_result->GetCurrentSelection();
	else if (tab_idx == 1) 
		var_sel_idx = pUni->m_result->GetCurrentSelection();
	else if (tab_idx == 2) 
		var_sel_idx = pBin->m_result->GetCurrentSelection();
	else if (tab_idx == 3) 
		var_sel_idx = pLag->m_result->GetCurrentSelection();
	else if (tab_idx == 4) 
		var_sel_idx = pRate->m_result->GetCurrentSelection();
	else if (tab_idx == 5)
		var_sel_idx = pDT->m_result->GetCurrentSelection();
	
	{
        /*
		pSpecial->m_result->SetSelection(var_sel_idx);
        pSpecial->InitFieldChoices();
		pUni->m_result->SetSelection(var_sel_idx);
        pUni->InitFieldChoices();
		pBin->m_result->SetSelection(var_sel_idx);
        pBin->InitFieldChoices();
		pLag->m_result->SetSelection(var_sel_idx);
        pLag->InitFieldChoices();
		pRate->m_result->SetSelection(var_sel_idx);
        pRate->InitFieldChoices();
		pDT->m_result->SetSelection(var_sel_idx);
        pDT->InitFieldChoices();
         */
	}
}

void FieldNewCalcSheetDlg::OnApplyClick( wxCommandEvent& event )
{
    wxLogMessage("In FieldNewCalcSheetDlg::OnApplyClick()");
	switch(m_note->GetSelection())
	{
		case 0:
			pSpecial->Apply();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			pDT->InitFieldChoices();
			break;			
		case 1:
			pUni->Apply();
			pSpecial->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			pDT->InitFieldChoices();
			break;
		case 2:
			pBin->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
			pDT->InitFieldChoices();
			break;
		case 3:
			pLag->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pRate->InitFieldChoices();
			pDT->InitFieldChoices();
			break;
		case 4:
			pRate->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pDT->InitFieldChoices();
			break;
		case 5:
			pDT->Apply();
			pSpecial->InitFieldChoices();
			pUni->InitFieldChoices();
			pBin->InitFieldChoices();
			pLag->InitFieldChoices();
			pRate->InitFieldChoices();
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

void FieldNewCalcSheetDlg::OnClose(wxCloseEvent& event)
{
    wxLogMessage("In FieldNewCalcSheetDlg::OnClose()");

    Destroy();
}

void FieldNewCalcSheetDlg::update(FramesManager* o)
{
}

void FieldNewCalcSheetDlg::update(TableState* o)
{
	pSpecial->InitFieldChoices();
	pUni->InitFieldChoices();
	pBin->InitFieldChoices();
	pLag->InitFieldChoices();
	pRate->InitFieldChoices();
	pDT->InitFieldChoices();
}

void FieldNewCalcSheetDlg::update(WeightsManState* o)
{
	pLag->InitWeightsList();
	pRate->InitWeightsList();
}
