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

#ifndef __GEODA_CENTER_GEN_UTILS_H__
#define __GEODA_CENTER_GEN_UTILS_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <wx/colour.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/string.h>
#include <wx/gdicmn.h> // for wxPoint / wxRealPoint
#include <wx/textwrapper.h>

// file name encodings
// in windows, wxString.fn_str() will return a wchar*, which take care of 
// international encodings
// in mac, wxString.mb_str() will return UTF8 char*
#ifdef __WIN32__
 #ifndef GET_ENCODED_FILENAME
  #define GET_ENCODED_FILENAME(a) a.ToUTF8().data()
 #endif
#else
 #ifndef GET_ENCODED_FILENAME
  #define GET_ENCODED_FILENAME(a) a.ToUTF8().data() 
 #endif
#endif

using namespace std;

class wxDC;
class TableState;

namespace StringUtils {
    int utf8_strlen(const string& str);
}

namespace DbfFileUtils {
    void SuggestDoubleParams(int length, int decimals, int* suggest_len, int* suggest_dec);
    double GetMaxDouble(int length, int decimals,int* suggest_len=0, int* suggest_dec=0);
    wxString GetMaxDoubleString(int length, int decimals);
    double GetMinDouble(int length, int decimals, int* suggest_len=0, int* suggest_dec=0);
    wxString GetMinDoubleString(int length, int decimals);
    wxInt64 GetMaxInt(int length);
    wxString GetMaxIntString(int length);
    wxInt64 GetMinInt(int length);
    wxString GetMinIntString(int length);
}

namespace GdaColorUtils {
	/** Returns colour in 6-hex-digit HTML format.
	 Eg wxColour(255,0,0) -> "#FF0000" */
	wxString ToHexColorStr(const wxColour& c);
	/** change brightness of input_color and leave result in output color
	 brightness = 75 by default, will slightly darken the input color.
	 brightness = 0 is black, brightness = 200 is white. */
	wxColour ChangeBrightness(const wxColour& input_col, int brightness = 75);
    
    void GetUnique20Colors(vector<wxColour>& colors);
    
    void GetLISAColors(vector<wxColour>& colors);
    void GetLISAColorLabels(vector<wxString>& labels);
    
    void GetLocalGColors(vector<wxColour>& colors);
    void GetLocalGColorLabels(vector<wxString>& labels);
    
    void GetLocalJoinCountColors(vector<wxColour>& colors);
    void GetLocalJoinCountColorLabels(vector<wxString>& labels);
    
    void GetLocalGearyColors(vector<wxColour>& colors);
    void GetLocalGearyColorLabels(vector<wxString>& labels);
    
    void GetMultiLocalGearyColors(vector<wxColour>& colors);
    void GetMultiLocalGearyColorLabels(vector<wxString>& labels);
    
    void GetPercentileColors(vector<wxColour>& colors);
    void GetPercentileColorLabels(vector<wxString>& labels);
    
    void GetBoxmapColors(vector<wxColour>& colors);
    void GetBoxmapColorLabels(vector<wxString>& labels);
    
    void GetStddevColors(vector<wxColour>& colors);
    void GetStddevColorLabels(vector<wxString>& labels);
    
    void GetQuantile2Colors(vector<wxColour>& colors);
    void GetQuantile3Colors(vector<wxColour>& colors);
    void GetQuantile4Colors(vector<wxColour>& colors);
    void GetQuantile5Colors(vector<wxColour>& colors);
    void GetQuantile6Colors(vector<wxColour>& colors);
    void GetQuantile7Colors(vector<wxColour>& colors);
    void GetQuantile8Colors(vector<wxColour>& colors);
    void GetQuantile9Colors(vector<wxColour>& colors);
    void GetQuantile10Colors(vector<wxColour>& colors);
}

namespace Gda {
	/** Returns a uniformly distributed
	 random unsigned 64-bit integer given a seed.  Has the property
	 that seed, seed+1, seed+2, .... seed+n are good random numbers. This
	 is useful for doing parallel Monte Carlo simulations with a common
	 random seed for reproducibility. */
	uint64_t ThomasWangHashUInt64(uint64_t key);
	
	/** Returns a uniformly distributed
	 random double on the unit interval given unsigned 64-bit integer as
	 a seed.  Has the property that seed, seed+1, seed+2, .... seed+n are
	 good random numbers. This is useful for doing parallel Monte Carlo
	 simulations with a common random seed for reproducibility. */
	double ThomasWangHashDouble(uint64_t key);
    
	double ThomasWangDouble(uint64_t& key);
	
