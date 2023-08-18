#define NODEBUG
#include <iostream>
#ifdef DEBUG
  #include <iterator>
#endif
#include <Eigen/SVD>
#include "pca.h"

using namespace Eigen;

Pca::Pca(double** x,  const unsigned int &nrows, const unsigned int &ncols)
{
    _nrows      = 0;
    _ncols      = 0;
    // Variables will be scaled by default
    _is_center  = true;
    _is_scale   = true;
    // By default will be used singular value decomposition
    _method   = "svd";
    _is_corr  = false;
    
    _kaiser   = 0;
    _thresh95 = 1;
    
    _ncols = ncols;
    _nrows = nrows;

    // Convert vector to Eigen 2-dimensional matrix
    _xXf.resize(_nrows, _ncols);
    for (unsigned int i = 0; i < _nrows; ++i) {
        for (unsigned int j = 0; j < _ncols; ++j) {
            _xXf(i, j) = x[i][j];
        }
    }
}

Pca::~Pca(void)
{
    _xXf.resize(0, 0);
    _x.clear();
}

std::vector<float> Pca::sd(void) { return _sd; };
std::vector<float> Pca::prop_of_var(void) {return _prop_of_var; };
std::vector<float> Pca::cum_prop(void) { return _cum_prop; };
std::vector<float> Pca::scores(void) { return _scores; };
std::vector<unsigned int> Pca::eliminated_columns(void) { return _eliminated_columns; }
std::string Pca::method(void) { return _method; }
unsigned int Pca::kaiser(void) { return _kaiser; };
unsigned int Pca::thresh95(void) { return _thresh95; };
unsigned int Pca::ncols(void) { return _ncols; }
unsigned int Pca::nrows(void) { return _nrows; }
bool Pca::is_scale(void) {  return _is_scale; }
bool Pca::is_center(void) { return _is_center; }

