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

#include <cfloat>
#include <iomanip>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/math/distributions/students_t.hpp>
#include <wx/dc.h>
#include "GeoDaConst.h"
#include "logger.h"
#include "GenUtils.h"

using namespace std;

// Sets all Secondary Attributes in GeoDaVarInfo based on Primary Attributes.
// This method must be called whenever a Primary attribute of any item in the
// GeoDaVarInfo vector changes.
int GeoDa::UpdateVarInfoSecondaryAttribs(std::vector<GeoDaVarInfo>& var_info)
{
	int num_vars = var_info.size();
	int ref_var = -1;
	for (int i=0; i<num_vars; i++) {
		if (ref_var == -1 && var_info[i].sync_with_global_time) ref_var = i;
		var_info[i].is_ref_variable = (i == ref_var);
		// The following parameters are set to default values here
		var_info[i].ref_time_offset = 0;
		var_info[i].time_min = var_info[i].time;
		var_info[i].time_max = var_info[i].time;
		var_info[i].min_over_time = var_info[i].min[var_info[i].time];
		var_info[i].max_over_time = var_info[i].max[var_info[i].time];
	}
	
	if (ref_var == -1) return ref_var;
	int ref_time = var_info[ref_var].time;
	int min_time = ref_time;
	int max_time = ref_time;
	for (int i=0; i<num_vars; i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].ref_time_offset = var_info[i].time - ref_time;
			if (var_info[i].time < min_time) min_time = var_info[i].time;
			if (var_info[i].time > max_time) max_time = var_info[i].time;
		}
	}
	int global_max_time = var_info[ref_var].max.size()-1;
	int min_ref_time = ref_time - min_time;
	int max_ref_time = global_max_time - (max_time - ref_time);
	for (int i=0; i<num_vars; i++) {
		if (var_info[i].sync_with_global_time) {
			var_info[i].time_min = min_ref_time + var_info[i].ref_time_offset;
			var_info[i].time_max = max_ref_time + var_info[i].ref_time_offset;
			for (int t=var_info[i].time_min; t<=var_info[i].time_max; t++) {
				if (var_info[i].min[t] < var_info[i].min_over_time) {
					var_info[i].min_over_time = var_info[i].min[t];
				}
				if (var_info[i].max[t] > var_info[i].max_over_time) {
					var_info[i].max_over_time = var_info[i].max[t];
				}
			}
		}
	}
	return ref_var;
}

void GeoDa::PrintVarInfoVector(std::vector<GeoDaVarInfo>& var_info)
{
	LOG_MSG("Entering GeoDa::PrintVarInfoVector");
	LOG(var_info.size());
	for (int i=0; i<var_info.size(); i++) {
		LOG_MSG("Primary Attributes:");
		LOG(var_info[i].name);
		LOG(var_info[i].is_time_variant);
		LOG(var_info[i].time);
		for (int t=0; t<var_info[i].min.size(); t++) {
			LOG(var_info[i].min[t]);
			LOG(var_info[i].max[t]);
		}
		LOG(var_info[i].sync_with_global_time);
		LOG(var_info[i].fixed_scale);
		
		LOG_MSG("Secondary Attributes:");
		LOG(var_info[i].is_ref_variable);
		LOG(var_info[i].ref_time_offset);
		LOG(var_info[i].time_min);
		LOG(var_info[i].time_max);
		LOG(var_info[i].min_over_time);
		LOG(var_info[i].max_over_time);
		LOG_MSG("\n");
	}
	LOG_MSG("Exiting GeoDa::PrintVarInfoVector");
}

/** Use with std::sort for sorting in ascending order */
bool GeoDa::dbl_int_pair_cmp_less(const dbl_int_pair_type& ind1,
								  const dbl_int_pair_type& ind2)
{
	return ind1.first < ind2.first;
}

