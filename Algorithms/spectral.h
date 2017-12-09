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

using namespace Eigen;
using namespace std;

class Spectral{
    
public:
    Spectral() : centers(2), kernel_type(1), normalise(1), max_iters(1000), gamma(0.001), constant(1.0), order(2.0), method('a'), dist('e'), npass(10), n_maxiter(300) {}
    explicit Spectral(MatrixXd& d) : centers(2), kernel_type(1), normalise(1), max_iters(1000), gamma(0.001), constant(1.0), order(2.0), method('a'), dist('e'), npass(10), n_maxiter(300) {X = d;}
    
    //void affinity_matrix(vector<vector<double> >& distances);
    void set_data(double** input_data, int nrows, int  ncols);
    void set_centers(const unsigned int i){centers = i;};
    void set_kernel(const unsigned int i){kernel_type = i;};
    void set_normalise(const unsigned int i){normalise = i;};
    void set_gamma(const double i){gamma = i;};
    void set_constant(const double i){constant = i;};
    void set_order(const double i){order = i;};
    void set_max_iters(const unsigned int i){max_iters = i;};
    void set_power_iters(const unsigned int i){power_iter = i;};
    void cluster(int maxiter=0);
    const std::vector<wxInt64> &get_assignments() const {return assignments;};
    
private:
    void generate_kernel_matrix();
    double kernel(const VectorXd& a, const VectorXd& b);
    void eigendecomposition();
    void fast_eigendecomposition();
    void kmeans();
    MatrixXd X, K, eigenvectors;
    VectorXd eigenvalues, cumulative;
    unsigned int centers, kernel_type, normalise, max_iters;
    double gamma, constant, order, delta;
    // parameters for KMeans
    char method, dist;
    int npass, n_maxiter; // max iteration of EM
    int power_iter;
    std::vector<wxInt64> assignments;
};

#endif
