#define NODEBUG
#include <iostream>
#ifdef DEBUG
  #include <iterator>
#endif
#include <Eigen/SVD>
#include "cluster.h"
#include "mds.h"

using namespace std;
using namespace Eigen;

MDS::MDS(int nrows, int ncolumns, double** data, int** mask, double weight[], int transpose, char dist, double** distmatrix, int low_dim)
{
    
    distmatrix = NULL;
    _transpose = 0;
}

MDS::~MDS()
{
  _xXf.resize(0, 0);
  _x.clear();
}

void MDS::clearDistMatrix()
{
    if (distmatrix) {
        for (int i = 1; i < num_obs; i++)
            delete[] distmatrix[i];
        delete[] distmatrix;
        distmatrix = NULL;
    }
}

bool MDS::compute()
{
    
    /* Calculate the distance matrix if the user didn't give it */
    distmatrix = distancematrix(nrows, ncolumns, _data, _mask, _weight, _dist, _transpose);
        if (!distmatrix) return false; /* Insufficient memory */
    }
    
    double** E = new double*[num_obs];
    for (int i = 0; i < num_obs; i++) {
        E[i] = new double[num_obs];
        for (int j=0; j<num_obs; j++) {
            E[i][j] = 0;
        }
    }

    double sum_E = 0, avg_E = 0;
    /* Calculate the distances and save them in the ragged array */
    /*  E = (-0.5 * d**2) */
    for (i = 1; i < n; i++) {
        for (j = 0; j < i; j++) {
            E[i][j]= -0.5 * distmatrix[i][j] *  distmatrix[i][j];
            E[j][i] = E[i][j];
        }
    }
    for (i = 1; i < n; i++)
        for (j = 0; j < i; j++)
            sum_E += E[i][j];
    sum_E += sum_E;
    avg_E = sum_E / (double)n;
    printf("sumE:%f\n", sum_E);
    
    /* Er = mat(mean(E,1)) */
    double* Er = (double*)malloc(n*sizeof(double));
    if(Er==NULL) return NULL; /* Not enough memory available */
    for (i=0; i<n; i++) {
        double row_sum;
        for (j=0;j<n;j++) {
            row_sum += E[i][j];
        }
        Er[i] = row_sum / (double) n;
    }
    
    /* Es = mat(mean(E,0)) */
    double* Es = (double*)malloc(n*sizeof(double));
    if(Es==NULL) return NULL; /* Not enough memory available */
    for (i=0; i<n; i++) {
        double col_sum;
        for (j=0;j<n;j++) {
            col_sum += E[j][i];
        }
        Es[i] = col_sum / (double) n;
    }
    
    /* # From Principles of Multivariate Analysis: A User's Perspective (page 107). */
    /* F = array(E - transpose(Er) - Es + mean(E)) */
    double** F;
    F = (double**)malloc(n*sizeof(double*));
    if(F==NULL) return NULL; /* Not enough memory available */
    F[0] = NULL;
    /* The zeroth row has zero columns. We allocate it anyway for convenience.*/
    for (i = 0; i < n; i++)
    { F[i] = (double*)malloc(n*sizeof(double));
        if (F[i]==NULL) break; /* Not enough memory available */
    }
    if (i < n) /* break condition encountered */
    { j = i;
        for (i = 0; i < j; i++) free(F[i]);
        return NULL;
    }
    
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            F[i][j] = E[i][j] - Er[j] - Es[j] + avg_E;
        }
    }
    
    /* [U, S, V] = svd(F) */
    double** V;
    double* S;
    double** Y;
    
    V = (double**)malloc(n*sizeof(double*));
    if(V==NULL) return NULL; /* Not enough memory available */
    V[0] = NULL;
    for (i = 0; i < n; i++)
    { V[i] = (double*)malloc(n*sizeof(double));
        if (V[i]==NULL) break; /* Not enough memory available */
    }
    if (i < n) /* break condition encountered */
    { j = i;
        for (i = 0; i < j; i++) free(V[i]);
        return NULL;
    }
    
    S = (double*)malloc(n*sizeof(double));
    if(S==NULL) return NULL; /* Not enough memory available */
    
    Y = (double**)malloc(n*sizeof(double*));
    if(Y==NULL) return NULL; /* Not enough memory available */
    Y[0] = NULL;
    for (i = 0; i < n; i++)
    { Y[i] = (double*)malloc(low_dim*sizeof(double));
        if (Y[i]==NULL) break; /* Not enough memory available */
    }
    if (i < n) /* break condition encountered */
    { j = i;
        for (i = 0; i < j; i++) free(Y[i]);
        return NULL;
    }
    
    printf("F[0][0]=%f,F[0][1]=%f\n", F[0][0], F[0][1]);
    int error = svd(nrows, ncolumns, F, S, V);
    if (error==0)
    {
        for (i=0;i<n;i++) S[i] = sqrt(S[i]);
        /*  U = F */
        /*  Y = U * sqrt(S) */
        for (i=0;i<n;i++)
            for (j=0;j<n;j++)
                F[i][j] = F[i][j] * S[j];
        
        /* return (Y[:,0:dimensions], S) */
        for (i=0;i<n;i++)
            for (j=0;j<low_dim;j++)
                Y[i][j] = F[i][j];
    }
    printf("Y[0][0]=%f,Y[0][1]=%f\n", Y[0][0], Y[0][1]);
    for (i = 0; i < n; i++) free(E[i]);
    for (i = 0; i < n; i++) free(F[i]);
    for (i = 0; i < n; i++) free(V[i]);
    free(E);
    free(F);
    free(V);
    free(Er);
    free(Es);
    free(S);
    
    return Y;
}
