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

#ifndef __GEODA_CENTER_SMOOTHING_UTILS_H__
#define __GEODA_CENTER_SMOOTHING_UTILS_H__

#include <map>
#include <vector>
#include <wx/string.h>
#include <wx/gdicmn.h> // for wxRealPoint
#include "Lowess.h"
#include "../GenGeomAlgs.h"
#include "../GdaShape.h"

/**
 These ultility functions are meant to support Scatterplot and
 similar views with some useful tools for processing linear
 and non-linear smoothing curves.  The choice of input/output formats
 is tied to the needs of GdaShape existing smoothing utilities used
 in GeoDa.  This has been for performance considerations in dealing
 with large data sets with real-time updates from mouse brushing.
 */

namespace SmoothingUtils {
    void CalcRegressionLine(GdaPolyLine& reg_line, // return value
                            double& slope, // return value
                            bool& infinite_slope, // return value
                            bool& regression_defined, // return value
                            wxRealPoint& a, // return value
                            wxRealPoint& b, // return value
                            double& cc_degs_of_rot, // return value
                            const AxisScale& axis_scale_x,
                            const AxisScale& axis_scale_y,
                            const SimpleLinearRegression& reg,
                            const wxPen& pen);
    
    double RegLineToDegCCFromHoriz(double a_x, double a_y,
                                   double b_x, double b_y);
    

    void CalcStatsRegimes(const std::vector<double>& X,
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
                          double& sse_unsel);
    
    void CalcRegressionSelOrExcl(const SampleStatistics& ss_X,
                                 const SampleStatistics& ss_Y,
                                 const std::vector<double>& X,
                                 const std::vector<double>& Y,
                                 const std::vector<bool>& X_undef,
                                 const std::vector<bool>& Y_undef,
                                 const std::vector<bool>& hl,
                                 bool selected,
                                 SimpleLinearRegression& r,
                                 double& ss_error);
    
	void CalcVarSdFromSumSquares(SampleStatistics& ss, double sum_squares);
	
	/** Attempt to extend the endpoints of a regression line/curve
	 out the nearest Bounding Box boundary using linear interpolation.
	 The input points are assumed to be sorted by X/Y coordinates.
	 The algorithm will return true if there are at least two distict
	 input points.  If it fails, it will return the first and last
	 input points */
    bool ExtendEndpointsToBB(const std::vector<double>& X,
                             const std::vector<double>& Y,
                             double bb_min_x, double bb_min_y,
                             double bb_max_x, double bb_max_y,
                             double& x_first, double& y_first,
                             double& x_last, double& y_last);
    
	struct LowessCacheEntry {
		LowessCacheEntry(size_t n) : sort_map(n), X_srt(n), Y_srt(n), YS_srt(n) {}
		// permutation vector of original data to get
		// sorted order above.  Has size num_obs.  This
		// is useful for quickly generating new LOWESS
		// smoothing on regimes.
		std::vector<size_t> sort_map;
		// original X data, sorted
		std::vector<double> X_srt;
		// original Y data, sorted
		std::vector<double> Y_srt;
		// smoothed Y data, sorted
		std::vector<double> YS_srt;
	};
	typedef std::map<wxString, LowessCacheEntry* > LowessCacheType;
	
	/** Returns a Lowess Cache string key based on x/y variable times. */
	wxString LowessCacheKey(int x_time, int y_time);

	/** Given a Lowess Cache, a cache key string (use LowessCacheKey method
	 to generate), the Lowess parameters and references to orignal data
	 corresponding to current time, find and return existing cached data,
	 or else generate new a new cache entry.  On success, a pointer the
	 the correct cache entry is returned.  Null is returned on failure.
	 */
    LowessCacheEntry* UpdateLowessCacheForTime(LowessCacheType& lowess_cache,
                                               const wxString& key,
                                               Lowess& lowess,
                                               const std::vector<double>& X,
                                               const std::vector<double>& Y,
                                               const std::vector<bool>& XY_undef);
    
    
    
	/** Given LowessCacheEntry, run LOWESS on subests specified by
	 bool vector. Note: it is assumed that the data in lce is sorted
	 by x-coordinates and that the sort_map in lce is needed to map
	 observations to highlight ids. */

    void CalcLowessRegimes(LowessCacheEntry* lce,
                           Lowess& lowess,
                           const std::vector<bool>& hl,
                           std::vector<double>& sel_smthd_srt_x,
                           std::vector<double>& sel_smthd_srt_y,
                           std::vector<double>& unsel_smthd_srt_x,
                           std::vector<double>& unsel_smthd_srt_y,
                           std::vector<bool>& undefs);
    
    
	/** Deletes (frees memory) all allocated cache values and empties cache. */
	void EmptyLowessCache(LowessCacheType& lowess_cache);
}

class ScatterPlotPens {
public:
	ScatterPlotPens();
	wxPen* GetRegPen() { return &reg_pen; }
	wxPen* GetRegSelPen() { return &reg_sel_pen; }
	wxPen* GetRegExlPen() { return &reg_exl_pen; }
	void SetPenColor(wxPen* pen, const wxColour& color);
	void SetPenTrans(wxPen* pen, double trans_val);
private:
	wxPen reg_pen;
	wxPen reg_sel_pen;
	wxPen reg_exl_pen;
	double reg_pens_trans_scalar;
};


#endif