/** Use with std::sort for sorting in descending order */
bool GeoDa::dbl_int_pair_cmp_greater(const dbl_int_pair_type& ind1,
									 const dbl_int_pair_type& ind2)
{
	return ind1.first > ind2.first;
}

/** Use with std::sort for sorting in ascending order */
bool GeoDa::dbl_int_pair_cmp_second_less(const dbl_int_pair_type& ind1,
										 const dbl_int_pair_type& ind2)
{
	return ind1.second < ind2.second;
}

/** Use with std::sort for sorting in descending order */
bool GeoDa::dbl_int_pair_cmp_second_greater(const dbl_int_pair_type& ind1,
											const dbl_int_pair_type& ind2)
{
	return ind1.second > ind2.second;
}


void HingeStats::CalculateHingeStats(
							const std::vector<GeoDa::dbl_int_pair_type>& data)
{
	num_obs = data.size();
	double N = num_obs;
	is_even_num_obs = (num_obs % 2) == 0;
	min_val = data[0].first;
	max_val = data[num_obs-1].first;
	Q2_ind = (N+1)/2.0 - 1;
	if (is_even_num_obs) {
		Q1_ind = (N+2)/4.0 - 1;
		Q3_ind = (3*N+2)/4.0 - 1;
	} else {
		Q1_ind = (N+3)/4.0 - 1;
		Q3_ind = (3*N+1)/4.0 - 1;
	}
	Q1 = (data[(int) floor(Q1_ind)].first +
		  data[(int) ceil(Q1_ind)].first)/2.0;
	Q2 = (data[(int) floor(Q2_ind)].first +
		  data[(int) ceil(Q2_ind)].first)/2.0;
	Q3 = (data[(int) floor(Q3_ind)].first +
		  data[(int) ceil(Q3_ind)].first)/2.0;
	IQR = Q3 - Q1;
	extreme_lower_val_15 = Q1 - 1.5*IQR;
	extreme_lower_val_30 = Q1 - 3.0*IQR;
	extreme_upper_val_15 = Q3 + 1.5*IQR;
	extreme_upper_val_30 = Q3 + 3.0*IQR;
	min_IQR_ind = -1;
	for (int i=0; i<num_obs; i++) {
		if (data[i].first < Q1) min_IQR_ind = i;
		else break;
	}
	if (min_IQR_ind < num_obs-1) min_IQR_ind++;
	max_IQR_ind = num_obs;
	for (int i=num_obs-1; i>=0; i--) {
		if (data[i].first > Q3) max_IQR_ind = i;
		else break;
	}
	if (max_IQR_ind > 0) max_IQR_ind--;
}

// Assume input v is sorted.  If not, can sort
// with std::sort(v.begin(), v.end())
// Testing: for v = {15, 20, 35, 40, 50},
// percentile(1, v) = 15, percentile(10, v) = 15, percentile(11) = 15.25
// percentile(50, v) = 35, percentile(89, v) = 49.5,
// percentile(90, v) = 50, percentile(99, v) = 50
double GeoDa::percentile(double x, const std::vector<double>& v)
{
	int N = v.size();
	double Nd = (double) N;
	double p_0 = (100.0/Nd) * (1.0-0.5);
	double p_Nm1 = (100.0/Nd) * (Nd-0.5);
	if (x <= p_0) return v[0];
	if (x >= p_Nm1) return v[N-1];
	
	for (int i=1; i<N; i++) {
		double p_i = (100.0/Nd) * ((((double) i)+1.0)-0.5);
		if (x == p_i) return v[i];
		if (x < p_i) {
			double p_im1 = (100.0/Nd) * ((((double) i))-0.5);
			return v[i-1] + Nd*((x-p_im1)/100.0)*(v[i]-v[i-1]);
		}
	}
	return v[N-1]; // execution should never get here
}

