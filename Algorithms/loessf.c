/* loessf.f -- translated by f2c (version 20190311).
 You must link the resulting object file with libf2c:
 on Microsoft Windows system, link with libf2c.lib;
 on Linux or Unix systems, link with .../path/to/libf2c.a -lm
 or, if you install libf2c.a in a standard place, with -lf2c -lm
 -- in that order, at the end of the command line, as in
 cc *.o -lf2c -lm
 Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

 http://www.netlib.org/f2c/libf2c.zip
 */

#include "../Regression/f2c.h"
#include "../Regression/blaswrap.h"

/* Table of constant values */

static integer c__2 = 2;
static integer c__180 = 180;
static integer c__4 = 4;
static integer c__1 = 1;
static integer c__120 = 120;
static integer c__121 = 121;
static integer c__0 = 0;
static integer c__1000 = 1000;
static integer c__15 = 15;
static integer c__21 = 21;
static integer c__182 = 182;
static integer c__101 = 101;
static integer c__193 = 193;
static integer c__181 = 181;
static integer c__122 = 122;
static integer c__104 = 104;
static integer c__105 = 105;
static integer c__123 = 123;
static integer c__10000 = 10000;
static integer c__194 = 194;
static integer c__196 = 196;
static integer c__174 = 174;
static integer c__171 = 171;
static integer c__100 = 100;
static integer c__195 = 195;
static integer c__102 = 102;
static integer c__103 = 103;
static integer c__172 = 172;
static integer c__173 = 173;
static integer c__186 = 186;
static integer c__175 = 175;
static integer c__187 = 187;
static integer c__185 = 185;


/*  The authors of this software are Cleveland, Grosse, and Shyu. */
/*  Copyright (c) 1989, 1992 by AT&T. */
/*  Permission to use, copy, modify, and distribute this software for any */
/*  purpose without fee is hereby granted, provided that this entire notice */
/*  is included in all copies of any software which is or includes a copy */
/*  or modification of this software and in all copies of the supporting */
/*  documentation for such software. */
/*  THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED */
/*  WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY */
/*  REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY */
/*  OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE. */
/*       altered by B.D. Ripley to */

/*       remove unused variables */
/*       make phi in ehg139 double precision to match calling sequence */
/*       pass integer not logical from C */

/*       Note that  ehg182(errormsg_code)  is in ./loessc.c */
/* Subroutine */ int ehg126_(integer *d__, integer *n, integer *vc,
                             doublereal *x, doublereal *v, integer *nvmax)
{
    /* Initialized data */

    static integer execnt = 0;

    /* System generated locals */
    integer v_dim1, v_offset, x_dim1, x_offset, i__1, i__2;
    doublereal d__1, d__2, d__3, d__4;

    /* Local variables */
    static integer i__, j, k;
    static doublereal t, mu, beta, alpha;
    extern doublereal d1mach_(integer *);
    static doublereal machin;

    /* Parameter adjustments */
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    /*     MachInf -> machin */
    ++execnt;
    if (execnt == 1) {
        /*     initialize  d1mach(2) === DBL_MAX: */
        machin = d1mach_(&c__2);
    }
    /*     fill in vertices for bounding box of $x$ */
    /*     lower left, upper right */
    i__1 = *d__;
    for (k = 1; k <= i__1; ++k) {
        alpha = machin;
        beta = -machin;
        i__2 = *n;
        for (i__ = 1; i__ <= i__2; ++i__) {
            t = x[i__ + k * x_dim1];
            alpha = min(alpha,t);
            beta = max(beta,t);
            /* L4: */
        }
        /*        expand the box a little */
        /* Computing MAX */
        /* Computing MAX */
        d__3 = abs(alpha), d__4 = abs(beta);
        d__1 = beta - alpha, d__2 = max(d__3,d__4) * 1e-10 + 1e-30;
        mu = max(d__1,d__2) * .005;
        alpha -= mu;
        beta += mu;
        v[k * v_dim1 + 1] = alpha;
        v[*vc + k * v_dim1] = beta;
        /* L3: */
    }
    /*     remaining vertices */
    i__1 = *vc - 1;
    for (i__ = 2; i__ <= i__1; ++i__) {
        j = i__ - 1;
        i__2 = *d__;
        for (k = 1; k <= i__2; ++k) {
            v[i__ + k * v_dim1] = v[j % 2 * (*vc - 1) + 1 + k * v_dim1];
            j = (integer) ((doublereal) j / 2.);
            /* L6: */
        }
        /* L5: */
    }
    return 0;
} /* ehg126_ */

/* Subroutine */ int ehg125_(integer *p, integer *nv, doublereal *v, integer *
                             vhit, integer *nvmax, integer *d__, integer *k, doublereal *t,
                             integer *r__, integer *s, integer *f, integer *l, integer *u)
{
    /* System generated locals */
    integer f_dim1, f_offset, l_dim1, l_offset, u_dim1, u_offset, v_dim1,
    v_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer h__, i__, j, m;
    static logical i1, i2;
    static integer i3, mm;
    extern /* Subroutine */ int ehg182_(integer *);
    static logical match;

    /* Parameter adjustments */
    --vhit;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;
    u_dim1 = *r__;
    u_offset = 1 + (u_dim1 << 1);
    u -= u_offset;
    l_dim1 = *r__;
    l_offset = 1 + (l_dim1 << 1);
    l -= l_offset;
    f_dim1 = *r__;
    f_offset = 1 + (f_dim1 << 1);
    f -= f_offset;

    /* Function Body */
    h__ = *nv;
    i__1 = *r__;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = *s;
        for (j = 1; j <= i__2; ++j) {
            ++h__;
            i__3 = *d__;
            for (i3 = 1; i3 <= i__3; ++i3) {
                v[h__ + i3 * v_dim1] = v[f[i__ + (j << 1) * f_dim1] + i3 *
                                         v_dim1];
                /* L5: */
            }
            v[h__ + *k * v_dim1] = *t;
            /*           check for redundant vertex */
            match = FALSE_;
            m = 1;
            /*           top of while loop */
        L6:
            if (! match) {
                i1 = m <= *nv;
            } else {
                i1 = FALSE_;
            }
            if (! i1) {
                goto L7;
            }
            match = v[m + v_dim1] == v[h__ + v_dim1];
            mm = 2;
            /*              top of while loop */
        L8:
            if (match) {
                i2 = mm <= *d__;
            } else {
                i2 = FALSE_;
            }
            if (! i2) {
                goto L9;
            }
            match = v[m + mm * v_dim1] == v[h__ + mm * v_dim1];
            ++mm;
            goto L8;
            /*              bottom of while loop */
        L9:
            ++m;
            goto L6;
            /*           bottom of while loop */
        L7:
            --m;
            if (match) {
                --h__;
            } else {
                m = h__;
                if (vhit[1] >= 0) {
                    vhit[m] = *p;
                }
            }
            l[i__ + (j << 1) * l_dim1] = f[i__ + (j << 1) * f_dim1];
            l[i__ + ((j << 1) + 1) * l_dim1] = m;
            u[i__ + (j << 1) * u_dim1] = m;
            u[i__ + ((j << 1) + 1) * u_dim1] = f[i__ + ((j << 1) + 1) *
                                                 f_dim1];
            /* L4: */
        }
        /* L3: */
    }
    *nv = h__;
    if (! (*nv <= *nvmax)) {
        ehg182_(&c__180);
    }
    return 0;
} /* ehg125_ */

integer ehg138_(integer *i__, doublereal *z__, integer *a, doublereal *xi,
                integer *lo, integer *hi, integer *ncmax)
{
    /* System generated locals */
    integer ret_val;

    /* Local variables */
    static integer j;
    static logical i1;

    /*     descend tree until leaf or ambiguous */
    /* Parameter adjustments */
    --z__;
    --hi;
    --lo;
    --xi;
    --a;

    /* Function Body */
    j = *i__;
    /*     top of while loop */
L3:
    if (a[j] != 0) {
        i1 = z__[a[j]] != xi[j];
    } else {
        i1 = FALSE_;
    }
    if (! i1) {
        goto L4;
    }
    if (z__[a[j]] <= xi[j]) {
        j = lo[j];
    } else {
        j = hi[j];
    }
    goto L3;
    /*     bottom of while loop */
L4:
    ret_val = j;
    return ret_val;
} /* ehg138_ */

/* Subroutine */ int ehg106_(integer *il, integer *ir, integer *k, integer *
                             nk, doublereal *p, integer *pi, integer *n)
{
    /* System generated locals */
    integer p_dim1, p_offset;

    /* Local variables */
    static integer i__, j, l, r__;
    static doublereal t;
    static integer ii;

    /* Partial sorting of p(1, il:ir) returning the sort indices pi() only */
    /* such that p(1, pi(k)) is correct */
    /*     implicit none */
    /* Arguments */
    /*  Input: */
    /*     using only       p(1, pi(*)) */
    /*  Output: */
    /* Variables */
    /*     find the $k$-th smallest of $n$ elements */
    /*     Floyd+Rivest, CACM Mar '75, Algorithm 489 */
    /* Parameter adjustments */
    --pi;
    p_dim1 = *nk;
    p_offset = 1 + p_dim1;
    p -= p_offset;

    /* Function Body */
    l = *il;
    r__ = *ir;
    /*     while (l < r ) */
L3:
    if (! (l < r__)) {
        goto L4;
    }
    /*        to avoid recursion, sophisticated partition deleted */
    /*        partition $x sub {l..r}$ about $t$ */
    t = p[pi[*k] * p_dim1 + 1];
    i__ = l;
    j = r__;
    ii = pi[l];
    pi[l] = pi[*k];
    pi[*k] = ii;
    if (t < p[pi[r__] * p_dim1 + 1]) {
        ii = pi[l];
        pi[l] = pi[r__];
        pi[r__] = ii;
    }
    /*        top of while loop */
L5:
    if (! (i__ < j)) {
        goto L6;
    }
    ii = pi[i__];
    pi[i__] = pi[j];
    pi[j] = ii;
    ++i__;
    --j;
    /*           top of while loop */
L7:
    if (! (p[pi[i__] * p_dim1 + 1] < t)) {
        goto L8;
    }
    ++i__;
    goto L7;
    /*           bottom of while loop */
L8:
    /*           top of while loop */
L9:
    if (! (t < p[pi[j] * p_dim1 + 1])) {
        goto L10;
    }
    --j;
    goto L9;
    /*           bottom of while loop */
L10:
    goto L5;
    /*        bottom of while loop */
L6:
    if (p[pi[l] * p_dim1 + 1] == t) {
        ii = pi[l];
        pi[l] = pi[j];
        pi[j] = ii;
    } else {
        ++j;
        ii = pi[r__];
        pi[r__] = pi[j];
        pi[j] = ii;
    }
    if (j <= *k) {
        l = j + 1;
    }
    if (*k <= j) {
        r__ = j - 1;
    }
    goto L3;
    /*     bottom of while loop */
L4:
    return 0;
} /* ehg106_ */

