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

#include <map>
#include <boost/unordered_map.hpp>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/gauge.h>
#include "../ShapeOperations/GalWeight.h"

#include "mix.h"
#include "Lite2.h"
#include "ML_im.h"
#include "smile.h"
#include "../Regression/DiagnosticReport.h"

#define geoda_sqr(x) ( (x) * (x) )

extern double fprob (int dfnum, int dfden, double F);
extern double* JarqueBera(double* e, long n, long k);

void Compute_MoranI(GalElement* g,
                    double *resid,
                    int dim,
                    double* rst);
void Compute_RSLmError(GalElement* g,
					   double *resid,
					   int dim,
					   double* rst,
                       double t);

void Compute_RSLmErrorRobust(GalElement* g,
							 double** cov,
							 DenseVector y,
							 DenseVector *x,
							 DenseVector ols,
							 double *resid,
							 int dim,
							 int expl,
							 double* rst,
                             double t);

void Compute_RSLmLag(GalElement* g,
					 double** cov,
					 DenseVector y,
					 DenseVector *x,
					 DenseVector ols,
					 double *resid,
					 int dim,
					 int expl,
					 double* rst,
                     double t);

void Compute_RSLmLagRobust(GalElement* g,
						   double** cov,
						   DenseVector y,
						   DenseVector *x,
						   DenseVector ols,
						   double *resid,
						   int dim,
						   int expl,
						   double* rst,
                           double t);

void Compute_RSLmSarma(GalElement* g,
					   double** cov,
					   DenseVector y,
					   DenseVector *x,
					   DenseVector ols,
					   double *resid,
					   int dim,
					   int expl,
					   double* rst,
                       double t);


bool ordinaryLS(DenseVector &y, 
				 DenseVector * X, 
				 double ** &cov, 
				 double * resid, 
				 DenseVector &ols);


extern double product(const double * v1, const double * v2, const int &sz);  
extern double cdf(double x);
extern float betai(float a, float b, float x);
extern double MC_Condition_Number(double**, int,int);
extern double *BP_Test(double *resid, int obs, double** X, int expl,
					   bool InclConst);
extern double *WhiteTest(int obs, int nvar, double* resid, double** X,
						 bool InclConstant);

void Lag(DenseVector &lag, const DenseVector &x, GalElement *g)
{
    for (int cnt = 0; cnt < x.getSize(); ++cnt)
        lag.setAt( cnt, g[cnt].SpatialLag(x.getThis()) );
}

void MakeFastLookupMat(GalElement *g, int dim,
					   std::vector< std::set<int> >& g_lookup)
{
	using namespace std;
	g_lookup.resize(dim);
    for (int cnt = 0; cnt < dim; ++cnt) {
        for (int cp = 0; cp < g[cnt].Size(); ++cp) {
			g_lookup[cnt].insert(g[cnt][cp]);
		}
	}
}

double T(GalElement *g, int dim)
{
    // tr(W'W+WW)
    // = tr(W'W) + tr(WW)
    // = w'_ij*w_ji + w_ij*w_ji
    
	using namespace std;
    double	sum = 0;
   
    /*
    int i=0, j=0;
    for (i = 0; i < dim; ++i) {
        for (j = 0; j < dim; ++j) {
            //w_ij * w_ji
            
                //sum += g[i][j] * g[j][i];
                sum += g[i].GetRW(j) * g[j].GetRW(i);
        }
    }
    for (i = 0; i < dim; ++i) {
        for (j = 0; j < dim; ++j) {
            //w'_ij * w_ji
                //sum += g[j][i] * g[j][i];
                sum += g[j].GetRW(i) * g[j].GetRW(i);
        }
    }
     */
    
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            double w_ij = g[i].GetRW(j);
            double w_ji = g[j].GetRW(i);
            sum += w_ij * w_ji;
            sum += w_ji * w_ji;
            
        }
    }
    /*
     // below is also incorrect when handling knn weights matrix
    int cnt = 0, cp = 0;
    for (cnt = 0; cnt < dim; ++cnt) {
        for (cp = 0; cp < g[cnt].Size(); ++cp) {
            sum += geoda_sqr(1.0/g[cnt].Size());
        }
    }
		
	for (cnt = 0; cnt < dim; ++cnt) {
        for (cp = 0; cp < g[cnt].Size(); ++cp) {
			if (g_lookup[g[cnt][cp]].find(cnt)  != g_lookup[g[cnt][cp]].end()) {
                // check if transpose element exists (ie, non-zero)
				sum += (1.0/g[cnt].Size()/g[ g[cnt][cp] ].Size());
			}
		}
	}
	*/
    
    return sum;
    //return 21.738295484789607;
}

// This original version of T computes the trace of W'W + WW where W
// is the row-standardized version of the weights matrix g.  The
// algorithm is correct of g is symmetric, but incorrect otherwise.
// The computation for trace WW was the problem, and is corrected with
// the new version of T above.
//double T(const GalElement *g, int dim)
//{
//   double	sum = 0;
//    int cnt = 0, cp = 0;
//    for (cnt = 0; cnt < dim; ++cnt)  
//        for (cp = 0; cp < g[cnt].Size(); ++cp)
//            sum += geoda_sqr(1.0/g[cnt].Size()) +
//				(1.0/g[cnt].Size()/g[ g[cnt].elt(cp) ].Size());
//    return sum;
//}

