#ifndef DLOESS_LOESSF_H_
#define DLOESS_LOESSF_H_

#include "loess.h"

void F77_SUB(lowesa)(double *trl, long int *n, long int *d, long int *tau,
                     long int *nsing, double *delta1, double *delta2);

void F77_SUB(lowesb)(double *xx, double *yy, double *ww, double *diagl,
                     long int *infl, long int *iv, void *liv, void *lv,
                     double *wv);

void F77_SUB(lowesc)(long int *n, double *l, double *ll, double *trl,
                     double *delta1, double *delta2);

void F77_SUB(lowesd)(long int *versio, long int *iv, long int *liv, long int *lv,
                     double *v, long int *d, long int *n, double *f,
                     long int *ideg, long int *nvmax, long int *setlf);

void F77_SUB(lowese)(long int *iv, void *liv, void *lv, double *wv, long int *m,
                     double *x, double *surface);

void F77_SUB(lowesf)(double *xx, double *yy, double *ww, long int *iv, void *liv,
                     void *lv, double *wv, long int *m, double *z, double *l,
                     long int *ihat, double *s);

void F77_SUB(lowesl)(int *iv, void *liv, void *lv, double *wv, long int *m,
                     double *z, double *l);

void F77_SUB(lowesp)(long int *n, double *y, double *yhat, double *pwgts,
                     double *rwgts, long int *pi, double *ytilde);

void F77_SUB(lowesw)(double *res, long int *n, double *rw,
                     long int *pi);

void F77_SUB(ehg169)(long int *d, long int *vc, long int *nc, long int *ncmax,
                     long int *nv, long int *nvmax, double *v, long int *a,
                     double *xi, long int *c, long int *hi, long int *lo);

void F77_SUB(ehg196)(long int *tau, long int *d, double *f, double *trl);

#endif // DLOESS_LOESSF_H_
