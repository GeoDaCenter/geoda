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

#ifndef __GEODA_CENTER_CREATING_WEIGHT_DLG_H__
#define __GEODA_CENTER_CREATING_WEIGHT_DLG_H__

#include <vector>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include "../DataViewer/TableStateObserver.h"

class wxSpinButton;
class GalElement;
class GwtElement;
class Project;
class TableInterface;
class TableState;

/** NOTE: CreatingWeightDlg is still modal even though it is a
 TableStateObserver. The plan is to make it non-modal in the future.*/
class CreatingWeightDlg: public wxDialog, public TableStateObserver
{
public:
    CreatingWeightDlg(wxWindow* parent,
					  Project* project,
					  wxWindowID id = -1,
					  const wxString& caption = "Weights File Creation",
					  const wxPoint& pos = wxDefaultPosition,
					  const wxSize& size = wxDefaultSize,
					  long style = wxCAPTION|wxSYSTEM_MENU );
    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Weights File Creation",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );
    void CreateControls();
	void OnCreateNewIdClick( wxCommandEvent& event );
    
    void OnDistanceMetricSelected(wxCommandEvent& event );
	void SetDistMetricEuclid(bool update_sel);
	void SetDistMetricArcMiles(bool update_sel);
	void SetDistMetricArcKms(bool update_sel);
	void OnIdVariableSelected( wxCommandEvent& event );
    void OnXSelected(wxCommandEvent& event );
    void OnYSelected(wxCommandEvent& event );
	void OnXTmSelected(wxCommandEvent& event );
    void OnYTmSelected(wxCommandEvent& event );
    void OnCRadioQueenSelected( wxCommandEvent& event );
    void OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event );
    void OnCRadioRookSelected( wxCommandEvent& event );
    void OnCRadioDistanceSelected( wxCommandEvent& event );
	void OnCThresholdTextEdit( wxCommandEvent& event );
    void OnCThresholdSliderUpdated( wxCommandEvent& event );
    void OnCRadioKnnSelected( wxCommandEvent& event );
    void OnCRadioGeoDaLSelected( wxCommandEvent& event );
    void OnCSpinKnnUpdated( wxSpinEvent& event );
    void OnCreateClick( wxCommandEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    void OnPrecisionThresholdCheck( wxCommandEvent& event );

	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
private:
	bool all_init;
    wxChoice* m_id_field;
    wxRadioButton* m_radio2;
    wxTextCtrl* m_contiguity;
    wxSpinButton* m_spincont;
    wxRadioButton* m_radio1;
    wxCheckBox* m_include_lower;
    wxChoice* m_distance_metric;
    wxChoice* m_X;
	wxChoice* m_X_time;
    wxChoice* m_Y;
	wxChoice* m_Y_time;
    wxRadioButton* m_radio3;
    wxTextCtrl* m_threshold;
    wxCheckBox* m_cbx_precision_threshold;
    wxTextCtrl* m_txt_precision_threshold;
    wxSlider* m_sliderdistance;
    wxRadioButton* m_radio4;
    wxCheckBox* m_radio9;
    wxTextCtrl* m_neighbors;
    wxSpinButton* m_spinneigh;

	Project* project;
	TableInterface* table_int;
	TableState* table_state;
	
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed
	// in wxGrid
	std::vector<int> col_id_map;
	
	int					m_radio;
	int					m_num_obs;
	double				m_thres_min; // minimum to avoid isolates
	double				m_thres_max; // maxiumum to include everything
	double				m_threshold_val;
	double				m_thres_val_valid;
	const double		m_thres_delta_factor;
	bool				m_cbx_precision_threshold_first_click; 
	
	int					m_method;  // 1 == Euclidean Dist, 2 = Arc Dist
	bool				m_arc_in_km; // true if Arc Dist in km, else miles
	std::vector<double>	m_XCOO;
	std::vector<double>	m_YCOO;

	// updates the enable/disable state of the Create button based
	// on the values of various other controls.
	void UpdateCreateButtonState();
	void UpdateTmSelEnableState();
	void SetRadioBtnAndAssocWidgets(int radio);
	void UpdateThresholdValues();
	void ResetThresXandYCombo();
	void EnableThresholdControls(bool b);
	void EnableContiguityRadioButtons(bool b);
	void EnableDistanceRadioButtons(bool b);
	void ClearRadioButtons();
	void InitFields();
	void InitDlg();
	bool CheckID(const wxString& id);
	bool IsSaveAsGwt(); // determine if save type will be GWT or GAL.
    bool Shp2GalProgress(GalElement *gal, GwtElement *gwt,
						 const wxString& ifn, const wxString& ofn,
						 const wxString& idd,
						 const std::vector<wxInt64>& id_vec);
   
	wxString s_int;
	
	DECLARE_EVENT_TABLE()
};

#endif