//
// Performs spatial LAG test specification: computes RS statistic
//
void Compute_RSLmLag(GalElement* g,
					 double** cov,
					 DenseVector y,
					 DenseVector *x,
					 DenseVector ols,
					 double *resid,
					 int dim,
					 int expl,
					 double *rst,
                     double t) // t = T(g, dim, g_lookup)
{
    double *Y = y.getThis();
    double const ee = norm(resid, dim);
    double const sigma2		=  ee / (dim);

	int cnt = 0;
    DenseVector	lag(dim), re(resid, dim);
    
    for (cnt = 0; cnt < dim; ++cnt) {
        lag.setAt( cnt, g[cnt].SpatialLag(Y) ); // Wy
    }

    double RS = geoda_sqr(re.product( lag ) / sigma2); // [e'Wy/sigma2]^2

    ols.timesMatrix(re, x);		// re = Xb ~ y_hat

    Lag(lag, re, g);			// lag = WXb 
    DenseVector		z(expl), z2(expl);

    for (cnt = 0; cnt < expl; ++cnt)
        z.setAt( cnt, x[cnt].product(lag) );		// z = X'WXb

    z.squareTimesColumn( z2, cov );			// z2 = (X'X)^(-1)X'WXb
    const double xMx = z.product(z2); // (WXb)'X(X'X)^(-1)X'WXb
    // lag.norm : (WXb)'(WXb)
    double v = (lag.norm() - xMx + t * sigma2) / sigma2;
    RS /= v;

    double const RS_stat = gammp( 0.5, RS * 0.5);
    rst[0] = RS;
    rst[1] = RS_stat;
    return;
}


//
// Performs Lag Robus test specification: computes RS statistic
//
void Compute_RSLmLagRobust(GalElement* g,
						   double** cov,
						   DenseVector y,
						   DenseVector *x,
						   DenseVector ols,
						   double *resid,
						   int dim,
						   int expl,
						   double *rst,
                           double T21) // T21 = T(g, dim, g_lookup)
{
    double *Y = y.getThis();
    double const ee = norm(resid, dim);
    double const sigma2		=  ee / (dim);


	int cnt = 0;
    DenseVector	Wy(dim), We(dim), e(resid, dim);
    for (cnt = 0; cnt < dim; ++cnt)
    {
        Wy.setAt( cnt, g[cnt].SpatialLag(Y)); // Wy
        We.setAt( cnt, g[cnt].SpatialLag(resid)); // We
    }

    double RS1 = e.product(Wy) / sigma2;  // e'Wy/sigma2
    double RS2 = e.product(We) / sigma2;  // e'We/sigma2
    double RS = geoda_sqr(RS1-RS2);

    ols.timesMatrix(e, x);		// e = Xb ~ y_hat

    Lag(Wy, e, g);			// Wy = WXb 
    DenseVector	z(expl), z2(expl);

    for (cnt = 0; cnt < expl; ++cnt)
        z.setAt( cnt, x[cnt].product(Wy) );		// z = X'WXb

    z.squareTimesColumn( z2, cov );			// z2 = (X'X)^(-1)X'WXb

    // T11 = (WXb)'[I - X(X'X)^(-1)X'](WXb)
    // (WXb)'(WXb) - (WXb)'(X(X'X)^(-1)X')(WXb)
    // Wy.norm : (WXb)'(WXb)
    // z.product(z2) : (WXb)'(X(X'X)^(-1)X')(WXb)
    const double T11 = Wy.norm() -  z.product(z2);
    const double T1 = T11 / sigma2;
    const double T2 = 1.0 / (T1 + T21);

    RS /= (1.0 / T2 - T21);

    double const RS_stat = gammp( 0.5, RS * 0.5 );
    rst[0] = RS;
    rst[1] = RS_stat;
    return;
}


void Compute_MoranI(GalElement* g,
					double *resid,
					int dim,
					double *rst)
{
	double const ee = norm(resid, dim); 
    double const sigma2		=  ee / (dim);
    DenseVector		re(resid, dim, false);
    DenseVector		lag( re.getSize() );

    
    //SparseMatrix orig(g, dim);
    //orig.rowStandardize();
    //orig.matrixColumn(lag, re);

    //double MoranI = re.product( lag ) / ee; // [e'We] / [ee]
    for (int cnt = 0; cnt < dim; ++cnt)
    {
        lag.setAt( cnt, g[cnt].SpatialLag(resid) ); // We
		re.setAt(cnt, resid[cnt]);
    }

    double MoranI = re.product( lag ) / ee; // [e'We] / [ee]
    
	double const M_stat = gammp( 0.5, fabs(MoranI) * 0.5);
	rst[0] = MoranI;
	rst[1] = M_stat;
}


void ReportDenseVector(const wxString ttl, DenseVector* X, int n, int k)
{
	char buf[222];wxString msg=ttl+ ":\n";
	for (int i=0;i<n;i++)
	{
		for (int j=0;j<k;j++)
		{
		 sprintf(buf,"(%f)",X[j].getValue(i));
		 msg += wxString::Format("%s",buf);
		}
		sprintf(buf,"\n");
		msg += wxString::Format("%s",buf);

	}
	wxMessageBox(msg);
}

extern bool SymMatInverse(double ** mt, const int dim);


