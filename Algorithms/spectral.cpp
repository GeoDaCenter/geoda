// Modified by: lixun910@gmail.com July 20 2017
// Originally from: Spectral clustering, by Tim Nugent 2014

#define NODEBUG
#include <iostream>
#ifdef DEBUG
#include <iterator>
#endif

#include <map>
#include <math.h>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/QR>

#include "cluster.h"
#include "spectral.h"

using namespace Eigen;
using namespace std;


/**
 * Performs eigenvector decomposition of an affinity matrix
 *
 * @param data 		the affinity matrix
 * @param numDims	the number of dimensions to consider when clustering
 */
SpectralClustering::SpectralClustering(double** input_data, int nrows, int  ncols, int numDims):
mNumDims(numDims),
mNumClusters(0),
method('a'), dist('e'), npass(10), n_maxiter(300)
{
    Eigen::MatrixXd data;
    data.resize(nrows, ncols);
    for (unsigned int i = 0; i < nrows; ++i) {
        for (unsigned int j = 0; j < ncols; ++j) {
            data(i, j) = input_data[i][j];
        }
    }
    Eigen::MatrixXd Deg = Eigen::MatrixXd::Zero(data.rows(),data.cols());
    
    // calc normalised laplacian
    for ( int i=0; i < data.cols(); i++) {
        Deg(i,i)=1/(sqrt((data.row(i).sum())) );
    }
    Eigen::MatrixXd Lapla = Deg * data * Deg;
    
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> s(Lapla, true);
    Eigen::VectorXd val = s.eigenvalues();
    Eigen::MatrixXd vec = s.eigenvectors();
    
    //sort eigenvalues/vectors
    int n = data.cols();
    for (int i = 0; i < n - 1; ++i) {
        int k;
        val.segment(i, n - i).maxCoeff(&k);
        if (k > 0) {
            std::swap(val[i], val[k + i]);
            vec.col(i).swap(vec.col(k + i));
        }
    }
    
    //choose the number of eigenvectors to consider
    if (mNumDims < vec.cols()) {
        mEigenVectors = vec.block(0,0,vec.rows(),mNumDims);
    } else {
        mEigenVectors = vec;
    }
}

SpectralClustering::~SpectralClustering() {
}

/**
 * Cluster by kmeans
 *
 * @param numClusters	the number of clusters to assign
 */
void SpectralClustering::clusterKmeans(int numClusters) {
    mNumClusters = numClusters;
    //return Kmeans::cluster(mEigenVectors, numClusters);
    int rows = mEigenVectors.rows();
    int columns = mEigenVectors.cols();
    
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++){ weight[j] = 1;}
    
    // init input_data[rows][cols]
    double** input_data = new double*[rows];
    int** mask = new int*[rows];
    for (int i=0; i<rows; i++) {
        input_data[i] = new double[columns];
        mask[i] = new int[columns];
        for (int j=0; j<columns; j++){
            input_data[i][j] = mEigenVectors(i,j);
            mask[i][j] = 1;
        }
    }
    
    double error;
    int ifound;
    
    kcluster(numClusters, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, NULL, 0);
    
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


void Spectral::set_data(double** input_data, int nrows, int  ncols)
{
    X.resize(nrows, ncols);
    for (unsigned int i = 0; i < nrows; ++i) {
        for (unsigned int j = 0; j < ncols; ++j) {
            X(i, j) = input_data[i][j];
        }
    }
}

double Spectral::kernel(const VectorXd& a, const VectorXd& b){
   
    //http://scikit-learn.org/stable/modules/generated/sklearn.cluster.SpectralClustering.html
    //  gamma : float, default=1.0 (Radial basis function kernel)
    // Kernel coefficient for rbf, poly, sigmoid, laplacian and chi2 kernels. Ignored for
    // affinity='nearest_neighbors
    switch(kernel_type){
        case 1  :
            return(pow(a.dot(b)+constant,order));
        default :
            return(exp(-gamma*((a-b).squaredNorm())));
    }
    
}

void Spectral::generate_kernel_matrix(){
    
    // Fill kernel matrix
    K.resize(X.rows(),X.rows());
    for(unsigned int i = 0; i < X.rows(); i++){
        for(unsigned int j = i; j < X.rows(); j++){
            K(i,j) = K(j,i) = kernel(X.row(i),X.row(j));
            //if(i == 0) cout << K(i,j) << " ";
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

void Spectral::eigendecomposition(){
    
    //Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> edecomp(K, true);
    
    EigenSolver<MatrixXd> edecomp(K);
    eigenvalues = edecomp.eigenvalues().real();
    eigenvectors = edecomp.eigenvectors().real();
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
    
    /*
     cout << "Sorted eigenvalues:" << endl;
     for(unsigned int i = 0; i < eigenvalues.rows(); i++){
     if(eigenvalues(i) > 0){
     cout << "PC " << i+1 << ": Eigenvalue: " << eigenvalues(i);
     printf("\t(%3.3f of variance, cumulative =  %3.3f)\n",eigenvalues(i)/eigenvalues.sum(),cumulative(i)/eigenvalues.sum());
     //cout << eigenvectors.col(i) << endl;
     }
     }
     cout << endl;
     */
    MatrixXd tmp = eigenvectors;
    
    // Select top K eigenvectors where K = centers
    eigenvectors = tmp.block(0,0,tmp.rows(),centers);
    
}


void Spectral::cluster(){
    
    generate_kernel_matrix();
    eigendecomposition();
    kmeans();
    
}

void Spectral::kmeans()
{
    int rows = eigenvectors.rows();
    int columns = eigenvectors.cols();
    
    int transpose = 0; // row wise
    int* clusterid = new int[rows];
    double* weight = new double[columns];
    for (int j=0; j<columns; j++){ weight[j] = 1;}
    
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
    
    kcluster(centers, rows, columns, input_data, mask, weight, transpose, npass, n_maxiter, method, dist, clusterid, &error, &ifound, NULL, 0);
    
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