/* Subroutine */ int ehg127_(doublereal *q, integer *n, integer *d__, integer
                             *nf, doublereal *f, doublereal *x, integer *psi, doublereal *y,
                             doublereal *rw, integer *kernel, integer *k, doublereal *dist,
                             doublereal *eta, doublereal *b, integer *od, doublereal *w,
                             doublereal *rcond, integer *sing, doublereal *sigma, doublereal *u,
                             doublereal *e, doublereal *dgamma, doublereal *qraux, doublereal *
                             work, doublereal *tol, integer *dd, integer *tdeg, integer *cdeg,
                             doublereal *s)
{
    /* Initialized data */

    static integer execnt = 0;

    /* System generated locals */
    integer b_dim1, b_offset, x_dim1, x_offset, i__1, i__2, i__3;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal g[15];
    static integer i__, j;
    static doublereal i1, i2;
    static integer i3;
    static doublereal i4, i5, i6, i7, i8;
    static integer i9;
    static doublereal i10;
    static integer jj;
    static doublereal rho, scal;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *,
                            integer *);
    static integer info, jpvt;
    extern /* Subroutine */ int ehg106_(integer *, integer *, integer *,
                                        integer *, doublereal *, integer *, integer *), ehg182_(integer *)
    , ehg184_(char *, doublereal *, integer *, integer *, ftnlen),
    dqrdc_(doublereal *, integer *, integer *, integer *, doublereal *
           , integer *, doublereal *, integer *), dsvdc_(doublereal *,
                                                         integer *, integer *, integer *, doublereal *, doublereal *,
                                                         doublereal *, integer *, doublereal *, integer *, doublereal *,
                                                         integer *, integer *), dqrsl_(doublereal *, integer *, integer *,
                                                                                       integer *, doublereal *, doublereal *, doublereal *, doublereal *,
                                                                                       doublereal *, doublereal *, doublereal *, integer *, integer *);
    extern doublereal d1mach_(integer *);
    static integer inorm2;
    static doublereal machep;
    extern integer idamax_(integer *, doublereal *, integer *);
    static doublereal colnor[15];
    static integer column;

    /* Parameter adjustments */
    --dist;
    --rw;
    --y;
    --psi;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --q;
    --w;
    --eta;
    b_dim1 = *nf;
    b_offset = 1 + b_dim1;
    b -= b_offset;
    --sigma;
    u -= 16;
    e -= 16;
    --dgamma;
    --qraux;
    --work;
    --cdeg;

    /* Function Body */
    /*     colnorm -> colnor */
    /*     E -> g */
    /*     MachEps -> machep */
    /*     V -> e */
    /*     X -> b */
    ++execnt;
    if (execnt == 1) {
        /*     initialize  d1mach(4) === 1 / DBL_EPSILON === 2^52  : */
        machep = d1mach_(&c__4);
    }
    /*     sort by distance */
    i__1 = *n;
    for (i3 = 1; i3 <= i__1; ++i3) {
        dist[i3] = 0.;
        /* L3: */
    }
    i__1 = *dd;
    for (j = 1; j <= i__1; ++j) {
        i4 = q[j];
        i__2 = *n;
        for (i3 = 1; i3 <= i__2; ++i3) {
            /* Computing 2nd power */
            d__1 = x[i3 + j * x_dim1] - i4;
            dist[i3] += d__1 * d__1;
            /* L5: */
        }
        /* L4: */
    }
    ehg106_(&c__1, n, nf, &c__1, &dist[1], &psi[1], n);
    rho = dist[psi[*nf]] * max(1.,*f);
    if (rho <= 0.) {
        ehg182_(&c__120);
    }
    /*     compute neighborhood weights */
    if (*kernel == 2) {
        i__1 = *nf;
        for (i__ = 1; i__ <= i__1; ++i__) {
            if (dist[psi[i__]] < rho) {
                i1 = sqrt(rw[psi[i__]]);
            } else {
                i1 = 0.;
            }
            w[i__] = i1;
            /* L6: */
        }
    } else {
        i__1 = *nf;
        for (i3 = 1; i3 <= i__1; ++i3) {
            w[i3] = sqrt(dist[psi[i3]] / rho);
            /* L7: */
        }
        i__1 = *nf;
        for (i3 = 1; i3 <= i__1; ++i3) {
            /* Computing 3rd power */
            d__2 = w[i3];
            /* Computing 3rd power */
            d__1 = 1 - d__2 * (d__2 * d__2);
            w[i3] = sqrt(rw[psi[i3]] * (d__1 * (d__1 * d__1)));
            /* L8: */
        }
    }
    if ((d__1 = w[idamax_(nf, &w[1], &c__1)], abs(d__1)) == 0.) {
        ehg184_("at ", &q[1], dd, &c__1, (ftnlen)3);
        ehg184_("radius ", &rho, &c__1, &c__1, (ftnlen)7);
        if (TRUE_) {
            ehg182_(&c__121);
        }
    }
    /*     fill design matrix */
    column = 1;
    i__1 = *nf;
    for (i3 = 1; i3 <= i__1; ++i3) {
        b[i3 + column * b_dim1] = w[i3];
        /* L9: */
    }
    if (*tdeg >= 1) {
        i__1 = *d__;
        for (j = 1; j <= i__1; ++j) {
            if (cdeg[j] >= 1) {
                ++column;
                i5 = q[j];
                i__2 = *nf;
                for (i3 = 1; i3 <= i__2; ++i3) {
                    b[i3 + column * b_dim1] = w[i3] * (x[psi[i3] + j * x_dim1]
                                                       - i5);
                    /* L11: */
                }
            }
            /* L10: */
        }
    }
    if (*tdeg >= 2) {
        i__1 = *d__;
        for (j = 1; j <= i__1; ++j) {
            if (cdeg[j] >= 1) {
                if (cdeg[j] >= 2) {
                    ++column;
                    i6 = q[j];
                    i__2 = *nf;
                    for (i3 = 1; i3 <= i__2; ++i3) {
                        /* Computing 2nd power */
                        d__1 = x[psi[i3] + j * x_dim1] - i6;
                        b[i3 + column * b_dim1] = w[i3] * (d__1 * d__1);
                        /* L13: */
                    }
                }
                i__2 = *d__;
                for (jj = j + 1; jj <= i__2; ++jj) {
                    if (cdeg[jj] >= 1) {
                        ++column;
                        i7 = q[j];
                        i8 = q[jj];
                        i__3 = *nf;
                        for (i3 = 1; i3 <= i__3; ++i3) {
                            b[i3 + column * b_dim1] = w[i3] * (x[psi[i3] + j *
                                                                 x_dim1] - i7) * (x[psi[i3] + jj * x_dim1]
                                                                                  - i8);
                            /* L15: */
                        }
                    }
                    /* L14: */
                }
            }
            /* L12: */
        }
        *k = column;
    }
    i__1 = *nf;
    for (i3 = 1; i3 <= i__1; ++i3) {
        eta[i3] = w[i3] * y[psi[i3]];
        /* L16: */
    }
    /*     equilibrate columns */
    i__1 = *k;
    for (j = 1; j <= i__1; ++j) {
        scal = 0.;
        i__2 = *nf;
        for (inorm2 = 1; inorm2 <= i__2; ++inorm2) {
            /* Computing 2nd power */
            d__1 = b[inorm2 + j * b_dim1];
            scal += d__1 * d__1;
            /* L18: */
        }
        scal = sqrt(scal);
        if (0. < scal) {
            i__2 = *nf;
            for (i3 = 1; i3 <= i__2; ++i3) {
                b[i3 + j * b_dim1] /= scal;
                /* L19: */
            }
            colnor[j - 1] = scal;
        } else {
            colnor[j - 1] = 1.;
        }
        /* L17: */
    }
    /*     singular value decomposition */
    dqrdc_(&b[b_offset], nf, nf, k, &qraux[1], &jpvt, &work[1], &c__0);
    dqrsl_(&b[b_offset], nf, nf, k, &qraux[1], &eta[1], &work[1], &eta[1], &
           eta[1], &work[1], &work[1], &c__1000, &info);
    i__1 = *k;
    for (i9 = 1; i9 <= i__1; ++i9) {
        i__2 = *k;
        for (i3 = 1; i3 <= i__2; ++i3) {
            u[i3 + i9 * 15] = 0.;
            /* L21: */
        }
        /* L20: */
    }
    i__1 = *k;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = *k;
        for (j = i__; j <= i__2; ++j) {
            /* FIXME: this has i = 3 vs bound 2 in a ggplot2 test */
            u[i__ + j * 15] = b[i__ + j * b_dim1];
            /* L23: */
        }
        /* L22: */
    }
    dsvdc_(&u[16], &c__15, k, k, &sigma[1], g, &u[16], &c__15, &e[16], &c__15,
           &work[1], &c__21, &info);
    if (! (info == 0)) {
        ehg182_(&c__182);
    }
    *tol = sigma[1] * (machep * 100);
    /* Computing MIN */
    d__1 = *rcond, d__2 = sigma[*k] / sigma[1];
    *rcond = min(d__1,d__2);
    if (sigma[*k] <= *tol) {
        ++(*sing);
        if (*sing == 1) {
            ehg184_("pseudoinverse used at", &q[1], d__, &c__1, (ftnlen)21);
            d__1 = sqrt(rho);
            ehg184_("neighborhood radius", &d__1, &c__1, &c__1, (ftnlen)19);
            ehg184_("reciprocal condition number ", rcond, &c__1, &c__1, (
                                                                          ftnlen)28);
        } else {
            if (*sing == 2) {
                ehg184_("There are other near singularities as well.", &rho, &
                        c__1, &c__1, (ftnlen)43);
            }
        }
    }
    /*     compensate for equilibration */
    i__1 = *k;
    for (j = 1; j <= i__1; ++j) {
        i10 = colnor[j - 1];
        i__2 = *k;
        for (i3 = 1; i3 <= i__2; ++i3) {
            e[j + i3 * 15] /= i10;
            /* L25: */
        }
        /* L24: */
    }
    /*     solve least squares problem */
    i__1 = *k;
    for (j = 1; j <= i__1; ++j) {
        if (*tol < sigma[j]) {
            i2 = ddot_(k, &u[j * 15 + 1], &c__1, &eta[1], &c__1) / sigma[j];
        } else {
            i2 = 0.;
        }
        dgamma[j] = i2;
        /* L26: */
    }
    i__1 = *od;
    for (j = 0; j <= i__1; ++j) {
        /*        bug fix 2006-07-04 for k=1, od>1.   (thanks btyner@gmail.com) */
        if (j < *k) {
            s[j] = ddot_(k, &e[j + 16], &c__15, &dgamma[1], &c__1);
        } else {
            s[j] = 0.;
        }
        /* L27: */
    }
    return 0;
} /* ehg127_ */

/* Subroutine */ int ehg131_(doublereal *x, doublereal *y, doublereal *rw,
                             doublereal *trl, doublereal *diagl, integer *kernel, integer *k,
                             integer *n, integer *d__, integer *nc, integer *ncmax, integer *vc,
                             integer *nv, integer *nvmax, integer *nf, doublereal *f, integer *a,
                             integer *c__, integer *hi, integer *lo, integer *pi, integer *psi,
                             doublereal *v, integer *vhit, doublereal *vval, doublereal *xi,
                             doublereal *dist, doublereal *eta, doublereal *b, integer *ntol,
                             doublereal *fd, doublereal *w, doublereal *vval2, doublereal *rcond,
                             integer *sing, integer *dd, integer *tdeg, integer *cdeg, integer *lq,
                             doublereal *lf, logical *setlf)
{
    /* System generated locals */
    integer lq_dim1, lq_offset, c_dim1, c_offset, lf_dim1, lf_dim2, lf_offset,
    v_dim1, v_offset, vval_dim1, vval_offset, vval2_dim1,
    vval2_offset, x_dim1, x_offset, i__1, i__2;

    /* Local variables */
    static integer j, i1, i2;
    extern /* Subroutine */ int ehg124_(integer *, integer *, integer *,
                                        integer *, integer *, integer *, integer *, integer *, doublereal
                                        *, integer *, integer *, doublereal *, integer *, integer *,
                                        integer *, doublereal *, integer *, integer *, integer *,
                                        doublereal *, integer *), ehg126_(integer *, integer *, integer *,
                                                                          doublereal *, doublereal *, integer *), ehg182_(integer *),
    ehg139_(doublereal *, integer *, integer *, integer *, integer *,
            integer *, doublereal *, doublereal *, integer *, integer *,
            doublereal *, doublereal *, doublereal *, integer *, integer *,
            doublereal *, doublereal *, doublereal *, doublereal *, integer *,
            doublereal *, doublereal *, doublereal *, integer *, integer *,
            integer *, doublereal *, integer *, integer *, integer *, integer
            *, doublereal *, integer *, integer *, integer *, integer *,
            integer *, doublereal *, logical *, doublereal *);
    extern doublereal dnrm2_(integer *, doublereal *, integer *);
    static doublereal delta[8];
    static integer identi;

    /*     Identity -> identi */
    /*     X -> b */
    /* Parameter adjustments */
    --dist;
    --psi;
    --pi;
    --diagl;
    --rw;
    --y;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --xi;
    --lo;
    --hi;
    --a;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    vval2_dim1 = *d__ - 0 + 1;
    vval2_offset = 0 + vval2_dim1;
    vval2 -= vval2_offset;
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;
    --vhit;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;
    lf_dim1 = *d__ - 0 + 1;
    lf_dim2 = *nvmax;
    lf_offset = 0 + lf_dim1 * (1 + lf_dim2);
    lf -= lf_offset;
    lq_dim1 = *nvmax;
    lq_offset = 1 + lq_dim1;
    lq -= lq_offset;
    --w;
    --eta;
    --b;
    --cdeg;

    /* Function Body */
    if (! (*d__ <= 8)) {
        ehg182_(&c__101);
    }
    /*     build $k$-d tree */
    ehg126_(d__, n, vc, &x[x_offset], &v[v_offset], nvmax);
    *nv = *vc;
    *nc = 1;
    i__1 = *vc;
    for (j = 1; j <= i__1; ++j) {
        c__[j + *nc * c_dim1] = j;
        vhit[j] = 0;
        /* L3: */
    }
    i__1 = *d__;
    for (i1 = 1; i1 <= i__1; ++i1) {
        delta[i1 - 1] = v[*vc + i1 * v_dim1] - v[i1 * v_dim1 + 1];
        /* L4: */
    }
    *fd *= dnrm2_(d__, delta, &c__1);
    i__1 = *n;
    for (identi = 1; identi <= i__1; ++identi) {
        pi[identi] = identi;
        /* L5: */
    }
    ehg124_(&c__1, n, d__, n, nv, nc, ncmax, vc, &x[x_offset], &pi[1], &a[1],
            &xi[1], &lo[1], &hi[1], &c__[c_offset], &v[v_offset], &vhit[1],
            nvmax, ntol, fd, dd);
    /*     smooth */
    if (*trl != 0.) {
        i__1 = *nv;
        for (i2 = 1; i2 <= i__1; ++i2) {
            i__2 = *d__;
            for (i1 = 0; i1 <= i__2; ++i1) {
                vval2[i1 + i2 * vval2_dim1] = 0.;
                /* L7: */
            }
            /* L6: */
        }
    }
    ehg139_(&v[v_offset], nvmax, nv, n, d__, nf, f, &x[x_offset], &pi[1], &
            psi[1], &y[1], &rw[1], trl, kernel, k, &dist[1], &dist[1], &eta[1]
            , &b[1], d__, &w[1], &diagl[1], &vval2[vval2_offset], nc, vc, &a[
                                                                             1], &xi[1], &lo[1], &hi[1], &c__[c_offset], &vhit[1], rcond, sing,
            dd, tdeg, &cdeg[1], &lq[lq_offset], &lf[lf_offset], setlf, &vval[
                                                                             vval_offset]);
    return 0;
} /* ehg131_ */

