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

#include <time.h>
#include <boost/foreach.hpp>
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces
#include <wx/filedlg.h>
#include <wx/textdlg.h>

#include "../FramesManager.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../TemplateCanvas.h"
#include "ProgressDlg.h"
#include "SaveToTableDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/GeodaWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../Regression/DiagnosticReport.h"
#include "../Regression/Lite2.h"
#include "../Regression/PowerLag.h"
#include "../Regression/mix.h"
#include "../Regression/ML_im.h"
#include "../Regression/smile.h"
#include "RegressionDlg.h"
#include "RegressionReportDlg.h"

bool classicalRegression(GalElement *g,
                         int num_obs,
                         double * Y,
						 int dim,
                         double ** X,
						 int expl,
                         DiagnosticReport *dr,
                         bool InclConstant,
						 bool m_moranz,
                         wxGauge* gauge,
						 bool do_white_test);

bool spatialLagRegression(GalElement *g,
                          int num_obs,
                          double * Y,
						  int dim,
                          double ** X,
                          int deps,
                          DiagnosticReport *dr,
						  bool InclConstant,
                          wxGauge* p_bar = 0) ;

bool spatialErrorRegression(GalElement *g,
                            int num_obs,
                            double * Y,
							int dim,
                            double ** XX,
                            int deps,
							DiagnosticReport *rr, 
							bool InclConstant,
                            wxGauge* p_bar = 0);

BEGIN_EVENT_TABLE( RegressionDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_RUN"), RegressionDlg::OnRunClick )
    EVT_BUTTON( XRCID("ID_VIEW_RESULTS"), RegressionDlg::OnViewResultsClick )
	EVT_BUTTON( XRCID("ID_SAVE_TO_TXT_FILE"),
			   RegressionDlg::OnSaveToTxtFileClick )
    //EVT_BUTTON( XRCID("ID_SETUP_AUTO"), RegressionDlg::OnSetupAutoModel )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VARIN"),
					   RegressionDlg::OnCListVarinDoubleClicked )
    EVT_LISTBOX_DCLICK( XRCID("IDC_LIST_VAROUT"),
					   RegressionDlg::OnCListVaroutDoubleClicked )
    EVT_CHECKBOX( XRCID("ID_STANDARDIZE"), RegressionDlg::OnStandardizeClick )
	EVT_CHECKBOX( XRCID("ID_PRED_VAL_CB"), RegressionDlg::OnPredValCbClick )
	EVT_CHECKBOX( XRCID("ID_COEF_VAR_MATRIX_CB"),
				 RegressionDlg::OnCoefVarMatrixCbClick )
    EVT_BUTTON( XRCID("IDC_BUTTON1"), RegressionDlg::OnCButton1Click )
    EVT_BUTTON( XRCID("IDC_BUTTON2"), RegressionDlg::OnCButton2Click )
    EVT_BUTTON( XRCID("IDC_RESET"), RegressionDlg::OnCResetClick )
    EVT_BUTTON( XRCID("IDC_BUTTON3"), RegressionDlg::OnCButton3Click )
    EVT_BUTTON( XRCID("IDC_BUTTON4"), RegressionDlg::OnCButton4Click )
    EVT_BUTTON( XRCID("IDC_BUTTON5"), RegressionDlg::OnCButton5Click )
    EVT_CHECKBOX( XRCID("IDC_WEIGHT_CHECK"),
				 RegressionDlg::OnCWeightCheckClick )
    EVT_BUTTON( XRCID("IDC_SAVE_REGRESSION"),
			   RegressionDlg::OnCSaveRegressionClick )
    EVT_BUTTON( XRCID("wxID_CLOSE"), RegressionDlg::OnCloseClick )
	EVT_MENU( XRCID("wxID_CLOSE"), RegressionDlg::OnCloseClick )
	EVT_CLOSE( RegressionDlg::OnClose )
    
    EVT_RADIOBUTTON( XRCID("IDC_RADIO1"), RegressionDlg::OnCRadio1Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO2"), RegressionDlg::OnCRadio2Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO3"), RegressionDlg::OnCRadio3Selected )
    //EVT_RADIOBUTTON( XRCID("IDC_RADIO4"), RegressionDlg::OnCRadio4Selected )

    EVT_BUTTON( XRCID("ID_OPEN_WEIGHT"), RegressionDlg::OnCOpenWeightClick )
END_EVENT_TABLE()


RegressionDlg::RegressionDlg(Project* project_s, wxWindow* parent,
							 wxString title,
							 wxWindowID id, const wxString& caption,
							 const wxPoint& pos, const wxSize& size,
							 long style )
