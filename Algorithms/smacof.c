
#include "smacof.h"

void smacofDistC(const double *x, const int *n, const int *p, double *d) {
    int k = 1, nn = *n, pp = *p;
    for (int j = 1; j <= nn - 1; j++) {
        for (int i = j + 1; i <= nn; i++) {
            double dij = 0.0;
            for (int s = 1; s <= pp; s++) {
                dij += SQUARE(x[MINDEX(i, s, nn)] - x[MINDEX(j, s, nn)]);
            }
            d[VINDEX(k)] = sqrt(dij);
            k++;
        }
    }
}

void smacofLossC(const double *dist, const double *w, const double *delta,
                 const int *m, double *stress) {
    int mm = *m;
    *stress = 0.0;
    for (int k = 1; k <= mm; k++) {
        *stress += w[VINDEX(k)] * SQUARE(delta[VINDEX(k)] - dist[VINDEX(k)]);
    }
    *stress /= 2.0;
    return;
}

void smacofBmatC(const double *dist, const double *w, const double *delta,
                 const int *n, double *b) {
    int nn = *n;
    for (int i = 1; i <= nn; i++) {
        b[TINDEX(i, i, nn)] = 0.0;
    }
    for (int j = 1; j <= nn - 1; j++) {
        for (int i = j + 1; i <= nn; i++) {
            int k = SINDEX(i, j, nn);
            double dinv = (dist[k] == 0.0) ? 0.0 : 1.0 / dist[k];
            double elem = w[k] * delta[k] * dinv;
            b[TINDEX(i, j, nn)] = -elem;
            b[TINDEX(i, i, nn)] += elem;
            b[TINDEX(j, j, nn)] += elem;
        }
    }
    return;
}

void smacofVmatC(const double *w, const int *n, double *v) {
    int nn = *n;
    for (int i = 1; i <= nn; i++) {
        v[TINDEX(i, i, nn)] = 0.0;
    }
    for (int j = 1; j <= nn - 1; j++) {
        for (int i = j + 1; i <= nn; i++) {
            double elem = w[SINDEX(i, j, nn)];
            v[TINDEX(i, j, nn)] = -elem;
            v[TINDEX(i, i, nn)] += elem;
            v[TINDEX(j, j, nn)] += elem;
        }
    }
    return;
}

void smacofHmatC(const double *x, const double *bmat, const double *vmat,
                 const double *w, const double *delta, const double *dist,
                 const int *n, const int *p, double *work, double *h) {
    int nn = *n, pp = *p, np = nn * pp;
    for (int s = 1; s <= pp; s++) {
        for (int t = 1; t <= s; t++) {
            for (int j = 1; j <= nn; j++) {
                for (int i = j; i <= nn; i++) {
                    work[TINDEX(i, j, nn)] = 0.0;
                }
            }
            for (int j = 1; j <= nn - 1; j++) {
                for (int i = j + 1; i <= nn; i++) {
                    double f1 = (x[MINDEX(i, s, nn)] - x[MINDEX(j, s, nn)]);
                    double f2 = (x[MINDEX(i, t, nn)] - x[MINDEX(j, t, nn)]);
                    double f3 = THIRD(dist[SINDEX(i, j, nn)]);
                    double f4 = delta[SINDEX(i, j, nn)] * w[SINDEX(i, j, nn)];
                    f3 = (f3 < 1e-10) ? 0 : 1 / f3;
                    double elem = f1 * f2 * f4 * f3;
                    work[TINDEX(i, j, nn)] = -elem;
                    work[TINDEX(i, i, nn)] += elem;
                    work[TINDEX(j, j, nn)] += elem;
                }
            }
            if (s == t) {
                for (int j = 1; j <= nn; j++) {
                    for (int i = j; i <= nn; i++) {
                        h[TINDEX((s - 1) * nn + i, (s - 1) * nn + j, nn * pp)] =
                            bmat[TINDEX(i, j, nn)] - work[TINDEX(i, j, nn)];
                    }
                }
            }
            if (s != t) {
                for (int i = 1; i <= nn; i++) {
                    for (int j = 1; j <= nn; j++) {
                        int ij = IMIN(i, j);
                        int ji = IMAX(i, j);
                        int sj = (s - 1) * nn + j;
                        int si = (t - 1) * nn + i;
                        h[TINDEX(sj, si, np)] = -work[TINDEX(ji, ij, nn)];
                    }
                }
            }
        }
    }
    return;
}

