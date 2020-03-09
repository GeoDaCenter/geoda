#include "S.h"
#include "loess.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>


void *safe_malloc(size_t n, unsigned long line)
{
    void *p = malloc(n);

    if (!p) {
        fprintf(stderr, "[%s:%lu] Out of memory (%lu bytes)\n",
                __FILE__, line, (unsigned long)n);
        exit(EXIT_FAILURE);
    }

    return p;
}


/* static functions */
static double _fmin(double a, double b)
{
    return(a < b ? a : b);
}

static double _fmax(double a, double b)
{
    return(a > b ? a : b);
}

/*
 * Rational approximation to inverse Gaussian distribution.
 * Absolute error is bounded by 4.5e-4.
 * Reference: Abramowitz and Stegun, page 933.
 * Assumption: 0 < p < 1.
 */
static double num[] = {
        2.515517,
        0.802853,
        0.010328
};

static double den[] = {
        1.000000,
        1.432788,
        0.189269,
        0.001308
};

double invigauss_quick(double p)
{
    int lower;
    double t, n, d, q;

    if(p == 0.5)
        return(0);
    lower = p < 0.5;
    p = lower ? p : 1 - p;
    t = sqrt(-2 * log(p));
    n = (num[2]*t + num[1])*t + num[0];
    d = ((den[3]*t + den[2])*t + den[1])*t + den[0];
    q = lower ? n/d - t : t - n/d;
    return(q);
}

/*
 * Quick approximation to inverse incomplete beta function,
 * by matching first two moments with the Gaussian distribution.
 * Assumption: 0 < p < 1, a,b > 0.
 */

static double invibeta_quick(double p, double a, double b)
{
    double x, m, s;

    x = a + b;
    m = a / x;
    s = sqrt((a*b) / (x*x*(x+1)));
    return(_fmax(0.0, _fmin(1.0, invigauss_quick(p)*s + m)));
}


/*
 * Inverse incomplete beta function.
 * Assumption: 0 <= p <= 1, a,b > 0.
 */
static double invibeta(double p, double a, double b)
{
    int i;
    double ql, qr, qm, qdiff;
    double pl, pr, pm, pdiff;

/*        MEANINGFUL(qm);*/
    qm = 0;
    if(p == 0 || p == 1)
        return(p);

    /* initialize [ql,qr] containing the root */
    ql = qr = invibeta_quick(p, a, b);
    pl = pr = ibeta(ql, a, b);
    if(pl == p)
        return(ql);
    if(pl < p)
        while(1) {
            qr += 0.05;
            if(qr >= 1) {
                pr = qr = 1;
                break;
                }
            pr = ibeta(qr, a, b);
            if(pr == p)
                return(pr);
            if(pr > p)
                break;
        }
    else
        while(1) {
            ql -= 0.05;
            if(ql <= 0) {
                pl = ql = 0;
                break;
                }
            pl = ibeta(ql, a, b);
            if(pl == p)
                return(pl);
            if(pl < p)
                break;
        }

    /* a few steps of bisection */
    for(i = 0; i < 5; i++) {
        qm = (ql + qr) / 2;
        pm = ibeta(qm, a, b);
        qdiff = qr - ql;
        pdiff = pm - p;
        if(fabs(qdiff) < DBL_EPSILON*qm || fabs(pdiff) < DBL_EPSILON)
            return(qm);
        if(pdiff < 0) {
            ql = qm;
            pl = pm;
        } else {
            qr = qm;
            pr = pm;
        }
    }

    /* a few steps of secant */
    for(i = 0; i < 40; i++) {
        qm = ql + (p-pl)*(qr-ql)/(pr-pl);
        pm = ibeta(qm, a, b);
        qdiff = qr - ql;
        pdiff = pm - p;
        if(fabs(qdiff) < 2*DBL_EPSILON*qm || fabs(pdiff) < 2*DBL_EPSILON)
            return(qm);
        if(pdiff < 0) {
            ql = qm;
            pl = pm;
        } else {
            qr = qm;
            pr = pm;
        }
    }

    /* no convergence */
    return(qm);
}


static double qt(double p, double df)
{
    double t;
    t = invibeta(fabs(2*p-1), 0.5, df/2);
    return((p>0.5?1:-1) * sqrt(t*df/(1-t)));
}


double pf(double q, double df1, double df2)
{
    return(ibeta(q*df1/(df2+q*df1), df1/2, df2/2));
}

/**********************************************************************/
 /*
 * Incomplete beta function.
 * Reference:  Abramowitz and Stegun, 26.5.8.
 * Assumptions: 0 <= x <= 1; a,b > 0.
 */