// Same assumptions as above
double GeoDa::percentile(double x, const GeoDa::dbl_int_pair_vec_type& v)
{
	int N = v.size();
	double Nd = (double) N;
	double p_0 = (100.0/Nd) * (1.0-0.5);
	double p_Nm1 = (100.0/Nd) * (Nd-0.5);
	if (x <= p_0) return v[0].first;
	if (x >= p_Nm1) return v[N-1].first;
	
	for (int i=1; i<N; i++) {
		double p_i = (100.0/Nd) * ((((double) i)+1.0)-0.5);
		if (x == p_i) return v[i].first;
		if (x < p_i) {
			double p_im1 = (100.0/Nd) * ((((double) i))-0.5);
			return v[i-1].first + Nd*((x-p_im1)/100.0)*(v[i].first
														-v[i-1].first);
		}
	}
	return v[N-1].first; // execution should never get here
}


SampleStatistics::SampleStatistics(const std::vector<double>& data)
	: sample_size(0), min(0), max(0), mean(0),
	var_with_bessel(0), var_without_bessel(0),
	sd_with_bessel(0), sd_without_bessel(0)
{
	CalculateFromSample(data);
}

void SampleStatistics::CalculateFromSample(const std::vector<double>& data)
{
	sample_size = data.size();
	if (sample_size == 0) return;

	CalcMinMax(data, min, max);
	mean = CalcMean(data);
	
	double n = sample_size;
	double sum_squares = 0;
	for (int i=0, iend = data.size(); i<iend; i++) {
		sum_squares += data[i] * data[i];
	}
	
	var_without_bessel = (sum_squares/n) - (mean*mean);
	sd_without_bessel = sqrt(var_without_bessel);
	
	if (sample_size == 1) {
		var_with_bessel = var_without_bessel;
		sd_with_bessel = sd_without_bessel;
	} else {
		var_with_bessel = (n/(n-1)) * var_without_bessel;
		sd_with_bessel = sqrt(var_with_bessel);
	}
}

/** We assume that the data has been sorted in ascending order */
void SampleStatistics::CalculateFromSample(
							const std::vector<GeoDa::dbl_int_pair_type>& data)
{
	sample_size = data.size();
	if (sample_size == 0) return;
	
	min = data[0].first;
	max = data[sample_size-1].first;
	mean = CalcMean(data);
	
	double n = sample_size;
	double sum_squares = 0;
	for (int i=0, iend = data.size(); i<iend; i++) {
		sum_squares += data[i].first * data[i].first;
	}
	
	var_without_bessel = (sum_squares/n) - (mean*mean);
	sd_without_bessel = sqrt(var_without_bessel);
	
	if (sample_size == 1) {
		var_with_bessel = var_without_bessel;
		sd_with_bessel = sd_without_bessel;
	} else {
		var_with_bessel = (n/(n-1)) * var_without_bessel;
		sd_with_bessel = sqrt(var_with_bessel);
	}
}

string SampleStatistics::ToString()
{
	ostringstream ss;
	ss << "sample_size = " << sample_size << endl;
	ss << "min = " << min << endl;
	ss << "max = " << max << endl;
	ss << "mean = " << mean << endl;
	ss << "var_with_bessel = " << var_with_bessel << endl;
	ss << "var_without_bessel = " << var_without_bessel << endl;
	ss << "sd_with_bessel = " << sd_with_bessel << endl;
	ss << "sd_without_bessel = " << sd_without_bessel << endl;
	return ss.str();
}

double SampleStatistics::CalcMin(const std::vector<double>& data)
{
	double min = std::numeric_limits<double>::max();
	for (int i=0, iend=data.size(); i<iend; i++) {
		if ( data[i] < min ) min = data[i];
	}
	return min;
}

double SampleStatistics::CalcMax(const std::vector<double>& data)
{
	double max = -std::numeric_limits<double>::max();
	for (int i=0, iend=data.size(); i<iend; i++) {
		if ( data[i] > max ) max = data[i];
	}
	return max;
}

