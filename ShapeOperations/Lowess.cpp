/**
 This source file is a modified version of loess.cpp from
 "The Variation Toolkit" which is distributed under the GNU GPL v3 license
 https://code.google.com/p/variationtoolkit/source/browse/trunk/src/loess.cpp
 
 Implementation of the loess algorithm in C++
 original code R project: http://www.r-project.org/
 http://svn.r-project.org/R/trunk/src/library/stats/src/lowess.c
 */

#include <string.h> // memset
#include <cmath>
#include <algorithm> // for std::partial_sort
#include <memory>
#include <stdexcept>
#include "Lowess.h"

using namespace std;

#define imin2(a,b) std::min(a,b)
#define imax2(a,b) std::max(a,b)
#define fmax2(a,b) std::max(a,b)

#define rPsort(x,n,k) std::partial_sort(&x[0],&x[n],&x[k]);

const double Lowess::default_f = 0.2;
const int Lowess::default_iter = 5;
const double Lowess::default_delta_factor = 0.02;
const int Lowess::max_iter = 10000;

Lowess::Lowess(double f, int iter, double delta_factor)
{
	SetF(f);
	SetIter(iter);
	SetDeltaFactor(delta_factor);
}

Lowess::Lowess(const Lowess& s)
{
	operator=(s);
}

Lowess& Lowess::operator=(const Lowess& s)
{
	f = s.f;
	iter = s.iter;
	delta_factor = s.delta_factor;
	return *this;
}

Lowess::~Lowess()
{
}

double Lowess::GetF() const
{
	return f;
}

void Lowess::SetF(double v)
{
	if (v < 0) v = 0;
	if (v > 1) v = 1;
	f = v;
}

int Lowess::GetIter() const
{
	return iter;
}

void Lowess::SetIter(int v)
{
	if (v < 0) v = 0;
	if (v > max_iter) v = max_iter;
	iter = v;
}

double Lowess::GetDeltaFactor() const
{
	return delta_factor;
}

void Lowess::SetDeltaFactor(double v)
{
	if (v <= 0) v = default_delta_factor;
	if (v > 1) v = 1;
	delta_factor = v;
}

void Lowess::calc(const std::vector<double>& x,
                  const std::vector<double>& y,
                  std::vector<double>& smoothed_y)
{
	size_t n = x.size();

	double rangeX = x[n-1]-x[0];
	double delta = delta_factor * rangeX;

	double* rw=new double[n];
	double* ys=new double[n];
	double* res=new double[n];
	memset((void*)rw, 0, sizeof(double)*n);
	memset((void*)ys, 0, sizeof(double)*n);
	memset((void*)res, 0, sizeof(double)*n);
	clowess(&x.front(), &y.front(), (int) n,
            f, (size_t) iter, delta, ys, rw, res);

	smoothed_y.resize(n);
	for (size_t i=0; i<n; ++i)
        smoothed_y[i] = ys[i];
    
	delete[] ys;
	delete[] rw;
	delete[] res;
}

void Lowess::lowest(const double *x, const double *y, int n,
                    const double *xs, double *ys,
                    int nleft, int nright, double *w,
                    bool userw, double *rw, bool *ok)
{
	int nrt, j;
	double a, b, c, h, h1, h9, r, range;
	
	x--;
	y--;
	w--;
	rw--;
	
	range = x[n]-x[1];
	h = fmax2(*xs-x[nleft], x[nright]-*xs);
	h9 = 0.999*h;
	h1 = 0.001*h;
	
	/* sum of weights */
	
	a = 0.;
	j = nleft;
	while (j <= n) {
		/* compute weights */
		/* (pick up all ties on right) */
		w[j] = 0.;
		r = fabs(x[j] - *xs);
		if (r <= h9) {
			if (r <= h1) {
				w[j] = 1.;
			} else {
				w[j] = fcube(1.-fcube(r/h));
			}
			if (userw) w[j] *= rw[j];
			a += w[j];
		} else if (x[j] > *xs) {
			break;
		}
		j = j+1;
	}
	
	/* rightmost pt (may be greater */
	/* than nright because of ties) */
	
	nrt = j-1;
	if (a <= 0.) {
		*ok = false;
	} else {
		*ok = true;
		/* weighted least squares */
		/* make sum of w[j] == 1 */
		for (j=nleft ; j<=nrt ; j++) 
			w[j] /= a;
		if (h > 0.) {
			a = 0.;
			
			/*  use linear fit */
			/* weighted center of x values */
			
			for (j=nleft ; j<=nrt ; j++) 
				a += w[j] * x[j];
			b = *xs - a;
			c = 0.;
			for (j=nleft ; j<=nrt ; j++) 
				c += w[j]*fsquare(x[j]-a);
			if (sqrt(c) > 0.001*range) {
				b /= c;
				/* points are spread out */
				/* enough to compute slope */
				for (j=nleft; j <= nrt; j++) 
					w[j] *= (b*(x[j]-a) + 1.);
			}
		}
		*ys = 0.;
		for (j=nleft; j <= nrt; j++) 
			*ys += w[j] * y[j];
	}
}

