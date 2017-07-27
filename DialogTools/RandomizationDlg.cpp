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

#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcbuffer.h>

#include "../rc/GeoDaIcon-16x16.xpm"
#include "../ShapeOperations/GalWeight.h"
#include "../GeoDa.h"
#include "../TemplateCanvas.h"
#include "../GdaConst.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "RandomizationDlg.h"


/////////////////////////////////////////////////////////////////////////////
//
//  --------------------------------------
//  |          Inference Settings         |
//  |-------------------------------------|
//  |                                     |
//  |  Bonferroni Bounds:  __0.43__       |
//  |                                     |
//  |  False  Discovery Rate: __0.____    |
//  |                                     |
//  |  Use specified p-value: _______     |
//  |                                     |
//  |                                     |
//  |                                     |
//  |   [ O K ]       [ Cancel ]          |
//  --------------------------------------
/////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( InferenceSettingsDlg, wxDialog )
EVT_BUTTON( wxID_OK, InferenceSettingsDlg::OnOkClick )
END_EVENT_TABLE()

InferenceSettingsDlg::InferenceSettingsDlg(wxWindow* parent,
                                           double _p_cutoff,
                                           double* _p_vals,
                                           int _n,
                                           Project* _p,
                                           wxWindowID id,
                                           const wxString& title,
                                           const wxPoint& pos,
                                           const wxSize& size )
