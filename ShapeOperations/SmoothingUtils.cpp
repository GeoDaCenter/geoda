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

#include <algorithm>
#include <assert.h>
#include <cfloat>
#include <wx/stopwatch.h>
#include "Lowess.h"
#include "SmoothingUtils.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "../logger.h"

/** reg_line, slope, infinite_slope and regression_defined are all return
 values. */
void SmoothingUtils::CalcRegressionLine(GdaPolyLine& reg_line,
										double& slope,
										bool& infinite_slope,
										bool& regression_defined,
										wxRealPoint& reg_a,
										wxRealPoint& reg_b,
										double& cc_degs_of_rot,
										const AxisScale& axis_scale_x,
										const AxisScale& axis_scale_y,
										const SimpleLinearRegression& reg,
										const wxPen& pen)
{
	reg_line.setBrush(*wxTRANSPARENT_BRUSH); // ensure brush is transparent
	slope = 0; //default
	infinite_slope = false; // default
	regression_defined = true; // default
	
	if (!reg.valid) {
		regression_defined = false;
		reg_line.setPen(*wxTRANSPARENT_PEN);
		return;
	}
	
	reg_a = wxRealPoint(0, 0);
	reg_b = wxRealPoint(0, 0);
	
	
	// bounding box is [axis_scale_x.scale_min, axis_scale_y.scale_max] x
	// [axis_scale_y.scale_min, axis_scale_y.scale_max]
	// By the constuction of the scale, we know that regression line is
	// not equal to any of the four bounding box lines.  Therefore, the
	// regression_line intersects the box at two unique points.  We must
	// determine the two points of interesection.
	if (reg.valid) {
		// It should be the case that the slope beta is at most 1/2.
		// So, we should calculate the points of intersection with the
		// two vertical bounding box lines.
        reg_a = wxRealPoint(axis_scale_x.scale_min,
                            reg.alpha + reg.beta*axis_scale_x.scale_min);
        reg_b = wxRealPoint(axis_scale_x.scale_max,
                            reg.alpha + reg.beta*axis_scale_x.scale_max);
        
		if (reg_a.y < axis_scale_y.scale_min) {
			reg_a.x = (axis_scale_y.scale_min - reg.alpha)/reg.beta;
			reg_a.y = axis_scale_y.scale_min;
            
		} else if (reg_a.y > axis_scale_y.scale_max) {
			reg_a.x = (axis_scale_y.scale_max - reg.alpha)/reg.beta;
			reg_a.y = axis_scale_y.scale_max;
            
		}
        
		if (reg_b.y < axis_scale_y.scale_min) {
			reg_b.x = (axis_scale_y.scale_min - reg.alpha)/reg.beta;
			reg_b.y = axis_scale_y.scale_min;
            
		} else if (reg_b.y > axis_scale_y.scale_max) {
			reg_b.x = (axis_scale_y.scale_max - reg.alpha)/reg.beta;
			reg_b.y = axis_scale_y.scale_max;
		}
        
		slope = reg.beta;
        
	} else {
		regression_defined = false;
		reg_line.setPen(*wxTRANSPARENT_PEN);
		return;
	}
	
	// scaling factors to transform to screen coordinates.
	double scaleX = 100.0 / (axis_scale_x.scale_range);
	double scaleY = 100.0 / (axis_scale_y.scale_range);	
	reg_a.x = (reg_a.x - axis_scale_x.scale_min) * scaleX;
	reg_a.y = (reg_a.y - axis_scale_y.scale_min) * scaleY;
	reg_b.x = (reg_b.x - axis_scale_x.scale_min) * scaleX;
	reg_b.y = (reg_b.y - axis_scale_y.scale_min) * scaleY;
	
	reg_line = GdaPolyLine(reg_a.x, reg_a.y, reg_b.x, reg_b.y);
    cc_degs_of_rot = RegLineToDegCCFromHoriz(reg_a.x, reg_a.y,
                                             reg_b.x, reg_b.y);
	reg_line.setPen(pen);
}

