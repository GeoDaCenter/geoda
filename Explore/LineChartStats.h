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

#ifndef __GEODA_CENTER_LINE_CHART_STATS_H__
#define __GEODA_CENTER_LINE_CHART_STATS_H__

#include <vector>
#include <wx/colour.h>
#include <wx/string.h>

typedef std::vector<size_t> vec_size_t_type;
typedef std::vector<double> vec_dbl_type;
typedef std::vector<vec_dbl_type> vec_vec_dbl_type;

struct LineChartStats {
    LineChartStats(const vec_vec_dbl_type& Y,
                   const std::vector<bool>& Y_undef,
                   const wxString& Yname,
                   const std::vector<bool>& tms_subset0,
                   const std::vector<bool>& tms_subset1,
                   const bool& compare_regimes,
                   const bool& compare_time_periods,
                   const bool& compare_r_and_t);
	
	wxString Yname;
	
	const vec_vec_dbl_type& Y;
    const std::vector<bool>& Y_undef;
	vec_dbl_type Y_avg;
	bool Y_avg_valid;
	double Y_avg_min, Y_avg_max;
	
	vec_dbl_type Y_sel_avg;
	int Y_sel_avg_valid;
	double Y_sel_avg_min, Y_sel_avg_max;
	
	vec_dbl_type Y_excl_avg;
	int Y_excl_avg_valid;
	double Y_excl_avg_min, Y_excl_avg_max;

	vec_dbl_type Y_ss;
	vec_dbl_type Y_sel_ss;
	vec_dbl_type Y_excl_ss;
	
	int obs_sz_i; // size of Y or total number observations
	double obs_sz_d;
	int sel_sz_i; // number of selected observations
	double sel_sz_d;
	int excl_sz_i; // number of excluded observations
	double excl_sz_d;
		
	const std::vector<bool>& tms_subset0;
	const std::vector<bool>& tms_subset1;
	bool time_invariant;
	
	double Y_avg_tm0; // sample mean
	bool Y_avg_tm0_valid;
	
	double Y_avg_tm1; // sample mean
	bool Y_avg_tm1_valid;
	
	
	double Y_sel_tm0_avg; // sample mean
	bool Y_sel_tm0_avg_valid;
	
	double Y_excl_tm0_avg; // sample mean
	bool Y_excl_tm0_avg_valid;
	
	double Y_sel_tm1_avg; // sample mean
	bool Y_sel_tm1_avg_valid;
	
	double Y_excl_tm1_avg; // sample mean
	bool Y_excl_tm1_avg_valid;
	
	vec_size_t_type Y_sel_sz; // sample size of selected
	vec_size_t_type Y_excl_sz; // sample size of excluded
	
	const bool& compare_regimes;
	const bool& compare_time_periods;
	const bool& compare_r_and_t;

	struct SampStats {
		SampStats();
		double mean; // sample mean
		bool mean_v; // is sample mean valid
		double var; // sample variance
		double sd; // sample standard deviation
		double sem; // standard error of the mean
		bool var_v; // is sample variance valid
		int sz_i; // sample size as integer
		double sz_d; // sample size as double
	};
	
	// Summary of the above, depending on compare_regimes, compare_time_periods
	// and compare_r_and_t.
	// When compare_regimes is true:
	//   s0: selected, restricted to Time 1 subset
	//   s1: excluded, restricted to Time 1 subset
	// When compare_time_periods is true:
	//   s0: Time 1 subset
	//   s1: Time 2 subset
	// When compare_r_and_t is true
	//   s0: selected, restricted to Time 1 subset
	//   s1: excluded, restricted to Time 1 subset
	//   s2: selected, restricted to Time 2 subset
	//   s3: excluded, restricted to Time 2 subset
	SampStats s0;
	SampStats s1;
	SampStats s2;
	SampStats s3;
		
	double test_stat; // The test statistic T
	bool test_stat_valid;
	double deg_free; // estimated combined degrees freedom for t-test
	double p_val; // probability that difference in means due to chance
	
	// used only for combined regimes and times comparisons
	// s0 compared to s1
	// s0 compared to s2
	// s0 compared to s3
	// s1 compared to s2
	// s1 compared to s3
	// s2 compared to s3	
	
	std::vector<SampStats*> ss_ptrs;

	// The test statistic T
	std::vector<double> test_stat_c; 
	std::vector<bool> test_stat_valid_c;
	// estimated combined degrees freedom for t-test
	std::vector<double> deg_free_c; 
	// probability that difference in means due to chance
	std::vector<double> p_val_c; 	
	
	/** Update all stats not dependent on regimes or time periods.  This
	 should be called first. */
	void UpdateNonRegimesNonTmsStats();
	
	/** 
     * Update all stats for regimes, but not stats dependent on time
	 * subsets.
     * default_Y_sel_avg_valid: -1 not set 0 set to false 1 set to true
     * default_Y_excl_avg_valid: -1 not set 0 set to false 1 set to true
     */
	void UpdateRegimesStats(const std::vector<bool>& hs,
                            int default_Y_sel_avg_valid = -1,
                            int default_Y_excl_avg_valid = -1);
	
	/** The following will update stats according to compare_regimes or
	 compare_time_periods */
	void UpdateOtherStats();
		
private:
	/** Call when compare_time_periods true.  Assumes
	 UpdateNonRegimesNonTmsStats has been called. */
	void UpdateCompareTmsStats();

	/** Call when compare_regimes true. Assumes both
	 UpdateNonRegimesNonTmsStats and UpdateRegimesStats have been called. */
	void UpdateCompareRegimesStats();
	
	/** Call when compare_r_and_t true. Assumes both
	 UpdateNonRegimesNonTmsStats and UpdateRegimesStats have been called. */
	void UpdateCompareRegAndTmStats();
	
	/** Call after SampStats s0 and s1 are fully updated.  Will update
	 final t-test stats that depend on these statistics */
	void UpdateTtest();
};



class LineChartCanvasCallbackInt
{
public:
	virtual void notifyNewSelection(const std::vector<bool>& tms_sel,
									bool shiftdown, bool pointsel) = 0;
	virtual void notifyNewHoverMsg(const wxString& msg) = 0;
};

#endif
