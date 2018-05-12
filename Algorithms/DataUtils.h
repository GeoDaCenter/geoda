

#ifndef __GEODA_CENTER_DATAUTILS_H
#define __GEODA_CENTER_DATAUTILS_H

#include <iostream>
#include <vector>
#include <cfloat>
#include <stdlib.h>
#include <math.h> 
#include <cmath>
#include <algorithm>    // std::max

#include "../GdaConst.h"
using namespace std;

class DataUtils {
public:
    static double ManhattanDistance(double* x1, double* x2, size_t size)
    {
        double d =0;
        for (size_t i =0; i<size; i++ ) {
            d += fabs(x1[i] - x2[i]);
        }
        return d;
    }
    
    static double EuclideanDistance(double* x1, double* x2, size_t size)
    {
        double d =0,tmp=0;
        for (size_t i =0; i<size; i++ ) {
            tmp = x1[i] - x2[i];
            d += tmp * tmp;
        }
        return d;
    }
    
    static void doubleCenter(vector<vector<double> >& matrix)
    {
        int n = matrix[0].size();
        int k = matrix.size();
        
        for (int j = 0; j < k; j++) {
            double avg = 0.0;
            for (int i = 0; i < n; i++) avg += matrix[j][i];
            avg /= n;
            for (int i = 0; i < n; i++) matrix[j][i] -= avg;
        }
        
        for (int i = 0; i < n; i++) {
            double avg = 0.0;
            for (int j = 0; j < k; j++) avg += matrix[j][i];
            avg /= k;
            for (int j = 0; j < k; j++) matrix[j][i] -= avg;
        }
    }
    
