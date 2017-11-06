

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
    
    static void randomize(vector<vector<double> >& matrix) {
        int n = matrix.size();
        int k = matrix[0].size();
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
    
    static void svd(vector<vector<double> >& matrix, vector<vector<double> >& svecs, vector<double>& svals)
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
        eigen(K, temp, svals);
        
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
    
    /*
    
    public static double[][] landmarkMatrix(double[][] matrix)
    {
        int k = matrix.length;
        int n = matrix[0].length;
        double[][] result = new double[k][k];
        int[] index = landmarkIndices(matrix);
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < k; j++) {
                result[i][j] = matrix[i][index[j]];
            }
        }
        return result;
    }
    
    public static double[][] pivotRows(double[][] matrix, int k)
    {
        int K = matrix.length;
        if (k >= K) {
            return matrix;
        }
        int n = matrix[0].length;
        System.out.println(n + " " + k + " " + K);
        double[][] result = new double[k][n];
        int pivot = 0;
        double[] min = new double[n];
        for (int i = 0; i < n; i++)
            min[i] = Double.MAX_VALUE;
        for (int i = 0; i < k; i++) {
            int argmax = 0;
            for (int j = 0; j < n; j++) {
                result[i][j] = matrix[i][pivot];
                min[j] = Math.min(min[j], result[i][j]);
                if (min[j] > min[argmax]) {
                    argmax = j;
                }
            }
            pivot = argmax;
        }
        return result;
    }
    
    public static void scale(double[][] x, double[][] D)
    {
        int n = x[0].length;
        int d = x.length;
        double xysum = 0.0D;
        double dsum = 0.0D;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < i; j++) {
                double dxy = 0.0D;
                for (int k = 0; k < d; k++) dxy += Math.pow(x[k][i] - x[k][j], 2.0D);
                xysum += Math.sqrt(dxy);
                dsum += D[i][j];
            }
        }
        dsum /= xysum;
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < d; k++) { x[k][i] *= dsum;
            }
        }
    }
    
    public static double[][] maxminPivotMatrix(double[][] matrix, int k)
    {
        int n = matrix[0].length;
        double[][] result = new double[k][n];
        int pivot = 0;
        double[] min = new double[n];
        for (int i = 0; i < n; i++) min[i] = Double.MAX_VALUE;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                result[i][j] = distance(matrix, pivot, j);
            }
            pivot = 0;
            for (int j = 0; j < n; j++) {
                min[j] = Math.min(min[j], result[i][j]);
                if (min[j] > min[pivot]) pivot = j;
            }
        }
        return result;
    }
  
    public static double[][] randomPivotMatrix(double[][] matrix, int k)
    {
        int n = matrix[0].length;
        double[][] result = new double[k][n];
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
