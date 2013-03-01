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

#include <limits>
#include <vector>
#include <set>
#include <string>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../ShapeOperations/ShapeFile.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GenUtils.h"

#include "AddIdVariable.h"
#include "CreatingWeightDlg.h"

BEGIN_EVENT_TABLE( CreatingWeightDlg, wxDialog )
	EVT_BUTTON( XRCID("ID_CREATE_ID"),
			   CreatingWeightDlg::OnCreateNewIdClick )
    EVT_CHOICE(XRCID("IDC_IDVARIABLE"),
				  CreatingWeightDlg::OnIdVariableSelected )
    EVT_CHOICE(XRCID("IDC_DISTANCE_METRIC"),
				  CreatingWeightDlg::OnDistanceMetricSelected )
    EVT_CHOICE(XRCID("IDC_XCOORDINATES"), CreatingWeightDlg::OnXSelected )
    EVT_CHOICE(XRCID("IDC_YCOORDINATES"), CreatingWeightDlg::OnYSelected )
	EVT_CHOICE(XRCID("IDC_XCOORD_TIME"), CreatingWeightDlg::OnXTmSelected )
	EVT_CHOICE(XRCID("IDC_YCOORD_TIME"), CreatingWeightDlg::OnYTmSelected )
	EVT_RADIOBUTTON( XRCID("IDC_RADIO_QUEEN"),
					CreatingWeightDlg::OnCRadioQueenSelected )
    EVT_SPIN( XRCID("IDC_SPIN_ORDEROFCONTIGUITY"),
			 CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_ROOK"),
					CreatingWeightDlg::OnCRadioRookSelected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO_DISTANCE"),
					CreatingWeightDlg::OnCRadioDistanceSelected )
	EVT_TEXT( XRCID("IDC_THRESHOLD_EDIT"),
			 CreatingWeightDlg::OnCThresholdTextEdit )
    EVT_SLIDER( XRCID("IDC_THRESHOLD_SLIDER"),
			   CreatingWeightDlg::OnCThresholdSliderUpdated )

    EVT_RADIOBUTTON( XRCID("IDC_RADIO_KNN"),
					CreatingWeightDlg::OnCRadioKnnSelected )
    EVT_SPIN( XRCID("IDC_SPIN_KNN"), CreatingWeightDlg::OnCSpinKnnUpdated )
    EVT_BUTTON( XRCID("wxID_OK"), CreatingWeightDlg::OnCreateClick )
    EVT_BUTTON( XRCID("wxID_CLOSE"), CreatingWeightDlg::OnCloseClick )
END_EVENT_TABLE()


CreatingWeightDlg::CreatingWeightDlg(wxWindow* parent,
									 Project* project_s,
									 wxWindowID id,
									 const wxString& caption,
									 const wxPoint& pos, 
									 const wxSize& size,
									 long style )
: all_init(false), m_thres_delta_factor(1.00001),
m_method(1), m_thres_val_valid(false), m_threshold_val(0.01),
project(project_s), grid_base(project_s->GetGridBase()),
m_num_obs(project_s->GetNumRecords()),
m_is_table_only(project_s->IsTableOnlyProject()),
m_is_space_time(project_s->GetGridBase()->IsTimeVariant())
{
	Create(parent, id, caption, pos, size, style);
	all_init = true;
}

bool CreatingWeightDlg::Create( wxWindow* parent, wxWindowID id,
								const wxString& caption, const wxPoint& pos,
								const wxSize& size, long style )
{
    m_field = 0;
    m_radio2 = 0;
    m_contiguity = 0;
    m_spincont = 0;
    m_radio1 = 0;
    m_include_lower = 0;
    m_distance_metric = 0;
    m_X = 0;
    m_Y = 0;
	m_X_time = 0;
    m_Y_time = 0;
    m_radio3 = 0;
    m_threshold = 0;
    m_sliderdistance = 0;
    m_radio4 = 0;
    m_neighbors = 0;
    m_spinneigh = 0;

    SetParent(parent);
    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    return true;
}

void CreatingWeightDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_WEIGHTS_FILE_CREATION");
    m_field = XRCCTRL(*this, "IDC_IDVARIABLE", wxChoice);
    m_contiguity = XRCCTRL(*this, "IDC_EDIT_ORDEROFCONTIGUITY", wxTextCtrl);
    m_spincont = XRCCTRL(*this, "IDC_SPIN_ORDEROFCONTIGUITY", wxSpinButton);
    m_include_lower = XRCCTRL(*this, "IDC_CHECK1", wxCheckBox);
    m_distance_metric = XRCCTRL(*this, "IDC_DISTANCE_METRIC", wxChoice);
    m_X = XRCCTRL(*this, "IDC_XCOORDINATES", wxChoice);
    m_Y = XRCCTRL(*this, "IDC_YCOORDINATES", wxChoice);
	m_X_time = XRCCTRL(*this, "IDC_XCOORD_TIME", wxChoice);
    m_Y_time = XRCCTRL(*this, "IDC_YCOORD_TIME", wxChoice);
	m_X_time->Show(false);
	m_Y_time->Show(false);
    m_threshold = XRCCTRL(*this, "IDC_THRESHOLD_EDIT", wxTextCtrl);
    m_sliderdistance = XRCCTRL(*this, "IDC_THRESHOLD_SLIDER", wxSlider);
    m_radio2 = XRCCTRL(*this, "IDC_RADIO_QUEEN", wxRadioButton);
    m_radio1 = XRCCTRL(*this, "IDC_RADIO_ROOK", wxRadioButton);
    m_radio3 = XRCCTRL(*this, "IDC_RADIO_DISTANCE", wxRadioButton);
    m_radio4 = XRCCTRL(*this, "IDC_RADIO_KNN", wxRadioButton);
    m_neighbors = XRCCTRL(*this, "IDC_EDIT_KNN", wxTextCtrl);
    m_spinneigh = XRCCTRL(*this, "IDC_SPIN_KNN", wxSpinButton);
	
	InitDlg();
}

void CreatingWeightDlg::OnCreateNewIdClick( wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnCreateNewIdClick");
	
	AddIdVariable dlg(grid_base, this);
    if (dlg.ShowModal() == wxID_OK) {
		// We know that the new id has been added to the the table in memory
		m_field->Insert(dlg.GetIdVarName(), 0);
		m_field->SetSelection(0);
		EnableDistanceRadioButtons(m_field->GetSelection() != wxNOT_FOUND);
		EnableContiguityRadioButtons((m_field->GetSelection() != wxNOT_FOUND)
									 && !m_is_table_only);
		UpdateCreateButtonState();
	} else {
		// A new id was not added to the dbf file, so do nothing.
	}	
	LOG_MSG("Exiting CreatingWeightDlg::OnCreateNewIdClick");
}

void CreatingWeightDlg::OnCloseClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CLOSE);
}

