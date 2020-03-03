#ifndef DLOESS_LOESSC_H_
#define DLOESS_LOESSC_H_

#include "loess.h"

#define GAUSSIAN    1
#define SYMMETRIC   0

void
loess_raw(double *y, double *x, double *weights, double *robust, long int *d,
          long int *n, double *span, long int *degree, long int *nonparametric,
          long int *drop_square, long int *sum_drop_sqr, double *cell,
          char **surf_stat, double *surface, long int *parameter, long int *a,
          double *xi, double *vert, double *vval, double *diagonal, double *trL,
          double *one_delta, double *two_delta, long int *setLf);

void
loess_dfit(double *y, double *x, double *x_evaluate, double *weights,
           double *span, long int *degree, long int *nonparametric,
           long int *drop_square, long int *sum_drop_sqr, long int *d, long int *n,
           long int *m, double *fit);

void
loess_dfitse(double *y, double *x, double *x_evaluate, double *weights,
             double *robust, long int *family, double *span, long int *degree,
             long int *nonparametric, long int *drop_square,
             long int *sum_drop_sqr, long int *d, long int *n, long int *m,
             double *fit, double *L);

void
loess_ifit(long int *parameter, long int *a, double *xi, double *vert,
           double *vval, long int *m, double *x_evaluate, double *fit);

void
loess_ise(double *y, double *x, double *x_evaluate, double *weights,
          double *span, long int *degree, long int *nonparametric,
          long int *drop_square, long int *sum_drop_sqr, double *cell, long int *d,
          long int *n, long int *m, double *fit, double *L);

void
loess_workspace(long int *d, long int *n, double *span, long int *degree,
                long int *nonparametric, long int *drop_square,
                long int *sum_drop_sqr, long int *setLf);

void
loess_prune(long int *parameter, long int *a, double *xi, double *vert,
            double *vval);

void
loess_grow(long int *parameter, long int *a, double *xi, double *vert,
           double *vval);

void
loess_free();

void F77_SUB(ehg182)(int *i);

void F77_SUB(ehg183)(char *s, int *i, int *n, int *inc);

void F77_SUB(ehg184)(char *s, double *x, int *n, int *inc);

#endif // __DLOESS_LOESSC_H__