: project(project_s), frames_manager(project_s->GetFramesManager()),
table_state(project_s->GetTableState()),
table_int(project_s->GetTableInt()),
w_man_int(project_s->GetWManInt()),
w_man_state(project_s->GetWManState()),
autoPVal(0.01),
regReportDlg(0)
{
    wxLogMessage("Open RegressionDlg.");
    
    if (project_s->GetTableInt()->GetNumberCols() == 0) {
        wxString err_msg = _("No numeric variables found in table.");
        wxMessageDialog dlg(NULL, err_msg, _("Warning"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        EndDialog(wxID_CANCEL);
    }
    
	Create(parent, id, caption, pos, size, style);
	
	RegressModel = 1;

	lastSelection = 1;
	nVarName = 0;
	m_Run = false;
	m_OpenDump = false;
	m_output1 = m_output2 = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	m_output1 = false;
	m_output2 = false;

	m_title = title;
	
	InitWeightsList();
	InitVariableList();
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	w_man_state->registerObserver(this);
}

RegressionDlg::~RegressionDlg()
{
    wxLogMessage("RegressionDlg::~RegressionDlg()");
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
	w_man_state->removeObserver(this);
}


void RegressionDlg::OnReportClose(wxWindowDestroyEvent& event)
{
    wxLogMessage("Close RegressionDlg.");
    regReportDlg = 0;
}

bool RegressionDlg::Create(wxWindow* parent, wxWindowID id,
							const wxString& caption, const wxPoint& pos,
							const wxSize& size, long style )
{
    m_varlist = NULL;
    m_dependent = NULL;
    m_independentlist = NULL;
    m_CheckConstant = NULL;
    m_CheckWeight = NULL;
    m_standardize = NULL;
    m_weights = NULL;
    m_radio4 = NULL;
    m_radio1 = NULL;
    m_radio2 = NULL;
    m_radio3 = NULL;
    m_gauge = NULL;
	m_gauge_text = NULL;
	m_white_test_cb = NULL;

    SetParent(parent);
    CreateControls();
    Centre();

    return true;
}

void RegressionDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_REGRESSION");
    m_varlist = XRCCTRL(*this, "IDC_LIST_VARIN", wxListBox);
    m_dependent = XRCCTRL(*this, "IDC_DEPENDENT_VAR", wxTextCtrl);
	m_dependent->SetMaxLength(0);
    m_independentlist = XRCCTRL(*this, "IDC_LIST_VAROUT", wxListBox);
	//Option to not use include constant removed.
    m_CheckConstant = NULL; // XRCCTRL(*this, "IDC_CHECK_CONSTANT", wxCheckBox);
    m_CheckWeight = XRCCTRL(*this, "IDC_WEIGHT_CHECK", wxCheckBox);
	//Option to not click standardize removed.
    m_standardize = NULL; // XRCCTRL(*this, "ID_STANDARDIZE", wxCheckBox);
    m_weights = XRCCTRL(*this, "IDC_CURRENTUSED_W", wxChoice);
    
    m_radio1 = XRCCTRL(*this, "IDC_RADIO1", wxRadioButton);
    m_radio2 = XRCCTRL(*this, "IDC_RADIO2", wxRadioButton);
    m_radio3 = XRCCTRL(*this, "IDC_RADIO3", wxRadioButton);
    //m_radio4 = XRCCTRL(*this, "IDC_RADIO4", wxRadioButton);
    
	
	m_pred_val_cb = XRCCTRL(*this, "ID_PRED_VAL_CB", wxCheckBox);
	m_coef_var_matrix_cb = XRCCTRL(*this, "ID_COEF_VAR_MATRIX_CB", wxCheckBox);
	m_white_test_cb = XRCCTRL(*this, "ID_WHITE_TEST_CB", wxCheckBox);
	m_white_test_cb->SetValue(false);
	
	m_gauge = XRCCTRL(*this, "IDC_GAUGE", wxGauge);
	m_gauge->SetRange(200);
	m_gauge->SetValue(0);
    m_gauge->Hide();
    
    m_gauge_text = XRCCTRL(*this, "IDC_GAUGE_TEXT", wxStaticText);
    
}

void RegressionDlg::OnSetupAutoModel(wxCommandEvent& event )
{
	wxString m = _("Please specify the p-value to be used in tests; \ndefault: p-value = 0.01");
    
	double val;
	wxString dlg_val;
	wxString cur_val;
	
	wxTextEntryDialog dlg(NULL, m, _("Enter a seed value"), cur_val);
	if (dlg.ShowModal() != wxID_OK) 
        return;
    
	dlg_val = dlg.GetValue();
	dlg_val.Trim(true);
	dlg_val.Trim(false);
	if (dlg_val.IsEmpty()) 
        return;
    
	if (dlg_val.ToDouble(&val)) {
		autoPVal = val;
	} else {
        wxString m = _("\"%s\" is not a valid p-value. Default p-value (0.01) is used");
        m = wxString::Format(m, dlg_val);
		wxMessageDialog dlg(NULL, m, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}

}

void RegressionDlg::OnRunClick( wxCommandEvent& event )
{
	wxLogMessage("Click RegressionDlg::OnRunClick");

    m_gauge->Show();
	UpdateMessageBox(_("calculating..."));
	
	wxString m_Yname = m_dependent->GetValue();
   
    wxLogMessage(_("y:") + m_Yname);
    
    // Y and X's data
    //wxString st;
    m_independentlist->SetSelection(0);
    m_independentlist->SetFirstItem(m_independentlist->GetSelection());
    
    const unsigned int sz = m_independentlist->GetCount();
    m_Yname.Trim(false);
    m_Yname.Trim(true);
    
    double** dt = new double* [sz + 1];
    for (int i = 0; i < sz + 1; i++)
        dt[i] = new double[m_obs];
    
    // WS1447
    // fill in each field from m_independentlist and tack on
    // m_Yname to the end
    // NOTE: We need to close this gapping memory leak!d  It looks like
    // dt and x is allocated, but never freed!
    std::vector<double> vec(m_obs);
    undefs.resize(m_obs);
    
    wxLogMessage("x:");
    
    for (int i=0; i < m_independentlist->GetCount(); i++) {
        wxString nm = name_to_nm[m_independentlist->GetString(i)];
        wxLogMessage(nm);
        
        int col = table_int->FindColId(nm);
        if (col == wxNOT_FOUND) {
            wxString err_msg = wxString::Format(_("Variable %s is no longer in the Table.  Please close and reopen the Regression Dialog to synchronize with Table data."), nm);
            wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            // free memory of dt[][]
            for (int i = 0; i < sz + 1; i++) delete[] dt[i];
            delete[] dt;
            return;
        }
        int tm = name_to_tm_id[m_independentlist->GetString(i)];
        table_int->GetColData(col, tm, vec);
        for (int j=0; j<m_obs; j++)
            dt[i][j] = vec[j];
        
        std::vector<bool> vec_undef(m_obs);
        table_int->GetColUndefined(col, tm, vec_undef);
        for (int j=0; j<m_obs; j++)
            undefs[j] = undefs[j] || vec_undef[j];
    }
    
    int y_col_id = table_int->FindColId(name_to_nm[m_Yname]);
    if (y_col_id == wxNOT_FOUND) {
        wxString err_msg = wxString::Format("Variable %s is no longer in the Table.  Please close and reopen the Regression Dialog to synchronize with Table data.", name_to_nm[m_Yname]);
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        // free memory of dt[][]
        for (int i = 0; i < sz + 1; i++) delete[] dt[i];
        delete[] dt;
        return;
    }
    
    table_int->GetColData(y_col_id, name_to_tm_id[m_Yname], vec);
    for (int j=0; j<m_obs; j++)
        dt[sz][j] = vec[j];
    
    std::vector<bool> vec_undef(m_obs);
    table_int->GetColUndefined(y_col_id, name_to_tm_id[m_Yname], vec_undef);
    for (int j=0; j<m_obs; j++)
        undefs[j] = undefs[j] || vec_undef[j];
    
	// Getting X's name
	int nX = m_independentlist->GetCount();
	
	m_Xnames.resize(nX+3);

	int ix = 0, ixName = 0;
	//Option to not use "include constant" removed.
	bool m_constant_term = true; // m_CheckConstant->GetValue();
	bool m_WeightCheck = m_CheckWeight->GetValue();
	bool m_standardization = true; // m_standardize->GetValue();

    // get valid obs
    int valid_obs = 0;
    std::map<int, int> orig_valid_map;
    
    for (int i=0; i<m_obs; i++) {
        if (!undefs[i]) {
            orig_valid_map[i] = valid_obs;
            valid_obs += 1;
        }
    }

    if (valid_obs == 0) {
        wxString err_msg = _("Please check the selected variables are all valid.");
        wxMessageDialog dlg(NULL, err_msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        // free memory of dt[][]
        for (int i = 0; i < sz + 1; i++) delete[] dt[i];
        delete[] dt;
        return;
    }
    
	if (m_constant_term) {
		nX = nX + 1;
		ix = 1; 
        ixName = 1;
		if (RegressModel > 1) { 
			ixName = 2;
		} 
	} else {
        ixName = 1;
	}

	if (m_constant_term) {
		x = new double* [nX + 1]; // the last one is for Y
		alloc(x[0], valid_obs, 1.0); // constant  with 1.0
        
	} else {
		x = new double* [nX];
	}
	
	nVarName = nX + ixName - 1;

	int col = 0, row = 0;
 
	for (int i = 0; i < sz + 1; i++) {
        x[i + ix] = new double[valid_obs];
        int xidx = 0;
        for (int j=0; j<m_obs; j++) {
            if (undefs[j])
                continue;
            x[i+ix][xidx++] = dt[i][j];
        }
	}
	
	if (m_constant_term) {
        y = new double[valid_obs];
        int yidx = 0;
        for (int j=0; j<valid_obs; j++) {
            y[j] = x[sz + 1][j];
        }
	} else {
        y = new double[valid_obs];
        for (int j=0; j<valid_obs; j++) {
            y[j] = x[sz][j];
        }
	}

    // free memory of dt[][]
    for (int i = 0; i < sz + 1; i++) {
        delete[] dt[i];
    }
    delete[] dt;
    
    
	double* result = NULL;
	
	const int n = valid_obs;
	bool do_white_test = m_white_test_cb->GetValue();
    if (m_constant_term) {
        if (RegressModel == 2) {
            wxString W_name = "W_" + m_Yname;
            W_name = W_name.Left(12);
            m_Xnames[0] = W_name;
            m_Xnames[1] = "CONSTANT";
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i + 2] = m_independentlist->GetString(i);
            }
        } else if (RegressModel == 3) {
            m_Xnames[nX] = "LAMBDA";
            m_Xnames[0]  = "CONSTANT";
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i + 1] = m_independentlist->GetString(i);
            }
        } else {
            m_Xnames[0]  = "CONSTANT";
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i + 1] = m_independentlist->GetString(i);
            }
        }
    } else {
        if (RegressModel == 2) {
            wxString W_name = "W_" + m_Yname;
            W_name = W_name.Left(12);
            m_Xnames[0] = W_name;
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i + 1] = m_independentlist->GetString(i);
            }
        } else if (RegressModel == 3) {
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i] = m_independentlist->GetString(i);
            }
            m_Xnames[nX] = "LAMBDA";
        } else {
            for (int i = 0; i < m_independentlist->GetCount(); i++) {
                m_Xnames[i] = m_independentlist->GetString(i);
            }
        }
    }
    
	if (m_WeightCheck) {
		boost::uuids::uuid id = GetWeightsId();
        GalElement* gal_weight = NULL;
        GalWeight* gw = w_man_int->GetGal(id);
        
        if (valid_obs == m_obs) {
    		gal_weight = gw ? gw->gal : NULL;
            
        } else {
            // construct a new weights with only valid records
            if (gw) {
                gal_weight = new GalElement[valid_obs];
                int cnt = 0;
                for (int i=0; i<m_obs; i++) {
                    if (!undefs[i]) {
                        vector<long> nbrs = gw->gal[i].GetNbrs();
                        vector<double> nbrs_w = gw->gal[i].GetNbrWeights();
                       
                        int n_idx = 0;
                        for (int j=0; j<nbrs.size(); j++) {
                            int nid = nbrs[j];
                            if ( !undefs[nid] ) {
                                double w = nbrs_w[j];
                                int new_nid = orig_valid_map[nid];
                                gal_weight[cnt].SetNbr(n_idx++, new_nid, w);
                            }
                        }
                        
                        cnt += 1;
                    }
                }
            }
        }
		
        bool isAuto = false;
        if (RegressModel == 4) {
            isAuto = true;
            // AUTO: step 1 -- run OLS with Heterogeneity test
            bool HetFlag = false;

            DiagnosticReport m_DR(n, nX, m_constant_term, true, 1);
            if ( false == m_DR.GetDiagStatus()) {
                UpdateMessageBox("");
                return;
            }
            
            if (gal_weight &&
				!classicalRegression(gal_weight, valid_obs, y, n, x, nX, &m_DR,
									 m_constant_term, true, m_gauge,
									 do_white_test)) 
            {
                wxString s = _("Error: the inverse matrix is ill-conditioned.");
                wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                m_OpenDump = false;
                OnCResetClick(event);
                UpdateMessageBox("");
                return;
            } else {
                double* rr = m_DR.GetKBtest();
                double Het = rr[2];
                
                if (Het < autoPVal) {
                    HetFlag = true;
                }
                
                rr = m_DR.GetLMLAG();
                double LMLag1 = rr[2];
                
                rr = m_DR.GetLMERR();
                double LMError1 = rr[2];
                
                if (LMError1 < autoPVal && LMLag1 < autoPVal) {
                    // both significant, goto robust tests
                    rr = m_DR.GetLMLAGRob();
                    double RLMLag1 = rr[2];
                    rr = m_DR.GetLMERRRob();
                    double RLMError1 = rr[2];
                    if (RLMLag1 < autoPVal && RLMLag1 < autoPVal) {
                        wxMessageBox(_("Both are significant, Spatial Lag Model has been selected."));
                        RegressModel = 2;
                    } else if (RLMLag1 <= RLMError1) {
                        // go to lag model estimation
                        RegressModel = 2;
                    } else if (RLMError1 < RLMLag1) {
                        // go to error model estimation
                        RegressModel = 3;
                    }
                } else if (LMError1 < autoPVal) {
                    // go to error model estimation
                    RegressModel = 3;
                } else if (LMLag1 < autoPVal) {
                    // go to lag model estimation
                    RegressModel = 2;
                } else {
                    // stick with OLS
                    RegressModel = 1;
                    if (HetFlag) {
                        // get White error variance
                        do_white_test = true;
                        wxMessageBox(_("OLS Model with White test has been selected."));
                    } else {
                        // stick with original one
                        do_white_test = false;
                        wxMessageBox(_("OLS Model has been selected."));
                    }
                }
            }
            m_DR.release_Var();
        }
        
        
		if (RegressModel == 1) {
            wxLogMessage("OLS model");
			DiagnosticReport m_DR(n, nX, m_constant_term, true, RegressModel);
            if ( false == m_DR.GetDiagStatus()) {
                UpdateMessageBox("");
                return;
            }
            
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight &&
				!classicalRegression(gal_weight, valid_obs, y, n, x, nX, &m_DR,
									 m_constant_term, true, m_gauge,
									 do_white_test))
            {
                wxString s = _("Error: the inverse matrix is ill-conditioned.");
                wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();

				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
				m_resid1= m_DR.GetResidual();
				printAndShowClassicalResults(table_int->GetTableName(),
											 w_man_int->GetLongDispName(id),
											 &m_DR,
											 n, nX, do_white_test);
				m_yhat1 = m_DR.GetYHAT();
				m_OpenDump = true;
				m_Run = true;
				b_done1 = false;
			}

			m_DR.release_Var();
			gal_weight = NULL;

		} else if (RegressModel == 2) {
            wxLogMessage("Spatial Lag model");
			// Check for Symmetry first
			WeightsMetaInfo::SymmetryEnum sym = w_man_int->IsSym(id);
			if (sym == WeightsMetaInfo::SYM_unknown) {
				ProgressDlg* p_dlg = new ProgressDlg(this, wxID_ANY,
													 _("Weights Symmetry Check"));
				p_dlg->Show();
				p_dlg->StatusUpdate(0, _("Checking Symmetry..."));
				sym = w_man_int->CheckSym(id, p_dlg);
				p_dlg->StatusUpdate(1, _("Finished"));
				p_dlg->Destroy();
			}
			if (sym != WeightsMetaInfo::SYM_symmetric) {
                wxString s = _("Spatial lag and error regressions require symmetric weights (not KNN). You can still use KNN weights to obtain spatial diagnostics for classic regressions.");
                wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                
				UpdateMessageBox("");
				return;
			}
			
			DiagnosticReport m_DR(n, nX + 1, m_constant_term, true,
								  RegressModel);
            if ( false == m_DR.GetDiagStatus()) {
                UpdateMessageBox("");
                return;
            }
            
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight && !spatialLagRegression(gal_weight, valid_obs,
													y, n, x, nX, &m_DR, true,
													m_gauge)) {
				wxMessageBox(_("Error: the inverse matrix is ill-conditioned."));
				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
				printAndShowLagResults(table_int->GetTableName(),
									   w_man_int->GetLongDispName(id),
									   &m_DR, n, nX);
				m_yhat2 = m_DR.GetYHAT();
				m_resid2= m_DR.GetResidual();
				m_prederr2 = m_DR.GetPredError();
				m_Run = true;
				m_OpenDump = true;
				b_done2 = false;
			}
			
			m_DR.release_Var();
			gal_weight = NULL;
            
		} else if (RegressModel == 3) {
            wxLogMessage("Spatial Error model");
			// Check for Symmetry first
			WeightsMetaInfo::SymmetryEnum sym = w_man_int->IsSym(id);
			if (sym == WeightsMetaInfo::SYM_unknown) {
				ProgressDlg* p_dlg = new ProgressDlg(this, wxID_ANY,
													 _("Weights Symmetry Check"));
				p_dlg->Show();
				p_dlg->StatusUpdate(0, _("Checking Symmetry..."));
				sym = w_man_int->CheckSym(id, p_dlg);
				p_dlg->StatusUpdate(1, _("Finished"));
				p_dlg->Destroy();
			}
			if (sym != WeightsMetaInfo::SYM_symmetric) {
                wxString s = _("Spatial lag and error regressions require symmetric weights (not KNN). You can still use KNN weights to obtain spatial diagnostics for classic regressions.");
                wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
				UpdateMessageBox("");
				return;
			}			
			
			// Error Model
			DiagnosticReport m_DR(n, nX + 1, m_constant_term, true,
								  RegressModel);
            if ( false == m_DR.GetDiagStatus()) {
                UpdateMessageBox("");
                return;
            }
            
			SetXVariableNames(&m_DR);
			m_DR.SetMeanY(ComputeMean(y, n));
			m_DR.SetSDevY(ComputeSdev(y, n));

			if (gal_weight && !spatialErrorRegression(gal_weight, valid_obs,
													  y, n, x, nX,
													  &m_DR, true, m_gauge)) {
				wxString s = _("Error: the inverse matrix is ill-conditioned.");
                wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                
				m_OpenDump = false;
				OnCResetClick(event);
				UpdateMessageBox("");
				return;
			} else {
	  			printAndShowErrorResults(table_int->GetTableName(),
										 w_man_int->GetLongDispName(id),
										 &m_DR, n, nX);
				m_yhat3 = m_DR.GetYHAT();
				m_resid3= m_DR.GetResidual();
				m_prederr3 = m_DR.GetPredError();
				m_Run = true;
				m_OpenDump = true;
				b_done3 = false;
			}

			m_DR.release_Var();
			gal_weight = NULL;

		} else {
			wxMessageBox(_("wrong model number"));
			UpdateMessageBox("");
			return;
		}
        
        if (isAuto)  {
            // reset regressModel after auto
            RegressModel = 4;
        }
        
        
        if (valid_obs == m_obs) {
            delete[] gal_weight;
        }
        
	} else {
		DiagnosticReport m_DR(n, nX, m_constant_term, false, RegressModel);
        if ( false == m_DR.GetDiagStatus()) {
            UpdateMessageBox("");
            return;
        }
		SetXVariableNames(&m_DR);
		m_DR.SetMeanY(ComputeMean(y, n));
		m_DR.SetSDevY(ComputeSdev(y, n));

		if (!classicalRegression((GalElement*)NULL, m_obs, y, n, x, nX, &m_DR, 
								 m_constant_term, false, m_gauge,
								 do_white_test)) {
            wxString s = _("Error: the inverse matrix is ill-conditioned.");
            wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            
			m_OpenDump = false;
			OnCResetClick(event);
			UpdateMessageBox("");
			return;
		} else {
			printAndShowClassicalResults(table_int->GetTableName(),
										 wxEmptyString, &m_DR, n, nX,
										 do_white_test);
			m_yhat1 = m_DR.GetYHAT();
			m_resid1= m_DR.GetResidual();
			m_OpenDump = true;
			m_Run = true;
			b_done1 = false;
		}
		m_DR.release_Var();
	}

	if (m_OpenDump) {
		//GdaFrame::GetGdaFrame()->DisplayRegression(logReport);
		DisplayRegression(logReport);
	}

    
	for (int i = 0; i < sz + 1; i++) {
        delete [] x[i];
    }
    delete[] x;
    delete [] y;

    
	EnablingItems();
	//FindWindow(XRCID("ID_RUN"))->Enable(false);
	UpdateMessageBox("done");
}

