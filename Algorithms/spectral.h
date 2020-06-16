// Modified by: lixun910@gmail.com
// Originally from: Spectral clustering, by Tim Nugent 2014

#ifndef __GEODA_CENTER_SPECTRAL_H
#define __GEODA_CENTER_SPECTRAL_H

#undef max
#include <vector>
#ifdef Success
#undef Success
#endif

#include <wx/wx.h>
#include <string>
#include <map>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include "../Weights/DistUtils.h"

using namespace Eigen;
using namespace std;

class Spectral{
    
public:
    Spectral() : centers(2), kernel_type(1), normalise(1), max_iters(1000),
      sigma(0.001), constant(1.0), order(2.0), method('a'), dist('e'),
      npass(10), n_maxiter(300), dist_util(NULL) {}
    
    Spectral(MatrixXd& d) : centers(2), kernel_type(1), normalise(1),
      max_iters(1000), sigma(0.001), constant(1.0), order(2.0), method('a'),
      dist('e'), npass(10), n_maxiter(300), dist_util(NULL) {X = d;}

    virtual ~Spectral();

    void set_data(double** input_data, int nrows, int ncols);
    void set_centers(const unsigned int i){centers = i;}
    
    void set_knn(const unsigned int k, bool is_mutual=false);
    void set_kernel(const unsigned int i){kernel_type = i;}
    void set_sigma(const double i){sigma = i;}
                    
    void set_normalise(const unsigned int i){normalise = i;}
    void set_constant(const double i){constant = i;}
    void set_order(const double i){order = i;}
    void set_max_iters(const unsigned int i){max_iters = i;}
    void set_power_iters(const unsigned int i){power_iter = i;}

    void set_kmeans_dist(char d) { dist = d;}
    void set_kmeans_method(char m) { method = m;}
    void set_kmeans_npass(int n) { npass = n; }
    void set_kmeans_maxiter(int n) { n_maxiter = n;}

    void cluster(int affinity_type=0);
    const std::vector<wxInt64> &get_assignments() const {return assignments;}
    
    MatrixXd X, K, eigenvectors;
    
private:
    void affinity_matrix();
    void generate_kernel_matrix();
    double kernel(const VectorXd& a, const VectorXd& b);

    VectorXd normalize_laplacian(MatrixXd& L);

    void generate_knn_matrix();
    
    void eigendecomposition();
    void arpack_eigendecomposition();
    
    void kmeans();

    VectorXd d; // for KNN affinity matrix, diagnoal W
    VectorXd eigenvalues, cumulative;
    unsigned int centers, kernel_type, normalise, max_iters, knn;
    double sigma, constant, order;
    // parameters for KMeans
    char method, dist;
    int npass, n_maxiter; // max iteration of EM
    int power_iter;
    std::vector<wxInt64> assignments;

    double** data;
    int nrows;
    int ncols;
    bool is_mutual;
    Gda::DistUtils* dist_util;
};

#endif
