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

#ifndef __GEODA_CENTER_RANDOMIZATION_DLG_H__
#define __GEODA_CENTER_RANDOMIZATION_DLG_H__

#include <vector>
#include "../ShapeOperations/Randik.h"

class GalElement;

class RandomizationDlg: public wxDialog
{    
    DECLARE_CLASS( RandomizationDlg )
    DECLARE_EVENT_TABLE()

public:
	RandomizationDlg( const std::vector<double>& raw_data1,
					 const GalElement* W, int NumPermutations,
					 wxWindow* parent, wxWindowID id = wxID_ANY,
					 const wxString& caption = "Randomization",
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& my_size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU);
	RandomizationDlg( const std::vector<double>& raw_data1,
					 const std::vector<double>& raw_data2,
					 const GalElement* W, int NumPermutations,
					 wxWindow* parent, wxWindowID id = wxID_ANY,
					 const wxString& caption = "Randomization",
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& my_size = wxDefaultSize,
					 long style = wxCAPTION|wxSYSTEM_MENU);
	virtual ~RandomizationDlg();
    void CreateControls();
	void Init();
	void CalcMoran();

    void OnPaint( wxPaintEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
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
	double Moran;
	double  MMean;
	double  MSdev;
	int     totFrequency;
	double  pseudo_p_val;
	double  expected_val;
	bool count_greater;
	
	int* perm;
	long* theRands;
	
    Randik  rng;
	bool    experiment_run_once;

private:
	RandomizationDlg() : start(-1), stop(1), Moran(0), Permutations(0) {}
};

#endif
