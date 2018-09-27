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

#ifndef __GEODA_CENTER_RANDOMIZATION_DLG_H__
#define __GEODA_CENTER_RANDOMIZATION_DLG_H__

#include <vector>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>

#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/Randik.h"



class GalElement;

class InferenceSettingsDlg : public wxDialog
{
public:
    InferenceSettingsDlg(wxWindow* parent,
                         double p_cutoff,
                         double* p_vals,
                         int n,
                         const wxString& title = _("Inference Settings"),
                         wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize );
    
    void OnAlphaTextCtrl(wxCommandEvent& ev);
    double GetAlphaLevel() { return p_cutoff;}
    double GetBO() {return bo;}
    double GetFDR() { return fdr; }
    double GetUserInput() { return user_input;}
    
protected:
    double bo;
    double fdr;
    double p_cutoff;
    double user_input;
    double* p_vals;
    int n;
    
    wxRadioButton* m_rdo_1;
    wxRadioButton* m_rdo_2;
    wxRadioButton* m_rdo_3;
    wxStaticText* m_txt_bo;
    wxStaticText* m_txt_fdr;
    wxTextCtrl* m_txt_pval;
    wxCheckBox* chk_pval;
    
    void Init(double* p_vals, int n, double current_p);

    void OnOkClick( wxCommandEvent& event );
    
    DECLARE_EVENT_TABLE()
};

class RandomizationPanel: public wxPanel
{
public:
    RandomizationPanel(const std::vector<double>& raw_data1,
                       const std::vector<bool>& undefs_s,
                       const GalElement* W, int NumPermutations,
                       bool reuse_user_seed,
                       uint64_t user_specified_seed,
                       wxFrame* parent,
                       const wxSize& size);
    RandomizationPanel(const std::vector<double>& raw_data1,
                       const std::vector<double>& raw_data2,
                       const std::vector<bool>& undefs_s,
                       const GalElement* W, int NumPermutations,
                       bool reuse_user_seed,
                       uint64_t user_specified_seed,
                       wxFrame* parent,
                       const wxSize& size);
   
    virtual ~RandomizationPanel();
    
	void Init();
	void CalcMoran();
   
	void OnRunClick( wxCommandEvent& event );

	void OnMouse( wxMouseEvent& event );
    void OnSize(wxSizeEvent& event);
    void OnPaint( wxPaintEvent& event );
    void CheckSize(const int width, const int height);
    void Paint(wxDC *dc);
	void Draw(wxDC* dc);
	void DrawRectangle(wxDC* dc, int left, int top, int right, int bottom,
					   const wxColour color);
	
    void SinglePermute();
	void RunPermutations();
	void RunRandomTrials();
	void UpdateStatistics();
	
    int	Width, Height, Left, Right, Top, Bottom;
	int num_obs;
    const int Permutations;
	// vector of Moran's I for every permutation experiment
	std::vector<double> MoranI;
    
	const double start, stop;
    double  range;
    int	    bins, minBin, maxBin;
    int	    binX, thresholdBin;
	std::vector<int> freq;
	
	bool is_bivariate;
	const GalElement* W;
	std::vector<double> raw_data1;
	std::vector<double> raw_data2;
	std::vector<bool> undefs;
	double Moran;
	double  MMean;
	double  MSdev;
	int     totFrequency;
	double  pseudo_p_val;
	double  expected_val;
	bool count_greater;
	
	int* perm;
	long* theRands;
	
	Randik*  rng;
	bool    experiment_run_once;
};

class RandomizationDlg: public wxFrame
{    
    //DECLARE_CLASS( RandomizationDlg )
    DECLARE_EVENT_TABLE()

public:
	RandomizationDlg(const std::vector<double>& raw_data1,
					 const GalWeight* W,
                     const std::vector<bool>& undef,
                     const std::vector<bool>& hl,
                     bool is_regime,
                     int NumPermutations,
                     bool reuse_user_seed,
					 uint64_t user_specified_seed,                    
					 wxWindow* parent, wxWindowID id = wxID_ANY,
					 const wxString& caption = _("Randomization"),
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& my_size = wxDefaultSize,
					 long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE);
	RandomizationDlg( const std::vector<double>& raw_data1,
					 const std::vector<double>& raw_data2,
					 const GalWeight* W,
                     const std::vector<bool>& undef,
                     const std::vector<bool>& hl,
                     bool is_regime,
                     int NumPermutations,
                     bool reuse_user_seed,
					 uint64_t user_specified_seed,
					 wxWindow* parent, wxWindowID id = wxID_ANY,
					 const wxString& caption = _("Randomization"),
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& my_size = wxDefaultSize,
					 long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE);
	virtual ~RandomizationDlg();
    
    void CreateControls();
    void CreateControls_regime();

	void OnMouse( wxMouseEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnRunAll( wxCommandEvent& event );

    RandomizationPanel* panel;
    RandomizationPanel* panel_sel;
    RandomizationPanel* panel_unsel;
    
    bool is_regime;
    GalWeight* copy_w;
    GalWeight* copy_w_sel;
    GalWeight* copy_w_unsel;
};

#endif
