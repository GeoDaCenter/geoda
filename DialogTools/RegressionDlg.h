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

#ifndef __GEODA_CENTER_REGRESSION_DLG_H__
#define __GEODA_CENTER_REGRESSION_DLG_H__

#include <vector>
#include <map>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "RegressionReportDlg.h"

class FramesManager;
class TableState;
class DiagnosticReport;
class TableInterface;
class Project;
class WeightsManState;

class wxButton;
class wxChoice;
class wxFlexGridSizer;

#include "../Regression/SpregJob.h"

class RegressionDlg: public wxDialog, public FramesManagerObserver,
  public TableStateObserver, public WeightsManStateObserver
{
    DECLARE_EVENT_TABLE()

public:
    RegressionDlg(Project* project,
				  wxWindow* parent,
				  wxString title = _("Regression"),
				  wxWindowID id = wxID_ANY,
				  const wxString& caption = _("Regression"),
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize, 
				  long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
	virtual ~RegressionDlg();

    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY,
				const wxString& caption = _("Regression"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    void CreateControls();
    void OnRunClick( wxCommandEvent& event );
    void OnInstallSpregClick( wxCommandEvent& event );
    void OnViewResultsClick( wxCommandEvent& event );
	void OnSaveToTxtFileClick( wxCommandEvent& event );
    void OnStandardizeClick( wxCommandEvent& event );
	void OnPredValCbClick( wxCommandEvent& event );
	void OnCoefVarMatrixCbClick( wxCommandEvent& event );
    void OnCListVarinDoubleClicked( wxCommandEvent& event );
    void OnCListVaroutDoubleClicked( wxCommandEvent& event );
    void OnCButton1Click( wxCommandEvent& event );
    void OnCButton2Click( wxCommandEvent& event );
    void OnCResetClick( wxCommandEvent& event );
    void OnCButton3Click( wxCommandEvent& event );
    void OnCButton4Click( wxCommandEvent& event );
    void OnCButton5Click( wxCommandEvent& event );
    void OnCWeightCheckClick( wxCommandEvent& event );
    void OnCSaveRegressionClick( wxCommandEvent& event );
    void OnCloseClick( wxCommandEvent& event );
	void OnClose(wxCloseEvent& event);
	void OnReportClose(wxWindowDestroyEvent& event);
    
    void OnCRadio1Selected( wxCommandEvent& event );
    void OnCRadio2Selected( wxCommandEvent& event );
    void OnCRadio3Selected( wxCommandEvent& event );
    void OnCRadio4Selected( wxCommandEvent& event );
    
    void OnCOpenWeightClick( wxCommandEvent& event );
    void OnSetupAutoModel( wxCommandEvent& event );
    
    
    void DisplayRegression(wxString dump);
    
    wxListBox* m_varlist;
	wxTextCtrl* m_dependent;
	wxListBox* m_independentlist;
    wxChoice* m_weights;  // Weights list
	std::vector<boost::uuids::uuid> w_ids; // Weights list corresponding ids
    wxCheckBox* m_CheckConstant;
    wxCheckBox* m_CheckWeight;
    wxCheckBox* m_standardize;
    wxRadioButton* m_radio4;
    wxRadioButton* m_radio1;
    wxRadioButton* m_radio2;
    wxRadioButton* m_radio3;
	wxGauge* m_gauge;
	wxStaticText* m_gauge_text;

	// the Models box is a notebook of two pages: GeoDa's own three models, and
	// the engine's - the second holds everything below
	wxNotebook* m_models_notebook;
	wxPanel* m_spreg_page;

	// the button that installs the engine behind the advanced models, the line
	// that says whether it is there, and the list of the models it offers:
	// all built in code, see RefreshSpregState
	wxButton* m_install_spreg_btn;
	wxStaticText* m_spreg_status;
	wxChoice* m_spreg_model_choice;
	// the extra variables some of the engine's models need, shown only when the
	// chosen model asks for them: a regime membership column, and the endogenous
	// variables with the instruments that identify them
	wxChoice* m_spreg_regime_choice;
	wxListBox* m_spreg_endog_list;
	wxListBox* m_spreg_instr_list;
	// the models that build their own weights from coordinates ask for a pair
	wxChoice* m_spreg_coord_x_choice;
	wxChoice* m_spreg_coord_y_choice;
	wxSizer* m_spreg_coords_row;
	wxSizer* m_spreg_regime_row;
	wxSizer* m_spreg_endog_row;
	wxSizer* m_spreg_instr_row;
	// one control per option the chosen model declares, built from the engine's
	// registry rather than from a list written here
	wxSizer* m_spreg_options_row;
	wxFlexGridSizer* m_spreg_options_grid;
	std::vector<wxString> m_spreg_option_names;
	std::vector<wxWindow*> m_spreg_option_ctrls;
	void FillSpregVariables(wxWindow* parent);
	void FillSpregOptions(wxWindow* parent, const SpregJob::ModelOption& model);
	std::map<wxString, wxString> ReadSpregOptions(bool& ok, wxString& err);
	bool GatherSpregExtras(const SpregJob::ModelOption& model, wxString& err,
						   SpregJob::Writer& writer, wxString& regimes_name,
						   std::vector<wxString>& yend_names,
						   std::vector<wxString>& q_names);
	std::vector<SpregJob::ModelOption> m_spreg_models;
	wxString m_spreg_model;            // empty: use the models above
	bool m_spreg_installed;
	std::vector<double> m_spreg_yhat, m_spreg_resid, m_spreg_prederr;
	std::vector<wxInt64> m_spreg_region;      // SKATER's region id per observation
	SpregJob::Result m_spreg_result;
	bool m_has_spreg_result;

	void RefreshSpregState();
	/// grows the dialog when the controls on the current page need more room
	void GrowToFit();
	void FillSpregModels(const wxString& engine_dir);
	void EnableNativeModels(bool enable);
	void OnSpregModelSelected(wxCommandEvent& event);
	bool RunSpregModel(wxCommandEvent& event);
	void ShowSpregResults(const SpregJob::Result& result, const wxString& dataset,
						  const wxString& weights_name);
    
    RegressionReportDlg *regReportDlg;

	Project* project;
	TableInterface* table_int;
	
	int			RegressModel;
	std::vector<wxString> m_Xnames;
	wxString	m_title;
	wxString	*lists;
	bool		*listb;
	double		*y;
	double		**x;
	bool		m_Run;
	bool		m_OpenDump;
	bool		m_output1, m_output2;
	wxCheckBox* m_pred_val_cb;
	wxCheckBox* m_coef_var_matrix_cb;
	wxCheckBox* m_white_test_cb;
	int			lastSelection;
	int			nVarName;
	double		*m_resid1, *m_yhat1;
	double		*m_resid2, *m_yhat2, *m_prederr2;
	double		*m_resid3, *m_yhat3, *m_prederr3;
	long		m_obs;
	bool		b_done1,b_done2, b_done3;
	int			m_nCount;
	int			m_nTimer;
    std::vector<bool> undefs;
		
	// name_to_nm is a mapping from variable name
	// in the the column which could include time such as
	// TEMP (1998)  -->  TEMP
	// since the Regression Dialog is non-blocking, we
	// need to use column name lookup to find columns in
	// the Table.
	std::map<wxString, wxString> name_to_nm;
	std::map<wxString, int> name_to_tm_id;
	wxString logReport;
	
	void InitVariableList();
	void EnablingItems();
	void InitWeightsList();
	boost::uuids::uuid GetWeightsId();

	void UpdateMessageBox(wxString msg);

	void SetXVariableNames(DiagnosticReport *dr);
	void printAndShowClassicalResults(const wxString& datasetname,
									  const wxString& wname,
									  DiagnosticReport *r, int Obs, int nX,
									  bool do_white_test);
	void printAndShowLagResults(const wxString& dname, const wxString& wname,
								DiagnosticReport *dr, int Obs, int nX);
	void printAndShowErrorResults(const wxString& datasetname,
								  const wxString& wname,
								  DiagnosticReport *r, int Obs, int nX);
	
    void SetupXNames(bool m_constant_term);
    
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return false; }
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};
	
private:
    double autoPVal;
	FramesManager* frames_manager;
	TableState* table_state;
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
};

#endif

