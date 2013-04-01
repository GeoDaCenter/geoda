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

/*
Main module to solve a ML estimation problem with the characteristic polynomial method.
test functionality for solving sparse polynomial problem
The IDE project should also include SLStream.cpp
 */
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/gauge.h>
#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"
#include "mix.h"

#include "Lite2.h"
#include "Weights.h"
#include "PowerLag.h"

#include "polym.h"
#include <time.h>
#include "../logger.h"
#include "ML_im.h"

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
    #include <vecLib/vecLib.h>
#else
	#include "blaswrap.h"
	#include "f2c.h"

    extern "C" int dgesvd_(char *jobu, char *jobvt, integer *m, integer *n,
        doublereal *a, integer *lda, doublereal *s, doublereal *u,
        integer *ldu, doublereal *vt, integer *ldvt, doublereal *work,
        integer *lwork, integer *info);
    extern "C" int dspev_(char *jobz, char *uplo, integer *n, doublereal *ap,
        doublereal *w, doublereal *z__, integer *ldz, doublereal *work,
        integer *info);
    extern "C" int dgeev_(char *jobvl, char *jobvr, integer *n, doublereal *a,
        integer *lda, doublereal *wr, doublereal *wi, doublereal *vl,
        integer *ldvl, doublereal *vr, integer *ldvr, doublereal *work,
        integer *lwork, integer *info);
    extern "C" int dgesv_(integer *n, integer *nrhs, doublereal *a,
        integer *lda, integer *ipiv, doublereal *b, integer *ldb,
        integer *info);
#endif

#define tol 1e-14


/* Template to compute value of the poynomial for any value.
     P(x) = a0 + a1*x + a2*x^2 + a3*x^3 + ... + aN*x^N
T is class of the polynomial and also is the class of the value.
Normally, T = double.
poly -- polynomial iterator (iterator over a0, a1, a2, ..., aN);
at   -- value at which the polynomial is being evaluated;
*/
template <class T>
T PolyValue(Iterator<T> poly, const T at)  
{
  T     acc(0), power(1);
  for ( ; poly; ++poly)  {
	acc += *poly * power;
	power *= at;		// update and store powers of the value
  };
  return acc;
}



/*   AddUp
* compute the sum of all elements in the Vector. Vector must be a vector of Pairs.
* it -- Iterator that provides access to elements of the Vector.
* Note: it could specify only a portion of an actual Vector.
*/
template <class IT>
VALUE AddUp(Iterator<IT> it)  
{
    VALUE sum = 0;
    for ( ; it; ++it)
        sum += (*it).second;
    return sum;
}


/*   RowStandardize
* a function to compute the sum of the elements in each row of the matrix and
* divide them by that sum (row-standardization). at the end, the sum of the elements
* in all non-empty rows is supposed to yield one. empty rows have no neigbors.
*/
void RowStandardize(Iterator<WMap> mt)  
{
	for ( ; mt; ++mt)  {
            VALUE rowsum = AddUp((*mt)());
            if (rowsum != 0)			// avoid standardizing empty rows
                for (Iterator<WPair> it= (*mt)() ; it; ++it)
                    (*it).second /= rowsum;
	  };
}

/*   RowStandardize
* dense matrix version
*/
void RowStandardize(Iterator<WVector> it)  
{
    const int		dim = it.count();
    for (int row = 0; row < dim; ++row)  {
        double rowSum = Sum( it[row]() );
        if (rowSum != 0)			// avoid empty rows
            for (int col = 0; col < dim; ++col)
                it[row][col] /= rowSum;
    };
}


/*    OnePoly
templated function to compute the logarithm of the polynomial Poly that is stored as
a product of series of polynomials; each polynomial in the series is computed with up
to Pr terms: S = a0 + a1*val + a2*val^2 + ... +aN*val^N, where N=Pr-1.
Poly -- iterator over the polynomial; Val -- value at which the polynomila to be evaluated;
Prec -- max precision in Poly; Pr -- desired precision (number of terms) of the result. 
*/
template <class V>
VALUE OnePoly(Iterator<V> Poly, const VALUE Val, const INDEX Prec, INDEX Pr)  {
    Iterator<V>   it, itr;
    Vector<VALUE> Power(Prec);			// use it to store powers of val: 1, val, val^2, ...
    VALUE         S= 1, LogJ= 0;
    
    while(Power)  {
        Power << S;
        S *= Val;
    };
    
    for ( ; Pr < Prec; ++Pr)
        Power[Pr]= 0;
        
    for (it= Poly; it; ++it)  {
        Iterator<VALUE>       Po= Power();
        S= 0;
        
        for (itr = (*it)(); itr; ++itr, ++Po)
            S += *itr * *Po;
            
        if (S < 1.0e-16)  { 
            S= 1.0e-16;
        };
        LogJ += log(S);
    };
    
    return LogJ;
}

/*   TraceProxy
used to estimate the residual resulted from truncating polynomial
*/
struct TraceProxy 
{
  VALUE S0, S1, Gamma0, Gamma1;
  TraceProxy(const VALUE * Trace, const INDEX Prec);
  TraceProxy(const VALUE a0= 0, const VALUE a1= 0) :
  	S0(a0), S1(a1), Gamma0(0), Gamma1(0)  {};
};

TraceProxy::TraceProxy(const VALUE * Trace, const INDEX Prec)  
{
  S0= -Trace[0];
  if (Trace[0]*Trace[2] <= 0)
  	  {  S0= 0;  Gamma0= 1;  }
  else Gamma0= sqrt(fabs(Trace[0]/Trace[2])) + 0.5/Prec;
  if (Gamma0 > 1)
  	 Gamma0= 1;
  S1= -Trace[1];
  if (Trace[1]*Trace[3] <= 0)  {  S1= 0;  Gamma1= 1;  }
  else  Gamma1= sqrt(fabs(Trace[1]/Trace[3])) + 0.01/Prec;
  if (Gamma1 > Gamma0)
  	 Gamma1= Gamma0;
return;
}


template <class IT>
void CrossConditioning(Iterator<IT> row, VALUE rowSqrtSum, Iterator<VALUE> sqrtSums)  {
    for ( ; row; ++row)  
        (*row).second /= (rowSqrtSum * sqrtSums[(*row).first]);
}
        
/*  MakeSym
* templated function that computes row-standardization of a symmetric matrix and computes
* a symmetric matrix that has the same spectral properties as row-standardized matrix.
*/
//template <class T>
void MakeSym(Iterator<WMap> it)  
{
    const INDEX         dim = it.count();
    Vector<VALUE> 	Sums(dim);
    Iterator<WMap> 	it2= it;
    
		double xx;
    for ( ; it2; ++it2)  {
        VALUE sum = AddUp((*it2)());
				xx = sqrt(sum);
        Sums << xx;
    };
    
    Iterator<VALUE>       is = Sums();
    
    for (it2 = it; it2; ++it2, ++is)  {
            CrossConditioning((*it2)(), *is, Sums());
    };
}

void MakeSym(Iterator<WVector> it)
{
    const int dim = it.count();
    Vector<double> sums(dim);
    for (int row = 0; row < dim; row++) {
		sums[row] = sqrt(Sum(it[row]()));
    }
    for (int row = 0; row < dim; row++) {
		double sr = sums[row];
        for (int col = 0; col < row; col++) {
			double nw = 0;
			double sc = sums[col];
			if (sr != 0 && sc != 0) nw = it[row][col] / (sr*sc);
			it[row][col] = nw;
			it[col][row] = nw;
        }
	}
}

// determines if a gwt structure is symmetric
bool isSym(const GwtElement* g, int obs)
{
	int cnt = 0, cp = 0, ct = 0;
	bool flag = false;
	for (cnt = 0; cnt < obs; cnt++) {
		for (cp = 0; cp < g[cnt].Size(); cp++) {
			flag = false;
			for (ct = 0; ct < g[g[cnt].elt(cp).nbx].Size(); ct++) {
				if (g[g[cnt].elt(cp).nbx].elt(ct).nbx == cnt) {
					if (g[g[cnt].elt(cp).nbx].elt(ct).weight
						== g[cnt].elt(cp).weight) {
						flag = true;
						break;
					}
				}
			}
			if (!flag) return false;
		}
	}
	return true;
}

/*   Estimate
* a function to compute the value of the logarithm of the characteristic polynomial. Uses Aitken's delta-square
* process -- takes three elements in the series to accelerate convergence -- if appropriate.
*/
double Estimate(Iterator<WVector> Poly, const double val, const INDEX Prec, const INDEX np)  {
    double s0, s1, s2, ds;
    // note that all polynomials should be either odd degree or all polys should be of even degree
    s0 = OnePoly(Poly, val, Prec, np-4);
    s1 = OnePoly(Poly, val, Prec, np-2);
    s2 = OnePoly(Poly, val, Prec, np);

    const double ds21= s2-s1, ds10= s1-s0;

    if (fabs(ds21-ds10) < 1e-14) ds = 0;	// if differences are too small, Aitken is meaningless
    else ds = ds21 * ds21 / (ds21-ds10);	// Aitken's formula to compute the correction

    return (s2 - ds);		// result = (prelim value) - (correction)
}

/*   PrimeEstimate
* a function to compute the value of the derivative of the logarithm of the characteristic polynomial.
* Uses Aitken's delta-square process if appropriate.
*/
double PrimeEstimate(Iterator<WVector> Poly, const double val, const INDEX Prec, const INDEX np)  {
  double 	s0, s1, s2, ds;
  s0 = OnePrime(Poly, val, Prec, np- 4);
  s1 = OnePrime(Poly, val, Prec, np- 2);
  s2 = OnePrime(Poly, val, Prec, np);
  const double 	ds21 = s2-s1, ds10 = s1-s0;
  if (fabs(ds21-ds10) < 1e-14) ds= 0;		// bypass Aitken, if differnce is too small
    else ds= ds21 * ds21 / (ds21-ds10);		// Aitken's formula
  return (s2 - ds);
}

/*   MakeEstimate
* computes log-Jacobian by applying Aitken's formula and approximation of the truncated terms for
* row-standardized matrices.
*/
double MakeEstimate(Iterator<WVector> Poly, const double val, const INDEX Prec) 
{
  double 	s0, s1, s2, ds, ns;
  s0= Estimate(Poly, val, Prec, Prec-4);
  s1= Estimate(Poly, val, Prec, Prec-2);
  s2= Estimate(Poly, val, Prec, Prec);
  const double 	v2= val * val;
  const double 	ds21= (s2-s1)*(1+v2*v2/Prec), ds10=s1-s0;
  if (fabs(ds21-ds10) < 1e-14) ds= 0;
    else ds= ds21 * ds21 / (ds21-ds10);
  if (fabs(s2) > 1e-14) ds *= 1.0 + 25.0*pow(val, 40) *(fabs(ds/s2));	// last-term correction
  ns= s2 - ds;
  return ns;
}

/*   MakePrimeEstimate
* computes derivative of log-Jacobian by applying Aitken's formula and approximation of the truncated terms for
* row-standardized matrices.
*/

double MakePrimeEstimate(Iterator<WVector> Poly, const double val, const INDEX Prec) {
  double 	s0, s1, s2, ds, ns;
  s0= PrimeEstimate(Poly, val, Prec, Prec-4);
  s1= PrimeEstimate(Poly, val, Prec, Prec-2);
  s2= PrimeEstimate(Poly, val, Prec, Prec);
  const double 	v2= val*val;
  const double 	ds21= (s2-s1)*(1+0.75*v2*v2/Prec), ds10=s1-s0;
  if (fabs(ds21-ds10) < 1e-14) ds= 0;
    else ds= ds21 * ds21 / (ds21-ds10);
//  if (fabs(s2) > 1e-14) ds *= 1+ 25*pow(val, 40) *(fabs(ds/s2));
  ns= s2 - ds;
  MakeEstimate(Poly, val, Prec);
  return ns;
}