void SampleStatistics::CalcMinMax(const std::vector<double>& data,
								  double& min, double& max)
{
	if (data.size() == 0) return;
	min = data[0];
	max = data[0];
	for (int i=1, iend=data.size(); i<iend; i++) {
		if ( data[i] < min ) {
			min = data[i];
		} else if ( data[i] > max ) {
			max = data[i];
		}
	}
}


double SampleStatistics::CalcMean(const std::vector<double>& data)
{
	if (data.size() == 0) return 0;
	double total = 0;
	for (int i=0, iend=data.size(); i<iend; i++) {
		total += data[i];
	}
	return total / (double) data.size();
}

double SampleStatistics::CalcMean(
							const std::vector<GeoDa::dbl_int_pair_type>& data)
{
	if (data.size() == 0) return 0;
	double total = 0;
	for (int i=0, iend=data.size(); i<iend; i++) {
		total += data[i].first;
	}
	return total / (double) data.size();
}

SimpleLinearRegression::SimpleLinearRegression(const std::vector<double>& X,
											   const std::vector<double>& Y,
											   double meanX, double meanY,
											   double varX, double varY)
	: covariance(0), correlation(0), alpha(0), beta(0), r_squared(0),
	std_err_of_estimate(0), std_err_of_beta(0), std_err_of_alpha(0),
	t_score_alpha(0), t_score_beta(0), p_value_alpha(0), p_value_beta(0),
	valid(false), valid_correlation(false), valid_std_err(false),
	error_sum_squares(0)
{
	CalculateRegression(X, Y, meanX, meanY, varX, varY);
}

void SimpleLinearRegression::CalculateRegression(const std::vector<double>& X,
												 const std::vector<double>& Y,
												 double meanX, double meanY,
												 double varX, double varY)
{
	LOG_MSG("Entering SimpleLinearRegression::CalculateRegression");
	LOG(meanX);
	LOG(meanY);
	LOG(varX);
	LOG(varY);
	if (X.size() != Y.size() || X.size() < 2 ) return;
	double expectXY = 0;
	for (int i=0, iend=X.size(); i<iend; i++) {
		expectXY += X[i]*Y[i];
	}
	expectXY /= (double) X.size();
	covariance = expectXY - meanX * meanY;
	LOG(expectXY);
	LOG(covariance);
	if (varX > 4*DBL_MIN) {
		beta = covariance / varX;
		alpha = meanY - beta * meanX;
		valid = true;
		LOG(alpha);
		LOG(beta);
	}
	double SS_tot = varY*Y.size();
	error_sum_squares = 0; // error_sum_squares = SS_err
	double err=0;
	for (int i=0, iend=Y.size(); i<iend; i++) {
		err = Y[i] - (alpha + beta * X[i]);
		error_sum_squares += err * err;
	}
	LOG(error_sum_squares);
	if (error_sum_squares < 16*DBL_MIN) {
		r_squared = 1;
	} else {
		r_squared = 1 - error_sum_squares / SS_tot;
	}
	LOG(r_squared);
	
	if (Y.size()>2 && varX > 4*DBL_MIN) {
		// error_sum_squares/(n-k-1), k=1
		std_err_of_estimate = error_sum_squares/(Y.size()-2); 
		std_err_of_estimate = sqrt(std_err_of_estimate);
		std_err_of_beta = std_err_of_estimate/sqrt(X.size()*varX);
		double sum_x_squared = 0;
		for (int i=0, iend=X.size(); i<iend; i++) {
			sum_x_squared += X[i] * X[i];
		}
		std_err_of_alpha = std_err_of_beta * sqrt(sum_x_squared / X.size());
		
		if (std_err_of_alpha >= 16*DBL_MIN) {
			t_score_alpha = alpha / std_err_of_alpha;
		} else {
			t_score_alpha = 100;
		}
		if (std_err_of_beta >= 16*DBL_MIN) {
			t_score_beta = beta	/ std_err_of_beta;
		} else {
			t_score_beta = 100;
		}
		p_value_alpha = TScoreTo2SidedPValue(t_score_alpha, X.size()-2);
		p_value_beta = TScoreTo2SidedPValue(t_score_beta, X.size()-2);
		
		LOG(std_err_of_estimate);
		LOG(std_err_of_beta);
		LOG(std_err_of_alpha);
		LOG(t_score_alpha);
		LOG(p_value_alpha);
		LOG(t_score_beta);
		LOG(p_value_beta);
		valid_std_err = true;
	}
	
	double d = sqrt(varX)*sqrt(varY);
	if (d > 4*DBL_MIN) {
		correlation = covariance / d;
		valid_correlation = true;
		LOG(correlation);
	}
	LOG_MSG("Exiting SimpleLinearRegression::CalculateRegression");
}