double Compute_MoranZ(GalElement* g,
					  double** D, // inverse([X'X]), size k by k
					  DenseVector *X, // size n by k, including constant term
					  int n,
					  int k,
					  const double moranI)
{
	using namespace std;
	SparseMatrix W(g, n);
	W.rowStandardize();

	DenseVector *weightedX = new DenseVector [k];
	DenseVector *weightedTX = new DenseVector [k];
	DenseVector *temp = new DenseVector [k];
	DenseVector *matrixA = new DenseVector [k];
	DenseVector *matrixB1 = new DenseVector [k];
	DenseVector *matrixB2 = new DenseVector [k];
	DenseVector *matrixB3 = new DenseVector [k];

	for (int i=0; i<k; i++) {
		weightedTX[i].alloc(n);
		W.WtTimesColumn(weightedTX[i], X[i]); //WtX = W'X
		weightedX[i].alloc(n);
		W.matrixColumn(weightedX[i], X[i]); // = WX
		temp[i].alloc(n);
		matrixA[i].alloc(k);
		matrixB1[i].alloc(k);
		matrixB2[i].alloc(k);
		matrixB3[i].alloc(k);
	}
	
	double s = 0.0;
	// Make a sparse, fast lookup version of W using hash tables
	// Note: following map can be either std::map or boost::unordered_map
	// unordered map is a hash table but has slower iterator access, while
	// map is a tree but has a fast iterator.
	vector< boost::unordered_map<int, double> > W_map(n);   // W
	vector< boost::unordered_map<int, double> > Wt_map(n);  // W'
	vector< map<int, bool> > B(n); // union of pattern of non-zeros in W and W'
	for (int i=0; i<n; i++) {
		Link *r = W.getRow(i).getNb();
		for (int nb=0, nb_sz=W.getRow(i).getSize(); nb<nb_sz; nb++) {
			int j=r[nb].getIx();
			double Wij = r[nb].getWeight();
			W_map[i][j] = Wij;
			Wt_map[j][i] = Wij;
			B[i][j] = true;
			B[j][i] = true;
		}
	}
	for (int i=0; i<n; i++) {
		boost::unordered_map<int, double>::iterator it;
		for (map<int, bool>::iterator B_it = B[i].begin();
			 B_it != B[i].end(); ++B_it) {
			int j = B_it->first;
			it = W_map[i].find(j);
			double Wij = (it != W_map[i].end()) ? Wij = it->second : 0;
			it = W_map[j].find(i);
			double Wji = (it != W_map[j].end()) ? Wji = it->second : 0;
			s += geoda_sqr(Wij + Wji);
		}
	}
	s *= 0.5;
	
	// A = (X'X)^-1X'WX 
	for (int i=0; i<k; i++) {
		for (int j=0; j<n; j++) {
			double c = 0.0;
			for (int l=0; l<k; l++) {
				c += D[i][l] * X[l].getValue(j);
			}
			temp[i].setAt(j, c);
		}

		for (int j=0; j<k; j++) {
			double c = 0.0;
			for (int l=0; l<n; l++) {
				c += temp[i].getValue(l) * weightedX[j].getValue(l);
			}
			matrixA[i].setAt(j, c);
		}
	}

	// tr geoda_sqr(A)
	double trAA = 0.0, trA = 0.0;
	for (int j=0; j<k; j++) {
		trA += matrixA[j].getValue(j);
		for (int l=0; l<k; l++) {
			trAA += matrixA[j].getValue(l) * matrixA[l].getValue(j);
		}
	}

	// make computation of B more efficiently
	// matrixB1, matrixB2, matrixB3 = X'WWX, X'WW'X, X'W'WX
	for (int i=0; i<k; i++) {
		for (int j=0; j<k; j++) {
			double b0 = 0.0;
			double b1 = 0.0;
			double b2 = 0.0;
			for (int l=0; l<n; l++) {
				b0 += weightedTX[i].getValue(l) * weightedX[j].getValue(l);
				b1 += weightedTX[i].getValue(l) * weightedTX[j].getValue(l);
				b2 += weightedX[i].getValue(l) * weightedX[j].getValue(l);
			}
			matrixB1[i].setAt(j, b0);
			matrixB2[i].setAt(j, b1);
			matrixB3[i].setAt(j, b2);
		}
	}

	double trB1 = 0.0, trB2 = 0.0, trB3 = 0.0;
	for (int i=0; i<k; i++) {
		for (int j=0; j<k; j++) {
			trB1 += (D[i][j] * matrixB1[j].getValue(i));
			trB2 += (D[i][j] * matrixB2[j].getValue(i));
			trB3 += (D[i][j] * matrixB3[j].getValue(i));
		}
	}
	// note that trB1 will be used twice
	double trB = 2 * trB1 + trB2 + trB3;

	double varI = (n-k) * (n-k+2.0) / 
				   (s + (2.0*trAA) - trB - (2.0*geoda_sqr(trA)/(n-k)));
	const double mI = trA / (n-k);
	const double zvalue = (moranI + mI) * sqrt(varI);

	return zvalue;
}


//
// Performs spatial error test specification: computes RS statistic
// t =  tr[(W'+W)*W]
void Compute_RSLmError(GalElement* g,
					   double *resid,
					   int dim, double *rst, double t)
{
    double const ee = norm(resid, dim);
    double const sigma2	=  ee / (dim);


    DenseVector	lag(dim), re(dim);
    for (int cnt = 0; cnt < dim; ++cnt)
    {
        lag.setAt( cnt, g[cnt].SpatialLag(resid) ); // We
		re.setAt(cnt, resid[cnt]);
    }

    double RS = geoda_sqr(re.product( lag ) / sigma2); // [e'We/sigma2]^2

    RS /= t;
    
	double const RS_stat = gammp( 0.5, RS * 0.5);
	rst[0] = RS;
	rst[1] = RS_stat;
	return;
}

void Compute_RSLmErrorRobust(GalElement* g,
							 double** cov,
							 DenseVector y,
							 DenseVector *x,
							 DenseVector ols,
							 double *resid,
							 int dim,
							 int expl,
							 double *rst,
                             double T21) //T21 = T(g, dim, g_lookup) tr[(W'+W)*W]
{
    double *Y = y.getThis();
    double const ee = norm(resid, dim);
    double const sigma2		=  ee / (dim);


	int cnt = 0;
    DenseVector	Wy(dim), We(dim), e(resid, dim);
    for (cnt = 0; cnt < dim; ++cnt) {
        Wy.setAt( cnt, g[cnt].SpatialLag(Y)); // Wy
        We.setAt( cnt, g[cnt].SpatialLag(resid)); // We
    }

    double RS1 = e.product(Wy) / sigma2;  // e'Wy/sigma2
    double RS2 = e.product(We) / sigma2;  // e'We/sigma2

    ols.timesMatrix(e, x);		// e = Xb ~ y_hat

    Lag(Wy, e, g);			// Wy = WXb 
    DenseVector	z(expl), z2(expl);

    for (cnt = 0; cnt < expl; ++cnt)
        z.setAt( cnt, x[cnt].product(Wy) );		// z = X'WXb

    z.squareTimesColumn( z2, cov );			// z2 = (X'X)^(-1)X'WXb

    // T11 = (WXb)'[I - X(X'X)^(-1)X'](WXb)
    // (WXb)'(WXb) - (WXb)'(X(X'X)^(-1)X')(WXb)
    // Wy.norm : (WXb)'(WXb)
    // z.product(z2) : (WXb)'(X(X'X)^(-1)X')(WXb)
    const double T11 = Wy.norm() -  z.product(z2);
    const double T1 = T11 / sigma2;
    
    const double T2 = 1.0 / (T1 + T21);

    const double RS = geoda_sqr(RS2 - (RS1 * T2 * T21)) / (T21-(T21*T21*T2));

    double const RS_stat = gammp( 0.5, RS * 0.5);
    rst[0] = RS;
    rst[1] = RS_stat;
    
}

