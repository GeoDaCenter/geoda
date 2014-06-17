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

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"

#include "mix.h"
#include "Lite2.h"
#include "DenseVector.h"

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
    #include <vecLib/vecLib.h>
#else
	#include "blaswrap.h"
	#include "f2c.h"

    extern "C" int dgesvd_(char *jobu, char *jobvt, integer *m, integer *n,
        doublereal *a, integer *lda, doublereal *s, doublereal *u, integer *ldu,
        doublereal *vt, integer *ldvt, doublereal *work, integer *lwork,
        integer *info);
    extern "C" int dspev_(char *jobz, char *uplo, integer *n, doublereal *ap,
        doublereal *w, doublereal *z__, integer *ldz, doublereal *work,
        integer *info);
#endif

extern bool ordinaryLS(DenseVector &y, 
				 DenseVector * X, 
				 double ** &cov, 
				 double * resid, 
				 DenseVector &ols);

// standard normal cumulative distribution function
double nc(double x)  
{ 
    double result;
    if (x < -7.)  result = ndf(x)/sqrt(1.+x*x);
    else if (x > 7.)  result = 1. - nc(-x);
    else  {
        result = 0.2316419;
        double a[5] = {0.31938153,-0.356563782,1.781477937,-1.821255978,1.330274429};
        result=1./(1+result*fabs(x));
        result=1-ndf(x)*(result*(a[0]+result*(a[1]+result*(a[2]+result*(a[3]+result*a[4])))));
        if (x<=0.) result=1.-result;
    };
    return result;
}

extern float erfcc(float x);

double cdf(double x)
{
	// cumulative distribution function of standard normal
    const double eps = 10e-17;
    double p=(1+erfcc(x/sqrt((double)2)))/2.0;;
    if (p==1.0)
			p = p -eps;
    else if (p==0)
       p = eps;
    
    return p;
}

// standard normal density function
double ndf(double t)  {
    return 0.398942280401433*exp(-t*t/2);
}

void error(const char *s, const char *s2)  
{
	wxMessageBox(wxString::Format("%s, %s", s, s2));
}

double product(const double * v1, const double * v2, const int &sz)  
{
    double s = 0;
    for (int cnt = 0; cnt < sz; ++cnt)
        s += v1[cnt] * v2[cnt];
    return s;
}

double *vproduct(double *v1, double* v2, int dim)
{
	double *rslt = new double[dim];
	if (rslt ==NULL)
		return NULL;
 
	for (int i=0;i < dim; i++)
		rslt[i] = v1[i] * v2[i];

	return rslt;

}

double norm(const double *v, const int size)  
{
    double scale = 0, ssq = 1, t;
    for (int cnt = 0; cnt < size; ++cnt)  
		{
        double absc = fabs(v[ cnt ]);
        if (scale < absc)  
				{
            t = scale / absc;
            ssq = 1.0 + ssq * t * t;
            scale = absc;
        }  
				else  if (absc > 0.0)  
				{
            t = absc / scale;
            ssq += t * t;
        };
    }
    return scale * scale * ssq;
}

// compute standard normal deviate using Box-Muller  transformation
double gauss()  
{
    static int iset = 0;
    static double gset;
    double v1, v2, fac, r;
    if (iset == 0)  {		// we don't have an extra deviate yet
        do  {
            v1 = 2.0 * rand() / RAND_MAX - 1.0;
            v2 = 2.0 * rand() / RAND_MAX - 1.0;
            r = v1*v1 + v2*v2;		
        }  while (r >= 1.0);		//generate a couple of new rands within unit circle
        fac = sqrt(-2.0 * log(r) / r);
        gset = v1 * fac;
        iset = 1;
        return v2 * fac;
    }  else  {
        iset = 0;
        return gset;
    }

}