// EasyMatInverse  --
// Inverts any matrix of size 2 by 2 or less. The matrix does not have to be symmetric.
// Returns false if fails (matrix is not a full rank matrix and true if success.
//
bool EasyMatInverse(Iterator<WVector> mt)  {
    CNT       dim;
    dim = mt.count();      // dimension of the problem
    if (dim == 0)  return false;
    if (dim == 1)
        if (fabs(mt[0][0]) < SL_SMALL) return false;
        else mt[0][0]= 1/mt[0][0];       // compute inverse
    else  {       // dim == 2
        VALUE det= mt[0][0] * mt[1][1] - mt[1][0] * mt[0][1];
        if (fabs(det) < SL_SMALL)  return false;
        else  {
            mt[1][0] /= -det;
            mt[0][1] /= -det;
            swap(mt[0][0], mt[1][1]);
            mt[1][1] /= det;
            mt[0][0] /= det;
        };
    };
    return true;
}

/*   ScalProd
* a function used to compute 
* v1[stop] = v1[stop]*t - sum(i=0, stop-1; v1[i]*v2[i]) * t
* in the context of LDL decomposition
*/
void ScalProd(const CNT stop, Iterator<VALUE> v1, Iterator<VALUE> v2, const VALUE t= 1)  {
    VALUE      ValueAtStop= v1[ stop ];
    for (CNT elt= 0; elt < stop; ++elt)
        ValueAtStop -= v1[ elt ] * v2[ elt ];
    v1[stop] = ValueAtStop * t;
}

/*   ScalProd1
* one of the functions for computing LDL decomposition
*/
void ScalProd1(const CNT start, const CNT stop, Iterator<VALUE> v1, Iterator<VALUE> v2)  {
    VALUE     ValueAtStart = v1[ start ];
    for (CNT elt= start+1; elt < stop; ++elt)
        ValueAtStart += v1[ elt ] * v2[ elt ];
    v2[ stop ] = - ValueAtStart;
}

/*   ScalProd2
* one of the functions for computing LDL decomposition
*/
void ScalProd2(const CNT start, const CNT stop, Iterator<VALUE> v1, Iterator<VALUE> v2,
               Iterator<VALUE> diag, const CNT first)  
{
    VALUE     ValueAtStart = diag[ start ];
    if (start != first)
        ValueAtStart *= v2[ start ];
    for (CNT elt= start+1; elt < stop; ++elt)
        ValueAtStart += diag[ elt ] * v1[ elt ] * v2[ elt ];
    if (fabs(ValueAtStart) < SL_SMALL)  ValueAtStart= 0;
    v1[ first ] = ValueAtStart;
}

// SymMatInverse --
// Inverts a symmetric positive or negative definite matrix of any size in situ.
// Returns false if fails (matrix is not a full rank matrix and true if success.
// Uses analog of Choleski decomposition: L-D-L' decomposition.
bool SymMatInverse(Iterator<WVector> mt)  {
    CNT       row, column, dim= mt.count();
    if (dim == 0) return 0;
    if (dim <= 2) return EasyMatInverse(mt);
    WVector   diag(dim, 0.0);
    VALUE     current;
    
// 1. compute L , D . NOTE: L has ones on the main diagonal.
    for (row= 0; row < dim; ++row)  {
        for (column= 0; column < row; ++column)
            diag[column] = mt[column][column] * mt[row][column];
        ScalProd(row, mt[row](), diag());
        current = mt[row][row];
        if (fabs(current) < SL_SMALL)  return 0;	// (near) singularity
        current = 1.0/current;
        for (column= row+1; column < dim; ++column)
            ScalProd(row, mt[column](), diag(), current);
    };

    
// 2. compute inv(L')
    for (row= 1; row < dim; ++row)
        for(column= 0; column < row; ++column)
            ScalProd1(column, row, mt[row](), mt[column]());
    
// 3. compute inv(D)
    for (row= 0; row < dim; ++row)  {
        VALUE       current= mt[row][row];
        diag[row]= fabs(current) < SL_SMALL ? 0 : 1/current;
    };
    
// 4. put the pieces together: inv(A)= inv(L')*inv(D)*inv(L)
// 4.a do the lower triangular part of A
    for (row= 0; row < dim; ++row)
        for (column= row; column < dim; ++column)
            ScalProd2(column, dim, mt[column](), mt[row](), diag(), row);
// 4.b  do the upper triangular part of A
    for (row= 1; row < dim; ++row)
        for (column= 0; column < row; ++column)
            mt[column][row] = mt[row][column];
    
    return true;
}

// computes OLS
// y -- dependent variable;
// X -- independednt variables;
// IncludeConst -- 'true' if include intercept;
// Inverse -- inv(X'X);
// resid -- residuals;
// ols -- coefficients.
/*bool OLS(Iterator<VALUE> y, 
				 Iterator<WVector> X, 
				 const bool IncludeConst, 
				 Vector<WVector> &cov,
         WVector &resid, WVector &ols)  
{
    CNT   vars= X.count();                // number of independent variables
    if (vars == 0) return false;
    CNT   obs= X[0].count();              // number of observations
    CNT   varsIncl= IncludeConst ? vars+1 : vars, row, column;
    cov.alloc(varsIncl);
    VALUE     val;
    for (row= 0; row < varsIncl; ++row, ++cov)
        (*cov).alloc(varsIncl, 0);
    for (row= 0; row < vars; ++row)
        for (column= row; column < vars; ++column)
            val= cov[row][column] = cov[column][row] = Product(X[row](), X[column]());
    if (IncludeConst)  {                  // include intercept
        cov[vars][vars]= obs;
        for (column= 0; column < vars; ++column)
            val= cov[vars][column] = cov[column][vars] = Sum(X[column]());
    };
    if (SymMatInverse(cov()))  
		{
        resid.alloc(obs);
        ols.alloc(varsIncl);
        WVector     temp(varsIncl);
        for (row= 0; row < vars; ++row)
            temp << Product(X[row](), y);
        if (IncludeConst)
            temp << Sum(y);                           // temp has X'y
        for (row= 0; row < varsIncl; ++row)
            ols << Product(cov[row](), temp());       // compute ols
        for (column= 0; column < obs; ++column)  
				{
            VALUE tempr = y[column];
            if (IncludeConst) tempr -= ols[vars];
            for (row= 0; row < vars; ++row)
                tempr -= ols[row] * X[row][column];
            resid << tempr;                           // compute residuals
        };
    };
    return true;
}*/


/*   computeRotation
* computes rotation elements
*/

void computeRotation(const double v, const double t, double &c, double &s, double &rotation)  {
    double	r;
    if (fabs(v) < fabs(t))  {
        s = v / t;
        r = sqrt(1.0 + geoda_sqr(s));
        rotation = r * t;
        s *= (c = 1.0 / r);
    }  else  {
        c = t / v;
        r = sqrt(1.0 + geoda_sqr(c));
        rotation = r * v;
        c *= (s = 1.0 / r);
    };
}

/*   Rotation
*  function to compute Givens rotation
*/

inline void Rotation(const double &c, const double &s, double &t, double &v)  {
    const double	x = t;
    t = c * x + s * v;
    v = s * x + c * v;
}


/*    QL_with_Shift
* an implementation of QL with shift algorithm for computing eigenvalues
* of a real symmetric tridiagonal matrix
*   see Numerical Recipes in C, pp. 380-381.
* input:
* D -- diagonal elements;
* E -- subdiagonal elements
* output:
* D -- the eigenvalues.
*/
void QL_with_Shift(double * d, double * e, const int dim)  {
    const int LIMIT = 30;			// limit on the number of iterations
    int		m, iter;
    double	s, c, root, g, p;
    for (int row = 0; row < dim; ++row)  {
        iter = 0;
        do  {
            for (m = row; m < dim-1; ++m)  {
                double t = fabs( d[m] ) + fabs( d[m+1] );
                if (fabs( e[m] ) + t == t)  break;
            };
            if (m != row)  {
                if (++iter == LIMIT)  {
                };
                g = (d[row+1] - d[row]) / (2.0 * e[row]);
                root = sqrt(geoda_sqr(g) + 1.0);
                if (g < 0) root = -root;
                g = d[m] - d[row] + e[row] / (g + root);
                s = c = 1.0;
                p = 0;
                for (int cp = m-1; cp >= row; --cp)  {
                    double t = c * e[cp];
                    computeRotation( g, s*e[cp], s, c, e[cp+1] );
                    g = d[cp+1] - p;
                    root = (d[cp] - g) * s + 2.0 * c * t;
                    p = s * root;
                    d[cp+1] = g + p;
                    g = c * root - t;
                };
                d[row] -= p;
                e[row] = g;
                e[m] = 0;
            };
        }  while (m != row);
    };
}


double normalize(WIterator arow, double & OD, const int row)  
{
    const int		last = row - 1;
    double		h = 0, scale = 0;
	int cnt = 0;
    for (cnt = 0; cnt < row; ++cnt)
        scale += fabs( arow[cnt] );

    if (scale == 0.0 || last == 0)  {		// skip the transformation
        OD = arow[ last ];		// off-diagonal is either 0 or arow[0]
        return 0;
    };
    for (cnt = 0; cnt < row; ++cnt)
        arow[ cnt ] /= scale;
    for (cnt = 0; cnt < row; ++cnt)
        h += geoda_sqr( arow[cnt] );
    
    double	f = arow[ last ];			// pretransform pivoting element -- ofddiagonal
    double	g = (f > 0) ? -sqrt(h) : sqrt(h);	// adjust to get the same norm
    OD = scale * g;					// new offdiagonal element
    h -= f * g;
    arow[last] = f - g;
    return h;
}


/*    householder
* implements Householder's method for reducing real symmetric matrix to 3D
*/
void householder(Iterator<WVector> a, double * diag, double * off)  
{
    const int	dim = a.count();
    int row = 0, j = 0, cp = 0;
    double *	w = new double[dim];
    for (row = dim-1; row > 0; --row)  
		{
        double scale = normalize(a[row](), off[row-1], row);
        if (scale > 0)  
				{
            double 	f = 0;
            for (j = 0; j < row; ++j)  
						{
                double g = 0;
                for (cp = 0; cp <= j; ++cp)
                    g += a[j][cp] * a[row][cp];
                for (cp = j+1; cp < row; ++cp)
                    g += a[cp][j] * a[row][cp];
                w[j] = g / scale;
                f += w[j] * a[row][j];
            };
            double	hh = f / (scale + scale);
            for (j = 0; j < row; ++j)  
						{
                f = a[row][j];
                w[j] -= hh * f;
                double g = w[j];
                for (cp = 0; cp <= j; ++cp)
                    a[j][cp] -= (f * w[cp] + g * a[row][cp]);
            };
        };
    };

    for (int cnt = 0; cnt < dim; ++cnt)
        diag[ cnt ] = a[ cnt ][ cnt ];

	delete [] w;
	w = NULL;
}

/*   SpatialLag
* function to compute spatial lag as a sparse-matrix/dense-vector product.
* optionally, might perform row-standardization of the spatial weights, but the matrix
* remains intact.
*/
void SpatialLag(Iterator<WMap> mt, const WIterator v, WVector &lag, bool need_std = false)  {
    lag.reset();
    for ( ; mt; ++mt)  {
        double lag_value = Product( (*mt)(), v);
        if (need_std) lag_value /= AddUp( (*mt)() );
        lag << lag_value;
    };
}

/*   SpatialLag
* function to compute spatial lag as a dense-matrix/dense-vector product.
* optionally, might perform row-standardization of the spatial weights, but the matrix
* remains intact.
*/
void SpatialLag(Iterator<WVector> mt, const WIterator v, WVector &lag, bool need_std = false)  {
    lag.reset();
    for ( ; mt; ++mt)  {
        double lag_value = Product( (*mt)(), v);
        if (need_std) lag_value /= Sum( (*mt)() );
        lag << lag_value;
    };
}


