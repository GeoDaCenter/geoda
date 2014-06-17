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

#include <fstream>
#include <wx/msgdlg.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../GeoDa.h"
#include "CreatingWeightDlg.h"
#include "SelectWeightDlg.h"

BEGIN_EVENT_TABLE( SelectWeightDlg, wxDialog )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_OPENWEIGHT1"),
					SelectWeightDlg::OnCRadioOpenweight1Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_OPENWEIGHT2"),
					SelectWeightDlg::OnCRadioOpenweight2Selected )
    EVT_BUTTON( XRCID("IDC_OPEN_FILEWEIGHT"),
			   SelectWeightDlg::OnCOpenFileweightClick )
	EVT_BUTTON( XRCID("ID_CREATE_WEIGHTS"),
			   SelectWeightDlg::OnCCreateWeightsClick )
    EVT_BUTTON( wxID_OK, SelectWeightDlg::OnOkClick )
END_EVENT_TABLE()

SelectWeightDlg::SelectWeightDlg(Project* project_s,
								 wxWindow* parent,
								 wxWindowID id,
								 const wxString& caption, const wxPoint& pos,
								 const wxSize& size, long style )
: project(project_s), table_int(project_s->GetTableInt()),
w_manager(project_s->GetWManager())
{
	Create(parent, id, caption, pos, size, style);

	m_checkButton->SetValue(w_manager->IsDefaultWeight());
	wxCommandEvent event;
	if (w_manager->GetNumWeights() > 0) {
		PumpingWeight();
		if (w_manager->IsDefaultWeight()) {
			m_weights->Select(w_manager->GetCurrWeightInd());
			m_wfile_fpath = w_manager->GetCurrWFilename();
			FindWindow(XRCID("wxID_OK"))->Enable(false);
		}
		m_radio1->SetValue(true);
		OnCRadioOpenweight1Selected(event);
	} else {
		m_wfile_fpath = wxEmptyString;
		m_radio2->SetValue(true);
		OnCRadioOpenweight2Selected(event);		
		FindWindow(XRCID("IDC_CURRENTUSED_W"))->Enable(false);
		FindWindow(XRCID("IDC_RADIO_OPENWEIGHT1"))->Enable(false);
	}

	std::ifstream ifl(m_wfile_fpath.mb_str());
	if (!ifl) {
			} else {
		FindWindow(XRCID("wxID_OK"))->Enable(true);
	}
}

bool SelectWeightDlg::Create( wxWindow* parent, wxWindowID id,
							  const wxString& caption, const wxPoint& pos,
							  const wxSize& size, long style )
{
    m_radio1 = NULL;
    m_weights = NULL;
    m_radio2 = NULL;
    m_field = NULL;
    m_checkButton = NULL;
    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}

void SelectWeightDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_SELECT_WEIGHT");
    m_radio1 = XRCCTRL(*this, "IDC_RADIO_OPENWEIGHT1", wxRadioButton);
    m_weights = XRCCTRL(*this, "IDC_CURRENTUSED_W", wxChoice);
    m_radio2 = XRCCTRL(*this, "IDC_RADIO_OPENWEIGHT2", wxRadioButton);
    m_field = XRCCTRL(*this, "IDC_EDIT_FILEWEIGHT", wxTextCtrl);
    m_checkButton = XRCCTRL(*this, "IDC_CHECK_DEFAULT", wxCheckBox);
}


void SelectWeightDlg::OnCRadioOpenweight1Selected( wxCommandEvent& event )
{
	FindWindow(XRCID("wxID_OK"))->Enable(true);
	FindWindow(XRCID("IDC_CURRENTUSED_W"))->Enable(true);
	FindWindow(XRCID("IDC_EDIT_FILEWEIGHT"))->Enable(false);
	FindWindow(XRCID("IDC_OPEN_FILEWEIGHT"))->Enable(false);
}


