

#ifndef __GEODA_CENTER_DATAUTILS_H
#define __GEODA_CENTER_DATAUTILS_H

#include <iostream>
#include <vector>
#include <cfloat>
#include <stdlib.h>
#include <math.h> 
#include <cmath>
#include <algorithm>    // std::max

#include "threadpool.h"
#include "rng.h"
#include "../GdaConst.h"
#include "../ShapeOperations/GalWeight.h"
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// DistMatrix
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DistMatrix
{
protected:
    std::vector<int> ids;
    bool has_ids;
public:
    DistMatrix(const std::vector<int>& _ids=std::vector<int>())
    : ids(_ids), has_ids(!_ids.empty()) {}
    virtual ~DistMatrix() {}
    // Get distance between i-th and j-th object
    // if ids vector is provided, the distance (i,j) -> distance(ids[i], ids[j])
    virtual double getDistance(int i, int j) = 0;
    virtual void setIds(const std::vector<int>& _ids) {
        ids = _ids;
        has_ids = !ids.empty();
    }
};

/*
class RowWiseDistMatrix : public DistMatrix
{
    double** dist;
    char dist_method;
    int num_vars;
    int num_rows;
    const std::vector<std::vector<double> >& data;
public:
    RowWiseDistMatrix(const double** const input_data,
                      const std::vector<int>& _ids=std::vector<int>())
    : data(_data), DistMatrix(_ids)
    {
        thread_pool pool;
        for (size_t i=0; i<num_rows; ++i) {
            pool.enqueue(boost::bind(&ComputeDistMatrix::compute_dist, this, i));
        }
    }
    virtual ~ComputeDistMatrix() {}

    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        if (has_ids) {
            i = ids[i];
            j = ids[j];
        }
        // lower part triangle, store column wise
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        int idx = n - (num_obs - c - 1) * (num_obs - c) / 2 + (r -c) -1 ;
        return dist[idx];
    }

    void compute_dist(size_t i)
    {
        for (size_t j=i+1; j < num_rows; ++j) {
            double val = 0, var_dist = 0;
            if (dist_method == 'm') {
                for (size_t v=0; v < num_vars; ++v) {
                    val += fabs(data[v][i] - data[v][j]);
                }
                var_dist = sqrt(val);
            } else {
                // euclidean as default 'e'
                for (size_t v=0; v < num_vars; ++v) {
                    val = data[v][i] - data[v][j];
                    var_dist += val * val;
                }
                var_dist = sqrt(var_dist);
            }
        }
    }
};
*/

class RawDistMatrix : public DistMatrix
{
    double** dist;
public:
    RawDistMatrix(double** dist, const std::vector<int>& _ids=std::vector<int>())
    : DistMatrix(_ids), dist(dist) {}
    virtual ~RawDistMatrix() {}
    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        if (has_ids) {
            i = ids[i];
            j = ids[j];
        }
        // lower part triangle
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        return dist[r][c];
    }
};

class RDistMatrix : public DistMatrix
{
    int num_obs;
    int n;
    const std::vector<double>& dist;
public:
    RDistMatrix(int num_obs, const std::vector<double>& dist, const std::vector<int>& _ids=std::vector<int>())
    : DistMatrix(_ids), num_obs(num_obs), dist(dist) {
        n = (num_obs - 1) * num_obs / 2;
    }
    virtual ~RDistMatrix() {}