double normW(const double *v, const double *w, const int size)  {
    double scale = 0, ssq = 1, t;
    for (int cnt = 0; cnt < size; ++cnt)  {
        double absc = fabs(v[cnt] / w[cnt]);
        if (scale < absc)  {
            t = scale / absc;
            ssq = 1.0 + ssq * t * t;
            scale = absc;
        }  else  if (absc > 0.0)  {
            t = absc / scale;
            ssq += t * t;
        };
    }
    return scale * scale * ssq;
}

float erfcc(float x)
{
	float t,z,ans;
	z=fabs(x);
	t=1.0/(1.0+0.5*z);
	ans = t * exp(-z*z-1.26551223+
		    t * (1.00002368+
				t * (0.37409196+
				t * (0.09678418+
				t * (-0.18628806+
				t * (0.27886807+
				t * (-1.13520398+
				t * (1.48851587+
				t * (-0.82215223+
				t * 0.17087277)))))))));
	return x >= 0.0 ? ans : 2.0-ans;
	
}

double gammaln(const double xx)  
{
    double cof[6] = { 76.18009173,    -86.50532033,        24.01409822,
        -1.231739516,     0.120858003e-2,    -0.536382e-5  };
    double x = xx - 1.0;
    double tmp = x + 5.5;
    tmp -= (x + 0.5) * log(tmp);
    double s = 1.0;
    for (int j = 0; j <= 5; ++j)  
		{
        x += 1.0;
        if (x != 0.0) 
					s += cof[j] / x;
    };
    return -tmp + log(2.50662827465*s);
}

