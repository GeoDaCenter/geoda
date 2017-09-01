#ifndef __GEODA_MDS_H__
#define __GEODA_MDS_H__

#include <vector>
#ifdef Success
#undef Success
#endif
#include <Eigen/Dense>

class MDS {
    
public:
  std::vector<float> scores(void);
    
  Eigen::MatrixXf eigen_vectors;
  Eigen::VectorXf eigen_values;
    
    MDS(int nrows, int ncolumns, double** data, int** mask,
        double weight[], int transpose, char dist, double** distmatrix, int low_dim);
    
    ~MDS(void);
    
protected:
    
    int num_obs;
    
    int nrows;
    
    int ncolumns;
    
    double** _data;
    
    double** _mask;
    
    double* _weight;
    
    char _dist;
    
    int _transpose;
    
    double** distmatrix;
    
   
    bool compute();
    
    void clearDistMatrix();
    
  std::vector<float>  _x;   // Initial matrix as vector filled by rows.
  Eigen::MatrixXf     _xXf; // Initial matrix as Eigen MatrixXf structure
  unsigned int  _nrows,     // Number of rows in matrix x.
                _ncols;     // Number of cols in matrix x.
    std::vector<float>  results;  // Standard deviation of each component
    
};

#endif