void Compute_RSLmSarma(GalElement* g,
					   double** cov,
					   DenseVector y,
					   DenseVector *x,
					   DenseVector ols,
					   double *resid,
					   int dim,
					   int expl,
					   double *rst,
                       double T21)
{
    double *Y = y.getThis();
    double const ee = norm(resid, dim);
    double const sigma2		=  ee / (dim);


	int cnt = 0;
    DenseVector	Wy(dim), We(dim), e(resid, dim);
    for (cnt = 0; cnt < dim; ++cnt) {
        Wy.setAt( cnt, g[cnt].SpatialLag(Y)); // Wy
        We.setAt( cnt, g[cnt].SpatialLag(resid)); // We
    }

    double RS1 = e.product(Wy) / sigma2;  // e'Wy/sigma2
    double RS2 = e.product(We) / sigma2;  // e'We/sigma2

    ols.timesMatrix(e, x);		// e = Xb ~ y_hat

    Lag(Wy, e, g);			// Wy = WXb 
    DenseVector	z(expl), z2(expl);

    for (cnt = 0; cnt < expl; ++cnt)
        z.setAt( cnt, x[cnt].product(Wy) );		// z = X'WXb

    z.squareTimesColumn( z2, cov );			// z2 = (X'X)^(-1)X'WXb

    // T11 = (WXb)'[I - X(X'X)^(-1)X'](WXb)
    // (WXb)'(WXb) - (WXb)'(X(X'X)^(-1)X')(WXb)
    // Wy.norm : (WXb)'(WXb)
    // z.product(z2) : (WXb)'(X(X'X)^(-1)X')(WXb)
    const double T11 = Wy.norm() -  z.product(z2);
    const double T1 = T11 / sigma2;
    
    const double T2 = 1.0 / (T1 + T21);

    const double RS = (geoda_sqr(RS1 - RS2)/ (1.0/T2 - T21)) + (RS2*RS2/T21);

    double const RS_stat = gammp( 1.0, RS * 0.5 );
    rst[0] = RS;
    rst[1] = RS_stat;
    return;
}


/*
 OLS computes Ordinary Least Squares estimates and places output in result
 Performs spatial error test specification: computes RS statistic
*/

extern double log_likelihood(double ss, int n);
extern double mic(const DenseVector &resid, 
									const DenseVector &residW, 
									const double rho, 
									const double trace, 
									const double trace2);
extern void cg(const SparseMatrix &m, 
							 const double rho, 
							 const DenseVector &rhs, 
							 DenseVector &sol);

extern void run1(SparseMatrix &w, 
				 const double rr, 
				 double &trace, 
				 double &trace2, 
				 double &frobenius,
				 wxGauge* p_bar,
				 double p_bar_min_fraction,
				 double p_bar_max_fraction);

bool SymMatInverse(double ** mt, const int dim);


void DevFromMean(int nObs, double* RawData)
{
	double sumX = 0.0;
	int cnt = 0;
	for (cnt= 0; cnt < nObs; ++cnt) 
	{
		sumX += RawData[cnt];
	}
	const double  meanX = sumX / nObs;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		RawData[cnt] -= meanX;
	}
}