//*** compute spatial lag of a series of variables organized in a matrix.
//*** In such a matrix, the variables are stored in columns: var[0] is 1st variable, var[1] is the second, ...

void SpatialLag(Iterator<WMap> mt, Iterator<WVector> var, WMatrix &lag)  {
    if (lag.count() != var.count())  {			// make sure the number of variables is correct
        lag = new WMatrix(var.count());
        lag.reset(var.count());
    };   
     
    for (int cnt = 0; cnt < (int) var.count(); ++cnt)	 {	// make sure each variable is correct
        if (lag[cnt].count() != var[cnt].count())  
            lag[cnt] = new WVector(var[cnt].count());
        lag[cnt].reset();
    };        

    for ( ; mt; ++mt)					// compute spatial lag: treat each 
        for (int cnt = 0; cnt < (int) var.count(); ++cnt)  
            lag[cnt] << Product( (*mt)(), var[cnt]() );
}

void SpatialLag(Iterator<WVector> mt, Iterator<WVector> var, WMatrix &lag)  {
    if (lag.count() != var.count())  {			// make sure the number of variables is correct
        lag = new WMatrix(var.count());
        lag.reset(var.count());
    };   

    for (int cnt = 0; cnt < (int) var.count(); ++cnt)	 {	// make sure each variable is correct
        if (lag[cnt].count() != var[cnt].count())  
            lag[cnt] = new WVector(var[cnt].count());
        lag[cnt].reset();
    };        

    for ( ; mt; ++mt)					// compute spatial lag: treat each 
        for (int cnt = 0; cnt < (int) var.count(); ++cnt)  
            lag[cnt] << Product( (*mt)(), var[cnt]() );
}


void copy(GWT &dst, Iterator<WMap> from)  {
    const size_t        dim= from.count();
    if (dim)  {
        dst.alloc(dim);
        do  {
            (*dst).copy((*from)());
            dst++;
            from++;
        }  while (from);
    };
}

void copy(WMatrix &dst, Iterator<WVector> src)  {
    const int		dim = src.count();
    if (dim > 0)  {
        dst.alloc(dim);
        for (int row = 0; row < dim; ++row)
            dst[row].copy(src[row]());
        dst.reset(dim);
    };
}

/*    norm
* function to compute sum of squares of a vector.
*/
VALUE norm(WIterator it)  {
    double scale = 0.0, ssq = 1.0, t;
    for ( ; it; ++it)  {
        double absc = fabs(*it);
        if (scale < absc)  {
            t = scale / absc;
            ssq = 1.0 + ssq * t * t;
            scale = absc;
        }  else  {
            t = absc / scale;
            ssq += t * t;
        };        
    };
    return scale * scale * ssq;
}    

/*   CL
* function to compute log-likelihood function for the spatial lag model
resid -- vector of residuals in regression y on X;
residW -- vector or residulas in regression of Wy on X;
rho -- value of the coefficient of spatial association.
Note: function uses static variables Poly and SL_Max_Precision.
*/
VALUE   CL(WVector & resid, WVector & residW, const VALUE rho)  {
    VALUE     lj = MakeEstimate(Poly(), rho, SL_Max_Precision);	// compute log-Jacobian
    WVector   tmp;
    tmp.reset();
    tmp.copy(residW());       // copy residiual of wy on X
    tmp *= rho;               // multiply it by rho
    tmp -= resid;             // subtract residual of y on X
    double    nn = resid.count(),
    product = norm(tmp());
    
		double sse = 0.5 * nn * log(product/nn);


    double sse1 = -1.0*((nn/2.0)*(log(2.0*M_PI))+(nn/2.0)*log((product/nn))+(product/(2.0*(product/nn))));

    double accum = lj - sse;
    double accum1 = lj - sse1;
    return accum;
}

/* _minus
* function to compute r = a -b *s;
* a, b, s -- vectors;
* s -- scalar.
*/
void _minus(WIterator a, WIterator b, WVector &r, const VALUE s)  
{
    if (a.count() != r.count())  
        r.alloc(a.count());
    r.reset();    
    for ( ; a; ++a, ++b)
            r << (*a - *b * s);
        
}

void _minus(Iterator<WVector> a, Iterator<WVector> b, WMatrix &m, const VALUE s)  {
    if (a.count() != m.count())  {			// make sure the number of variables is correct
        m.alloc(a.count());
        m.reset(a.count());
    };   
	int cnt = 0;
    for (cnt = 0; cnt < (int) a.count(); ++cnt)  {		// make sure each variable has correct size
        if (m[cnt].count() != a[cnt].count())
            m[cnt] = new WVector(a[cnt].count());
        m[cnt].reset();
    };        
            
    for (cnt = 0; cnt < (int) a.count(); ++cnt)  
            for (int obs = 0; obs < (int) a[cnt].count(); ++obs)
                m[cnt] << (a[cnt][obs] - b[cnt][obs] * s);
}


inline void SHFT(double &a, double &b, double &c, const double d)  {
	a = b;
	b = c;
	c = d;
}

VALUE ErrorLogLikelihood(Iterator<WVector> X, Iterator<WVector> lagX, WIterator y, WIterator lagY, Iterator<WMap> W, const VALUE lambda, WVector &egls)  {
    // compute log-Jacobian: SIGMA(ln(1 - lambda * eigenval(i)) ...
    VALUE accum = MakeEstimate(Poly(), lambda, SL_Max_Precision);

    // compute sse (sum-squared error)
    WMatrix XminusLambdaLagX(X.count());
    WVector YminusLambdaLagY(y.count());

    _minus(X, lagX, XminusLambdaLagX, lambda);
    _minus(y, lagY, YminusLambdaLagY, lambda);

	int row = 0, column = 0, deps = X.count(), dim = y.count();
	double **cov = new double * [deps], *resid = new double [dim];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	DenseVector p_y(dim), *p_x = new DenseVector [deps], eg(deps);
	for (row = 0; row < dim; row++) {
		p_y.setAt(row, YminusLambdaLagY[row]);
	}
	for (row = 0; row < deps; row++) {
		p_x[row].alloc(dim);
		for (column = 0; column < dim; column++) {
			p_x[row].setAt(column, XminusLambdaLagX[row][column]);
		}
	}
    ordinaryLS( p_y, p_x, cov, resid, eg );
    for (row = 0; row < deps; row++) egls[row] = eg.getValue(row);
    
    WVector rs(y.count(), 0), lagResid(y.count(), 0), RminusLambdaLagR(y.count(), 0);
    for (row = 0; row < dim; row++) rs[row] = resid[row];
    _minus(rs(), lagResid(), RminusLambdaLagR, lambda);
    
    VALUE sse = norm(RminusLambdaLagR());		

    // This is [-N/2 * ln(u'u/N)]
    VALUE addOn = -0.5 * y.count() * log(sse/y.count());

    // See Anselin - Bera (equation 37) pg.258
    return (accum + addOn);
}  

inline double findQuadMax(double x0, double f0, double x1, double f1, double x2, double f2)  {
    const double s1 = (x0 - x1) * (f2 - f1), s2 = (x2 - x1) * (f0 - f1);
    const double denominator = 2.0 * (s1 - s2);
    double value;
    if (fabs(denominator) == 0)
        value = x1;

    else  {
            double numerator = s1 * (x0 - x1) - s2 * (x2 - x1);
            value = x1 + numerator/denominator;
    };    

    return value;  
}

VALUE GoldenSectionError(const VALUE left, 
												 const VALUE middle, 
												 const VALUE right, 
												 const WMatrix &X, 
												 const WVector &y,
                         Iterator<WMap> W, 
												 double * &beta,
												 double * LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
  	x2 += GoldenToo * (right - middle);
    else
  	x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    WMatrix lagX;
    WVector lagY(X[0].count()), egls(X.count());
    SpatialLag(W, X(), lagX);
    SpatialLag(W, y(), lagY);

    // maximizing
    f2 = ErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls);
    f1 = ErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls);
    

//  this is 'classic' golden section
//  and tol is defined as  1e-14
  while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  
	{
  	if (f1 < f2)  {
  		SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
  		SHFT(f0, f1, f2, ErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls));
  	}  else  {
  		SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
  		SHFT(f3, f2, f1, ErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls));
  	}
  	++Counter;
  }
  if (f2 > f1)  {
  	f1= f2;
  	x1= x2;
  };

  // The Log-likelihood
  const double n = W.count();
  *LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);

  beta = new double[ X[0].count() ];
  for (int cnt = 0; cnt < (int) egls.count(); ++cnt)
      beta[cnt] = egls[cnt];
  return  x1;
}

VALUE GoldenSectionLag(const VALUE left, 
											 const VALUE middle, 
											 const VALUE right, 
											 WVector &resid, 
											 WVector &residW,
											 double* LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
  	x2 += GoldenToo * (right - middle);
    else
  	x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    f2 = CL(resid, residW, x2);
    f1 = CL(resid, residW, x1);
  while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  
	{
  	if (f1 < f2)  {
  		SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
  		SHFT(f0, f1, f2, CL(resid, residW, x2));
  	}  else  {
  		SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
  		SHFT(f3, f2, f1, CL(resid, residW, x1));
  	};
  	++Counter;
  };
  if (f2 > f1)  
	{
  	f1= f2;
  	x1= x2;
  };

  const double n = resid.count();
	*LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);
	
  return  x1;
}


VALUE GoldenSection(const VALUE left, const VALUE middle, const VALUE right, WVector &resid, WVector &residW)  
{
  const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
  VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
  x0= left;
  x3= right;
  x1= x2= middle;
  if (fabs(right-middle) > fabs(middle-left))
  	x2 += GoldenToo * (right - middle);
  else
  	x1 -= GoldenToo * (middle - left);
  int   Counter = 2;
  f2 = CL(resid, residW, x2);
  f1 = CL(resid, residW, x1);
  while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  
	{
  	if (f1 < f2)  {
  		SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
  		SHFT(f0, f1, f2, CL(resid, residW, x2));
  	}  else  {
  		SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
  		SHFT(f3, f2, f1, CL(resid, residW, x1));
  	};
  	++Counter;
  };
  if (f2 > f1)  
	{
  	f1= f2;
  	x1= x2;
  };

  return  x1;
}

double ComputeExtreme(const double d10, const double d12, const double f10, const double f12, const double x1)  {
   double denominator= d10 * f12 - d12 * f10;
   if (denominator < 1e-14) return x1;
   double numerator = d10 * d10 * f12 - d12 * d12 * f10;
   return  (x1 + numerator / denominator / 2);
 }

VALUE Converge(const VALUE left, const VALUE middle, const VALUE right, WVector &resid, WVector &residW)  {
    VALUE	x0, x1, x2, f0, f1, f2, dx, xn;
    x0= left;
    x1= middle;
    x2= right;
    f0= CL(resid, residW, x0);
    f1= CL(resid, residW, x1);
  f2= CL(resid, residW, x2);
  xn= ComputeExtreme(x1-x0, x1-x2, f1-f0, f1-f2, x1);
  if (x1 == xn)  {
    return 0;
  };

  if (xn > x1)  {
    dx= x1-x0;
    x0= x1;   x1= xn;
    f0= f1;
    f1= CL(resid, residW, x1);
  }  else  if (xn < x1)  {
    x2= x1; x1= xn;
    f2= f1;
  }  else  {
  };

  return xn;
}


#include "SparseVector.h"
#include "DenseVector.h"
#include "Link.h"
#include "SparseRow.h"
#include "SparseMatrix.h"
#include "DenseMatrix.h"

