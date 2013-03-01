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

class FramesManager;
class DiagnosticReport;
class WeightsManager;
class DbfGridTableBase;
class Project;

class RegressionDlg: public wxDialog, public FramesManagerObserver
{
    DECLARE_EVENT_TABLE()

public:
    RegressionDlg(Project* project,
				  wxWindow* parent,
				  wxString title = "Regression",
				  wxWindowID id = -1,
				  const wxString& caption = "Regression", 
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize, 
				  long style = wxCAPTION|wxSYSTEM_MENU );
	virtual ~RegressionDlg();

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Regression",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxCAPTION|wxSYSTEM_MENU );

    void CreateControls();
    void OnRunClick( wxCommandEvent& event );
    void OnViewResultsClick( wxCommandEvent& event );
	void OnSaveToTxtFileClick( wxCommandEvent& event );
    void OnStandardizeClick( wxCommandEvent& event );
	void OnPredValCbClick( wxCommandEvent& event );
	void OnCoefVarMatrixCbClick( wxCommandEvent& event );
	void OnMoranZValCbClick( wxCommandEvent& event );
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
    void OnCRadio1Selected( wxCommandEvent& event );
    void OnCRadio2Selected( wxCommandEvent& event );
    void OnCRadio3Selected( wxCommandEvent& event );
    void OnCOpenWeightClick( wxCommandEvent& event );

    wxListBox* m_varlist;
	wxTextCtrl* m_dependent;
	wxListBox* m_independentlist;
    wxChoice* m_choice;
    wxCheckBox* m_CheckConstant;
    wxCheckBox* m_CheckWeight;
    wxCheckBox* m_standardize;
    wxRadioButton* m_radio1;
    wxRadioButton* m_radio2;
    wxRadioButton* m_radio3;
	wxGauge* m_gauge;
	wxStaticText* m_gauge_text;

	Project* project;
	DbfGridTableBase* grid_base;
	WeightsManager* w_manager;
	
	int			RegressModel;
	std::vector<wxString> m_Xnames;
	wxString	m_title;
	wxString	*lists;
	bool		*listb;
	double		*y;
	double		**x;
	bool		m_Run;
	bool		m_OpenDump;
	bool		m_output1, m_output2, m_moranz;
	wxCheckBox* m_pred_val_cb;
	wxCheckBox* m_coef_var_matrix_cb;
	wxCheckBox* m_moran_z_val_cb;
	int			lastSelection;
	int			nVarName;
	double		*m_resid1, *m_yhat1;
	double		*m_resid2, *m_yhat2, *m_prederr2;
	double		*m_resid3, *m_yhat3, *m_prederr3;
	long		m_obs;
	bool		b_done1,b_done2, b_done3;
	int			m_nCount;
	int			m_nTimer;
	
	// name_to_nm is a mapping from variable name
	// in the the column which could include time such as
	// TEMP (1998)  -->  TEMP
	// since the Regression Dialog is non-blocking, we
	// need to use column name lookup to find columns in
	// the Table.
	std::map<wxString, wxString> name_to_nm;
	std::map<wxString, int> name_to_tm_id;
	std::vector<int> col_id_map;
	wxString logReport;
	
	void InitVariableList();
	void EnablingItems();

	void UpdateMessageBox(wxString msg);

	void SetXVariableNames(DiagnosticReport *dr);
	void printAndShowClassicalResults(const wxString& datasetname,
									  const wxString& wname,
									  DiagnosticReport *r, int Obs, int nX);
	void printAndShowLagResults(const wxString& dname, const wxString& wname,
								DiagnosticReport *dr, int Obs, int nX);
	void printAndShowErrorResults(const wxString& datasetname,
								  const wxString& wname,
								  DiagnosticReport *r, int Obs, int nX);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
protected:
	FramesManager* frames_manager;
};

#endif

