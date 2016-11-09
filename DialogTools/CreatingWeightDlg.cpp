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

#include <limits>
#include <vector>
#include <set>
#include <string>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include <wx/grid.h>
#include <wx/regex.h>
#include "../FramesManager.h"
#include "../ShapeOperations/PolysToContigWeights.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../Project.h"
#include "../GeneralWxUtils.h"
#include "../GenGeomAlgs.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GenUtils.h"
#include "../SpatialIndAlgs.h"
#include "../PointSetAlgs.h"
#include "AddIdVariable.h"
#include "CreatingWeightDlg.h"

BEGIN_EVENT_TABLE( CreatingWeightDlg, wxDialog )
EVT_CLOSE( CreatingWeightDlg::OnClose )
EVT_BUTTON( XRCID("ID_CREATE_ID"), CreatingWeightDlg::OnCreateNewIdClick )
EVT_CHOICE(XRCID("IDC_IDVARIABLE"), CreatingWeightDlg::OnIdVariableSelected )
EVT_CHOICE(XRCID("IDC_DISTANCE_METRIC"), CreatingWeightDlg::OnDistanceChoiceSelected )
EVT_CHOICE(XRCID("IDC_XCOORDINATES"), CreatingWeightDlg::OnXSelected )
EVT_CHOICE(XRCID("IDC_YCOORDINATES"), CreatingWeightDlg::OnYSelected )
EVT_CHOICE(XRCID("IDC_XCOORD_TIME"), CreatingWeightDlg::OnXTmSelected )
EVT_CHOICE(XRCID("IDC_YCOORD_TIME"), CreatingWeightDlg::OnYTmSelected )
EVT_RADIOBUTTON( XRCID("IDC_RADIO_QUEEN"), CreatingWeightDlg::OnCRadioQueenSelected )
EVT_SPIN( XRCID("IDC_SPIN_ORDEROFCONTIGUITY"), CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated )
EVT_RADIOBUTTON( XRCID("IDC_RADIO_ROOK"), CreatingWeightDlg::OnCRadioRookSelected )
EVT_RADIOBUTTON( XRCID("IDC_RADIO_DISTANCE"), CreatingWeightDlg::OnCRadioDistanceSelected )
EVT_TEXT( XRCID("IDC_THRESHOLD_EDIT"), CreatingWeightDlg::OnCThresholdTextEdit )
EVT_SLIDER( XRCID("IDC_THRESHOLD_SLIDER"), CreatingWeightDlg::OnCThresholdSliderUpdated )

EVT_RADIOBUTTON( XRCID("IDC_RADIO_KNN"), CreatingWeightDlg::OnCRadioKnnSelected )
EVT_SPIN( XRCID("IDC_SPIN_KNN"), CreatingWeightDlg::OnCSpinKnnUpdated )
EVT_BUTTON( XRCID("wxID_OK"), CreatingWeightDlg::OnCreateClick )
EVT_CHECKBOX( XRCID("IDC_PRECISION_CBX"), CreatingWeightDlg::OnPrecisionThresholdCheck)
END_EVENT_TABLE()


CreatingWeightDlg::CreatingWeightDlg(wxWindow* parent,
                                     Project* project_s,
                                     wxWindowID id,
                                     const wxString& caption,
                                     const wxPoint& pos, 
                                     const wxSize& size,
                                     long style )
: all_init(false), 
m_thres_delta_factor(1.00001),
m_is_arc(false), 
m_arc_in_km(false), 
m_thres_val_valid(false),
m_threshold_val(0.01),
project(project_s),
frames_manager(project_s->GetFramesManager()),
table_int(project_s->GetTableInt()),
table_state(project_s->GetTableState()),
w_man_int(project_s->GetWManInt()),
w_man_state(project_s->GetWManState()),
m_num_obs(project_s->GetNumRecords()),
m_cbx_precision_threshold_first_click(true),
suspend_table_state_updates(false)
{
    wxLogMessage("Open CreatingWeightDlg");
	Create(parent, id, caption, pos, size, style);
	all_init = true;
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	w_man_state->registerObserver(this);
}

CreatingWeightDlg::~CreatingWeightDlg()
{
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
	w_man_state->removeObserver(this);
}