    static void multiply(vector<vector<double> >& matrix, double factor)
    {
        int n = matrix[0].size();
        int k = matrix.size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] *= factor;
            }
        }
    }
    
    static void squareEntries(vector<vector<double> >& matrix)
    {
        int n = matrix[0].size();
        int k = matrix.size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] = pow(matrix[i][j], 2.0);
            }
        }
    }
    
    static double prod(vector<double> x, vector<double> y) {
        int n = x.size();
        double val = 0;
        for (int i=0; i<n; i++) {
            val += x[i] * y[i];
        }
        return val;
    }
    
    static double normalize(vector<double>& x) {
        double norm = sqrt(prod(x, x));
        for (int i = 0; i < x.size(); i++) x[i] /= norm;
        return norm;
    }
    
    static void normalize(vector<vector<double> >& x)
    {
        for (int i = 0; i < x.size(); i++) normalize(x[i]);
    }
    
    static void eigen(vector<vector<double> >& matrix, vector<vector<double> >& evecs, vector<double>& evals, int maxiter) {
        
        if ( GdaConst::use_gda_user_seed) {
            srand(GdaConst::gda_user_seed);
        }
        
        int d = evals.size();
        int k = matrix.size();
        //double eps = 1.0E-10;
        double eps = GdaConst::gda_eigen_tol;
        for (int m = 0; m < d; m++) {
            if (m > 0)
                for (int i = 0; i < k; i++)
                    for (int j = 0; j < k; j++)
                        matrix[i][j] -= evals[(m - 1)] * evecs[(m - 1)][i] * evecs[(m - 1)][j];
            for (int i = 0; i < k; i++)
                evecs[m][i] = (double) rand() / RAND_MAX;
            normalize(evecs[m]);
            
            double r = 0.0;
            
            for (int iter = 0; (fabs(1.0 - r) > eps) && (iter < maxiter); iter++) {
            //for (int iter = 0; iter < maxiter; iter++) {
                vector<double> q(k,0);
                for (int i = 0; i < k; i++) {
                    for (int j = 0; j < k; j++)
                        q[i] += matrix[i][j] * evecs[m][j];
                }
                evals[m] = prod(evecs[m], q);
                normalize(q);
                r = abs(prod(evecs[m], q));
                evecs[m] = q;
            }
        }
    }
    
    static void reverse_eigen(vector<vector<double> >& matrix, vector<vector<double> >& evecs, vector<double>& evals, int maxiter) {
        
        if ( GdaConst::use_gda_user_seed) {
            srand(GdaConst::gda_user_seed);
        }
        double rho = largestEigenvalue(matrix);
        int d = evals.size();
        int k = matrix.size();
        double eps = 1.0E-6;
        for (int m = 0; m < d; m++) {
            if (m > 0)
                for (int i = 0; i < k; i++)
                    for (int j = 0; j < k; j++)
                        matrix[i][j] -= evals[(m - 1)] * evecs[(m - 1)][i] * evecs[(m - 1)][j];
            for (int i = 0; i < k; i++)
                evecs[m][i] = (double) rand() / RAND_MAX;
            normalize(evecs[m]);
            
            double r = 0.0;
            
            for (int iter = 0; (fabs(1.0 - r) > eps) && (iter < maxiter); iter++) {
                vector<double> q(k,0);
                for (int i = 0; i < k; i++) {
                    q[i] -= rho * evecs[m][i];
                    for (int j = 0; j < k; j++)
                        q[i] += matrix[i][j] * evecs[m][j];
                }
                evals[m] = prod(evecs[m], q);
                normalize(q);
                r = abs(prod(evecs[m], q));
                evecs[m] = q;
            }
        }
    }
 
    static double smallestEigenvalue(vector<vector<double> >& matrix)
    {
        int n = matrix.size();
        double rho = largestEigenvalue(matrix);
        double eps = 1.0E-6;
        int maxiter = 100;
        double lambda = 0.0;
        vector<double> x(n);
        for (int i = 0; i < n; i++)
            x[i] = (0.5 - (double) rand() / RAND_MAX);
        normalize(x);
        
        double r = 0.0;
        
        for (int iter = 0; (abs(1.0 - r) > eps) && (iter < 100); iter++) {
            vector<double> q(n,0);
            
            for (int i = 0; i < n; i++) {
                q[i] -= rho * x[i];
                for (int j = 0; j < n; j++)
                    q[i] += matrix[i][j] * x[j];
            }
            lambda = prod(x, q);
            normalize(q);
            r = fabs(prod(x, q));
            x = q;
        }
        return lambda + rho;
    }
    
    static double largestEigenvalue(vector<vector<double> >& matrix)
    {
        int n = matrix.size();
        double eps = 1.0E-6;
        int maxiter = 100;
        double lambda = 0.0;
        vector<double> x(n,1.0);
        double r = 0.0;
        
        for (int iter = 0; (fabs(1.0 - r) > eps) && (iter < 100); iter++) {
            vector<double> q(n,0);
            
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++)
                    q[i] += matrix[i][j] * x[j];
            }
            lambda = prod(x, q);
            normalize(q);
            r = fabs(prod(x, q));
            x = q;
        }
        return lambda;
    }
    
    static void randomize(vector<vector<double> >& matrix) {
        if ( GdaConst::use_gda_user_seed) {
            srand(GdaConst::gda_user_seed);
        }
        int k = matrix.size();
        int n = matrix[0].size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] = (double) rand() / RAND_MAX;
            }
        }
    }
    
    static vector<int> landmarkIndices(vector<vector<double> >& matrix) {
        int k = matrix.size();
        int n = matrix[0].size();
        vector<int> result(k);
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                if (matrix[i][j] == 0.0) {
                    result[i] = j;
                }
            }
        }
        return result;
    }
    
    static vector<vector<double> > copyMatrix(vector<vector<double> >& matrix) {
        int k = matrix.size();
        int n = matrix[0].size();
        vector<vector<double> > copy(k);
        
        for (int i = 0; i < k; i++) {
            copy[i].resize(n);
            for (int j = 0; j < n; j++) {
                copy[i][j] = matrix[i][j];
            }
        }
        return copy;
    }
    
    static double** fullRaggedMatrix(double** matrix, int n, int k, bool isSqrt=false) {
        double** copy = new double*[k];
        
        //for (int i = 0; i < k; i++) {
            //copy[i] = new double[n];
            //for (int j = 0; j < n; j++)
            //    copy[i][j] = 0;
        //}
        
        for (int i = 1; i < k; i++) {
            copy[i] = new double[n];
            for (int j = 0; j < i; j++) {
                if (isSqrt) copy[i][j] = sqrt(matrix[i][j]);
                copy[i][j] = matrix[i][j];
                copy[j][i] = copy[i][j];
            }
        }
        
        return copy;
    }
    
    static double* getPairWiseDistance(double** matrix, int n, int k, double dist(double* , double* , size_t))
    {
        unsigned long long _n = n;
        
        unsigned long long cnt = 0;
        unsigned long long nn = _n*(_n-1)/2;
        double* result = new double[nn];
        for (int i=0; i<n; i++) {
            for (int j=i+1; j<n; j++) {
                result[cnt++] = dist(matrix[i], matrix[j], k);
            }
        }
        return result;
    }
    
    static vector<vector<double> > copyRaggedMatrix(double** matrix, int n, int k) {
        vector<vector<double> > copy(k);
        
        for (int i = 0; i < k; i++) {
            copy[i].resize(n);
            for (int j = 0; j < n; j++)
                copy[i][j] = 0;
        }
        
        for (int i = 1; i < k; i++) {
            for (int j = 0; j < i; j++) {
                copy[i][j] = matrix[i][j];
                copy[j][i] = matrix[i][j];
            }
        }
        
        return copy;
    }
   
    static void selfprod(vector<vector<double> >& d, vector<vector<double> >& result)
    {
        int k = d.size();
        int n = d[0].size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j <= i; j++) {
                double sum = 0.0;
                for (int m = 0; m < n; m++) sum += d[i][m] * d[j][m];
                result[i][j] = sum;
                result[j][i] = sum;
            }
        }
    }
    
    static void svd(vector<vector<double> >& matrix, vector<vector<double> >& svecs, vector<double>& svals, int maxiter=100)
    {
        int k = matrix.size();
        int n = matrix[0].size();
        int d = svecs.size();
        
        for (int m = 0; m < d; m++) svals[m] = normalize(svecs[m]);
        vector<vector<double> > K(k);
        for (int i=0; i<k; i++) K[i].resize(k);
        
        selfprod(matrix, K);
        vector<vector<double> > temp(d);
        for (int i=0; i<d; i++) temp[i].resize(k);
        
        for (int m = 0; m < d; m++) {
            for (int i = 0; i < k; i++) {
                for (int j = 0; j < n; j++) {
                    temp[m][i] += matrix[i][j] * svecs[m][j];
                }
            }
        }
        for (int m = 0; m < d; m++) svals[m] = normalize(svecs[m]);
        eigen(K, temp, svals, maxiter);
        
        vector<vector<double> > tempOld(d);
        for (int i=0; i<d; i++) tempOld[i].resize(k);
        
        for (int m = 0; m < d; m++)
            for (int i = 0; i < k; i++)
                for (int j = 0; j < k; j++)
                    tempOld[m][j] += K[i][j] * temp[m][i];
        for (int m = 0; m < d; m++) {
            svals[m] = normalize(tempOld[m]);
        }
        
        for (int m = 0; m < d; m++) {
            svals[m] = sqrt(svals[m]);
            for (int i = 0; i < n; i++) {
                svecs[m][i] = 0.0;
                for (int j = 0; j < k; j++) {
                    svecs[m][i] += matrix[j][i] * temp[m][j];
                }
            }
        }
        for (int m = 0; m < d; m++) {
            normalize(svecs[m]);
        }
    }
    
    
    static vector<vector<double> > landmarkMatrix(vector<vector<double> >& matrix)
    {
        int k = matrix.size();
        int n = matrix[0].size();
        
        vector<vector<double> > result(k);
        for (int i=0; i<k; i++) result[i].resize(k);
        
        vector<int> index = landmarkIndices(matrix);
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < k; j++) {
                result[i][j] = matrix[i][index[j]];
            }
        }
        return result;
    }
   
    /*
    static vector<vector<double> > pivotRows(vector<vector<double> >& matrix, int k)
    {
        int K = matrix.size();
        if (k >= K) {
            return matrix;
        }
        int n = matrix[0].size();
        //System.out.println(n + " " + k + " " + K);
        vector<vector<double> > result(k);
        for (int i=0; i<n; i++) result[i].resize(n);
        
        int pivot = 0;
        vector<double> _min(n);
        for (int i = 0; i < n; i++)
            _min[i] = DBL_MAX;
        for (int i = 0; i < k; i++) {
            int argmax = 0;
            for (int j = 0; j < n; j++) {
                result[i][j] = matrix[i][pivot];
                _min[j] = std::min(_min[j], result[i][j]);
                if (_min[j] > _min[argmax]) {
                    argmax = j;
                }
            }
            pivot = argmax;
        }
        return result;
    }*/
    
    static void scale(vector<vector<double> >& x, vector<vector<double> >& D)
    {
        int n = x[0].size();
        int d = x.size();
        double xysum = 0.0;
        double dsum = 0.0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < i; j++) {
                double dxy = 0.0;
                for (int k = 0; k < d; k++) dxy += pow(x[k][i]-x[k][j], 2.0);
                xysum += sqrt(dxy);
                dsum += D[i][j];
            }
        }
        dsum /= xysum;
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < d; k++) x[k][i] *= dsum;
        }
    }
    
    /*
    static vector<vector<double> > maxminPivotMatrix(vector<vector<double> >& matrix, int k)
    {
        int n = matrix[0].size();
        vector<vector<double> > result(k);
        for (int i=0; i<n; i++) result[i].resize(n);
        int pivot = 0;
        vector<double> min(n);
        for (int i = 0; i < n; i++) min[i] = DBL_MAX;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                result[i][j] = distance(matrix, pivot, j);
            }
            pivot = 0;
            for (int j = 0; j < n; j++) {
                min[j] = std::min(min[j], result[i][j]);
                if (min[j] > min[pivot]) pivot = j;
            }
        }
        return result;
    }
  
    static vector<vector<double> > randomPivotMatrix(vector<vector<double> >& matrix, int k)
    {
        int n = matrix[0].size();
        vector<vector<double> > result(k);
        for (int i=0; i<n; i++) result[i].resize(n);
     
        boolean[] isPivot = new boolean[n];
        int pivot = 0;
        for (int i = 0; i < k; i++) {
            do {
                pivot = (int)(Math.random() * n);
            } while (
                     
                     isPivot[pivot] != 0);
            isPivot[pivot] = true;
            for (int j = 0; j < n; j++) {
                result[i][j] = distance(matrix, pivot, j);
            }
        }
        return result;
    }
    */
};

#endif