inline void skipTillNumber(ifstream &f)  
{
    char ch;
    while (f >> ch)
        if ( (ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch == '+')
            break;
    if (f) f.putback(ch);
}

int readInt(ifstream &f)  
{
    skipTillNumber(f);
    int result;
    f >> result;
    return result;
}

double readDouble(ifstream &f)  
{
    skipTillNumber(f);
    double result;
    f >> result;
    return result;
}

void extract(const SparseVector &v, 
						 const double *scale, 
						 const int row, 
						 double &trace, 
						 double &trace2, 
						 double &frobenius)  
{
    double t2 = 0, fr = 0, t;
    double scale2 = 0, ssq2 = 1, scalef = 0, ssqf = 1;
    for (int cnt = 0; cnt < v.getNzEntries(); ++cnt)  {
        int ix = v.getIx( cnt );
        double val = fabs( v.getValue( ix ) );
        if (scale2 < val)  {
            t = scale2 / val;
            ssq2 = 1.0 + ssq2 * t * t;
            scale2 = val;
        }  else if (val > 0.0)  {
            t = val / scale2;
            ssq2 += t * t;
        };
        double vf = val / scale[ix];
        if (scalef < vf)  {
            t = scalef / vf;
            ssqf = 1.0 + ssqf * t * t;
            scalef = vf;
        }  else if (vf > 0.0)  {
            t = vf / scalef;
            ssqf += t * t;
        };
//        t2 += geoda_sqr(val);
//        fr += geoda_sqr( val / scale[ix] );
    };
    t2 = scale2 * scale2 * ssq2;
    fr = scalef * scalef * ssqf;

    trace += v.getValue(row);
    trace2 += t2;
    frobenius += fr * geoda_sqr(scale[row]);
}


void extract(const DenseVector &v, const double *scale, const int row, double &trace, double &trace2, double &frobenius)  {
    double t2 = 0, fr =0;
    for (int cnt = 0; cnt < v.getSize(); ++cnt)  {
        t2 += geoda_sqr(v.getValue(cnt));
        fr += geoda_sqr(v.getValue(cnt) / scale[cnt]);
    };
    trace += v.getValue(row);
    trace2 += t2;
    frobenius += fr * geoda_sqr(scale[row]);
}

// EasyMatInverse  --
// Inverts any matrix of size 2 by 2 or less. The matrix does not have to be symmetric.
// Returns false if fails (matrix is not a full rank matrix and true if success.
//

bool EasyMatInverse(double ** mt, const int dim)  {
    if (dim == 0)  return false;
    if (dim == 1)
        if (fabs(mt[0][0]) < ML_SMALL) return false;
        else mt[0][0]= 1.0 / mt[0][0];       // compute inverse
    else  {       // dim == 2
        double det= mt[0][0] * mt[1][1] - mt[1][0] * mt[0][1];
        if (fabs(det) < ML_SMALL)  return false;
        else  {
            mt[1][0] /= -det;
            mt[0][1] /= -det;
            swap(mt[0][0], mt[1][1]);
            mt[1][1] /= det;
            mt[0][0] /= det;
        };
    };
    return true;
}

void ScalProd(const int stop1, double * v1, double * v2, const double t = 1)  {
    double      ValueAtStop= v1[ stop1 ];
    for (int elt= 0; elt < stop1; ++elt)
        ValueAtStop -= v1[ elt ] * v2[ elt ];
    v1[stop1] = ValueAtStop * t;
    return;
}

void ScalProd(const int stop1, double ** v1, double * v2, const double t = 1)  {
    double      ValueAtStop= v1[stop1][ stop1 ];
    for (int elt= 0; elt < stop1; ++elt)
        ValueAtStop -= v1[stop1][ elt ] * v2[ elt ];
    v1[stop1][stop1] = ValueAtStop * t;
    return;
}


void ScalProd1(const int start1, const int stop1, double * v1, double * v2)  
{
    double     ValueAtStart= v1[ start1 ];
    for (int elt = start1+1; elt < stop1; ++elt)
        ValueAtStart += v1[ elt ] * v2[ elt ];
    v2[ stop1 ] = - ValueAtStart;
    return;
}

void ScalProd2(const int start1, const int stop1, double * v1, double * v2,
               double * diag, const int first)  
{

    double     ValueAtStart = diag[ start1 ];
    if (start1 != first) 
			ValueAtStart = ValueAtStart * v2[ start1 ];

    for (int elt = start1+1; elt < stop1; ++elt)
        ValueAtStart += (diag[ elt ] * v1[ elt ] * v2[ elt ]);

    if (fabs(ValueAtStart) < ML_SMALL)  ValueAtStart= 0;
    v1[ first ] = ValueAtStart;

}

void CopyMatrix(double **m, double ** &a, int dim1, int dim2)
{
	typedef double* double_ptr_type;
	a = new double_ptr_type[dim1];
	for (int i=0;i<dim1;i++)
	{
		a[i] = new double[dim2];
		for (int j=0;j<dim2;j++)
			a[i][j] = m[i][j];
	}
}
	
void CopyVector(double * &a, double *b, long n)  
{
    a = new double[n];
    for (int cnt = 0; cnt < n; ++cnt) 
			a[cnt] = b[cnt];
}


double sum(int stop, double v1[], double diag[])  {
    double s = v1[stop];
    for (int cnt = 0; cnt < stop; ++cnt)
        s -= geoda_sqr(v1[cnt]) * diag[cnt];
    return s;
}

void makeSum2(int stop, double v1[], double v2[], double diag[])  {
    double s = v1[stop];
    for (int cnt = 0; cnt < stop; ++cnt)
        s -= v1[cnt] * v2[cnt] * diag[cnt];
    v1[stop] = s / diag[stop];
}

void product1(int start, int stop, double v1[], double v2[])  {
    double valueAtStart = v1[start];
    for (int cnt = start+1; cnt < stop; ++cnt)
        valueAtStart += v1[cnt] * v2[cnt];
    v2[stop] = -valueAtStart;
}

void product2(int start, double v1[], double v2[], int first, double diag[], int dim)  {
    double valueAtStart = diag[start];
    if (start != first) valueAtStart *= v2[start];
    for (int cnt = start+1; cnt < dim; ++cnt)
        valueAtStart += diag[cnt] * v1[cnt] * v2[cnt];
    v1[first] = (fabs(valueAtStart) < ML_SMALL) ? 0 : valueAtStart;
}

// SymMatInverse --
// Inverts a symmetric matrix of any size in situ.
// Returns false if fails (matrix is not a full rank matrix and true if success.
// Uses analog of Choleski decomposition: L-D-L' decomposition.
bool SymMatInverse(double ** mt, const int dim)  
{
	long int n = dim, nrhs = dim;
	long int lda = dim, ldb = dim;
	long int *ipiv = new long int [dim], info = 0;
	int i = 0, j = 0;
	double *a = new double [dim * dim];
	double *b = new double [dim * dim];
	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			a[i + dim * j] = mt[i][j];
			b[i + dim * j] = 0.0;
		}
		b[i + dim * i] = 1.0;
	}

// use __WXMAC__ to call vecLib format
//#ifdef WORDS_BIGENDIAN
//#ifdef __WXMAC__
	dgesv_(&n, &nrhs, a, &lda, ipiv, b, &ldb, &info);
//#else
//	dgesv_((integer*)&n, (integer*)&nrhs, (doublereal*)a, (integer*)&lda, (integer*)ipiv, (doublereal*)b, (integer*)&ldb, (integer*)&info);
//#endif

	if (!info) {
		for (i = 0; i < dim; i++) {
			for (j = 0; j < dim; j++) {
				mt[i][j] = b[i + dim * j];
			}
		}
		return true;
	} else {
		return false;
	}
}

bool SymMatInverse(double ** mt, const int dim, double ** &cov)  
{
		CopyMatrix(mt, cov, dim,dim);
	long int n = dim, nrhs = dim;
	long int lda = dim, ldb = dim;
	long int *ipiv = new long int [dim], info = 0;
	int i = 0, j = 0;
	double *a = new double [dim * dim];
	double *b = new double [dim * dim];
	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			a[i + dim * j] = mt[i][j];
			b[i + dim * j] = 0.0;
		}
		b[i + dim * i] = 1.0;
	}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
	dgesv_(&n, &nrhs, a, &lda, ipiv, b, &ldb, &info);
#else
	dgesv_((integer*)&n, (integer*)&nrhs, (doublereal*)a, (integer*)&lda, (integer*)ipiv, (doublereal*)b, (integer*)&ldb, (integer*)&info);
#endif

	if (!info) {
		for (i = 0; i < dim; i++) {
			for (j = 0; j < dim; j++) {
				mt[i][j] = b[i + dim * j];
			}
		}
		return true;
	} else {
		return false;
	}
}

void residual(const DenseVector &rhs, const DenseVector * X, const DenseVector &ols, DenseVector &resid)  
{
    const int dim = rhs.getSize(), vars = ols.getSize();
    resid.alloc(dim);
    resid.copy(rhs);
    for (int cnt = 0; cnt < vars; ++cnt)
        resid.addTimes(X[cnt], -ols.getValue(cnt));
}
extern void PrintCovariance(double **,int);

// computes OLS and return the inv cov
// y -- dependent variable;
// X -- independednt variables;
// IncludeConst -- 'true' if include intercept;
// Inverse -- inv(X'X);
// resid -- residuals;
// ols -- coefficients.
/*bool OLS(DenseVector &y, 
				 DenseVector * X, 
				 const bool IncludeConst, 
				 double ** &cov, 
				 double * resid, 
				 DenseVector &ols)  
{


    const int vars = ols.getSize();	//       vars -- number of independent variables

    
		if (vars == 0) return false;
    int					obs= y.getSize();              // number of observations
    int					varsIncl = vars, row, column;
    double			val;

		
    cov = new double* [varsIncl];
    for (row= 0; row < varsIncl; ++row)
//			cov[row] = new double[varsIncl];
        alloc(cov[row], varsIncl);		// allocate cov[row] & initialize to zero

    for (row= 0; row < vars; ++row)  
		{
        for (column = row+1; column < vars; ++column)
            val= cov[row][column] = cov[column][row] = X[row].product(X[column]);
        cov[row][row] = X[row].norm();
    };

		
    if (SymMatInverse(cov, varsIncl) == false)  
			return false;


    double *temp = new double[ varsIncl ];
    for (row= 0; row < vars; ++row)
        temp[row] = y.product(X[row]);
    
		for (row = 0; row < varsIncl; ++row)
        ols.setAt( row, product(cov[row], temp, varsIncl) );       // compute ols

    for (column= 0; column < obs; ++column)  
		{
        double tempr = y.getValue(column);
        for (row= 0; row < vars; ++row)
				{
            tempr -= ols.getValue(row) * X[row].getValue(column);
				}
        resid[column] = tempr;                           // compute residuals
    };

    release(&temp);
    return true;
}*/