/** This method will be used for adding text annotations to the displayed
 regression lines.  To avoid drawing text upside down, we will only
 returns values in the range [90,-90). The return value is the degrees
 of counter-clockwise rotation from the x-axis. */
double SmoothingUtils::RegLineToDegCCFromHoriz(double a_x, double a_y,
											   double b_x, double b_y)
{	
	double dist = GenUtils::distance(wxRealPoint(a_x,a_y),wxRealPoint(b_x,b_y));
	if (dist <= 4*DBL_MIN) return 0;
	// normalize slope vector c = (c_x, c_y)
	double x = (b_x - a_x) / dist;
	double y = (b_y - a_y) / dist;
	const double eps = 0.00001;
	if (-eps <= x && x <= eps) return 90;
	if (-eps <= y && y <= eps) return 0;
	
	//Recall: (x,y) = (cos(theta), sin(theta))  and  theta = acos(x)
	double theta = acos(x) * 57.2957796; // 180/pi = 57.2957796
	if (y < 0) theta = 360.0 - theta;
	
	return theta;
}

void
SmoothingUtils::CalcStatsRegimes(const std::vector<double>& X,
                                 const std::vector<double>& Y,
                                 const std::vector<bool>& X_undef,
                                 const std::vector<bool>& Y_undef,
                                 const SampleStatistics& statsX,
                                 const SampleStatistics& statsY,
                                 const SimpleLinearRegression& regressionXY,
                                 const std::vector<bool>& hl,
                                 SampleStatistics& statsXselected,
                                 SampleStatistics& statsYselected,
                                 SampleStatistics& statsXexcluded,
                                 SampleStatistics& statsYexcluded,
                                 SimpleLinearRegression& regressionXYselected,
                                 SimpleLinearRegression& regressionXYexcluded,
                                 double& sse_sel,
                                 double& sse_unsel)
{
	// find mean for X and Y according to highlight_state for both
	// the currently selected, and the complement.
	statsXselected = SampleStatistics();
	statsYselected = SampleStatistics();
	statsXexcluded = SampleStatistics();
	statsYexcluded = SampleStatistics();
	regressionXYselected = SimpleLinearRegression();
	regressionXYexcluded = SimpleLinearRegression();
    
	int selected_cnt = 0;
	int excluded_cnt = 0;
	
	// calculate mean, min and max
    statsXselected.min = std::numeric_limits<double>::max();
	statsYselected.min = std::numeric_limits<double>::max();
	statsXexcluded.min = std::numeric_limits<double>::max();
	statsYexcluded.min = std::numeric_limits<double>::max();
	statsXselected.max = -std::numeric_limits<double>::max();
	statsYselected.max = -std::numeric_limits<double>::max();
	statsXexcluded.max = -std::numeric_limits<double>::max();
	statsYexcluded.max = -std::numeric_limits<double>::max();
    
	for (int i=0, iend=X.size(); i<iend; i++) {
        if (X_undef[i] || Y_undef[i])
            continue;
		if (hl[i]) {
			selected_cnt++;
			statsXselected.mean += X[i];
			statsYselected.mean += Y[i];
			if (X[i] < statsXselected.min) statsXselected.min = X[i];
			if (Y[i] < statsYselected.min) statsYselected.min = Y[i];
			if (X[i] > statsXselected.max) statsXselected.max = X[i];
			if (Y[i] > statsYselected.max) statsYselected.max = Y[i];
		} else {
			excluded_cnt++;
			statsXexcluded.mean += X[i];
			statsYexcluded.mean += Y[i];
			if (X[i] < statsXexcluded.min) statsXexcluded.min = X[i];
			if (Y[i] < statsYexcluded.min) statsYexcluded.min = Y[i];
			if (X[i] > statsXexcluded.max) statsXexcluded.max = X[i];
			if (Y[i] > statsYexcluded.max) statsYexcluded.max = Y[i];
		}
	}
    
	if (selected_cnt == 0) {
		statsXexcluded = statsX;
		statsYexcluded = statsY;
		regressionXYexcluded = regressionXY;
        
	} else if (excluded_cnt == 0) {
		statsXselected = statsX;
		statsYselected = statsY;
		regressionXYselected = regressionXY;
        
	} else {
		statsXselected.mean /= selected_cnt;
		statsYselected.mean /= selected_cnt;
		statsXexcluded.mean /= excluded_cnt;
		statsYexcluded.mean /= excluded_cnt;
		statsXselected.sample_size = selected_cnt;
		statsYselected.sample_size = selected_cnt;
		statsXexcluded.sample_size = excluded_cnt;
		statsYexcluded.sample_size = excluded_cnt;
		
		double sum_squaresXselected = 0;
		double sum_squaresYselected = 0;
		double sum_squaresXexcluded = 0;
		double sum_squaresYexcluded = 0;
        
		// calculate standard deviations and variances
		for (int i=0, iend=X.size(); i<iend; i++) {
            if (X_undef[i] || Y_undef[i])
                continue;
			if (hl[i]) {
				sum_squaresXselected += X[i] * X[i];
				sum_squaresYselected += Y[i] * Y[i];
			} else {
				sum_squaresXexcluded += X[i] * X[i];
				sum_squaresYexcluded += Y[i] * Y[i];
			}
		}
		
		SmoothingUtils::CalcVarSdFromSumSquares(statsXselected,
                                                sum_squaresXselected);
		SmoothingUtils::CalcVarSdFromSumSquares(statsYselected,
                                                sum_squaresYselected);
		SmoothingUtils::CalcVarSdFromSumSquares(statsXexcluded,
                                                sum_squaresXexcluded);
		SmoothingUtils::CalcVarSdFromSumSquares(statsYexcluded,
                                                sum_squaresYexcluded);
		
		SmoothingUtils::CalcRegressionSelOrExcl(statsXselected, statsYselected,
                                                X, Y, X_undef, Y_undef, hl,
                                                true, regressionXYselected,
                                                sse_sel);
        SmoothingUtils::CalcRegressionSelOrExcl(statsXexcluded, statsYexcluded,
                                                X, Y, X_undef, Y_undef, hl,
                                                false, regressionXYexcluded,
                                                sse_unsel);
	}
}