	inline bool IsNaN(double x) { return x != x; }
	inline bool IsFinite(double x) { return x-x == 0; }
    
    uint64_t factorial(unsigned int n);
    
    double combinatorial(unsigned int n, unsigned int r);
    
    wxString CreateUUID(int nSize);
    
    wxString DetectDateFormat(wxString s, vector<wxString>& date_items);
    
    unsigned long long DateToNumber(wxString s_date, wxRegEx& regex, vector<wxString>& date_items);
    
	// useful for sorting a vector of double with their original indexes:
	// vector<dbl_int_pair_type> data;
	// sort(data.begin(), data.end(), Gda::dbl_int_pair_cmp_less);	
	typedef pair<double, int> dbl_int_pair_type;
	typedef vector<dbl_int_pair_type> dbl_int_pair_vec_type;
	bool dbl_int_pair_cmp_less(const dbl_int_pair_type& ind1,
							   const dbl_int_pair_type& ind2);
	bool dbl_int_pair_cmp_greater(const dbl_int_pair_type& ind1,
								  const dbl_int_pair_type& ind2);
	bool dbl_int_pair_cmp_second_less(const dbl_int_pair_type& ind1,
									  const dbl_int_pair_type& ind2);
	bool dbl_int_pair_cmp_second_greater(const dbl_int_pair_type& ind1,
										 const dbl_int_pair_type& ind2);
    typedef pair<wxString, int> str_int_pair_type;
    typedef vector<str_int_pair_type> str_int_pair_vec_type;
    
    // Percentile using Linear interpolation between closest ranks
    // Definition as described in Matlab documentation
    // and at http://en.wikipedia.org/wiki/Percentile
    // Assumes that input vector v is sorted in ascending order.
    // Duplicate values are allowed.
    double percentile(double x, const vector<double>& v);
    double percentile(double x, const Gda::dbl_int_pair_vec_type& v);
    double percentile(double x, const Gda::dbl_int_pair_vec_type& v,
                      const vector<bool>& undefs);
}

// Note: In "Exploratory Data Analysis", pp 32-34, 1977, Tukey only defines
// hinge values when the number of values N = 4n+5 where n=0,1,2,...
// When the N values are in increasing order a1, a2, ... aN,
// the lower hinge value, median and upper hinge values are defined
// H1 = a((N+3)/4), M = a((N+1)/2) and H2 = a(3N+1)/4
// When N is of the form 4n+5, the hinges H1 and H2 are identical to
// the quartiles Q1 and Q3. Tukey gave no general definition for H1 and H2,
// and so people define H1 and H2 by Q1 and Q3.  Since quartiles values
// also have no universally agreed upon general definition, we
// follow the definition given by Tukey, Mosteller and Hoaglin in
// "Understanding Robust and Exploratory Data Analysis" 1983.
// For a sample size N,  Tukey et al define:
// Q1 = the value at (N+3)/4 when N odd and (N+2)/4 when N even
// Q3 = the value at (3N+1)/4 when N odd and (3N+2)/4 when N even
// Q2 = the value at (N+1)/2 (the usual median)
// Note: In the above Tukey assumes the values are enumerated 1, 2, 3,...,
// but we enumerate as 0, 1, 2,... and therefore we subtract 1 from each
// of the above. Just as for medians when N is even, when indecies 
// defined above are of the form k + 1/2, we take the average of the values
// at k and k+1.

// Box Map categories:
// < extreme_lower_val (hinge = 1.5 or 3.0)
// >= extreme_lower_val and < Q1
// >= Q1 and <= median
// > median and <= Q3
// <= extreme_upper_val and > Q3
// > extreme_upper_val (hinge = 1.5 or 3.0)

// Box Plot extents:
// upper and lower whiskers correspond to extreme_lower and upper values
// IQR corresponds to all values between and including Q1 and Q3
// median is draw as a line through IQR
// classification is same as Box Map, except for middle two categories
// for Box Map are merged to form IQR

// When the number of observations is odd, we put the median observation
// in the lower of the two IQR categories.