// computes OLS and return the inv cov
// y -- dependent variable;
// X -- independednt variables;
// IncludeConst -- 'true' if include intercept;
// Inverse -- inv(X'X);
// resid -- residuals;
// ols -- coefficients.
bool ordinaryLS(DenseVector &y, 
				 DenseVector * X, 
				 double ** &cov, 
				 double * resid, 
				 DenseVector &ols)  
{
	const int vars = ols.getSize();	// vars -- number of independent variables

	if (vars == 0) return false;
	int obs = y.getSize(); // number of observations
	int row = 0, column = 0;
	double val = 0.0;

	for (row = 0; row < vars; row++) {
		for (column = 0; column < vars; column++) {
			cov[row][column] = 0;
		}
	}

	// use SVD to calculate matrix inversion
	// use dgesvd_

	double *temp = new double [vars];
	for (row = 0; row < vars; row++)
		temp[row] = y.product(X[row]);

	char jobu = 'S', jobvt = 'S';
	long int m = obs, n = vars;
	long int lda = obs, ldu = obs, ldvt = vars, lwork = 5 * obs, info = 0;
	double *a = new double [obs * vars];
	double *s = new double [vars];
	double *u = new double [ldu * vars];
	double *vt = new double [ldvt * vars];
	double *work = new double [lwork];
	for (row = 0; row < obs; row++) {
		for (column = 0; column < vars; column++) {
			a[row + obs * column] = X[column].getValue(row);
		}
	}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__MMM
	dgesvd_(&jobu, &jobvt, &m, &n, a,
			&lda, s, u, &ldu,
			vt, &ldvt, work,
			&lwork, &info);
#else
	dgesvd_(&jobu, &jobvt, (integer*)&m, (integer*)&n, (doublereal*)a,
			(integer*)&lda, (doublereal*)s, (doublereal*)u, (integer*)&ldu,
			(doublereal*)vt, (integer*)&ldvt, (doublereal*)work,
			(integer*)&lwork, (integer*)&info);
#endif

	if (!info) {
		// (X'X)^(-1) = VW^(-2)V'
		for (row = 0; row < vars; row++) {
			for (column = 0; column < vars; column++) {
				for (m = 0; m < vars; m++) {
					cov[row][column] += (vt[row * vars + m] * vt[m + column * vars]) / (s[m] * s[m]);
				}
			}
		}

		for (row = 0; row < vars; row++) {
			val = 0.0;
			for (column = 0; column < vars; column++) {
				val += cov[row][column] * temp[column];
			}
			ols.setAt(row, val);
		}
	} else {
		release(&temp);
		return false;
	}

	for (column = 0; column < obs; column++) {
		double tempr = y.getValue(column);
		for (row = 0; row < vars; row++) {
			tempr -= ols.getValue(row) * X[row].getValue(column);
		}
		resid[column] = tempr; // compute residuals
	}

	release(&temp);
	return true;
}


void cg(const SparseMatrix &m, const double rho, const DenseVector &rhs,
		DenseVector &sol)  
{
    const int LIMIT = 50;
    double n, beta, n_lag;

    DenseVector		resid( m.dim() ), p( m.dim() ), d( m.dim() );

    sol.copy( rhs );
    m.IminusRhoThis( rho, sol, p );		// p = (I-rho*M)*sol
    resid.minus( rhs, p );			// residuals
    n = resid.norm();				// norm residuals

    int it = 0;
    while (n > 1.0e-16 && it++ < LIMIT)  {
        if (it == 1) {
			d.copy(resid); 
		} else  {
            beta = n / n_lag;
            d.timesPlus( resid, beta );
        }
        m.IminusRhoThis( rho, d, p );		// p = (I-rho*W)*d
        double alpha = n / d.product( p );	// alpha = n / d'p
        sol.addTimes( d, alpha );		// sol += alpha*d
        resid.addTimes(p, -alpha );		// resid -= alpha*p
        n_lag = n;
        n = resid.norm();
	}
}

double log_likelihood(double ss, int n)  
{
    double nn = 1.0 * n;
//    return -0.5 * nn * log(ss / nn);


    return -1.0*((nn/2.0)*(log(2.0*M_PI))+(nn/2.0)*log((ss/nn))+(ss/(2.0*(ss/nn))));


}

double Lc(const double * resid, const double * residW, const SparseMatrix &w, const double rho, DenseVector &sol,
          const DenseVector &y, const DenseVector &lag)  {
    const int dim = w.dim();
    DenseVector 	rhs(dim);
    double ss, ssSol, ssRhs, s;
	int cnt = 0;
    if (rho == 0)  {  ss = norm(resid, dim);  return log_likelihood(ss, dim);  }
        else  {
            for (cnt = 0; cnt < dim; ++cnt)  {
                rhs.setAt( cnt, (resid[cnt] - residW[cnt] * rho) *w.getScale()[cnt] );
//                rhs.setAt( cnt, resid[cnt] - residW[cnt] * rho );
            };
            w.IminusRhoThis( rho, rhs, sol );
//            cg(w, rho, rhs, sol);
            ssSol = sol.norm();
            ss = rhs.norm();
            for (cnt = 0; cnt < dim; ++cnt)  {
                sol.setAt( cnt, sol.getValue(cnt) / w.getScale()[cnt] );
                rhs.setAt( cnt, rhs.getValue(cnt) / w.getScale()[cnt] );
            };
            ssRhs = rhs.norm();
            s = sol.norm();
            double p =0, pr = 0, lalpha;
            for (cnt = 0; cnt < dim; ++cnt)
                if (fabs((double)y.getValue(cnt)) > 0 && fabs((double)rhs.getValue(cnt)) > 0)  {
                    p += log(fabs((double)y.getValue(cnt)));
                    pr += log(fabs((double)rhs.getValue(cnt)));
                };
            lalpha = p - pr;
        };
    double ll  = log_likelihood(s, dim);
    double llo = log_likelihood(ssRhs, dim);
    double lJ = ll - llo;
    return ll;
}

double LcPrime(const DenseVector &v, const double * residW, const SparseMatrix &w, const double rho)  
{
    const int dim = w.dim();
    DenseVector 	prime(dim), rhs(dim);
    w.matrixColumn(rhs, v);
    for (int cnt = 0; cnt < dim; ++cnt)
        rhs.plusAt( cnt, -residW[cnt] );
    cg(w, rho, rhs, prime);
    double pp = -0.5 * dim * prime.product(v) / v.norm();

    return pp;
}    

void run1(SparseMatrix &w, const double rr, double &trace, double &trace2,
		  double &frobenius,
		  wxGauge* p_bar, double p_bar_min_fraction, double p_bar_max_fraction)
{
	LOG_MSG("Entering run1");
    const int LIMIT = 50;
    const double EPS = 1.0e-14;
    const int dim = w.dim();
    SparseVector	sol( dim ), resid( dim ), p( dim ), d( dim );
    double rho, beta, rho_lag;

    int its = 0 , nzs = 0;
    
    trace = 0, trace2 = 0, frobenius = 0;
    double s0 = 0, s1 = 0, s2 = 0, sse = 0;
    double gs0 = 0, gs1 = 0, gs2 = 0;
	
	int g_min, g_max, prev_g_val; 
	int cur_g_val, g_val_init, g_val_final, g_val_range;
	int loop_min = 0, loop_max = max(0, dim-1);
	if (p_bar) {
		g_min = 0;
		g_max = p_bar->GetRange();
		g_val_init = p_bar_min_fraction * g_max;
		g_val_final = p_bar_max_fraction * g_max;
		g_val_range = g_val_final - g_val_init;
		prev_g_val = g_val_init;
		cur_g_val = prev_g_val;
		p_bar->SetValue(g_val_init);
		p_bar->Update();
	}	
    for (int ix = 0; ix < dim; ++ix) {
		if (p_bar) {
			cur_g_val = (ix*g_val_range)/loop_max + g_val_init;
			if (cur_g_val > prev_g_val) {
				p_bar->SetValue(cur_g_val);
				prev_g_val = cur_g_val;
				p_bar->Update();
			}
		}
		sol.reset();
        sol.setAt( ix, 1 );
        w.rowIminusRhoThis( rr, p, sol );			// p = Ax
        resid.minus( sol, p );			// r = b - Ax
        rho = resid.norm();			// rho = ss of resid
        int it = 0;				// iteration counter
        while (rho > EPS && it < LIMIT) {
            ++it;
            if (it == 1) {
				d.copy(resid);
            } else {
                beta = rho / rho_lag;
                d.timesPlus(resid, beta);
            }
            w.rowIminusRhoThis( rr, p, d );			// p = Ad
            double alpha = rho / d.product( p );	// alpha = rho / d'p
            sol.addTimes( d, alpha );			// sol = sol + alpha*d
            resid.addTimes( p, -alpha );		// resid = resid - alpha*p
            rho_lag = rho;
            rho = resid.norm();
            s0 = s1;
            s1 = s2;
            s2 = w.getRow(ix).timesColumn(sol);
        }
        w.rowMatrix( p, sol );				// p = (Winv(I-rW))i 
        
        extract(p, w.getScale(), ix, trace, trace2, frobenius);
        its += it;
        nzs += sol.getNzEntries();
        double se = 0;
        if (s2-s1 > 1.0e-14)  se = geoda_sqr(s2-s1)/(s2-2.0*s1+s0);
        sse += s2 - se;
        gs0 += s0;
        gs1 += s1;
        gs2+= s2;
    }
	if (p_bar) {
		p_bar->SetValue(g_val_final);
		p_bar->Update();
	}
    LOG_MSG("Exiting run1");
}

/*   ECL
* function to compute log-likelihood function for the spatial lag model
resid -- vector of residuals in regression y on X;
residW -- vector or residulas in regression of Wy on X;
rho -- value of the coefficient of spatial association.
Note: function uses eigenvalues to compute log-Jacobian.
*/
VALUE   ECL(WVector & resid, WVector & residW, const VALUE rho, const double * d)  
{
    const int		dim = resid.count();
    int cnt = 0;
    double lj = 0;
    for (cnt = 0; cnt < dim; ++cnt)		// compute log-Jacobian
        lj += log(1.0 - rho * d[cnt]);

    WVector   tmp;
    tmp.reset();
    tmp.copy(residW());       // copy residiual of wy on X
    tmp *= rho;               // multiply it by rho
    tmp -= resid;             // subtract residual of y on X
    double    nn= resid.count(), product = norm(tmp());
    double sse = 0.5 * nn * log(product/nn);
    double accum = lj - sse;
    // double sse1 = -1.0*((nn/2.0)*(log(2.0*M_PI))+(nn/2.0)*log((product/nn))+(product/(2.0*(product/nn))));
    // double accum1 = lj - sse1;

    return accum;
}

/*   ECL
* function to compute log-likelihood function for the spatial lag model
resid -- vector of residuals in regression y on X;
residW -- vector or residulas in regression of Wy on X;
rho -- value of the coefficient of spatial association.
Note: function uses eigenvalues to compute log-Jacobian.
*/
VALUE   ECL(WVector & resid, WVector & residW, const VALUE rho, const double* wr, const double* wi)  
{
    const int		dim = resid.count();
    int cnt = 0;
    double jr = 1, ji = 0;
    for (cnt = 0; cnt < dim; ++cnt)		// compute log-Jacobian
    {
        jr = (1.0 - rho * wr[cnt]) * (1.0 - rho * jr) - (rho * ji * wi[cnt]);
        ji = -rho * (ji * (1.0 - rho * wr[cnt]) + wi[cnt] * (1.0 - rho * jr));
    }
    jr = log(jr);
cout << ji << endl;
    WVector   tmp;
    tmp.reset();
    tmp.copy(residW());       // copy residiual of wy on X
    tmp *= rho;               // multiply it by rho
    tmp -= resid;             // subtract residual of y on X
    double    nn= resid.count(), product = norm(tmp());
    double sse = 0.5 * nn * log(product/nn);
    double accum = jr - sse;

    // double sse1 = -1.0*((nn/2.0)*(log(2.0*M_PI))+(nn/2.0)*log((product/nn))+(product/(2.0*(product/nn))));
    // double accum1 = lj - sse1;

    return accum;

}


VALUE SmallGoldenSectionLag(const VALUE left, 
														const VALUE middle, 
														const VALUE right, 
														WVector &resid, 
														WVector &residW, 
														const double *d,
														double *LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
        x2 += GoldenToo * (right - middle);
    else
        x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    f2 = ECL(resid, residW, x2, d);
    f1 = ECL(resid, residW, x1, d);
    while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  {
        if (f1 < f2)  
				{
            SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
            SHFT(f0, f1, f2, ECL(resid, residW, x2, d));
        }  else  
				{
            SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
            SHFT(f3, f2, f1, ECL(resid, residW, x1, d));
        };
        ++Counter;
    };
    if (f2 > f1)  
		{
        f1= f2;
        x1= x2;
    };

	const double n = (double) resid.count();
	*LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);
  return  x1;
}