void SmoothingUtils::CalcRegressionSelOrExcl(const SampleStatistics& ss_X,
											 const SampleStatistics& ss_Y,
											 const std::vector<double>& X,
											 const std::vector<double>& Y,
                                             const std::vector<bool>& X_undef,
                                             const std::vector<bool>& Y_undef,
											 const std::vector<bool>& hl,
											 bool selected,
											 SimpleLinearRegression& r,
											 double& ss_error)
{
	if (ss_X.sample_size != ss_Y.sample_size ||
        ss_X.sample_size < 2 ||
        ss_X.var_without_bessel <= 4*DBL_MIN )
    {
        return;
    }
	
	int n=0;
	double expectXY = 0;
	double sum_x_squared = 0;
    
	if (selected) {
		for (int i=0, iend=X.size(); i<iend; i++) {
            if (X_undef[i] || Y_undef[i])
                continue;
			if (hl[i]) {
				expectXY += X[i]*Y[i];
				sum_x_squared += X[i] * X[i];
				n++;
			}
		}
	} else {
		for (int i=0, iend=X.size(); i<iend; i++) {
            if (X_undef[i] || Y_undef[i])
                continue;
			if (!hl[i]) {
				expectXY += X[i]*Y[i];
				sum_x_squared += X[i] * X[i];
				n++;
			}
		}
	}
	expectXY /= (double) ss_X.sample_size;
	
	r.covariance = expectXY - ss_X.mean * ss_Y.mean;
	r.beta = r.covariance / ss_X.var_without_bessel;
    
	double d = ss_X.sd_without_bessel * ss_Y.sd_without_bessel;
    
	if (d > 4*DBL_MIN) {
		r.correlation = r.covariance / d;
		r.valid_correlation = true;
	} else {
		r.valid_correlation = false;
	}
	
	r.alpha = ss_Y.mean - r.beta * ss_X.mean;
	r.valid = true;
	
	double SS_tot = ss_Y.var_without_bessel * ss_Y.sample_size;
	double SS_err = 0;
	double err=0;
	if (selected) {
		for (int i=0, iend=Y.size(); i<iend; i++) {
            if (X_undef[i] || Y_undef[i])
                continue;
			if (hl[i]) {
				err = Y[i] - (r.alpha + r.beta * X[i]);
				SS_err += err * err;
			}
		}
		ss_error = SS_err;
	} else {
		for (int i=0, iend=Y.size(); i<iend; i++) {
            if (X_undef[i] || Y_undef[i])
                continue;
			if (!hl[i]) {
				err = Y[i] - (r.alpha + r.beta * X[i]);
				SS_err += err * err;
			}
		}
		ss_error = SS_err;
	}
	if (SS_err < 16*DBL_MIN) {
		r.r_squared = 1;
	} else {
		r.r_squared = 1 - SS_err / SS_tot;
	}
	if (n>2 && ss_X.var_without_bessel > 4*DBL_MIN) {
		r.std_err_of_estimate = SS_err/(n-2); // SS_err/(n-k-1), k=1
		r.std_err_of_estimate = sqrt(r.std_err_of_estimate);
		r.std_err_of_beta = r.std_err_of_estimate/
		sqrt(n*ss_X.var_without_bessel);
		r.std_err_of_alpha = r.std_err_of_beta * sqrt(sum_x_squared / n);
		
		if (r.std_err_of_alpha >= 16*DBL_MIN) {
			r.t_score_alpha = r.alpha / r.std_err_of_alpha;
		} else {
			r.t_score_alpha = 100;
		}
		if (r.std_err_of_beta >= 16*DBL_MIN) {
			r.t_score_beta = r.beta / r.std_err_of_beta;
		} else {
			r.t_score_beta = 100;
		}
		r.p_value_alpha =
		SimpleLinearRegression::TScoreTo2SidedPValue(r.t_score_alpha, n-2);
		r.p_value_beta =
		SimpleLinearRegression::TScoreTo2SidedPValue(r.t_score_beta, n-2);
		
		r.valid_std_err = true;
	}	
}

