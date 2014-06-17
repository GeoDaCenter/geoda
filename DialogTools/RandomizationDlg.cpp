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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcbuffer.h>

#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2cnt.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GdaConst.h"
#include "../GdaConst.h"
#include "../logger.h"
#include "RandomizationDlg.h"

IMPLEMENT_CLASS( RandomizationDlg, wxDialog )

BEGIN_EVENT_TABLE( RandomizationDlg, wxDialog )
    EVT_PAINT( RandomizationDlg::OnPaint )
    EVT_BUTTON( XRCID("ID_CLOSE"), RandomizationDlg::OnCloseClick )
    EVT_BUTTON( XRCID("ID_OK"), RandomizationDlg::OnOkClick )
END_EVENT_TABLE()

RandomizationDlg::RandomizationDlg( const std::vector<double>& raw_data1_s,
								   const std::vector<double>& raw_data2_s,
								   const GalElement* W_s,
								   int NumPermutations,
								   wxWindow* parent, wxWindowID id,
								   const wxString& caption, 
								   const wxPoint& pos, const wxSize& size,
								   long style )
: start(-1), stop(1), raw_data1(raw_data1_s), raw_data2(raw_data2_s), W(W_s),
num_obs(raw_data1_s.size()), Permutations(NumPermutations),
MoranI(NumPermutations, 0), is_bivariate(true)
{
	LOG_MSG("In RandomizationDlg::RandomizationDlg");
	
	SetParent(parent);
    CreateControls();
    Centre();
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	
	CalcMoran();
	Init();
}

RandomizationDlg::RandomizationDlg( const std::vector<double>& raw_data1_s,
								   const GalElement* W_s,
								   int NumPermutations,
								   wxWindow* parent, wxWindowID id,
								   const wxString& caption, 
								   const wxPoint& pos, const wxSize& size,
								   long style )
: start(-1), stop(1), raw_data1(raw_data1_s), W(W_s),
num_obs(raw_data1_s.size()), Permutations(NumPermutations),
MoranI(NumPermutations, 0), is_bivariate(false)
{
	LOG_MSG("In RandomizationDlg::RandomizationDlg");
	
	SetParent(parent);
    CreateControls();
    Centre();
	
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	
	CalcMoran();
	Init();
}

void RandomizationDlg::CalcMoran()
{
	Moran = 0;
	if (is_bivariate) {
		for (int i=0; i<num_obs; i++) {
			Moran += W[i].SpatialLag(raw_data2) * raw_data1[i];
		}
	} else {
		for (int i=0; i<num_obs; i++) {
			Moran += W[i].SpatialLag(raw_data1) * raw_data1[i];
		}
	}
	Moran /= (double) num_obs - 1.0;
}

void RandomizationDlg::Init()
{
	perm = new int[num_obs];
	theRands = new long[num_obs];
	
	if (Permutations <= 10) bins = 10;
	else if (Permutations <= 100) bins = 20;
	else if (Permutations <= 1000) bins = (Permutations+1)/4;
	else bins = 300;
	
	freq.resize(bins);
	
	range = (stop-start) / bins;
	// find the bin for the original Moran's I
	thresholdBin = (int)floor( (Moran-start)/range );
	// leave a room for those who smaller than I
	if (thresholdBin <= 0) thresholdBin = 1;
	// take the last bin
	else if (thresholdBin >= bins) thresholdBin = bins-1;	
	
	experiment_run_once = false;
}

RandomizationDlg::~RandomizationDlg()
{
	if (perm) delete [] perm;
	if (theRands) delete [] theRands;
}

void RandomizationDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_RANDOMIZATION");
}