double ibeta(double x, double a, double b)
{
    int flipped = 0, i, k, count;
    double I, temp, pn[6], ak, bk, next, prev, factor, val;

    if (x <= 0)
        return(0);
    if (x >= 1)
        return(1);

    /* use ibeta(x,a,b) = 1-ibeta(1-x,b,a) */
    if ((a+b+1)*x > (a+1)) {
        flipped = 1;
        temp = a;
        a = b;
        b = temp;
        x = 1 - x;
    }

    pn[0] = 0.0;
    pn[2] = pn[3] = pn[1] = 1.0;
    count = 1;
    val = x/(1.0-x);
    bk = 1.0;
    next = 1.0;
    do {
        count++;
        k = count/2;
        prev = next;
        if (count%2 == 0)
            ak = -((a+k-1.0)*(b-k)*val)/((a+2.0*k-2.0)*(a+2.0*k-1.0));
        else
            ak = ((a+b+k-1.0)*k*val)/((a+2.0*k)*(a+2.0*k-1.0));
        pn[4] = bk*pn[2] + ak*pn[0];
        pn[5] = bk*pn[3] + ak*pn[1];
        next = pn[4] / pn[5];
        for (i=0; i<=3; i++)
            pn[i] = pn[i+2];
        if (fabs(pn[4]) >= DBL_MAX)
            for (i=0; i<=3; i++)
                pn[i] /= DBL_MAX;
        if (fabs(pn[4]) <= DBL_MIN)
            for (i=0; i<=3; i++)
                pn[i] /= DBL_MIN;
    } while (fabs(next-prev) > DBL_EPSILON*prev);
    factor = a*log(x) + (b-1)*log(1-x);
    factor -= lgamma(a+1) + lgamma(b) - lgamma(a+b);
    I = exp(factor) * next;
    return(flipped ? 1-I : I);
}


void anova(loess *one, loess *two, anova_struct *out)
{
    double  one_d1, one_d2, one_s, two_d1, two_d2, two_s,
            rssdiff, d1diff, tmp;
    int max_enp;

    one_d1 = one->outputs->one_delta;
    one_d2 = one->outputs->two_delta;
    one_s = one->outputs->residual_scale;
    two_d1 = two->outputs->one_delta;
    two_d2 = two->outputs->two_delta;
    two_s = two->outputs->residual_scale;

    rssdiff = fabs(one_s * one_s * one_d1 - two_s * two_s * two_d1);
    d1diff = fabs(one_d1 - two_d1);
    out->dfn = d1diff * d1diff / fabs(one_d2 - two_d2);
    max_enp = (one->outputs->enp > two->outputs->enp);
    tmp = max_enp ? one_d1 : two_d1;
    out->dfd = tmp * tmp / (max_enp ? one_d2 : two_d2);
    tmp = max_enp ? one_s : two_s;
    out->F_value = (rssdiff / d1diff) / (tmp * tmp);
    out->Pr_F = 1 - pf(out->F_value, out->dfn, out->dfd);
}


void pointwise(prediction *pre, double coverage,
      confidence_intervals *ci)
{
    double t_dist, limit, fit;
    int i;

    ci->fit = MALLOC(pre->m * sizeof(double));
    ci->upper = MALLOC(pre->m * sizeof(double));
    ci->lower = MALLOC(pre->m * sizeof(double));

    t_dist = qt(1 - (1 - coverage)/2, pre->df);
    for(i = 0; i < pre->m; i++) {
        limit = pre->se_fit[i] * t_dist;
        ci->fit[i] = fit = pre->fit[i];
        ci->upper[i] = fit + limit;
        ci->lower[i] = fit - limit;
    }
}


void pw_free_mem(confidence_intervals *ci)
{
    free(ci->fit);
    free(ci->upper);
    free(ci->lower);
}



typedef double doublereal;
typedef int integer;

void Recover(char *a, int *b)
{
    printf("%s", a);
    exit(1);
}

/*  d1mach may be replaced by Fortran code:
    mail netlib@netlib.bell-labs.com
    send d1mach from core.
*/

doublereal F77_SUB(d1mach) (integer *i)
{
switch(*i){
    case 1: return DBL_MIN;
    case 2: return DBL_MAX;
    case 3: return DBL_EPSILON/FLT_RADIX;
    case 4: return DBL_EPSILON;
    case 5: return log10(FLT_RADIX);
    default: Recover("Invalid argument to d1mach()", 0L);
    }
}