void SmoothingUtils::CalcVarSdFromSumSquares(SampleStatistics& ss,
											 double sum_squares)
{
	double n = ss.sample_size;
	ss.var_without_bessel = (sum_squares/n) - (ss.mean*ss.mean);
	ss.sd_without_bessel = sqrt(ss.var_without_bessel);
	
	if (ss.sample_size == 1) {
		ss.var_with_bessel = ss.var_without_bessel;
		ss.sd_with_bessel = ss.sd_without_bessel;
	} else {
		ss.var_with_bessel = (n/(n-1)) * ss.var_without_bessel;
		ss.sd_with_bessel = sqrt(ss.var_with_bessel);
	}
}

bool SmoothingUtils::ExtendEndpointsToBB(const std::vector<double>& X,
										 const std::vector<double>& Y,
										 double bb_min_x, double bb_min_y,
										 double bb_max_x, double bb_max_y,
										 double& x_first, double& y_first,
										 double& x_last, double& y_last)
{
	size_t n = X.size();
	bool success = true;
	// Extend end points to bounding box with linear interpolation.
	{
		wxString m;
		// First extend towards left.
		// Find first point that differs from X[0], Y[0];
		double x0=X[0], y0=Y[0];
		double x1, y1;
		double x2=x0;
		double y2=y0;
		bool found = false;
		for (size_t i=1; i<n && !found; ++i) {
			if (!GenGeomAlgs::nearlyEqual(x0, X[i]) ||
					!GenGeomAlgs::nearlyEqual(y0, Y[i])) {
				x1 = X[i];
				y1 = Y[i];
				found = true;
			}
		}
		if (found) {
			wxString p0; p0 << "(" << x0 << ", " << y0 << ")";
			wxString p1; p1 << "(" << x1 << ", " << y1 << ")";
			if (GenGeomAlgs::ExtendRayToBB(x1, y1, x0, y0, x2, y2,
										   bb_min_x, bb_min_y, 
										   bb_max_x, bb_max_y))
			{
				wxString p2; p2 << "(" << x2 << ", " << y2 << ")";
				m << "Extended ray anchored at " << p1 << ", from ";
				m << p0 << " to " << p2;
			} else {
				m << "Could not extend ray " << p1 << ", " << p0;
				success = false;
			}
		} else {
			success = false;
		}
		x_first = x2;
		y_first = y2;
	}
	{
		wxString m;
		// First extend towards right.
		// Find first point that differs from X[n-1], Y[n-1];
		double x1=X[n-1], y1=Y[n-1];
		double x0, y0;
		double x2=x1;
		double y2=y1;
		bool found = false;
		for (size_t i=n-2; i>=0 && !found; --i) {
			if (!GenGeomAlgs::nearlyEqual(x0, X[i]) ||
					!GenGeomAlgs::nearlyEqual(y0, Y[i])) {
				x0 = X[i];
				y0 = Y[i];
				found = true;
			}
		}
		if (found) {
			wxString p0; p0 << "(" << x0 << ", " << y0 << ")";
			wxString p1; p1 << "(" << x1 << ", " << y1 << ")";
			if (GenGeomAlgs::ExtendRayToBB(x0, y0, x1, y1, x2, y2,
										   bb_min_x, bb_min_y, 
										   bb_max_x, bb_max_y))
			{
				wxString p2; p2 << "(" << x2 << ", " << y2 << ")";
				m << "Extended ray anchored at " << p0 << ", from ";
				m << p1 << " to " << p2;
			} else {
				m << "Could not extend ray " << p0 << ", " << p1;
				success = false;
			}
		} else {
			success = false;
		}
		x_last = x2;
		y_last = y2;
	}
	return success;
}