: wxDialog(parent, id, title, pos, size), p_cutoff(_p_cutoff), p_vals(_p_vals), n(_n), fdr(0), bo(0)
{
    wxLogMessage("Open InferenceSettingsDlg.");
    
    wxString p_str = wxString::Format("%g", p_cutoff);
    wxPanel *panel = new wxPanel(this);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    
    // Parameters
    wxFlexGridSizer* gbox = new wxFlexGridSizer(9,2,5,0);
    wxStaticText* st10 = new wxStaticText(panel, wxID_ANY, _("Bonferroni bound:"),
                                          wxDefaultPosition, wxSize(180,-1));
    m_txt_bo = new wxStaticText(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(150,-1));
    gbox->Add(st10, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_txt_bo, 1, wxEXPAND);
    wxStaticText* st11 = new wxStaticText(panel, wxID_ANY, _("False Discovery Rate:"),
                                          wxDefaultPosition, wxSize(180,-1));
    m_txt_fdr = new wxStaticText(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(150,-1));
    gbox->Add(st11, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(m_txt_fdr, 1, wxEXPAND);
    wxStaticText* st12 = new wxStaticText(panel, wxID_ANY, _("Use specified significance:"),
                                          wxDefaultPosition, wxSize(180,-1));
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    chk_pval = new wxCheckBox(panel, wxID_ANY, "");
    m_txt_pval = new wxTextCtrl(panel, XRCID("ID_INFERENCE_TCTRL"), p_str, wxDefaultPosition, wxSize(100,-1),wxTE_PROCESS_ENTER);
    m_txt_pval->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    //Connect(XRCID("ID_INFERENCE_TCTRL"), wxEVT_TEXT_ENTER, wxCommandEventHandler(InferenceSettingsDlg::OnAlphaTextCtrl));
    Connect(XRCID("ID_INFERENCE_TCTRL"), wxEVT_TEXT,
            wxCommandEventHandler(InferenceSettingsDlg::OnAlphaTextCtrl));
    chk_pval->SetValue(true);
    
    hbox1->Add(chk_pval,0, wxALIGN_CENTER_VERTICAL | wxTOP, 12);
    hbox1->Add(m_txt_pval,0,wxALIGN_CENTER_VERTICAL | wxTOP, 10);
    gbox->Add(st12, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 10);
    gbox->Add(hbox1, 1, wxEXPAND);

    // Buttons
    wxButton *okButton = new wxButton(panel, wxID_OK, wxT("OK"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(panel, wxID_CANCEL, wxT("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    hbox2->Add(okButton, 1, wxALIGN_CENTER | wxALL, 5);
    hbox2->Add(closeButton, 1, wxALIGN_CENTER | wxALL, 5);
    
    // Container
    vbox->Add(gbox, 1,  wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxALL, 10);
    
    wxBoxSizer *container = new wxBoxSizer(wxHORIZONTAL);
    container->Add(vbox);
    
    panel->SetSizer(container);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL, 0);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    sizerAll->Fit(this);
    
    Centre();
    
    Init(p_vals, n, p_cutoff);
}

int compare (const void * a, const void * b)
{
    if (*(double*)a > *(double*)b) return 1;
    else if (*(double*)a < *(double*)b) return -1;
    else return 0;
}

void InferenceSettingsDlg::Init(double* p_vals, int n, double current_p)
{
    double bonferroni_bound = current_p / (double)n;
    wxString bo_str = wxString::Format("%g", bonferroni_bound);;
    m_txt_bo->SetLabel(bo_str);
    
    std::vector<double> pvals;
    for (int i=0; i<n; i++) pvals.push_back(p_vals[i]);
    
    // FDR
    // sort all p-values from smallest to largets
    std::sort(pvals.begin(), pvals.end());

    int i_0 = -1;
    bool stop = false;
    double p_start = current_p;
    
    while (!stop) {
        // find the i_0 that corresponds to p = alpha
        for (int i=1; i<=n; i++) {
            if (pvals[i] >= p_start) {
                if (i_0 == i) {
                    stop = true;
                }
                i_0 = i;
                break;
            }
        }
        if (i_0 < 0)
            stop = true;
        
        // compute p* = i_0 x alpha / N
        p_start = i_0 * current_p / (double)n ;
    }
    
    wxString fdr_str = wxString::Format("%g", p_start);
    m_txt_fdr->SetLabel(fdr_str);
    
    bo = bonferroni_bound;
    fdr = p_start;
}

void InferenceSettingsDlg::OnAlphaTextCtrl(wxCommandEvent& ev)
{
    wxString val = m_txt_pval->GetValue();
    val.Trim(false);
    val.Trim(true);
    double pval;
    bool is_valid = val.ToDouble(&pval);
    if (is_valid) {
        Init(p_vals, n, pval);
    }
    ev.Skip();
}

void InferenceSettingsDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("In InferenceSettingsDlg::OnOkClick()");
    
    if (chk_pval->GetValue()) {
        wxString p_val = m_txt_pval->GetValue();
        p_val.ToDouble(&p_cutoff);
    
        EndDialog(wxID_OK);
    } else {
        EndDialog(wxID_CANCEL);
    }
}

/////////////////////////////////////////////////////////////////////////////

RandomizationPanel::RandomizationPanel(const std::vector<double>& raw_data1_s,
                                       const std::vector<bool>& undefs_s,
                                       const GalElement* W_s, int NumPermutations,
                                       bool reuse_user_seed,
                                       uint64_t user_specified_seed,
                                       wxFrame* parent,
                                       const wxSize& size)
: start(-1),
stop(1),
raw_data1(raw_data1_s),
undefs(undefs_s),
W(W_s),
num_obs(raw_data1_s.size()),
Permutations(NumPermutations),
MoranI(NumPermutations, 0),
is_bivariate(false),
wxPanel(parent, -1, wxDefaultPosition, size)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    
    Connect(wxEVT_PAINT, wxPaintEventHandler(RandomizationPanel::OnPaint));
    Connect(wxEVT_SIZE, wxSizeEventHandler(RandomizationPanel::OnSize));
    Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(RandomizationPanel::OnMouse));

    if (reuse_user_seed)
        rng = new Randik(user_specified_seed);
    else
        rng = new Randik();
    
	CalcMoran();
    Init();
}

RandomizationPanel::RandomizationPanel(const std::vector<double>& raw_data1_s,
                                       const std::vector<double>& raw_data2_s,
                                       const std::vector<bool>& undefs_s,
                                       const GalElement* W_s, int NumPermutations,
                                       bool reuse_user_seed,
                                       uint64_t user_specified_seed,
                                       wxFrame* parent,
                                       const wxSize& size)
: start(-1), stop(1), raw_data1(raw_data1_s), raw_data2(raw_data2_s),
undefs(undefs_s),
W(W_s),
num_obs(raw_data1_s.size()), Permutations(NumPermutations),
MoranI(NumPermutations, 0), is_bivariate(true),
wxPanel(parent, -1, wxDefaultPosition, size)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    
    Connect(wxEVT_PAINT, wxPaintEventHandler(RandomizationPanel::OnPaint));
    Connect(wxEVT_SIZE, wxSizeEventHandler(RandomizationPanel::OnSize));
	Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(RandomizationPanel::OnMouse));
    
    if (reuse_user_seed)
        rng = new Randik(user_specified_seed);
    else
        rng = new Randik();
    
	CalcMoran();
    Init();
}