void CreatingWeightDlg::OnCreateClick( wxCommandEvent& event )
{
	if (m_radio == -1) {
		wxString msg;
		msg << "Please select a weights type.";
		wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_radio == 3 || m_radio == 7) {
		if (!m_thres_val_valid) {
			wxString msg;
			msg << "The currently entered threshold value is not ";
			msg << "a valid number.  Please move the slider, or enter ";
			msg << "a valid number.";
			wxMessageDialog dlg(this, msg, "Error", wxOK | wxICON_ERROR);
			dlg.ShowModal();
			event.Skip();
			return;
		}
		if (m_threshold_val*m_thres_delta_factor < m_thres_min) {
			wxString msg;
			msg << "The currently entered threshold value of ";
			msg << m_threshold_val << " is less than ";
			msg << m_thres_min << " which is the minimum value for which ";
			msg << "there will be no neighborless observations (isolates). ";
			msg << "Press Yes to proceed anyhow, press No to abort.";
			wxMessageDialog dlg(this, msg, "Warning",
								wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if (dlg.ShowModal() != wxID_YES) {
				event.Skip();
				return;
			}
		}	
	}
	
	wxString wildcard;
	wxString defaultDir(project->GetMainDir());
	wxString defaultFile(project->GetMainName());
	LOG(defaultDir);
	LOG(defaultFile);
	if (IsSaveAsGwt()) {
		defaultFile += ".gwt";
		wildcard = "GWT files (*.gwt)|*.gwt";
	} else {
		defaultFile += ".gal";
		wildcard = "GAL files (*.gal)|*.gal";
	}
	
	wxFileDialog dlg(this,
                     "Choose an output weights file name.",
                     defaultDir,
					 defaultFile,
					 wildcard,
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	wxString outputfile;
    if (dlg.ShowModal() != wxID_OK) return;
	outputfile = dlg.GetPath();
	
	wxString id = wxEmptyString;
	if ( m_field->GetSelection() != wxNOT_FOUND ) {
		id = m_field->GetString(m_field->GetSelection());
	} else {
		return; // we must have key id variable
	}
	
	if ((m_field->GetSelection() != wxNOT_FOUND) && !CheckID(id)) return;
	std::vector<wxInt64> id_vec(m_num_obs);
	int col = grid_base->FindColId(id);
	grid_base->col_data[col]->GetVec(id_vec, 0);
	
	int m_ooC = m_spincont->GetValue();
	int m_kNN = m_spinneigh->GetValue();
	int m_alpha = 1;

	GalElement *gal = 0;
    GalElement *Hgal = 0;
	GwtElement *gwt = 0;
	bool done = false;

	m_method = m_distance_metric->GetSelection() + 1;
	
	wxString str_X = m_X->GetString(m_X->GetSelection());
	wxString str_Y = m_Y->GetString(m_Y->GetSelection());

	bool m_check1 = m_include_lower->GetValue();

	switch (m_radio)
	{
		case 3: // threshold distance
		{
			double t_val = m_threshold_val;
			if (t_val <= 0) t_val = std::numeric_limits<float>::min();
			if (t_val > 0) {
				gwt = shp2gwt(m_num_obs, m_XCOO, m_YCOO,
							  t_val * m_thres_delta_factor,
							  1, m_method);
				if (gwt == 0) return;
				
				Shp2GalProgress(0, gwt, project->GetFullShpName(),
								outputfile, id, id_vec);
				done = true;
				event.Skip();
			}
		}
			break;
			
		case 4: // k nn
		{
			if (m_kNN > 0 && m_kNN < m_num_obs) {
				gwt = DynKNN(m_XCOO, m_YCOO, m_kNN+1, m_method);
				if (gwt==0) return;
				
				Shp2GalProgress(0, gwt, project->GetFullShpName(), outputfile,
								id, id_vec);
				done = true;
				event.Skip();
				delete [] gwt;
				gwt = 0;
			} else {
				wxString s = wxString::Format("Error: Maximum # of neighbors "
											  "(%d) exceeded.",
											  (int) m_num_obs-1);
				wxMessageBox(s);
			}
		}
			break;
			
		case 5: // rook
		case 6: // queen
		{
			bool is_rook = (m_radio == 5);
			if (project->main_data.header.shape_type == Shapefile::POINT) {
				if (project->IsPointDuplicates()) {
					project->DisplayPointDupsWarning();
				}
				
				gal = project->GetVoronoiRookNeighborGal();
				if (!gal) {
					wxString msg("There was a problem generating voronoi "
								 "contiguity neighbors.  Please report this.");
					wxMessageDialog dlg(NULL, msg,
										"Voronoi Contiguity Error",
										wxOK | wxICON_ERROR);
					dlg.ShowModal();
					break;
				}
			} else {
				gal = shp2gal(project->GetFullShpName(),
							  (is_rook ? 1 : 0), true);
			}
			
			if (!gal) break;
			if (m_ooC > 1) {
				Hgal = HOContiguity(m_ooC, m_num_obs, gal, m_check1);
				Shp2GalProgress(Hgal, 0, project->GetFullShpName(), outputfile,
								id, id_vec);
				done = true;
				event.Skip();
			} else {
 		        Shp2GalProgress(gal, 0, project->GetFullShpName(), outputfile,
								id, id_vec);
				done = true;
				event.Skip();
			}
		}
			break;

		default:
			break;
	};
	
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
}

void CreatingWeightDlg::OnCRadioRookSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(5);
}

void CreatingWeightDlg::OnCRadioQueenSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(6);
}