/* Subroutine */ int ehg133_(integer *n, integer *d__, integer *vc, integer *
                             nvmax, integer *nc, integer *ncmax, integer *a, integer *c__, integer
                             *hi, integer *lo, doublereal *v, doublereal *vval, doublereal *xi,
                             integer *m, doublereal *z__, doublereal *s)
{
    /* System generated locals */
    integer c_dim1, c_offset, v_dim1, v_offset, vval_dim1, vval_offset,
    z_dim1, z_offset, i__1, i__2;

    /* Local variables */
    static integer i__, i1;
    extern doublereal ehg128_(doublereal *, integer *, integer *, integer *,
                              integer *, doublereal *, integer *, integer *, integer *,
                              doublereal *, integer *, doublereal *);
    static doublereal delta[8];

    /* Var */
    /* Parameter adjustments */
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;
    --xi;
    --lo;
    --hi;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    --a;
    --s;
    z_dim1 = *m;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;

    /* Function Body */
    i__1 = *m;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = *d__;
        for (i1 = 1; i1 <= i__2; ++i1) {
            delta[i1 - 1] = z__[i__ + i1 * z_dim1];
            /* L4: */
        }
        s[i__] = ehg128_(delta, d__, ncmax, vc, &a[1], &xi[1], &lo[1], &hi[1],
                         &c__[c_offset], &v[v_offset], nvmax, &vval[vval_offset]);
        /* L3: */
    }
    return 0;
} /* ehg133_ */

/* Subroutine */ int ehg140_(integer *iw, integer *i__, integer *j)
{
    /* Parameter adjustments */
    --iw;

    /* Function Body */
    iw[*i__] = *j;
    return 0;
} /* ehg140_ */

/* Subroutine */ int ehg141_(doublereal *trl, integer *n, integer *deg,
                             integer *k, integer *d__, integer *nsing, integer *dk, doublereal *
                             delta1, doublereal *delta2)
{
    /* Initialized data */

    static doublereal c__[48] = { .297162,.380266,.5886043,.4263766,.3346498,
        .6271053,.5241198,.3484836,.6687687,.6338795,.4076457,.7207693,
        .1611761,.3091323,.4401023,.2939609,.3580278,.5555741,.397239,
        .4171278,.6293196,.4675173,.469907,.6674802,.2848308,.2254512,
        .2914126,.5393624,.251723,.389897,.7603231,.2969113,.474013,
        .9664956,.3629838,.5348889,.207567,.2822574,.2369957,.3911566,
        .2981154,.3623232,.5508869,.3501989,.4371032,.7002667,.4291632,
        .493037 };

    /* System generated locals */
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal), exp(doublereal), pow_dd(doublereal *, doublereal
                                                     *);

    /* Local variables */
    static integer i__;
    static doublereal z__, c1, c2, c3, c4, zz[1], corx;
    extern /* Subroutine */ int ehg184_(char *, doublereal *, integer *,
                                        integer *, ftnlen);
    extern doublereal ehg176_(doublereal *);

    /*     coef, d, deg, del */
    if (*deg == 0) {
        *dk = 1;
    }
    if (*deg == 1) {
        *dk = *d__ + 1;
    }
    if (*deg == 2) {
        *dk = (integer) ((doublereal) ((*d__ + 2) * (*d__ + 1)) / 2.);
    }
    corx = sqrt(*k / (doublereal) (*n));
    z__ = (sqrt(*k / *trl) - corx) / (1 - corx);
    if (*nsing == 0 && 1. < z__) {
        ehg184_("Chernobyl! trL<k", trl, &c__1, &c__1, (ftnlen)16);
    }
    if (z__ < 0.) {
        ehg184_("Chernobyl! trL>n", trl, &c__1, &c__1, (ftnlen)16);
    }
    /* Computing MIN */
    d__1 = 1., d__2 = max(0.,z__);
    z__ = min(d__1,d__2);
    /* R fix */
    zz[0] = z__;
    c4 = exp(ehg176_(zz));
    i__ = (min(*d__,4) - 1 + (*deg - 1 << 2)) * 3 + 1;
    if (*d__ <= 4) {
        c1 = c__[i__ - 1];
        c2 = c__[i__];
        c3 = c__[i__ + 1];
    } else {
        c1 = c__[i__ - 1] + (*d__ - 4) * (c__[i__ - 1] - c__[i__ - 4]);
        c2 = c__[i__] + (*d__ - 4) * (c__[i__] - c__[i__ - 3]);
        c3 = c__[i__ + 1] + (*d__ - 4) * (c__[i__ + 1] - c__[i__ - 2]);
    }
    d__1 = 1 - z__;
    *delta1 = *n - *trl * exp(c1 * pow_dd(&z__, &c2) * pow_dd(&d__1, &c3) *
                              c4);
    i__ += 24;
    if (*d__ <= 4) {
        c1 = c__[i__ - 1];
        c2 = c__[i__];
        c3 = c__[i__ + 1];
    } else {
        c1 = c__[i__ - 1] + (*d__ - 4) * (c__[i__ - 1] - c__[i__ - 4]);
        c2 = c__[i__] + (*d__ - 4) * (c__[i__] - c__[i__ - 3]);
        c3 = c__[i__ + 1] + (*d__ - 4) * (c__[i__ + 1] - c__[i__ - 2]);
    }
    d__1 = 1 - z__;
    *delta2 = *n - *trl * exp(c1 * pow_dd(&z__, &c2) * pow_dd(&d__1, &c3) *
                              c4);
    return 0;
} /* ehg141_ */

/* Subroutine */ int lowesc_(integer *n, doublereal *l, doublereal *ll,
                             doublereal *trl, doublereal *delta1, doublereal *delta2)
{
    /* System generated locals */
    integer l_dim1, l_offset, ll_dim1, ll_offset, i__1, i__2;

    /* Local variables */
    static integer i__, j;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *,
                            integer *);

    /*     compute $LL~=~(I-L)(I-L)'$ */
    /* Parameter adjustments */
    ll_dim1 = *n;
    ll_offset = 1 + ll_dim1;
    ll -= ll_offset;
    l_dim1 = *n;
    l_offset = 1 + l_dim1;
    l -= l_offset;

    /* Function Body */
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        --l[i__ + i__ * l_dim1];
        /* L3: */
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = i__;
        for (j = 1; j <= i__2; ++j) {
            ll[i__ + j * ll_dim1] = ddot_(n, &l[i__ + l_dim1], n, &l[j +
                                                                     l_dim1], n);
            /* L5: */
        }
        /* L4: */
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = *n;
        for (j = i__ + 1; j <= i__2; ++j) {
            ll[i__ + j * ll_dim1] = ll[j + i__ * ll_dim1];
            /* L7: */
        }
        /* L6: */
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        ++l[i__ + i__ * l_dim1];
        /* L8: */
    }
    /*     accumulate first two traces */
    *trl = 0.;
    *delta1 = 0.;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        *trl += l[i__ + i__ * l_dim1];
        *delta1 += ll[i__ + i__ * ll_dim1];
        /* L9: */
    }
    /*     $delta sub 2 = "tr" LL sup 2$ */
    *delta2 = 0.;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        *delta2 += ddot_(n, &ll[i__ + ll_dim1], n, &ll[i__ * ll_dim1 + 1], &
                         c__1);
        /* L10: */
    }
    return 0;
} /* lowesc_ */

/* Subroutine */ int ehg169_(integer *d__, integer *vc, integer *nc, integer *
                             ncmax, integer *nv, integer *nvmax, doublereal *v, integer *a,
                             doublereal *xi, integer *c__, integer *hi, integer *lo)
{
    /* System generated locals */
    integer c_dim1, c_offset, v_dim1, v_offset, i__1, i__2, i__3, i__4;
    doublereal d__1;

    /* Builtin functions */
    integer pow_ii(integer *, integer *);

    /* Local variables */
    static integer i__, j, k, p, mc, mv;
    extern /* Subroutine */ int ehg125_(integer *, integer *, doublereal *,
                                        integer *, integer *, integer *, integer *, doublereal *, integer
                                        *, integer *, integer *, integer *, integer *), ehg182_(integer *)
    ;
    extern integer ifloor_(doublereal *);
    static integer novhit[1];

    /*     as in bbox */
    /*     remaining vertices */
    /* Parameter adjustments */
    --lo;
    --hi;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    --xi;
    --a;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    i__1 = *vc - 1;
    for (i__ = 2; i__ <= i__1; ++i__) {
        j = i__ - 1;
        i__2 = *d__;
        for (k = 1; k <= i__2; ++k) {
            v[i__ + k * v_dim1] = v[j % 2 * (*vc - 1) + 1 + k * v_dim1];
            d__1 = (doublereal) j / 2.;
            j = ifloor_(&d__1);
            /* L4: */
        }
        /* L3: */
    }
    /*     as in ehg131 */
    mc = 1;
    mv = *vc;
    novhit[0] = -1;
    i__1 = *vc;
    for (j = 1; j <= i__1; ++j) {
        c__[j + mc * c_dim1] = j;
        /* L5: */
    }
    /*     as in rbuild */
    p = 1;
    /*     top of while loop */
L6:
    if (! (p <= *nc)) {
        goto L7;
    }
    if (a[p] != 0) {
        k = a[p];
        /*           left son */
        ++mc;
        lo[p] = mc;
        /*           right son */
        ++mc;
        hi[p] = mc;
        i__2 = k - 1;
        i__1 = pow_ii(&c__2, &i__2);
        i__4 = *d__ - k;
        i__3 = pow_ii(&c__2, &i__4);
        ehg125_(&p, &mv, &v[v_offset], novhit, nvmax, d__, &k, &xi[p], &i__1,
                &i__3, &c__[p * c_dim1 + 1], &c__[lo[p] * c_dim1 + 1], &c__[
                                                                            hi[p] * c_dim1 + 1]);
    }
    ++p;
    goto L6;
    /*     bottom of while loop */
L7:
    if (! (mc == *nc)) {
        ehg182_(&c__193);
    }
    if (! (mv == *nv)) {
        ehg182_(&c__193);
    }
    return 0;
} /* ehg169_ */

doublereal ehg176_(doublereal *z__)
{
    /* Initialized data */

    static integer d__ = 1;
    static integer vc = 2;
    static integer nv = 10;
    static integer nc = 17;
    static integer a[17] = { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,0,0 };
    static struct {
        integer e_1[7];
        integer fill_2[7];
        integer e_3;
        integer fill_4[2];
    } equiv_87 = { 3, 5, 7, 9, 11, 13, 15, {0}, 17 };

#define hi ((integer *)&equiv_87)

    static struct {
        integer e_1[7];
        integer fill_2[7];
        integer e_3;
        integer fill_4[2];
    } equiv_88 = { 2, 4, 6, 8, 10, 12, 14, {0}, 16 };

#define lo ((integer *)&equiv_88)

    static struct {
        doublereal e_1[7];
        doublereal fill_2[7];
        doublereal e_3;
        doublereal fill_4[2];
    } equiv_89 = { .3705, .2017, .5591, .1204, .2815, .4536, .7132, {0},
        .8751 };

#define xi ((doublereal *)&equiv_89)

    static integer c__[34]    /* was [2][17] */ = { 1,2,1,3,3,2,1,4,4,3,3,5,
        5,2,1,6,6,4,4,7,7,3,3,8,8,5,5,9,9,2,9,10,10,2 };
    static doublereal vval[20]    /* was [2][10] */ = { -.090572,4.4844,
        -.010856,-.7736,-.053718,-.3495,.026152,-.7286,-.058387,.1611,
        .095807,-.7978,-.031926,-.4457,-.06417,.032813,-.020636,.335,
        .040172,-.041032 };
    static doublereal v[10]    /* was [10][1] */ = { -.005,1.005,.3705,.2017,
        .5591,.1204,.2815,.4536,.7132,.8751 };

    /* System generated locals */
    doublereal ret_val;

    /* Local variables */
    extern doublereal ehg128_(doublereal *, integer *, integer *, integer *,
                              integer *, doublereal *, integer *, integer *, integer *,
                              doublereal *, integer *, doublereal *);



    /* Parameter adjustments */
    --z__;

    /* Function Body */
    ret_val = ehg128_(&z__[1], &d__, &nc, &vc, a, xi, lo, hi, c__, v, &nv,
                      vval);
    return ret_val;
} /* ehg176_ */

#undef xi
#undef lo
#undef hi


/* Subroutine */ int lowesa_(doublereal *trl, integer *n, integer *d__,
                             integer *tau, integer *nsing, doublereal *delta1, doublereal *delta2)
{
    static doublereal d1a, d1b, d2a, d2b;
    static integer dka, dkb;
    extern /* Subroutine */ int ehg141_(doublereal *, integer *, integer *,
                                        integer *, integer *, integer *, integer *, doublereal *,
                                        doublereal *);
    static doublereal alpha;

    ehg141_(trl, n, &c__1, tau, d__, nsing, &dka, &d1a, &d2a);
    ehg141_(trl, n, &c__2, tau, d__, nsing, &dkb, &d1b, &d2b);
    alpha = (doublereal) (*tau - dka) / (doublereal) (dkb - dka);
    *delta1 = (1 - alpha) * d1a + alpha * d1b;
    *delta2 = (1 - alpha) * d2a + alpha * d2b;
    return 0;
} /* lowesa_ */

