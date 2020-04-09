#ifndef SMACOF_H
#define SMACOF_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef lapack_int
#define lapack_int long int
#endif

#ifndef LAPACKE_malloc
#define LAPACKE_malloc( size ) malloc( size )
#endif
#ifndef LAPACKE_free
#define LAPACKE_free( p )      free( p )
#endif

#define LAPACK_C2INT( x ) (lapack_int)(*((float*)&x ))
#define LAPACK_Z2INT( x ) (lapack_int)(*((double*)&x ))

#define LAPACK_ROW_MAJOR               101
#define LAPACK_COL_MAJOR               102

#define LAPACK_WORK_MEMORY_ERROR       -1010
#define LAPACK_TRANSPOSE_MEMORY_ERROR  -1011

static inline int VINDEX(const int);
static inline int MINDEX(const int, const int, const int);
static inline int SINDEX(const int, const int, const int);
static inline int TINDEX(const int, const int, const int);
static inline int AINDEX(const int, const int, const int, const int, const int);

static inline double SQUARE(const double);
static inline double THIRD(const double);
static inline double FOURTH(const double);
static inline double FIFTH(const double);

//static inline double MAX(const double, const double);
//static inline double MIN(const double, const double);
static inline int IMIN(const int, const int);
static inline int IMAX(const int, const int);
static inline int IMOD(const int, const int);

void dposv(const int *, const int *, double *, double *);
void dsyevd(const int *, double *, double *);
void dgeqrf(const int *, const int *, double *, double *);
void dorgqr(const int *, const int *, double *, double *);
void dortho(const int *, const int *, double *);
void dorpol(const int *, double *);
void primat(const int *, const int *, const int *, const int *, const double *);
void priarr(const int *, const int *, const int *, const int *, const int *,
            const double *);
void pritrl(const int *, const int *, const int *, const double *);
void pritru(const int *, const int *, const int *, const double *);
void mpinve(const int *, double *, double *);
void mpower(const int *, double *, double *, double *);
void mpinvt(const int *, double *, double *);
void trimat(const int *, const double *, double *);
void mattri(const int *, const double *, double *);
void mutrma(const int *, const int *, const double *, const double *, double *);

void jacobiC(const int *, double *, double *, double *, double *, const int *,
             const double *);

void smacofDistC(const double *, const int *, const int *, double *);
void smacofLossC(const double *, const double *, const double *, const int *,
                 double *);
void smacofBmatC(const double *, const double *, const double *, const int *,
                 double *);
void smacofVmatC(const double *, const int *, double *);
void smacofGuttmanC(const double *, const double *, const double *, const int *,
                    const int *, double *, double *);
void smacofGradientC(const double *, const double *, const double *,
                     const int *, const int *, double *, double *);
void smacofHmatC(const double *, const double *, const double *, const double *,
                 const double *, const double *, const int *, const int *,
                 double *, double *);
void smacofHessianC(const double *, const double *, const double *,
                    const double *, const double *, const double *, const int *,
                    const int *, double *, double *);
void smacofInitialC(const double *, const int *, const int *, double *,
                    double *, double *, double *, double *);
double runSmacof(const double *delta, int m, int p, int itmax, double eps,int* _itel, double **xnew);

static inline int VINDEX(const int i) { return i - 1; }

static inline int MINDEX(const int i, const int j, const int n) {
    return (i - 1) + (j - 1) * n;
}

static inline int AINDEX(const int i, const int j, const int k, const int n,
                         const int m) {
    return (i - 1) + (j - 1) * n + (k - 1) * n * m;
}

static inline int SINDEX(const int i, const int j, const int n) {
    return ((j - 1) * n) - (j * (j - 1) / 2) + (i - j) - 1;
}

static inline int TINDEX(const int i, const int j, const int n) {
    return ((j - 1) * n) - ((j - 1) * (j - 2) / 2) + (i - (j - 1)) - 1;
}

static inline double SQUARE(const double x) { return x * x; }
static inline double THIRD(const double x) { return x * x * x; }
static inline double FOURTH(const double x) { return x * x * x * x; }
static inline double FIFTH(const double x) { return x * x * x * x * x; }

//static inline double MAX(const double x, const double y) {
//    return (x > y) ? x : y;
//}

//static inline double MIN(const double x, const double y) {
//    return (x < y) ? x : y;
//}

static inline int IMAX(const int x, const int y) { return (x > y) ? x : y; }

static inline int IMIN(const int x, const int y) { return (x < y) ? x : y; }

static inline int IMOD(const int x, const int y) {
    return (((x % y) == 0) ? y : (x % y));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SMACOF_H */