struct HingeStats {
	HingeStats() : num_obs(0), min_val(0),
		max_val(0), is_even_num_obs(false),
		Q2(0), Q2_ind(0), Q1(0), Q1_ind(0),
		Q3(0), Q3_ind(0), min_IQR_ind(0), max_IQR_ind(0) {}
	void CalculateHingeStats(const vector<Gda::dbl_int_pair_type>& data);
	void CalculateHingeStats(const vector<Gda::dbl_int_pair_type>& data,
                             const vector<bool>& data_undef);
	int num_obs;
	double min_val;
	double max_val;
	bool is_even_num_obs; // will determine if one or two medians
	// Note: for each of Q1_ind, Q2_ind and Q3_ind, the index number
	// is either a whole number, or whole number + 1/2.  When the
	// index is not integral, then the value that corresponds to the
	// index is obtained by averaging the two values above and below.
	double Q1; // = H1 or lower hinge
	double Q1_ind; // can be a whole number or whole number + 1/2
	double Q2; // = median
	double Q2_ind; // can be a whole number or whole number + 1/2
	double Q3; // = H2 or upper hinge
	double Q3_ind;
	int min_IQR_ind; // index of first data point in IQR
	int max_IQR_ind; // index of last data point in IQR
	double IQR;
	double extreme_lower_val_15;
	double extreme_lower_val_30;
	double extreme_upper_val_15;
	double extreme_upper_val_30;
};

struct SampleStatistics {
	SampleStatistics();
    SampleStatistics(const vector<double>& data);
    SampleStatistics(const vector<double>& data,
                     const vector<bool>& undefs);
    SampleStatistics(const vector<double>& data,
                     const vector<bool>& undefs1,
                     const vector<bool>& undefs2);
    void CalculateFromSample(const vector<double>& data);
    void CalculateFromSample(const vector<double>& data,
                             const vector<bool>& undefs);
    void CalculateFromSample(const vector<Gda::dbl_int_pair_type>& data,
                             const vector<bool>& undefs);
    
	string ToString();
	
	int sample_size;
	double min;
	double max;
	double mean;
	double var_with_bessel;
	double var_without_bessel;
	double sd_with_bessel;
	double sd_without_bessel;
	
	static double CalcMin(const vector<double>& data);
	static double CalcMax(const vector<double>& data);
	static void   CalcMinMax(const vector<double>& data, double& min,
						     double& max);
	static double CalcMean(const vector<double>& data);
	static double CalcMean(const vector<Gda::dbl_int_pair_type>& data);
    
};

struct SimpleLinearRegression {
	SimpleLinearRegression() : covariance(0), correlation(0), alpha(0),
		beta(0), r_squared(0), std_err_of_estimate(0), std_err_of_beta(0),
		std_err_of_alpha(0), t_score_alpha(0), t_score_beta(0),
		p_value_alpha(0), p_value_beta(0),
		valid(false), valid_correlation(false),
		valid_std_err(false) {}
    
	SimpleLinearRegression(const vector<double>& X,
						   const vector<double>& Y,
						   double meanX, double meanY,
						   double varX, double varY);
    
	SimpleLinearRegression(const vector<double>& X,
						   const vector<double>& Y,
                           const vector<bool>& X_undef,
						   const vector<bool>& Y_undef,
						   double meanX, double meanY,
						   double varX, double varY);
    
	void CalculateRegression(const vector<double>& X,
							 const vector<double>& Y,
							 double meanX, double meanY,
							 double varX, double varY);
    
	static double TScoreTo2SidedPValue(double tscore, int df);
    
	string ToString();

    int n;
	double covariance;
	double correlation;
	double alpha;
	double beta;
	double r_squared;
	double std_err_of_estimate;
	double std_err_of_beta;
	double std_err_of_alpha;
	double t_score_alpha;
	double t_score_beta;
	double p_value_alpha;
	double p_value_beta;
	bool valid;
	bool valid_correlation;
	bool valid_std_err;
	double error_sum_squares;
};

struct AxisScale {
    AxisScale();
	AxisScale(double data_min_s, double data_max_s, int ticks_s = 5,
              int lbl_precision=2, bool lbl_prec_fixed_point=false);
	AxisScale(const AxisScale& s);
    AxisScale& operator=(const AxisScale& s);
	void CalculateScale(double data_min_s, double data_max_s,
						const int ticks = 5);
	void SkipEvenTics(); // only display every other tic value
	void ShowAllTics();
	string ToString();
	
	double data_min;
	double data_max;
	double scale_min;
	double scale_max;
	double scale_range;
	double tic_inc;	
    int lbl_precision;
    bool lbl_prec_fixed_point;
	int ticks;
	int p; // power of ten to scale significant digit
	vector<double>tics; // numerical tic values
	vector<string>tics_str; // tics in formated string representation
	vector<bool>tics_str_show; // if false, then don't draw tic string
};