void RegressionDlg::DisplayRegression(wxString dump)
{
    wxLogMessage("RegressionDlg::DisplayRegression()");
    wxDateTime now = wxDateTime::Now();
    dump = ">>" + now.FormatDate() + " " + now.FormatTime() + _("\nREGRESSION\n----------\n") + dump;
    if (regReportDlg == 0) {
        regReportDlg = new RegressionReportDlg(this, dump);
        regReportDlg->Connect(wxEVT_DESTROY, wxWindowDestroyEventHandler(RegressionDlg::OnReportClose), NULL, this);
    } else {
        regReportDlg->AddNewReport(dump);
    }
    regReportDlg->Show(true);
    regReportDlg->m_textbox->SetSelection(0, 0);
}

void RegressionDlg::SetupXNames(bool m_constant_term)
{
    wxLogMessage("RegressionDlg::SetupXNames()");

}
void RegressionDlg::OnViewResultsClick( wxCommandEvent& event )
{
    wxLogMessage(_("Click RegressionDlg::OnViewResultsClick"));
 	if (m_OpenDump) {
		GdaFrame::GetGdaFrame()->DisplayRegression(logReport);
	}
}

void RegressionDlg::OnSaveToTxtFileClick( wxCommandEvent& event )
{
    wxLogMessage(_("Click RegressionDlg::OnSaveToTxtFileClick"));
    
 	if (!m_OpenDump)
        return;
	
	wxFileDialog dlg( this, _("Regression Output Text File"), wxEmptyString,
					 wxEmptyString,
					 "TXT files (*.txt)|*.txt",
					 wxFD_SAVE );
	if (dlg.ShowModal() != wxID_OK) return;
	
	wxFileName new_txt_fname(dlg.GetPath());
	wxString new_main_dir = new_txt_fname.GetPathWithSep();
	wxString new_main_name = new_txt_fname.GetName();
	wxString new_txt = new_main_dir + new_main_name + ".txt";
	
	// Prompt for overwrite permission
	if (wxFileExists(new_txt)) {
		wxString msg;
		msg << new_txt << _(" already exists.  OK to overwrite?");
		wxMessageDialog dlg (this, msg, _("Overwrite?"),
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES)
            return;
	}
	
	bool failed = false;
	// Automatically overwrite existing csv since we have 
	// permission to overwrite.

	if (wxFileExists(new_txt) && !wxRemoveFile(new_txt)) failed = true;
	
	if (!failed) {
		// write logReport to a text file
		wxFFileOutputStream output(new_txt);
		if (output.IsOk()) {
			wxTextOutputStream txt_out( output );
			//txt_out << logReport;
            if (regReportDlg)
                txt_out << regReportDlg->m_textbox->GetValue();
			txt_out.Flush();
			output.Close();
		} else {
			failed = true;
		}
	}
	
	if (failed) {
		wxString msg;
		msg << _("Unable to overwrite ") << new_txt;
		wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
}

void RegressionDlg::OnCListVarinDoubleClicked( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCListVarinDoubleClicked()");
	if (lastSelection == 1) {
		OnCButton1Click(event);
	} else {
		OnCButton2Click(event);
	}
}

void RegressionDlg::OnCListVaroutDoubleClicked( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCListVaroutDoubleClicked()");
	OnCButton3Click(event);
}

void RegressionDlg::OnCButton1Click( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCButton1Click()");
	if (m_varlist->GetCount() > 0) {
		if (m_varlist->GetSelection() >= 0) {
			wxString temp = m_dependent->GetValue();
			m_dependent->SetValue(
							m_varlist->GetString(m_varlist->GetSelection()));
			m_varlist->Delete(m_varlist->GetSelection());
			if (temp != wxEmptyString) m_varlist->Append(temp);
			b_done1 = b_done2 = b_done3 = false;
			m_nCount = 0;
			m_varlist->SetSelection(0);
			m_varlist->SetFirstItem(m_varlist->GetSelection());
		}
	}
	lastSelection = 2;
	EnablingItems();
}

void RegressionDlg::OnCButton2Click( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCButton2Click()");
	if (m_varlist->GetCount() > 0) {
		if (m_varlist->GetSelection() >= 0) {
			int cur_sel = m_varlist->GetSelection();
			m_independentlist->Append(m_varlist->GetString(cur_sel));
			m_varlist->Delete(cur_sel);
			if (cur_sel == m_varlist->GetCount()) {
				cur_sel = m_varlist->GetCount()-1;
			}
			m_varlist->SetSelection(cur_sel);
			m_varlist->SetFirstItem(m_varlist->GetSelection());
			b_done1 = b_done2 = b_done3 = false;
			m_nCount = 0;
		}
	}
	EnablingItems();
	lastSelection = 2;
}

void RegressionDlg::OnCResetClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCResetClick");
    
	logReport = wxEmptyString;
	lastSelection = 1;
	nVarName = 0;
	m_Run = false;
	m_OpenDump = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	RegressModel = 1;
	m_white_test_cb->SetValue(false);
	m_white_test_cb->Enable(true);
	
	m_gauge->SetValue(0);

	m_dependent->SetValue(wxEmptyString);
	InitVariableList();
}