// yuntien: August 2005
// Regression
bool classicalRegression(GalElement *g,
						 int num_obs,
						 double * Y, int dim, 
						 double ** X, int expl, 
						 DiagnosticReport *dr, 
						 bool InclConstant,
						 bool m_moranz,
						 wxGauge* gauge,
						 bool do_white_test)
{
	int g_rng = 100;
	if (gauge) {
		g_rng = gauge->GetRange();
		gauge->SetValue(0);
	}
	
	double df = (dim - expl);
	DenseVector	y(Y, dim, false), ols(expl);
	DenseVector	*x = new DenseVector[expl + 1];

	int i = 0, j = 0, cnt = 0;
	for (cnt = 0; cnt < expl; ++cnt) {
		x[cnt].absorb(X[cnt], dim, false);
	}

	double **cov = new double* [expl], **ocov = new double* [expl];
	for (i = 0; i < expl; i++) {
		alloc(cov[i], expl);
		alloc(ocov[i], expl);
	}
	double *resid = new double [dim];

	// Compute OLS
	if (!ordinaryLS(y, x, cov, resid, ols)) return false;
	if (gauge) gauge->SetValue(g_rng / 3);

	// store the coefficients into the results
	double ee = product(resid, resid, dim); 
	double sigma2 = ee / df;
	double **D = new double* [expl];

	for (cnt = 0; cnt < expl; ++cnt) {
		dr->SetCoeff(cnt, ols.getValue(cnt));
		dr->SetStdError(cnt, sqrt(cov[cnt][cnt] * sigma2));
		const double zval = dr->GetCoefficient(cnt) / dr->GetStdError(cnt);
		dr->SetZValue(cnt, zval);
		double tcdf = df / (df + geoda_sqr(dr->GetZValue(cnt)));
		dr->SetProbVal(cnt, betai(df / 2.0, 0.5, tcdf));

		D[cnt] = new double [expl];
		for (j = 0; j < expl; j++) {
			D[cnt][j] = cov[cnt][j] * sigma2;
			dr->SetCovar(cnt, j, D[cnt][j]);
		}
	}

	DenseVector	y_hat(dim);
	for (i = 0; i < expl; i++)
	{
		y_hat.addTimes(x[i], ols.getValue(i));
	}

	// copy residuals and y_hat
	for (i = 0; i < dim; i++)
	{
		dr->SetResidual(i, resid[i]);
		dr->SetYHat(i, y_hat.getValue(i));
	}

	// diagnostics for spatial dependence
	if (g != NULL)
	{
		double *rst = new double[2];
        
        double t = T(g, dim); // tr[(W'+W)*W]

		Compute_RSLmError(g, resid, dim, rst, t);
		dr->SetLmError(0, 1.0);
		dr->SetLmError(1, rst[0]);
		dr->SetLmError(2, rst[1]);


		Compute_RSLmErrorRobust(g, cov, y, x, ols, resid, dim, expl, rst, t);
		dr->SetLmErrRobust(0, 1.0);
		dr->SetLmErrRobust(1, rst[0]);
		dr->SetLmErrRobust(2, rst[1]);


		Compute_RSLmLag(g, cov, y, x, ols, resid, dim, expl, rst, t);
		dr->SetLmLag(0, 1.0);
		dr->SetLmLag(1, rst[0]);
		dr->SetLmLag(2, rst[1]);


		Compute_RSLmLagRobust(g, cov, y, x, ols, resid, dim, expl, rst, t);
		dr->SetLmLagRobust(0, 1.0);
		dr->SetLmLagRobust(1, rst[0]);
		dr->SetLmLagRobust(2, rst[1]);


		Compute_RSLmSarma(g, cov, y, x, ols, resid, dim, expl, rst, t);
		dr->SetLmSarma(0, 2.0);
		dr->SetLmSarma(1, rst[0]);
		dr->SetLmSarma(2, rst[1]);


		Compute_MoranI(g, resid, dim, rst);
		dr->SetMoranI(0, rst[0]);
		if (m_moranz)
		{
			const double MoranZ = Compute_MoranZ(g, cov, x, dim, expl, rst[0]);
			dr->SetMoranI(1, MoranZ);
			dr->SetMoranI(2, 2.0 * (1.0 - nc(fabs(MoranZ))));
		}
	}
	if (gauge) gauge->SetValue((2*g_rng)/3);
	release(&D);


	double const sigma2ml = ee / dim;
	dr->SetSigSq(sigma2);
	dr->SetSigSqLm(sigma2ml);

	const int n = dim; int k = expl;
	double const ybar = y.sum() / n;

	double sum_y = 0.0;
	double R2;

	if (!InclConstant)
	{
		double e_bar = 0;
		for (cnt = 0; cnt < dim; cnt++)
			e_bar += resid[cnt];
		e_bar /= dim;

		DevFromMean(dim, resid);

		double e2 = norm(resid, dim); 

		for (cnt = 0; cnt < n; cnt++)
			sum_y += geoda_sqr(y.getValue(cnt) - e_bar);

		R2 = 1.0 - (e2 / (sum_y));

	}
	else
	{
		for (cnt = 0; cnt < n; cnt++)
			sum_y += geoda_sqr(y.getValue(cnt) - ybar);

		R2 = 1.0 - (ee / sum_y);
	}
	if (fabs(R2) > 1.0 || R2 < 0)
		R2 = 0.0;

	dr->SetR2Fit(R2);
	dr->SetR2Adjust(1.0 - ((n - 1) * ((1.0 - R2) / (n - k))));

	double lik = -1.0 * ((n / 2.0) * (log(2.0 * M_PI)) +
						 (n / 2.0) * log((ee / n)) +
						 (ee / (2.0 * (ee / n))));
	dr->SetLIK(lik);
	dr->SetAIC(-2.0 * lik + 2.0 * k); // # Akaike AIC
	dr->SetSC(-2.0 * lik + k * log((double) n)); // # Schwartz SC 

	double f_value;
	if (k == 1)
		f_value = geoda_sqr(dr->GetZValue(0)); // F test when k=1
	else
		f_value = (R2 / (k - 1.)) / ((1. - R2) / (n - k));// # F-test when k>1
	dr->SetFTest(f_value);
	dr->SetFTestProb(fprob(k - 1, n - k, f_value)); // Prob of F-test
	dr->SetRSS(ee);

	dr->SetCondNumber(MC_Condition_Number(X, n, k));
	double *jb = JarqueBera(resid, dim, expl);
	dr->SetJBTest(0, 2.0);
	dr->SetJBTest(1, jb[0]);
	dr->SetJBTest(2, jb[2]);

//	int wdf= InclConstant ? (geoda_sqr(k-1)+3*(k-1))/2 : (geoda_sqr(k)+3*k)/2;
//	release(&resid);
	resid = dr->GetResidual();

	if (do_white_test) {
		double *white = WhiteTest(dim, expl, resid, X, InclConstant);
		dr->SetWhiteTest(0, white[0]);
		dr->SetWhiteTest(1, white[1]);
		dr->SetWhiteTest(2, white[2]);
	}


//	resid = dr->GetResidual();
	double *bp = BP_Test(resid, dim, X, expl, InclConstant);

	if (bp == NULL)
	{
		dr->SetBPTest(0, expl);
		dr->SetBPTest(1, -1.0);
		dr->SetBPTest(2, -1.0);

		dr->SetKBTest(0, expl);
		dr->SetKBTest(1, -1.0);
		dr->SetKBTest(2, -1.0);
	}
	else
	{
		dr->SetBPTest(0, bp[1]);
		dr->SetBPTest(1, bp[0]);
		dr->SetBPTest(2, bp[2]);

		dr->SetKBTest(0, bp[4]);
		dr->SetKBTest(1, bp[3]);
		dr->SetKBTest(2, bp[5]);
	}

	release(&cov);
	release(&x);
	if (gauge) gauge->SetValue(g_rng);

    return true;
}

