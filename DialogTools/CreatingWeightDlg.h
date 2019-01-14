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

#ifndef __GEODA_CENTER_CREATING_WEIGHT_DLG_H__
#define __GEODA_CENTER_CREATING_WEIGHT_DLG_H__

#include <vector>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../VarCalc/WeightsMetaInfo.h"
#include "../Weights/DistUtils.h"

class wxSpinButton;
class FramesManager;
class GalWeight;
class GwtWeight;
class Project;
class TableInterface;
class TableState;
class WeightsManState;
class WeightsManInterface;

class CreatingWeightDlg: public wxDialog, public FramesManagerObserver,
public TableStateObserver, public WeightsManStateObserver
{
public:
    CreatingWeightDlg(wxWindow* parent,
                      Project* project,
                      bool user_xy = false,
                      wxWindowID id = wxID_ANY,
                      const wxString& caption = _("Weights File Creation"),
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
	virtual ~CreatingWeightDlg();
	void OnClose(wxCloseEvent& ev);
	bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxString& caption = _("Weights File Creation"),
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
							
	void CreateControls();
	void OnCreateNewIdClick( wxCommandEvent& event );
    void OnDistanceChoiceSelected(wxCommandEvent& event );
	void SetDistChoiceEuclid(bool update_sel);
	void SetDistChoiceArcMiles(bool update_sel);
	void SetDistChoiceArcKms(bool update_sel);
	void OnIdVariableSelected( wxCommandEvent& event );
	void OnXSelected(wxCommandEvent& event );
	void OnYSelected(wxCommandEvent& event );
	void OnXTmSelected(wxCommandEvent& event );
	void OnYTmSelected(wxCommandEvent& event );
	void OnCRadioQueenSelected( wxCommandEvent& event );
	void OnCSpinOrderofcontiguityUpdated( wxSpinEvent& event );
	void OnCThresholdTextEdit( wxCommandEvent& event );
    void OnCBandwidthThresholdTextEdit( wxCommandEvent& event );
    void OnCBandwidthThresholdSliderUpdated( wxCommandEvent& event );
	void OnCSpinKnnUpdated( wxSpinEvent& event );
    void OnCSpinKernelKnnUpdated( wxSpinEvent& event );
	void OnCreateClick( wxCommandEvent& event );
	void OnPrecisionThresholdCheck( wxCommandEvent& event );
    void OnInverseDistCheck( wxCommandEvent& event );
    void OnInverseKNNCheck( wxCommandEvent& event );
    void OnCThresholdSliderUpdated( wxCommandEvent& event );
    void OnCSpinPowerInverseDistUpdated( wxSpinEvent& event );
    void OnCSpinPowerInverseKNNUpdated( wxSpinEvent& event );
    void OnDistanceWeightsInputUpdate( wxBookCtrlEvent& event );
    void OnDistanceWeightsVarsSel( wxCommandEvent& event );
    void OnDistanceMetricVarsSel( wxCommandEvent& event );
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const { return 0;}
	virtual void closeObserver(boost::uuids::uuid id) {};
    
    void SetXCOO(const std::vector<double>& xx);
    void SetYCOO(const std::vector<double>& yy);
	
protected:

	bool all_init;
    
    // controls
	wxChoice* m_id_field;
    // contiguity weight
    wxNotebook* m_nb_weights_type;
	wxRadioButton* m_radio_queen; // IDC_RADIO_QUEEN
	wxTextCtrl* m_contiguity;
	wxSpinButton* m_spincont;
	wxRadioButton* m_radio_rook; // IDC_RADIO_ROOK
	wxCheckBox* m_include_lower;
    wxCheckBox* m_cbx_precision_threshold;
    wxTextCtrl* m_txt_precision_threshold;
    // distance weight
    wxChoice* m_trans_choice_vars;
    wxChoice* m_dist_choice_vars;
	wxChoice* m_dist_choice;
	wxChoice* m_X;
	wxChoice* m_X_time;
	wxChoice* m_Y;
	wxChoice* m_Y_time;
    wxListBox* m_Vars;
	wxNotebook* m_nb_distance_variables;
    wxNotebook* m_nb_distance_methods;
	wxTextCtrl* m_threshold;
	wxSlider* m_sliderdistance;
    wxCheckBox* m_use_inverse;
    wxTextCtrl* m_power;
    wxSpinButton* m_spinn_inverse;
	wxTextCtrl* m_neighbors;
	wxSpinButton* m_spinneigh;
    wxCheckBox* m_use_inverse_knn;
    wxTextCtrl* m_power_knn;
    wxSpinButton* m_spinn_inverse_knn;
    wxChoice* m_kernel_methods;
    wxTextCtrl* m_kernel_neighbors;
    wxSpinButton* m_spinn_kernel;
    wxRadioButton* m_radio_adaptive_bandwidth;
    wxRadioButton* m_radio_auto_bandwidth;
    wxRadioButton* m_radio_manu_bandwdith;
    wxTextCtrl* m_manu_bandwidth;
    wxSlider* m_bandwidth_slider;
    wxRadioButton* m_kernel_diagnals;
    wxRadioButton* m_const_diagnals;
    wxButton* m_btn_ok;

	FramesManager* frames_manager;
	Project* project;
	TableInterface* table_int;
	TableState* table_state;
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;

    bool user_xy;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed
	// in wxGrid
	std::vector<int> col_id_map;
	
	int					m_num_obs;
	double				m_thres_min; // minimum to avoid isolates
	double				m_thres_max; // maxiumum to include everything
	double				m_threshold_val;
    double              m_bandwidth_thres_val;
    
    double              m_thres_min_multivars; // minimum to avoid isolates
    double              m_thres_max_multivars; // maxiumum to include everything
    double              m_threshold_val_multivars;
    double              m_bandwidth_thres_val_multivars;
    
	bool				m_thres_val_valid;
    bool                m_bandwidth_thres_val_valid;
	const double		m_thres_delta_factor;
	bool				m_cbx_precision_threshold_first_click; 
	bool				m_is_arc; // true = Arc Dist, false = Euclidean Dist
	bool				m_arc_in_km; // true if Arc Dist in km, else miles
	std::vector<double>	m_XCOO;
	std::vector<double>	m_YCOO;
	
    GeoDa::DistUtils* dist_util;
    std::vector<wxString> col_names;
    
	WeightsMetaInfo::DistanceMetricEnum dist_metric;
	WeightsMetaInfo::DistanceUnitsEnum  dist_units;
	WeightsMetaInfo::DistanceValuesEnum dist_values;

    wxString dist_units_str;
    
	wxString dist_var_1;
	long     dist_tm_1;
	wxString dist_var_2;
	long     dist_tm_2;
	
	// updates the enable/disable state of the Create button based
	// on the values of various other controls.
    void UpdateThresholdValuesMultiVars();
	void UpdateCreateButtonState();
	void UpdateTmSelEnableState();
	void UpdateThresholdValues();
	void ResetThresXandYCombo();
	void InitFields();
	void InitDlg();
	bool CheckID(const wxString& id);
    bool CheckThresholdInput();
    bool CheckTableVariableInput();
    void CreateWeightsFromTable(wxString id, wxString outputfile,
                                WeightsMetaInfo& wmi);
    double GetBandwidth();
	bool IsSaveAsGwt(); // determine if save type will be GWT or GAL.
	bool WriteWeightFile(GalWeight* Wp_gal, GwtWeight* Wp_gwt,
                         const wxString& ifn, const wxString& ofn,
                         const wxString& idd,
                         WeightsMetaInfo& wmi);
    void CreateWeights();
	
	wxString s_int;
	bool suspend_table_state_updates;
	
	DECLARE_EVENT_TABLE()
};

#endif