void CreatingWeightDlg::EnableThresholdControls( bool b )
{
	// This either enable the Threshold distance controls.  This does not
	// affect the state of the Theshold button itself.
	FindWindow(XRCID("IDC_STATIC1"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC2"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC3"))->Enable(b);
	FindWindow(XRCID("IDC_XCOORDINATES"))->Enable(b);
	FindWindow(XRCID("IDC_YCOORDINATES"))->Enable(b);
	FindWindow(XRCID("IDC_DISTANCE_METRIC"))->Enable(b);
	FindWindow(XRCID("IDC_THRESHOLD_SLIDER"))->Enable(b);
	FindWindow(XRCID("IDC_THRESHOLD_EDIT"))->Enable(b);
	UpdateTmSelEnableState();
	if (!b) {
		m_X_time->Disable();
		m_Y_time->Disable();
	}
}

void CreatingWeightDlg::UpdateTmSelEnableState()
{
	int m_x_sel = m_X->GetSelection();
	if (!m_is_table_only) m_x_sel -= 2;
	if (m_is_space_time && m_x_sel >= 0) {
		int col = col_id_map[m_x_sel];
		m_X_time->Enable(grid_base->IsColTimeVariant(col));
	} else {
		m_X_time->Disable();
	}
	int m_y_sel = m_Y->GetSelection();
	if (!m_is_table_only) m_y_sel -= 2;
	if (m_is_space_time && m_y_sel >= 0) {
		int col = col_id_map[m_y_sel];
		m_Y_time->Enable(grid_base->IsColTimeVariant(col));
	} else {
		m_Y_time->Disable();
	}
}

