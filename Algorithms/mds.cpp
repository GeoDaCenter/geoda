

#include <stdlib.h>
#include <math.h> 

#include "DataUtils.h"
#include "mds.h"

AbstractMDS::AbstractMDS(int _dim)
{
    dim = _dim;
    result = NULL;
}
AbstractMDS::~AbstractMDS()
{
    if (result) {
        for (int i=0; i<dim; i++) delete[] result[i];
        delete[] result;
        result = NULL;
    }
}

double** AbstractMDS::GetResult()
{
    return result;
}

double** AbstractMDS::fullmds(double** d, int n, int k, int dim)
{
    // int k = d.length
    // int n = d[0].length
    //double[][] result = new double[dim][d.length];
    double** result = new double*[dim];
    for (int i=0; i<dim; i++) result[i] = new double[k];
    
    DataUtils::doubleCenter(d, n, k);
    DataUtils::squareEntries(d, n, k);
    DataUtils::multiply(d, -0.5, n, k);
    
    DataUtils::randomize(result, k, dim);
    double* evals = new double[dim];
    DataUtils::eigen(d, result, evals, dim, k);
    /*
    for (int i = 0; i < dim; i++) {
        evals[i] = sqrt(evals[i]);
        for (int j = 0; j < k; j++) {
            result[i][j] *= evals[i];
        }
    }
     */
    return result;
}

double* AbstractMDS::pivotmds(double** input, int n, double** result, int k)
{
    double* evals = new double[k];
    DataUtils::doubleCenter(input, n, n);
    DataUtils::multiply(input, -0.5, n, n);
    DataUtils::svd(input, result, evals);
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            result[i][j] *= sqrt(evals[i]);
        }
    }
    return evals;
}

FastMDS::FastMDS(double** distances, int n, int k, int _dim)
: AbstractMDS(_dim)
{
    result = fullmds(distances, n, k, dim);
}

FastMDS::~FastMDS()
{
    if (result) {
        for (int i=0; i<dim; i++) delete[] result[i];
        delete[] result;
        result = NULL;
    }
}