//Used by betai: Evaluates continued fraction for incomplete beta function by 
// modified Lentz's method (x5.2).
float betacf(float a, float b, float x)
{
	const int MAXIT = 100;
  const double EPS = 3.0e-7;
	const double FPMIN = 1.0e-30;

	int m,m2;
	float aa,c,d,del,h,qab,qam,qap;
	// These q's will be used in factors that occur
	// in the coe.cients (6.4.6). 

	qab=a+b; 
	qap=a+1.0;
	qam=a-1.0;
	c=1.0; 

	// First step of Lentz's method.

	d=1.0-qab*x/qap;
	if (fabs(d) < FPMIN) d=FPMIN;
	d=1.0/d;
	h=d;
	for (m=1;m<=MAXIT;m++) 
	{
		m2=2*m;
		aa=m*(b-m)*x/((qam+m2)*(a+m2));
		d=1.0+aa*d; 
		// One step (the even one) of the recurrence.
		if (fabs(d) < FPMIN) d=FPMIN;
			c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
			d=1.0/d;
		h *= d*c;
		aa = -(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
		d=1.0+aa*d; 
		// Next step of the recurrence (the odd one).
		if (fabs(d) < FPMIN) d=FPMIN;
			c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
			d=1.0/d;
		del=d*c;
		h *= del;
		if (fabs(del-1.0) < EPS) break; 
		//Are we done?
	}
	if (m > MAXIT) 
	{
		// cout << "a or b too big, or MAXIT too small in betacf" << '\n';
			return -999;
	}
	return h;
}

	
// Returns the incomplete beta function Ix(a; b)
float betai(float a, float b, float x)
{
	float bt;
	if (x < 0.0 || x > 1.0 || a==0.0 || b==0.0) 
	{
		// cout << "Bad x in routine betai" << '\n';
		return -999;
	}
	if (x == 0.0 || x == 1.0) 
		bt=0.0;
	else 
		// Factors in front of the continued fraction.
	  bt = exp(gammaln(a+b)-gammaln(a)-gammaln(b)+a*log(x)+b*log(1.0-x));

	if (x < (a+1.0)/(a+b+2.0)) 
		// Use continued fraction directly.
		return bt*betacf(a,b,x)/a;
	else 
		// Use continued fraction after making the symmetry
	  // transformation. 
		return 1.0-bt*betacf(b,a,1.0-x)/b;
}

/*
Returns the incomplete gamma function P(a; x) 
evaluated by its series representation as gamser.
Also returns ln 
*/
double gser(const double a, const double x)  
{
    const int ITMAX = 100;
    const double EPS = 3.0e-7;

    double gln = gammaln(a);
    double ap = a;
    double del, sum;
		if (a > 0.0 || a < 0.0) 
		{
			del = sum = 1.0 / a;
			for (int n = 1; n <= ITMAX; ++n)  
			{
					ap += 1.0;
					del *= x/ap;
					sum += del;
					if (fabs(del) < fabs(sum)*EPS)
							return sum*exp(-x+a*log(x)-gln);
			}
		}
    return 0;
}

/*
Returns the incomplete gamma function Q(a; x) 
evaluated by its continued fraction representation
as gammcf. Also returns ln 
*/
double gcf(const double a, const double x)  
{
    const int ITMAX = 100;
    const double EPS = 3.0e-7;
    double gold = 0, g, fac = 1.0, b1 = 1.0;
    double b0 = 0, anf, ana, an, a1, a0 = 1.0;
    double gln = gammaln(a);
    a1 = x;
    for (int n = 1; n <= ITMAX; ++n)  
		{
        an = 1.0 * n;
        ana = an - a;
        a0 = (a1 + a0*ana) * fac;
        b0 = (b1 + b0*ana) * fac;
        anf = an * fac;
        a1 = x * a0 + anf * a1;
        b1 = x * b0 + anf * b1;
        if (fabs(a1) > 0.0)  
				{
            fac = 1.0 / a1;
            g = b1 * fac;
            if (fabs((g-gold)/g) < EPS)
                return exp(-x+a*log(x)-gln) * g;
            gold = g;
        };
    };
    return 0;
}


// compute incomplete gamma function
double gammp(const double a, const double x)  
{
    if (x < 0 || a < 0)
		{
				return -1;
		}

		double chi;
    if (x < (a + 1.0))
        chi = 1.0 - gser(a, x);
    else
        chi =  gcf(a, x);
    return chi;
}


            
// Returns the (1-tailed) significance level (p-value) of an F
// statistic given the degrees of freedom for the numerator (dfR-dfF) and
// the degrees of freedom for the denominator (dfF).
double fprob (int dfnum, int dfden, double F)
{
	  const double a = (double) dfnum;
	  const double b = (double) dfden;

		const double baF = ( b + a * F);
		if (fabs(baF) == 0.0) return -9.0;


		return betai(0.5*b, 0.5*a, b / baF);
}

/*
Returns the area under the normal curve 'to the left of' the given z value.
Thus, 
    for z<0, zprob(z) = 1-tail probability
    for z>0, 1.0-zprob(z) = 1-tail probability
    for any z, 2.0*(1.0-zprob(abs(z))) = 2-tail probability
Adapted from z.c in Gary Perlman's |Stat.
*/
double zprob(double z)
{
	double x,y,w, prob;
  const double Z_MAX = 6.0;//    # maximum meaningful z-value
  if (z == 0.0)
			x = 0.0;
	else
	{
		y = 0.5 * fabs(z);
		if (y >= (Z_MAX*0.5))
				x = 1.0;
		else if (y < 1.0)
		{
				w = y*y;
				x = ((((((((0.000124818987 * w
				-0.001075204047) * w +0.005198775019) * w
						-0.019198292004) * w +0.059054035642) * w
					-0.151968751364) * w +0.319152932694) * w
				-0.531923007300) * w +0.797884560593) * y * 2.0;
		}
		else
		{
				y = y - 2.0;
				x = (((((((((((((-0.000045255659 * y
						 +0.000152529290) * y -0.000019538132) * y
					 -0.000676904986) * y +0.001390604284) * y
				 -0.000794620820) * y -0.002034254874) * y
						 +0.006549791214) * y -0.010557625006) * y
					 +0.011630447319) * y -0.009279453341) * y
				 +0.005353579108) * y -0.002141268741) * y
			 +0.000535310849) * y +0.999936657524;
		}
	}
  
	if (z > 0.0)
		prob = ((x+1.0)*0.5);
  else
		prob = ((1.0-x)*0.5);

  return prob;
}







inline double _exp(double x)
{
	const double BIG = 20.0;
  if (x < -BIG)
      return 0.0;
  else
      return exp(x);
}

/*
Returns the (1-tailed) probability value associated with the provided
chi-square value and df.  Adapted from chisq.c in Gary Perlman's |Stat.
*/
double chicdf(double chisq, int df)
{
    const double BIG = 20.0;
		double const pi = 3.141592653589793;
		double a,c,e,s,y,z;
		bool even = false;
		int k = df /2;

    if (chisq <=0 || df < 1)
        return 1.0;

    a = 0.5 * chisq;

    if (df==(k*2))
        even = true;

    if (df > 1)
        y = _exp(-a);

    if (even)
        s = y;
    else
        s = 2.0 * zprob(-sqrt(chisq));
    if (df > 2)
		{
        double chisq1 = 0.5 * (df - 1.0);
        if (even)
            z = 1.0;
        else
            z = 0.5;

        if (a > BIG)
        {
            if (even)
                e = 0.0;
            else
                e = log(sqrt(pi));
            c = log(a);
            while (z <= chisq)
						{
                e = log(z) + e;
                s = s + _exp(c*z-a-e);
                z = z + 1.0;
						}
            return s;
				}
        else
				{
            if (even)
                e = 1.0;
            else
                e = 1.0 / sqrt(pi) / sqrt(a);
            c = 0.0;
            while (z <= chisq)
						{
                e = e * (a/float(z));
                c = c + e;
                z = z + 1.0;
						}
            return (c*y+s);
				}
			}
    else
        return s;

}



extern double product(const double * v1, const double * v2, const int &sz);

double* JarqueBera(double* e, long n, long k)
{

	double sigma2 = norm(e,n);
	if (n <=30) 
       sigma2 = sigma2 / (n-1); //        # unbiased estimator of population sig sq.
  else
       sigma2 = sigma2 / n; // # mean square of sample residuals

  double* m2e = vproduct(e,e,n);

  double skewness = product(m2e,e,n) / n / pow(sigma2,1.5);
  skewness *= skewness;
  double kurtosis = product(m2e,m2e,n) /n / geoda_sqr(sigma2);

  double jb = n * (skewness/6.0 + (geoda_sqr(kurtosis-3.0) / 24.0));

	double* rslt = new double [3];
	rslt[0] = jb;
	rslt[1] = 2.0;
	rslt[2] = gammp( 1.0, jb/2.0 );
// rslt[2] = chicdf(jb,2.0);
/*
	rslt[3] = skewness;
	rslt[4] = 1.0;
	rslt[5] = chicdf(skewness,1);
	rslt[6] = kurtosis;
	rslt[7] = 1.0;
	rslt[8] = chicdf(kurtosis,1);
*/
	return rslt;
//local JB = (r(N)/6)*((r(skewness)^2)+[(1/4)*(r(kurtosis)-3)^2])

}
extern bool SymMatInverse(Iterator<WVector> mt);
extern bool SymMatInverse(double ** mt, const int dim);


double *BP_Test(double *resid, int obs, double** X, int nvar, bool InclConst)
{
	DenseVector e(resid, obs, false), g(obs), sqvar(obs);
	DenseVector *x = new DenseVector [nvar];
	int i = 0, j = 0;
 	double ns = 0;

	// Assign x = X * X

	x[0].alloc(obs);
	for (j = 0; j < obs; j++) {
		x[0].setAt(j, 1.0);
	}
	for (i = 1; i < nvar; i++) {
		if (InclConst) {
			x[i].absorb(X[i], obs);
		} else {
			x[i].absorb(X[i - 1], obs);
		}
		ns = x[i].norm();
		for (j = 0; j < obs; j++) {
			x[i].setAt(j, geoda_sqr(x[i].getValue(j) / sqrt(ns)));
		}
	}

	double **cov = new double * [nvar];
	for (i = 0; i < nvar; i++)
		alloc(cov[i], nvar);

	// use SVD to compute inverse(Z'Z)
	// use dgesvd_

	char jobu = 'S', jobvt = 'S';
	long int m = obs, n = nvar;
	long int lda = obs, ldu = obs, ldvt = nvar, lwork = 5 * obs, info = 0;
	double *a = new double [obs * nvar];
	double *s = new double [nvar];
	double *u = new double [ldu * nvar];
	double *vt = new double [ldvt * nvar];
	double *work = new double [lwork];
	for (i = 0; i < obs; i++) {
		for (j = 0; j < nvar; j++) {
			a[i + obs * j] = x[j].getValue(i);
		}
	}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
	dgesvd_(&jobu, &jobvt, &m, &n, a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork, &info);
#else
	dgesvd_(&jobu, &jobvt, (integer*)&m, (integer*)&n, (doublereal*)a, (integer*)&lda, (doublereal*)s, (doublereal*)u, (integer*)&ldu, (doublereal*)vt, (integer*)&ldvt, (doublereal*)work, (integer*)&lwork, (integer*)&info);
#endif

	if (!info) {
		// (z'z)^(-1) = VW^(-2)V'
		for (i = 0; i < nvar; i++) {
			for (j = 0; j < nvar; j++) {
				for (m = 0; m < nvar; m++) {
					cov[i][j] += (vt[i * nvar + m] * vt[m + j * nvar]) / (s[m] * s[m]);
				}
			}
		}
	} else {
		// do nothing
	}

	double mse = e.norm() / obs;

	// compute gi

	for (j = 0; j < obs; j++) {
		double e2 = geoda_sqr(e.getValue(j));
		g.setAt(j, e2 - mse);
		sqvar.setAt(j, geoda_sqr(e2 - mse));
	}
	double mean = sqvar.sum() / obs;

	DenseVector gz(nvar), gzizz(nvar);

	for (i = 0; i < nvar; i++)
		gz.setAt(i, g.product(x[i])); // gz = g'z (1 x expl)

	gz.squareTimesColumn(gzizz, cov); // gzizz = g'z * inv(cov)
	double bp = gz.product(gzizz); // bp = g'z[(z'z)^-1]z'g

	double *rslt = new double [6];
	rslt[0] = 1. / (2 * geoda_sqr(mse)) * bp; // Breusch-Pagan
	rslt[1] = InclConst ? nvar - 1 : nvar;
 	rslt[2] = gammp(rslt[1] / 2.0, rslt[0] / 2.0);
	rslt[3] = (1.0 / mean) * bp; // Koenker-Basset
	rslt[4] = rslt[1];
 	rslt[5] = gammp(rslt[1] / 2.0, rslt[3] / 2.0);

	release(&x);
	release(&cov);

	return rslt;
}

double MC_Condition_Number(double **X, int dim, int expl)
{
	int i = 0, j = 0, row = 0, column = 0; 
	double xn = 0;
	DenseVector *x = new DenseVector [dim];

	for (i = 0; i < expl; i++)
		x[i].absorb(X[i], dim, false);

	for (i = 0; i < expl; i++) {
		xn = x[i].norm();
		for (j = 0; j < dim; j++) {
			x[i].setAt(j, x[i].getValue(j) / sqrt(xn));
		}
	}

	// use CLAPACK to compute the largest and the smallest eigenvalues
	// use dspev_

	char jobz = 'N', uplo = 'U';
	long int n = expl;
	long int ldz = expl, lwork = 3 * expl, info = 0;
	double *a = new double [expl * (expl + 1) / 2];
	double *s = new double [expl];
	double *z = NULL;
	double *work = new double [lwork];
	for (row = 0; row < expl; row++) {
		for (column = row + 1; column < expl; column++)
			a[row + column * (column + 1) / 2] = x[row].product(x[column]);
			a[row + row * (row + 1) / 2] = x[row].norm();
	}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
	dspev_(&jobz, &uplo, &n, a, s, z, &ldz, work, &info);
#else
	dspev_(&jobz, &uplo, (integer*)&n, (doublereal*)a, (doublereal*)s, (doublereal*)z, (integer*)&ldz, (doublereal*)work, (integer*)&info);
#endif

	if (!info) {
		double max = s[expl - 1], min = s[0];
		return sqrt(max / min);
	} else {
	//	cerr << "error in computing eigenvalues" << endl;
		wxMessageBox("error in computing eigenvalues");
		return -999;
	}
}


double* WhiteTest(int obs, int nvar, double* resid, double** X, bool InclConstant)
{
	typedef double* double_ptr_type;
	double *r2 = new double[obs];
	int i = 0, jj = 0;
	//	if (!InclConstant) DevFromMean(obs,resid);

	// (1) r2 = Compute e2
	double r2_bar = 0;
	for (i=0;i<obs;i++)
	{
		r2[i] = geoda_sqr(resid[i]);
		r2_bar += r2[i];
	}
	r2_bar /= obs;
	
	
	// (2) define (n*n + 3n)/2 memory location for w
	const int df = InclConstant? (geoda_sqr(nvar-1)+3*(nvar-1))/2 : (geoda_sqr(nvar)+3*nvar)/2;
	double_ptr_type *w  = new double_ptr_type[df+1];

	for (i=0;i<=df;i++) w[i] = new double[obs];


	// (3) keep original X into w[1 .. nvar][]
	int ix = InclConstant? 0 : 1;
	int k = nvar+ix;

	for (jj=0;jj<obs;jj++)
		w[0][jj] = 1.0;

	if (InclConstant)
	{
		for (i=1;i<nvar;i++)
		{

				for (jj=0;jj<obs;jj++)
					w[i][jj] = X[i][jj];
		}
	}
	else
	{
		for (i=0;i<nvar;i++)
				for (jj=0;jj<obs;jj++)
					w[i+1][jj] = X[i][jj];
	}

	// (4) Create cross product of Xs and store them into w[nvar ... df][]
	for (i=1-ix;i<nvar;i++)
	{
		for (int j=i; j<nvar;j++)
		{
			for (jj=0;jj<obs;jj++)
				w[k][jj] = X[i][jj] * X[j][jj];
			k++;
		}
	}

	// (5) Compute OLS, r2 on w
	DenseVector		yw(r2, obs, false), olsw(k);
  DenseVector *xw = new DenseVector[k];

  for (int cnt = 0; cnt < k; ++cnt)  
	{

     xw[cnt].absorb(w[cnt], obs, false);
	}


  double ** cov = new double * [k];
  double *u = new double[obs];
	for (i = 0; i < k; i++) {
		cov[i] = new double [k];
		for (jj = 0; jj < k; jj++) {
			cov[i][jj] = 0;
		}
	}
	double *rsl = new double[3];
	rsl[0] = df;
	rsl[1] = -99999;
	rsl[2] = -99999;
  if (!ordinaryLS(yw, xw, cov, u, olsw))
	{
		return rsl;
	}

	double s_u = 0.0;
	for (i=0;i<obs;i++)
		s_u += geoda_sqr(r2[i] - r2_bar);

	rsl[1]= obs * ( 1 - (norm(u,obs) / s_u));
//	rsl[1]= chicdf(rsl[0],df);
	rsl[2]= gammp( double (df) / 2.0, rsl[1]/2.0 );

	
	return rsl;

}