void smacofGuttmanC(const double *x, const double *bmat, const double *vinv,
                    const int *n, const int *p, double *work, double *y) {
    (void)mutrma(n, p, bmat, x, work);
    (void)mutrma(n, p, vinv, work, y);
    return;
}

void smacofGradientC(const double *x, const double *bmat, const double *vmat,
                     const int *n, const int *p, double *work, double *y) {
    int nn = *n, pp = *p;
    (void)mutrma(n, p, vmat, x, y);
    (void)mutrma(n, p, bmat, x, work);
    for (int i = 1; i <= nn; i++) {
        for (int s = 1; s <= pp; s++) {
            y[MINDEX(i, s, nn)] -= work[MINDEX(i, s, nn)];
        }
    }
    return;
}

void smacofHessianC(const double *x, const double *bmat, const double *vmat,
                    const double *w, const double *delta, const double *dist,
                    const int *n, const int *p, double *work, double *h) {
    int nn = *n, pp = *p, np = nn * pp;
    (void)smacofHmatC(x, bmat, vmat, w, delta, dist, n, p, work, h);
    for (int j = 1; j <= np; j++) {
        for (int i = j; i <= np; i++) {
            h[TINDEX(i, j, np)] = -h[TINDEX(i, j, np)];
        }
    }
    for (int s = 1; s <= pp; s++) {
        for (int j = 1; j <= nn; j++) {
            for (int i = j; i <= nn; i++) {
                h[TINDEX((s - 1) * nn + i, (s - 1) * nn + j, np)] +=
                    vmat[TINDEX(i, j, nn)];
            }
        }
    }
    return;
}

void smacofNewtonC() { return; }


double runSmacof(const double *delta, int m, int p, int itmax, double eps,
                 double *_xold, int* _itel, double **_xnew)
{
    int itel = 1;

    // smacofInitialRC(delta, p)
    int n = (int)( round((1 + sqrt( 1 + 8 * m)) / 2) );
    int r = (int)( n * (n+1) / 2);

    double *w, *work1, *work2, *work3, *work4, *xold;

    int w_length = m;
    w = malloc(w_length * sizeof(double));
    for (int i=0; i<m; ++i) w[i] = 1;

    work1 = calloc(n, sizeof(double));
    work2 = calloc(r, sizeof(double));
    work3 = calloc(n*n, sizeof(double));
    work4 = calloc(n, sizeof(double));

    int x_length = n * p;
    xold = calloc(x_length, sizeof(double));
    for (int i=0; i<x_length; i++) xold[i] = _xold[i];

    smacofInitialC(delta, &n, &p, work1, work2, work3, work4, xold);
    free(work1);
    free(work2);
    free(work3);
    free(work4);

    //dold <- smacofDistRC (xold, p)
    int d_length = n * (n-1)/2;
    double *dold = calloc(d_length, sizeof(double));
    smacofDistC(xold, &n, &p, dold);

    //sold <- smacofLossRC (dold, w, delta)
    double sold;
    smacofLossC(dold, w, delta, &d_length, &sold);

    //bold <- smacofBmatRC (dold, w, delta)
    int b_length = n * (n+1)/2;
    double *bold = calloc(b_length, sizeof(double));
    smacofBmatC(dold, w, delta, &n, bold);

    //vinv <- mpowerRC (smacofVmatRC (w), -1)
    int v_length = n * (n+1) / 2;
    double *vinv = calloc(v_length, sizeof(double));
    smacofVmatC(w, &n, vinv);
    double *xpow = calloc(v_length, sizeof(double));
    int p_length = round ((sqrt (1 + 8 * v_length) - 1) / 2);
    double pv = -1;
    mpower(&p_length, vinv, &pv, xpow);

    double eiff, tmp_eiff, snew;

    double* xnew = calloc(x_length, sizeof(double));
    double* dnew = calloc(d_length, sizeof(double));
    double* bnew = calloc(b_length, sizeof(double));

    for (int i=0; i<itmax; i++) {
        int _m, _n, _r, _p;
        //xnew <- smacofGuttmanRC (xold, bold, vinv)
        _m = b_length;
        _n = round ((sqrt (1 + 8 * _m) - 1) / 2);
        _r = x_length;
        _p = round(_r/_n);
        double *work = calloc(_r, sizeof(double));
        smacofGuttmanC(xold, bold, xpow, &_n, &_p, work, xnew);
        free(work);
        
        //eiff <- max (abs (xold - xnew))
        for (int j=0; j<_r; ++j) {
            tmp_eiff = fabs(xold[j] - xnew[j]);
            if (j == 0) eiff = tmp_eiff;
            else if (eiff < tmp_eiff) eiff = tmp_eiff;
        }

        //dnew <- smacofDistRC (xnew, p)
        _n = n;
        smacofDistC(xnew, &_n, &p, dnew);

        //bnew <- smacofBmatRC (dnew, w, delta)
        _n = n;
        smacofBmatC(dnew, w, delta, &_n, bnew);

        //snew <- smacofLossRC (dnew, w, delta)
        smacofLossC(dnew, w, delta, &d_length, &snew);

        if ((eiff < eps) || (itel == itmax)) {
            break;
        }
        itel = itel + 1;
        //xold = xnew;
        //bold = bnew;
        //dold = dnew;
        sold = snew;
        for (int j=0; j<x_length; ++j) xold[j] = xnew[j];
        for (int j=0; j<b_length; ++j) bold[j] = bnew[j];
        for (int j=0; j<d_length; ++j) dold[j] = dnew[j];
    }

    free(xold);
    free(bold);
    free(dold);
    free(xpow);

    *_xnew = xnew;
    *_itel = itel;
    return snew;
}