wxString SmoothingUtils::LowessCacheKey(int x_time, int y_time)
{
	wxString key;
	key << "x" << x_time << "y" << y_time;
	return key;
}


SmoothingUtils::LowessCacheEntry* 
SmoothingUtils::UpdateLowessCacheForTime(LowessCacheType& lowess_cache,
										 const wxString& key, Lowess& lowess,
										 const std::vector<double>& X,
										 const std::vector<double>& Y,
                                         const std::vector<bool>& XY_undefs)
{
	size_t n = X.size();
	SmoothingUtils::LowessCacheType::iterator it = lowess_cache.find(key);
	LowessCacheEntry* lce = 0;
    
	if (it != lowess_cache.end())
        lce = it->second;
	if (lce) {
		//LOG_MSG("LOWESS cache entry found for key: " + key);
		return lce;
	}
	//LOG_MSG("No LOWESS cache entry found for key: " + key);
   
    Gda::dbl_int_pair_vec_type Xpairs;
    
    int valid_n = 0;
	for (size_t i=0; i<n; ++i) {
        if (!XY_undefs[i]) {
            valid_n += 1;
            Xpairs.push_back(std::make_pair(X[i], i));
        }
    }
    
	// Sort in ascending order by value
	std::sort(Xpairs.begin(), Xpairs.end(), Gda::dbl_int_pair_cmp_less);
    
	lce = new LowessCacheEntry(valid_n);
    
	for (size_t i=0; i<Xpairs.size(); ++i) {
        int obj_id = Xpairs[i].second;
        double obj_val = Xpairs[i].first;
		lce->sort_map[i] = obj_id;
		lce->X_srt[i] = obj_val;
		lce->Y_srt[i] = Y[obj_id];
	}
	
	lowess.calc(lce->X_srt, lce->Y_srt, lce->YS_srt);
	lowess_cache[key] = lce;
	
	return lce;
}

