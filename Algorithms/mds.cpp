

#include <stdlib.h>
#include <math.h> 

#include "DataUtils.h"
#include "mds.h"

AbstractMDS::AbstractMDS(int _n, int _dim)
{
    n = _n;
    dim = _dim;
    result.resize(dim);
    for (int i=0; i<dim; i++) result.resize(n);
}
AbstractMDS::~AbstractMDS()
{
}

vector<vector<double> >& AbstractMDS::GetResult()
{
    return result;
}

void AbstractMDS::fullmds(vector<vector<double> >& d, int dim)
{
    int k = d.size();
    int n = d[0].size();
    
    DataUtils::doubleCenter(d);
    DataUtils::squareEntries(d);
    DataUtils::multiply(d, -0.5);
    
    DataUtils::randomize(result);
    vector<double> evals(dim);
    
    DataUtils::eigen(d, result, evals);
    for (int i = 0; i < dim; i++) {
        evals[i] = sqrt(evals[i]);
        for (int j = 0; j < k; j++) {
            result[i][j] *= evals[i];
        }
    }
}

vector<double> AbstractMDS::pivotmds(vector<vector<double> >& input, vector<vector<double> >& result)
{
    int k = input.size();
    int n = input[0].size();
   
    result.empty();
    result.resize(k);
    for (int i=0; i<k; i++) result.resize(n);
    
    vector<double> evals(k);
    DataUtils::doubleCenter(input);
    DataUtils::multiply(input, -0.5);
    DataUtils::svd(input, result, evals);
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            result[i][j] *= sqrt(evals[i]);
        }
    }
    return evals;
}

FastMDS::FastMDS(vector<vector<double> >& distances, int dim)
: AbstractMDS(distances.size(), dim)
{
    int k = distances.size();
    result.resize(dim);
    for (int i=0; i<dim; i++) result[i].resize(k);
}

FastMDS::~FastMDS()
{
}

/*
public static double[] lmds(double[][] P, double[][] result)
{
    double[][] distances = new double[P.length][P[0].length];
    for (int i = 0; i < distances.length; i++) {
        for (int j = 0; j < distances[0].length; j++) {
            distances[i][j] = P[i][j];
        }
    }
    Data.squareEntries(distances);
    int k = distances.length;
    int n = distances[0].length;
    int d = result.length;
    
    double[] mean = new double[n];
    for (int i = 0; i < n; i++) for (int j = 0; j < k; j++) mean[i] += distances[j][i];
    for (int i = 0; i < n; i++) { mean[i] /= k;
    }
    double[] lambda = new double[d];
    double[][] temp = new double[d][k];
    Data.randomize(temp);
    
    
    double[][] K = Data.landmarkMatrix(P);
    Data.squareEntries(K);
    Data.doubleCenter(K);
    Data.multiply(K, -0.5D);
    eigen(K, temp, lambda);
    for (int i = 0; i < temp.length; i++) {
        for (int j = 0; j < temp[0].length; j++) {
            temp[i][j] *= Math.sqrt(lambda[i]);
        }
    }
    
    
    for (int m = 0; m < d; m++) {
        for (int i = 0; i < n; i++) {
            result[m][i] = 0.0D;
            for (int j = 0; j < k; j++) {
                result[m][i] -= 0.5D * (distances[j][i] - mean[i]) * temp[m][j] / lambda[m];
            }
        }
    }
    return lambda;
}

public static double[][] classicalScaling(double[][] d, int dim)
{
    int n = d[0].length;
    double[][] dist = new double[d.length][d[0].length];
    for (int i = 0; i < d.length; i++) {
        for (int j = 0; j < d[0].length; j++) {
            dist[i][j] = d[i][j];
        }
    }
    double[][] result = new double[dim][n];
    Data.randomize(result);
    ClassicalScaling.lmds(dist, result);
    return result;
}

public static double[][] classicalScaling(double[][] d)
{
    return classicalScaling(d, 2);
}

public static double[][] stressMinimization(double[][] d, double[][] w)
{
    return stressMinimization(d, w, 2);
}

public static double[][] stressMinimization(double[][] d, int dim)
{
    double[][] x = classicalScaling(d, dim);
    StressMinimization sm = new StressMinimization(d, x);
    sm.iterate(0, 0, 5);
    return x;
}

public static double[][] stressMinimization(double[][] d, double[][] w, int dim)
{
    double[][] x = classicalScaling(d, dim);
    StressMinimization sm = new StressMinimization(d, x, w);
    sm.iterate(0, 0, 3);
    return x;
}

public static double[][] stressMinimization(double[][] d)
{
    return stressMinimization(d, 2);
}
*/