void SelectWeightDlg::OnCRadioOpenweight2Selected( wxCommandEvent& event )
{
	FindWindow(XRCID("wxID_OK"))->Enable(m_field->GetValue() != wxEmptyString);
	FindWindow(XRCID("IDC_CURRENTUSED_W"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_FILEWEIGHT"))->Enable(true);
	FindWindow(XRCID("IDC_OPEN_FILEWEIGHT"))->Enable(true);
}

void SelectWeightDlg::OnOkClick( wxCommandEvent& event )
{
	if (m_radio1->GetValue()) {
		int pos = m_weights->GetSelection();
		w_manager->SetCurrWeightInd(pos);
		m_wfile_fpath = w_manager->GetCurrWFilename();
		w_manager->SetDefaultWeight(m_checkButton->GetValue());
	}
	if (m_radio2->GetValue()) {
		wxString ext = GenUtils::GetFileExt(m_wfile_fpath).Lower();
		wxString flname = m_wfile_fpath;
	
		int obs = w_manager->GetNumObservations();
		if (ext == "gal") {
			GalElement* tempGal = WeightUtils::ReadGal(flname, table_int);
			if (tempGal == NULL) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				return;
			}
			GalWeight* w = new GalWeight();
			w->num_obs = obs;
			w->wflnm = m_wfile_fpath;
			w->gal = tempGal;
			if (!w_manager->AddWeightFile(w, m_checkButton->GetValue())) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				m_wfile_fpath = wxEmptyString;
				delete w;
				return;
			}
		} else if (ext == "gwt") {
			GalElement* tempGal = WeightUtils::ReadGwtAsGal(flname, table_int);
			if (tempGal == NULL) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				return;
			}
			GalWeight* w = new GalWeight();
			w->num_obs = obs;
			w->wflnm = m_wfile_fpath;
			w->gal = tempGal;
			if (!w_manager->AddWeightFile(w, m_checkButton->GetValue())) {
				FindWindow(XRCID("wxID_OK"))->Enable(false);
				m_wfile_fpath = wxEmptyString;
				delete w;
				return;
			}
		} else {
			FindWindow(XRCID("wxID_OK"))->Enable(false);
			wxMessageBox("Error: Wrong file extension");
			m_wfile_fpath = wxEmptyString;
			return;
		}
	}
	event.Skip();
	EndDialog(wxID_OK);
}

void SelectWeightDlg::OnCOpenFileweightClick( wxCommandEvent& event )
{
    wxFileDialog dlg( this, "Input Weights File", "",
					 "Weights Files (*.gal, *.gwt)|*.gal;*.gwt");

	
	wxString m_path = wxEmptyString;
    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		m_field->SetValue(m_path);
		m_wfile_fpath = m_path;

		FindWindow(XRCID("wxID_OK"))->Enable(true);
		FindWindow(XRCID("IDC_CHECK_DEFAULT"))->Enable(true);
	}
}

void SelectWeightDlg::OnCCreateWeightsClick( wxCommandEvent& event )
{
	CreatingWeightDlg dlg(this, project);
	dlg.ShowModal();
	if (w_manager->GetNumWeights() > 0) {
		PumpingWeight();
		m_radio2->SetValue(false);
		m_radio1->Enable(true);
		m_radio1->SetValue(true);
		OnCRadioOpenweight1Selected( event );
		m_checkButton->SetValue(w_manager->IsDefaultWeight());
	}
}

void SelectWeightDlg::PumpingWeight()
{
	m_weights->Clear();

	for (int i=0; i<w_manager->GetNumWeights(); i++) {
		m_weights->Append(w_manager->GetWFilename(i));
	}

	if (w_manager->GetNumWeights() > 0) {
		if (w_manager->GetCurrWeightInd() >= 0) {
			m_weights->SetSelection(w_manager->GetCurrWeightInd());
		} else {
			m_weights->SetSelection(0);
		}
	}
}
