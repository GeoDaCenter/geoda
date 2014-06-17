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
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
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
    EVT_CHECKBOX( XRCID("IDC_PRECISION_CBX"),
                 CreatingWeightDlg::OnPrecisionThresholdCheck)
END_EVENT_TABLE()


CreatingWeightDlg::CreatingWeightDlg(wxWindow* parent,
									 Project* project_s,
									 wxWindowID id,
									 const wxString& caption,
									 const wxPoint& pos, 
									 const wxSize& size,
									 long style )
: all_init(false), m_thres_delta_factor(1.00001),
m_method(1), m_arc_in_km(false), m_thres_val_valid(false),
m_threshold_val(0.01),
project(project_s), table_int(project_s->GetTableInt()),
m_num_obs(project_s->GetNumRecords()),
m_cbx_precision_threshold_first_click(true)
{
	Create(parent, id, caption, pos, size, style);
	all_init = true;
}

bool CreatingWeightDlg::Create( wxWindow* parent, wxWindowID id,
								const wxString& caption, const wxPoint& pos,
								const wxSize& size, long style )
{
    m_id_field = 0;
    m_radio2 = 0;
    m_contiguity = 0;
    m_spincont = 0;
    m_radio1 = 0;
    m_include_lower = 0;
    m_txt_precision_threshold = 0;
    m_cbx_precision_threshold = 0;
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
    m_id_field = XRCCTRL(*this, "IDC_IDVARIABLE", wxChoice);
    m_contiguity = XRCCTRL(*this, "IDC_EDIT_ORDEROFCONTIGUITY", wxTextCtrl);
    m_spincont = XRCCTRL(*this, "IDC_SPIN_ORDEROFCONTIGUITY", wxSpinButton);
    m_include_lower = XRCCTRL(*this, "IDC_CHECK1", wxCheckBox);
    m_cbx_precision_threshold= XRCCTRL(*this, "IDC_PRECISION_CBX", wxCheckBox);
    m_distance_metric = XRCCTRL(*this, "IDC_DISTANCE_METRIC", wxChoice);
    m_X = XRCCTRL(*this, "IDC_XCOORDINATES", wxChoice);
    m_Y = XRCCTRL(*this, "IDC_YCOORDINATES", wxChoice);
	m_X_time = XRCCTRL(*this, "IDC_XCOORD_TIME", wxChoice);
    m_Y_time = XRCCTRL(*this, "IDC_YCOORD_TIME", wxChoice);
	m_X_time->Show(false);
	m_Y_time->Show(false);
    m_threshold = XRCCTRL(*this, "IDC_THRESHOLD_EDIT", wxTextCtrl);
    m_txt_precision_threshold = XRCCTRL(*this, "IDC_PRECISION_THRESHOLD_EDIT",
                                    wxTextCtrl);
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

	AddIdVariable dlg(table_int, this);
    if (dlg.ShowModal() == wxID_OK) {
		// We know that the new id has been added to the the table in memory
		m_id_field->Insert(dlg.GetIdVarName(), 0);
		m_id_field->SetSelection(0);
		EnableDistanceRadioButtons(m_id_field->GetSelection() != wxNOT_FOUND);
		EnableContiguityRadioButtons((m_id_field->GetSelection() != wxNOT_FOUND)
									 && !project->IsTableOnlyProject());
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
			if (dlg.ShowModal() != wxID_YES) return;
		}
	}
	
	wxString wildcard;
	wxString defaultFile(project->GetProjectTitle());
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
                     "",
					 defaultFile,
					 wildcard,
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	wxString outputfile;
    if (dlg.ShowModal() != wxID_OK) return;
	outputfile = dlg.GetPath();
	
	wxString id = wxEmptyString;
	if ( m_id_field->GetSelection() != wxNOT_FOUND ) {
		id = m_id_field->GetString(m_id_field->GetSelection());
	} else {
		return; // we must have key id variable
	}
	
	if ((m_id_field->GetSelection() != wxNOT_FOUND) && !CheckID(id)) return;
	std::vector<wxInt64> id_vec(m_num_obs);
	int col = table_int->FindColId(id);
	table_int->GetColData(col, 0, id_vec);
	
	int m_ooC = m_spincont->GetValue();
	int m_kNN = m_spinneigh->GetValue();
	int m_alpha = 1;

	GalElement *gal = 0;
    GalElement *Hgal = 0;
	GwtElement *gwt = 0;
	bool done = false;
	
	wxString str_X = m_X->GetString(m_X->GetSelection());
	wxString str_Y = m_Y->GetString(m_Y->GetSelection());

	bool m_check1 = m_include_lower->GetValue();

	switch (m_radio)
	{
		case 3: // threshold distance
		{
			double t_val = m_threshold_val;
			if (t_val <= 0) t_val = std::numeric_limits<float>::min();
			if (m_method == 2 && m_arc_in_km) {
				t_val /= 1.609344; // convert km to mi
			}
			if (t_val > 0) {
				gwt = shp2gwt(m_num_obs, m_XCOO, m_YCOO,
							  t_val * m_thres_delta_factor,
							  1, m_method);
				if (gwt == 0) {
					wxString m;
					m << "No weights file was created due to all observations ";
					m << "being isolates for the specified threshold value.  ";
					m << "Increase the threshold to create a ";
					m << "non-empty weights file.";
					wxMessageDialog dlg(this, m, "Error", wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
				
				Shp2GalProgress(0, gwt,
								project->GetProjectTitle(), outputfile,
								id, id_vec);
				if (gwt) delete [] gwt; gwt = 0;
				done = true;
			}
		}
			break;
			
		case 4: // k nn
		{
			if (m_kNN > 0 && m_kNN < m_num_obs) {
				gwt = DynKNN(m_XCOO, m_YCOO, m_kNN+1, m_method);
				if (gwt==0) return;
				
				Shp2GalProgress(0, gwt,
								project->GetProjectTitle(), outputfile,
								id, id_vec);
				if (gwt) delete [] gwt; gwt = 0;
				done = true;
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
				
				std::vector<std::set<int> > nbr_map;
				if (is_rook) {
					project->GetVoronoiRookNeighborMap(nbr_map);
				} else {
					project->GetVoronoiQueenNeighborMap(nbr_map);
				}
				gal = Gda::VoronoiUtils::NeighborMapToGal(nbr_map);
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
                double precision_threshold = 0.0;
                if ( m_cbx_precision_threshold->IsChecked()) {
                    if (!m_txt_precision_threshold->IsEmpty()) {
                        wxString prec_thres =
                            m_txt_precision_threshold->GetValue();
                        double value;
                        if ( prec_thres.ToDouble(&value) )
                            precision_threshold = value;
                    } else {
                        precision_threshold = 0.0;
                    }
                }
				gal = shp2gal(project->main_data, (is_rook ? 1 : 0), true,
                              precision_threshold);
			}
			
			if (!gal) {
                // could be an empty weights file, and should prompt user
                // to setup Precision Threshold
                wxString msg("GeoDa can't find weights information from "
                             "shapes.  You can try to set precision "
                             "threshold value to find neighbor shapes "
                             "using fuzzy matching approach.");
                wxMessageDialog dlg(NULL, msg,
                                    "Empty Contiguity Weights Created",
                                    wxOK | wxICON_WARNING);
                dlg.ShowModal();
                
                m_cbx_precision_threshold->SetValue(true);
                m_txt_precision_threshold->Enable(true);
                // give a suggested value
                double shp_min_x = (double)project->main_data.header.bbox_x_min;
                double shp_max_x = (double)project->main_data.header.bbox_x_max;
                double shp_min_y = (double)project->main_data.header.bbox_y_min;
                double shp_max_y = (double)project->main_data.header.bbox_y_max;
                double shp_x_len = shp_max_x - shp_min_x;
                double shp_y_len = shp_max_y - shp_min_y;
                double pixel_len = MIN(shp_x_len, shp_y_len) / 4096.0; // 4K LCD
                double suggest_precision = pixel_len * 10E-7;
                // round it to power of 10
                suggest_precision = log10(suggest_precision);
                suggest_precision = ceil(suggest_precision);
                suggest_precision = pow(10, suggest_precision);
                wxString tmpTxt;
                tmpTxt << suggest_precision;
                m_txt_precision_threshold->SetValue(tmpTxt);
                break;
            }
			if (m_ooC > 1) {
				Hgal = HOContiguity(m_ooC, m_num_obs, gal, m_check1);
				Shp2GalProgress(Hgal, 0,
								project->GetProjectTitle(), outputfile,
								id, id_vec);
			} else {
 		        Shp2GalProgress(gal, 0,
								project->GetProjectTitle(), outputfile,
								id, id_vec);
			}
			if (Hgal) delete [] Hgal; Hgal = 0;
			if (gal) delete [] gal; gal = 0;
			done = true;
		}
			break;

		default:
			break;
	};
	
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
}

void CreatingWeightDlg::OnPrecisionThresholdCheck( wxCommandEvent& event )
{
	if (m_cbx_precision_threshold_first_click) {
		// Show a warning message regarding the use of this function
		wxString msg;
		msg << "Precision threshold should normally be set to 0.  ";
		msg << "For data sources with neighboring polygons whose vertex ";
		msg << "coordinates do not match exactly, a very small positive ";
		msg << "threshold value can be chosen so that neighboring polygons ";
		msg << "are correctly identified.  This is an advanced option. ";
		msg << "It is important to visually verify the resulting weights ";
		msg << "to ensure there are no false-positives.";
		wxMessageDialog dlg(NULL, msg, "About Precision Threshold",
							wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		m_cbx_precision_threshold_first_click = false;
	}
    if ( m_cbx_precision_threshold->IsChecked() ) {
        m_txt_precision_threshold->Enable(true);
        m_cbx_precision_threshold->SetValue(true);
    } else {
        m_txt_precision_threshold->Enable(false);
        m_cbx_precision_threshold->SetValue(false);
    }
}

void CreatingWeightDlg::OnCRadioRookSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(5);
}

void CreatingWeightDlg::OnCRadioQueenSelected( wxCommandEvent& event )
{
	SetRadioBtnAndAssocWidgets(6);
}

void CreatingWeightDlg::update(TableState* o)
{
	LOG_MSG("In CreatingWeightDlg::update(TableState* o)");
	Refresh();
}

void CreatingWeightDlg::EnableThresholdControls( bool b )
{
	// This either enable the Threshold distance controls.  This does not
	// affect the state of the Theshold button itself.
	FindWindow(XRCID("IDC_STATIC1"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC2"))->Enable(b);
	FindWindow(XRCID("IDC_STATIC3"))->Enable(b);
	m_X->Enable(b);
	m_Y->Enable(b);
	m_distance_metric->Enable(b);
	m_sliderdistance->Enable(b);
	m_threshold->Enable(b);
	UpdateTmSelEnableState();
	if (!b) {
		m_X_time->Disable();
		m_Y_time->Disable();
	}
}

void CreatingWeightDlg::UpdateTmSelEnableState()
{
	int m_x_sel = m_X->GetSelection();
	if (!project->IsTableOnlyProject()) m_x_sel -= 2;
	if (table_int->IsTimeVariant() && m_x_sel >= 0) {
		int col = col_id_map[m_x_sel];
		m_X_time->Enable(table_int->IsColTimeVariant(col));
	} else {
		m_X_time->Disable();
	}
	int m_y_sel = m_Y->GetSelection();
	if (!project->IsTableOnlyProject()) m_y_sel -= 2;
	if (table_int->IsTimeVariant() && m_y_sel >= 0) {
		int col = col_id_map[m_y_sel];
		m_Y_time->Enable(table_int->IsColTimeVariant(col));
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
	m_contiguity->Enable(false);
	m_spincont->Enable(false);
    m_cbx_precision_threshold->Enable(false);
	m_include_lower->Enable(false);
	EnableThresholdControls(false);
	FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(false);
	m_neighbors->Enable(false);
    m_spinneigh->Enable(false);

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
			m_contiguity->Enable(true);
			m_spincont->Enable(true);
            m_cbx_precision_threshold->Enable(true);
			m_include_lower->Enable(true);
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
			m_X->Enable(true);
			m_Y->Enable(true);
			SetDistMetricEuclid(true);
			m_distance_metric->Enable(false);
			
			FindWindow(XRCID("IDC_STATIC_KNN"))->Enable(true);
			m_neighbors->Enable(true);
			m_spinneigh->Enable(true);
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
			int x_sel = (project->IsTableOnlyProject() ? 
						 m_X->GetSelection() : m_X->GetSelection()-2);
			int col_id = col_id_map[x_sel];
			int tm = 0;
			if (table_int->IsTimeVariant() &&
				table_int->IsColTimeVariant(col_id)) {
				tm = m_X_time->GetSelection();
			}
			table_int->GetColData(col_id, tm, m_XCOO);
		}
		if (v2 != wxEmptyString) {
			int y_sel = (project->IsTableOnlyProject() ? 
						 m_Y->GetSelection() : m_Y->GetSelection()-2);
			int col_id = col_id_map[y_sel];
			int tm = 0;
			if (table_int->IsTimeVariant() &&
				table_int->IsColTimeVariant(col_id)) {
				tm = m_Y_time->GetSelection();
			}
			table_int->GetColData(col_id, tm, m_YCOO);
		}
	}
	
	m_thres_min = ComputeCutOffPoint(m_XCOO, m_YCOO, m_method);
	m_thres_max = ComputeMaxDistance(m_XCOO, m_YCOO, m_method);
	if (m_method == 2 && m_arc_in_km) {
		m_thres_min *= 1.609344; // convert mi to km
		m_thres_max *= 1.609344;
	}
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
	if (m_id_field->GetSelection() == wxNOT_FOUND) enable = false;
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
	m_Y->Clear();
	if (!project->IsTableOnlyProject()) {
		m_X->Append("<X-Centroids>");
		m_X->Append("<X-Mean-Centers>");
		m_X->SetSelection(0);
		m_Y->Append("<Y-Centroids>");
		m_Y->Append("<Y-Mean-Centers>");
		m_Y->SetSelection(0);
	}
}

void CreatingWeightDlg::InitFields()
{
	m_id_field->Clear();
	m_X_time->Clear();
	m_Y_time->Clear();
	ResetThresXandYCombo();
	
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		
		m_X->Append(table_int->GetColName(col));
		m_Y->Append(table_int->GetColName(col));
		
		if (table_int->GetColType(col) == GdaConst::long64_type) {
			if (!table_int->IsColTimeVariant(col)) {
				m_id_field->Append(table_int->GetColName(col));
			}
		}
	}
	m_id_field->SetSelection(-1);
	
	if (table_int->IsTimeVariant()) {
		std::vector<wxString> tm_strs;
		table_int->GetTimeStrings(tm_strs);
		for (int t=0, sz=tm_strs.size(); t<sz; ++t) {
			m_X_time->Append(tm_strs[t]);
			m_Y_time->Append(tm_strs[t]);
		}
	}
	
	if (!project->IsTableOnlyProject()) {
		m_X->SetSelection(0);
		m_Y->SetSelection(0);
	}
	
	m_X_time->Disable();
	m_Y_time->Disable();
}

bool CreatingWeightDlg::CheckID(const wxString& id)
{
	std::vector<wxInt64> id_vec(m_num_obs);
	int col = table_int->FindColId(id);
	table_int->GetColData(col, 0, id_vec);
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
	m_id_field->Clear();
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
	m_id_field->Enable(false);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(false);
	m_distance_metric->Clear();
	m_distance_metric->Append("Euclidean Distance");
	m_distance_metric->Append("Arc Distance (mi)");
	m_distance_metric->Append("Arc Distance (km)");
	m_distance_metric->SetSelection(0);
	ClearRadioButtons();
	SetRadioBtnAndAssocWidgets(-1);
	EnableContiguityRadioButtons(false);
	EnableDistanceRadioButtons(false);
	
	// Previously from OpenShapefile:
	FindWindow(XRCID("ID_ID_VAR_STAT_TXT"))->Enable(true);
	m_id_field->Enable(true);
	FindWindow(XRCID("ID_CREATE_ID"))->Enable(true);
	FindWindow(XRCID("wxID_OK"))->Enable(false);
    m_cbx_precision_threshold->Enable(false);
    m_txt_precision_threshold->Enable(false);
	
	col_id_map.clear();
	table_int->FillNumericColIdMap(col_id_map);
	
	InitFields();
	int sl_x, sl_y;
	m_sliderdistance->GetPosition(&sl_x, &sl_y);
	wxSize sl_size = m_sliderdistance->GetSize();
	m_sliderdistance->SetSize(sl_x, sl_y, 500, sl_size.GetHeight());
	
	m_X_time->Show(table_int->IsTimeVariant());
	m_Y_time->Show(table_int->IsTimeVariant());
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
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnYTmSelected(wxCommandEvent& event )
{
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnDistanceMetricSelected(wxCommandEvent& event )
{
	wxString s = m_distance_metric->GetStringSelection();
	if (s == "Euclidean Distance") {
		SetDistMetricEuclid(false);
		UpdateThresholdValues();
	} else if (s == "Arc Distance (mi)") {
		SetDistMetricArcMiles(false);
		UpdateThresholdValues();
	} else if (s == "Arc Distance (km)") {
		SetDistMetricArcKms(false);
		UpdateThresholdValues();
	}
}

void CreatingWeightDlg::SetDistMetricEuclid(bool update_sel)
{
	if (update_sel) m_distance_metric->SetSelection(0);
	m_method = 1;
	m_arc_in_km = false;
}

void CreatingWeightDlg::SetDistMetricArcMiles(bool update_sel)
{
	if (update_sel) m_distance_metric->SetSelection(1);
	m_method = 2;
	m_arc_in_km = false;
}

void CreatingWeightDlg::SetDistMetricArcKms(bool update_sel)
{
	if (update_sel) m_distance_metric->SetSelection(2);
	m_method = 2;
	m_arc_in_km = true;
}


void CreatingWeightDlg::OnIdVariableSelected( wxCommandEvent& event )
{
	EnableDistanceRadioButtons(m_id_field->GetSelection() != wxNOT_FOUND);
	EnableContiguityRadioButtons((m_id_field->GetSelection() != wxNOT_FOUND) &&
								 !project->IsTableOnlyProject());
	UpdateCreateButtonState();	
}

/** layer_name: layer name
 * ofn: output file name
 * idd: id column name
 * id: id column vector
 */
bool CreatingWeightDlg::Shp2GalProgress(GalElement *gal, GwtElement *gwt,
										const wxString& layer_name,
										const wxString& ofn,
										const wxString& idd,
										const std::vector<wxInt64>& id_vec)
{
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(false);

	bool success = false;
	bool flag = false;
	bool geodaL=true; // always save as "Legacy" format.
	if (gal) // gal
		flag = SaveGal(gal, layer_name, ofn, idd, id_vec);
	else if (m_radio == 3) // binary distance
		flag = WriteGwt(gwt, layer_name, ofn, idd, id_vec, 1, geodaL );
	else if (m_radio == 4) // kNN
		flag = WriteGwt(gwt, layer_name, ofn, idd, id_vec, -2, geodaL);
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
				GalElement* tempGal=WeightUtils::ReadGal(ofn, table_int);
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
				GalElement* tempGal=WeightUtils::ReadGwtAsGal(ofn, table_int);
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
		
	FindWindow(XRCID("wxID_OK"))->Enable(true);
	FindWindow(XRCID("wxID_CLOSE"))->Enable(true);
	return success;
}