RandomizationPanel::~RandomizationPanel()
{
	if (perm) delete [] perm;
	if (theRands) delete [] theRands;
	if (rng) 
		delete rng;
}

void RandomizationPanel::OnMouse( wxMouseEvent& event )
{
	if (event.RightUp()) {
		wxMenu* popupMenu = new wxMenu(wxEmptyString);
		popupMenu->Append(XRCID("RUN_RANDOM"), "Run");
		Connect(XRCID("RUN_RANDOM"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RandomizationPanel::OnRunClick));
		PopupMenu(popupMenu, event.GetPosition());
	}
}

void RandomizationPanel::OnRunClick( wxCommandEvent& event )
{
	RunRandomTrials();
	Refresh();
}

void RandomizationPanel::CalcMoran()
{
    std::vector<bool> XY_undefs;
    std::vector<double> X;
    std::vector<double> Y;
   
    for (int i=0; i<num_obs; i++) {
        if (undefs[i])
            continue;
        double Wdata = 0;
        if (is_bivariate) {
            Wdata = W[i].SpatialLag(raw_data2);
        } else {
            Wdata = W[i].SpatialLag(raw_data1);
		}
        X.push_back(raw_data1[i]);
        Y.push_back(Wdata);
        XY_undefs.push_back(false);
    }
    
    SampleStatistics statsX(X, XY_undefs);
    SampleStatistics statsY(Y, XY_undefs);
    SimpleLinearRegression lreg (X, Y, XY_undefs, XY_undefs,
                                         statsX.mean, statsY.mean,
                                         statsX.var_without_bessel,
                                         statsY.var_without_bessel);
    Moran = lreg.beta;
    /*
	Moran = 0;
    for (int i=0; i<num_obs; i++) {
        if (is_bivariate) {
			Moran += W[i].SpatialLag(raw_data2) * raw_data1[i];
        } else {
			Moran += W[i].SpatialLag(raw_data1) * raw_data1[i];
		}
	}
	Moran /= (double) num_obs - 1.0;
     */
}

void RandomizationPanel::Init()
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