void Lowess::clowess(const double *x, const double *y, int n,
                     double f, size_t iter, double delta,
                     double *ys, double *rw, double *res)
{
	size_t cur_iter;
	int i, j, last, m1, m2, nleft, nright, ns;
	bool ok;
	double alpha, c1, c9, cmad, cut, d1, d2, denom, r, sc;
	
	if (n < 2) {
		ys[0] = y[0]; return;
	}
	
	/* nleft, nright, last, etc. must all be shifted to get rid of these: */
	x--;
	y--;
	ys--;
	
	
	/* at least two, at most n points */
	ns = imax2(2, imin2(n, (int)(f*n + 1e-7)));
	
	/* robustness iterations */
	
	cur_iter = 1;
	while (cur_iter <= iter+1) {
		nleft = 1;
		nright = ns;
		last = 0;       /* index of prev estimated point */
		i = 1;          /* index of current point */
		
		for(;;) {
			if (nright < n) {
				
				/* move nleft,  nright to right */
				/* if radius decreases */
				
				d1 = x[i] - x[nleft];
				d2 = x[nright+1] - x[i];
				
				/* if d1 <= d2 with */
				/* x[nright+1] == x[nright], */
				/* lowest fixes */
				
				if (d1 > d2) {
					
					/* radius will not */
					/* decrease by */
					/* move right */
					
					nleft++;
					nright++;
					continue;
				}
			}
			
			/* fitted value at x[i] */
			
			lowest(&x[1], &y[1], n, &x[i], &ys[i],
						 nleft, nright, res, cur_iter>1, rw, &ok);
			if (!ok) ys[i] = y[i];
			
			/* all weights zero */
			/* copy over value (all rw==0) */
			
			if (last < i-1) {
				denom = x[i]-x[last];
				
				/* skipped points -- interpolate */
				/* non-zero - proof? */
				
				for (j = last+1; j < i; ++j) {
					alpha = (x[j]-x[last])/denom;
					ys[j] = alpha*ys[i] + (1.-alpha)*ys[last];
				}
			}
			
			/* last point actually estimated */
			last = i;
			
			/* x coord of close points */
			cut = x[last]+delta;
			for (i = last+1; i <= n; i++) {
				if (x[i] > cut)
					break;
				if (x[i] == x[last]) {
					ys[i] = ys[last];
					last = i;
				}
			}
			i = imax2(last+1, i-1);
			if (last >= n)
				break;
		}
		/* residuals */
		for(i = 0; i < n; i++)
			res[i] = y[i+1] - ys[i+1];
		
		/* overall scale estimate */
		sc = 0.;
		for(i = 0; i < n; i++) sc += fabs(res[i]);
		sc /= n;
		
		/* compute robustness weights */
		/* except last time */
		
		if (cur_iter > iter) 
			break;

		for(i = 0 ; i < n ; i++) 
			rw[i] = fabs(res[i]);
		
		/* Compute   cmad := 6 * median(rw[], n)  ---- */
		m1 = n/2;
		/* partial sort, for m1 & m2 */
		//rPsort(rw, n, m1);
		rPsort(rw, m1, n);
		if (n % 2 == 0) {
			m2 = n-m1-1;
			//rPsort(rw, n, m2);
			rPsort(rw, m2, n);
			cmad = 3.*(rw[m1]+rw[m2]);
		}
		else { /* n odd */
			cmad = 6.*rw[m1];
		}
		
		if (cmad < 1e-7 * sc) /* effectively zero */
			break;
		c9 = 0.999*cmad;
		c1 = 0.001*cmad;
		for(i = 0 ; i < n ; i++) {
			r = fabs(res[i]);
			if (r <= c1)
				rw[i] = 1.;
			else if (r <= c9)
				rw[i] = fsquare(1.-fsquare(r/cmad));
			else
				rw[i] = 0.;
		}
		cur_iter++;
	}
}
//see also ..R-2.11.0/src/library/stats/R/lowess.R