double SimpleLinearRegression::TScoreTo2SidedPValue(double tscore, int df)
{
	using namespace boost::math;
	students_t dist(df);
	// Cumulative Distribution Function evaluated at tscore
	if ( tscore >= 0) {
		return 2*(1.0-cdf(dist, tscore));
	} else {
		return 2*cdf(dist,tscore);
	}

}

string SimpleLinearRegression::ToString()
{
	ostringstream ss;
	ss << "covariance = " << covariance << endl;
	ss << "correlation = " << correlation << endl;
	ss << "alpha = " << alpha << endl;
	ss << "beta = " << beta << endl;
	ss << "r_squared = " << r_squared << endl;
	ss << "valid = " << (valid ? "true" : "false") << endl;
	ss << "valid_correlation = " << (valid_correlation ? "true" : "false")
		<< endl;
	ss << "error_sum_squares = " << error_sum_squares << endl;
	return ss.str();
}

AxisScale::AxisScale(double data_min_s, double data_max_s, int ticks_s)
: data_min(0), data_max(0), scale_min(0), scale_max(0),
scale_range(0), tic_inc(0), p(0), ticks(ticks_s)
{
	CalculateScale(data_min_s, data_max_s, ticks_s);
}

AxisScale::AxisScale(const AxisScale& s)
: data_min(s.data_min), data_max(s.data_max),
	scale_min(s.scale_min), scale_max(s.scale_max),
	scale_range(s.scale_range), tic_inc(s.tic_inc), p(s.p),
	tics(s.tics), tics_str(s.tics_str), tics_str_show(s.tics_str_show),
	ticks(s.ticks)
{
}

AxisScale& AxisScale::operator=(const AxisScale& s)
{
	data_min = s.data_min;
	data_max = s.data_max;
	scale_min = s.scale_min;
	scale_max = s.scale_max;
	scale_range = s.scale_range;
	tic_inc = s.tic_inc;
	p = s.p;
	tics = s.tics;
	tics_str = s.tics_str;
	tics_str_show = s.tics_str_show;
	ticks = s.ticks;
	return *this;
}

void AxisScale::CalculateScale(double data_min_s, double data_max_s,
							   const int ticks)
{
	if (data_min_s <= data_max_s) {
		data_min = data_min_s;
		data_max = data_max_s;
	} else {
		data_min = data_max_s;
		data_max = data_min_s;	
	}
	
	double data_range = data_max - data_min;
	if ( data_range <= 2*DBL_MIN ) {
		scale_max = ceil((data_max + 0.05)*10)/10;
		scale_min = floor((data_min - 0.05)*10)/10;
		scale_range = scale_max - scale_min;
		p = 1;
		tic_inc = scale_range/2;
		tics.resize(3);
		tics_str.resize(3);
		tics[0] = scale_min;
		tics[1] = scale_min + tic_inc;
		tics[2] = scale_max;
	} else {
		p = floor(log10(data_range))-1;
		scale_max = ceil(data_max / pow((double)10,p)) * pow((double)10,p);
		scale_min = floor(data_min / pow((double)10,p)) * pow((double)10,p);
		scale_range = scale_max - scale_min;
		tic_inc = floor((scale_range / pow((double)10,p))/ticks)
			* pow((double)10,p);
		if (scale_min + tic_inc*(ticks+1) <= scale_max + 2*DBL_MIN) {
			tics.resize(ticks+2);
			tics_str.resize(ticks+2);
		} else {
			tics.resize(ticks+1);
			tics_str.resize(ticks+1);
		}
		for (int i=0, iend=tics.size(); i<iend; i++) {
			tics[i] = scale_min + i*tic_inc;
		}
	}
	tics_str_show.resize(tics_str.size());
	for (int i=0, iend=tics.size(); i<iend; i++) {
		ostringstream ss;
		ss << tics[i];
		tics_str[i] = ss.str();
		tics_str_show[i] = true;
	}
}