    virtual double getDistance(int i, int j) {
        if (i == j) return 0;
        if (has_ids) {
            i = ids[i];
            j = ids[j];
        }
        // lower part triangle, store column wise
        int r = i > j ? i : j;
        int c = i < j ? i : j;
        int idx = n - (num_obs - c - 1) * (num_obs - c) / 2 + (r -c) -1 ;
        return dist[idx];
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// DataUtils
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DataUtils {
public:
    static void Shuffle(std::vector<int>& arry, Xoroshiro128Random& rng)
    {
        //random_shuffle
        for (int i=arry.size()-1; i>=1; --i) {
            int k = rng.nextInt(i+1);
            while (k>=i) k = rng.nextInt(i+1);
            if (k != i) std::iter_swap(arry.begin() + k, arry.begin()+i);
        }
    }

    static double ManhattanDistance(const std::vector<std::vector<double> >& col_data, int p, int q)
    {
        double d =0;
        for (size_t i =0; i<col_data.size(); i++ ) {
            d += fabs(col_data[i][p] - col_data[i][q]);
        }
        return d;
    }

    static double ManhattanDistance(const std::vector<double>& x1, const std::vector<double>& x2)
    {
        double d =0;
        size_t size = x1.size();
        for (size_t i =0; i<size; i++ ) {
            d += fabs(x1[i] - x2[i]);
        }
        return d;
    }

    
    static double ManhattanDistance(double* x1, double* x2, size_t size, double* weight)
    {
        double d =0;
        for (size_t i =0; i<size; i++ ) {
            d += fabs(x1[i] - x2[i]) * weight[i];
        }
        return d;
    }

    static double EuclideanDistance(const std::vector<std::vector<double> >& col_data, int p, int q)
    {
        double d =0, tmp=0;
        for (size_t i =0; i<col_data.size(); i++ ) {
            tmp = (col_data[i][p] - col_data[i][q]);
            d += tmp * tmp;
        }
        return d;
    }

    static double EuclideanDistance(const std::vector<double>& x1, const std::vector<double>& x2)
    {
        double d =0,tmp=0;
        size_t size = x1.size();

        for (size_t i =0; i<size; i++ ) {
            tmp = (x1[i] - x2[i]);
            d += tmp * tmp;
        }
        return d; // squared
    }

    static double EuclideanDistance(double* x1, const std::vector<double>& x2)
    {
        double d =0,tmp=0;
        size_t size = x2.size();

        for (size_t i =0; i<size; i++ ) {
            tmp = (x1[i] - x2[i]);
            d += tmp * tmp;
        }
        return d; // squared
    }

    static double EuclideanDistance(double* x1, double* x2, size_t size, double* weight)
    {
        double d =0,tmp=0;
        for (size_t i =0; i<size; i++ ) {
            tmp = (x1[i] - x2[i]);
            if (weight) {
                d += tmp * tmp * weight[i];
            } else {
                d += tmp * tmp;
            }
        }
        return d;
    }
    
    static void doubleCenter(vector<vector<double> >& matrix)
    {
        int n = (int)matrix[0].size();
        int k = (int)matrix.size();
        
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
        int n = (int)matrix[0].size();
        int k = (int)matrix.size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] *= factor;
            }
        }
    }
    
    static void squareEntries(vector<vector<double> >& matrix)
    {
        int n = (int)matrix[0].size();
        int k = (int)matrix.size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] = pow(matrix[i][j], 2.0);
            }
        }
    }
    
    static double prod(vector<double> x, vector<double> y) {
        int n = (int)x.size();
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
            srand((int)GdaConst::gda_user_seed);
        }
        
        int d = (int)evals.size();
        int k = (int)matrix.size();
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
            srand((int)GdaConst::gda_user_seed);
        }
        double rho = largestEigenvalue(matrix);
        int d = (int)evals.size();
        int k = (int)matrix.size();
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
        int n = (int)matrix.size();
        double rho = largestEigenvalue(matrix);
        double eps = 1.0E-6;
        //int maxiter = 100;
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
        int n = (int)matrix.size();
        double eps = 1.0E-6;
        //int maxiter = 100;
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
            srand((int)GdaConst::gda_user_seed);
        }
        int k = (int)matrix.size();
        int n = (int)matrix[0].size();
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < n; j++) {
                matrix[i][j] = (double) rand() / RAND_MAX;
            }
        }
    }
    
    static vector<int> landmarkIndices(vector<vector<double> >& matrix) {
        int k = (int)matrix.size();
        int n = (int)matrix[0].size();
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
        int k = (int)matrix.size();
        int n = (int)matrix[0].size();
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
        
        for (int i = 0; i < k; i++) {
            copy[i] = new double[n];
            for (int j = 0; j < n; j++)
                copy[i][j] = 0;
        }
        
        for (int i = 1; i < k; i++) {
            for (int j = 0; j < i; j++) {
                if (isSqrt) copy[i][j] = sqrt(matrix[i][j]);
                copy[i][j] = matrix[i][j];
                copy[j][i] = copy[i][j];
            }
        }
        
        return copy;
    }

    static double** ComputeFullDistMatrix(double** data, double* weight,
                                          int rows, int columns
                                          , double dist(double* , double* , size_t, double*))
    {
        double** dist_matrix  = new double*[rows];
        for (size_t i=0; i<rows; ++i) {
            dist_matrix[i] = new double[rows];
        }
        for (size_t i=0; i<rows; ++i) {
            for (size_t j=i; j<rows; ++j) {
                if ( i == j ) {
                    dist_matrix[i][j] = 0;
                    continue;
                }
                dist_matrix[i][j] = dist(data[i], data[j], columns, weight);
                dist_matrix[i][j] = sqrt(dist_matrix[i][j]);
                dist_matrix[j][i] = dist_matrix[i][j] ;
            }
        }
        return dist_matrix;
    }

    // upper triangular part of a symmetric matrix
    static double* getPairWiseDistance(double** matrix, double* weight, int n, int k, double dist(double* , double* , size_t, double*))
    {
        unsigned long long _n = n;
        
        unsigned long long cnt = 0;
        unsigned long long nn = _n*(_n-1)/2;
        double* result = new double[nn];
        for (int i=0; i<n; i++) {
            for (int j=i+1; j<n; j++) {
                result[cnt++] = dist(matrix[i], matrix[j], k, weight);
            }
        }
        return result;
    }
    
    static double* getContiguityPairWiseDistance(GalElement* w, double** matrix, double* weight, int n, int k, double dist(double* , double* , size_t, double*))
    {
        unsigned long long _n = n;
        
        unsigned long long cnt = 0;
        unsigned long long nn = _n*(_n-1)/2;
        double* result = new double[nn];
        for (int i=0; i<n; i++) {
            for (int j=i+1; j<n; j++) {
                if (w[i].Check(j)) {
                    result[cnt++] = dist(matrix[i], matrix[j], k, weight);
                } else {
                    result[cnt++] = DBL_MAX;
                }
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
        int k = (int)d.size();
        int n = (int)d[0].size();
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
        int k = (int)matrix.size();
        int n = (int)matrix[0].size();
        int d = (int)svecs.size();
        
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
        int k = (int)matrix.size();
        
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
        int n = (int)x[0].size();
        int d = (int)x.size();
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
