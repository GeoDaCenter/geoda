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
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
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

class RegressionDlg: public wxDialog, public FramesManagerObserver,
  public TableStateObserver, public WeightsManStateObserver
{
    DECLARE_EVENT_TABLE()

public:
    RegressionDlg(Project* project,
				  wxWindow* parent,
				  wxString title = _("Regression"),
				  wxWindowID id = -1,
				  const wxString& caption = _("Regression"),
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize, 
				  long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );
	virtual ~RegressionDlg();

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = _("Regression"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

    void CreateControls();
    void OnRunClick( wxCommandEvent& event );
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