/** only display every other tic value */
void AxisScale::SkipEvenTics()
{
	for (int i=0; i<tics_str_show.size(); i++) tics_str_show[i] = (i%2 == 0);
}

void AxisScale::ShowAllTics()
{
	for (int i=0; i<tics_str_show.size(); i++) tics_str_show[i] = true;
}

string AxisScale::ToString()
{
	ostringstream ss;
	ss << "data_min = " << data_min << endl;
	ss << "data_max = " << data_max << endl;
	ss << "scale_min = " << scale_min << endl;
	ss << "scale_max = " << scale_max << endl;
	ss << "scale_range = " << scale_range << endl;
	ss << "p = " << p << endl;
	ss << "tic_inc = " << tic_inc << endl;
	for (int i=0, iend=tics.size(); i<iend; i++) {
		ss << "tics[" << i << "] = " << tics[i];
		ss << ",  tics_str[" << i << "] = " << tics_str[i] << endl;
	}
	ss << "Exiting AxisScale::CalculateScale" << endl;
	return ss.str();
}


/** convert input rectangle corners s1 and s2 into screen-coordinate corners */
void GenUtils::StandardizeRect(const wxPoint& s1, const wxPoint& s2,
							   wxPoint& lower_left, wxPoint& upper_right)
{
	lower_left = s1;
	upper_right = s2;
	if (lower_left.x > upper_right.x) {
		GenUtils::swap<int>(lower_left.x, upper_right.x);
	}
	if (lower_left.y < upper_right.y) {
		GenUtils::swap<int>(lower_left.y, upper_right.y);
	}
}

/** assumes input corners are all screen-coordinate correct for
 lower left and upper right corners */
bool GenUtils::RectsIntersect(const wxPoint& r1_lower_left,
							  const wxPoint& r1_upper_right,
							  const wxPoint& r2_lower_left,
							  const wxPoint& r2_upper_right)
{
	// return negation of all situations where rectangles
	// do not intersect.
	return (!((r1_lower_left.x > r2_upper_right.x) ||
			  (r1_upper_right.x < r2_lower_left.x) ||
			  (r1_lower_left.y < r2_upper_right.y) ||
			  (r1_upper_right.y > r2_lower_left.y)));
}

bool GenUtils::CounterClockwise(const wxPoint& p1, const wxPoint& p2,
								const wxPoint& p3)
{
	return ((p2.y-p1.y)*(p3.x-p2.x) < (p3.y-p2.y)*(p2.x-p1.x));
}

bool GenUtils::LineSegsIntersect(const wxPoint& l1_p1, const wxPoint& l1_p2,
								 const wxPoint& l2_p1, const wxPoint& l2_p2)
{
	return ((CounterClockwise(l2_p1, l2_p2, l1_p1) !=
			 CounterClockwise(l2_p1, l2_p2, l1_p2)) &&
			(CounterClockwise(l1_p1, l1_p2, l2_p1) !=
			 CounterClockwise(l1_p1, l1_p2, l2_p2)));
}