void RegressionDlg::OnCButton3Click( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCButton3Click()");
	if (m_independentlist->GetCount() > 0) {
		if(m_independentlist->GetSelection() >= 0) {
			int cur_sel = m_independentlist->GetSelection();
			m_varlist->Append(m_independentlist->GetString(cur_sel));
			m_independentlist->Delete(cur_sel);
			if (cur_sel == m_independentlist->GetCount()) {
				cur_sel = m_independentlist->GetCount()-1;
			}
			if (cur_sel >= 0) {
				m_independentlist->SetSelection(cur_sel);
				m_independentlist->SetFirstItem(m_independentlist->GetSelection());
			}
			m_nCount = 0;
			b_done1 = b_done2 = b_done3 = false;
		}
	}
	if(m_independentlist->GetCount() <= 0) m_Run = false;
	EnablingItems();	
}

void RegressionDlg::OnCButton4Click( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCButton4Click()");
	for (unsigned int i=0; i<m_varlist->GetCount(); i++) {
		m_independentlist->Append(m_varlist->GetString(i));
	}
	m_varlist->Clear();	
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	EnablingItems();		
	lastSelection = 2;
}

void RegressionDlg::OnCButton5Click( wxCommandEvent& event )
{
    wxLogMessage("RegressionDlg::OnCButton5Click()");
	for (unsigned int i=0; i<m_independentlist->GetCount(); i++) {
		m_varlist->Append(m_independentlist->GetString(i));
	}
	m_independentlist->Clear();	
	m_Run = false;
	b_done1 = b_done2 = b_done3 = false;
	m_nCount = 0;

	EnablingItems();	
	lastSelection = 2;
}