/* Subroutine */ int ehg191_(integer *m, doublereal *z__, doublereal *l,
                             integer *d__, integer *n, integer *nf, integer *nv, integer *ncmax,
                             integer *vc, integer *a, doublereal *xi, integer *lo, integer *hi,
                             integer *c__, doublereal *v, integer *nvmax, doublereal *vval2,
                             doublereal *lf, integer *lq)
{
    /* System generated locals */
    integer z_dim1, z_offset, l_dim1, l_offset, v_dim1, v_offset, vval2_dim1,
    vval2_offset, lf_dim1, lf_dim2, lf_offset, lq_dim1, lq_offset,
    c_dim1, c_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer i__, j, p, i1, i2;
    static doublereal zi[8];
    static integer lq1;
    extern doublereal ehg128_(doublereal *, integer *, integer *, integer *,
                              integer *, doublereal *, integer *, integer *, integer *,
                              doublereal *, integer *, doublereal *);

    /* Args */
    /* Var */
    /* Parameter adjustments */
    z_dim1 = *m;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;
    l_dim1 = *m;
    l_offset = 1 + l_dim1;
    l -= l_offset;
    --hi;
    --lo;
    --xi;
    --a;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    lq_dim1 = *nvmax;
    lq_offset = 1 + lq_dim1;
    lq -= lq_offset;
    lf_dim1 = *d__ - 0 + 1;
    lf_dim2 = *nvmax;
    lf_offset = 0 + lf_dim1 * (1 + lf_dim2);
    lf -= lf_offset;
    vval2_dim1 = *d__ - 0 + 1;
    vval2_offset = 0 + vval2_dim1;
    vval2 -= vval2_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
        i__2 = *nv;
        for (i2 = 1; i2 <= i__2; ++i2) {
            i__3 = *d__;
            for (i1 = 0; i1 <= i__3; ++i1) {
                vval2[i1 + i2 * vval2_dim1] = 0.;
                /* L5: */
            }
            /* L4: */
        }
        i__2 = *nv;
        for (i__ = 1; i__ <= i__2; ++i__) {
            /*           linear search for i in Lq */
            lq1 = lq[i__ + lq_dim1];
            lq[i__ + lq_dim1] = j;
            p = *nf;
            /*           top of while loop */
        L7:
            if (! (lq[i__ + p * lq_dim1] != j)) {
                goto L8;
            }
            --p;
            goto L7;
            /*           bottom of while loop */
        L8:
            lq[i__ + lq_dim1] = lq1;
            if (lq[i__ + p * lq_dim1] == j) {
                i__3 = *d__;
                for (i1 = 0; i1 <= i__3; ++i1) {
                    vval2[i1 + i__ * vval2_dim1] = lf[i1 + (i__ + p * lf_dim2)
                                                      * lf_dim1];
                    /* L9: */
                }
            }
            /* L6: */
        }
        i__2 = *m;
        for (i__ = 1; i__ <= i__2; ++i__) {
            i__3 = *d__;
            for (i1 = 1; i1 <= i__3; ++i1) {
                zi[i1 - 1] = z__[i__ + i1 * z_dim1];
                /* L11: */
            }
            l[i__ + j * l_dim1] = ehg128_(zi, d__, ncmax, vc, &a[1], &xi[1], &
                                          lo[1], &hi[1], &c__[c_offset], &v[v_offset], nvmax, &
                                          vval2[vval2_offset]);
            /* L10: */
        }
        /* L3: */
    }
    return 0;
} /* ehg191_ */

/* Subroutine */ int ehg196_(integer *tau, integer *d__, doublereal *f,
                             doublereal *trl)
{
    static integer dka, dkb;
    static doublereal trla, trlb;
    extern /* Subroutine */ int ehg197_(integer *, integer *, integer *,
                                        doublereal *, integer *, doublereal *);
    static doublereal alpha;

    ehg197_(&c__1, tau, d__, f, &dka, &trla);
    ehg197_(&c__2, tau, d__, f, &dkb, &trlb);
    alpha = (doublereal) (*tau - dka) / (doublereal) (dkb - dka);
    *trl = (1 - alpha) * trla + alpha * trlb;
    return 0;
} /* ehg196_ */

/* Subroutine */ int ehg197_(integer *deg, integer *tau, integer *d__,
                             doublereal *f, integer *dk, doublereal *trl)
{
    /* System generated locals */
    doublereal d__1, d__2;

    /* Local variables */
    static doublereal g1;

    *dk = 0;
    if (*deg == 1) {
        *dk = *d__ + 1;
    }
    if (*deg == 2) {
        *dk = (integer) ((doublereal) ((*d__ + 2) * (*d__ + 1)) / 2.);
    }
    g1 = (*d__ * -.08125 + .13) * *d__ + 1.05;
    /* Computing MAX */
    d__1 = 0., d__2 = (g1 - *f) / *f;
    *trl = *dk * (max(d__1,d__2) + 1);
    return 0;
} /* ehg197_ */

/* Subroutine */ int ehg192_(doublereal *y, integer *d__, integer *n, integer
                             *nf, integer *nv, integer *nvmax, doublereal *vval, doublereal *lf,
                             integer *lq)
{
    /* System generated locals */
    integer lq_dim1, lq_offset, lf_dim1, lf_dim2, lf_offset, vval_dim1,
    vval_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer i__, j, i1, i2;
    static doublereal i3;

    /* Parameter adjustments */
    --y;
    lq_dim1 = *nvmax;
    lq_offset = 1 + lq_dim1;
    lq -= lq_offset;
    lf_dim1 = *d__ - 0 + 1;
    lf_dim2 = *nvmax;
    lf_offset = 0 + lf_dim1 * (1 + lf_dim2);
    lf -= lf_offset;
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;

    /* Function Body */
    i__1 = *nv;
    for (i2 = 1; i2 <= i__1; ++i2) {
        i__2 = *d__;
        for (i1 = 0; i1 <= i__2; ++i1) {
            vval[i1 + i2 * vval_dim1] = 0.;
            /* L4: */
        }
        /* L3: */
    }
    i__1 = *nv;
    for (i__ = 1; i__ <= i__1; ++i__) {
        i__2 = *nf;
        for (j = 1; j <= i__2; ++j) {
            i3 = y[lq[i__ + j * lq_dim1]];
            i__3 = *d__;
            for (i1 = 0; i1 <= i__3; ++i1) {
                vval[i1 + i__ * vval_dim1] += i3 * lf[i1 + (i__ + j * lf_dim2)
                                                      * lf_dim1];
                /* L7: */
            }
            /* L6: */
        }
        /* L5: */
    }
    return 0;
} /* ehg192_ */