void RandomizationDlg::OnCloseClick( wxCommandEvent& event )
{	
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void RandomizationDlg::OnOkClick( wxCommandEvent& event )
{
	wxRect rcClient = GetClientRect();
	CheckSize(rcClient.GetWidth(), rcClient.GetHeight());
    //RunPermutations();
	RunRandomTrials();
	Refresh();
	FindWindow(XRCID("ID_CLOSE"))->SetLabel("Done");
}
 
void RandomizationDlg::DrawRectangle(wxDC* dc, int left, int top, int right,
									 int bottom, const wxColour color)
{
	wxPen Pen;
	Pen.SetColour(color);
	dc->SetPen(Pen);
	wxBrush Brush; 
	Brush.SetColour(color);
	dc->SetBrush(Brush); 

	if (left >= right) right = left+1;
	if (top >= bottom) bottom = top+1;
	
    dc->DrawRectangle(wxRect(left, bottom, right-left, top-bottom)); 
}

// NOTE: must carefully look at thresholdBin!
void RandomizationDlg::RunRandomTrials()
{
	totFrequency = 0;
	for (int i=0; i<bins; i++) freq[i]=0;
	// thresholdBin bin already has one permutation
	freq[ thresholdBin ] = 1;
	// leftmost and the righmost are the same so far
	minBin = thresholdBin; 
	maxBin = thresholdBin;

	for (int i=0; i<Permutations; i++) {
		//create a random permutation
		rng.Perm(num_obs, perm, theRands);
		double newMoran = 0;
		if (is_bivariate) {
			for (int i=0; i<num_obs; i++) {
				newMoran += (W[i].SpatialLag(raw_data2, perm)
							 * raw_data1[perm[i]]);
			}
		} else {
			for (int i=0; i<num_obs; i++) {
				newMoran += (W[i].SpatialLag(raw_data1, perm)
							 * raw_data1[perm[i]]);
			}
		}
		newMoran /= (double) num_obs - 1.0;
		// find its place in the distribution
		MoranI[ totFrequency++ ] = newMoran;
		int newBin = (int)floor( (newMoran - start)/range );
		if (newBin < 0) newBin = 0;
		else if (newBin >= bins) newBin = bins-1;
		
		freq[newBin] = freq[newBin] + 1;
		if (newBin < minBin) minBin = newBin;
		if (newBin > maxBin) maxBin = newBin;
	}
	UpdateStatistics();
}

/** For a pseudo p-val based on permutations, we use a one-sided test,
 counting the number of statistics more extreme than the observed
 value.  By "more extreme" we mean the number of values larger than
 the statistic for a statistic larger than the median, and the opposite
 for a statistic that is smaller than the mean.  An equivalent approach,
 which we use for LISA p-vals, is to always count the number larger
 and convert to 1-p if the value is larger than 0.5.  This has the advantage
 of not having to actually compute the median in order to find the p-val.  The
 results are identical. */

void RandomizationDlg::UpdateStatistics()
{
	double sMoran = 0;
	for (int i=0; i < totFrequency; i++) {
		sMoran += MoranI[i];
	}
	MMean = sMoran / (totFrequency); // was (totFrequency-1)
	double MM2 = 0;
	for (int i=0; i < totFrequency; i++) {
		MM2 += ((MoranI[i]-MMean) * (MoranI[i]-MMean));
	}
	MSdev = sqrt(MM2/(totFrequency));
	
	count_greater = true;
	int signFrequency = 0;
	for (int i=0; i < totFrequency; i++) {
		if (MoranI[i] <= Moran) signFrequency++;
	}
	if (totFrequency-signFrequency <= signFrequency) {
		signFrequency = totFrequency-signFrequency;
		count_greater = false;
	}
	
	pseudo_p_val = (((double) signFrequency)+1.0)/(((double) totFrequency)+1.0);
	expected_val = (double) -1/(num_obs - 1);
}

void RandomizationDlg::Draw(wxDC* dc)
{	
	int fMax = freq[0];
	for (int i=1; i<bins; i++) if (fMax < freq[i]) fMax = freq[i];

	for (int i=0; i < bins; i++) {
		double df = double (freq[i]* Height) / double (fMax);
		freq[i] = int(df);
	}

	wxColour color = (count_greater ? GdaConst::outliers_colour :
					  GdaConst::textColor);
	for (int i=0; i < thresholdBin; i++) {
		if (freq[i] > 0) {
			int xx = Top + Height - freq[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}

	color = (!count_greater ? GdaConst::outliers_colour :
			 GdaConst::textColor);
	for (int i=thresholdBin+1; i < bins; i++) {
		if (freq[i] > 0) {
			int xx = Top + Height - freq[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}	
	
	DrawRectangle(dc, Left + thresholdBin*binX, Top,
				  Left + (thresholdBin+1)*binX, Top+  Height,
				  GdaConst::highlight_color );

	wxPen drawPen(*wxBLACK_PEN);
	drawPen.SetColour(GdaConst::textColor);
	dc->SetPen(drawPen);

	//dc->DrawLine(Left, Top, Left, Top+Height); // Vertical axis
	dc->DrawLine(Left, Top+Height, Left+Width, Top+Height); // Horizontal axis

	drawPen.SetColour(wxColour(20, 20, 20));
	dc->SetPen(drawPen);
	dc->SetBrush(*wxWHITE_BRUSH);
	const int hZero= (int)(Left+(0-start)/(stop-start)*Width);
	dc->DrawRectangle(hZero, Top-2 , 3, Height+2);
  
	int fs = 4 + Bottom/4;
	wxFont nf(*wxSMALL_FONT);
	nf.SetPointSize(fs);
	dc->SetFont(nf);
	
	drawPen.SetColour(GdaConst::textColor);
	dc->SetPen(drawPen);
	double zval = 0;
	if (MSdev != 0) zval = (Moran-MMean)/MSdev;
	wxString text;
	dc->SetTextForeground(wxColour(0,0,0));	
	text = wxString::Format("I: %-7.4f  E[I]: %-7.4f  mean: %-7.4f"
							"  sd: %-7.4f  z-value: %-7.4f",
							Moran, expected_val, MMean, MSdev, zval);
	dc->DrawText(text, Left, Top + Height + Bottom/2);
 
	text = wxString::Format("permutations: %d  ", Permutations);
	dc->DrawText(text, Left+5, 20);

	text = wxString::Format("pseudo p-value: %-7.6f", pseudo_p_val);
	dc->DrawText(text, Left+5, 35);
}


void RandomizationDlg::OnPaint( wxPaintEvent& event )
{
	if (!experiment_run_once) {
		experiment_run_once = true;
		wxCommandEvent ev;
		OnOkClick(ev);
	}
    wxAutoBufferedPaintDC dc(this);
	dc.Clear();
	Paint(&dc);
}

void RandomizationDlg::Paint(wxDC *dc)
{
	wxRect rcClient = GetClientRect();
	CheckSize(rcClient.GetWidth(), rcClient.GetHeight());
	
    Draw(dc);
}

void RandomizationDlg::CheckSize(const int width, const int height)
{
	Left = 10;
    Bottom = 20;
	Right = 10;
	Top = 10;
	Height = 40;
	Width = 40;
	
	int res = width - Width - Left - Right;
	if (res < 0) res = 0;
	int rata = (int) floor((double) res / (bins + 2));
	if (rata == 0) rata = 1;
	Left += rata;
	binX = rata;  //  vertical scale is under control of Format
	Right = width - Left - binX*bins;
	if (Right < 0) Right= 0;
	Width = width - Left - Right;
	res = height - Height - Top - Bottom;
	if  (res < 0) res = 0;
	rata = res / 16;
	Top += rata;
	Bottom += rata;
	Height = height - Top - Bottom;
}