int Pca::CalculateSVD()
{
    if ((1 == _ncols) || (1 == _nrows))
        return -1;
    
    float denom = static_cast<float>((_nrows > 1)? _nrows - 1: 1);
    
    // Singular Value Decomposition is on
    _method = "svd";
    JacobiSVD<MatrixXf> svd(_xXf, ComputeThinV);
    VectorXf eigen_singular_values = svd.singularValues();
    eigen_vectors = svd.matrixV();
    
    VectorXf tmp_vec = eigen_singular_values.array().square();
    float tmp_sum = tmp_vec.sum();
    tmp_vec /= tmp_sum;
    
    // PC's standard deviation and
    // PC's proportion of variance
    _kaiser = 0;
    unsigned int lim = (_nrows < _ncols) ? _nrows : _ncols;
    eigen_values.resize(lim);
    for (unsigned int i = 0; i < lim; ++i) {
        _sd.push_back(eigen_singular_values(i)/sqrt(denom));
        eigen_values[i] = _sd[i] * _sd[i];
        if (_sd[i] >= 1) {
            _kaiser = i + 1;
        }
        _prop_of_var.push_back(tmp_vec(i));
    }
    #ifdef DEBUG
      std::cout << "\n\nStandard deviations for PCs:\n";
      copy(_sd.begin(), _sd.end(),std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\n\nKaiser criterion: PC #" << _kaiser << std::endl;
    #endif
    tmp_vec.resize(0);
    
    // PC's cumulative proportion
    _thresh95 = 1;
    _cum_prop.push_back(_prop_of_var[0]); 
    for (unsigned int i = 1; i < _prop_of_var.size(); ++i) {
        _cum_prop.push_back(_cum_prop[i-1]+_prop_of_var[i]);
        if (_cum_prop[i] < 0.95) {
            _thresh95 = i+1;
        }
    }
    #ifdef DEBUG
      std::cout << "\nCumulative proportion:\n";
      copy(_cum_prop.begin(), _cum_prop.end(),std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\n\nThresh95 criterion: PC #" << _thresh95 << std::endl;
    #endif
    
    // Scores
    MatrixXf eigen_scores = _xXf * eigen_vectors;
    #ifdef DEBUG
      std::cout << "\n\nEigen vectors:\n" << eigen_vectors;
      std::cout << "\n\nRotated values (scores):\n" << eigen_scores;
    #endif
    _scores.reserve(eigen_scores.rows()*eigen_scores.cols());
    for (unsigned int i = 0; i < eigen_scores.rows(); ++i) {
        for (unsigned int j = 0; j < eigen_scores.cols(); ++j) {
            _scores.push_back(eigen_scores(i, j));
        }
    }
    eigen_scores.resize(0, 0);
    #ifdef DEBUG
      std::cout << "\n\nScores in vector:\n";
      copy(_scores.begin(), _scores.end(),std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\n";
    #endif
    return 0;
}

int Pca::Calculate()
{
    if ((1 == _ncols) || (1 == _nrows))
        return -1;
    // COR OR COV MATRICES ARE HERE
    /* Straight and simple: if the scales are similar use cov-PCA, if not, use corr-PCA; otherwise, you better have a defense for not. If in doubt, use an F-test for the equality of the variances (ANOVA). If it fails the F-test, use corr; otherwise, use cov.
     */
    _method = "eigen";
    
    // Calculate covariance matrix
    MatrixXf eigen_cov; // = MatrixXf::Zero(_ncols, _ncols);
    VectorXf sds;
    // (TODO) Should be weighted cov matrix, even if is_center == false
    eigen_cov = (1.0 /(_nrows/*-1*/)) * _xXf.transpose() * _xXf;
    // diagnal are the variances
    sds = eigen_cov.diagonal().array().sqrt();
    MatrixXf outer_sds = sds * sds.transpose();
#ifdef DEBUG
      std::cout << _xXf << std::endl;
      std::cout << eigen_cov << std::endl;
      std::cout << sds << std::endl;
      std::cout << outer_sds << std::endl;
#endif
    eigen_cov = eigen_cov.array() / outer_sds.array();
    outer_sds.resize(0, 0);
    // ?If data matrix is scaled, covariance matrix is equal to correlation matrix
    EigenSolver<MatrixXf> edc(eigen_cov);
    VectorXf eigen_eigenvalues = edc.eigenvalues().real();
    MatrixXf eigen_eigenvectors = edc.eigenvectors().real();
    #ifdef DEBUG
      std::cout << eigen_cov << std::endl;
      std::cout << std::endl << eigen_eigenvalues.transpose() << std::endl;
      std::cout << std::endl << eigen_eigenvectors << std::endl;
    #endif
    // The eigenvalues and eigenvectors are not sorted in any particular order.
    // So, we should sort them
    typedef std::pair<float, int> eigen_pair;
    std::vector<eigen_pair> ep;	
    for (unsigned int i = 0 ; i < _ncols; ++i) {
	    ep.push_back(std::make_pair(eigen_eigenvalues(i), i));
    }
    sort(ep.begin(), ep.end()); // Ascending order by default
    // Sort them all in descending order
    MatrixXf eigen_eigenvectors_sorted = MatrixXf::Zero(eigen_eigenvectors.rows(), eigen_eigenvectors.cols());
    VectorXf eigen_eigenvalues_sorted = VectorXf::Zero(_ncols);
    int colnum = 0;
    int i = ep.size()-1;
    for (; i > -1; i--) {
      eigen_eigenvalues_sorted(colnum) = ep[i].first;
      eigen_eigenvectors_sorted.col(colnum++) += eigen_eigenvectors.col(ep[i].second);
    }
    #ifdef DEBUG
      std::cout << std::endl << eigen_eigenvalues_sorted.transpose() << std::endl;
      std::cout << std::endl << eigen_eigenvectors_sorted << std::endl;
    #endif  
    // We don't need not sorted arrays anymore
    eigen_eigenvalues.resize(0);
    eigen_eigenvectors.resize(0, 0);
    
    _sd.clear(); _prop_of_var.clear(); _kaiser = 0;
    float tmp_sum = eigen_eigenvalues_sorted.sum();
    for (unsigned int i = 0; i < _ncols; ++i) {
      _sd.push_back(sqrt(eigen_eigenvalues_sorted(i)));
      if (_sd[i] >= 1) {
        _kaiser = i + 1;
      }
      _prop_of_var.push_back(eigen_eigenvalues_sorted(i)/tmp_sum);
    }
    #ifdef DEBUG
      std::cout << "\nStandard deviations for PCs:\n";
      copy(_sd.begin(), _sd.end(), std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\nProportion of variance:\n";
      copy(_prop_of_var.begin(), _prop_of_var.end(), std::ostream_iterator<float>(std::cout," ")); 
      std::cout << "\nKaiser criterion: PC #" << _kaiser << std::endl;
    #endif
    // PC's cumulative proportion
    _cum_prop.clear(); _thresh95 = 1;
    _cum_prop.push_back(_prop_of_var[0]);
    for (unsigned int i = 1; i < _prop_of_var.size(); ++i) {
      _cum_prop.push_back(_cum_prop[i-1]+_prop_of_var[i]);
      if (_cum_prop[i] < 0.95) {
        _thresh95 = i+1;
      }
    }  
    #ifdef DEBUG
      std::cout << "\n\nCumulative proportions:\n";
      copy(_cum_prop.begin(), _cum_prop.end(), std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\n\n95% threshold: PC #" << _thresh95 << std::endl;
    #endif
    eigen_values = eigen_eigenvalues_sorted;
    eigen_vectors = eigen_eigenvectors_sorted;
    // Scores for PCA with correlation matrix
    // Scale before calculating new values
    for (unsigned int i = 0; i < _ncols; ++i) {
     _xXf.col(i) /= sds(i);
    }
    sds.resize(0);
    MatrixXf eigen_scores = _xXf * eigen_eigenvectors_sorted;
    #ifdef DEBUG
      std::cout << "\n\nRotated values (scores):\n" << eigen_scores;
    #endif
    _scores.clear();
    _scores.reserve(_ncols*_nrows);
    for (unsigned int i = 0; i < _nrows; ++i) {
      for (unsigned int j = 0; j < _ncols; ++j) {
        _scores.push_back(eigen_scores(i, j));
      }
    }
    eigen_scores.resize(0, 0);
    #ifdef DEBUG
      std::cout << "\n\nScores in vector:\n";
      copy(_scores.begin(), _scores.end(), std::ostream_iterator<float>(std::cout," "));  
      std::cout << "\n";
    #endif
    return 0;
}