// NOTE: must carefully look at thresholdBin!
void RandomizationPanel::RunRandomTrials()
{
	totFrequency = 0;
	for (int i=0; i<bins; i++) 
		freq[i]=0;
	
	// thresholdBin bin already has one permutation
	freq[ thresholdBin ] = 1;
	// leftmost and the righmost are the same so far
	minBin = thresholdBin; 
	maxBin = thresholdBin;
	
	for (int i=0; i<Permutations; i++) {
		//create a random permutation
		rng->Perm(num_obs, perm, theRands);
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

void RandomizationPanel::UpdateStatistics()
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

void RandomizationPanel::DrawRectangle(wxDC* dc, int left, int top, int right,
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

void RandomizationPanel::Draw(wxDC* dc)
{
    // make a copy of freq;
    std::vector<int> freq_back(freq);
    
    // draw white background
    wxSize sz = this->GetClientSize();
    DrawRectangle(dc, 0, 0, sz.x, sz.y, GdaConst::canvas_background_color);
    
	int fMax = freq_back[0];
	for (int i=1; i<bins; i++) 
        if (fMax < freq_back[i])
            fMax = freq_back[i];

	for (int i=0; i < bins; i++) {
		double df = double (freq_back[i]* Height) / double (fMax);
		freq_back[i] = int(df);
	}

	wxColour color = count_greater ? GdaConst::outliers_colour : GdaConst::textColor;
	for (int i=0; i < thresholdBin; i++) {
		if (freq_back[i] > 0) {
			int xx = Top + Height - freq_back[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}

	color = !count_greater ? GdaConst::outliers_colour : GdaConst::textColor;
	for (int i=thresholdBin+1; i < bins; i++) {
		if (freq_back[i] > 0) {
			int xx = Top + Height - freq_back[i];
			if (xx < Top) xx=Top;
			DrawRectangle(dc, Left + i*binX, xx,
						  Left + i*binX + binX, Top + Height, color);
		}
	}	
	
	DrawRectangle(dc, Left + thresholdBin*binX, Top,
				  Left + (thresholdBin+1)*binX, Top+  Height,
				  //GdaConst::highlight_color );
                  wxColour(49, 163, 84));

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
  
	//int fs = 4 + Bottom/4;
	//wxFont nf(*wxSMALL_FONT);
	//nf.SetPointSize(13);
	//dc->SetFont(nf);

	dc->SetFont(*GdaConst::small_font);
	
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
	dc->DrawText(text, Left+5, 35);

	text = wxString::Format("pseudo p-value: %-7.6f", pseudo_p_val);
	dc->DrawText(text, Left+5, 50);
}


void RandomizationPanel::OnSize( wxSizeEvent& event )
{
    Refresh();
}

void RandomizationPanel::OnPaint( wxPaintEvent& event )
{
	if (!experiment_run_once) {
		experiment_run_once = true;
		//wxCommandEvent ev;
		//OnOkClick(ev);
        RunRandomTrials();
        Refresh();
	}
    wxAutoBufferedPaintDC dc(this);
	dc.Clear();
	Paint(&dc);
}

void RandomizationPanel::Paint(wxDC *dc)
{
	wxRect rcClient = GetClientRect();
	CheckSize(rcClient.GetWidth(), rcClient.GetHeight());
	
    Draw(dc);
}

void RandomizationPanel::CheckSize(const int width, const int height)
{
	Left = 10;
    Bottom = 20;
	Right = 10;
	Top = 60;
	Height = 0;
	Width = 0;
	
	int res = width - Width - Left - Right;
	if (res < 0) res = 0;
	int rata = (int) floor((double) res / (bins + 1));
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


const int ID_BUTTON = wxID_ANY;

BEGIN_EVENT_TABLE( RandomizationDlg, wxFrame)
    EVT_CLOSE( RandomizationDlg::OnClose)
    EVT_BUTTON( XRCID("ID_OK"), RandomizationDlg::OnOkClick )
	EVT_MOUSE_EVENTS(RandomizationDlg::OnMouse)
END_EVENT_TABLE()

RandomizationDlg::RandomizationDlg(const std::vector<double>& raw_data1_s,
								   const std::vector<double>& raw_data2_s,
								   const GalWeight* W_s,
                                   const std::vector<bool>& _undef,
                                   const std::vector<bool>& hl,
                                   bool _is_regime,
								   int NumPermutations,
                                   bool reuse_user_seed,
                                   uint64_t user_specified_seed,
								   wxWindow* parent, wxWindowID id,
								   const wxString& caption, 
								   const wxPoint& pos, const wxSize& size,
								   long style )
: wxFrame(parent, id, "", wxDefaultPosition, wxSize(550,300)),
copy_w(NULL), copy_w_sel(NULL), copy_w_unsel(NULL), is_regime(_is_regime)
{
	wxLogMessage("Open RandomizationDlg (bivariate).");
	
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
    SetBackgroundColour(*wxWHITE);
   
    int num_obs = raw_data1_s.size();
    
    bool has_undef = false;
    bool has_hl = false;
    
    for (int i=0; i<num_obs;i++) if (_undef[i]) has_undef = true;
    for (int i=0; i<num_obs;i++) if (hl[i]) has_hl = true;
   
    wxSize sz(500,300);
   
    if (has_hl ) {
        sz.SetHeight(200);
    }
    
    GalElement* W = NULL;
    if (has_undef) {
        copy_w = new GalWeight(*W_s);
        copy_w->Update(_undef);
        W = copy_w->gal;
    } else {
        W = W_s->gal;
    }
    
    panel = new RandomizationPanel(raw_data1_s, raw_data2_s, _undef, W, NumPermutations, reuse_user_seed, user_specified_seed, this, sz);
    
    
    if ( has_hl && is_regime) {
        std::vector<bool> sel_undefs(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            sel_undefs[i] = _undef[i] || !hl[i];
        }
        copy_w_sel = new GalWeight(*W_s);
        copy_w_sel->Update(sel_undefs);
        panel_sel = new RandomizationPanel(raw_data1_s, raw_data2_s, sel_undefs, copy_w_sel->gal, NumPermutations, reuse_user_seed, user_specified_seed, this, sz);
        
        std::vector<bool> unsel_undefs(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            unsel_undefs[i] = _undef[i] || hl[i];
        }
        copy_w_unsel = new GalWeight(*W_s);
        copy_w_unsel->Update(unsel_undefs);
        panel_unsel = new RandomizationPanel(raw_data1_s, raw_data2_s, unsel_undefs, copy_w_unsel->gal, NumPermutations, reuse_user_seed, user_specified_seed, this, sz);
        
        CreateControls_regime();
        
    } else {
        CreateControls();
    }
}

RandomizationDlg::RandomizationDlg( const std::vector<double>& raw_data1_s,
								   const GalWeight* W_s,
                                   const std::vector<bool>& _undef,
                                   const std::vector<bool>& hl,
                                   bool _is_regime,
								   int NumPermutations,
                                   bool reuse_user_seed,
                                   uint64_t user_specified_seed,
								   wxWindow* parent, wxWindowID id,
								   const wxString& caption, 
								   const wxPoint& pos, const wxSize& size,
								   long style )
: wxFrame(parent, id, "", wxDefaultPosition, wxSize(550,300)),
copy_w(NULL), copy_w_sel(NULL), copy_w_unsel(NULL), is_regime(_is_regime)
{
	wxLogMessage("Open RandomizationDlg (univariate).");
	
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
    SetBackgroundColour(*wxWHITE);
    
    int num_obs = raw_data1_s.size();
    
    bool has_undef = false;
    bool has_hl = false;
    
    for (int i=0; i<num_obs;i++) if (_undef[i]) has_undef = true;
    for (int i=0; i<num_obs;i++) if (hl[i]) has_hl = true;

    wxSize sz(500,300);
    
    if (has_hl ) {
        sz.SetHeight(200);
    }
    
    GalElement* W = W_s->gal;
    if (has_undef) {
        copy_w = new GalWeight(*W_s);
        copy_w->Update(_undef);
        W = copy_w->gal;
    }
    panel = new RandomizationPanel(raw_data1_s, _undef, W, NumPermutations, reuse_user_seed, user_specified_seed, this, sz);
   
   
    if ( has_hl && is_regime ) {
        std::vector<bool> sel_undefs(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            sel_undefs[i] = _undef[i] || !hl[i];
        }
        copy_w_sel = new GalWeight(*W_s);
        copy_w_sel->Update(sel_undefs);
        panel_sel = new RandomizationPanel(raw_data1_s, sel_undefs, copy_w_sel->gal, NumPermutations, reuse_user_seed, user_specified_seed, this,sz);
        
        std::vector<bool> unsel_undefs(num_obs, false);
        for (int i=0; i<num_obs; i++) {
            unsel_undefs[i] = _undef[i] || hl[i];
        }
        copy_w_unsel = new GalWeight(*W_s);
        copy_w_unsel->Update(unsel_undefs);
        panel_unsel = new RandomizationPanel(raw_data1_s, unsel_undefs, copy_w_unsel->gal, NumPermutations, reuse_user_seed, user_specified_seed, this, sz);
        
        CreateControls_regime();
        
    } else {
        CreateControls();
    }
}



RandomizationDlg::~RandomizationDlg()
{
    wxLogMessage("Close RandomizationDlg");
    
    if (copy_w) {
        delete copy_w;
        copy_w = NULL;
    }
    if (copy_w_sel) {
        delete copy_w_sel;
        copy_w_sel = NULL;
    }
    if (copy_w_unsel) {
        delete copy_w_unsel;
        copy_w_unsel = NULL;
    }
}

void RandomizationDlg::CreateControls()
{
    wxButton *button = new wxButton(this, ID_BUTTON, wxT("Run"));
   
    wxBoxSizer* panel_box = new wxBoxSizer(wxVERTICAL);
    panel_box->Add(button, 0, wxALIGN_CENTER | wxALIGN_TOP | wxALL, 10);
    panel_box->Add(panel, 1, wxEXPAND | wxALL, 10);
    
    SetSizer(panel_box);
    panel_box->Fit(this);
    Centre();
    
    Connect(ID_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(RandomizationDlg::OnOkClick));
}

void RandomizationDlg::CreateControls_regime()
{
    wxButton *button = new wxButton(this, ID_BUTTON, wxT("Run"));
    
    wxBoxSizer* panel_box = new wxBoxSizer(wxVERTICAL);
    panel_box->Add(button, 0, wxALIGN_CENTER | wxALIGN_TOP | wxALL, 10);
    wxStaticText* lbl1 = new wxStaticText(this, wxID_ANY, _("All"));
    lbl1->SetFont(*GdaConst::small_font);
    panel_box->Add(lbl1, 0,  wxALIGN_CENTER| wxALL, 0);
    panel_box->Add(panel, 1, wxEXPAND | wxALL, 10);
    wxStaticText* lbl2 = new wxStaticText(this, wxID_ANY, _("Selected"));
    lbl2->SetFont(*GdaConst::small_font);
    panel_box->Add(lbl2, 0,  wxALIGN_CENTER | wxALL, 0);
    panel_box->Add(panel_sel, 1, wxEXPAND | wxALL, 10);
    wxStaticText* lbl3 = new wxStaticText(this, wxID_ANY, _("Unselected"));
    lbl3->SetFont(*GdaConst::small_font);
    panel_box->Add(lbl3, 0,  wxALIGN_CENTER | wxALL, 0);
    panel_box->Add(panel_unsel, 1, wxEXPAND | wxALL, 10);
    
    SetSizer(panel_box);
    panel_box->Fit(this);
    Centre();
    
    Connect(ID_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(RandomizationDlg::OnRunAll));
}

void RandomizationDlg::OnClose( wxCloseEvent& event )
{
    Destroy();
    event.Skip();
}

void RandomizationDlg::OnRunAll( wxCommandEvent& event )
{
    wxLogMessage("Click RandomizationDlg::OnRunAll");
	panel->RunRandomTrials();
	panel->Refresh();
	panel_sel->RunRandomTrials();
	panel_sel->Refresh();
	panel_unsel->RunRandomTrials();
	panel_unsel->Refresh();
}

void RandomizationDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("Click RandomizationDlg::OnOkClick");
	panel->RunRandomTrials();
	panel->Refresh();
}
void RandomizationDlg::OnMouse( wxMouseEvent& event )
{
	if (event.RightDown()) {
		wxMenu* popupMenu = new wxMenu(wxEmptyString);
		popupMenu->Append(XRCID("RUN_RANDOM"), "Run");
		Connect(XRCID("RUN_RANDOM"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RandomizationDlg::OnOkClick));
		PopupMenu(popupMenu, event.GetPosition());
	}
}