doublereal ehg128_(doublereal *z__, integer *d__, integer *ncmax, integer *vc,
                   integer *a, doublereal *xi, integer *lo, integer *hi, integer *c__,
                   doublereal *v, integer *nvmax, doublereal *vval)
{
    /* System generated locals */
    integer c_dim1, c_offset, v_dim1, v_offset, vval_dim1, vval_offset, i__1,
    i__2;
    doublereal ret_val, d__1;

    /* Local variables */
    static doublereal g[2304]    /* was [9][256] */, h__;
    static integer i__, j, m;
    static doublereal s;
    static integer t[20];
    static doublereal g0[9], g1[9];
    static integer i1;
    static logical i2, i3, i4, i5, i6, i7, i8, i9;
    static doublereal v0, v1;
    static logical i10;
    static integer i11, i12;
    static doublereal ge;
    static integer ig, ii, lg;
    static doublereal gn;
    static integer ll;
    static doublereal gs, gw;
    static integer nt, ur;
    static doublereal gpe, gpn, gps, gpw, sew, sns, phi0, phi1, psi0, psi1;
    extern /* Subroutine */ int ehg182_(integer *), ehg184_(char *,
                                                            doublereal *, integer *, integer *, ftnlen);
    static doublereal xibar;

    /*     implicit none */
    /* Args */
    /* Vars */
    /*     locate enclosing cell */
    /* Parameter adjustments */
    --z__;
    --hi;
    --lo;
    --xi;
    --a;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    nt = 1;
    t[nt - 1] = 1;
    j = 1;
    /*     top of while loop */
L3:
    if (! (a[j] != 0)) {
        goto L4;
    }
    ++nt;
    if (z__[a[j]] <= xi[j]) {
        i1 = lo[j];
    } else {
        i1 = hi[j];
    }
    t[nt - 1] = i1;
    if (! (nt < 20)) {
        ehg182_(&c__181);
    }
    j = t[nt - 1];
    goto L3;
    /*     bottom of while loop */
L4:
    /*     tensor */
    i__1 = *vc;
    for (i12 = 1; i12 <= i__1; ++i12) {
        i__2 = *d__;
        for (i11 = 0; i11 <= i__2; ++i11) {
            g[i11 + i12 * 9 - 9] = vval[i11 + c__[i12 + j * c_dim1] *
                                        vval_dim1];
            /* L6: */
        }
        /* L5: */
    }
    lg = *vc;
    ll = c__[j * c_dim1 + 1];
    ur = c__[*vc + j * c_dim1];
    for (i__ = *d__; i__ >= 1; --i__) {
        h__ = (z__[i__] - v[ll + i__ * v_dim1]) / (v[ur + i__ * v_dim1] - v[
                                                                            ll + i__ * v_dim1]);
        if (h__ < -.001) {
            ehg184_("eval ", &z__[1], d__, &c__1, (ftnlen)5);
            ehg184_("lowerlimit ", &v[ll + v_dim1], d__, nvmax, (ftnlen)11);
        } else {
            if (1.001 < h__) {
                ehg184_("eval ", &z__[1], d__, &c__1, (ftnlen)5);
                ehg184_("upperlimit ", &v[ur + v_dim1], d__, nvmax, (ftnlen)
                        11);
            }
        }
        if (-.001 <= h__) {
            i2 = h__ <= 1.001;
        } else {
            i2 = FALSE_;
        }
        if (! i2) {
            ehg182_(&c__122);
        }
        lg = (integer) ((doublereal) lg / 2.);
        i__1 = lg;
        for (ig = 1; ig <= i__1; ++ig) {
            /*           Hermite basis */
            /* Computing 2nd power */
            d__1 = 1 - h__;
            phi0 = d__1 * d__1 * (h__ * 2 + 1);
            /* Computing 2nd power */
            d__1 = h__;
            phi1 = d__1 * d__1 * (3 - h__ * 2);
            /* Computing 2nd power */
            d__1 = 1 - h__;
            psi0 = h__ * (d__1 * d__1);
            /* Computing 2nd power */
            d__1 = h__;
            psi1 = d__1 * d__1 * (h__ - 1);
            g[ig * 9 - 9] = phi0 * g[ig * 9 - 9] + phi1 * g[(ig + lg) * 9 - 9]
            + (psi0 * g[i__ + ig * 9 - 9] + psi1 * g[i__ + (ig + lg)
                                                     * 9 - 9]) * (v[ur + i__ * v_dim1] - v[ll + i__ * v_dim1]);
            i__2 = i__ - 1;
            for (ii = 1; ii <= i__2; ++ii) {
                g[ii + ig * 9 - 9] = phi0 * g[ii + ig * 9 - 9] + phi1 * g[ii
                                                                          + (ig + lg) * 9 - 9];
                /* L9: */
            }
            /* L8: */
        }
        /* L7: */
    }
    s = g[0];
    /*     blending */
    if (*d__ == 2) {
        /*        ----- North ----- */
        v0 = v[ll + v_dim1];
        v1 = v[ur + v_dim1];
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g0[i11] = vval[i11 + c__[j * c_dim1 + 3] * vval_dim1];
            /* L10: */
        }
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g1[i11] = vval[i11 + c__[j * c_dim1 + 4] * vval_dim1];
            /* L11: */
        }
        xibar = v[ur + (v_dim1 << 1)];
        m = nt - 1;
        /*        top of while loop */
    L12:
        if (m == 0) {
            i4 = TRUE_;
        } else {
            if (a[t[m - 1]] == 2) {
                i3 = xi[t[m - 1]] == xibar;
            } else {
                i3 = FALSE_;
            }
            i4 = i3;
        }
        if (i4) {
            goto L13;
        }
        --m;
        /*           voidp junk */
        goto L12;
        /*        bottom of while loop */
    L13:
        if (m >= 1) {
            m = hi[t[m - 1]];
            /*           top of while loop */
        L14:
            if (! (a[m] != 0)) {
                goto L15;
            }
            if (z__[a[m]] <= xi[m]) {
                m = lo[m];
            } else {
                m = hi[m];
            }
            goto L14;
            /*           bottom of while loop */
        L15:
            if (v0 < v[c__[m * c_dim1 + 1] + v_dim1]) {
                v0 = v[c__[m * c_dim1 + 1] + v_dim1];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g0[i11] = vval[i11 + c__[m * c_dim1 + 1] * vval_dim1];
                    /* L16: */
                }
            }
            if (v[c__[m * c_dim1 + 2] + v_dim1] < v1) {
                v1 = v[c__[m * c_dim1 + 2] + v_dim1];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g1[i11] = vval[i11 + c__[m * c_dim1 + 2] * vval_dim1];
                    /* L17: */
                }
            }
        }
        h__ = (z__[1] - v0) / (v1 - v0);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        gn = phi0 * g0[0] + phi1 * g1[0] + (psi0 * g0[1] + psi1 * g1[1]) * (
                                                                            v1 - v0);
        gpn = phi0 * g0[2] + phi1 * g1[2];
        /*        ----- South ----- */
        v0 = v[ll + v_dim1];
        v1 = v[ur + v_dim1];
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g0[i11] = vval[i11 + c__[j * c_dim1 + 1] * vval_dim1];
            /* L18: */
        }
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g1[i11] = vval[i11 + c__[j * c_dim1 + 2] * vval_dim1];
            /* L19: */
        }
        xibar = v[ll + (v_dim1 << 1)];
        m = nt - 1;
        /*        top of while loop */
    L20:
        if (m == 0) {
            i6 = TRUE_;
        } else {
            if (a[t[m - 1]] == 2) {
                i5 = xi[t[m - 1]] == xibar;
            } else {
                i5 = FALSE_;
            }
            i6 = i5;
        }
        if (i6) {
            goto L21;
        }
        --m;
        /*           voidp junk */
        goto L20;
        /*        bottom of while loop */
    L21:
        if (m >= 1) {
            m = lo[t[m - 1]];
            /*           top of while loop */
        L22:
            if (! (a[m] != 0)) {
                goto L23;
            }
            if (z__[a[m]] <= xi[m]) {
                m = lo[m];
            } else {
                m = hi[m];
            }
            goto L22;
            /*           bottom of while loop */
        L23:
            if (v0 < v[c__[m * c_dim1 + 3] + v_dim1]) {
                v0 = v[c__[m * c_dim1 + 3] + v_dim1];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g0[i11] = vval[i11 + c__[m * c_dim1 + 3] * vval_dim1];
                    /* L24: */
                }
            }
            if (v[c__[m * c_dim1 + 4] + v_dim1] < v1) {
                v1 = v[c__[m * c_dim1 + 4] + v_dim1];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g1[i11] = vval[i11 + c__[m * c_dim1 + 4] * vval_dim1];
                    /* L25: */
                }
            }
        }
        h__ = (z__[1] - v0) / (v1 - v0);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        gs = phi0 * g0[0] + phi1 * g1[0] + (psi0 * g0[1] + psi1 * g1[1]) * (
                                                                            v1 - v0);
        gps = phi0 * g0[2] + phi1 * g1[2];
        /*        ----- East ----- */
        v0 = v[ll + (v_dim1 << 1)];
        v1 = v[ur + (v_dim1 << 1)];
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g0[i11] = vval[i11 + c__[j * c_dim1 + 2] * vval_dim1];
            /* L26: */
        }
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g1[i11] = vval[i11 + c__[j * c_dim1 + 4] * vval_dim1];
            /* L27: */
        }
        xibar = v[ur + v_dim1];
        m = nt - 1;
        /*        top of while loop */
    L28:
        if (m == 0) {
            i8 = TRUE_;
        } else {
            if (a[t[m - 1]] == 1) {
                i7 = xi[t[m - 1]] == xibar;
            } else {
                i7 = FALSE_;
            }
            i8 = i7;
        }
        if (i8) {
            goto L29;
        }
        --m;
        /*           voidp junk */
        goto L28;
        /*        bottom of while loop */
    L29:
        if (m >= 1) {
            m = hi[t[m - 1]];
            /*           top of while loop */
        L30:
            if (! (a[m] != 0)) {
                goto L31;
            }
            if (z__[a[m]] <= xi[m]) {
                m = lo[m];
            } else {
                m = hi[m];
            }
            goto L30;
            /*           bottom of while loop */
        L31:
            if (v0 < v[c__[m * c_dim1 + 1] + (v_dim1 << 1)]) {
                v0 = v[c__[m * c_dim1 + 1] + (v_dim1 << 1)];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g0[i11] = vval[i11 + c__[m * c_dim1 + 1] * vval_dim1];
                    /* L32: */
                }
            }
            if (v[c__[m * c_dim1 + 3] + (v_dim1 << 1)] < v1) {
                v1 = v[c__[m * c_dim1 + 3] + (v_dim1 << 1)];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g1[i11] = vval[i11 + c__[m * c_dim1 + 3] * vval_dim1];
                    /* L33: */
                }
            }
        }
        h__ = (z__[2] - v0) / (v1 - v0);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        ge = phi0 * g0[0] + phi1 * g1[0] + (psi0 * g0[2] + psi1 * g1[2]) * (
                                                                            v1 - v0);
        gpe = phi0 * g0[1] + phi1 * g1[1];
        /*        ----- West ----- */
        v0 = v[ll + (v_dim1 << 1)];
        v1 = v[ur + (v_dim1 << 1)];
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g0[i11] = vval[i11 + c__[j * c_dim1 + 1] * vval_dim1];
            /* L34: */
        }
        i__1 = *d__;
        for (i11 = 0; i11 <= i__1; ++i11) {
            g1[i11] = vval[i11 + c__[j * c_dim1 + 3] * vval_dim1];
            /* L35: */
        }
        xibar = v[ll + v_dim1];
        m = nt - 1;
        /*        top of while loop */
    L36:
        if (m == 0) {
            i10 = TRUE_;
        } else {
            if (a[t[m - 1]] == 1) {
                i9 = xi[t[m - 1]] == xibar;
            } else {
                i9 = FALSE_;
            }
            i10 = i9;
        }
        if (i10) {
            goto L37;
        }
        --m;
        /*           voidp junk */
        goto L36;
        /*        bottom of while loop */
    L37:
        if (m >= 1) {
            m = lo[t[m - 1]];
            /*           top of while loop */
        L38:
            if (! (a[m] != 0)) {
                goto L39;
            }
            if (z__[a[m]] <= xi[m]) {
                m = lo[m];
            } else {
                m = hi[m];
            }
            goto L38;
            /*           bottom of while loop */
        L39:
            if (v0 < v[c__[m * c_dim1 + 2] + (v_dim1 << 1)]) {
                v0 = v[c__[m * c_dim1 + 2] + (v_dim1 << 1)];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g0[i11] = vval[i11 + c__[m * c_dim1 + 2] * vval_dim1];
                    /* L40: */
                }
            }
            if (v[c__[m * c_dim1 + 4] + (v_dim1 << 1)] < v1) {
                v1 = v[c__[m * c_dim1 + 4] + (v_dim1 << 1)];
                i__1 = *d__;
                for (i11 = 0; i11 <= i__1; ++i11) {
                    g1[i11] = vval[i11 + c__[m * c_dim1 + 4] * vval_dim1];
                    /* L41: */
                }
            }
        }
        h__ = (z__[2] - v0) / (v1 - v0);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        gw = phi0 * g0[0] + phi1 * g1[0] + (psi0 * g0[2] + psi1 * g1[2]) * (
                                                                            v1 - v0);
        gpw = phi0 * g0[1] + phi1 * g1[1];
        /*        NS */
        h__ = (z__[2] - v[ll + (v_dim1 << 1)]) / (v[ur + (v_dim1 << 1)] - v[
                                                                            ll + (v_dim1 << 1)]);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        sns = phi0 * gs + phi1 * gn + (psi0 * gps + psi1 * gpn) * (v[ur + (
                                                                           v_dim1 << 1)] - v[ll + (v_dim1 << 1)]);
        /*        EW */
        h__ = (z__[1] - v[ll + v_dim1]) / (v[ur + v_dim1] - v[ll + v_dim1]);
        /*        Hermite basis */
        /* Computing 2nd power */
        d__1 = 1 - h__;
        phi0 = d__1 * d__1 * (h__ * 2 + 1);
        /* Computing 2nd power */
        d__1 = h__;
        phi1 = d__1 * d__1 * (3 - h__ * 2);
        /* Computing 2nd power */
        d__1 = 1 - h__;
        psi0 = h__ * (d__1 * d__1);
        /* Computing 2nd power */
        d__1 = h__;
        psi1 = d__1 * d__1 * (h__ - 1);
        sew = phi0 * gw + phi1 * ge + (psi0 * gpw + psi1 * gpe) * (v[ur +
                                                                     v_dim1] - v[ll + v_dim1]);
        s = sns + sew - s;
    }
    ret_val = s;
    return ret_val;
} /* ehg128_ */

integer ifloor_(doublereal *x)
{
    /* System generated locals */
    integer ret_val;

    ret_val = (integer) (*x);
    if ((doublereal) ret_val > *x) {
        --ret_val;
    }
    return ret_val;
} /* ifloor_ */

/* DSIGN is unused, causes conflicts on some platforms */
/*       DOUBLE PRECISION function DSIGN(a1,a2) */
/*       DOUBLE PRECISION a1, a2 */
/*       DSIGN=DABS(a1) */
/*       if(a2.ge.0)DSIGN=-DSIGN */
/*       end */
/* ehg136()  is the workhorse of lowesf(.) */
/*     n = number of observations */
/*     m = number of x values at which to evaluate */
/*     f = span */
/*     nf = min(n, floor(f * n)) */
/* Subroutine */ int ehg136_(doublereal *u, integer *lm, integer *m, integer *
                             n, integer *d__, integer *nf, doublereal *f, doublereal *x, integer *
                             psi, doublereal *y, doublereal *rw, integer *kernel, integer *k,
                             doublereal *dist, doublereal *eta, doublereal *b, integer *od,
                             doublereal *o, integer *ihat, doublereal *w, doublereal *rcond,
                             integer *sing, integer *dd, integer *tdeg, integer *cdeg, doublereal *
                             s)
{
    /* System generated locals */
    integer o_dim1, o_offset, b_dim1, b_offset, s_dim1, s_offset, u_dim1,
    u_offset, x_dim1, x_offset, i__1, i__2, i__3;

    /* Local variables */
    static doublereal e[225]    /* was [15][15] */, g[225]    /* was [15][
                                                               15] */;
    static integer i__, j, l;
    static doublereal q[8];
    static integer i1;
    static doublereal i2, tol;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *,
                            integer *);
    static integer info;
    static doublereal work[15];
    extern /* Subroutine */ int ehg127_(doublereal *, integer *, integer *,
                                        integer *, doublereal *, doublereal *, integer *, doublereal *,
                                        doublereal *, integer *, integer *, doublereal *, doublereal *,
                                        doublereal *, integer *, doublereal *, doublereal *, integer *,
                                        doublereal *, doublereal *, doublereal *, doublereal *,
                                        doublereal *, doublereal *, doublereal *, integer *, integer *,
                                        integer *, doublereal *), ehg182_(integer *);
    static doublereal scale, sigma[15];
    extern /* Subroutine */ int dqrsl_(doublereal *, integer *, integer *,
                                       integer *, doublereal *, doublereal *, doublereal *, doublereal *,
                                       doublereal *, doublereal *, doublereal *, integer *, integer *);
    static doublereal qraux[15], dgamma[15];
    static integer identi;

    /*     V -> g */
    /*     U -> e */
    /*     Identity -> identi */
    /*     L -> o */
    /*     X -> b */
    /* Parameter adjustments */
    o_dim1 = *m;
    o_offset = 1 + o_dim1;
    o -= o_offset;
    --dist;
    --rw;
    --y;
    --psi;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    u_dim1 = *lm;
    u_offset = 1 + u_dim1;
    u -= u_offset;
    --w;
    --eta;
    b_dim1 = *nf;
    b_offset = 1 + b_dim1;
    b -= b_offset;
    s_dim1 = *od - 0 + 1;
    s_offset = 0 + s_dim1;
    s -= s_offset;
    --cdeg;

    /* Function Body */
    if (*k > *nf - 1) {
        ehg182_(&c__104);
    }
    if (*k > 15) {
        ehg182_(&c__105);
    }
    i__1 = *n;
    for (identi = 1; identi <= i__1; ++identi) {
        psi[identi] = identi;
        /* L3: */
    }
    i__1 = *m;
    for (l = 1; l <= i__1; ++l) {
        i__2 = *d__;
        for (i1 = 1; i1 <= i__2; ++i1) {
            q[i1 - 1] = u[l + i1 * u_dim1];
            /* L5: */
        }
        ehg127_(q, n, d__, nf, f, &x[x_offset], &psi[1], &y[1], &rw[1],
                kernel, k, &dist[1], &eta[1], &b[b_offset], od, &w[1], rcond,
                sing, sigma, e, g, dgamma, qraux, work, &tol, dd, tdeg, &cdeg[
                                                                              1], &s[l * s_dim1]);
        if (*ihat == 1) {
            /*           $L sub {l,l} = */
            /*           V sub {1,:} SIGMA sup {+} U sup T */
            /*           (Q sup T W e sub i )$ */
            if (! (*m == *n)) {
                ehg182_(&c__123);
            }
            /*           find $i$ such that $l = psi sub i$ */
            i__ = 1;
            /*           top of while loop */
        L6:
            if (! (l != psi[i__])) {
                goto L7;
            }
            ++i__;
            if (! (i__ < *nf)) {
                ehg182_(&c__123);
                /* next line is not in current dloess */
                goto L7;
            }
            goto L6;
            /*           bottom of while loop */
        L7:
            i__2 = *nf;
            for (i1 = 1; i1 <= i__2; ++i1) {
                eta[i1] = 0.;
                /* L8: */
            }
            eta[i__] = w[i__];
            /*           $eta = Q sup T W e sub i$ */
            dqrsl_(&b[b_offset], nf, nf, k, qraux, &eta[1], &eta[1], &eta[1],
                   &eta[1], &eta[1], &eta[1], &c__1000, &info);
            /*           $gamma = U sup T eta sub {1:k}$ */
            i__2 = *k;
            for (i1 = 1; i1 <= i__2; ++i1) {
                dgamma[i1 - 1] = 0.;
                /* L9: */
            }
            i__2 = *k;
            for (j = 1; j <= i__2; ++j) {
                i2 = eta[j];
                i__3 = *k;
                for (i1 = 1; i1 <= i__3; ++i1) {
                    dgamma[i1 - 1] += i2 * e[j + i1 * 15 - 16];
                    /* L11: */
                }
                /* L10: */
            }
            /*           $gamma = SIGMA sup {+} gamma$ */
            i__2 = *k;
            for (j = 1; j <= i__2; ++j) {
                if (tol < sigma[j - 1]) {
                    dgamma[j - 1] /= sigma[j - 1];
                } else {
                    dgamma[j - 1] = 0.;
                }
                /* L12: */
            }
            /*           voidp junk */
            /*           voidp junk */
            o[l + o_dim1] = ddot_(k, g, &c__15, dgamma, &c__1);
        } else {
            if (*ihat == 2) {
                /*              $L sub {l,:} = */
                /*              V sub {1,:} SIGMA sup {+} */
                /*              ( U sup T Q sup T ) W $ */
                i__2 = *n;
                for (i1 = 1; i1 <= i__2; ++i1) {
                    o[l + i1 * o_dim1] = 0.;
                    /* L13: */
                }
                i__2 = *k;
                for (j = 1; j <= i__2; ++j) {
                    i__3 = *nf;
                    for (i1 = 1; i1 <= i__3; ++i1) {
                        eta[i1] = 0.;
                        /* L15: */
                    }
                    i__3 = *k;
                    for (i1 = 1; i1 <= i__3; ++i1) {
                        eta[i1] = e[i1 + j * 15 - 16];
                        /* L16: */
                    }
                    dqrsl_(&b[b_offset], nf, nf, k, qraux, &eta[1], &eta[1],
                           work, work, work, work, &c__10000, &info);
                    if (tol < sigma[j - 1]) {
                        scale = 1. / sigma[j - 1];
                    } else {
                        scale = 0.;
                    }
                    i__3 = *nf;
                    for (i1 = 1; i1 <= i__3; ++i1) {
                        eta[i1] *= scale * w[i1];
                        /* L17: */
                    }
                    i__3 = *nf;
                    for (i__ = 1; i__ <= i__3; ++i__) {
                        o[l + psi[i__] * o_dim1] += g[j * 15 - 15] * eta[i__];
                        /* L18: */
                    }
                    /* L14: */
                }
            }
        }
        /* L4: */
    }
    return 0;
} /* ehg136_ */