/** If input string has length < width, then prepends (or appends
 if pad_left=false) string with spaces so that total length is now width.
 If string length >= width, then returns original input string. */
wxString GenUtils::Pad(const wxString& s, int width, bool pad_left)
{
	if (s.length() >= width) return s;
	int pad_len = width - s.length();
	wxString output;
	if (!pad_left) output << s;
	for (int i=0; i<pad_len; i++) output << " ";
	if (pad_left) output << s;
	return output;
}

wxString GenUtils::DblToStr(double x, int precision)
{
	std::stringstream ss;
	ss << std::setprecision(precision);
	ss << x;
	return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString GenUtils::PtToStr(const wxPoint& p)
{
	std::stringstream ss;
	ss << "(" << p.x << "," << p.y << ")";
	return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString GenUtils::PtToStr(const wxRealPoint& p)
{
	std::stringstream ss;
	ss << std::setprecision(5);
	ss << "(" << p.x << "," << p.y << ")";
	return wxString(ss.str().c_str(), wxConvUTF8);
}

// NOTE: should take into account undefined values.
void GenUtils::DeviationFromMean(int nObs, double* data)
{
	if (nObs == 0) return;
	double sum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) sum += data[i];
	const double mean = sum / (double) nObs;
	for (int i=0, iend=nObs; i<iend; i++) data[i] -= mean;
}

void GenUtils::DeviationFromMean(std::vector<double>& data)
{
	LOG_MSG("Entering GenUtils::DeviationFromMean");
	if (data.size() == 0) return;
	double sum = 0.0;
	for (int i=0, iend=data.size(); i<iend; i++) sum += data[i];
	const double mean = sum / (double) data.size();
	LOG(mean);
	for (int i=0, iend=data.size(); i<iend; i++) data[i] -= mean;
	LOG_MSG("Exiting GenUtils::DeviationFromMean");
}

bool GenUtils::StandardizeData(int nObs, double* data)
{
	if (nObs <= 1) return false;
	GenUtils::DeviationFromMean(nObs, data);
	double ssum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (nObs-1.0));
	LOG(sd);
	if (sd == 0) return false;
	for (int i=0, iend=nObs; i<iend; i++) data[i] /= sd;
	return true;
}

bool GenUtils::StandardizeData(std::vector<double>& data)
{
	LOG_MSG("Entering GenUtils::StandardizeData");
	if (data.size() <= 1) return false;
	GenUtils::DeviationFromMean(data);
	double ssum = 0.0;
	for (int i=0, iend=data.size(); i<iend; i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (data.size()-1.0));
	LOG(sd);
	if (sd == 0) return false;
	for (int i=0, iend=data.size(); i<iend; i++) data[i] /= sd;
	LOG_MSG("Exiting GenUtils::StandardizeData");
	return true;
}

wxString GenUtils::swapExtension(const wxString& fname, const wxString& ext)
{
	if (ext.IsEmpty()) return fname;
	wxString prefix = fname.BeforeLast('.');
	if (prefix.IsEmpty()) return fname + "." + ext;
	return prefix + "." + ext;
}

wxString GenUtils::GetFileDirectory(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Left(pos);
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Left(pos);
	return wxEmptyString;
}