bool spatialLagRegression(GalElement *g,
						  int num_obs,
						  double * Y, 
						  int dim, 
						  double ** X, 
						  int deps, 
						  DiagnosticReport *dr, 
						  bool InclConstant,
						  wxGauge* p_bar)  
{
	typedef double* double_ptr_type;
	const int n = dim;
	DenseVector		y(Y, dim, false), *x = new DenseVector[deps];
	int cnt = 0, row = 0, column = 0;
	for (cnt = 0; cnt < deps; ++cnt)
		x[cnt].absorb(X[cnt], dim, false);
	
	double LogLike = 0, initRho = 0;
	
	initRho = SimulationLag(g, num_obs, 41, 0.31, Y, X, deps,
							!InclConstant, &LogLike,
							p_bar, 0, 0.1);
	SparseMatrix	orig(g, dim);

	double **cov = new double * [deps];
	double *resid = new double [n];
	double *residW = new double [n];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	
	DenseVector	lag( y.getSize() ), ols(deps), ols_lag(deps), beta(deps);
	DenseVector xbeta2(n);
	orig.rowStandardize();
	orig.matrixColumn(lag, y);
	orig.makeStdSymmetric();
	ordinaryLS(  y, x, cov, resid,  ols);
	
	double likOLS = log_likelihood(product(resid,resid,n),n);
	
 	ordinaryLS(lag, x, cov, residW, ols_lag);
	
	DenseVector		r(resid, n), rw(residW, n);
	
	double trace, trace2, fr;
	
	run1( orig, initRho, trace, trace2, fr, p_bar, 0.1, 0.55 );
	// correction for rho:  m
	// final rho: finRho
	double m = mic(r, rw, initRho, trace, trace2);
	double finRho = initRho - m;
	
	run1( orig, finRho, trace, trace2, fr, p_bar, 0.55, 1 );	
	
	// approximate computational error: m 
	m = mic(r, rw, finRho, trace, trace2);
	
	beta.minusTimes(ols, ols_lag, finRho);
	
	DenseVector		xbeta(n), rfin(n);
	
	for (int vr = 0; vr < deps; ++vr)
		xbeta.addTimes( x[vr], beta.getValue(vr) );
	
	rfin.copy(r);
	rfin.addTimes(rw, -finRho);
	double sigma2 = rfin.norm() / n;
	
	// autoregressive variable is the last
	double_ptr_type* info_matrix = new double_ptr_type[deps + 2];		
	
	for (cnt = 0; cnt < deps+2; ++cnt)
		info_matrix[cnt] = new double [deps+2];
	
	// Ed2L/dbdb
	for (cnt = 0; cnt < deps; ++cnt)  
	{
		for (int rc = cnt+1; rc < deps; ++rc)
			info_matrix[cnt][rc] = info_matrix[rc][cnt] = x[cnt].product(x[rc]) / sigma2;
		info_matrix[cnt][cnt] = x[cnt].norm() / sigma2;
	}
	
	// Ed2L/dbdr
	DenseVector		rhs(n), sol(n), works(n);
	for (cnt = 0; cnt < n; ++cnt)
		rhs.setAt( cnt, xbeta.getValue(cnt) * orig.getScale()[cnt] );
	
	//	orig.matrixColumn(xbeta2,xbeta); ? Xb / [I-rW]	
	cg(orig, finRho, rhs, sol);
	
	orig.matrixColumn(works, sol);
	
	// scale sol back to make it equal to (I-rW)^(-1)Xbeta
	for ( cnt = 0; cnt < dim; ++cnt)
		sol.setAt( cnt, sol.getValue(cnt) / orig.getScale()[cnt] ); 
	
	// Copy the residuals & predicted values
	for (cnt = 0; cnt < dim; ++cnt)
	{
		dr->SetResidual(cnt,rfin.getValue(cnt));
		dr->SetYHat(cnt,sol.getValue(cnt));
		dr->SetPredErr(cnt,Y[cnt]-sol.getValue(cnt));
	}
	
	
	for (cnt = 0; cnt < n; ++cnt)
		works.setAt( cnt, works.getValue(cnt) / orig.getScale()[cnt] );
	
	for (cnt = 0; cnt < deps; ++cnt)
		info_matrix[cnt][deps] = info_matrix[deps][cnt] = x[cnt].product(works) / sigma2;
	
	// Ed2L/dr2
	double ss = works.norm() / sigma2;
	info_matrix[deps][deps] = trace2 + fr + ss;
	
    info_matrix[ deps+1 ][ deps+1 ] = 0.5 * n / sigma2 / sigma2;
    for (cnt = 0; cnt < deps; ++cnt)
        info_matrix[deps+1][cnt] = info_matrix[cnt][deps+1] = 0.0;
    info_matrix[deps+1][deps] = info_matrix[deps][deps+1] = trace / sigma2;
	

	if (!SymMatInverse(info_matrix, deps+2)) return false;
	
	double df = (n - (deps + 1));
	
 	// Lag Coefficient 
	double ste = sqrt(info_matrix[deps][deps]);
	dr->SetCoeff(0,finRho);
	dr->SetStdError(0,ste);
	dr->SetZValue(0,finRho/ste);
	dr->SetProbVal(0,2.0*(1.0-nc(fabs(finRho/ste))));
	
	for (cnt = 1; cnt <= deps; ++cnt)  
	{
		double std = sqrt(info_matrix[cnt-1][cnt-1]); 
		double coe = beta.getValue(cnt-1);
		double sta = coe / std;
		
		dr->SetCoeff(cnt,coe);
		dr->SetStdError(cnt,std);
		dr->SetZValue(cnt,sta);
		dr->SetProbVal(cnt,2.0*(1.0-nc(fabs(sta))));
		
	}
	
	for (cnt = 0; cnt < deps+1; ++cnt)  
	{
		for (int j=cnt;j<deps+1;j++)
		{
			dr->SetCovar(cnt,j,info_matrix[cnt][j]);
			dr->SetCovar(j,cnt,info_matrix[j][cnt]);
		}
	}
	
	
	/* Fitting Information
	 (for each variable there will be COEFF, SDEV, t-Stat, Prob)
	 +5 : R2, Sq.Corr, LIK, AIC, SC
	 +2 : SIG-SQ, SIG-SQ std-error
	 */
	
 	int k = deps+1;
	double const ybar = y.sum() / n;
	
	double sum_y2 = 0.0;
	double R2;
	double ee = sigma2 * n;
	
	
	if (!InclConstant)
	{
		double e_bar = 0;
		for (cnt = 0;cnt < n;cnt++)
			e_bar += rfin.getValue(cnt);
		e_bar /= n;
		
		
		double e2 = 0;
		for (cnt = 0; cnt < n;cnt++)
		{
			e2  += geoda_sqr(rfin.getValue(cnt) - e_bar);
			sum_y2  += geoda_sqr(y.getValue(cnt));
		}
		R2 = 1.0 - (e2 / sum_y2);
	}
	else
	{
		sum_y2 = 0.0;
		for (cnt = 0; cnt < n;cnt++)
			sum_y2 += geoda_sqr(y.getValue(cnt)-ybar);
		
		R2 = 1.0 - (ee / sum_y2);
	}
	if (fabs(R2) > 1.0 || R2 < 0)
		R2 = 0.0;
	
	dr->SetR2Fit(R2);
	dr->SetR2Adjust(1.0 - ((n-1) * ((1.0 - R2) / (n-k))));
	
	
	double const lik = LogLike;
	//  double const lik = -1.0*((n/2.0)*(log(2.0*M_PI))+(n/2.0)*log((ee/n))+(ee/(2.0*(ee/n))));
	double const aic = -2.0*lik + 2.0*k; // # Akaike AIC
	double const sc  = -2.0*lik + k*log((double) n); // # Schwartz SC 
	
	dr->SetLIK(lik);
	dr->SetAIC(aic); // # Akaike AIC
	dr->SetSC(sc); // # Schwartz SC 
	dr->SetSigSq(sigma2);
	
	double LRtest = 2.0 * (lik - likOLS);
	dr->SetLR_Test(0,1.0);
	dr->SetLR_Test(1,LRtest);
	dr->SetLR_Test(2,gammp(0.5,LRtest * 0.5));
	
	resid = rfin.getThis();
	double *bp = BP_Test(resid, n, X, k-1, InclConstant);
	
	if (bp == NULL)
	{
		dr->SetBPTest(0,k-2);
		dr->SetBPTest(1,0.0);
		dr->SetBPTest(2,-1.0);
		
	}
	else
	{
		dr->SetBPTest(0,bp[1]);
		dr->SetBPTest(1,bp[0]);
		dr->SetBPTest(2,bp[2]);
		
	}
	
	/*
	 double w = (n-1.0)*geoda_sqr(finRho) / (1.0-geoda_sqr(finRho));
	 dr->SetWaldTest(0,k-1);
	 dr->SetWaldTest(1, w);
	 dr->SetWaldTest(2,gammp((k-1)/2.0,w/2.0));
	 */
	
	return true;
}