VALUE SmallGoldenSectionLag(const VALUE left, 
														const VALUE middle, 
														const VALUE right, 
														WVector &resid, 
														WVector &residW, 
														const double *wr,
														const double *wi,
														double *LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
        x2 += GoldenToo * (right - middle);
    else
        x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    f2 = ECL(resid, residW, x2, wr, wi);
    f1 = ECL(resid, residW, x1, wr, wi);
    while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  {
        if (f1 < f2)  
				{
            SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
            SHFT(f0, f1, f2, ECL(resid, residW, x2, wr, wi));
        }  else  
				{
            SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
            SHFT(f3, f2, f1, ECL(resid, residW, x1, wr, wi));
        };
        ++Counter;
    };
    if (f2 > f1)  
		{
        f1= f2;
        x1= x2;
    };
 
	const double n = (double) resid.count();
	*LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);
  return  x1;
}

double SmallSimulationLag(Weights &W,
						  int num_obs,
						  const double rho, 
						  double* my_Y,
						  double** my_X, 
						  const	int		deps,
						  bool InclConstant,
						  double* LogLik, bool asym,
						  wxGauge* p_bar,
						  double p_bar_min_fraction,
						  double p_bar_max_fraction)  
{
    W.Transform(W_MAT);               // makes sure it is properly formated
    const int   dim = W.dim();
    int   cnt=0;
    WVector       p_y(dim), p_lag(dim);
    DenseVector y(my_Y, dim, false), lag(dim), *x = new DenseVector [deps];
    // read in data
    for (cnt = 0; cnt < dim; ++cnt) {
        p_y << my_Y[cnt];
    }
    for (cnt = 0; cnt < deps; cnt++) {
        x[cnt].absorb(my_X[cnt], dim);
    }

	int row = 0, column = 0;
    WMatrix	sym;
    copy(sym, W.Mit());
	
	if (!asym) {
		MakeSym(sym());       // make it symmetric; has eigenvalues of the rowstandardized matrix
	} else {
		RowStandardize(sym());
	}
    RowStandardize(W.Mit()); // non-symmetric, row-standardized -- used to compute spatial lag

    p_lag.alloc();
    SpatialLag(W.Mit(), p_y(), p_lag);
    for (cnt = 0; cnt < dim; cnt++)
    	lag.setAt(cnt, p_lag[cnt]);

	double *s = new double [dim], *wr = new double [dim], *wi = new double [dim];
	if (!asym)
	{
		// assume real and symmetric matrix
		// use CLAPACK to compute all eigenvalues
		// use dspev_

		char jobz = 'N', uplo = 'U';
		long int n = dim;
		long int ldz = dim, lwork = 3 * dim, info = 0;
		double *a = new double [dim * (dim + 1) / 2];
		double *z = NULL;
		double *work = new double [lwork];
		for (row = 0; row < dim; row++) {
			for (column = row; column < dim; column++) {
				a[row + column * (column + 1) / 2] = sym[row][column];
			}
		}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
//#ifdef __WXMAC__
		dspev_(&jobz, &uplo, &n, a, s, z, &ldz, work, &info);
//#else
//		dspev_(&jobz, &uplo, (integer*)&n, (doublereal*)a, (doublereal*)s, (doublereal*)z, (integer*)&ldz, (doublereal*)work, (integer*)&info);
//#endif

		if (!info) {
			// eigenvalues are in s
			// good and nothing else to do in this step
		} else {
			cerr << "error in computing eigenvalues" << endl;
		//	wxMessageBox("error in computing eigenvalues";
			return -1;
		}
	}
	else
	{
		// assume real and asymmetric matrix
		// use CLAPACK to compute all eigenvalues
		// use dgeev_

		char jobvl = 'N', jobvr = 'N';
		long int n = dim;
		long int lda = dim, ldvl = dim, ldvr = dim, lwork = 3 * dim, info = 0;
		double *a = new double [dim * dim];
		double *vl = NULL, *vr = NULL;
		double *work = new double [lwork];
		for (row = 0; row < dim; row++) {
			for (column = 0; column < dim; column++) {
				a[row + column * dim] = sym[row][column];
			}
		}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
//#ifdef __WXMAC__
		dgeev_(&jobvl, &jobvr, &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, work, &lwork, &info);
//#else
//		dgeev_(&jobvl, &jobvr, (integer*)&n, (doublereal*)a, (integer*)&lda, (doublereal*)wr, (doublereal*)wi, (doublereal*)vl, (integer*)&ldvl, (doublereal*)vr, (integer*)&ldvr, (doublereal*)work, (integer*)&lwork, (integer*)&info);
//#endif

		if (!info) {
			// eigenvalues are in wr and wi
			// good and nothing else to do in this step
		} else {
			cerr << "error in computing eigenvalues" << endl;
		//	wxMessageBox("error in computing eigenvalues";
			return -1;
		}
	}

	double **cov = new double * [deps], *resid = new double [dim], *residW = new double [dim];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	DenseVector ols(deps), olsW(deps);
	if (!ordinaryLS(y, x, cov, resid, ols))
		cerr << "svd error\n"; // ols = cov X' y
	if (!ordinaryLS(lag, x, cov, residW, olsW))
		cerr << "svd error\n"; // olsW = cov X' lag
	WVector re(dim), reW(dim);
	for (cnt = 0; cnt < dim; cnt++) {
		re << resid[cnt];
		reW << residW[cnt];
	}
    VALUE rhoEstimate = 0.0;
    if (asym)
    	rhoEstimate = SmallGoldenSectionLag(-1, 0, 1, re, reW, wr, wi, LogLik);
    else
    	rhoEstimate = SmallGoldenSectionLag(-1, 0, 1, re, reW, s, LogLik);

    return rhoEstimate;
}

double SimulationLag(const GalElement *weight,
					 int num_obs,
					 int Precision, 
					 const double			rho, 
					 double*		my_Y,
					 double**		my_X,
					 const int				deps,
					 bool InclConstant,
					 double* LogLik,
					 wxGauge* p_bar,
					 double p_bar_min_fraction,
					 double p_bar_max_fraction)
{
	LOG_MSG("Entering SimulationLag, GalElement*");
  	Weights  W(weight, num_obs);          // read the weights matrix
	
    if (W.dim() < SMALL_DIM)
        return SmallSimulationLag(W, num_obs, rho, my_Y, my_X, deps,
								  InclConstant, LogLik, false,
								  p_bar, p_bar_max_fraction,
								  p_bar_max_fraction);
    
    W.Transform(W_GWT);               // makes sure it is formated
    const int   dim= W.Git().count();
    int   cnt = 0, cp = 0;
    WVector       p_y(dim), p_lag(dim);

    DenseVector y(my_Y, dim, false), lag(dim), *x = new DenseVector [deps];
    // // read in data
    for (cnt = 0; cnt < dim; ++cnt) {
        p_y << my_Y[cnt];
    }
    for (cnt = 0; cnt < deps; cnt++) {
    	x[cnt].absorb(my_X[cnt], dim);
    }

    GWT sym;
    copy(sym, W.Git());
	// make it symmetric; has eigenvalues of the rowstandardized matrix
    MakeSym(sym()); 
	// non-symmetric, row-standardized -- used to compute spatial lag
    RowStandardize(W.Git());
    p_lag.alloc();
    SpatialLag(W.Git(), p_y(), p_lag);
    for (cnt = 0; cnt < dim; cnt++)
    	lag.setAt(cnt, p_lag[cnt]);

    clock_t       start, stop;
    // "  computing polynomial 
    start= clock();

    InitPoly(Precision, dim);
    SparsePoly(sym());
    // "  --- finished computing polynomial" 
	double **cov = new double * [deps];
	double *resid = new double [dim];
	double *residW = new double [dim];
	for (cnt = 0; cnt < deps; cnt++) {
		cov[cnt] = new double [deps];
		for (cp = 0; cp < deps; cp++) {
			cov[cnt][cp] = 0;
		}
	}
	DenseVector ols(deps), olsW(deps);
	if (!ordinaryLS(y, x, cov, resid, ols))
		cerr << "svd error\n"; // ols = cov X' y
	if (!ordinaryLS(lag, x, cov, residW, olsW))
		cerr << "svd error\n"; // olsW = cov X' lag
	WVector re(dim), reW(dim);
	for (cnt = 0; cnt < dim; cnt++) {
		re << resid[cnt];
		reW << residW[cnt];
	}
    VALUE rhoEstimate = 0.0;
	// e0: resid, eL: residw see Oleg's paper
    rhoEstimate = GoldenSectionLag(-1, 0, 1, re, reW, LogLik);
    stop= clock();

	LOG_MSG("Exiting SimulationLag");
    return rhoEstimate;
}


VALUE SmallErrorLogLikelihood(Iterator<WVector> X, 
							  Iterator<WVector> lagX, 
							  WIterator y, 
							  WIterator lagY, 
							  Iterator<WVector> W, 
							  const VALUE lambda, 
							  WVector &egls, 
							  double * d,
							  bool InclConstant)  
{
	int cnt = 0;
    double lj = 0;
    for (cnt = 0; cnt < (int) y.count(); ++cnt)		// compute log-Jacobian
        lj += log(1.0 - lambda * d[cnt]);
    
    WMatrix XminusLambdaLagX(X.count());
    WVector YminusLambdaLagY(y.count());

    _minus(X, lagX, XminusLambdaLagX, lambda);
    _minus(y, lagY, YminusLambdaLagY, lambda);

	int row = 0, column = 0, deps = X.count(), dim = y.count();
	double **cov = new double * [deps], *resid = new double [dim];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	DenseVector p_y(dim), *p_x = new DenseVector [deps], eg(deps);
	for (row = 0; row < dim; row++) {
		p_y.setAt(row, YminusLambdaLagY[row]);
	}
	for (row = 0; row < deps; row++) {
		p_x[row].alloc(dim);
		for (column = 0; column < dim; column++) {
			p_x[row].setAt(column, XminusLambdaLagX[row][column]);
		}
	}
    ordinaryLS( p_y, p_x, cov, resid, eg );
    for (row = 0; row < deps; row++) egls[row] = eg.getValue(row);

    WVector rs(y.count(), 0), lagResid(y.count(), 0), RminusLambdaLagR(y.count(), 0);
    for (row = 0; row < dim; row++) rs[row] = resid[row];
    _minus(rs(), lagResid(), RminusLambdaLagR, lambda);

    VALUE sse = norm(RminusLambdaLagR());		

    VALUE addOn = -0.5 * y.count() * log(sse/y.count());
    return (lj + addOn);
}  

VALUE SmallErrorLogLikelihood(Iterator<WVector> X, 
															Iterator<WVector> lagX, 
															WIterator y, 
															WIterator lagY, 
															Iterator<WVector> W, 
															const VALUE lambda, 
															WVector &egls, 
															double* wr,
															double* wi,
															bool InclConstant)  
{
	int cnt = 0;
    double jr = 1, ji = 0;
    for (cnt = 0; cnt < (int) y.count(); ++cnt)		// compute log-Jacobian
    {
        jr = (1.0 - lambda * wr[cnt]) * (1.0 - lambda * jr) - (lambda * wi[cnt] * ji);
        ji = -lambda * (ji * (1.0 - lambda * wr[cnt]) + wi[cnt] * (1.0 - lambda * jr));
    }
    jr = log(jr);
cout << ji << endl;    
    WMatrix XminusLambdaLagX(X.count());
    WVector YminusLambdaLagY(y.count());

    _minus(X, lagX, XminusLambdaLagX, lambda);
    _minus(y, lagY, YminusLambdaLagY, lambda);

	int row = 0, column = 0, deps = X.count(), dim = y.count();
	double **cov = new double * [deps], *resid = new double [dim];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	DenseVector p_y(dim), *p_x = new DenseVector [deps], eg(deps);
	for (row = 0; row < dim; row++) {
		p_y.setAt(row, YminusLambdaLagY[row]);
	}
	for (row = 0; row < deps; row++) {
		p_x[row].alloc(dim);
		for (column = 0; column < dim; column++) {
			p_x[row].setAt(column, XminusLambdaLagX[row][column]);
		}
	}
    ordinaryLS( p_y, p_x, cov, resid, eg );
    for (row = 0; row < deps; row++) egls[row] = eg.getValue(row);

    WVector rs(y.count(), 0), lagResid(y.count(), 0), RminusLambdaLagR(y.count(), 0);
    for (row = 0; row < dim; row++) rs[row] = resid[row];
    _minus(rs(), lagResid(), RminusLambdaLagR, lambda);

    VALUE sse = norm(RminusLambdaLagR());		

    VALUE addOn = -0.5 * y.count() * log(sse/y.count());
    return (jr + addOn);
}  