void smacofInitialC(const double *delta, const int *n, const int *p,
                    double *work1, double *work2, double *work3, double *work4,
                    double *x) {
    int nn = *n, pp = *p, itmax = 100;
    double s, ss = 0.0, eps = 1e-6;
    for (int i = 1; i <= nn; i++) {
        work1[VINDEX(i)] = 0.0;
        for (int j = 1; j <= nn; j++) {
            if (i == j) continue;
            int ij = IMAX(i, j);
            int ji = IMIN(i, j);
            s = SQUARE(delta[SINDEX(ij, ji, nn)]);
            ss += s;
            work1[VINDEX(i)] += s;
            if (j < i) continue;
            work2[TINDEX(ij, ji, nn)] = s;
        }
        work1[VINDEX(i)] /= (double)nn;
    }
    ss /= SQUARE((double)nn);
    for (int j = 1; j <= nn; j++) {
        for (int i = j; i <= nn; i++) {
            work2[TINDEX(i, j, nn)] -= work1[VINDEX(i)];
            work2[TINDEX(i, j, nn)] -= work1[VINDEX(j)];
            work2[TINDEX(i, j, nn)] += ss;
            work2[TINDEX(i, j, nn)] *= -0.5;
        }
    }
    (void)jacobiC(n, work2, work3, work1, work4, &itmax, &eps);
    for (int i = 1; i <= nn; i++) {
        for (int j = 1; j <= pp; j++) {
            s = work2[TINDEX(j, j, nn)];
            if (s <= 0) continue;
            x[MINDEX(i, j, nn)] = work3[MINDEX(i, j, nn)] * sqrt(s);
        }
    }
    return;
}

void smacofUpdateC(const double *xold, const double *w, const double *delta,
                   const double *vinv, const int *n, const int *p, double *dnew,
                   double *bmat, double *work, double *snew, double *xnew) {
    int nn = *n, pp = *p, mm = nn * (nn - 1) / 2;
    (void)mutrma(n, p, bmat, xold, work);
    (void)mutrma(n, p, vinv, work, xnew);
    for (int j = 1; j <= nn - 1; j++) {
        for (int i = j + 1; i <= nn; i++) {
            double dij = 0.0;
            for (int s = 1; s <= pp; s++) {
                dij += SQUARE(xnew[MINDEX(i, s, nn)] - xnew[MINDEX(j, s, nn)]);
            }
            dnew[SINDEX(i, j, nn)] = sqrt(dij);
        }
    }
    for (int i = 1; i <= nn; i++) {
        bmat[TINDEX(i, i, nn)] = 0.0;
    }
    for (int j = 1; j <= nn - 1; j++) {
        for (int i = j + 1; i <= nn; i++) {
            int k = SINDEX(i, j, nn);
            double dinv = (dnew[k] == 0.0) ? 0.0 : 1.0 / dnew[k];
            double elem = w[k] * delta[k] * dinv;
            bmat[TINDEX(i, j, nn)] = -elem;
            bmat[TINDEX(i, i, nn)] += elem;
            bmat[TINDEX(j, j, nn)] += elem;
        }
    }
    *snew = 0.0;
    for (int k = 1; k <= mm; k++) {
        *snew += w[VINDEX(k)] * SQUARE(delta[VINDEX(k)] - dnew[VINDEX(k)]);
    }
    *snew /= 2.0;
}
