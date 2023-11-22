

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

std::vector<std::vector<double> >& AbstractMDS::GetResult()
{
    return result;
}

void AbstractMDS::fullmds(std::vector<std::vector<double> >& d, int dim, int maxiter)
{
    int k = d.size();
    int n = d[0].size();
    
    DataUtils::doubleCenter(d);
    DataUtils::squareEntries(d);
    DataUtils::multiply(d, -0.5);
    
    DataUtils::randomize(result);
    std::vector<double> evals(dim);
    
    DataUtils::eigen(d, result, evals, maxiter);
    for (int i = 0; i < dim; i++) {
        evals[i] = sqrt(evals[i]);
        for (int j = 0; j < k; j++) {
            result[i][j] *= evals[i];
        }
    }
}

std::vector<double> AbstractMDS::pivotmds(std::vector<std::vector<double> >& input, std::vector<std::vector<double> >& result)
{
    int k = input.size();
    int n = input[0].size();
   
    result.empty();
    result.resize(k);
    for (int i=0; i<k; i++) result.resize(n);
    
    std::vector<double> evals(k);
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

FastMDS::FastMDS(std::vector<std::vector<double> >& distances, int dim, int maxiter)
: AbstractMDS(distances.size(), dim)
{
    int k = distances.size();
    result.resize(dim);
    for (int i=0; i<dim; i++) result[i].resize(k);
    
    result = classicalScaling(distances, dim, maxiter);
}

FastMDS::~FastMDS()
{
}

std::vector<std::vector<double> > FastMDS::classicalScaling(std::vector<std::vector<double> >& d, int dim, int maxiter)
{
    std::vector<std::vector<double> > dist = d; // deep copy
    int n = d[0].size();
    /*
    std::vector<std::vector<double> > dist(d.size());
    for(int i=0; i<d.size(); i++) dist[i].resize(n);
    for (int i = 0; i < d.size(); i++) {
        for (int j = 0; j < d[0].size(); j++) {
            dist[i][j] = d[i][j];
        }
    }*/
    
    std::vector<std::vector<double> > result(dim);
    for (int i=0; i<dim; i++) result[i].resize(n);
    DataUtils::randomize(result);
    
    std::vector<double> lambds = lmds(dist, result, maxiter);
    return result;
}

std::vector<double> FastMDS::lmds(std::vector<std::vector<double> >& P, std::vector<std::vector<double> >& result, int maxiter)
{
    
    std::vector<std::vector<double> > distances = P; // deep copy
    /*
    std::vector<std::vector<double> > distances(P.size());
    for (int i=0; i<P.size(); i++) distances[i].resize(P[0].size());
    
    for (int i = 0; i < distances.size(); i++) {
        for (int j = 0; j < distances[0].size(); j++) {
            distances[i][j] = P[i][j];
        }
    }
     */
    DataUtils::squareEntries(distances);
    int k = distances.size();
    int n = distances[0].size();
    int d = result.size();
    
    std::vector<double> mean(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < k; j++) mean[i] += distances[j][i];
    for (int i = 0; i < n; i++)  mean[i] /= k;
    
    std::vector<double> lambda(d);
    std::vector<std::vector<double> > temp(d);
    for (int i=0; i<d; i++) temp[i].resize(k);
    DataUtils::randomize(temp);
    
    std::vector<std::vector<double> > K = DataUtils::landmarkMatrix(P);
    //DataUtils::squareEntries(K);
    DataUtils::doubleCenter(K);
    DataUtils::multiply(K, -0.5);
    
    std::vector<std::vector<double> > E = K;
    
    DataUtils::eigen(K, temp, lambda, maxiter);
    for (int i = 0; i < temp.size(); i++) {
        for (int j = 0; j < temp[0].size(); j++) {
            temp[i][j] *= sqrt(lambda[i]);
        }
    }
   
    //result = temp;
    for (int m = 0; m < d; m++) {
        for (int i = 0; i < n; i++) {
            result[m][i] = temp[m][i];
        }
    }
    return lambda;
}

/*
std::vector<std::vector<double> > classicalScaling(std::vector<std::vector<double> > d, int dim)
{
    int n = d[0].size();
    std::vector<std::vector<double> > dist = new double[d.size()][d[0].size()];
    for (int i = 0; i < d.size(); i++) {
        for (int j = 0; j < d[0].size(); j++) {
            dist[i][j] = d[i][j];
        }
    }
    std::vector<std::vector<double> > result = new double[dim][n];
    DataUtils::randomize(result);
    ClassicalScaling.lmds(dist, result);
    return result;
}

std::vector<std::vector<double> > classicalScaling(std::vector<std::vector<double> > d)
{
    return classicalScaling(d, 2);
}

std::vector<std::vector<double> > stressMinimization(std::vector<std::vector<double> > d, std::vector<std::vector<double> > w)
{
    return stressMinimization(d, w, 2);
}

std::vector<std::vector<double> > stressMinimization(std::vector<std::vector<double> > d, int dim)
{
    std::vector<std::vector<double> > x = classicalScaling(d, dim);
    StressMinimization sm = new StressMinimization(d, x);
    sm.iterate(0, 0, 5);
    return x;
}

std::vector<std::vector<double> > stressMinimization(std::vector<std::vector<double> > d, std::vector<std::vector<double> > w, int dim)
{
    std::vector<std::vector<double> > x = classicalScaling(d, dim);
    StressMinimization sm = new StressMinimization(d, x, w);
    sm.iterate(0, 0, 3);
    return x;
}

std::vector<std::vector<double> > stressMinimization(std::vector<std::vector<double> > d)
{
    return stressMinimization(d, 2);
}
*/