void RegressionDlg::OnCSaveRegressionClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCSaveRegressionClick");
	if (!table_int) return;
	int n_obs = table_int->GetNumberRows();

    std::vector<bool> save_undefs(n_obs);
	std::vector<double> yhat(table_int->GetNumberRows());
	std::vector<double> resid(table_int->GetNumberRows());
	std::vector<double> prederr(RegressModel > 1 ? n_obs : 0);
	std::vector<SaveToTableEntry> data(RegressModel > 1 ? 3 : 2);
		
	wxString pre = "";
	if (RegressModel==1) {
		pre = "OLS_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat1[i];
			resid[i] = m_resid1[i];
            save_undefs[i] = undefs[i];
		}
	} else if (RegressModel==2) {
		pre = "LAG_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat2[i];
			resid[i] = m_resid2[i];
			prederr[i] = m_prederr2[i];
            save_undefs[i] = undefs[i];
		}
	} else { // RegressModel==3
		pre = "ERR_";
		for (int i=0; i<n_obs; i++) {
			yhat[i] = m_yhat3[i];
			resid[i] = m_resid3[i];
			prederr[i] = m_prederr3[i];
            save_undefs[i] = undefs[i];
		}
	}	
	
	data[0].d_val = &yhat;
    data[0].undefined = &save_undefs;
	data[0].label = "Predicted Value";
	data[0].field_default = pre + "PREDIC";
	data[0].type = GdaConst::double_type;
	
	data[1].d_val = &resid;
    data[1].undefined = &save_undefs;
	data[1].label = "Residual";
	data[1].field_default = pre + "RESIDU";
	data[1].type = GdaConst::double_type;
		
	if (RegressModel > 1) {
		data[2].d_val = &prederr;
        data[2].undefined = &save_undefs;
		data[2].label = "Prediction Error";
		data[2].field_default = pre + "PRDERR";
		data[2].type = GdaConst::double_type;
	}
	
	SaveToTableDlg dlg(project, this, data,
					   _("Save Regression Results"),
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();	
	
	if (project->FindTableGrid()) project->FindTableGrid()->Refresh();
}

void RegressionDlg::OnCloseClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCloseClick");
    
	event.Skip();
	EndDialog(wxID_CLOSE);
	Destroy();
}

void RegressionDlg::OnClose(wxCloseEvent& event)
{
	Destroy();
}

void RegressionDlg::OnCOpenWeightClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCOpenWeightClick");
    
	GdaFrame::GetGdaFrame()->OnToolsWeightsManager(event);
	bool m_Run1 = m_independentlist->GetCount() > 0;
	bool enable_run = (m_Run1 &&
					   (!m_CheckWeight->GetValue() ||
						(m_CheckWeight->GetValue() &&
						 (!GetWeightsId().is_nil()))));
	FindWindow(XRCID("ID_RUN"))->Enable(enable_run);	
}

void RegressionDlg::InitVariableList()
{
    wxLogMessage("RegressionDlg::InitVariableList()");
	UpdateMessageBox(wxEmptyString);

	m_varlist->Clear();
	m_independentlist->Clear();
	m_Xnames.clear();
	m_radio1->SetValue(true);
	//Option to not use include constant removed.
	//m_CheckConstant->SetValue(true);
	//m_standardize->SetValue(true);

	m_resid1 = NULL;
	m_yhat1 = NULL;
	m_resid2 = NULL;
	m_yhat2 = NULL;
	m_prederr2 = NULL;
	m_resid3 = NULL;
	m_yhat3 = NULL;
	m_prederr3 = NULL;
 
	m_obs = project->GetNumRecords();
	if (m_obs <= 0) {
        wxString s = _("Error: no records found in data source.");
        wxMessageDialog dlg(NULL, s, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
		return;
	}
	
	lists = NULL;
	listb = NULL;

	std::vector<int> col_id_map;
	table_int->FillNumericColIdMap(col_id_map);
	name_to_nm.clear(); // map to table_int col id
	name_to_tm_id.clear(); // map to corresponding time id
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int id = col_id_map[i];
		wxString name = table_int->GetColName(id);
		if (table_int->IsColTimeVariant(id)) {
			for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
				wxString nm = name;
				nm << " (" << table_int->GetTimeString(t) << ")";
				name_to_nm[nm] = name;
				name_to_tm_id[nm] = t;
				m_varlist->Append(nm);
			}
		} else {
			name_to_nm[name] = name;
			name_to_tm_id[name] = 0;
			m_varlist->Append(name);
		}
	}

	y = NULL;
	x = NULL;
	m_varlist->SetSelection(0);
	m_varlist->SetFirstItem(m_varlist->GetSelection());
	EnablingItems();
}

void RegressionDlg::EnablingItems()
{
    bool m_Run1 = m_independentlist->GetCount() > 0;
	bool enable_run = (m_Run1 &&
					   (!m_CheckWeight->GetValue() ||
						(m_CheckWeight->GetValue() &&
						 (m_weights->GetSelection() !=wxNOT_FOUND))));
	FindWindow(XRCID("ID_RUN"))->Enable(enable_run);	

    wxString m_Yname = m_dependent->GetValue();

	FindWindow(XRCID("IDC_BUTTON2"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON3"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON4"))->Enable(m_Yname!=wxEmptyString);
	FindWindow(XRCID("IDC_BUTTON5"))->Enable(m_Yname!=wxEmptyString);

	FindWindow(XRCID("IDC_LIST_VAROUT"))->Enable(m_Yname != wxEmptyString);	
	
	bool m_WeightCheck = m_CheckWeight->GetValue();

	FindWindow(XRCID("IDC_WEIGHT_CHECK"))->Enable(m_Run1);
	FindWindow(XRCID("IDC_CURRENTUSED_W"))->Enable(m_Run1 && m_WeightCheck);
	FindWindow(XRCID("ID_OPEN_WEIGHT"))->Enable(m_Run1 && m_WeightCheck);
	//Option to not use include constant removed.
	//FindWindow(XRCID("IDC_CHECK_CONSTANT"))->Enable(m_Run1);
	//FindWindow(XRCID("ID_STANDARDIZE"))->Enable(m_Run1);
	//FindWindow(XRCID("ID_VIEW_RESULTS"))->Enable(!logReport.IsEmpty());
	FindWindow(XRCID("ID_SAVE_TO_TXT_FILE"))->Enable(!logReport.IsEmpty());
	FindWindow(XRCID("IDC_SAVE_REGRESSION"))->Enable(m_Run);
    
	FindWindow(XRCID("IDC_RADIO1"))->Enable(m_Run1);
	FindWindow(XRCID("IDC_RADIO2"))->Enable(m_WeightCheck && m_Run1);	
	FindWindow(XRCID("IDC_RADIO3"))->Enable(m_WeightCheck && m_Run1);
    //FindWindow(XRCID("IDC_RADIO4"))->Enable(m_WeightCheck && m_Run1);
}

/** Refreshes weights list and remembers previous selection if
 weights choice is still there and a selection was previously made */
void RegressionDlg::InitWeightsList()
{
	boost::uuids::uuid old_id = GetWeightsId();
	w_ids.clear();
	w_man_int->GetIds(w_ids);
	m_weights->Clear();
	for (size_t i=0; i<w_ids.size(); ++i) {
		m_weights->Append(w_man_int->GetLongDispName(w_ids[i]));
	}
	m_weights->SetSelection(wxNOT_FOUND);
	if (old_id.is_nil() && !w_man_int->GetDefault().is_nil()) {
		for (long i=0; i<w_ids.size(); ++i) {
			if (w_ids[i] == w_man_int->GetDefault()) {
				m_weights->SetSelection(i);
			}
		}
	} else if (!old_id.is_nil()) {
		for (long i=0; i<w_ids.size(); ++i) {
			if (w_ids[i] == old_id) m_weights->SetSelection(i);
		}
	}
}