VALUE SmallGoldenSectionError(const VALUE left, 
															const VALUE middle, 
															const VALUE right, 
															const WMatrix &X, 
															const WVector &y,
															Iterator<WVector> W, 
															double * &beta, 
															double *d,
															bool InclConstant,
															double *LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
        x2 += GoldenToo * (right - middle);
    else
        x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    WMatrix lagX;
    WVector lagY(X[0].count()), egls(X.count());
    SpatialLag(W, X(), lagX);
    SpatialLag(W, y(), lagY);
	f2 = SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls, d,InclConstant);
    f1 = SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls, d,InclConstant);

    //  this is 'classic' golden section
    while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  {
        if (f1 < f2)  {
            SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
            SHFT(f0, f1, f2, SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls, d,InclConstant));
        }  else  {
            SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
            SHFT(f3, f2, f1, SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls, d,InclConstant));
        };
        ++Counter;
    };
    if (f2 > f1)  {
        f1= f2;
        x1= x2;
    };

		const int n = W.count();
		*LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);

    beta = new double[ X[0].count() ];
    for (int cnt = 0; cnt < (int) egls.count(); ++cnt)
        beta[cnt] = egls[cnt];
    return  x1;
}

VALUE SmallGoldenSectionError(const VALUE left, 
															const VALUE middle, 
															const VALUE right, 
															const WMatrix &X, 
															const WVector &y,
															Iterator<WVector> W, 
															double * &beta, 
															double *wr,
															double *wi,
															bool InclConstant,
															double *LogLik)  
{
    const VALUE   GoldenRatio = (sqrt((double)5)-1)/2, GoldenToo = 1 - GoldenRatio;
    VALUE     x0, x1, x2, x3, f0, f1, f2, f3;
    x0= left;
    x3= right;
    x1= x2= middle;
    if (fabs(right-middle) > fabs(middle-left))
        x2 += GoldenToo * (right - middle);
    else
        x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    WMatrix lagX;
    WVector lagY(X[0].count()), egls(X.count());
    SpatialLag(W, X(), lagX);
    SpatialLag(W, y(), lagY);
    f2 = SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls, wr, wi, InclConstant);
    f1 = SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls, wr, wi, InclConstant);


    //  this is 'classic' golden section
    while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  {
        if (f1 < f2)  {
            SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
            SHFT(f0, f1, f2, SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x2, egls, wr, wi, InclConstant));
        }  else  {
            SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
            SHFT(f3, f2, f1, SmallErrorLogLikelihood(X(), lagX(), y(), lagY(), W, x1, egls, wr, wi, InclConstant));
        };
        ++Counter;
    };
    if (f2 > f1)  {
        f1= f2;
        x1= x2;
    };

		const int n = W.count();
		*LogLik = f1 - n/2.0 - n/2.0 * log(2.0*M_PI);

	beta = new double[ X[0].count() ];
    for (int cnt = 0; cnt < (int) egls.count(); ++cnt)
        beta[cnt] = egls[cnt];
    return  x1;
}

double SmallSimulationError(Weights &W, 
							const double rho, 
							const double* my_Y,
							double** my_X,
							const int deps,
							double * &beta, 
							bool InclConstant,
							double *LogLik, bool asym,
							wxGauge* p_bar,
							double p_bar_min_fraction,
							double p_bar_max_fraction)  
{
    W.Transform(W_MAT);               // makes sure it is formated
    const int   dim = W.dim();
    int expl = 0, cnt = 0;
    WVector      	y(dim);
    WMatrix       	X(2);
    // read in data
    for (cnt = 0; cnt < dim; ++cnt) y << my_Y[cnt];
    X = new WMatrix(deps);

    for (expl = 0; expl < deps; expl++)  
		{
        for (cnt = 0; cnt < dim; ++cnt)
            X[expl] << my_X[expl][cnt];
    };
    X.reset(deps);

    WMatrix		sym;
    copy(sym, W.Mit());
	
	//{
	//	LOG_MSG("sym dump before MakeSym:");
	//	Iterator<WVector> it = sym();
	//	for (int i=0; i<dim; i++) {
	//		for (int j=0; j<dim; j++) {
	//			LOG_MSG(wxString::Format("sym[%d][%d] = %f", i,j, it[i][j]));
	//		}
	//	}
	//}
	
    if (!asym) {
    	MakeSym(sym());                 // make it symmetric, while preserving eigenvalues -- used for computing log-Jacobian
    } else {
    	RowStandardize(sym());
	}
	
	//{
	//	LOG_MSG("sym dump after MakeSym:");
	//	Iterator<WVector> it = sym();
	//	for (int i=0; i<dim; i++) {
	//		for (int j=0; j<dim; j++) {
	//			LOG_MSG(wxString::Format("sym[%d][%d] = %f", i,j, it[i][j]));
	//		}
	//	}
	//}
	
    RowStandardize(W.Mit());		// non-symmetric, row-standardized -- used to compute spatial lag

    VALUE 	lambdaEstimate = 0.0;
    clock_t       start, stop;
	int row = 0, column = 0;
    // computing eigenvalues ...
    start = clock();

	double *s = new double [dim], *wr = new double [dim], *wi = new double [dim];
	if (!asym)
	{
		// assume real and symmetric matrix
		// use CLAPACK to compute all eigenvalues
		// use dspev_

		char jobz = 'N', uplo = 'U';
		long int n = dim;
		long int ldz = dim, lwork = 3 * dim, info = 0;
		double *a = new double [dim * (dim + 1) / 2];
		double *z = NULL;
		double *work = new double [lwork];
		for (row = 0; row < dim; row++) {
			for (column = row; column < dim; column++) {
				LOG(sym[row][column]);
				a[row + column * (column + 1) / 2] = sym[row][column];
			}
		}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__
		dspev_(&jobz, &uplo, &n, a, s, z, &ldz, work, &info);
#else
		dspev_(&jobz, &uplo, (integer*)&n, (doublereal*)a, (doublereal*)s, (doublereal*)z, (integer*)&ldz, (doublereal*)work, (integer*)&info);
#endif

		if (!info) {
			// eigenvalues are in s
			// good and nothing else to do in this step
		} else {
			cerr << "error in computing eigenvalues" << endl;
			wxMessageBox("Error: There was an error computing eigenvalues.");
			return -1;
		}
	}
	else
	{
		// assume real and asymmetric matrix
		// use CLAPACK to compute all eigenvalues
		// use dgeev_

		char jobvl = 'N', jobvr = 'N';
		long int n = dim;
		long int lda = dim, ldvl = dim, ldvr = dim, lwork = 3 * dim, info = 0;
		double *a = new double [dim * dim];
		double *vl = NULL, *vr = NULL;
		double *work = new double [lwork];
		for (row = 0; row < dim; row++) {
			for (column = 0; column < dim; column++) {
				a[row + column * dim] = sym[row][column];
			}
		}

// use __WXMAC__ to call vecLib
//#ifdef WORDS_BIGENDIAN
#ifdef __WXMAC__
		dgeev_(&jobvl, &jobvr, &n, a, &lda, wr, wi, vl, &ldvl, vr, &ldvr, work, &lwork, &info);
#else
		dgeev_(&jobvl, &jobvr, (integer*)&n, (doublereal*)a, (integer*)&lda, (doublereal*)wr, (doublereal*)wi, (doublereal*)vl, (integer*)&ldvl, (doublereal*)vr, (integer*)&ldvr, (doublereal*)work, (integer*)&lwork, (integer*)&info);
#endif

		if (!info) {
			// eigenvalues are in wr and wi
			// good and nothing else to do in this step
		} else {
			cerr << "error in computing eigenvalues" << endl;
			wxMessageBox("Error: There was an error computing eigenvalues.");
			return -1;
		}
	}

    if (asym) {
    	lambdaEstimate = SmallGoldenSectionError(-1, 0, 1, X, y, W.Mit(), beta, wr, wi, InclConstant, LogLik);
    } else {
		lambdaEstimate = SmallGoldenSectionError(-1, 0, 1, X, y, W.Mit(), beta, s, InclConstant, LogLik);
	}
    stop = clock();

    return lambdaEstimate;
}

double SimulationError(const GalElement *my_gal,
					   int num_obs,
					   int Precision, 
					   const double rho, 
					   const double* my_Y,
					   double** my_X,
					   const int deps,
					   double * &beta, 
					   bool InclConstant,
					   double* LogLik,
					   wxGauge* p_bar,
					   double p_bar_min_fraction,
					   double p_bar_max_fraction)  
{
    Weights W(my_gal, num_obs);          
    const int   dim = W.dim();
    if (dim < SMALL_DIM)
        return  SmallSimulationError(W, rho, my_Y, my_X, deps, beta,
									 InclConstant, LogLik, false,
									 p_bar, p_bar_min_fraction,
									 p_bar_max_fraction);
    W.Transform(W_GWT);               // makes sure it is formated
    int			cnt;
    WVector      	y(dim);
    WMatrix       	X(2);
    // read in data
    for (cnt = 0; cnt < dim; ++cnt) y << my_Y[cnt];
    X = new WMatrix(deps);

    for (int expl = 0; expl < deps; expl++)  
		{
        for (cnt = 0; cnt < dim; ++cnt)
            X[expl] << my_X[expl][cnt];
    };
    X.reset(deps);

    // ready to make symmetric
    GWT sym;
    copy(sym, W.Git());
    MakeSym(sym());   // make it symmetric, while preserving eigenvalues -- used for computing log-Jacobian

    RowStandardize(W.Git());	// non-symmetric, row-standardized -- used to compute spatial lag
    VALUE lambdaEstimate = 0.0;
    InitPoly(Precision, dim);
    SparsePoly(sym());
    Destroy(sym());		// don't need that spatial weights anymore

    lambdaEstimate = GoldenSectionError(-1, 0, 1, X, y, W.Git(), beta, LogLik);
    return lambdaEstimate;
}


void sdiff(const SparseMatrix &w, const double rho, const DenseVector &v, DenseVector &d)  {
    const int dim = w.dim();
	int cnt = 0;
    DenseVector		vt(dim);
    for(cnt = 0; cnt < dim; ++cnt)
        vt.setAt( cnt, v.getValue(cnt) * w.getScale()[cnt] );
    DenseVector		lag(dim);
    w.matrixColumn(lag, vt);
    
    for (cnt = 0; cnt < dim; ++cnt)
        d.setAt( cnt, (vt.getValue(cnt)-rho*lag.getValue(cnt)) / w.getScale()[cnt] );
}

double Lco(const DenseVector &y, const DenseVector &lag, const DenseVector * X, const SparseMatrix &w, const double rho, DenseVector &sol)  {
    const int dim = w.dim();
    DenseVector 	rhs1(dim), rhs2(dim), y_rho(dim), lag_rho(dim);
    w.IminusRhoThis ( rho, y, y_rho );
    w.IminusRhoThis ( rho, lag, lag_rho );
    double rho_ols = y_rho.product(lag_rho) / lag_rho.norm();
    double rho_I = y_rho.product(lag_rho) / y_rho.norm();
    double rho_IV = lag_rho.sum() / y_rho.sum();
    DenseVector		sol_lag(dim), resid(dim);
	int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt)  {
        sol.setAt( cnt , y_rho.getValue(cnt) - lag_rho.getValue(cnt) * rho_ols);
        resid.setAt( cnt , y_rho.getValue(cnt) - lag_rho.getValue(cnt) * rho);
    };
    double dr = resid.norm();
    w.IminusRhoThis( rho, sol, resid );