void CreatingWeightDlg::OnClose(wxCloseEvent& ev)
{
	wxLogMessage("Close CreatingWeightDlg");
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

bool CreatingWeightDlg::Create( wxWindow* parent, wxWindowID id,
                             const wxString& caption, const wxPoint& pos,
                             const wxSize& size, long style )
{
	m_id_field = 0;
	m_radio_queen = 0;
	m_contiguity = 0;
	m_spincont = 0;
	m_radio_rook = 0;
	m_include_lower = 0;
	m_txt_precision_threshold = 0;
	m_cbx_precision_threshold = 0;
	m_dist_choice = 0;
	m_X = 0;
	m_Y = 0;
	m_X_time = 0;
	m_Y_time = 0;
	m_radio_thresh = 0;
	m_threshold = 0;
	m_sliderdistance = 0;
	m_radio_knn = 0;
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
	m_dist_choice = XRCCTRL(*this, "IDC_DISTANCE_METRIC", wxChoice);
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
	m_radio_queen = XRCCTRL(*this, "IDC_RADIO_QUEEN", wxRadioButton);
	m_radio_rook = XRCCTRL(*this, "IDC_RADIO_ROOK", wxRadioButton);
	m_radio_thresh = XRCCTRL(*this, "IDC_RADIO_DISTANCE", wxRadioButton);
	m_radio_knn = XRCCTRL(*this, "IDC_RADIO_KNN", wxRadioButton);
	m_neighbors = XRCCTRL(*this, "IDC_EDIT_KNN", wxTextCtrl);
	m_spinneigh = XRCCTRL(*this, "IDC_SPIN_KNN", wxSpinButton);
	
	InitDlg();
}

void CreatingWeightDlg::OnCreateNewIdClick( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCreateNewIdClick");
	
	suspend_table_state_updates = true;
	AddIdVariable dlg(table_int, this);
	if (dlg.ShowModal() == wxID_OK) {
		// We know that the new id has been added to the the table in memory
    	col_id_map.clear();
    	table_int->FillColIdMap(col_id_map);
    	
    	InitFields();
		m_id_field->SetSelection(0);
        
		EnableDistanceRadioButtons(m_id_field->GetSelection() != wxNOT_FOUND);
		EnableContiguityRadioButtons((m_id_field->GetSelection() != wxNOT_FOUND) && !project->IsTableOnlyProject());
		UpdateCreateButtonState();
	} else {
		// A new id was not added to the dbf file, so do nothing.
	}
	suspend_table_state_updates = false;
	event.Skip();
}

void CreatingWeightDlg::OnCreateClick( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCreateClick");
    try {
        CreateWeights();
    } catch(GdaException e) {
        wxString msg;
        msg << e.what();
        wxMessageDialog dlg(this, msg , _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }
}

void CreatingWeightDlg::CreateWeights()
{
	WeightsMetaInfo wmi;
	
	if (m_radio == NO_RADIO) {
        wxString msg = _("Please select a weights type.");
		wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	
	if (m_radio == THRESH) {
		if (!m_thres_val_valid) {
			wxString msg = _("The currently entered threshold value is not a valid number.  Please move the slider, or enter a valid number.");
			wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			return;
		}
		if (m_threshold_val*m_thres_delta_factor < m_thres_min) {
			wxString msg = wxString::Format(_("The currently entered threshold value of %f is less than %f which is the minimum value for which there will be no neighborless observations (isolates). \n\nPress Yes to proceed anyhow, press No to abort."), m_threshold_val, m_thres_min);
			wxMessageDialog dlg(this, msg, _("Warning"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if (dlg.ShowModal() != wxID_YES)
                return;
		}
	}
	
	wxString wildcard;
	wxString defaultFile(project->GetProjectTitle());
	if (IsSaveAsGwt()) {
		defaultFile += ".gwt";
		wildcard = "GWT files (*.gwt)|*.gwt";
	} else {
		defaultFile += ".gal";
		wildcard = "GAL files (*.gal)|*.gal";
	}
	
	wxFileDialog dlg(this,
                     _("Choose an output weights file name."),
                     project->GetWorkingDir().GetPath(),
                     defaultFile,
                     wildcard,
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	wxString outputfile;
	if (dlg.ShowModal() != wxID_OK)
        return;
	outputfile = dlg.GetPath();
	
    wxLogMessage(_("CreateWeights()") + outputfile);
    
	wxString id = wxEmptyString;
	if ( m_id_field->GetSelection() != wxNOT_FOUND ) {
		id = m_id_field->GetString(m_id_field->GetSelection());
	} else {
		return; // we must have key id variable
	}
	
	if ((m_id_field->GetSelection() != wxNOT_FOUND) && !CheckID(id))
        return;
    
	int m_ooC = m_spincont->GetValue();
	int m_kNN = m_spinneigh->GetValue();
	int m_alpha = 1;
	
	bool done = false;
	
	wxString str_X = m_X->GetString(m_X->GetSelection());
	wxString str_Y = m_Y->GetString(m_Y->GetSelection());
	if (m_X->GetSelection() < 0) {
		dist_values = WeightsMetaInfo::DV_unspecified;
	} else if (m_X->GetSelection() == 0) {
		dist_values = WeightsMetaInfo::DV_centroids;
	} else if (m_X->GetSelection() == 1) {
		dist_values = WeightsMetaInfo::DV_mean_centers;
	} else {
		dist_values = WeightsMetaInfo::DV_vars;
	}
	
	bool m_check1 = m_include_lower->GetValue();
	
	switch (m_radio) {
		case THRESH:
		{
			GwtWeight* Wp = 0;
			double t_val = m_threshold_val;
            if (t_val <= 0) {
                t_val = std::numeric_limits<float>::min();
            }
			wmi.SetToThres(id, dist_metric, dist_units, dist_units_str,dist_values, t_val, dist_var_1, dist_tm_1, dist_var_2, dist_tm_2);
            
			if (m_is_arc && m_arc_in_km) {
				//t_val /= GenGeomAlgs::one_mi_in_km; // convert km to mi
			}
            
			if (t_val > 0) {
				using namespace SpatialIndAlgs;
				Wp = thresh_build(m_XCOO, m_YCOO, t_val * m_thres_delta_factor, m_is_arc, !m_arc_in_km);
				if (!Wp || !Wp->gwt) {
					wxString m = _("No weights file was created due to all observations being isolates for the specified threshold value. Increase the threshold to create a non-empty weights file.");
					wxMessageDialog dlg(this, m, _("Error"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
					return;
				}
				
				WriteWeightFile(0, Wp->gwt, project->GetProjectTitle(), outputfile, id, wmi);
				if (Wp) delete Wp;
				done = true;
			}
		}
			break;
			
		case KNN: // k nn
		{
			wmi.SetToKnn(id, dist_metric, dist_units, dist_units_str, dist_values, m_kNN, dist_var_1, dist_tm_1, dist_var_2, dist_tm_2);
            
			if (m_kNN > 0 && m_kNN < m_num_obs) {
				GwtWeight* Wp = 0;
				Wp = SpatialIndAlgs::knn_build(m_XCOO, m_YCOO, m_kNN, dist_metric == WeightsMetaInfo::DM_arc, dist_units == WeightsMetaInfo::DU_mile);
                
				if (!Wp->gwt)
                    return;
                Wp->id_field = id;
                
				WriteWeightFile(0, Wp->gwt, project->GetProjectTitle(), outputfile, id, wmi);
				if (Wp) delete Wp;
				done = true;
                
			} else {
				wxString s = wxString::Format(_("Error: Maximum number of neighbors %d exceeded."), m_num_obs-1);
				wxMessageBox(s);
			}
		}
			break;
			
		case ROOK:
		case QUEEN:
		{
			GalElement *gal = 0;
			bool is_rook = (m_radio == ROOK);
			if (is_rook) {
				wmi.SetToRook(id, m_ooC, m_check1);
			} else {
				wmi.SetToQueen(id, m_ooC, m_check1);
			}
			if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
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
					wxString msg = _("There was a problem generating voronoi contiguity neighbors. Please report this.");
					wxMessageDialog dlg(NULL, msg, _("Voronoi Contiguity Error"), wxOK | wxICON_ERROR);
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
                        if ( prec_thres.ToDouble(&value) ) {
							precision_threshold = value;
                        }
					} else {
						precision_threshold = 0.0;
					}
				}
				gal = PolysToContigWeights(project->main_data, !is_rook, precision_threshold);
			}
		
            bool empty_w = true;
            bool has_island = false;
            
        	for (size_t i=0; i<m_num_obs; ++i) {
                if (gal[i].Size() >0) {
                    empty_w = false;
                } else {
                    has_island = true;
                }
            }
            
			if (empty_w) {
				// could be an empty weights file, and should prompt user
				// to setup Precision Threshold
				wxString msg = _("None of your observations have neighbors. This could be related to digitizing problems, which can be fixed by adjusting the precision threshold.");
				wxMessageDialog dlg(NULL, msg, "Empty Contiguity Weights", wxOK | wxICON_WARNING);
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
            if (has_island) {
                wxString msg = _("There is at least one neighborless observation. Check the weights histogram and linked map to see if the islands are real or not. If not, adjust the distance threshold (points) or the precision threshold (polygons).");
                wxMessageDialog dlg(NULL, msg, "Neighborless Observation", wxOK | wxICON_WARNING);
                dlg.ShowModal();
            }
			if (m_ooC > 1) {
				Gda::MakeHigherOrdContiguity(m_ooC, m_num_obs, gal, m_check1);
				WriteWeightFile(gal, 0, project->GetProjectTitle(), outputfile, id, wmi);
			} else {
				WriteWeightFile(gal, 0, project->GetProjectTitle(), outputfile, id, wmi);
			}
			if (gal) delete [] gal; gal = 0;
			done = true;
		}
			break;
			
		default:
			break;
	};
}

void CreatingWeightDlg::OnPrecisionThresholdCheck( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnPrecisionThresholdCheck");
    
	if (m_cbx_precision_threshold_first_click) {
		// Show a warning message regarding the use of this function
		wxString msg = _("Set the threshold to bridge the gap between disconnected polygons (often caused by digitizing errors). The value depends on your measurement unit (e.g. 1 foot or 0.0000001 degrees). Use the weights histogram to detect neighborless observations.");
		wxMessageDialog dlg(NULL, msg, _("About Precision Threshold"),
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
    wxLogMessage("Click CreatingWeightDlg::OnCRadioRookSelected");
	SetRadioBtnAndAssocWidgets(ROOK);
	SetRadioButtons(ROOK);
}

void CreatingWeightDlg::OnCRadioQueenSelected( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCRadioQueenSelected");
	SetRadioBtnAndAssocWidgets(QUEEN);
	SetRadioButtons(QUEEN);
}

void CreatingWeightDlg::update(FramesManager* o)
{
}

void CreatingWeightDlg::update(TableState* o)
{
	if (suspend_table_state_updates) return;
	if (o->GetEventType() == TableState::cols_delta ||
			o->GetEventType() == TableState::col_rename ||
			o->GetEventType() == TableState::col_data_change ||
			o->GetEventType() == TableState::col_order_change ||
			o->GetEventType() == TableState::time_ids_add_remove ||
			o->GetEventType() == TableState::time_ids_rename ||
			o->GetEventType() == TableState::time_ids_swap)
	{
		InitDlg();
	}
	Refresh();
}

void CreatingWeightDlg::update(WeightsManState* o)
{
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
	m_dist_choice->Enable(b);
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

void CreatingWeightDlg::SetRadioBtnAndAssocWidgets(RadioBtnId radio)
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
	
	if ((radio == QUEEN) || (radio == ROOK) ||
			(radio == THRESH) || (radio == KNN)) {
		m_radio = radio;
	} else {
		m_radio = NO_RADIO;
	}
	UpdateCreateButtonState();
	
	switch (m_radio) {
		case QUEEN:
		case ROOK: {
			FindWindow(XRCID("IDC_STATIC_OOC1"))->Enable(true);
			m_contiguity->Enable(true);
			m_spincont->Enable(true);
			m_cbx_precision_threshold->Enable(true);
			m_include_lower->Enable(true);
		}
			break;
		case THRESH: {
			EnableThresholdControls(true);
		}
			break;
		case KNN: {
			FindWindow(XRCID("IDC_STATIC1"))->Enable(true);
			FindWindow(XRCID("IDC_STATIC2"))->Enable(true);
			FindWindow(XRCID("IDC_STATIC3"))->Enable(true);
			m_X->Enable(true);
			m_Y->Enable(true);
			//SetDistChoiceEuclid(true);
			m_dist_choice->Enable(true);
			m_sliderdistance->Enable(false);
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
	if (!all_init) return;
	int sl_x, sl_y;
	m_sliderdistance->GetPosition(&sl_x, &sl_y);
	wxSize sl_size = m_sliderdistance->GetSize();
	m_sliderdistance->SetSize(sl_x, sl_y, 500, sl_size.GetHeight());
	
	if (m_X->GetSelection() == wxNOT_FOUND ||
        m_Y->GetSelection() == wxNOT_FOUND) {
        return;
    }
    
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
	dist_var_1 = v1;
	dist_var_2 = v2;
	
	if (v1 == wxEmptyString || v2 == wxEmptyString) {
		if (mean_center) {
			project->GetMeanCenters(m_XCOO, m_YCOO);
		} else {
			project->GetCentroids(m_XCOO, m_YCOO);
		}
	}
	if (v1 != wxEmptyString || v2 != wxEmptyString) {
		if (v1 != wxEmptyString) {
            // minus 2 is for <X-Centroids> and <X-Mean> selection options in Dropdown
			int x_sel = (project->IsTableOnlyProject() ? m_X->GetSelection() : m_X->GetSelection()-2);
			int col_id = col_id_map[x_sel];
			int tm = 0;
			dist_tm_1 = -1;
			if (table_int->IsTimeVariant() &&
					table_int->IsColTimeVariant(col_id)) {
				tm = m_X_time->GetSelection();
				dist_tm_1 = tm;
			}
			table_int->GetColData(col_id, tm, m_XCOO);
		}
		if (v2 != wxEmptyString) {
			int y_sel = (project->IsTableOnlyProject() ? m_Y->GetSelection() : m_Y->GetSelection()-2);
			int col_id = col_id_map[y_sel];
			int tm = 0;
			dist_tm_2 = -1;
			if (table_int->IsTimeVariant() &&
					table_int->IsColTimeVariant(col_id)) {
				tm = m_Y_time->GetSelection();
				dist_tm_2 = tm;
			}
			table_int->GetColData(col_id, tm, m_YCOO);
		}
	}
	
	m_thres_min = SpatialIndAlgs::find_max_1nn_dist(m_XCOO, m_YCOO,
                                                    m_is_arc,
                                                    !m_arc_in_km);
	{
		using namespace PointSetAlgs;
		using namespace GenGeomAlgs;
		wxRealPoint pt1, pt2;
		double d = EstDiameter(m_XCOO, m_YCOO, m_is_arc, pt1, pt2);
		if (m_is_arc) {
			double r = DegToRad(d);
			m_thres_max = m_arc_in_km ? EarthRadToKm(r) : EarthRadToMi(r);
		} else {
			m_thres_max = d;
		}
	}

	m_threshold_val = (m_sliderdistance->GetValue() *
										 (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_thres_val_valid = true;
	m_threshold->ChangeValue( wxString::Format("%f", m_threshold_val));
}

void CreatingWeightDlg::OnCThresholdTextEdit( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCThresholdTextEdit:");
    
	if (!all_init) return;
	wxString val = m_threshold->GetValue();
	val.Trim(false);
	val.Trim(true);
    
    wxLogMessage(val);
    
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
    wxLogMessage("Click CreatingWeightDlg::OnCThresholdSliderUpdated:");
    
	if (!all_init) return;
	bool m_rad_inv_dis_val = false;
	
	m_threshold_val = (m_sliderdistance->GetValue() * (m_thres_max-m_thres_min)/100.0) + m_thres_min;
	m_threshold->ChangeValue( wxString::Format("%f", (double) m_threshold_val));
	if (m_threshold_val > 0)  {
		FindWindow(XRCID("wxID_OK"))->Enable(true);
	}
    
    wxString str_val;
    str_val << m_threshold_val;
    wxLogMessage(str_val);
}

void CreatingWeightDlg::OnCRadioDistanceSelected( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCRadioDistanceSelected");
    
	// Threshold Distance radio button selected
	SetRadioBtnAndAssocWidgets(THRESH);
	SetRadioButtons(THRESH);
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnCRadioKnnSelected( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCRadioKnnSelected");
    
	SetRadioBtnAndAssocWidgets(KNN);
	SetRadioButtons(KNN);
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCSpinOrderofcontiguityUpdated");
    
	wxString val;
	val << m_spincont->GetValue();
	m_contiguity->SetValue(val);
}

void CreatingWeightDlg::OnCSpinKnnUpdated( wxSpinEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnCSpinKnnUpdated");
    
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
	if (m_radio == NO_RADIO) enable = false;
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

void CreatingWeightDlg::SetRadioButtons(CreatingWeightDlg::RadioBtnId id)
{
	m_radio_queen->SetValue(id == CreatingWeightDlg::QUEEN);
	m_radio_rook->SetValue(id == CreatingWeightDlg::ROOK);
	m_radio_thresh->SetValue(id == CreatingWeightDlg::THRESH);
	m_radio_knn->SetValue(id == CreatingWeightDlg::KNN);
	if (id != QUEEN && id != ROOK && id != THRESH && id != KNN) {
		m_radio = NO_RADIO;
	} else {
		m_radio = id;
	}
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
		
		if (table_int->GetColType(col) == GdaConst::long64_type ||
            table_int->GetColType(col) == GdaConst::string_type) {
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
    std::vector<wxString> str_id_vec(m_num_obs);
    
	int col = table_int->FindColId(id);
    
    if (table_int->GetColType(col) == GdaConst::long64_type){
        table_int->GetColData(col, 0, str_id_vec);
        
    } else if (table_int->GetColType(col) == GdaConst::string_type) {
        // to handle string field with only numbers
        // Note: can't handle real string (a-zA-Z) here since it's hard
        // to control in weights file (.gal/.gwt/..)
        table_int->GetColData(col, 0, str_id_vec);
        
        wxRegEx regex;
        regex.Compile("^[0-9a-zA-Z_]+$");
        
        for (int i=0, iend=str_id_vec.size(); i<iend; i++) {
            wxString item  = str_id_vec[i];
            if (regex.Matches(item)) {
                str_id_vec[i] = item;
            } else {
                wxString msg = id;
                msg += _(" should contains only numbers/letters as IDs.  Please choose a different ID Variable.");
        		wxMessageBox(msg);
        		return false;
            }
        }
    }
    
	std::set<wxString> id_set;
	for (int i=0, iend=str_id_vec.size(); i<iend; i++) {
		id_set.insert(str_id_vec[i]);
	}
	if (str_id_vec.size() != id_set.size()) {
		wxString msg = id + _(" has duplicate values.  Please choose a different ID Variable.");
		wxMessageBox(msg);
		return false;
	}
	return true;
}

void CreatingWeightDlg::InitDlg()
{
	suspend_table_state_updates = false;
	m_is_arc = false;
	m_arc_in_km = false;
	m_thres_val_valid = false;
	m_threshold_val = 0.01;
	dist_metric = WeightsMetaInfo::DM_euclidean;
	dist_units = WeightsMetaInfo::DU_unspecified;
    dist_units_str = project->project_unit;
	dist_values = WeightsMetaInfo::DV_centroids;
	dist_var_1 = "";
	dist_tm_1 = -1;
	dist_var_2 = "";
	dist_tm_2 = -1;
	
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
	m_dist_choice->Clear();
	m_dist_choice->Append("Euclidean Distance");
	m_dist_choice->Append("Arc Distance (mi)");
	m_dist_choice->Append("Arc Distance (km)");
	m_dist_choice->SetSelection(0);
	SetRadioButtons(NO_RADIO);
	SetRadioBtnAndAssocWidgets(NO_RADIO);
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
	table_int->FillColIdMap(col_id_map);
	
	InitFields();
	int sl_x, sl_y;
	m_sliderdistance->GetPosition(&sl_x, &sl_y);
	wxSize sl_size = m_sliderdistance->GetSize();
	m_sliderdistance->SetSize(sl_x, sl_y, 500, sl_size.GetHeight());
	
	m_X_time->Show(table_int->IsTimeVariant());
	m_Y_time->Show(table_int->IsTimeVariant());
	m_spincont->SetRange(1, (int) m_num_obs / 2);
	m_spinneigh->SetRange(1, (int) m_num_obs - 1);
	
	m_XCOO.resize(m_num_obs);
	m_YCOO.resize(m_num_obs);
	Refresh();
}

bool CreatingWeightDlg::IsSaveAsGwt()
{
	// determine if save type will be GWT or GAL.
	// m_radio values:
	// THRESH - GWT
	// KNN - GWT
	// ROOK - GAL
	// QUEEN - GAL
	return 	!(m_radio == ROOK || m_radio == QUEEN);	
}

void CreatingWeightDlg::OnXSelected(wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnXSelected");
    
	if (m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" &&
        m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" ) {
		m_Y->SetSelection(0);
	}
	if ( m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" && 
			m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" ) {
		m_Y->SetSelection(1);
	}
	if ( m_X->GetSelection() > 1 && m_Y->GetSelection() <= 1) {
		m_Y->SetSelection(m_X->GetSelection());
	}
	UpdateTmSelEnableState();
	UpdateThresholdValues();
	UpdateCreateButtonState();
    
    wxString msg;
    msg << _("selected:") << m_X->GetSelection();
    wxLogMessage(msg);
}

void CreatingWeightDlg::OnYSelected(wxCommandEvent& event )
{
	wxLogMessage("Click CreatingWeightDlg::OnYSelected");
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Centroids>" && 
			m_X->GetString(m_X->GetSelection()) == "<X-Mean-Centers>" ) {
		m_X->SetSelection(0);
	}
	if ( m_Y->GetString(m_Y->GetSelection()) == "<Y-Mean-Centers>" && 
			m_X->GetString(m_X->GetSelection()) == "<X-Centroids>" ) {
		m_X->SetSelection(1);
	}
	if ( m_Y->GetSelection() > 1 && m_X->GetSelection() <= 1) {
		m_X->SetSelection(m_Y->GetSelection());
	}
	UpdateTmSelEnableState();
	UpdateThresholdValues();
	UpdateCreateButtonState();
    
    wxString msg;
    msg << "selected:" << m_Y->GetSelection();
    wxLogMessage(msg);
}

void CreatingWeightDlg::OnXTmSelected(wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnXTmSelected");
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnYTmSelected(wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnYTmSelected");
    
	UpdateThresholdValues();
}

void CreatingWeightDlg::OnDistanceChoiceSelected(wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnDistanceChoiceSelected");
    
	wxString s = m_dist_choice->GetStringSelection();
	if (s == "Euclidean Distance") {
		SetDistChoiceEuclid(false);
		UpdateThresholdValues();
	} else if (s == "Arc Distance (mi)") {
		SetDistChoiceArcMiles(false);
		UpdateThresholdValues();
	} else if (s == "Arc Distance (km)") {
		SetDistChoiceArcKms(false);
		UpdateThresholdValues();
	}
}

void CreatingWeightDlg::SetDistChoiceEuclid(bool update_sel)
{
	if (update_sel) m_dist_choice->SetSelection(0);
	m_is_arc = false;
	m_arc_in_km = false;
	
	dist_metric = WeightsMetaInfo::DM_euclidean;
    
    // note: the projection information can be used (after version 1.8.10)
    // to read the UNIT meta data.
    dist_units_str = project->project_unit;
    
    dist_units = WeightsMetaInfo::DU_unspecified;
}

void CreatingWeightDlg::SetDistChoiceArcMiles(bool update_sel)
{
	if (update_sel) m_dist_choice->SetSelection(1);
	m_is_arc = true;
	m_arc_in_km = false;
	
	dist_metric = WeightsMetaInfo::DM_arc;
	dist_units = WeightsMetaInfo::DU_mile;
}

void CreatingWeightDlg::SetDistChoiceArcKms(bool update_sel)
{
	if (update_sel) m_dist_choice->SetSelection(2);
	m_is_arc = true;
	m_arc_in_km = true;
	
	dist_metric = WeightsMetaInfo::DM_arc;
	dist_units = WeightsMetaInfo::DU_km;
}


void CreatingWeightDlg::OnIdVariableSelected( wxCommandEvent& event )
{
    wxLogMessage("Click CreatingWeightDlg::OnIdVariableSelected");
    
    wxString id = wxEmptyString;
	if ( m_id_field->GetSelection() != wxNOT_FOUND ) {
		id = m_id_field->GetString(m_id_field->GetSelection());
	} else {
		return; // we must have key id variable
	}
	
	if ((m_id_field->GetSelection() != wxNOT_FOUND) && !CheckID(id)) {
        m_id_field->SetSelection(-1);
        return;
    }
    
	EnableDistanceRadioButtons(m_id_field->GetSelection() != wxNOT_FOUND);
	EnableContiguityRadioButtons((m_id_field->GetSelection() != wxNOT_FOUND) && !project->IsTableOnlyProject());
	UpdateCreateButtonState();
    
    wxString msg;
    msg << _("selected:") << m_id_field->GetSelection();
    wxLogMessage(msg);
}

/** layer_name: layer name
 * ofn: output file name
 * idd: id column name
 * id: id column vector
 * WeightsMetaInfo contains all meta info.
 */
bool CreatingWeightDlg::WriteWeightFile(GalElement *gal, GwtElement *gwt,
                                        const wxString& layer_name,
                                        const wxString& ofn,
                                        const wxString& idd,
                                        const WeightsMetaInfo& wmi)
{
	FindWindow(XRCID("wxID_OK"))->Enable(false);
	
	bool success = false;
	bool flag = false;
	//bool geodaL=true; // always save as "Legacy" format.
    
    
    int col = table_int->FindColId(idd);
    
	if (gal) { // gal
        
        if (table_int->GetColType(col) == GdaConst::long64_type){
            std::vector<wxInt64> id_vec(m_num_obs);
            table_int->GetColData(col, 0, id_vec);
            flag = Gda::SaveGal(gal, layer_name, ofn, idd, id_vec);
            
        } else if (table_int->GetColType(col) == GdaConst::string_type) {
            std::vector<wxString> id_vec(m_num_obs);
            table_int->GetColData(col, 0, id_vec);
            flag = Gda::SaveGal(gal, layer_name, ofn, idd, id_vec);
        }
        
	} else if (m_radio == THRESH || m_radio == KNN) { // binary distance
        if (table_int->GetColType(col) == GdaConst::long64_type){
            std::vector<wxInt64> id_vec(m_num_obs);
            table_int->GetColData(col, 0, id_vec);
    		flag = Gda::SaveGwt(gwt, layer_name, ofn, idd, id_vec);
            
        } else if (table_int->GetColType(col) == GdaConst::string_type) {
            std::vector<wxString> id_vec(m_num_obs);
            table_int->GetColData(col, 0, id_vec);
    		flag = Gda::SaveGwt(gwt, layer_name, ofn, idd, id_vec);
        }
	} else {
		flag = false;
	}
    
	if (!flag) {
		wxString msg = _("Failed to create the weights file.");
		wxMessageDialog dlg(NULL, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	} else {
		wxFileName t_ofn(ofn);
		wxString file_name(t_ofn.GetFullName());
		wxString msg = wxString::Format(_("Weights file \"%s\" created successfully."), file_name);
		wxMessageDialog dlg(NULL, msg, _("Success"), wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		success = true;
	}
	
	if (success) {
		wxFileName t_ofn(ofn);
		wxString ext = t_ofn.GetExt().Lower();
		GalWeight* w = 0;
		if (ext != "gal" && ext != "gwt") {
			//LOG_MSG("File extention not gal or gwt");
		} else {
			GalElement* tempGal = 0;
			if (ext == "gal") {
				tempGal=WeightUtils::ReadGal(ofn, table_int);
			} else { // ext == "gwt"
				tempGal=WeightUtils::ReadGwtAsGal(ofn, table_int);
			}
			if (tempGal != 0) {
				w = new GalWeight();
				w->num_obs = table_int->GetNumberRows();
				w->wflnm = ofn;
				w->gal = tempGal;
                w->id_field = idd;
				
				WeightsMetaInfo e(wmi);
				e.filename = ofn;
				boost::uuids::uuid uid = w_man_int->RequestWeights(e);
				if (uid.is_nil())
                    success = false;
				if (success) {
					// deep copy of w
					GalWeight* dcw = new GalWeight(*w);
					success = ((WeightsNewManager*) w_man_int)->AssociateGal(uid, dcw);
                    
					if (success) {
						w_man_int->MakeDefault(uid);
						//wxCommandEvent event;
						//GdaFrame::GetGdaFrame()->OnToolsWeightsManager(event);
					} else {
						delete dcw;
					}
					//GdaFrame::GetGdaFrame()->ShowConnectivityMapView(uid);
				}
                delete w;
			} else {
				success = false;
			}
		}
	}
	
	FindWindow(XRCID("wxID_OK"))->Enable(true);
	return success;
}