namespace GenUtils {
	// other
	wxString BoolToStr(bool b);
	bool StrToBool(const wxString& s);
	wxString Pad(const wxString& s, int width, bool pad_left=true);
    wxString PadTrim(const wxString& s, int width, bool pad_left=true);
	wxString DblToStr(double x, int precision = 3, bool fixed_point=false);
    wxString IntToStr(int x, int precision = 0);
	wxString PtToStr(const wxPoint& p);
	wxString PtToStr(const wxRealPoint& p);
    void Transformation(int trans_type, vector<vector<double> >& data,
                        vector<vector<bool> >& undef);
    
	void MeanAbsoluteDeviation(int nObs, double* data);
    void MeanAbsoluteDeviation(int nObs, double* data, vector<bool>& undef);
	void MeanAbsoluteDeviation(vector<double>& data);
    void MeanAbsoluteDeviation(vector<double>& data, vector<bool>& undef);
    
	void DeviationFromMean(int nObs, double* data);
    void DeviationFromMean(int nObs, double* data, vector<bool>& undef);
    void DeviationFromMean(std::vector<double>& data, std::vector<bool>& undef);
    void DeviationFromMean(vector<double>& data);
    void DeviationFromMedian(vector<double>& data);
    void DeviationFromMedoid(vector<double>& data, double medoid_val);

	double Sum(vector<double>& data);
    double Median(std::vector<double>& data);

	double SumOfSquares(vector<double>& data);
    double SumOfSquaresMedian(vector<double>& data);
    double SumOfSquaresMedoid(vector<double>& data, double medoid_val);

    double SumOfManhattanMedian(vector<double>& data);
    double SumOfManhattanMedoid(vector<double>& data, double medoid_val);

	bool StandardizeData(int nObs, double* data);
    bool StandardizeData(int nObs, double* data, vector<bool>& undef);
	bool StandardizeData(vector<double>& data);
    bool StandardizeData(vector<double>& data, vector<bool>& undef);

    void RangeAdjust(std::vector<double>& data);
    void RangeAdjust(std::vector<double>& data, std::vector<bool>& undef);
    void RangeStandardize(std::vector<double>& data);
    void RangeStandardize(std::vector<double>& data, std::vector<bool>& undef);

    std::vector<double> rankify(const vector<double>& x);
    void rankify_fast(const vector<double>& x, std::vector<double>& Rank_X);
    double RankCorrelation(vector<double>& x, vector<double>& y);
    double Correlation(vector<double>& x, vector<double>& y);
    double GetVariance(vector<double>& data);
	wxString swapExtension(const wxString& fname, const wxString& ext);
	wxString GetFileDirectory(const wxString& path);
	wxString GetFileName(const wxString& path);
	wxString GetFileNameNoExt(const wxString& path);
	wxString GetFileExt(const wxString& path);
	/** If path is a relative path, return an absolute path based
	 on proj_path path. If path is absolute, return it as is. */
	wxString RestorePath(const wxString& proj_path, const wxString& path);
	wxString SimplifyPath(const wxString& proj_path, const wxString& path);
	/** Input: wd should be the working directory of the project file.  It
	 is assumed that this directory is valid.
	 path can be either a dir or file path.  If path is
	 relative, then path is returned unchanged.  If path is abolute, then
	 if directory or file is either the same as, or within a subdirectory
	 of current working directory, then return a relative path with
	 respect to the current working directory.  Otherwise, return the
	 resource string unchanged.  Within the the program, all resource
	 file should be stored with an absolute path.  Upon saving the
	 project file, each of these resource strings should be processed
	 by SimplfyPath to see if they can be converted into a relative path
	 with respect to the current Working Directory (project file location). */
	wxString SimplifyPath(const wxFileName& wd, const wxString& path);
	void SplitLongPath(const wxString& path, vector<wxString>& parts,
					   wxString& html_formatted,
					   int max_chars_per_part = 30);
	wxInt32 Reverse(const wxInt32 &val);
	long ReverseInt(const int &val);
	void SkipTillNumber(istream &s);
	void longToString(const long d, char* Id, const int base);
	double distance(const wxRealPoint& p1, const wxRealPoint& p2);
	double distance(const wxRealPoint& p1, const wxPoint& p2);
	double distance(const wxPoint& p1, const wxRealPoint& p2);
	double distance(const wxPoint& p1, const wxPoint& p2);
	double distance_sqrd(const wxRealPoint& p1, const wxRealPoint& p2);
	double distance_sqrd(const wxRealPoint& p1, const wxPoint& p2);
	double distance_sqrd(const wxPoint& p1, const wxRealPoint& p2);
	double distance_sqrd(const wxPoint& p1, const wxPoint& p2);
	double pointToLineDist(const wxPoint& p0, const wxPoint& p1,
						   const wxPoint& p2); 
	void strToInt64(const char *str, wxInt64 *val);
	void strToInt64(const wxString& str, wxInt64 *val);
	bool validInt(const char* str);
	bool validInt(const wxString& str);
	bool isEmptyOrSpaces(const char *str);
	bool isEmptyOrSpaces(const wxString& str);
	bool ExistsShpShxDbf(const wxFileName& fname, bool* shp_found,
						 bool* shx_found, bool* dbf_found);
	wxString FindLongestSubString(const vector<wxString> strings,
								  bool case_sensitive=false);
	wxString WrapText(wxWindow *win, const wxString& text, int widthMax);
	wxString GetExeDir();
	wxString GetWebPluginsDir();
	wxString GetResourceDir();
    wxString GetSamplesDir();
    wxString GetUserSamplesDir();
    wxString GetBasemapDir();
    wxString GetCachePath();
    wxString GetLangSearchPath();
	wxString GetLangConfig();
    wxString GetLoggerPath();