void CreatingWeightDlg::SetRadioBtnAndAssocWidgets(int radio)
{
	// Updates the value of m_radio and disables
	// wigets associated with deslectd radio buttons.
	
	// Disable everything to begin.	
	FindWindow(XRCID("IDC_STATIC_OOC1"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY"))->Enable(false);
	FindWindow(XRCID("IDC_SPIN_ORDEROFCONTIGUITY"))->Enable(false);
	FindWindow(XRCID("IDC_CHECK1"))->Enable(false);
	EnableThresholdControls(false);
	FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(false);
	FindWindow(XRCID("IDC_EDIT_KNN"))->Enable(false);
    FindWindow(XRCID("IDC_SPIN_KNN"))->Enable(false);

	if ((radio == 7) || (radio == 3) || (radio == 4) ||
		(radio == 5) || (radio == 6)) {
		m_radio = radio;
	} else {
		m_radio = -1;
	}
	UpdateCreateButtonState();
	
	switch (m_radio) {
		case 6: // queen
		case 5: { // rook
			FindWindow(XRCID("IDC_STATIC_OOC1"))->Enable(true);
			FindWindow(XRCID("IDC_EDIT_ORDEROFCONTIGUITY"))->Enable(true);
			FindWindow(XRCID("IDC_SPIN_ORDEROFCONTIGUITY"))->Enable(true);
			FindWindow(XRCID("IDC_CHECK1"))->Enable(true);
		}
			break;
		case 3: { // threshold distance
			EnableThresholdControls(true);
		}
			break;
		case 4: { // k-nn
			FindWindow(XRCID("IDC_STATIC1"))->Enable(true);
			FindWindow(XRCID("IDC_STATIC2"))->Enable(true);
			FindWindow(XRCID("IDC_STATIC3"))->Enable(true);
			FindWindow(XRCID("IDC_XCOORDINATES"))->Enable(true);
			FindWindow(XRCID("IDC_YCOORDINATES"))->Enable(true);
			m_distance_metric->Enable(true);
			m_distance_metric->SetSelection(0);
			m_method = 1;
			m_distance_metric->Enable(false);
			
			FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(true);
			FindWindow(XRCID("IDC_EDIT_KNN"))->Enable(true);
			FindWindow(XRCID("IDC_SPIN_KNN"))->Enable(true);
			UpdateTmSelEnableState();
		}
			break;
		default:
			break;
	}
}

// This function is only called when one of the choices that affects
// the entire threshold scale is changed.  This function will use
// the current position of the slider
void CreatingWeightDlg::UpdateThresholdValues()
{
	LOG_MSG("Entering CreatingWeightDlg::UpdateThresholdValues");
	if (!all_init) return;
	int sl_x, sl_y;
	m_sliderdistance->GetPosition(&sl_x, &sl_y);
	wxSize sl_size = m_sliderdistance->GetSize();
	m_sliderdistance->SetSize(sl_x, sl_y, 500, sl_size.GetHeight());
	
	if (m_X->GetSelection() == wxNOT_FOUND ||
		m_Y->GetSelection() == wxNOT_FOUND) return;
	wxString mm_x = m_X->GetString(m_X->GetSelection());
	wxString mm_y = m_Y->GetString(m_Y->GetSelection());
	wxString v1 = mm_x;
	wxString v2 = mm_y;
	
	bool mean_center = false;
	if (mm_x == "<X-Centroids>") {
		v1 = wxEmptyString;
	}
	if (mm_y == "<Y-Centroids>") {
		v2 = wxEmptyString;
	}
	if (mm_x == "<X-Mean-Centers>") {
		v1 = wxEmptyString;
		mean_center = true;
	}
	if (mm_y == "<Y-Mean-Centers>") {
		v2 = wxEmptyString;
		mean_center = true;
	}
	if (v1 == wxEmptyString || v2 == wxEmptyString) {
		if (mean_center) {
			project->GetMeanCenters(m_XCOO, m_YCOO);
		} else {
			project->GetCentroids(m_XCOO, m_YCOO);
		}
	}
	if (v1 != wxEmptyString || v1 != wxEmptyString) {
		if (v1 != wxEmptyString) {
			int x_sel = (m_is_table_only ? 
						 m_X->GetSelection() : m_X->GetSelection()-2);
			int col_id = col_id_map[x_sel];
			int tm = 0;
			if (m_is_space_time &&
				grid_base->IsColTimeVariant(col_id)) {
				tm = m_X_time->GetSelection();
			}
			grid_base->col_data[col_id]->GetVec(m_XCOO, tm);
		}
		if (v2 != wxEmptyString) {
			int y_sel = (m_is_table_only ? 
						 m_Y->GetSelection() : m_Y->GetSelection()-2);
			int col_id = col_id_map[y_sel];
			int tm = 0;
			if (m_is_space_time &&
				grid_base->IsColTimeVariant(col_id)) {
				tm = m_Y_time->GetSelection();
			}
			grid_base->col_data[col_id]->GetVec(m_YCOO, tm);
		}
	}
	
	m_thres_min = ComputeCutOffPoint(m_XCOO, m_YCOO, m_method);
	m_thres_max = ComputeMaxDistance(m_XCOO, m_YCOO, m_method);
	LOG(m_thres_min);
	LOG(m_thres_max);
	m_threshold_val = (m_sliderdistance->GetValue() *
					   (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_thres_val_valid = true;
	m_threshold->ChangeValue( wxString::Format("%f", m_threshold_val));
	LOG_MSG("Exiting CreatingWeightDlg::UpdateThresholdValues");
}

void CreatingWeightDlg::OnCThresholdTextEdit( wxCommandEvent& event )
{
	if (!all_init) return;
	LOG_MSG("In CreatingWeightDlg::OnCThresholdTextEdit");
	wxString val = m_threshold->GetValue();
	val.Trim(false);
	val.Trim(true);
	double t = m_threshold_val;
	m_thres_val_valid = val.ToDouble(&t);
	if (m_thres_val_valid) {
		m_threshold_val = t;
		if (t <= m_thres_min) {
			m_sliderdistance->SetValue(0);
		} else if (t >= m_thres_max) {
			m_sliderdistance->SetValue(100);
		} else {
			double s = (t-m_thres_min)/(m_thres_max-m_thres_min) * 100;
			m_sliderdistance->SetValue((int) s);
		}
	}
}

void CreatingWeightDlg::OnCThresholdSliderUpdated( wxCommandEvent& event )
{
	if (!all_init) return;
	bool m_rad_inv_dis_val = false;
	
	m_threshold_val = (m_sliderdistance->GetValue() *
					   (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_threshold->ChangeValue( wxString::Format("%f", (double) m_threshold_val));
	if (m_threshold_val > 0)  {
	    FindWindow(XRCID("wxID_OK"))->Enable(true);
	}
}

void CreatingWeightDlg::OnCRadioDistanceSelected( wxCommandEvent& event )
{
	// Threshold Distance radio button selected
	SetRadioBtnAndAssocWidgets(3);
	UpdateThresholdValues();
}


void CreatingWeightDlg::OnCRadioGeoDaLSelected( wxCommandEvent& event )
{
	// do nothing for now, perhaps will force save as GAL in future.
    event.Skip();
}

void CreatingWeightDlg::OnCRadioKnnSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(4);
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event )
{
	wxString val;
	val << m_spincont->GetValue();
    m_contiguity->SetValue(val);
}

void CreatingWeightDlg::OnCSpinKnnUpdated( wxSpinEvent& event )
{
	wxString val;
	val << m_spinneigh->GetValue();
	m_neighbors->SetValue(val);
}

// updates the enable/disable state of the Create button based
// on the values of various other controls.
void CreatingWeightDlg::UpdateCreateButtonState()
{
	bool enable = true;

	// Check that a Weights File ID variable is selected.
	if (m_field->GetSelection() == wxNOT_FOUND) enable = false;
	// Check that a weight type radio button choice is selected.
	if (m_radio == -1) enable = false;
	if (m_X->GetSelection() == wxNOT_FOUND ||
		m_Y->GetSelection() == wxNOT_FOUND) enable = false;
	
	FindWindow(XRCID("wxID_OK"))->Enable(enable);
}

void CreatingWeightDlg::EnableContiguityRadioButtons(bool b)
{
	FindWindow(XRCID("IDC_RADIO_ROOK"))->Enable(b);
	FindWindow(XRCID("IDC_RADIO_QUEEN"))->Enable(b);
}

void CreatingWeightDlg::EnableDistanceRadioButtons(bool b)
{
	FindWindow(XRCID("IDC_RADIO_DISTANCE"))->Enable(b);
	FindWindow(XRCID("IDC_RADIO_KNN"))->Enable(b);
}

void CreatingWeightDlg::ClearRadioButtons()
{
	m_radio1->SetValue(false);
	m_radio2->SetValue(false);
	m_radio3->SetValue(false);
	m_radio4->SetValue(false);
	m_radio = -1;
}

void CreatingWeightDlg::ResetThresXandYCombo()
{
	m_X->Clear();
	if (!m_is_table_only) {
		m_X->Append("<X-Centroids>");
		m_X->Append("<X-Mean-Centers>");
		m_X->SetSelection(0);
		m_Y->Clear();
		m_Y->Append("<Y-Centroids>");
		m_Y->Append("<Y-Mean-Centers>");
		m_Y->SetSelection(0);
	}
}

void CreatingWeightDlg::InitFields()
{
	m_field->Clear();
	m_X_time->Clear();
	m_Y_time->Clear();
	ResetThresXandYCombo();
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		if (m_is_space_time && grid_base->IsColTimeVariant(col)) {
			wxString t;
			t << " (" << grid_base->time_ids[0] << ")";
			m_X->Append(grid_base->col_data[col_id_map[i]]->name + t);
			m_Y->Append(grid_base->col_data[col_id_map[i]]->name + t);
		} else {
			m_X->Append(grid_base->col_data[col_id_map[i]]->name);
			m_Y->Append(grid_base->col_data[col_id_map[i]]->name);
		}
		if (!m_is_space_time &&
			grid_base->col_data[col_id_map[i]]->type
				== GeoDaConst::long64_type) {
			m_field->Append(grid_base->col_data[col_id_map[i]]->name);
		}
	}
	if (m_is_space_time) {
		m_field->Append(grid_base->GetSpTblSpColName());
		m_field->SetSelection(0);
		EnableDistanceRadioButtons(true);
		EnableContiguityRadioButtons(true && !m_is_table_only);
		UpdateCreateButtonState();
		FindWindow(XRCID("ID_CREATE_ID"))->Enable(false);
			
		for (int i=0; i<grid_base->time_steps; i++) {
			wxString t;
			t << grid_base->time_ids[i];
			m_X_time->Append(t);
			m_Y_time->Append(t);
		}
		m_X_time->SetSelection(0);
		m_Y_time->SetSelection(0);
	}
	
	if (!m_is_table_only) {
		m_X->SetSelection(0);
		m_Y->SetSelection(0);
	}
	m_X_time->Disable();
	m_Y_time->Disable();
}

void CreatingWeightDlg::UpdateFieldNamesTm()
{
	if (!m_is_space_time) return;
	int x_sel = m_X->GetSelection();
	int y_sel = m_Y->GetSelection();
	ResetThresXandYCombo();
	wxString x_tm_str;
	x_tm_str << " (" << grid_base->time_ids[m_X_time->GetSelection()] << ")";
	wxString y_tm_str;
	y_tm_str << " (" << grid_base->time_ids[m_Y_time->GetSelection()] << ")";
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		if (grid_base->IsColTimeVariant(col)) {
			m_X->Append(grid_base->col_data[col_id_map[i]]->name + x_tm_str);
			m_Y->Append(grid_base->col_data[col_id_map[i]]->name + y_tm_str);
		} else {
			m_X->Append(grid_base->col_data[col_id_map[i]]->name);
			m_Y->Append(grid_base->col_data[col_id_map[i]]->name);
		}
	}
	m_X->SetSelection(x_sel);
	m_Y->SetSelection(y_sel);
}

bool CreatingWeightDlg::CheckID(const wxString& id)
{
	std::vector<wxInt64> id_vec(m_num_obs);
	int col = grid_base->FindColId(id);
	grid_base->col_data[col]->GetVec(id_vec, 0);
	std::set<wxInt64> id_set;
	for (int i=0, iend=id_vec.size(); i<iend; i++) {
		id_set.insert(id_vec[i]);
	}
	if (id_vec.size() != id_set.size()) {
		wxString msg = id + " has duplicate values.  Please choose ";
		msg += "a different ID Variable.";
		wxMessageBox(msg);
		return false;
	}
	return true;
}

void CreatingWeightDlg::InitDlg()
{
	m_field->Clear();
	m_contiguity->SetValue( "1");
	ResetThresXandYCombo();
	m_sliderdistance->SetRange(0, 100);
	m_threshold->SetValue( "0.0");
	m_spincont->SetRange(1,10);
	m_spincont->SetValue(1);
	m_spinneigh->SetRange(1,10);
	m_spinneigh->SetValue(4);
	m_neighbors->SetValue( "4");
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	FindWindow(XRCID("ID_ID_VAR_STAT_TXT"))->Enable(false);
	FindWindow(XRCID("IDC_IDVARIABLE"))->Enable(false);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(false);
	m_distance_metric->Clear();
	m_distance_metric->Append("<Euclidean Distance>");
	m_distance_metric->Append("<Arc Distance (miles)>");
	m_distance_metric->SetSelection(0);
	ClearRadioButtons();
	SetRadioBtnAndAssocWidgets(-1);
	EnableContiguityRadioButtons(false);
	EnableDistanceRadioButtons(false);
	
	// Previously from OpenShapefile:
	FindWindow(XRCID("ID_ID_VAR_STAT_TXT"))->Enable(true);
	FindWindow(XRCID("IDC_IDVARIABLE"))->Enable(true);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(true);
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	
	col_id_map.clear();
	grid_base->FillNumericColIdMap(col_id_map);
	
	InitFields();
	int sl_x, sl_y;
	m_sliderdistance->GetPosition(&sl_x, &sl_y);
	wxSize sl_size = m_sliderdistance->GetSize();
	m_sliderdistance->SetSize(sl_x, sl_y, 500, sl_size.GetHeight());
	
	m_X_time->Show(m_is_space_time);
	m_Y_time->Show(m_is_space_time);
	m_spincont->SetRange(1, (int) m_num_obs / 2);
	m_spinneigh->SetRange(1, (int) m_num_obs - 1);
	
	if (m_radio1->GetValue()) m_radio = 5;
	else if (m_radio2->GetValue()) m_radio = 6;
	else if (m_radio3->GetValue()) m_radio = 3;
	else if (m_radio4->GetValue()) m_radio = 4;
	
	m_XCOO.resize(m_num_obs);
	m_YCOO.resize(m_num_obs);
	Refresh();
}

bool CreatingWeightDlg::IsSaveAsGwt()
{
	// determine if save type will be GWT or GAL.
	// m_radio values:
	// 3 - threshold distance - GWT
	// 4 - k-nn - GWT
	// 5 - rook - GAL
	// 6 - queen - GAL
	return 	!(m_radio == 5 || m_radio == 6);	
}

void CreatingWeightDlg::OnXSelected(wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnXSelected");
	if ( m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" && 
		m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" ) {
		m_Y->SetSelection(0);
	}
	if ( m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" && 
		m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" ) {
		m_Y->SetSelection(1);
	}
	UpdateTmSelEnableState();
	UpdateThresholdValues();
	UpdateCreateButtonState();
	LOG_MSG("Exiting CreatingWeightDlg::OnXSelected");	
}

void CreatingWeightDlg::OnYSelected(wxCommandEvent& event )
{
	LOG_MSG("Entering CreatingWeightDlg::OnYSelected");
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" && 
		m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" ) {
		m_X->SetSelection(0);
	}
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" && 
		m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" ) {
		m_X->SetSelection(1);
	}
	UpdateTmSelEnableState();
	UpdateThresholdValues();
	UpdateCreateButtonState();
	LOG_MSG("Exiting CreatingWeightDlg::OnYSelected");
}

void CreatingWeightDlg::OnXTmSelected(wxCommandEvent& event )
{
	UpdateFieldNamesTm();
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnYTmSelected(wxCommandEvent& event )
{
	UpdateFieldNamesTm();
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnDistanceMetricSelected(wxCommandEvent& event )
{
	m_method = m_distance_metric->GetSelection() + 1;
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnIdVariableSelected( wxCommandEvent& event )
{
	EnableDistanceRadioButtons(m_field->GetSelection() != wxNOT_FOUND);
	EnableContiguityRadioButtons((m_field->GetSelection() != wxNOT_FOUND) &&
								 !m_is_table_only);
	UpdateCreateButtonState();	
}

bool CreatingWeightDlg::Shp2GalProgress(GalElement *gl, GwtElement *gw,
										const wxString& ifn,
										const wxString& ofn,
										const wxString& idd,
										const std::vector<wxInt64>& id_vec)
{
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(false);

	bool success = false;
	bool flag = false;
	bool geodaL=true; // always save as "Legacy" format.
	if (gl) // gal
		flag = SaveGal(gl, ifn, ofn, idd, id_vec);
	else if (m_radio == 3) // binary distance
		flag = WriteGwt(gw, ifn, ofn, idd, id_vec, 1, geodaL );
	else if (m_radio == 4) // kNN
		flag = WriteGwt(gw, ifn, ofn, idd, id_vec, -2, geodaL);
	else flag = false;

	if (!flag) {
		wxString msg("Failed to create the weights file.");
		wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
	} else {
		wxFileName t_ofn(ofn);
		wxString file_name(t_ofn.GetFullName());
		
		wxString msg = wxEmptyString;
		msg = "Weights file \"" + file_name + "\" created successfully.";
		wxMessageDialog dlg(NULL, msg, "Success", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		success = true;
	}

	if (success) {
		wxFileName t_ofn(ofn);
		wxString ext = t_ofn.GetExt().Lower();
		if (ext != "gal" && ext != "gwt") {
			LOG_MSG("File extention not gal or gwt");
		} else {
			WeightsManager* w_manager = project->GetWManager();
			
			int obs = w_manager->GetNumObservations();
			if (ext == "gal") {
				GalElement* tempGal=WeightUtils::ReadGal(ofn, grid_base);
				if (tempGal != 0) {
					GalWeight* w = new GalWeight();
					w->num_obs = obs;
					w->wflnm = ofn;
					w->gal = tempGal;
					if (!w_manager->AddWeightFile(w, true)) {
						delete w;
						success = false;
					}
				}
			} else { // ext == "gwt"
				GalElement* tempGal=WeightUtils::ReadGwtAsGal(ofn, grid_base);
				if (tempGal != 0) {
					GalWeight* w = new GalWeight();
					w->num_obs = obs;
					w->wflnm = ofn;
					w->gal = tempGal;
					if (!w_manager->AddWeightFile(w, true)) {
						delete w;
						success = false;
					}
				}
			}
		}
	}
	
	if (gl) delete [] gl;
	if (gw) delete [] gw;
	
	FindWindow(XRCID("wxID_OK"))->Enable(true);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
	return success;
}


