/**
 This source file is a modified version of loess.cpp from
 "The Variation Toolkit" which is distributed under the GNU GPL v3 license
 https://code.google.com/p/variationtoolkit/source/browse/trunk/src/loess.h
 
 Implementation of the loess algorithm in C++
 original code R project: http://www.r-project.org/
 http://svn.r-project.org/R/trunk/src/library/stats/src/lowess.c
 
 See https://stat.ethz.ch/R-manual/R-devel/library/stats/html/lowess.html
 for original R code documentation.
 */

#ifndef __GEODA_CENTER_LOWESS_H__
#define __GEODA_CENTER_LOWESS_H__

#include <vector>

/** This class is a wrapper for function "calc" that performs the computations
 for the LOWESS smoother which uses locally-weighted polynomial regression. */
class Lowess {
public:
	Lowess(double f=default_f, int iter=default_iter,
				 double delta_factor=default_delta_factor);
	Lowess(const Lowess& s);
	Lowess& operator=(const Lowess& s);
	virtual ~Lowess();
	
	double GetF() const;
	void SetF(double v);
	int GetIter() const;
	void SetIter(int v);
	double GetDeltaFactor() const;
	void SetDeltaFactor(double v);

	/** Perform LOWESS smoothing and return results in smoothed_y.
	 The input data is assumed to be sorted by values in x and to only contain
	 finte, well-defined real numbers. */
	void calc(const std::vector<double>& x, const std::vector<double>& y,
						std::vector<double>& smoothed_y);

	// Defaults
	static const double default_f;
	static const int default_iter;
	static const double default_delta_factor;
	static const int max_iter;
	
private:
	/** f: default value is 0.7.
	 The smoother span. This gives the proportion of points in the plot which
	 influence the smooth at each value. Larger values give more smoothness. */
	double f;
	
	/** iter: default value is 5.
	 The number of ‘robustifying’ iterations which should be performed.
	 Using smaller values of iter will make lowess run faster. */
	int iter;
	
	/** delta_factor: default value is 2/100.
	 Values <= 0 default to 2/100, while values > 1 default to 1.
	 delta_factor is multiplied by the range of input x to calculate delta.
	 delta is used to speed up computation: Instead of computing the local
	 polynomial fit at each data point, it is not computed for points within
	 delta of the last computed point, and linear interpolation is used to
	 fill in the fitted values for the skipped points. */
	double delta_factor;
	
	static inline double fsquare(double x) { return x * x; }
	static inline double fcube(double x) { return x * x * x; }
	
	void lowest(const double *x, const double *y,
							int n, const double *xs, double *ys,
							int nleft, int nright, double *w,
							bool userw, double *rw, bool *ok);
	
	void clowess(const double  *x, const double *y, int n,
							 double f, size_t iter, double delta,
							 double *ys, double *rw, double *res);
};

#endif