/** Returns weights selection or nil if none selected */
boost::uuids::uuid RegressionDlg::GetWeightsId()
{
	long sel = m_weights->GetSelection();
	if (w_ids.size() == 0 || sel == wxNOT_FOUND) {
		return boost::uuids::nil_uuid();
	}
	return w_ids[sel];
}

void RegressionDlg::OnCWeightCheckClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCWeightCheckClick");
    
	b_done1 = b_done2 = b_done3 = false;
	EnablingItems();

	if (!m_CheckWeight->IsChecked()) m_radio1->SetValue(true);
}

void RegressionDlg::UpdateMessageBox(wxString msg)
{
	m_gauge_text->SetLabelText(msg);
	m_gauge_text->Update();
    if (msg.IsNull() || msg.IsEmpty())
        m_gauge->Hide();
}

void RegressionDlg::SetXVariableNames(DiagnosticReport *dr)
{
	for (int i = 0; i < nVarName; i++) {
		dr->SetXVarNames(i, m_Xnames[i]);
	}
}

void RegressionDlg::printAndShowClassicalResults(const wxString& datasetname,
												 const wxString& wname,
												 DiagnosticReport *r,
												 int Obs, int nX,
												 bool do_white_test)
{
	wxString f; // temporary formatting string
	wxString slog;
	
	logReport = wxEmptyString; // reset log report
	int cnt = 0;
	
	slog << "SUMMARY OF OUTPUT: ORDINARY LEAST SQUARES ESTIMATION\n"; cnt++;
	slog << "Data set            :  " << datasetname << "\n"; cnt++;
	slog << "Dependent Variable  :";
    
    if (m_dependent->GetValue().length() > 12 )
        slog << "  " << GenUtils::Pad(m_dependent->GetValue(), 12) << "\n";
    else
        slog << GenUtils::Pad(m_dependent->GetValue(), 12) <<  "  ";
        
	slog << "Number of Observations:" << wxString::Format("%5d\n",Obs); cnt++;
	f = "Mean dependent var  :%12.6g  Number of Variables   :%5d\n";
	slog << wxString::Format(f, r->GetMeanY(), nX); cnt++;
	f = "S.D. dependent var  :%12.6g  Degrees of Freedom    :%5d \n";
	slog << wxString::Format(f, r->GetSDevY(), Obs-nX); cnt++;
	slog << "\n"; cnt++;
	
	f = "R-squared           :%12.6f  F-statistic           :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetR2(), r->GetFtest());
	f = "Adjusted R-squared  :%12.6f  Prob(F-statistic)     :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetR2_adjust(), r->GetFtestProb());
	f = "Sum squared residual:%12.6g  Log likelihood        :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetRSS() ,r->GetLIK());
	f = "Sigma-square        :%12.6g  Akaike info criterion :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetSIQ_SQ(), r->GetAIC());
	f = "S.E. of regression  :%12.6g  Schwarz criterion     :%12.6g\n"; cnt++;
	slog << wxString::Format(f, sqrt(r->GetSIQ_SQ()), r->GetOLS_SC());
	f = "Sigma-square ML     :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetSIQ_SQLM());
	f = "S.E of regression ML:%12.6g\n\n"; cnt++; cnt++;
	slog << wxString::Format(f, sqrt(r->GetSIQ_SQLM()));
	
	slog << "--------------------------------------";
	slog << "---------------------------------------\n"; cnt++;
	slog << "       Variable      Coefficient      ";
	slog << "Std.Error    t-Statistic   Probability\n"; cnt++;
	slog << "--------------------------------------";
	slog << "---------------------------------------\n"; cnt++;
	
	for (int i=0; i<nX; i++) {
		slog << GenUtils::PadTrim(r->GetXVarName(i), 18);
		slog << wxString::Format("  %12.6g   %12.6g   %12.6g   %9.5f\n",
								 r->GetCoefficient(i), r->GetStdError(i),
								 r->GetZValue(i), r->GetProbability(i)); cnt++;
	}
	slog << "----------------------------------------";
	slog << "-------------------------------------\n\n"; cnt++; cnt++;
	
	slog << "REGRESSION DIAGNOSTICS  \n"; cnt++;
	double *rr = r->GetBPtest();
	if (rr[1] > 1) {
		slog << wxString::Format("MULTICOLLINEARITY CONDITION NUMBER   %7f\n",
								 r->GetConditionNumber()); cnt++;
	} else {
		slog << wxString::Format("MULTICOLLINEARITY CONDITION NUMBER   %7f\n",
								 r->GetConditionNumber()); cnt++;
		slog << "                                ";
		slog << "      (Extreme Multicollinearity)\n"; cnt++;
	}
	slog << "TEST ON NORMALITY OF ERRORS\n"; cnt++;
	slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
	rr = r->GetJBtest();
	f = "Jarque-Bera           %2.0f        %11.4f        %9.5f\n"; cnt++;
	slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	
	slog << "\n"; cnt++;
	slog << "DIAGNOSTICS FOR HETEROSKEDASTICITY  \n"; cnt++;
	slog << "RANDOM COEFFICIENTS\n"; cnt++;
	slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
	rr = r->GetBPtest();
	if (rr[1] > 0) {
		f = "Breusch-Pagan test    %2.0f        %11.4f        %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	} else {
		f = "Breusch-Pagan test    %2.0f        %11.4f        N/A\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1]);
	}
	rr = r->GetKBtest();
	if (rr[1]>0) {
		f = "Koenker-Bassett test  %2.0f        %11.4f        %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	} else {
		f = "Koenker-Bassett test  %2.0f        %11.4f        N/A\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1]);
	}
	if (do_white_test) {
		slog << "SPECIFICATION ROBUST TEST\n"; cnt++;
		rr = r->GetWhitetest();
		slog << "TEST                  DF           VALUE             PROB\n"; cnt++;
		if (rr[2] < 0.0) {
			f = "White                 %2.0f            N/A            N/A\n"; cnt++;
			slog << wxString::Format(f, rr[0]);
		} else {
			f = "White                 %2.0f        %11.4f        %9.5f\n"; cnt++;
			slog << wxString::Format(f, rr[0], rr[1], rr[2]);
		}
	}
	
	bool m_WeightCheck = m_CheckWeight->GetValue();
	if (m_WeightCheck) {
		slog <<"\n"; cnt++;
		slog << "DIAGNOSTICS FOR SPATIAL DEPENDENCE   \n"; cnt++;
		slog << "FOR WEIGHT MATRIX : " << wname;
		slog << "\n   (row-standardized weights)\n"; cnt++; cnt++;
		rr = r->GetMoranI();
		slog << "TEST                          ";
		slog << "MI/DF        VALUE          PROB\n"; cnt++;
		f = "Moran's I (error)           %8.4f   %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1] ,rr[2]);
		rr = r->GetLMLAG();
		f = "Lagrange Multiplier (lag)      %2.0f      %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
		rr = r->GetLMLAGRob();
		f = "Robust LM (lag)                %2.0f      %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
		rr = r->GetLMERR();
		f = "Lagrange Multiplier (error)    %2.0f      %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
		rr = r->GetLMERRRob();
		f = "Robust LM (error)              %2.0f      %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
		rr = r->GetLMSarma();
		f = "Lagrange Multiplier (SARMA)    %2.0f      %11.4f      %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	}
	
	if (m_output2) {
		slog << "\n"; cnt++;
		slog << "COEFFICIENTS VARIANCE MATRIX\n"; cnt++;
		int start = 0;
		while (start < nX) {
			wxString st = wxEmptyString;
			for (int j=start; j<nX && j<start+5; j++) {
				slog << " " << GenUtils::Pad(r->GetXVarName(j), 10) << " ";
			}
			slog << "\n"; cnt++;
			for (int i=0; i<nX; i++) {
				st = wxEmptyString;
				for (int j=start; j<nX && j<start+5; j++) {
					slog << wxString::Format(" %10.6f ", r->GetCovariance(i,j));
				}
				slog << "\n"; cnt++;
			}
			slog << "\n"; cnt++;
			start += 5;
		}
	}
	
	if (m_output1) {
		slog << "\n"; cnt++;
		slog << "  OBS    " << GenUtils::Pad(m_dependent->GetValue(), 12);
		slog << "        PREDICTED        RESIDUAL\n"; cnt++;
		double *res = r->GetResidual();
		double *yh = r->GetYHAT();
		for (int i=0; i<Obs; i++) {
			slog << wxString::Format("%5d     %12.5f    %12.5f    %12.5f\n",
									 i+1, y[i], yh[i], res[i]); cnt++;
		}
		res = NULL;
		yh = NULL;
	}
	
	slog << "============================== END OF REPORT";
	slog <<  " ================================\n\n"; cnt++; cnt++;
	
	slog << "\n\n"; cnt++; cnt++;
	logReport << slog;
}

void RegressionDlg::printAndShowLagResults(const wxString& datasetname,
										   const wxString& wname,
										   DiagnosticReport *r, int Obs, int nX)
{
	wxString f; // temporary formatting string
	wxString slog;
	
	logReport = wxEmptyString; // reset log report
	int cnt = 0;
	wxString m_Yname = m_dependent->GetValue();
	slog << "SUMMARY OF OUTPUT: SPATIAL LAG MODEL - ";
	slog << "MAXIMUM LIKELIHOOD ESTIMATION\n"; cnt++;
	slog << "Data set            : " << datasetname << "\n"; cnt++;
	slog << "Spatial Weight      : " << wname << "\n"; cnt++;
    
    if (m_Yname.length() > 12 )
        f = "Dependent Variable  :  %12s  \nNumber of Observations:%5d\n";
    else
        f = "Dependent Variable  :%12s  Number of Observations:%5d\n";
    cnt++;
    
	slog << wxString::Format(f, m_Yname, Obs);
	f = "Mean dependent var  :%12.6g  Number of Variables   :%5d\n"; cnt++;
	slog << wxString::Format(f, r->GetMeanY(), nX+1);
	f = "S.D. dependent var  :%12.6g  Degrees of Freedom    :%5d\n"; cnt++;
	slog << wxString::Format(f, r->GetSDevY(), Obs-nX-1);
	f = "Lag coeff.   (Rho)  :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetCoefficient(0));
	slog << "\n"; cnt++;
	
	f = "R-squared           :%12.6f  Log likelihood        :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetR2(), r->GetLIK());
	//f = "Sq. Correlation     :%12.6f  Akaike info criterion :%12.6g\n";
	//slog << wxString::Format(f, r->GetR2_adjust(), r->GetAIC());
	f = "Sq. Correlation     : -            Akaike info criterion :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetAIC());
	f = "Sigma-square        :%12.6g  Schwarz criterion     :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetSIQ_SQ(),r->GetOLS_SC());
	f = "S.E of regression   :%12.6g";
	slog << wxString::Format(f, sqrt(r->GetSIQ_SQ()));
	slog << "\n\n"; cnt++; cnt++;
	
	slog << "----------------------------------------";
	slog << "-------------------------------------\n"; cnt++;
	slog << "       Variable       Coefficient     ";
	slog << "Std.Error       z-value    Probability\n"; cnt++;
	slog << "----------------------------------------";
	slog << "-------------------------------------\n"; cnt++;
	for (int i=0; i<nX+1; i++) {
		slog << GenUtils::PadTrim(wxString(r->GetXVarName(i)), 18);
		f = "  %12.6g   %12.6g   %12.6g   %9.5f\n"; cnt++;
		slog << wxString::Format(f, r->GetCoefficient(i), r->GetStdError(i),
								 r->GetZValue(i), r->GetProbability(i));
	}
	slog << "----------------------------------------";
	slog << "-------------------------------------\n\n"; cnt++; cnt++;
	
	slog << "REGRESSION DIAGNOSTICS\n"; cnt++;
	slog << "DIAGNOSTICS FOR HETEROSKEDASTICITY \n"; cnt++;
	slog << "RANDOM COEFFICIENTS\n"; cnt++;
	slog << "TEST                                     ";
	slog << "DF      VALUE        PROB\n"; cnt++;
	double *rr = r->GetBPtest();
	if (rr[1] >0) {
		f = "Breusch-Pagan test                      ";
		f += "%2.0f    %11.4f   %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	} else {
		f = "Breusch-Pagan test                      ";
		f += "%2.0f    N/A      N/A\n"; cnt++;
		slog << wxString::Format(f, rr[0]);
	}
	slog << "\n"; cnt++;
	
	slog << "DIAGNOSTICS FOR SPATIAL DEPENDENCE\n"; cnt++;
	slog << "SPATIAL LAG DEPENDENCE FOR WEIGHT MATRIX : " << wname << "\n"; cnt++;
	slog << "TEST                                     ";
	slog << "DF      VALUE        PROB\n"; cnt++;
	rr = r->GetLRTest();
	f = "Likelihood Ratio Test                   %2.0f    %11.4f   %9.5f\n"; cnt++;
	slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	
	if (m_output2) {
		slog << "\n"; cnt++;
		slog << "COEFFICIENTS VARIANCE MATRIX\n"; cnt++;
		int start = 0;
		while (start < nX+1) {
			wxString st = wxEmptyString;
			for (int j=start; j<nX+1 && j<start+5; j++) {
				int jj = (j==nX ? 0 : j+1);
				slog << " " << GenUtils::Pad(r->GetXVarName(jj), 10) << " ";
			}
			slog << "\n"; cnt++;
			for (int i=0; i<nX+1; i++) {
				st = wxEmptyString;
				for (int j=start; j<nX+1 && j<start+5; j++) {
					slog << wxString::Format(" %10.6f ", r->GetCovariance(i,j));
				}
				slog << "\n"; cnt++;
			}
			slog << "\n"; cnt++;
			start += 5;
		}
	}
	
	if (m_output1) {
		slog << "\n"; cnt++;
		slog << "  OBS    " << GenUtils::Pad(m_Yname, 12);
		slog << "        PREDICTED        RESIDUAL       PRED ERROR\n"; cnt++;
		double *res  = r->GetResidual();
		double *yh = r->GetYHAT();
		double *pe = r->GetPredError();
		for (int i=0; i<Obs; i++) {
			f = "%5d     %12.5g    %12.5f    %12.5f    %12.5f\n"; cnt++;
			slog << wxString::Format(f, i+1, y[i], yh[i], res[i], pe[i]);
		}
		res = NULL;
		yh = NULL;
	}
	
	slog << "============================== END OF REPORT";
	slog <<  " ================================\n\n"; cnt++; cnt++;
	
	logReport << slog;
}

void RegressionDlg::printAndShowErrorResults(const wxString& datasetname,
											 const wxString& wname,
											 DiagnosticReport *r,
											 int Obs, int nX)
{
	wxString m_Yname = m_dependent->GetValue();
	wxString f; // temporary formatting string
	wxString slog;
	
	logReport = wxEmptyString; // reset log report
	int cnt = 0;
	slog << "SUMMARY OF OUTPUT: SPATIAL ERROR MODEL - ";
	slog << "MAXIMUM LIKELIHOOD ESTIMATION \n"; cnt++;
	slog << "Data set            : " << datasetname << "\n"; cnt++;
	slog << "Spatial Weight      : " << wname << "\n"; cnt++;
	
    if (m_Yname.length() > 12 )
        slog << "Dependent Variable  :  " << GenUtils::Pad(m_Yname, 12) << "\n";
    else
        slog << "Dependent Variable  :" << GenUtils::Pad(m_Yname, 12) << "  ";
    
	slog << wxString::Format("Number of Observations:%5d\n", Obs); cnt++;
	f = "Mean dependent var  :%12.6f  Number of Variables   :%5d\n"; cnt++;
	slog << wxString::Format(f, r->GetMeanY(), nX);
	f = "S.D. dependent var  :%12.6f  Degrees of Freedom    :%5d\n"; cnt++;
	slog << wxString::Format(f, r->GetSDevY(), Obs-nX);
	f = "Lag coeff. (Lambda) :%12.6f\n"; cnt++;
	slog << wxString::Format(f, r->GetCoefficient(nX));
	
	slog << "\n"; cnt++;
	f = "R-squared           :%12.6f  R-squared (BUSE)      : - \n"; cnt++;
	slog << wxString::Format(f, r->GetR2());
	f = "Sq. Correlation     : -            Log likelihood        :%12.6f\n"; cnt++;
	slog << wxString::Format(f, r->GetLIK());
	f = "Sigma-square        :%12.6g  Akaike info criterion :%12.6g\n"; cnt++;
	slog << wxString::Format(f, r->GetSIQ_SQ(), r->GetAIC());
	f = "S.E of regression   :%12.6g  Schwarz criterion     :%12.6g\n\n"; cnt++; cnt++;
	slog << wxString::Format(f, sqrt(r->GetSIQ_SQ()), r->GetOLS_SC());
	
	slog << "----------------------------------------";
	slog << "-------------------------------------\n"; cnt++;
	slog << "       Variable       Coefficient     ";
	slog << "Std.Error       z-value    Probability\n"; cnt++;
	slog << "----------------------------------------";
	slog << "-------------------------------------\n"; cnt++;
	for (int i=0; i<nX+1; i++) {
		slog << GenUtils::PadTrim(wxString(r->GetXVarName(i)), 18);
		f = "  %12.6g   %12.6g   %12.6g   %9.5f\n"; cnt++;
		slog << wxString::Format(f, r->GetCoefficient(i), r->GetStdError(i),
								 r->GetZValue(i), r->GetProbability(i));
	}
	slog << "----------------------------------------";
	slog << "-------------------------------------\n\n"; cnt++; cnt++;
	
	slog << "REGRESSION DIAGNOSTICS\n"; cnt++;
	slog << "DIAGNOSTICS FOR HETEROSKEDASTICITY \n"; cnt++;
	slog << "RANDOM COEFFICIENTS\n"; cnt++;
	slog << "TEST                                     ";
	slog << "DF      VALUE        PROB\n"; cnt++;
	double *rr = r->GetBPtest();
	if (rr[1] >0) {
		f = "Breusch-Pagan test                      ";
		f += "%2.0f    %11.4f   %9.5f\n"; cnt++;
		slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	} else {
		f = "Breusch-Pagan test                      ";
		f += "%2.0f    N/A      N/A\n"; cnt++;
		slog << wxString::Format(f, rr[0]);
	}
	slog << "\n"; cnt++;
	
	slog << "DIAGNOSTICS FOR SPATIAL DEPENDENCE \n"; cnt++;
	slog << "SPATIAL ERROR DEPENDENCE FOR WEIGHT MATRIX : " << wname << "\n"; cnt++;
	slog << "TEST                                     ";
	slog << "DF      VALUE        PROB\n"; cnt++;
	rr = r->GetLRTest();
	f = "Likelihood Ratio Test                   %2.0f    %11.4f   %9.5f\n"; cnt++;
	slog << wxString::Format(f, rr[0], rr[1], rr[2]);
	
	if (m_output2) {
		slog << "\n"; cnt++;
		slog << "COEFFICIENTS VARIANCE MATRIX\n"; cnt++;
		int start = 0;
		while (start < nX+1) {
			wxString st = wxEmptyString;
			for (int j=start; j<nX+1 && j<start+5; j++) {
				slog << " " << GenUtils::Pad(r->GetXVarName(j), 10) << " ";
			}
			slog << "\n"; cnt++;
			for (int i=0; i<nX+1; i++) {
				st = wxEmptyString;
				for (int j=start; j<nX+1 && j<start+5; j++) {
					slog << wxString::Format(" %10.6f ", r->GetCovariance(i,j));
				}
				slog << "\n"; cnt++;
			}
			slog << "\n"; cnt++;
			start += 5;
		}
	}
	
	if (m_output1) {
		slog << "\n"; cnt++;
		slog << "  OBS    " << GenUtils::Pad(m_Yname, 12);
		slog << "        PREDICTED        RESIDUAL       PRED ERROR\n"; cnt++;
		double *res  = r->GetResidual();
		double *yh = r->GetYHAT();
		double *pe = r->GetPredError();
		for (int i=0; i<m_obs; i++) {
			f = "%5d     %12.5g    %12.5f    %12.5f    %12.5f\n"; cnt++;
			slog << wxString::Format(f, i+1, y[i], yh[i], res[i], pe[i]);
		}
		res = NULL;
		yh = NULL;
	}
	
	slog << "============================== END OF REPORT";
	slog <<  " ================================\n\n"; cnt++; cnt++;
	
	logReport << slog;
}

void RegressionDlg::OnCRadio1Selected( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCRadio1Selected");
    
	m_Run = false;
	RegressModel = 1;
	UpdateMessageBox(" ");
    EnablingItems();
	m_white_test_cb->Enable(true);
	m_gauge->SetValue(0);
}

void RegressionDlg::OnCRadio2Selected( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCRadio2Selected");
    
	m_Run = false;
	RegressModel = 2;
	UpdateMessageBox(" ");
    EnablingItems();
	m_white_test_cb->Enable(false);
	m_gauge->SetValue(0);
}

void RegressionDlg::OnCRadio3Selected( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCRadio3Selected");
    
	m_Run = false;
	RegressModel = 3;
	UpdateMessageBox(" ");
    EnablingItems();
	m_white_test_cb->Enable(false);
	m_gauge->SetValue(0);
}

void RegressionDlg::OnCRadio4Selected( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCRadio4Selected");
    
	m_Run = false;
	RegressModel = 4;
	UpdateMessageBox(" ");
    EnablingItems();
	m_white_test_cb->Enable(false);
	m_gauge->SetValue(0);
}

void RegressionDlg::OnStandardizeClick( wxCommandEvent& event )
{
	wxLogMessage("row standardization is by default");
	// m_standardize->SetValue(true);
}

void RegressionDlg::OnPredValCbClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnPredValCbClick");
    
	m_output1 = m_pred_val_cb->GetValue() == 1;
}