void SmoothingUtils::CalcLowessRegimes(LowessCacheEntry* lce,
                                       Lowess& lowess,
                                       const std::vector<bool>& hl,
                                       std::vector<double>& sel_smthd_srt_x,
                                       std::vector<double>& sel_smthd_srt_y,
                                       std::vector<double>& unsel_smthd_srt_x,
                                       std::vector<double>& unsel_smthd_srt_y,
                                       std::vector<bool>& undefs)
{
	if (!lce || !(hl.size() > 1))
        return;
    
    
    size_t n = 0;
	size_t tot_hl = 0;
    size_t tot_uhl = 0;
    
    for (size_t i=0; i<hl.size(); ++i) {
        if (undefs[i])
            continue;
       
        n++;
        
        if (hl[i]) {
            ++tot_hl;
        } else
            ++tot_uhl;
    }
    
	sel_smthd_srt_x.resize(tot_hl);
	sel_smthd_srt_y.resize(tot_hl);
	unsel_smthd_srt_x.resize(tot_uhl);
	unsel_smthd_srt_y.resize(tot_uhl);

    
	if (tot_hl > 0) {
		size_t ss_size = tot_hl;
		std::vector<double> Y_sorted(ss_size);
		size_t ss_cnt = 0;
        
		for (size_t i=0; i<n; ++i) {
			size_t ii = lce->sort_map[i];
            if (undefs[ii])
                continue;
			if (hl[ii]) {
				sel_smthd_srt_x[ss_cnt] = lce->X_srt[i];
				Y_sorted[ss_cnt] = lce->Y_srt[i];
				++ss_cnt;
			}
		}
		//assert(ss_cnt == ss_size);
		
		if (ss_size == 1) {
			sel_smthd_srt_y[0] = Y_sorted[0];
		} else {
			lowess.calc(sel_smthd_srt_x, Y_sorted, sel_smthd_srt_y);
		}
	}
	
	if (tot_uhl > 0) {
		size_t ss_size = tot_uhl;
		std::vector<double> Y_sorted(ss_size);
		size_t ss_cnt = 0;
        
		for (size_t i=0; i<n; ++i) {
			size_t ii = lce->sort_map[i];
            if (undefs[ii])
                continue;
			if (!hl[ii]) {
				//X_sorted[ss_cnt] = lce->X_srt[i];
				unsel_smthd_srt_x[ss_cnt] = lce->X_srt[i];
				Y_sorted[ss_cnt] = lce->Y_srt[i];
				++ss_cnt;
			}
		}
		//assert(ss_cnt == ss_size);
		
		if (ss_size == 1) {
			//YS_sorted[0] = Y_sorted[0];
			unsel_smthd_srt_y[0] = Y_sorted[0];
		} else {
			//lowess.calc(X_sorted, Y_sorted, YS_sorted);
			lowess.calc(unsel_smthd_srt_x, Y_sorted, unsel_smthd_srt_y);
		}
	}
}

void SmoothingUtils::EmptyLowessCache(LowessCacheType& lowess_cache)
{
	for (LowessCacheType::iterator i=lowess_cache.begin();
			 i!=lowess_cache.end(); ++i)
	{
		wxString key = i->first;
		LowessCacheEntry* lce = i->second;
		if (lce) delete lce;
	}
	lowess_cache.clear();
}

ScatterPlotPens::ScatterPlotPens()
{
	reg_pens_trans_scalar = 0.65;
	SetPenColor(&reg_pen, GdaConst::scatterplot_regression_color);
	reg_pen.SetWidth(2);
	
	SetPenColor(&reg_sel_pen, GdaConst::scatterplot_regression_selected_color);
	reg_sel_pen.SetWidth(2);
	
	SetPenColor(&reg_exl_pen, GdaConst::scatterplot_regression_excluded_color);
	reg_exl_pen.SetWidth(2);
}

void ScatterPlotPens::SetPenColor(wxPen* pen, const wxColour& color)
{
	pen->SetColour(color);
	SetPenTrans(pen, reg_pens_trans_scalar);
}

/** trans_val: Transparance Value 0 to 1 */
void ScatterPlotPens::SetPenTrans(wxPen* pen, double trans_val)
{
	if (trans_val < 0) trans_val = 0;
	if (trans_val > 1) trans_val = 1;
	int alpha = 255 * trans_val;
	wxColour cur_col(pen->GetColour());
	cur_col.ChangeLightness(alpha);
	pen->SetColour(cur_col);
}

