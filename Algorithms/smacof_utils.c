#include "smacof.h"

// complete set of orthogonal polynomials

void dorpol(const int *n, double *p) {
    for (int i = 1; i <= *n; i++) {
        p[MINDEX(i, 1, *n)] = 1.0;
        double di = (double)i;
        for (int j = 2; j <= *n; j++) {
            p[MINDEX(i, j, *n)] = p[MINDEX(i, j - 1, *n)] * di;
        }
    }
    (void)dortho(n, n, p);
    return;
}

// double centering

void docent(const int *n, double *x, double *y) {
    int nn = *n;
    double *ssum = (double *)calloc((size_t)nn, sizeof(double)), t = 0.0;
    for (int i = 1; i <= nn; i++) {
        double s = 0.0;
        for (int j = 1; j <= nn; j++) {
            int ij = IMAX(i, j);
            int ji = IMIN(j, i);
            s += x[TINDEX(ij, ji, nn)];
            t += x[TINDEX(ij, ji, nn)];
        }
        ssum[VINDEX(i)] = s / ((double)nn);
    }
    t /= (double)SQUARE(nn);
    for (int j = 1; j <= nn; j++) {
        for (int i = j; i <= nn; i++) {
            y[TINDEX(i, j, nn)] =
                -(x[TINDEX(i, j, nn)] - ssum[VINDEX(i)] - ssum[VINDEX(j)] + t) /
                2.0;
        }
    }
    free(ssum);
    return;
}

// print a general matrix

void primat(const int *n, const int *m, const int *w, const int *p,
            const double *x) {
    for (int i = 1; i <= *n; i++) {
        for (int j = 1; j <= *m; j++) {
            printf(" %+*.*f ", *w, *p, x[MINDEX(i, j, *n)]);
        }
        printf("\n");
    }
    printf("\n\n");
    return;
}

// print strict lower triangle

void pritrl(const int *n, const int *w, const int *p, const double *x) {
    for (int i = 1; i <= *n; i++) {
        for (int j = 1; j <= i; j++) {
            if (i == j) {
                for (int k = 1; k <= *w + 2; k++) {
                    printf("%c", '*');
                }
            }
            if (i > j) {
                printf(" %+*.*f ", *w, *p, x[SINDEX(i, j, *n)]);
            }
        }
        printf("\n");
    }
    printf("\n\n");
    return;
}

// print inclusive lower triangle

void pritru(const int *n, const int *w, const int *p, const double *x) {
    for (int i = 1; i <= *n; i++) {
        for (int j = 1; j <= i; j++) {
            printf(" %+*.*f ", *w, *p, x[TINDEX(i, j, *n)]);
        }
        printf("\n");
    }
    printf("\n\n");
    return;
}

// print general array

void priarr(const int *n, const int *m, const int *r, const int *w,
            const int *p, const double *x) {
    for (int k = 1; k <= *r; k++) {
        for (int i = 1; i <= *n; i++) {
            for (int j = 1; j <= *m; j++) {
                printf(" %+*.*f ", *w, *p, x[AINDEX(i, j, k, *n, *m)]);
            }
            printf("\n");
        }
        printf("\n\n");
    }
    printf("\n\n\n");
    return;
}

// arbitrary power of a matrix

void mpower(const int *n, double *x, double *power, double *xpow) {
    int nn = *n, itmax = 100;
    double eps = 1e-6;
    double *e = (double *)calloc((size_t)SQUARE(nn), sizeof(double));
    double *oldi = (double *)calloc((size_t)nn, sizeof(double));
    double *oldj = (double *)calloc((size_t)nn, sizeof(double));
    (void)jacobiC(n, x, e, oldi, oldj, &itmax, &eps);
    int k = 1;
    for (int i = 1; i <= nn; i++) {
        double s = x[VINDEX(k)];
        oldi[VINDEX(i)] = (s > 1e-10) ? pow(s, *power) : 0.0;
        k += (nn - (i - 1));
    }
    for (int j = 1; j <= nn; j++) {
        for (int i = j; i <= nn; i++) {
            double s = 0.0;
            for (int k = 1; k <= nn; k++) {
                s +=
                    e[MINDEX(i, k, nn)] * e[MINDEX(j, k, nn)] * oldi[VINDEX(k)];
            }
            xpow[TINDEX(i, j, nn)] = s;
        }
    }
    free(e);
    free(oldi);
    free(oldj);
    return;
}

// inclusive lower triangle to symmetric

void trimat(const int *n, const double *x, double *y) {
    int nn = *n;
    for (int i = 1; i <= nn; i++) {
        for (int j = 1; j <= nn; j++) {
            y[MINDEX(i, j, nn)] =
                (i >= j) ? x[TINDEX(i, j, nn)] : x[TINDEX(j, i, nn)];
        }
    }
    return;
}

// symmetric to inclusive lower triangular

void mattri(const int *n, const double *x, double *y) {
    int nn = *n;
    for (int j = 1; j <= nn; j++) {
        for (int i = j; i <= nn; i++) {
            y[TINDEX(i, j, nn)] = x[MINDEX(i, j, nn)];
        }
    }
    return;
}

// premultiply matrix by symmetric matrix in inclusive triangular storage

void mutrma(const int *n, const int *m, const double *a, const double *x,
            double *y) {
    int nn = *n, mm = *m;
    for (int i = 1; i <= nn; i++) {
        for (int j = 1; j <= mm; j++) {
            double s = 0.0;
            for (int k = 1; k <= nn; k++) {
                int ik = IMAX(i, k);
                int ki = IMIN(i, k);
                s += a[TINDEX(ik, ki, nn)] * x[MINDEX(k, j, nn)];
            }
            y[MINDEX(i, j, nn)] = s;
        }
    }
}

// direct sum of two matrices -- can be used recursively

void dirsum(const int *n, const int *m, const double *a, const double *b,
            double *c) {
    int nn = *n, mm = *m, mn = nn + mm;
    for (int j = 1; j <= nn; j++) {
        for (int i = j; i <= nn; i++) {
            c[TINDEX(i, j, mn)] = a[TINDEX(i, j, nn)];
        }
    }
    for (int j = 1; j <= mm; j++) {
        for (int i = j; i <= mm; i++) {
            c[TINDEX(nn + i, nn + j, mn)] = b[TINDEX(i, j, mm)];
        }
    }
    return;
}
