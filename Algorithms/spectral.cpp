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
#include <Eigen/Core>
#include <Spectra/SymEigsShiftSolver.h>
#include <Spectra/SymEigsSolver.h>
// <Spectra/MatOp/DenseSymShiftSolve.h> is implicitly included
#include <iostream>

#include "../kNN/ANN/ANN.h"

#include "cluster.h"
#include "spectral.h"
#include "DataUtils.h"

using namespace Eigen;
using namespace Spectra;

Spectral::~Spectral()
{
    if (dist_util) {
        delete dist_util;
    }
}
void Spectral::set_data(double** input_data, int nrows, int  ncols)
{
    this->data = input_data;
    this->nrows = nrows;
    this->ncols = ncols;

    // create a DistUtils for KNN weights creation
    if (dist_util) {
        delete dist_util;
    }
    dist_util = new Gda::DistUtils(input_data, nrows, ncols, dist=='b'?1:2);
    
    X.resize(nrows, ncols);
    for (int i = 0; i < nrows; ++i) {
        for (int j = 0; j < ncols; ++j) {
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
    // sklearn: gamma = 1.0 / N,  gamma = 1/(2sigma^2) => sigma = sqrt(1/gamma) / 2.0;
    switch(kernel_type){
        case 1  :
            return(pow(a.dot(b)+constant,order));
        default :
            //return(exp(-gamma*((a-b).squaredNorm())));
            return exp(-((a-b).squaredNorm()/(2 * sigma * sigma)) );
    }
    
}

void Spectral::set_knn(const unsigned int k, bool is_mutual)
{
    this->is_mutual = is_mutual;
    bool is_inverse = false;
    double power = 1.0;

    Gda::Weights w = dist_util->CreateKNNWeights(k, is_inverse, power);
    K.setZero(nrows, nrows);
    for (int i=0; i<nrows; ++i) {
        for (int j=0; j<k; ++j) {
            int nbr = w[i][j].first;
            // KNN graph
            K(i, nbr) = 1;
        }
        K(i, i)  = 1; // same as sklearn, include self as neighbor
    }
}

void Spectral::affinity_matrix()
{
    // deprecated: classic way to fill Laplacian matrix
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
    d = normalize_laplacian(K);
}

VectorXd Spectral::normalize_laplacian(MatrixXd& L)
{
    // Normalise Laplacian: see scipy.sparse.csgraph.laplacian
    std::vector<bool> isolated_node_mask(L.size());
    VectorXd d = L.rowwise().sum() - L.diagonal();
    for(int i = 0; i < d.rows(); i++){
        if (d(i) == 0) {
            d(i) = 1;
            isolated_node_mask[i] = true;
        } else {
            d(i) = 1.0/sqrt(d(i));
            isolated_node_mask[i] = false;
        }
    }
    L = (d.asDiagonal() * L * d.asDiagonal()) * -1;
    for (int i=0; i<L.rows(); ++i) {
        L(i, i) = isolated_node_mask[i] ? 0 : 1;
    }
    return d;
}

void Spectral::generate_knn_matrix()
{
    // The following implementation is ported from sklearn
    // sklearn/cluster/_spectral.py#L160
    MatrixXd A = (K + K.transpose())/2.0; // Adjacency matrix
    if (is_mutual) {
        for (int i=0; i<A.rows(); ++i) {
            for (int j=i; j < A.rows(); ++j) {
                if (A(i,j) == 0.5) {
                    A(i,j) = 0;
                    A(j,i) = 0;
                }
            }
        }
    }
    // Normalise Laplacian
    d = normalize_laplacian(A);
    K = A;
}

void Spectral::arpack_eigendecomposition()
{
    // get largest eigenvalues for (I - K)
    //laplacian = _set_diag(laplacian, 1, norm_laplacian); laplacian *= -1
    for (int i=0; i<K.rows(); ++i) {
        K(i, i) = 1;
    }
    for (int i=0; i<K.rows(); ++i) {
        for (int j=0; j<K.rows(); ++j) {
            if (K(i,j) != 0)
                K(i,j) *= -1;
        }
    }
    for (int i=0; i<K.rows(); ++i) {
        for (int j=i; j<K.rows(); ++j) {
            if (K(i,j) != K(j,i)) {
                K(j, i) = K(i, j); // force symmetric, high precision issue
            }
        }
    }
    // eigendecomposition
    try {
        if(call_symeigshiftssolver(K) == false) {
            // try other method in arpack library
            if (call_symeigssolver(K) == false ) {
                // fall back to classic eigendecomposition
                eigendecomposition(false);
            }
        }
    } catch(const std::invalid_argument& ia) {
        //std::cerr << "Invalid argument: " << ia.what() << '\n';
        // try other method in arpack library
        if (call_symeigssolver(K) == false ) {
            // fall back to classic eigendecomposition
            eigendecomposition(false);
        }
    }

    for (int i=0; i<eigenvectors.cols(); ++i) {
        for (int j=0; j<eigenvectors.rows(); ++j) {
            eigenvectors(j,i) = eigenvectors(j,i) * d(j);
        }
    }
}

bool Spectral::call_symeigssolver(MatrixXd& L)
{
    DenseSymMatProd<double> op(L);
    SymEigsSolver< double, LARGEST_ALGE, DenseSymMatProd<double> > eigs(&op, centers, 2*centers);
    eigs.init();
    int nconv =  eigs.compute();
    if(eigs.info() == SUCCESSFUL) {
        eigenvalues = eigs.eigenvalues();
        eigenvectors = eigs.eigenvectors();
        return true;
    }
    return false;
}

bool Spectral::call_symeigshiftssolver(MatrixXd& L)
{
    // Construct matrix operation object using the wrapper class
    DenseSymShiftSolve<double> op(L);

    // L has eigenvalues between [0,2], so (I - L) has eigenvalues between [-1,1]
    double shift = 1.0;
    // Construct eigen solver object with shift 1 (the value of the shift)
    // This will find eigenvalues that are closest to 1
    SymEigsShiftSolver< double, LARGEST_MAGN, DenseSymShiftSolve<double> > eigs(&op, centers, 2*centers, shift);
    eigs.init();
    int nconv =  eigs.compute();
    if(eigs.info() == SUCCESSFUL) {
        eigenvalues = eigs.eigenvalues();
        eigenvectors = eigs.eigenvectors();
        return true;
    }
    return false;
}

static bool inline eigen_greater(const std::pair<double,VectorXd>& a, const std::pair<double,VectorXd>& b)
{
    return a.first > b.first;
}

void Spectral::eigendecomposition(bool raw_matrix)
{
    if (raw_matrix) {
        // get largest eigenvalues for (I - K)
        //K = MatrixXd::Identity(K.rows(), K.rows()) - K;
        for (int i=0; i<K.rows(); ++i) {
            K(i, i) = 1;
        }
        for (int i=0; i<K.rows(); ++i) {
            for (int j=0; j<K.rows(); ++j) {
                if (K(i,j) != 0)
                    K(i,j) *= -1;
            }
        }
        for (int i=0; i<K.rows(); ++i) {
            for (int j=i; j<K.rows(); ++j) {
                if (K(i,j) != K(j,i)) {
                    K(j, i) = K(i, j); // force symmetric, high precision issue
                }
            }
        }
    }

    // call classic SVD
    //Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> edecomp(K, true);
    EigenSolver<MatrixXd> edecomp(K);
    eigenvalues = edecomp.eigenvalues().real();
    eigenvectors = edecomp.eigenvectors().real();

    cumulative.resize(eigenvalues.rows());
    std::vector<std::pair<double,VectorXd> > eigen_pairs;
    double c = 0.0;
    for(unsigned int i = 0; i < eigenvectors.cols(); i++){
        if(normalise){
            double norm = eigenvectors.col(i).norm();
            eigenvectors.col(i) /= norm;
        }
        eigen_pairs.push_back(std::make_pair(eigenvalues(i),eigenvectors.col(i)));
    }
    // http://stackoverflow.com/questions/5122804/sorting-with-lambda
    sort(eigen_pairs.begin(),eigen_pairs.end(), eigen_greater);
    
    if(centers > eigen_pairs.size()) centers = (int)eigen_pairs.size();
    
    for(unsigned int i = 0; i < eigen_pairs.size(); i++){
        eigenvalues(i) = eigen_pairs[i].first;
        c += eigenvalues(i);
        cumulative(i) = c;
        eigenvectors.col(i) = eigen_pairs[i].second;
    }

    MatrixXd tmp = eigenvectors;
    
    // Select top K eigenvectors where K = centers
    eigenvectors = tmp.block(0,0,tmp.rows(),centers);
}


void Spectral::cluster(int affinity_type)
{
    if (affinity_type == 0) {
        // kernel
        generate_kernel_matrix();
        
    } else {
        // KNN
        generate_knn_matrix();
    }

    arpack_eigendecomposition();
    
    kmeans();
}

void Spectral::kmeans()
{
    int rows = (int)eigenvectors.rows();
    int columns = (int)eigenvectors.cols();
    
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
        srand((int)GdaConst::gda_user_seed);
        s1 = rand();
    }
    if (s1 > 0) {
        s2 = s1 + rows;
        for (int i = 0; i < rows; i++) uniform(s1, s2);
    }
    kcluster(centers, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, NULL, 0, s1, s2);

    // clean memory
    for (int i=0; i<rows; i++) {
        if (input_data[i]) delete[] input_data[i];
        if (mask[i]) delete[] mask[i];
        assignments.push_back(clusterid[i] + 1);
        //clusters_undef.push_back(ifound == -1);
    }
    if (input_data) delete[] input_data;
    if (weight) delete[] weight;
    if (clusterid) delete[] clusterid;
    if (mask) delete[] mask;
    
    input_data = NULL;
    weight = NULL;
    clusterid = NULL;
    mask = NULL;
}