/* called from lowesb() ... compute fit ..?..?... */
/* somewhat similar to ehg136 */
/* Subroutine */ int ehg139_(doublereal *v, integer *nvmax, integer *nv,
                             integer *n, integer *d__, integer *nf, doublereal *f, doublereal *x,
                             integer *pi, integer *psi, doublereal *y, doublereal *rw, doublereal *
                             trl, integer *kernel, integer *k, doublereal *dist, doublereal *phi,
                             doublereal *eta, doublereal *b, integer *od, doublereal *w,
                             doublereal *diagl, doublereal *vval2, integer *ncmax, integer *vc,
                             integer *a, doublereal *xi, integer *lo, integer *hi, integer *c__,
                             integer *vhit, doublereal *rcond, integer *sing, integer *dd, integer
                             *tdeg, integer *cdeg, integer *lq, doublereal *lf, logical *setlf,
                             doublereal *s)
{
    /* System generated locals */
    integer lq_dim1, lq_offset, c_dim1, c_offset, lf_dim1, lf_dim2, lf_offset,
    b_dim1, b_offset, s_dim1, s_offset, v_dim1, v_offset, vval2_dim1,
    vval2_offset, x_dim1, x_offset, i__1, i__2, i__3, i__4;

    /* Local variables */
    static doublereal e[225]    /* was [15][15] */;
    static integer i__, j, l;
    static doublereal q[8], u[225]    /* was [15][15] */, z__[8], i1;
    static integer i2, i3;
    static doublereal i4;
    static integer i5, i6;
    static doublereal i7;
    static integer ii;
    static doublereal tol;
    static integer leaf[256];
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *,
                            integer *);
    static integer info;
    static doublereal term, work[15];
    extern /* Subroutine */ int ehg127_(doublereal *, integer *, integer *,
                                        integer *, doublereal *, doublereal *, integer *, doublereal *,
                                        doublereal *, integer *, integer *, doublereal *, doublereal *,
                                        doublereal *, integer *, doublereal *, doublereal *, integer *,
                                        doublereal *, doublereal *, doublereal *, doublereal *,
                                        doublereal *, doublereal *, doublereal *, integer *, integer *,
                                        integer *, doublereal *), ehg182_(integer *), ehg137_(doublereal *
                                                                                              , integer *, integer *, integer *, integer *, integer *, integer *
                                                                                              , integer *, integer *, doublereal *, integer *, integer *);
    extern doublereal ehg128_(doublereal *, integer *, integer *, integer *,
                              integer *, doublereal *, integer *, integer *, integer *,
                              doublereal *, integer *, doublereal *);
    static integer ileaf, nleaf;
    static doublereal scale, sigma[15];
    extern /* Subroutine */ int dqrsl_(doublereal *, integer *, integer *,
                                       integer *, doublereal *, doublereal *, doublereal *, doublereal *,
                                       doublereal *, doublereal *, doublereal *, integer *, integer *);
    static doublereal qraux[15], dgamma[15];
    static integer identi;

    /*     V -> e */
    /*     Identity -> identi */
    /*     X -> b */
    /*     l2fit with trace(L) */
    /* Parameter adjustments */
    --vhit;
    --diagl;
    --phi;
    --dist;
    --rw;
    --y;
    --psi;
    --pi;
    vval2_dim1 = *d__ - 0 + 1;
    vval2_offset = 0 + vval2_dim1;
    vval2 -= vval2_offset;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;
    lf_dim1 = *d__ - 0 + 1;
    lf_dim2 = *nvmax;
    lf_offset = 0 + lf_dim1 * (1 + lf_dim2);
    lf -= lf_offset;
    lq_dim1 = *nvmax;
    lq_offset = 1 + lq_dim1;
    lq -= lq_offset;
    --w;
    --eta;
    b_dim1 = *nf;
    b_offset = 1 + b_dim1;
    b -= b_offset;
    s_dim1 = *od - 0 + 1;
    s_offset = 0 + s_dim1;
    s -= s_offset;
    --hi;
    --lo;
    --xi;
    --a;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    --cdeg;

    /* Function Body */
    if (*k > *nf - 1) {
        ehg182_(&c__104);
    }
    if (*k > 15) {
        ehg182_(&c__105);
    }
    if (*trl != 0.) {
        i__1 = *n;
        for (i5 = 1; i5 <= i__1; ++i5) {
            diagl[i5] = 0.;
            /* L3: */
        }
        i__1 = *nv;
        for (i6 = 1; i6 <= i__1; ++i6) {
            i__2 = *d__;
            for (i5 = 0; i5 <= i__2; ++i5) {
                vval2[i5 + i6 * vval2_dim1] = 0.;
                /* L5: */
            }
            /* L4: */
        }
    }
    i__1 = *n;
    for (identi = 1; identi <= i__1; ++identi) {
        psi[identi] = identi;
        /* L6: */
    }
    i__1 = *nv;
    for (l = 1; l <= i__1; ++l) {
        i__2 = *d__;
        for (i5 = 1; i5 <= i__2; ++i5) {
            q[i5 - 1] = v[l + i5 * v_dim1];
            /* L8: */
        }
        ehg127_(q, n, d__, nf, f, &x[x_offset], &psi[1], &y[1], &rw[1],
                kernel, k, &dist[1], &eta[1], &b[b_offset], od, &w[1], rcond,
                sing, sigma, u, e, dgamma, qraux, work, &tol, dd, tdeg, &cdeg[
                                                                              1], &s[l * s_dim1]);
        if (*trl != 0.) {
            /*           invert $psi$ */
            i__2 = *n;
            for (i5 = 1; i5 <= i__2; ++i5) {
                phi[i5] = 0.;
                /* L9: */
            }
            i__2 = *nf;
            for (i__ = 1; i__ <= i__2; ++i__) {
                phi[psi[i__]] = (doublereal) i__;
                /* L10: */
            }
            i__2 = *d__;
            for (i5 = 1; i5 <= i__2; ++i5) {
                z__[i5 - 1] = v[l + i5 * v_dim1];
                /* L11: */
            }
            ehg137_(z__, &vhit[l], leaf, &nleaf, d__, nv, nvmax, ncmax, &a[1],
                    &xi[1], &lo[1], &hi[1]);
            i__2 = nleaf;
            for (ileaf = 1; ileaf <= i__2; ++ileaf) {
                i__3 = hi[leaf[ileaf - 1]];
                for (ii = lo[leaf[ileaf - 1]]; ii <= i__3; ++ii) {
                    i__ = (integer) phi[pi[ii]];
                    if (i__ != 0) {
                        if (! (psi[i__] == pi[ii])) {
                            ehg182_(&c__194);
                        }
                        i__4 = *nf;
                        for (i5 = 1; i5 <= i__4; ++i5) {
                            eta[i5] = 0.;
                            /* L14: */
                        }
                        eta[i__] = w[i__];
                        /*                    $eta = Q sup T W e sub i$ */
                        dqrsl_(&b[b_offset], nf, nf, k, qraux, &eta[1], work,
                               &eta[1], &eta[1], work, work, &c__1000, &info)
                        ;
                        i__4 = *k;
                        for (j = 1; j <= i__4; ++j) {
                            if (tol < sigma[j - 1]) {
                                i4 = ddot_(k, &u[j * 15 - 15], &c__1, &eta[1],
                                           &c__1) / sigma[j - 1];
                            } else {
                                i4 = 0.;
                            }
                            dgamma[j - 1] = i4;
                            /* L15: */
                        }
                        i__4 = *d__ + 1;
                        for (j = 1; j <= i__4; ++j) {
                            /*                       bug fix 2006-07-15 for k=1, od>1.   (thanks btyner@gmail.com) */
                            if (j <= *k) {
                                vval2[j - 1 + l * vval2_dim1] = ddot_(k, &e[j
                                                                            - 1], &c__15, dgamma, &c__1);
                            } else {
                                vval2[j - 1 + l * vval2_dim1] = 0.;
                            }
                            /* L16: */
                        }
                        i__4 = *d__;
                        for (i5 = 1; i5 <= i__4; ++i5) {
                            z__[i5 - 1] = x[pi[ii] + i5 * x_dim1];
                            /* L17: */
                        }
                        term = ehg128_(z__, d__, ncmax, vc, &a[1], &xi[1], &
                                       lo[1], &hi[1], &c__[c_offset], &v[v_offset],
                                       nvmax, &vval2[vval2_offset]);
                        diagl[pi[ii]] += term;
                        i__4 = *d__;
                        for (i5 = 0; i5 <= i__4; ++i5) {
                            vval2[i5 + l * vval2_dim1] = 0.;
                            /* L18: */
                        }
                    }
                    /* L13: */
                }
                /* L12: */
            }
        }
        if (*setlf) {
            /*           $Lf sub {:,l,:} = V SIGMA sup {+} U sup T Q sup T W$ */
            if (! (*k >= *d__ + 1)) {
                ehg182_(&c__196);
            }
            i__2 = *nf;
            for (i5 = 1; i5 <= i__2; ++i5) {
                lq[l + i5 * lq_dim1] = psi[i5];
                /* L19: */
            }
            i__2 = *nf;
            for (i6 = 1; i6 <= i__2; ++i6) {
                i__3 = *d__;
                for (i5 = 0; i5 <= i__3; ++i5) {
                    lf[i5 + (l + i6 * lf_dim2) * lf_dim1] = 0.;
                    /* L21: */
                }
                /* L20: */
            }
            i__2 = *k;
            for (j = 1; j <= i__2; ++j) {
                i__3 = *nf;
                for (i5 = 1; i5 <= i__3; ++i5) {
                    eta[i5] = 0.;
                    /* L23: */
                }
                i__3 = *k;
                for (i5 = 1; i5 <= i__3; ++i5) {
                    eta[i5] = u[i5 + j * 15 - 16];
                    /* L24: */
                }
                dqrsl_(&b[b_offset], nf, nf, k, qraux, &eta[1], &eta[1], work,
                       work, work, work, &c__10000, &info);
                if (tol < sigma[j - 1]) {
                    scale = 1. / sigma[j - 1];
                } else {
                    scale = 0.;
                }
                i__3 = *nf;
                for (i5 = 1; i5 <= i__3; ++i5) {
                    eta[i5] *= scale * w[i5];
                    /* L25: */
                }
                i__3 = *nf;
                for (i__ = 1; i__ <= i__3; ++i__) {
                    i7 = eta[i__];
                    i__4 = *d__;
                    for (i5 = 0; i5 <= i__4; ++i5) {
                        if (i5 < *k) {
                            lf[i5 + (l + i__ * lf_dim2) * lf_dim1] += e[i5 +
                                                                        1 + j * 15 - 16] * i7;
                        } else {
                            lf[i5 + (l + i__ * lf_dim2) * lf_dim1] = 0.;
                        }
                        /* L27: */
                    }
                    /* L26: */
                }
                /* L22: */
            }
        }
        /* L7: */
    }
    if (*trl != 0.) {
        if (*n <= 0) {
            *trl = 0.;
        } else {
            i3 = *n;
            i1 = diagl[i3];
            for (i2 = i3 - 1; i2 >= 1; --i2) {
                i1 = diagl[i2] + i1;
                /* L28: */
            }
            *trl = i1;
        }
    }
    return 0;
} /* ehg139_ */

