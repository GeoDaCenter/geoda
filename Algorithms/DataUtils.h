

#ifndef __GEODA_CENTER_DATAUTILS_H
#define __GEODA_CENTER_DATAUTILS_H

#include <vector>
#include <stdlib.h>
#include <math.h> 
#include <cmath>
#include <algorithm>    // std::max

using namespace std;


class DataUtils {
public:
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
    
    static double normalize(vector<double> x) {
        double norm = sqrt(prod(x, x));
        for (int i = 0; i < x.size(); i++) x[i] /= norm;
        return norm;
    }
    
    static void normalize(vector<vector<double> >& x)
    {
        for (int i = 0; i < x.size(); i++) normalize(x[i]);
    }
    
    static void eigen(vector<vector<double> >& matrix, vector<vector<double> >& evecs, vector<double>& evals) {
        int d = evals.size();
        int k = matrix.size();
        double eps = 1.0E-6;
        int maxiter = 100;
        for (int m = 0; m < d; m++) {
            if (m > 0)
                for (int i = 0; i < k; i++)
                    for (int j = 0; j < k; j++)
                        matrix[i][j] -= evals[(m - 1)] * evecs[(m - 1)][i] * evecs[(m - 1)][j];
            for (int i = 0; i < k; i++)
                evecs[m][i] = (double) rand() / RAND_MAX;
            normalize(evecs[m]);
            
            double r = 0.0;
            
            for (int iter = 0; (fabs(1.0 - r) > eps) && (iter < 100); iter++) {
                double* q = new double[k];
            for (int i = 0; i < n; i++) q[i] = 0;
                for (int i = 0; i < k; i++) {
                    for (int j = 0; j < k; j++)
                        q[i] += matrix[i][j] * evecs[m][j];
                }
                evals[m] = prod(evecs[m], q);
                normalize(q);
                r = abs(prod(evecs[m], q));
                delete[] evecs[m];
                evecs[m] = q;
            }
        }
    }
  
    static double largestEigenvalue(vector<vector<double> >& matrix, int n)
    {
        double eps = 1.0E-6;
        int maxiter = 100;
        double lambda = 0.0;
        double* x = new double[n];
        for (int i = 0; i < n; i++) {
            x[i] = 1.0;
        }
        double r = 0.0;
        
        for (int iter = 0; (fabs(1.0 - r) > eps) && (iter < 100); iter++) {
            double* q = new double[n];
            for (int i = 0; i < n; i++) q[i] = 0;
            
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++)
                    q[i] += matrix[i][j] * x[j];
            }
            lambda = prod(x, q, k);
            normalize(q, k);
            r = fabs(prod(x, q, k));
            delete[] x;
            x = q;
        }
        delete[] x;
        return lambda;
    }
    
    static double smallestEigenvalue(vector<vector<double> >& matrix, int n)
    {
        double rho = largestEigenvalue(matrix, n);
        double eps = 1.0E-6;
        int maxiter = 100;
        double lambda = 0.0;
        double* x = new double[n];
        for (int i = 0; i < n; i++)
            x[i] = (0.5 - (double) rand() / RAND_MAX);
        normalize(x, n);
        
        double r = 0.0;
        
        for (int iter = 0; (abs(1.0 - r) > eps) && (iter < 100); iter++) {
            double* q = new double[n];
            for (int i = 0; i < n; i++) q[i] = 0;
            
            for (int i = 0; i < n; i++) {
                q[i] -= rho * x[i];
                for (int j = 0; j < n; j++)
                    q[i] += matrix[i][j] * x[j];
            }
            lambda = prod(x, q, n);
            normalize(q, n);
            r = fabs(prod(x, q, n));
            delete[] x;
            x = q;
        }
        delete[] x;
        return lambda + rho;
    }
    
    static void randomize(vector<vector<double> >& matrix, int n, int k) {
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] = (double) rand() / RAND_MAX;
            }
        }
    }
    
    static int* landmarkIndices(vector<vector<double> >& matrix, int n, int k) {
        //int k = matrix.length;
        //int n = matrix[0].length;
        int* result = new int[k];
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                if (matrix[i][j] == 0.0) {
                    result[i] = j;
                }
            }
        }
        return result;
    }
    
    static double** copyMatrix(vector<vector<double> >& matrix, int n, int k) {
        double** copy = new double*[k];
        for (int i=0; i<k; i++) copy[i] = new double[n];
        
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                copy[i][j] = matrix[i][j];
            }
        }
        return copy;
    }
    
    static double** copyRaggedMatrix(vector<vector<double> >& matrix, int n, int k) {
        double** copy = new double*[k];
        for (int i=0; i<k; i++) copy[i] = new double[n];
       
        
        for (int i = 1; i < k; i++)
            for (int j = 0; j < n; j++)
                copy[i][j] = 0;
            
        for (int i = 1; i < k; i++)
            for (int j = 0; j < i; j++) {
                copy[i][j] = matrix[i][j];
                copy[j][i] = matrix[i][j];
            }
            
        return copy;
    }
   
    static void selfprod(double** d, int n, int k, double** result)
    {
        //int k = d.length;
        //int n = d[0].length;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j <= i; j++) {
                double sum = 0.0;
                for (int m = 0; m < n; m++) sum += d[i][m] * d[j][m];
                result[i][j] = sum;
                result[j][i] = sum;
            }
        }
    }
    
    static void svd(vector<vector<double> >& matrix, vector<vector<double> >& svecs, vector<double>& svals)
    {
        int k = matrix.size();
        int n = matrix[0].size();
        int d = svecs.size();
        
        for (int m = 0; m < d; m++) svals[m] = normalize(svecs[m]);
        double[][] K = new double[k][k];
        
        Data.selfprod(matrix, K);
        double[][] temp = new double[d][k];
        
        
        for (int m = 0; m < d; m++) {
            for (int i = 0; i < k; i++) {
                for (int j = 0; j < n; j++) {
                    temp[m][i] += matrix[i][j] * svecs[m][j];
                }
            }
        }
        for (int m = 0; m < d; m++) svals[m] = Data.normalize(svecs[m]);
        eigen(K, temp, svals);
        double[][] tempOld = new double[d][k];
        for (int m = 0; m < d; m++)
            for (int i = 0; i < k; i++)
                for (int j = 0; j < k; j++)
                    tempOld[m][j] += K[i][j] * temp[m][i];
        for (int m = 0; m < d; m++) { svals[m] = Data.normalize(tempOld[m]);
        }
        
        for (int m = 0; m < d; m++) {
            svals[m] = Math.sqrt(svals[m]);
            for (int i = 0; i < n; i++) {
                svecs[m][i] = 0.0D;
                for (int j = 0; j < k; j++) {
                    svecs[m][i] += matrix[j][i] * temp[m][j];
                }
            }
        }
        for (int m = 0; m < d; m++) { Data.normalize(svecs[m]);
        }
    }
};

#endif
