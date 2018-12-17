// Modified by: lixun910@gmail.com July 20 2017
// Originally from: Spectral clustering, by Tim Nugent 2014

#define NODEBUG
#include <iostream>
#ifdef DEBUG
#include <iterator>
#endif

#include <map>
#include <math.h>
#include <boost/heap/priority_queue.hpp>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/QR>

#include "cluster.h"
#include "spectral.h"
#include "DataUtils.h"

using namespace Eigen;
using namespace std;


void Spectral::set_data(double** input_data, int nrows, int  ncols)
{
    X.resize(nrows, ncols);
    for (unsigned int i = 0; i < nrows; ++i) {
        for (unsigned int j = 0; j < ncols; ++j) {
            X(i, j) = input_data[i][j];
        }
    }
}

double Spectral::kernel(const VectorXd& a, const VectorXd& b)
{
    //http://scikit-learn.org/stable/modules/generated/sklearn.cluster.SpectralClustering.html
    //  gamma : float, default=1.0 (Radial basis function kernel)
    // Kernel coefficient for rbf, poly, sigmoid, laplacian and chi2 kernels. Ignored for
    // affinity='nearest_neighbors
    switch(kernel_type){
        case 1  :
            return(pow(a.dot(b)+constant,order));
        default :
            //return(exp(-gamma*((a-b).squaredNorm())));
            return exp(-((a-b).squaredNorm()/(2 * sigma * sigma)) );
    }
    
}

void Spectral::affinity_matrix()
{
    // Fill Laplacian matrix
    K.resize(X.rows(),X.rows());
    for(unsigned int i = 0; i < X.rows(); i++){
        for(unsigned int j = i; j < X.rows(); j++){
            K(i,j) = K(j,i) = kernel(X.row(i),X.row(j));
        }
    }
    
    // Normalise Laplacian
    VectorXd d = K.rowwise().sum();
    MatrixXd _D = d.asDiagonal();
    MatrixXd U = _D - K;
    
    for(unsigned int i = 0; i < d.rows(); i++){
        d(i) = 1.0/sqrt(d(i));
    }
    MatrixXd l = d.asDiagonal() * U * d.asDiagonal();
    K = l;
}

void Spectral::generate_kernel_matrix()
{
    // If you have an affinity matrix, such as a distance matrix,
    // for which 0 means identical elements, and high values means very
    // dissimilar elements, it can be transformed in a similarity matrix
    // that is well suited for the algorithm by applying
    // the Gaussian (RBF, heat) kernel:
    //    np.exp(- X ** 2 / (2. * delta ** 2))
    //    delta = X.maxCoeff() - X.minCoeff();
    
    // Fill kernel matrix
    K.resize(X.rows(),X.rows());
    for(unsigned int i = 0; i < X.rows(); i++){
        for(unsigned int j = i; j < X.rows(); j++){
            K(i,j) = K(j,i) = kernel(X.row(i),X.row(j));
        }
    }
    
    // Normalise kernel matrix
    VectorXd d = K.rowwise().sum();
    for(unsigned int i = 0; i < d.rows(); i++){
        d(i) = 1.0/sqrt(d(i));
    }
    MatrixXd l = (K * d.asDiagonal());
    for(unsigned int i = 0; i < l.rows(); i++){
        for(unsigned int j = 0; j < l.cols(); j++){
            l(i,j) = l(i,j) * d(i);
        }
    }
    K = l;
}

struct CompareDist
{
    bool operator()(const pair<int, double>& lhs, const pair<int, double>& rhs) const { return lhs.second > rhs.second;}
};

void Spectral::generate_knn_matrix()
{
    typedef boost::heap::priority_queue<pair<int, double>, boost::heap::compare<CompareDist> > PriorityQueue;
    
    // Fill euclidean dista matrix and filter using KNN
    K.resize(X.rows(),X.rows());
    double squared_dist = 0;
    for (unsigned int i = 0; i < X.rows(); i++){
        PriorityQueue top_K;
        for(unsigned int j = i; j < X.rows(); j++){
            squared_dist =  (X.row(i) - X.row(j)).norm();
            K(i,j) = K(j,i) = squared_dist;
            if (i != j) top_K.push(std::make_pair(j,squared_dist));
        }
        if (top_K.size() > knn) {
            double min_dist = 0;
            for (int j=0; j<knn; j++) {
                std::pair<int, double> item = top_K.top();
                top_K.pop();
                min_dist = item.second;
            }
            for(unsigned int j = i; j < X.rows(); j++){
                if (K(j,i) > min_dist) {
                    K(i,j) = K(j,i) = 0;
                }
            }
            for(unsigned int j = i; j < X.rows(); j++){
                if (K(j,i) != 0 ) {
                    K(i,j) = K(j,i) = 1;
                }
            }
        }
    }
    
    // Normalise kernel matrix
    VectorXd d = K.rowwise().sum();
    for(unsigned int i = 0; i < d.rows(); i++){
        d(i) = 1.0/sqrt(d(i));
    }
    MatrixXd l = (K * d.asDiagonal());
    for(unsigned int i = 0; i < l.rows(); i++){
        for(unsigned int j = 0; j < l.cols(); j++){
            l(i,j) = l(i,j) * d(i);
        }
    }
    K = l;
}

static bool inline eigen_greater(const pair<double,VectorXd>& a, const pair<double,VectorXd>& b)
{
    return a.first > b.first;
}