/* Subroutine */ int lowesb_(doublereal *xx, doublereal *yy, doublereal *ww,
                             doublereal *diagl, integer *infl, integer *iv, integer *liv, integer *
                             lv, doublereal *wv)
{
    /* System generated locals */
    integer i__1;
    doublereal d__1;

    /* Local variables */
    static doublereal trl;
    extern /* Subroutine */ int ehg131_(doublereal *, doublereal *,
                                        doublereal *, doublereal *, doublereal *, integer *, integer *,
                                        integer *, integer *, integer *, integer *, integer *, integer *,
                                        integer *, integer *, doublereal *, integer *, integer *, integer
                                        *, integer *, integer *, integer *, doublereal *, integer *,
                                        doublereal *, doublereal *, doublereal *, doublereal *,
                                        doublereal *, integer *, doublereal *, doublereal *, doublereal *,
                                        doublereal *, integer *, integer *, integer *, integer *,
                                        integer *, doublereal *, logical *), ehg182_(integer *), ehg183_(
                                                                                                         char *, integer *, integer *, integer *, ftnlen);
    static logical setlf;
    extern integer ifloor_(doublereal *);

    /* Var */
    /* Parameter adjustments */
    --wv;
    --iv;
    --diagl;
    --ww;
    --yy;
    --xx;

    /* Function Body */
    if (! (iv[28] != 173)) {
        ehg182_(&c__174);
    }
    if (iv[28] != 172) {
        if (! (iv[28] == 171)) {
            ehg182_(&c__171);
        }
    }
    iv[28] = 173;
    if (*infl != 0) {
        trl = 1.;
    } else {
        trl = 0.;
    }
    setlf = iv[27] != iv[25];
    d__1 = iv[3] * wv[2];
    i__1 = ifloor_(&d__1);
    ehg131_(&xx[1], &yy[1], &ww[1], &trl, &diagl[1], &iv[20], &iv[29], &iv[3],
            &iv[2], &iv[5], &iv[17], &iv[4], &iv[6], &iv[14], &iv[19], &wv[1]
            , &iv[iv[7]], &iv[iv[8]], &iv[iv[9]], &iv[iv[10]], &iv[iv[22]], &
            iv[iv[27]], &wv[iv[11]], &iv[iv[23]], &wv[iv[13]], &wv[iv[12]], &
            wv[iv[15]], &wv[iv[16]], &wv[iv[18]], &i__1, &wv[3], &wv[iv[26]],
            &wv[iv[24]], &wv[4], &iv[30], &iv[33], &iv[32], &iv[41], &iv[iv[
                                                                            25]], &wv[iv[34]], &setlf);
    if ((doublereal) iv[14] < iv[6] + (doublereal) iv[4] / 2.) {
        ehg183_("k-d tree limited by memory; nvmax=", &iv[14], &c__1, &c__1, (
                                                                              ftnlen)34);
    } else {
        if (iv[17] < iv[5] + 2) {
            ehg183_("k-d tree limited by memory. ncmax=", &iv[17], &c__1, &
                    c__1, (ftnlen)34);
        }
    }
    return 0;
} /* lowesb_ */

/* lowesd() : Initialize iv(*) and v(1:4) */
/* ------     called only by loess_workspace()  in ./loessc.c */
/* Subroutine */ int lowesd_(integer *versio, integer *iv, integer *liv,
                             integer *lv, doublereal *v, integer *d__, integer *n, doublereal *f,
                             integer *ideg, integer *nvmax, integer *setlf)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1;

    /* Builtin functions */
    integer pow_ii(integer *, integer *);

    /* Local variables */
    static integer i__, j, i1, i2, nf, vc;
    extern /* Subroutine */ int ehg182_(integer *);
    static integer ncmax, bound;
    extern integer ifloor_(doublereal *);


    /*     unnecessary initialization of i1 to keep g77 -Wall happy */

    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */
    i1 = 0;
    /*     version -> versio */
    if (! (*versio == 106)) {
        ehg182_(&c__100);
    }
    iv[28] = 171;
    iv[2] = *d__;
    iv[3] = *n;
    vc = pow_ii(&c__2, d__);
    iv[4] = vc;
    if (! (0. < *f)) {
        ehg182_(&c__120);
    }
    /* Computing MIN */
    d__1 = *n * *f;
    i__1 = *n, i__2 = ifloor_(&d__1);
    nf = min(i__1,i__2);
    iv[19] = nf;
    iv[20] = 1;
    if (*ideg == 0) {
        i1 = 1;
    } else {
        if (*ideg == 1) {
            i1 = *d__ + 1;
        } else {
            if (*ideg == 2) {
                i1 = (integer) ((doublereal) ((*d__ + 2) * (*d__ + 1)) / 2.);
            }
        }
    }
    iv[29] = i1;
    iv[21] = 1;
    iv[14] = *nvmax;
    ncmax = *nvmax;
    iv[17] = ncmax;
    iv[30] = 0;
    iv[32] = *ideg;
    if (! (*ideg >= 0)) {
        ehg182_(&c__195);
    }
    if (! (*ideg <= 2)) {
        ehg182_(&c__195);
    }
    iv[33] = *d__;
    for (i2 = 41; i2 <= 49; ++i2) {
        iv[i2] = *ideg;
        /* L3: */
    }
    iv[7] = 50;
    iv[8] = iv[7] + ncmax;
    iv[9] = iv[8] + vc * ncmax;
    iv[10] = iv[9] + ncmax;
    iv[22] = iv[10] + ncmax;
    /*     initialize permutation */
    j = iv[22] - 1;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        iv[j + i__] = i__;
        /* L4: */
    }
    iv[23] = iv[22] + *n;
    iv[25] = iv[23] + *nvmax;
    if (*setlf != 0) {
        iv[27] = iv[25] + *nvmax * nf;
    } else {
        iv[27] = iv[25];
    }
    bound = iv[27] + *n;
    if (! (bound - 1 <= *liv)) {
        ehg182_(&c__102);
    }
    iv[11] = 50;
    iv[13] = iv[11] + *nvmax * *d__;
    iv[12] = iv[13] + (*d__ + 1) * *nvmax;
    iv[15] = iv[12] + ncmax;
    iv[16] = iv[15] + *n;
    iv[18] = iv[16] + nf;
    iv[24] = iv[18] + iv[29] * nf;
    iv[34] = iv[24] + (*d__ + 1) * *nvmax;
    if (*setlf != 0) {
        iv[26] = iv[34] + (*d__ + 1) * *nvmax * nf;
    } else {
        iv[26] = iv[34];
    }
    bound = iv[26] + nf;
    if (! (bound - 1 <= *lv)) {
        ehg182_(&c__103);
    }
    v[1] = *f;
    v[2] = .05;
    v[3] = 0.;
    v[4] = 1.;
    return 0;
} /* lowesd_ */

/* Subroutine */ int lowese_(integer *iv, integer *liv, integer *lv,
                             doublereal *wv, integer *m, doublereal *z__, doublereal *s)
{
    /* System generated locals */
    integer z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int ehg133_(integer *, integer *, integer *,
                                        integer *, integer *, integer *, integer *, integer *, integer *,
                                        integer *, doublereal *, doublereal *, doublereal *, integer *,
                                        doublereal *, doublereal *), ehg182_(integer *);

    /* Parameter adjustments */
    --iv;
    --wv;
    --s;
    z_dim1 = *m;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;

    /* Function Body */
    if (! (iv[28] != 172)) {
        ehg182_(&c__172);
    }
    if (! (iv[28] == 173)) {
        ehg182_(&c__173);
    }
    ehg133_(&iv[3], &iv[2], &iv[4], &iv[14], &iv[5], &iv[17], &iv[iv[7]], &iv[
                                                                              iv[8]], &iv[iv[9]], &iv[iv[10]], &wv[iv[11]], &wv[iv[13]], &wv[iv[
                                                                                                                                                12]], m, &z__[z_offset], &s[1]);
    return 0;
} /* lowese_ */

/* "direct" (non-"interpolate") fit aka predict() : */
/* Subroutine */ int lowesf_(doublereal *xx, doublereal *yy, doublereal *ww,
                             integer *iv, integer *liv, integer *lv, doublereal *wv, integer *m,
                             doublereal *z__, doublereal *l, integer *ihat, doublereal *s)
{
    /* System generated locals */
    integer z_dim1, z_offset, l_dim1, l_offset;

    /* Local variables */
    static logical i1;
    extern /* Subroutine */ int ehg136_(doublereal *, integer *, integer *,
                                        integer *, integer *, integer *, doublereal *, doublereal *,
                                        integer *, doublereal *, doublereal *, integer *, integer *,
                                        doublereal *, doublereal *, doublereal *, integer *, doublereal *,
                                        integer *, doublereal *, doublereal *, integer *, integer *,
                                        integer *, integer *, doublereal *), ehg182_(integer *);

    /*     m = number of x values at which to evaluate */
    /* Parameter adjustments */
    --xx;
    --yy;
    --ww;
    --iv;
    --wv;
    --s;
    l_dim1 = *m;
    l_offset = 1 + l_dim1;
    l -= l_offset;
    z_dim1 = *m;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;

    /* Function Body */
    if (171 <= iv[28]) {
        i1 = iv[28] <= 174;
    } else {
        i1 = FALSE_;
    }
    if (! i1) {
        ehg182_(&c__171);
    }
    iv[28] = 172;
    if (! (iv[14] >= iv[19])) {
        ehg182_(&c__186);
    }
    /* do the work; in ehg136()  give the argument names as they are there: */
    /*          ehg136(u,lm,m, n,    d,    nf,   f,   x,   psi,     y ,rw, */
    ehg136_(&z__[z_offset], m, m, &iv[3], &iv[2], &iv[19], &wv[1], &xx[1], &
            iv[iv[22]], &yy[1], &ww[1], &iv[20], &iv[29], &wv[iv[15]], &wv[iv[
                                                                              16]], &wv[iv[18]], &c__0, &l[l_offset], ihat, &wv[iv[26]], &wv[4],
            &iv[30], &iv[33], &iv[32], &iv[41], &s[1]);
    /*          kernel,  k,     dist,       eta,       b,     od,o,ihat, */
    /*              w,     rcond,sing,    dd,    tdeg,cdeg,  s) */
    return 0;
} /* lowesf_ */

/* Subroutine */ int lowesl_(integer *iv, integer *liv, integer *lv,
                             doublereal *wv, integer *m, doublereal *z__, doublereal *l)
{
    /* System generated locals */
    integer l_dim1, l_offset, z_dim1, z_offset;

    /* Local variables */
    extern /* Subroutine */ int ehg182_(integer *), ehg191_(integer *,
                                                            doublereal *, doublereal *, integer *, integer *, integer *,
                                                            integer *, integer *, integer *, integer *, doublereal *, integer
                                                            *, integer *, integer *, doublereal *, integer *, doublereal *,
                                                            doublereal *, integer *);

    /* Parameter adjustments */
    --iv;
    --wv;
    l_dim1 = *m;
    l_offset = 1 + l_dim1;
    l -= l_offset;
    z_dim1 = *m;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;

    /* Function Body */
    if (! (iv[28] != 172)) {
        ehg182_(&c__172);
    }
    if (! (iv[28] == 173)) {
        ehg182_(&c__173);
    }
    if (! (iv[26] != iv[34])) {
        ehg182_(&c__175);
    }
    ehg191_(m, &z__[z_offset], &l[l_offset], &iv[2], &iv[3], &iv[19], &iv[6],
            &iv[17], &iv[4], &iv[iv[7]], &wv[iv[12]], &iv[iv[10]], &iv[iv[9]],
            &iv[iv[8]], &wv[iv[11]], &iv[14], &wv[iv[24]], &wv[iv[34]], &iv[
                                                                            iv[25]]);
    return 0;
} /* lowesl_ */

/* Subroutine */ int lowesr_(doublereal *yy, integer *iv, integer *liv,
                             integer *lv, doublereal *wv)
{
    extern /* Subroutine */ int ehg182_(integer *), ehg192_(doublereal *,
                                                            integer *, integer *, integer *, integer *, integer *, doublereal
                                                            *, doublereal *, integer *);

    /* Parameter adjustments */
    --wv;
    --iv;
    --yy;

    /* Function Body */
    if (! (iv[28] != 172)) {
        ehg182_(&c__172);
    }
    if (! (iv[28] == 173)) {
        ehg182_(&c__173);
    }
    ehg192_(&yy[1], &iv[2], &iv[3], &iv[19], &iv[6], &iv[14], &wv[iv[13]], &
            wv[iv[34]], &iv[iv[25]]);
    return 0;
} /* lowesr_ */