void RegressionDlg::OnCoefVarMatrixCbClick( wxCommandEvent& event )
{
    wxLogMessage("Click RegressionDlg::OnCoefVarMatrixCbClick");
    
	m_output2 = m_coef_var_matrix_cb->GetValue() == 1;
}

void RegressionDlg::update(FramesManager* o)
{
}

/** If variables are simply being added, then we will just add these to the
list of vaiables rather than do a reset.  Otherwise, a reset will be done.
This could be made more intelligent in the future, but is probably good
enough for now. */
void RegressionDlg::update(TableState* o)
{
	bool add_vars_only_event = false;
	TableState::EventType et = o->GetEventType();
	if (et == TableState::cols_delta) {
		add_vars_only_event = true;
		BOOST_FOREACH(const TableDeltaEntry& e, o->GetTableDeltaListRef()) {
			if (!e.insert) add_vars_only_event = false;
		}
		if (add_vars_only_event) {
			// name_to_nm, name_to_tm_id and m_varlist need updating
			// Note: we have ensured that the Table received this event
			// first and has already updated it's information.  Therefore, it
			// is safe to query the table for details about new columns
			BOOST_FOREACH(const TableDeltaEntry& e, o->GetTableDeltaListRef()) {
				wxString name = e.group_name;
				int id = table_int->FindColId(name);
				bool tm_vari = table_int->IsColTimeVariant(id);
				if (tm_vari) {
					for (int t=0; t<table_int->GetColTimeSteps(id); t++) {
						wxString nm = name;
						nm << " (" << table_int->GetTimeString(t) << ")";
						name_to_nm[nm] = name;
						name_to_tm_id[nm] = t;
						m_varlist->Append(nm);
					}
				} else {
					name_to_nm[name] = name;
					name_to_tm_id[name] = 0;
					m_varlist->Append(name);
				}
			}
		}
        
		Refresh();
	} else if (et == TableState::col_disp_decimals_change) {
		add_vars_only_event = true;
	}
	if (!add_vars_only_event) {
		// default is to reset dialog
		//wxCommandEvent event;
		//m_OpenDump = false;
		//OnCResetClick(event);
		//kUpdateMessageBox("");
	}
}

void RegressionDlg::update(WeightsManState* o)
{
	// Need to refresh weights list
	InitWeightsList();
	bool m_Run1 = m_independentlist->GetCount() > 0;
	bool enable_run = (m_Run1 &&
					   (!m_CheckWeight->GetValue() ||
						(m_CheckWeight->GetValue() &&
						 (!GetWeightsId().is_nil()))));
	FindWindow(XRCID("ID_RUN"))->Enable(enable_run);
}