    bool less_vectors(const vector<int>& a,const vector<int>& b);
    bool smaller_pair(const std::pair<int, int>& a,
                      const std::pair<int, int>& b);
    
    // Act like matlab's [Y,I] = SORT(X)
    // Input:
    //   unsorted  unsorted vector
    // Output:
    //   sorted     sorted vector, allowed to be same as unsorted
    //   index_map  an index map such that sorted[i] = unsorted[index_map[i]]
    template <class T>
    void sort(
              vector<T> &unsorted,
              vector<T> &sorted,
              vector<size_t> &index_map);
    // Act like matlab's Y = X[I]
    // where I contains a vector of indices so that after,
    // Y[j] = X[I[j]] for index j
    // this implies that Y.size() == I.size()
    // X and Y are allowed to be the same reference
    template< class T >
    void reorder(
                 vector<T> & unordered,
                 vector<size_t> const & index_map,
                 vector<T> & ordered);
    
    // Comparison struct used by sort
    // http://bytes.com/topic/c/answers/132045-sort-get-index
    template<class T> struct index_cmp
    {
        index_cmp(const T arr) : arr(arr) {}
        bool operator()(const size_t a, const size_t b) const
        {
            return arr[a] < arr[b];
        }
        const T arr;
    };
    
    template <class T>
    void sort(
              vector<T> & unsorted,
              vector<T> & sorted,
              vector<size_t> & index_map)
    {
        // Original unsorted index map
        index_map.resize(unsorted.size());
        for(size_t i=0;i<unsorted.size();i++)
        {
            index_map[i] = i;
        }
        // Sort the index map, using unsorted for comparison
        sort(
             index_map.begin(),
             index_map.end(),
             index_cmp<vector<T>& >(unsorted));
        
        sorted.resize(unsorted.size());
        reorder(unsorted,index_map,sorted);
    }
    // This implementation is O(n), but also uses O(n) extra memory
    template< class T >
    void reorder(
                 vector<T> & unordered,
                 vector<size_t> const & index_map,
                 vector<T> & ordered)
    {
        // copy for the reorder according to index_map, because unsorted may also be
        // sorted
        vector<T> copy = unordered;
        ordered.resize(index_map.size());
        for(int i = 0; i<index_map.size();i++)
        {
            ordered[i] = copy[index_map[i]];
        }
    }
}

/** Old code used by LISA functions */
class GeoDaSet {
private:
    int size;
	int current;
    int* buffer;
    char* flags;
public:
	GeoDaSet(const int sz) : size(sz), current(0) {
		buffer = new int [ size ];
		flags = new char [ size ];
		memset(flags, '\x0', size);
	}
	virtual ~GeoDaSet() {
		if (buffer) delete [] buffer; buffer = 0;
		if (flags) delete [] flags; flags = 0;
		size = current = 0;
	}
    bool Belongs( const int elt) const {
		return flags[elt] != 0; }; // true if the elt belongs to the set
    void Push(const int elt) {
		// insert element in the set, if it is not yet inserted
		if (flags[elt] == 0)  {
			buffer[ current++ ] = elt;
			flags[elt] = 'i';  // check elt in
        }
    }
    int Pop() { // remove element from the set
		if (current == 0) return -1; // formerly GdaConst::EMPTY
        int rtn= buffer[ --current ];
        flags[rtn]= '\x0';   // check it out
        return rtn;
    }
    int Size() const { return current; }
	void Reset() {
		memset(flags, '\x0', size);
		current = 0;
	}
};

#endif