void Spectral::fast_eigendecomposition()
{
    // get top N = centers eigen values/vectors
    int n = K.rows();
    vector<vector<double> > matrix(n);
    for (int i=0; i< n; i++) {
        matrix[i].resize(n);
        for (int j=0; j<n; j++) matrix[i][j] = K(i,j);
    }
  
    vector<vector<double> > evecs(centers);
    for (int i=0; i<centers; i++) evecs[i].resize(n);
    DataUtils::randomize(evecs);
    vector<double> lambda(centers);
    DataUtils::eigen(matrix, evecs, lambda, power_iter);
    for (int i = 0; i < evecs.size(); i++) {
        for (int j = 0; j < evecs[0].size(); j++) {
            evecs[i][j] *= sqrt(lambda[i]);
        }
    }
   
    // normalise eigenvectors
    for (int i = 0; i < evecs.size(); i++) {
        double norm = 0;
        for (int j = 0; j < evecs[0].size(); j++) {
            norm += evecs[i][j] * evecs[i][j];
        }
        norm = sqrt(norm);
        for (int j = 0; j < evecs[0].size(); j++) {
            evecs[i][j] /= norm;
        }
    }
    
    eigenvectors.resize(n, centers);
    eigenvalues.resize(centers);
   
    for (int c=0; c<centers; c++) {
        for (int i=0; i<n; i++) {
            eigenvectors(i, c) = evecs[c][i];
        }
        eigenvalues[c] = lambda[c];
    }
}

void Spectral::eigendecomposition()
{
    
    //Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> edecomp(K, true);
    
    EigenSolver<MatrixXd> edecomp(K);
    eigenvalues = edecomp.eigenvalues().real();
    eigenvectors = edecomp.eigenvectors().real();
    for(unsigned int i = 0; i < eigenvalues.rows(); i++){
        cout << "Eigenvalue: " << eigenvalues(i) << endl;
    }
    cumulative.resize(eigenvalues.rows());
    vector<pair<double,VectorXd> > eigen_pairs;
    double c = 0.0;
    for(unsigned int i = 0; i < eigenvectors.cols(); i++){
        if(normalise){
            double norm = eigenvectors.col(i).norm();
            eigenvectors.col(i) /= norm;
        }
        eigen_pairs.push_back(make_pair(eigenvalues(i),eigenvectors.col(i)));
    }
    // http://stackoverflow.com/questions/5122804/sorting-with-lambda
    sort(eigen_pairs.begin(),eigen_pairs.end(), eigen_greater);
    
    if(centers > eigen_pairs.size()) centers = eigen_pairs.size();
    
    for(unsigned int i = 0; i < eigen_pairs.size(); i++){
        eigenvalues(i) = eigen_pairs[i].first;
        c += eigenvalues(i);
        cumulative(i) = c;
        eigenvectors.col(i) = eigen_pairs[i].second;
    }
     cout << "Sorted eigenvalues:" << endl;
     for(unsigned int i = 0; i < eigenvalues.rows(); i++){
         if(i<2){
             cout << "PC " << i+1 << ": Eigenvalue: " << eigenvalues(i);
             printf("\t(%3.3f of variance, cumulative =  %3.3f)\n",eigenvalues(i)/eigenvalues.sum(),cumulative(i)/eigenvalues.sum());
             cout << eigenvectors.col(i) << endl;
         }
     }
     cout << endl;
    MatrixXd tmp = eigenvectors;
    
    // Select top K eigenvectors where K = centers
    eigenvectors = tmp.block(0,0,tmp.rows(),centers);
    
}


void Spectral::cluster(int affinity_type)
{
    if (affinity_type == 0) {
        // kernel
        //affinity_matrix();
        generate_kernel_matrix();
        
    } else {
        // KNN
        generate_knn_matrix();
    }
    
    if (power_iter>0) {
        fast_eigendecomposition();
    } else {
        // try other method than eigen3, e.g. Intel MLK
        eigendecomposition();
    }
    kmeans();
}

void Spectral::kmeans()
{
    int rows = eigenvectors.rows();
    int columns = eigenvectors.cols();
    
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++) weight[j] = 1;
    
    // init input_data[rows][cols]
    double** input_data = new double*[rows];
    int** mask = new int*[rows];
    for (int i=0; i<rows; i++) {
        input_data[i] = new double[columns];
        mask[i] = new int[columns];
        for (int j=0; j<columns; j++){
            input_data[i][j] = eigenvectors(i,j);
            mask[i][j] = 1;
        }
    }
    
    double error;
    int ifound;
    int s1=0;
    int s2 =0;
    if (GdaConst::use_gda_user_seed) {
        srand(GdaConst::gda_user_seed);
        s1 = rand();
    }
    if (s1 > 0) {
        s2 = s1 + rows;
        for (int i = 0; i < rows; i++) uniform(s1, s2);
    }
    kcluster(centers, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, NULL, 0, s1, s2);
    
    //vector<bool> clusters_undef;
    
    // clean memory
    for (int i=0; i<rows; i++) {
        delete[] input_data[i];
        delete[] mask[i];
        assignments.push_back(clusterid[i] + 1);
        //clusters_undef.push_back(ifound == -1);
    }
    delete[] input_data;
    delete[] weight;
    delete[] clusterid;
    delete[] mask;
    
    input_data = NULL;
    weight = NULL;
    clusterid = NULL;
    mask = NULL;
}