wxString GenUtils::GetFileName(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::GetFileExt(const wxString& path)
{
	int pos = path.Find('.',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::GetTheFileTitle(const wxString& path)
{
	wxString strResult = GetFileName(path);
	int pos = strResult.Find('.',true);
	if (pos >= 0)
		return strResult.Left(pos);
	return strResult;
}

/*
 Reverse
 Changes the order of bytes in the presentation of a 4 byte number.
  */
wxInt32 GenUtils::Reverse(const wxInt32 &val)
{
	union {
		wxInt32 v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

long GenUtils::ReverseInt(const int &val)
{
	union {
		int v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

void GenUtils::SkipTillNumber(std::istream &s)
{
	char ch;
	while (s >> ch) {
		if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.')
			break;
	}
	if (s.good())
		s.putback(ch);
}

// This is an implementation of ltoa
void GenUtils::longToString(const long d, char* Id, const int base) 
{
	int i = 0;
	long j = d;
	char rId[ GeoDaConst::ShpObjIdLen ];
	if (d == 0) {
		Id[0] = '0';
		Id[1] = '\0';
		return;
	}
	if (d < 0) j = -d;
	while (j != 0) {
		rId[i] = (j % base) + '0';
		j = j / base;
		i++;
	}
	j = i;
	if (d < 0) {
		Id[0] = '-';
		Id[i + 1] = '\0';
		while (i > 0) {
			Id[i] = rId[j - i];
			i--;
		}
		return;
	}
	
	Id[i] = '\0';
	while (i > 0) {
		Id[i - 1] = rId[j - i];
		i--;
	}
	return;
}

// Calculates Euclidean distance
double GenUtils::distance(const wxRealPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxRealPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

// Calculates Euclidean distance
double GenUtils::distance_sqrd(const wxRealPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxRealPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

// calculates distance from point p0 to an infinite line passing through
// points p1 and p2
double GenUtils::pointToLineDist(const wxPoint& p0, const wxPoint& p1,
								 const wxPoint& p2)
{
	double d_p1p2 = distance(p1, p2);
	if (d_p1p2 <= 16*DBL_MIN) return distance(p0, p1);
	return abs((p2.x-p1.x)*(p1.y-p0.y)-(p1.x-p0.x)*(p2.y-p1.y))/d_p1p2;
}

void GenUtils::strToInt64(const wxString& str, wxInt64 *val)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	strToInt64(buf, val);
}

// Convert an ASCII string into a wxInt64 (or long long)
void GenUtils::strToInt64(const char *str, wxInt64 *val)
{
	wxInt64 total = 0;
	bool minus = 0;
 
	while (isspace(*str)) str++;
	if (*str == '+') {
		str++;
	} else if (*str == '-') {
		minus = true;
		str++;
	}
	while (isdigit(*str)) {
		total *= 10;
		total += (*str++ - '0');
	}
	*val = minus ? -total : total;
}

bool GenUtils::validInt(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return validInt(buf);
}

// Checks that an ASCII string can be parsed to a valid integer.  At least
// one digit must been found.
bool GenUtils::validInt(const char* str)
{
	//LOG_MSG(wxString::Format("GenUtils::validInt(\"%s\"):", str));
	while (isspace(*str)) str++;
	if (*str == '+' || *str == '-') str++;
	const char* t = str;
	while (isdigit(*str)) str++;
	if (t == str) {
		// no digit found so return false
		//LOG_MSG("   no digit found");
		return false;
	}
	while (isspace(*str)) str++;
	// only return true if we are finally pointing at
	// the null terminating character.
	//LOG_MSG(wxString::Format("   final char is null: %d", *str == '\0'));
	return *str == '\0';
}

bool GenUtils::isEmptyOrSpaces(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return isEmptyOrSpaces(buf);
}

// returns true if the string is either empty
// or has only space characters
bool GenUtils::isEmptyOrSpaces(const char *str)
{
	while (isspace(*str)) str++;
	// if the first not-space char is not the end of the string,
	// return false.
	return *str == '\0';
}

bool GenUtils::ExistsShpShxDbf(const wxFileName& fname, bool* shp_found,
							   bool* shx_found, bool* dbf_found)
{
	wxFileName shp(fname);
	shp.SetExt("shp");
	wxFileName shx(fname);
	shx.SetExt("shx");
	wxFileName dbf(fname);
	dbf.SetExt("dbf");
	if (shp_found) *shp_found = shp.FileExists();
	if (shx_found) *shx_found = shx.FileExists();
	if (dbf_found) *dbf_found = dbf.FileExists();
	return shp.FileExists() && shx.FileExists() && dbf.FileExists();
}