extern void EGLS(const double lambda, const DenseVector &y,
				 const DenseVector * X, const SparseMatrix &w, 
				 DenseVector &egls);
void residual(const DenseVector &rhs, const DenseVector * X,
			  const DenseVector &ols, DenseVector &resid);
double mie(const DenseVector &rsd, const DenseVector &lag_resid,
		   const double trace, const double trace2,
           const DenseVector &y, const DenseVector *X,
		   const SparseMatrix &w, const int vars, const double lambda);


bool spatialErrorRegression(GalElement *g,
							int num_obs,
							double * Y, 
							int dim, 
							double ** XX, 
							int deps, 
							DiagnosticReport *rr, 
							bool InclConstant,
							wxGauge* p_bar)  
{
	typedef double* double_ptr_type;
	DenseVector		y(Y, dim, false), *X = new DenseVector[deps];
	int cnt = 0, row = 0, column = 0;
	for (cnt = 0; cnt < deps; ++cnt)
		X[cnt].absorb(XX[cnt], dim, false);
	
	double * beta;
	const int n = dim;
	
	double LogLike = 0, initLambda = 0;
	initLambda = SimulationError(g, num_obs, 100, 0.31, Y, XX, deps, beta,
								 !InclConstant, &LogLike, p_bar, 0.0, 0.1 );
	release(&beta);
	
	double **cov = new double * [deps], *e_ols = new double [n];
	for (row = 0; row < deps; row++) {
		cov[row] = new double [deps];
		for (column = 0; column < deps; column++) {
			cov[row][column] = 0;
		}
	}
	
	DenseVector		ols(deps);
	
	ordinaryLS(  y, X, cov, e_ols,  ols);
	double likOLS = log_likelihood(product(e_ols,e_ols,n),n);
	
	// it will be appropriate to check if spatial weights is symmetric
	// check if we have indepenedent variables
	// determine similarity transfortmation
	
	SparseMatrix	orig(g, dim);
	orig.rowStandardize();
	double	 trace, trace2, fr;
	
	DenseVector		egls(deps), resid(dim), rsd(dim), lag_resid(dim);
	EGLS(initLambda, y, X, orig, egls);
	
	residual(y, X, egls, resid);
	orig.IminusRhoThis(initLambda, resid, rsd);
	orig.matrixColumn(lag_resid, resid);
	double sigma2 = rsd.norm() / dim;
	
	orig.makeStdSymmetric();
	run1( orig, initLambda, trace, trace2, fr, p_bar, 0.1, 0.55 );
	orig.makeRowStd();
	
	// correction for lambda: m 
	// final lambda: finLambda 
	double m = mie(rsd, lag_resid, trace, trace2, y, X, orig, deps, initLambda);
	const double lambda = initLambda - m;
	
	orig.makeStdSymmetric();
	
	run1( orig, lambda, trace, trace2, fr, p_bar, 0.55, 1 );
	orig.makeRowStd();
	
	EGLS(lambda, y, X, orig, egls);
	residual(y, X, egls, resid);
	orig.IminusRhoThis(lambda, resid, rsd);
	orig.matrixColumn(lag_resid, resid);
	sigma2 = rsd.norm() / dim;
	
	// approximate computational error for lambda: m;
	m = mie(rsd, lag_resid, trace, trace2, y, X, orig, deps, lambda);
	
	orig.makeStdSymmetric();
	//===
	//    error_info(finLambda, orig, trace2, fr, X, sigma2, egls, NULL);
	// Error_info
	//===
	const int vars = egls.getSize();
	
	DenseVector		xbeta(dim);
	
	for (int vr = 0; vr < vars; ++vr)
		xbeta.addTimes( X[vr], egls.getValue(vr) );
	
	for (cnt = 0; cnt < dim; ++cnt) {
		//		rr->SetResidual(cnt,rsd.getValue(cnt));
		rr->SetPredErr(cnt,Y[cnt]-xbeta.getValue(cnt));
		rr->SetResidual(cnt,
						Y[cnt]-xbeta.getValue(cnt)
						-(lambda*lag_resid.getValue(cnt)));
		rr->SetYHat(cnt,xbeta.getValue(cnt));
	}
	
	// keep autoregressive variable last
	double_ptr_type* info_matrix = new double_ptr_type[vars + 2];
	
	for (cnt = 0; cnt < vars+2; ++cnt) info_matrix[cnt] = new double [vars+2];
	
	//d2L/dbdl and d2L/dbds
	for (cnt = 0; cnt < vars; ++cnt) {
		info_matrix[cnt][vars] = info_matrix[vars][cnt] = 0;
		info_matrix[cnt][vars+1] = info_matrix[vars+1][cnt] = 0;
	}
	
	//d2L/dl2
	info_matrix[vars][vars] = trace2 + fr;
	
	//d2L/dbdb'
	DenseVector * lag_X = new DenseVector[vars], works(dim);
	
	for (cnt = 0; cnt < vars; ++cnt) {
		lag_X[cnt].alloc(dim);
		orig.scaleUp(works, X[cnt]);
		orig.IminusRhoThis(lambda, works, lag_X[cnt]);
		orig.scaleDown( lag_X[cnt] );
	}
	
	for (cnt = 0; cnt < vars; ++cnt) {
		info_matrix[cnt][cnt] = lag_X[cnt].norm() / sigma2;
		for (int rc = cnt+1; rc < vars; ++rc) {
			info_matrix[cnt][rc] = 
			info_matrix[rc][cnt] = 
			lag_X[cnt].product(lag_X[rc]) / sigma2;
		}
	}
	
	// d2L/ds2ds2
	info_matrix[vars+1][vars+1] = 0.5 * dim / sigma2 / sigma2;
	for (cnt = 0; cnt < vars; ++cnt)
        info_matrix[vars+1][cnt] = info_matrix[cnt][vars+1] = 0.0;
	// d2L/ds2dl
	info_matrix[vars][vars+1] = info_matrix[vars+1][vars] = trace / sigma2;
	
	release(&lag_X);
	
	const int k = vars;
	double const ybar = y.sum() / n;
	
	double sum_y2 = 0.0;
	double R2;
	double ee = rsd.norm();
	
	
	if (!InclConstant) {
		double e_bar = 0;
		for (cnt = 0;cnt < n;cnt++)
			e_bar += rsd.getValue(cnt);
		e_bar /= n;
		
		
		double e2 = 0;
		for (cnt = 0; cnt < n;cnt++)
		{
			e2  += geoda_sqr(rsd.getValue(cnt)-e_bar);
			sum_y2  += geoda_sqr(y.getValue(cnt));
		}
		R2 = 1.0 - (e2 / sum_y2);
	} else {
		sum_y2 = 0.0;
		for (cnt = 0; cnt < n;cnt++) sum_y2 += geoda_sqr(y.getValue(cnt)-ybar);
		
		R2 = 1.0 - (ee / sum_y2);
	}
	if (fabs(R2) > 1.0 || R2 < 0) R2 = 0.0;
	
	rr->SetR2Fit(R2);
	rr->SetR2Adjust(1.0 - ((n-1) * ((1.0 - R2) / (n-k))));
	
	
	double const lik = LogLike;
	//  double const lik = -1.0*((n/2.0)*(log(2.0*M_PI))+(n/2.0)*log((ee/n))+(ee/(2.0*(ee/n))));
	double const aic = -2.0*lik + 2.0*k; // # Akaike AIC
	double const sc  = -2.0*lik + k*log((double)n); // # Schwartz SC 
	
	rr->SetLIK(lik);
	rr->SetAIC(aic); // # Akaike AIC
	rr->SetSC(sc); // # Schwartz SC 
	
	double LRtest = 2.0 * (lik - likOLS);
	rr->SetLR_Test(0,1.0);
	rr->SetLR_Test(1,LRtest);
	rr->SetLR_Test(2,gammp(0.5,LRtest * 0.5));
	
	rr->SetSigSq(sigma2);
	// invert information matrix in place
	if (!SymMatInverse(info_matrix, vars+2)) return false;
	for (cnt = 0; cnt < vars; ++cnt)  
	{
		double std = sqrt(info_matrix[cnt][cnt]);
		double zval= egls.getValue(cnt)/std;
		rr->SetCoeff(cnt,egls.getValue(cnt));
		rr->SetStdError(cnt,std);
		rr->SetZValue(cnt,zval);
		rr->SetProbVal(cnt,2.0*(1.0-nc(fabs(zval))));
	};
	double ste = sqrt( info_matrix[vars][vars] );
	double zval= lambda/ste;
	rr->SetCoeff(cnt,lambda);
	rr->SetStdError(cnt,ste);
	rr->SetZValue(cnt,zval);
	rr->SetProbVal(cnt,2.0*(1.0-nc(fabs(zval))));
	
	for (cnt = 0; cnt < vars+1; ++cnt)  
	{
		for (int j=cnt;j<vars+1;j++)
		{
			rr->SetCovar(cnt,j,info_matrix[cnt][j]);
			rr->SetCovar(j,cnt,info_matrix[j][cnt]);
		}
	}
	
	double* r = rsd.getThis();
	double *bp = BP_Test(r,n, XX, k, InclConstant);
	
	if (bp == NULL) {
		rr->SetBPTest(0,k-1);
		rr->SetBPTest(1,0.0);
		rr->SetBPTest(2,-1.0);
		
	} else {
		rr->SetBPTest(0,bp[1]);
		rr->SetBPTest(1,bp[0]);
		rr->SetBPTest(2,bp[2]);
		
	}
	
	double w = (n-1.0)*geoda_sqr(lambda) / (1.0-geoda_sqr(lambda));
	rr->SetWaldTest(0,k-1);
	rr->SetWaldTest(1, w);
	rr->SetWaldTest(2,gammp((k-1)/2.0,w/2.0));
	
	
	for (cnt = 0; cnt < vars+2; ++cnt) delete [] info_matrix[cnt];
	delete [] info_matrix;
	info_matrix = NULL;
	
	return true;
}