/* Subroutine */ int lowesw_(doublereal *res, integer *n, doublereal *rw,
                             integer *pi)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1, d__2;

    /* Local variables */
    static integer i__, i1, nh;
    static doublereal cmad;
    extern /* Subroutine */ int ehg106_(integer *, integer *, integer *,
                                        integer *, doublereal *, integer *, integer *);
    extern doublereal d1mach_(integer *);
    static integer identi;
    static doublereal rsmall;
    extern integer ifloor_(doublereal *);

    /* Tranliterated from Devlin's ratfor */
    /*     implicit none */
    /* Args */
    /* Var */
    /*     Identity -> identi */
    /*     find median of absolute residuals */
    /* Parameter adjustments */
    --pi;
    --rw;
    --res;

    /* Function Body */
    i__1 = *n;
    for (i1 = 1; i1 <= i__1; ++i1) {
        rw[i1] = (d__1 = res[i1], abs(d__1));
        /* L3: */
    }
    i__1 = *n;
    for (identi = 1; identi <= i__1; ++identi) {
        pi[identi] = identi;
        /* L4: */
    }
    d__1 = (doublereal) (*n) / 2.;
    nh = ifloor_(&d__1) + 1;
    /*     partial sort to find 6*mad */
    ehg106_(&c__1, n, &nh, &c__1, &rw[1], &pi[1], n);
    if (*n - nh + 1 < nh) {
        i__1 = nh - 1;
        i__2 = nh - 1;
        ehg106_(&c__1, &i__1, &i__2, &c__1, &rw[1], &pi[1], n);
        cmad = (rw[pi[nh]] + rw[pi[nh - 1]]) * 3;
    } else {
        cmad = rw[pi[nh]] * 6;
    }
    rsmall = d1mach_(&c__1);
    if (cmad < rsmall) {
        i__1 = *n;
        for (i1 = 1; i1 <= i__1; ++i1) {
            rw[i1] = 1.;
            /* L5: */
        }
    } else {
        i__1 = *n;
        for (i__ = 1; i__ <= i__1; ++i__) {
            if (cmad * .999 < rw[i__]) {
                rw[i__] = 0.;
            } else {
                if (cmad * .001 < rw[i__]) {
                    /* Computing 2nd power */
                    d__2 = rw[i__] / cmad;
                    /* Computing 2nd power */
                    d__1 = 1 - d__2 * d__2;
                    rw[i__] = d__1 * d__1;
                } else {
                    rw[i__] = 1.;
                }
            }
            /* L6: */
        }
    }
    return 0;
} /* lowesw_ */

/* Subroutine */ int lowesp_(integer *n, doublereal *y, doublereal *yhat,
                             doublereal *pwgts, doublereal *rwgts, integer *pi, doublereal *ytilde)
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static doublereal c__;
    static integer i__, m;
    static doublereal i1;
    static integer i2, i3;
    static doublereal i4, mad;
    extern /* Subroutine */ int ehg106_(integer *, integer *, integer *,
                                        integer *, doublereal *, integer *, integer *);
    extern integer ifloor_(doublereal *);

    /* Var */
    /*     median absolute deviation (using partial sort): */
    /* Parameter adjustments */
    --ytilde;
    --pi;
    --rwgts;
    --pwgts;
    --yhat;
    --y;

    /* Function Body */
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        ytilde[i__] = (d__1 = y[i__] - yhat[i__], abs(d__1)) * sqrt(pwgts[i__]
                                                                    );
        pi[i__] = i__;
        /* L3: */
    }
    d__1 = (doublereal) (*n) / 2.;
    m = ifloor_(&d__1) + 1;
    ehg106_(&c__1, n, &m, &c__1, &ytilde[1], &pi[1], n);
    if (*n - m + 1 < m) {
        i__1 = m - 1;
        i__2 = m - 1;
        ehg106_(&c__1, &i__1, &i__2, &c__1, &ytilde[1], &pi[1], n);
        mad = (ytilde[pi[m - 1]] + ytilde[pi[m]]) / 2;
    } else {
        mad = ytilde[pi[m]];
    }
    /*     magic constant */
    /* Computing 2nd power */
    d__1 = mad * 6;
    c__ = d__1 * d__1 / 5;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        /* Computing 2nd power */
        d__1 = y[i__] - yhat[i__];
        ytilde[i__] = 1 - d__1 * d__1 * pwgts[i__] / c__;
        /* L5: */
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        ytilde[i__] *= sqrt(rwgts[i__]);
        /* L6: */
    }
    if (*n <= 0) {
        i4 = 0.;
    } else {
        i3 = *n;
        i1 = ytilde[i3];
        for (i2 = i3 - 1; i2 >= 1; --i2) {
            i1 = ytilde[i2] + i1;
            /* L7: */
        }
        i4 = i1;
    }
    c__ = *n / i4;
    /*     pseudovalues */
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
        ytilde[i__] = yhat[i__] + c__ * rwgts[i__] * (y[i__] - yhat[i__]);
        /* L8: */
    }
    return 0;
} /* lowesp_ */

/* Subroutine */ int ehg124_(integer *ll, integer *uu, integer *d__, integer *
                             n, integer *nv, integer *nc, integer *ncmax, integer *vc, doublereal *
                             x, integer *pi, integer *a, doublereal *xi, integer *lo, integer *hi,
                             integer *c__, doublereal *v, integer *vhit, integer *nvmax, integer *
                             fc, doublereal *fd, integer *dd)
{
    /* System generated locals */
    integer c_dim1, c_offset, v_dim1, v_offset, x_dim1, x_offset, i__1, i__2,
    i__3, i__4;
    doublereal d__1;

    /* Builtin functions */
    double sqrt(doublereal);
    integer pow_ii(integer *, integer *);

    /* Local variables */
    static integer k, l, m, p, u;
    static logical i1, i2;
    static integer i4;
    static doublereal diag[8];
    static logical leaf;
    static doublereal diam;
    extern /* Subroutine */ int ehg125_(integer *, integer *, doublereal *,
                                        integer *, integer *, integer *, integer *, doublereal *, integer
                                        *, integer *, integer *, integer *, integer *), ehg106_(integer *,
                                                                                                integer *, integer *, integer *, doublereal *, integer *,
                                                                                                integer *), ehg129_(integer *, integer *, integer *, doublereal *,
                                                                                                                    integer *, integer *, doublereal *);
    static integer check;
    static doublereal sigma[8];
    static integer lower, upper, inorm2;
    extern integer idamax_(integer *, doublereal *, integer *);
    static integer offset;

    /* Parameter adjustments */
    --pi;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --hi;
    --lo;
    --xi;
    --a;
    c_dim1 = *vc;
    c_offset = 1 + c_dim1;
    c__ -= c_offset;
    --vhit;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    p = 1;
    l = *ll;
    u = *uu;
    lo[p] = l;
    hi[p] = u;
    /*     top of while loop */
L3:
    if (! (p <= *nc)) {
        goto L4;
    }
    i__1 = *dd;
    for (i4 = 1; i4 <= i__1; ++i4) {
        diag[i4 - 1] = v[c__[*vc + p * c_dim1] + i4 * v_dim1] - v[c__[p *
                                                                      c_dim1 + 1] + i4 * v_dim1];
        /* L5: */
    }
    diam = 0.;
    i__1 = *dd;
    for (inorm2 = 1; inorm2 <= i__1; ++inorm2) {
        /* Computing 2nd power */
        d__1 = diag[inorm2 - 1];
        diam += d__1 * d__1;
        /* L6: */
    }
    diam = sqrt(diam);
    if (u - l + 1 <= *fc) {
        i1 = TRUE_;
    } else {
        i1 = diam <= *fd;
    }
    if (i1) {
        leaf = TRUE_;
    } else {
        if (*ncmax < *nc + 2) {
            i2 = TRUE_;
        } else {
            i2 = (doublereal) (*nvmax) < *nv + (doublereal) (*vc) / 2.;
        }
        leaf = i2;
    }
    if (! leaf) {
        ehg129_(&l, &u, dd, &x[x_offset], &pi[1], n, sigma);
        k = idamax_(dd, sigma, &c__1);
        m = (integer) ((doublereal) (l + u) / 2.);
        ehg106_(&l, &u, &m, &c__1, &x[k * x_dim1 + 1], &pi[1], n);
        /*           all ties go with hi son */
        /*           top of while loop */
        /*     bug fix from btyner@gmail.com 2006-07-20 */
        offset = 0;
    L7:
        if (m + offset >= u || m + offset < l) {
            goto L8;
        }
        if (offset < 0) {
            lower = l;
            check = m + offset;
            upper = check;
        } else {
            lower = m + offset + 1;
            check = lower;
            upper = u;
        }
        ehg106_(&lower, &upper, &check, &c__1, &x[k * x_dim1 + 1], &pi[1], n);
        if (x[pi[m + offset] + k * x_dim1] == x[pi[m + offset + 1] + k *
                                                x_dim1]) {
            offset = -offset;
            if (offset >= 0) {
                ++offset;
            }
            goto L7;
        } else {
            m += offset;
            goto L8;
        }
        /*           bottom of while loop */
    L8:
        if (v[c__[p * c_dim1 + 1] + k * v_dim1] == x[pi[m] + k * x_dim1]) {
            leaf = TRUE_;
        } else {
            leaf = v[c__[*vc + p * c_dim1] + k * v_dim1] == x[pi[m] + k *
                                                              x_dim1];
        }
    }
    if (leaf) {
        a[p] = 0;
    } else {
        a[p] = k;
        xi[p] = x[pi[m] + k * x_dim1];
        /*           left son */
        ++(*nc);
        lo[p] = *nc;
        lo[*nc] = l;
        hi[*nc] = m;
        /*           right son */
        ++(*nc);
        hi[p] = *nc;
        lo[*nc] = m + 1;
        hi[*nc] = u;
        i__2 = k - 1;
        i__1 = pow_ii(&c__2, &i__2);
        i__4 = *d__ - k;
        i__3 = pow_ii(&c__2, &i__4);
        ehg125_(&p, nv, &v[v_offset], &vhit[1], nvmax, d__, &k, &xi[p], &i__1,
                &i__3, &c__[p * c_dim1 + 1], &c__[lo[p] * c_dim1 + 1], &c__[
                                                                            hi[p] * c_dim1 + 1]);
    }
    ++p;
    l = lo[p];
    u = hi[p];
    goto L3;
    /*     bottom of while loop */
L4:
    return 0;
} /* ehg124_ */

/* Subroutine */ int ehg129_(integer *l, integer *u, integer *d__, doublereal
                             *x, integer *pi, integer *n, doublereal *sigma)
{
    /* Initialized data */

    static integer execnt = 0;

    /* System generated locals */
    integer x_dim1, x_offset, i__1, i__2;
    doublereal d__1, d__2;

    /* Local variables */
    static integer i__, k;
    static doublereal t, beta, alpha;
    extern doublereal d1mach_(integer *);
    static doublereal machin;

    /* Parameter adjustments */
    --sigma;
    --pi;
    x_dim1 = *n;
    x_offset = 1 + x_dim1;
    x -= x_offset;

    /* Function Body */
    /*     MachInf -> machin */
    ++execnt;
    if (execnt == 1) {
        /*     initialize  d1mach(2) === DBL_MAX: */
        machin = d1mach_(&c__2);
    }
    i__1 = *d__;
    for (k = 1; k <= i__1; ++k) {
        alpha = machin;
        beta = -machin;
        i__2 = *u;
        for (i__ = *l; i__ <= i__2; ++i__) {
            t = x[pi[i__] + k * x_dim1];
            /* Computing MIN */
            d__1 = alpha, d__2 = x[pi[i__] + k * x_dim1];
            alpha = min(d__1,d__2);
            beta = max(beta,t);
            /* L4: */
        }
        sigma[k] = beta - alpha;
        /* L3: */
    }
    return 0;
} /* ehg129_ */

/* {called only from ehg127}  purpose...?... */
/* Subroutine */ int ehg137_(doublereal *z__, integer *kappa, integer *leaf,
                             integer *nleaf, integer *d__, integer *nv, integer *nvmax, integer *
                             ncmax, integer *a, doublereal *xi, integer *lo, integer *hi)
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    static integer p;
    extern /* Subroutine */ int ehg182_(integer *);
    static integer pstack[20], stackt;

    /*     stacktop -> stackt */
    /*     find leaf cells affected by $z$ */
    /* Parameter adjustments */
    --leaf;
    --z__;
    --hi;
    --lo;
    --xi;
    --a;

    /* Function Body */
    stackt = 0;
    p = 1;
    *nleaf = 0;
    /*     top of while loop */
L3:
    if (! (0 < p)) {
        goto L4;
    }
    if (a[p] == 0) {
        /*           leaf */
        ++(*nleaf);
        leaf[*nleaf] = p;
        /*           Pop */
        if (stackt >= 1) {
            p = pstack[stackt - 1];
        } else {
            p = 0;
        }
        /* Computing MAX */
        i__1 = 0, i__2 = stackt - 1;
        stackt = max(i__1,i__2);
    } else {
        if (z__[a[p]] == xi[p]) {
            /*              Push */
            ++stackt;
            if (! (stackt <= 20)) {
                ehg182_(&c__187);
            }
            pstack[stackt - 1] = hi[p];
            p = lo[p];
        } else {
            if (z__[a[p]] <= xi[p]) {
                p = lo[p];
            } else {
                p = hi[p];
            }
        }
    }
    goto L3;
    /*     bottom of while loop */
L4:
    if (! (*nleaf <= 256)) {
        ehg182_(&c__185);
    }
    return 0;
} /* ehg137_ */

/* -- For Error messaging, call the "a" routines at the bottom of  ./loessc.c  : */
/* Subroutine */ int ehg183_(char *s, integer *i__, integer *n, integer *inc,
                             ftnlen s_len)
{
    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    integer i_len(char *, ftnlen);

    /* Local variables */
    extern /* Subroutine */ int ehg183a_(char *, integer *, integer *,
                                         integer *, integer *, ftnlen);

    i__1 = i_len(s, s_len);
    ehg183a_(s, &i__1, i__, n, inc, s_len);
    return 0;
} /* ehg183_ */

/* Subroutine */ int ehg184_(char *s, doublereal *x, integer *n, integer *inc,
                             ftnlen s_len)
{
    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    integer i_len(char *, ftnlen);

    /* Local variables */
    extern /* Subroutine */ int ehg184a_(char *, integer *, doublereal *,
                                         integer *, integer *, ftnlen);

    i__1 = i_len(s, s_len);
    ehg184a_(s, &i__1, x, n, inc, s_len);
    return 0;
} /* ehg184_ */