//    w.matrixColumn(sol_lag, sol);
    for (cnt = 0; cnt < dim; ++cnt)  {
//        sol_lag.setAt( cnt, sol_lag.getValue(cnt) / w.getScale()[cnt] );
//        sol.setAt( cnt, sol.getValue(cnt) / w.getScale()[cnt] );
    };
//    for (int cnt = 0; cnt < dim; ++cnt)
//        resid.setAt( cnt, sol.getValue(cnt) - rho * sol_lag.getValue(cnt) );
    
    double eWy = lag.product(sol), eWy_rho = lag_rho.product(sol);
    w.IminusRhoThis( rho_ols, sol, resid );
    
    return rho_ols;
}

//*** maximization routine using golden section method
double goldeno(const double left, const double right, const SparseMatrix &w,
              const DenseVector &y, const DenseVector &lag, const DenseVector * X)  {
    const int dim = w.dim();
    
    double middle = (left + right) / 2;
    double     x0, x1, x2, x3, f0, f1, f2, f3;
    x0 = left;
    x3 = right;
    x1 = x2 = middle;
    DenseVector		sol(dim);
    DenseVector		y_rho(dim), lag_rho(dim);
    

    if (fabs(right-middle) > fabs(middle-left))
        x2 += GoldenToo * (right - middle);
    else
        x1 -= GoldenToo * (middle - left);
    int   Counter = 2;
    double ols = y.product(lag) / lag.norm(), mi = y.product(lag) / y.norm();

    f1 = 1; x1 = mi;
    int it = 0;
    while (fabs(f1-x1) > TOLERANCE && it < 50)  {
        ++it;
        f1 = Lco(y, lag, X, w, x1, sol);
        if (f1+x1 > 1) x1 = (x1+f1)/2;
            else
                x1 += f1;
    };
    Lco(y, lag, X, w, 0.241867, sol);
    Lco(y, lag, X, w, 0.954481, sol);
    Lco(y, lag, X, w, 0.954482, sol);
    return x1;
    
    f2 = Lco(y, lag, X, w, x2, sol);
    f1 = Lco(y, lag, X, w, x1, sol);
    Lco(y, lag, X, w, f1, sol);
    Lco(y, lag, X, w, f2, sol);
    Lco(y, lag, X, w, -0.0586045, sol);
    exit(0);
    //  while (fabs(x3-x0) > tol*(fabs(x1)+fabs(x2)))  {			// here tol is used as RELATIVE accuracy
    while (fabs(x3-x0) > TOLERANCE)  {							// here tol is used as an ABSOLUTE accuracy
        if (f1 < f2)  {
            SHFT(x0, x1, x2, GoldenRatio*x2+GoldenToo*x3);
            SHFT(f0, f1, f2, Lco(y, lag, X, w, x2, sol));
        }  else  {
            SHFT(x3, x2, x1, GoldenRatio*x1+GoldenToo*x0);
            SHFT(f3, f2, f1, Lco(y, lag, X, w, x1, sol));
        };
        ++Counter;
    };
    if (f2 > f1)  
		{			// x2 has value larger than x1 has
        f1 = f2;
        x1 = x2;
    };
    return  x1;
    }


double lcdfPlus(const DenseVector &x, const DenseVector &df)  {
    double ss = x.norm(), ss2 = 0;
    for (int cnt = 0; cnt < x.getSize(); ++cnt)
        ss2 += geoda_sqr(x.getValue(cnt) + df.getValue(cnt));
    return (ss2-ss)/ss;
}

double estLj(DenseVector &y, DenseVector &lag, const double rho, const SparseMatrix &w)  {
    const double eps = 1.0e-6;
    const int dim = y.getSize();
    DenseVector		e(dim), eeps(dim), works(dim);
	int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt)
        e.setAt( cnt, y.getValue(cnt) - rho * lag.getValue(cnt) );
    double sss = 0;
    for (cnt = 0; cnt < dim; ++cnt)  {
        works.reset();
        works.setAt( cnt, eps);
        w.IminusRhoThis( rho, works, eeps);
        double ss = lcdfPlus(e, eeps) / eps;
        sss += ss;
    };
    
    return 0;
}

double getLj(const SparseMatrix &w, const double rho, const DenseVector &y, const DenseVector &lag)  
{
    const int dim = w.dim();
    DenseVector		e(dim), sol(dim), rm(dim);
	int cnt = 0;
    for (cnt = 0; cnt < dim; ++cnt)
        e.setAt( cnt, (y.getValue(cnt) - rho * lag.getValue(cnt)) * w.getScale()[cnt] );
    

    w.IminusRhoThis( rho, e, rm);
    cg(w, rho, e, sol);
    for (cnt = 0; cnt < dim; ++cnt)  {
        e.setAt( cnt, e.getValue(cnt) / w.getScale()[cnt] );
        sol.setAt( cnt, sol.getValue(cnt) / w.getScale()[cnt] );
        rm.setAt( cnt, rm.getValue(cnt) / w.getScale()[cnt] );
    };
    double en = e.norm(), sn = sol.norm(), rn = rm.norm();
    
    return 0;
}
        

void EGLS(const double lambda, 
					const DenseVector &y, 
					const DenseVector * X, 
					const SparseMatrix &w, 
					DenseVector &egls)
{
    const int dim = w.dim(), vars = egls.getSize();

    DenseVector		mY(dim), * mX = new DenseVector[vars];
    for (int cnt = 0; cnt < vars; ++cnt)  {
        mX[cnt].alloc(dim);
        w.IminusRhoThis(lambda, X[cnt], mX[cnt]);
    };
    w.IminusRhoThis(lambda, y, mY);		// mY = (I-lambda*W)y
    double ** cov = new double * [vars], * resid = new double [dim];
    int a = 0, b = 0;
    for (a = 0; a < vars; a++) {
    	cov[a] = new double [vars];
    	for (b = 0; b < vars; b++) {
    		cov[a][b] = 0;
    	}
    }
    ordinaryLS(mY, mX, cov, resid, egls);
	delete [] mX;
	mX = NULL;
	delete [] cov;
	cov = NULL;
	delete [] resid;
	resid = NULL;
}

double mic(const DenseVector &resid, const DenseVector &residW, const double rho, const double trace, const double trace2)  {
    const int dim = resid.getSize();
    DenseVector		rd(dim);

    rd.minusTimes(resid, residW, rho);
    const double sse = rd.norm();
    
    double fd = dim * rd.product(residW) - trace * sse;
    double sd = -dim * residW.norm() + 2.0 * trace * rd.product(residW) - trace2 * sse;
    return -fd / sd;
}

double mie(const DenseVector &rsd, const DenseVector &lag_resid,
		   const double trace, const double trace2,
           const DenseVector &y, const DenseVector *X,
		   const SparseMatrix &w, const int vars, const double lambda)
{
	typedef double* double_ptr_type;
    const int dim = rsd.getSize();
    const double sse = rsd.norm(), cross = rsd.product(lag_resid);
	int cnt = 0;
    DenseVector * dX = new DenseVector[ vars ], dY(dim);

    w.IminusRhoThis(lambda, y, dY);
    for (cnt = 0; cnt < vars; ++cnt)  {
        dX[cnt].alloc(dim);
        w.IminusRhoThis(lambda, X[cnt], dX[cnt]);
    };

    DenseVector * lagX = new DenseVector[vars], lagY(dim);

    w.matrixColumn( lagY, y );
    for (cnt = 0; cnt < vars; ++cnt)  {
        lagX[cnt].alloc(dim);
        w.matrixColumn( lagX[cnt], X[cnt] );
    };

    double_ptr_type* cov = new double_ptr_type [vars];

    for (cnt = 0; cnt < vars; ++cnt)  {
        cov[cnt] = new double[vars];
        for (int cp = 0; cp < cnt; ++cp)
            cov[cnt][cp] = cov[cp][cnt] = dX[cnt].product(dX[cp]);
        cov[cnt][cnt] = dX[cnt].norm();
    };
    if (SymMatInverse(cov, vars) == false)  return 0;
    double_ptr_type* lagDX = new double_ptr_type[vars];	// X(I-lamW)WX

    for ( cnt = 0; cnt < vars; ++cnt)  {
        lagDX[cnt] = new double[vars];
        for (int cp = 0; cp < cnt; ++cp)
            lagDX[cnt][cp] = lagDX[cp][cnt] = (dX[cnt].product(lagX[cp]) + dX[cp].product(lagX[cnt]));
        lagDX[cnt][cnt] = 2.0 * dX[cnt].product(lagX[cnt]);
    };

    DenseVector s1(vars), wk(vars), wk1(vars), wk2(dim), s5(vars); 
    
    for (cnt = 0; cnt < vars; ++cnt)
        s1.setAt( cnt, rsd.product(dX[cnt])*dim/sse);		// s1 = partial dL/dBeta
    
    for (cnt = 0; cnt < vars; ++cnt)
        wk.setAt(cnt, -lag_resid.product(dX[cnt]) - rsd.product(lagX[cnt]));
    wk.timesSquareMatrix(s5, cov);				// s5 == dBeta/dLambda

    double pp = s1.product(s5);
    double fd = dim * cross / sse - trace + pp;			// dL/dlambda

    for (cnt = 0; cnt < vars; ++cnt)
        wk.setAt( cnt, 2.0*cross*rsd.product(dX[cnt]) - sse*lag_resid.product(dX[cnt]) - sse*rsd.product(lagX[cnt]) );

    wk.times(dim/sse/sse);		// = (partial) d2L/dLambdadBeta
    double sd1 = 2.0 * wk.product(s5);	

    for (cnt = 0; cnt < vars; ++cnt)
        wk.setAt( cnt, rsd.product(dX[cnt]) );
    double_ptr_type* d2LdBeta2 = new double_ptr_type[vars];
    for (cnt = 0; cnt < vars; ++cnt)  {
        d2LdBeta2[cnt] = new double[vars];
        for (int cp = 0; cp < cnt; ++cp)
            d2LdBeta2[cnt][cp] = d2LdBeta2[cp][cnt] = wk.getValue(cnt)*wk.getValue(cp) - sse*dX[cnt].product(dX[cp]);
        d2LdBeta2[cnt][cnt] = geoda_sqr(wk.getValue(cnt)) - sse*dX[cnt].norm();
    };
    for (cnt = 0; cnt < vars; ++cnt)
        for (int cp = 0; cp < vars; ++cp)
            d2LdBeta2[cnt][cp] *= (1.0*dim/sse/sse);		// d2LdBeta2 = (partial) d2L/dBeta2
    s5.timesSquareMatrix(wk, d2LdBeta2);
    double sd2 = s5.product(wk);


    s5.timesSquareMatrix(wk, lagDX);
    for (cnt = 0; cnt < vars; ++cnt)
        wk.setAt(cnt, wk.getValue(cnt) + lagX[cnt].product(lag_resid));
    wk.timesSquareMatrix(wk1, cov);
    wk1.times(2);					// wk1 = (partial) d2beta/dlambda2
    double sd3 = wk1.product(s1);

    for (cnt = 0; cnt < vars; ++cnt)  {
        release(&cov[cnt]);
        release(&lagDX[cnt]);
        release(&d2LdBeta2[cnt]);
    };
	delete [] lagX;
	lagX = NULL;
	delete [] dX;
	dX = NULL;
    release(&cov);
    release(&lagDX);
    release(&d2LdBeta2);

    double psd = dim * (2.0*cross*cross - lag_resid.norm()*sse)/sse/sse - trace2;	// partial d2L/dLambda2


    double sd = psd + sd1 + sd2 + sd3;
    return -fd / sd;
}

extern float betai(float a, float b, float x);

