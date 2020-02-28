/* supp.f -- translated by f2c (version 20160102).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

#include "../../Regression/f2c.h"
#include "../../Regression/blaswrap.h"

/* Table of constant values */

static integer c__9 = 9;
static integer c__1 = 1;
static integer c__3 = 3;
static integer c__5 = 5;
static integer c__2 = 2;
static integer c__102 = 102;
static integer c__103 = 103;

/* Subroutine */ int ehg182_(integer *i__)
{
    /* Builtin functions */
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);
    /* Subroutine */ int s_stop(char *, ftnlen);

    /* Fortran I/O blocks */
    static cilist io___1 = { 0, 6, 0, 0, 0 };
    static cilist io___2 = { 0, 6, 0, 0, 0 };
    static cilist io___3 = { 0, 6, 0, 0, 0 };
    static cilist io___4 = { 0, 6, 0, 0, 0 };
    static cilist io___5 = { 0, 6, 0, 0, 0 };
    static cilist io___6 = { 0, 6, 0, 0, 0 };
    static cilist io___7 = { 0, 6, 0, 0, 0 };
    static cilist io___8 = { 0, 6, 0, 0, 0 };
    static cilist io___9 = { 0, 6, 0, 0, 0 };
    static cilist io___10 = { 0, 6, 0, 0, 0 };
    static cilist io___11 = { 0, 6, 0, 0, 0 };
    static cilist io___12 = { 0, 6, 0, 0, 0 };
    static cilist io___13 = { 0, 6, 0, 0, 0 };
    static cilist io___14 = { 0, 6, 0, 0, 0 };
    static cilist io___15 = { 0, 6, 0, 0, 0 };
    static cilist io___16 = { 0, 6, 0, 0, 0 };
    static cilist io___17 = { 0, 6, 0, 0, 0 };
    static cilist io___18 = { 0, 6, 0, 0, 0 };
    static cilist io___19 = { 0, 6, 0, 0, 0 };
    static cilist io___20 = { 0, 6, 0, 0, 0 };
    static cilist io___21 = { 0, 6, 0, 0, 0 };
    static cilist io___22 = { 0, 6, 0, 0, 0 };
    static cilist io___23 = { 0, 6, 0, 0, 0 };
    static cilist io___24 = { 0, 6, 0, 0, 0 };
    static cilist io___25 = { 0, 6, 0, 0, 0 };
    static cilist io___26 = { 0, 6, 0, 0, 0 };
    static cilist io___27 = { 0, 6, 0, 0, 0 };
    static cilist io___28 = { 0, 6, 0, 0, 0 };
    static cilist io___29 = { 0, 6, 0, 0, 0 };
    static cilist io___30 = { 0, 6, 0, 0, 0 };
    static cilist io___31 = { 0, 6, 0, 0, 0 };
    static cilist io___32 = { 0, 6, 0, 0, 0 };
    static cilist io___33 = { 0, 6, 0, 0, 0 };
    static cilist io___34 = { 0, 6, 0, 0, 0 };
    static cilist io___35 = { 0, 6, 0, 0, 0 };
    static cilist io___36 = { 0, 6, 0, 0, 0 };


    if (*i__ == 100) {
	s_wsle(&io___1);
	do_lio(&c__9, &c__1, "wrong version number in lowesd.  Probably typo"
		" in caller.", (ftnlen)57);
	e_wsle();
    }
    if (*i__ == 101) {
	s_wsle(&io___2);
	do_lio(&c__9, &c__1, "d>dMAX in ehg131.  Need to recompile with incr"
		"eased dimensions.", (ftnlen)63);
	e_wsle();
    }
    if (*i__ == 102) {
	s_wsle(&io___3);
	do_lio(&c__9, &c__1, "liv too small.   (Discovered by lowesd)", (
		ftnlen)39);
	e_wsle();
    }
    if (*i__ == 103) {
	s_wsle(&io___4);
	do_lio(&c__9, &c__1, "lv too small.    (Discovered by lowesd)", (
		ftnlen)39);
	e_wsle();
    }
    if (*i__ == 104) {
	s_wsle(&io___5);
	do_lio(&c__9, &c__1, "alpha too small.  fewer data values than degre"
		"es of freedom.", (ftnlen)60);
	e_wsle();
    }
    if (*i__ == 105) {
	s_wsle(&io___6);
	do_lio(&c__9, &c__1, "k>d2MAX in ehg136.  Need to recompile with inc"
		"reased dimensions.", (ftnlen)64);
	e_wsle();
    }
    if (*i__ == 106) {
	s_wsle(&io___7);
	do_lio(&c__9, &c__1, "lwork too small", (ftnlen)15);
	e_wsle();
    }
    if (*i__ == 107) {
	s_wsle(&io___8);
	do_lio(&c__9, &c__1, "invalid value for kernel", (ftnlen)24);
	e_wsle();
    }
    if (*i__ == 108) {
	s_wsle(&io___9);
	do_lio(&c__9, &c__1, "invalid value for ideg", (ftnlen)22);
	e_wsle();
    }
    if (*i__ == 109) {
	s_wsle(&io___10);
	do_lio(&c__9, &c__1, "lowstt only applies when kernel=1.", (ftnlen)34)
		;
	e_wsle();
    }
    if (*i__ == 110) {
	s_wsle(&io___11);
	do_lio(&c__9, &c__1, "not enough extra workspace for robustness calc"
		"ulation", (ftnlen)53);
	e_wsle();
    }
    if (*i__ == 120) {
	s_wsle(&io___12);
	do_lio(&c__9, &c__1, "zero-width neighborhood. make alpha bigger", (
		ftnlen)42);
	e_wsle();
    }
    if (*i__ == 121) {
	s_wsle(&io___13);
	do_lio(&c__9, &c__1, "all data on boundary of neighborhood. make alp"
		"ha bigger", (ftnlen)55);
	e_wsle();
    }
    if (*i__ == 122) {
	s_wsle(&io___14);
	do_lio(&c__9, &c__1, "extrapolation not allowed with blending", (
		ftnlen)39);
	e_wsle();
    }
    if (*i__ == 123) {
	s_wsle(&io___15);
	do_lio(&c__9, &c__1, "ihat=1 (diag L) in l2fit only makes sense if z"
		"=x (eval=data).", (ftnlen)61);
	e_wsle();
    }
    if (*i__ == 171) {
	s_wsle(&io___16);
	do_lio(&c__9, &c__1, "lowesd must be called first.", (ftnlen)28);
	e_wsle();
    }
    if (*i__ == 172) {
	s_wsle(&io___17);
	do_lio(&c__9, &c__1, "lowesf must not come between lowesb and lowese"
		", lowesr, or lowesl.", (ftnlen)66);
	e_wsle();
    }
    if (*i__ == 173) {
	s_wsle(&io___18);
	do_lio(&c__9, &c__1, "lowesb must come before lowese, lowesr, or low"
		"esl.", (ftnlen)50);
	e_wsle();
    }
    if (*i__ == 174) {
	s_wsle(&io___19);
	do_lio(&c__9, &c__1, "lowesb need not be called twice.", (ftnlen)32);
	e_wsle();
    }
    if (*i__ == 180) {
	s_wsle(&io___20);
	do_lio(&c__9, &c__1, "nv>nvmax in cpvert.", (ftnlen)19);
	e_wsle();
    }
    if (*i__ == 181) {
	s_wsle(&io___21);
	do_lio(&c__9, &c__1, "nt>20 in eval.", (ftnlen)14);
	e_wsle();
    }
    if (*i__ == 182) {
	s_wsle(&io___22);
	do_lio(&c__9, &c__1, "svddc failed in l2fit.", (ftnlen)22);
	e_wsle();
    }
    if (*i__ == 183) {
	s_wsle(&io___23);
	do_lio(&c__9, &c__1, "didnt find edge in vleaf.", (ftnlen)25);
	e_wsle();
    }
    if (*i__ == 184) {
	s_wsle(&io___24);
	do_lio(&c__9, &c__1, "zero-width cell found in vleaf.", (ftnlen)31);
	e_wsle();
    }
    if (*i__ == 185) {
	s_wsle(&io___25);
	do_lio(&c__9, &c__1, "trouble descending to leaf in vleaf.", (ftnlen)
		36);
	e_wsle();
    }
    if (*i__ == 186) {
	s_wsle(&io___26);
	do_lio(&c__9, &c__1, "insufficient workspace for lowesf.", (ftnlen)34)
		;
	e_wsle();
    }
    if (*i__ == 187) {
	s_wsle(&io___27);
	do_lio(&c__9, &c__1, "insufficient stack space", (ftnlen)24);
	e_wsle();
    }
    if (*i__ == 188) {
	s_wsle(&io___28);
	do_lio(&c__9, &c__1, "lv too small for computing explicit L", (ftnlen)
		37);
	e_wsle();
    }
    if (*i__ == 191) {
	s_wsle(&io___29);
	do_lio(&c__9, &c__1, "computed trace L was negative; something is wr"
		"ong!", (ftnlen)50);
	e_wsle();
    }
    if (*i__ == 192) {
	s_wsle(&io___30);
	do_lio(&c__9, &c__1, "computed delta was negative; something is wron"
		"g!", (ftnlen)48);
	e_wsle();
    }
    if (*i__ == 193) {
	s_wsle(&io___31);
	do_lio(&c__9, &c__1, "workspace in loread appears to be corrupted", (
		ftnlen)43);
	e_wsle();
    }
    if (*i__ == 194) {
	s_wsle(&io___32);
	do_lio(&c__9, &c__1, "trouble in l2fit/l2tr", (ftnlen)21);
	e_wsle();
    }
    if (*i__ == 195) {
	s_wsle(&io___33);
	do_lio(&c__9, &c__1, "only constant, linear, or quadratic local mode"
		"ls allowed", (ftnlen)56);
	e_wsle();
    }
    if (*i__ == 196) {
	s_wsle(&io___34);
	do_lio(&c__9, &c__1, "degree must be at least 1 for vertex influence"
		" matrix", (ftnlen)53);
	e_wsle();
    }
    if (*i__ == 999) {
	s_wsle(&io___35);
	do_lio(&c__9, &c__1, "not yet implemented", (ftnlen)19);
	e_wsle();
    }
    s_wsle(&io___36);
    do_lio(&c__9, &c__1, "Assert failed, error code ", (ftnlen)26);
    do_lio(&c__3, &c__1, (char *)&(*i__), (ftnlen)sizeof(integer));
    e_wsle();
    s_stop("", (ftnlen)0);
    return 0;
} /* ehg182_ */

/* Subroutine */ int ehg183_(char *s, integer *i__, integer *n, integer *inc, 
	ftnlen s_len)
{
    /* System generated locals */
    integer i_dim1, i_offset, i__1;

    /* Builtin functions */
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);

    /* Local variables */
    static integer j;

    /* Fortran I/O blocks */
    static cilist io___37 = { 0, 6, 0, 0, 0 };


    /* Parameter adjustments */
    i_dim1 = *inc;
    i_offset = 1 + i_dim1;
    i__ -= i_offset;

    /* Function Body */
    s_wsle(&io___37);
    do_lio(&c__9, &c__1, s, s_len);
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	do_lio(&c__3, &c__1, (char *)&i__[j * i_dim1 + 1], (ftnlen)sizeof(
		integer));
    }
    e_wsle();
    return 0;
} /* ehg183_ */

/* Subroutine */ int ehg184_(char *s, doublereal *x, integer *n, integer *inc,
	 ftnlen s_len)
{
    /* System generated locals */
    integer x_dim1, x_offset, i__1;

    /* Builtin functions */
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);

    /* Local variables */
    static integer j;

    /* Fortran I/O blocks */
    static cilist io___39 = { 0, 6, 0, 0, 0 };


    /* Parameter adjustments */
    x_dim1 = *inc;
    x_offset = 1 + x_dim1;
    x -= x_offset;

    /* Function Body */
    s_wsle(&io___39);
    do_lio(&c__9, &c__1, s, s_len);
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	do_lio(&c__5, &c__1, (char *)&x[j * x_dim1 + 1], (ftnlen)sizeof(
		doublereal));
    }
    e_wsle();
    return 0;
} /* ehg184_ */

/* Subroutine */ int losave_(integer *iunit, integer *iv, integer *liv, 
	integer *lv, doublereal *v)
{
    /* Initialized data */

    static integer execnt = 0;

    extern /* Subroutine */ int ehg167_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, doublereal *);

    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */
    ++execnt;
    ehg167_(iunit, &iv[2], &iv[4], &iv[5], &iv[6], &iv[14], &v[iv[11]], &iv[
	    iv[7]], &v[iv[12]], &v[iv[13]]);
    return 0;
} /* losave_ */

/* Subroutine */ int ehg167_(integer *iunit, integer *d__, integer *vc, 
	integer *nc, integer *nv, integer *nvmax, doublereal *v, integer *a, 
	doublereal *xi, doublereal *vval)
{
    /* System generated locals */
    integer v_dim1, v_offset, vval_dim1, vval_offset, i__1, i__2;

    /* Builtin functions */
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void);

    /* Local variables */
    static integer i__, j;

    /* Fortran I/O blocks */
    static cilist io___42 = { 0, 0, 0, 0, 0 };
    static cilist io___44 = { 0, 0, 0, 0, 0 };
    static cilist io___46 = { 0, 0, 0, 0, 0 };
    static cilist io___47 = { 0, 0, 0, 0, 0 };
    static cilist io___48 = { 0, 0, 0, 0, 0 };


    /* Parameter adjustments */
    --xi;
    --a;
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    io___42.ciunit = *iunit;
    s_wsle(&io___42);
    do_lio(&c__3, &c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    do_lio(&c__3, &c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    do_lio(&c__3, &c__1, (char *)&(*nv), (ftnlen)sizeof(integer));
    e_wsle();
    i__1 = *d__;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L10: */
	io___44.ciunit = *iunit;
	s_wsle(&io___44);
	do_lio(&c__5, &c__1, (char *)&v[i__ * v_dim1 + 1], (ftnlen)sizeof(
		doublereal));
	do_lio(&c__5, &c__1, (char *)&v[*vc + i__ * v_dim1], (ftnlen)sizeof(
		doublereal));
	e_wsle();
    }
    j = 0;
    i__1 = *nc;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (a[i__] != 0) {
	    io___46.ciunit = *iunit;
	    s_wsle(&io___46);
	    do_lio(&c__3, &c__1, (char *)&a[i__], (ftnlen)sizeof(integer));
	    do_lio(&c__5, &c__1, (char *)&xi[i__], (ftnlen)sizeof(doublereal))
		    ;
	    e_wsle();
	} else {
	    io___47.ciunit = *iunit;
	    s_wsle(&io___47);
	    do_lio(&c__3, &c__1, (char *)&a[i__], (ftnlen)sizeof(integer));
	    do_lio(&c__3, &c__1, (char *)&j, (ftnlen)sizeof(integer));
	    e_wsle();
	}
/* L20: */
    }
    i__1 = *nv;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L30: */
	io___48.ciunit = *iunit;
	s_wsle(&io___48);
	i__2 = *d__;
	for (j = 0; j <= i__2; ++j) {
	    do_lio(&c__5, &c__1, (char *)&vval[j + i__ * vval_dim1], (ftnlen)
		    sizeof(doublereal));
	}
	e_wsle();
    }
    return 0;
} /* ehg167_ */

/* Subroutine */ int lohead_(integer *iunit, integer *d__, integer *vc, 
	integer *nc, integer *nv)
{
    /* Builtin functions */
    integer s_rsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_rsle(void), pow_ii(integer *, integer *);

    /* Fortran I/O blocks */
    static cilist io___49 = { 0, 0, 0, 0, 0 };


    io___49.ciunit = *iunit;
    s_rsle(&io___49);
    do_lio(&c__3, &c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    do_lio(&c__3, &c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    do_lio(&c__3, &c__1, (char *)&(*nv), (ftnlen)sizeof(integer));
    e_rsle();
    *vc = pow_ii(&c__2, d__);
    return 0;
} /* lohead_ */

/* Subroutine */ int loread_(integer *iunit, integer *d__, integer *vc, 
	integer *nc, integer *nv, integer *iv, integer *liv, integer *lv, 
	doublereal *v)
{
    /* Initialized data */

    static integer execnt = 0;

    extern /* Subroutine */ int ehg182_(integer *), ehg168_(integer *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, integer *, doublereal *, doublereal *), ehg169_(integer *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, integer *, doublereal *, integer *, integer *, integer *);
    static integer bound;

    /* Parameter adjustments */
    --iv;
    --v;

    /* Function Body */
    ++execnt;
    iv[28] = 173;
    iv[2] = *d__;
    iv[4] = *vc;
    iv[14] = *nv;
    iv[17] = *nc;
    iv[7] = 50;
    iv[8] = iv[7] + *nc;
    iv[9] = iv[8] + *vc * *nc;
    iv[10] = iv[9] + *nc;
    bound = iv[10] + *nc;
    if (! (bound - 1 <= *liv)) {
	ehg182_(&c__102);
    }
    iv[11] = 50;
    iv[13] = iv[11] + *nv * *d__;
    iv[12] = iv[13] + (*d__ + 1) * *nv;
    bound = iv[12] + *nc;
    if (! (bound - 1 <= *lv)) {
	ehg182_(&c__103);
    }
    ehg168_(iunit, d__, vc, nc, nv, nv, &v[iv[11]], &iv[iv[7]], &v[iv[12]], &
	    v[iv[13]]);
    ehg169_(d__, vc, nc, nc, nv, nv, &v[iv[11]], &iv[iv[7]], &v[iv[12]], &iv[
	    iv[8]], &iv[iv[9]], &iv[iv[10]]);
    return 0;
} /* loread_ */

/* Subroutine */ int ehg168_(integer *iunit, integer *d__, integer *vc, 
	integer *nc, integer *nv, integer *nvmax, doublereal *v, integer *a, 
	doublereal *xi, doublereal *vval)
{
    /* System generated locals */
    integer v_dim1, v_offset, vval_dim1, vval_offset, i__1, i__2;

    /* Builtin functions */
    integer s_rsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_rsle(void);

    /* Local variables */
    static integer i__, j;

    /* Fortran I/O blocks */
    static cilist io___53 = { 0, 0, 0, 0, 0 };
    static cilist io___54 = { 0, 0, 0, 0, 0 };
    static cilist io___55 = { 0, 0, 0, 0, 0 };


    /* Parameter adjustments */
    --xi;
    --a;
    vval_dim1 = *d__ - 0 + 1;
    vval_offset = 0 + vval_dim1;
    vval -= vval_offset;
    v_dim1 = *nvmax;
    v_offset = 1 + v_dim1;
    v -= v_offset;

    /* Function Body */
    i__1 = *d__;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L10: */
	io___53.ciunit = *iunit;
	s_rsle(&io___53);
	do_lio(&c__5, &c__1, (char *)&v[i__ * v_dim1 + 1], (ftnlen)sizeof(
		doublereal));
	do_lio(&c__5, &c__1, (char *)&v[*vc + i__ * v_dim1], (ftnlen)sizeof(
		doublereal));
	e_rsle();
    }
    i__1 = *nc;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L20: */
	io___54.ciunit = *iunit;
	s_rsle(&io___54);
	do_lio(&c__3, &c__1, (char *)&a[i__], (ftnlen)sizeof(integer));
	do_lio(&c__5, &c__1, (char *)&xi[i__], (ftnlen)sizeof(doublereal));
	e_rsle();
    }
    i__1 = *nv;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L30: */
	io___55.ciunit = *iunit;
	s_rsle(&io___55);
	i__2 = *d__;
	for (j = 0; j <= i__2; ++j) {
	    do_lio(&c__5, &c__1, (char *)&vval[j + i__ * vval_dim1], (ftnlen)
		    sizeof(doublereal));
	}
	e_rsle();
    }
    return 0;
} /* ehg168_ */

/* Subroutine */ int ehg170_(integer *k, integer *d__, integer *vc, integer *
	nv, integer *nvmax, integer *nc, integer *ncmax, integer *a, integer *
	c__, integer *hi, integer *lo, doublereal *v, doublereal *vval, 
	doublereal *xi)
{
    /* Initialized data */

    static integer execnt = 0;

    /* Format strings */
    static char fmt_50[] = "(\002      double precision z(\002,i2,\002)\002)";
    static char fmt_51[] = "(\002      integer a(\002,i5,\002), c(\002,i3"
	    ",\002,\002,i5,\002)\002)";
    static char fmt_52[] = "(\002      integer hi(\002,i5,\002), lo(\002,i5"
	    ",\002)\002)";
    static char fmt_53[] = "(\002      double precision v(\002,i5,\002,\002,"
	    "i2,\002)\002)";
    static char fmt_54[] = "(\002      double precision vval(0:\002,i2,\002"
	    ",\002,i5,\002)\002)";
    static char fmt_55[] = "(\002      double precision xi(\002,i5,\002)\002)"
	    ;
    static char fmt_56[] = "(\002      double precision ehg128\002)";
    static char fmt_57[] = "(\002      data d,vc,nv,nc /\002,i2,\002,\002,"
	    "i3,\002,\002,i5,\002,\002,i5,\002/\002)";
    static char fmt_58[] = "(\002      data a(\002,i5,\002) /\002,i5,\002"
	    "/\002)";
    static char fmt_59[] = "(\002      data hi(\002,i5,\002),lo(\002,i5,\002"
	    "),xi(\002,i5,\002) /\002,i5,\002,\002,i5,\002,\002,1pe15.6,\002"
	    "/\002)";
    static char fmt_60[] = "(\002      data c(\002,i3,\002,\002,i5,\002) "
	    "/\002,i5,\002/\002)";
    static char fmt_61[] = "(\002      data vval(0,\002,i5,\002) /\002,1pe15"
	    ".6,\002/\002)";
    static char fmt_62[] = "(\002      data v(\002,i5,\002,\002,i2,\002) "
	    "/\002,1pe15.6,\002/\002)";
    static char fmt_63[] = "(\002      data vval(\002,i2,\002,\002,i5,\002"
	    ") /\002,1pe15.6,\002/\002)";

    /* System generated locals */
    integer c_dim1, c_offset, v_dim1, v_offset, vval_dim1, vval_offset, i__1, 
	    i__2;

    /* Builtin functions */
    integer s_wsle(cilist *), do_lio(integer *, integer *, char *, ftnlen), 
	    e_wsle(void), s_wsfe(cilist *), do_fio(integer *, char *, ftnlen),
	     e_wsfe(void);

    /* Local variables */
    static integer i__, j;

    /* Fortran I/O blocks */
    static cilist io___58 = { 0, 0, 0, 0, 0 };
    static cilist io___59 = { 0, 0, 0, fmt_50, 0 };
    static cilist io___60 = { 0, 0, 0, 0, 0 };
    static cilist io___61 = { 0, 0, 0, fmt_51, 0 };
    static cilist io___62 = { 0, 0, 0, fmt_52, 0 };
    static cilist io___63 = { 0, 0, 0, fmt_53, 0 };
    static cilist io___64 = { 0, 0, 0, fmt_54, 0 };
    static cilist io___65 = { 0, 0, 0, fmt_55, 0 };
    static cilist io___66 = { 0, 0, 0, fmt_56, 0 };
    static cilist io___67 = { 0, 0, 0, fmt_57, 0 };
    static cilist io___69 = { 0, 0, 0, fmt_58, 0 };
    static cilist io___70 = { 0, 0, 0, fmt_59, 0 };
    static cilist io___72 = { 0, 0, 0, fmt_60, 0 };
    static cilist io___73 = { 0, 0, 0, fmt_61, 0 };
    static cilist io___74 = { 0, 0, 0, fmt_62, 0 };
    static cilist io___75 = { 0, 0, 0, fmt_63, 0 };
    static cilist io___76 = { 0, 0, 0, 0, 0 };
    static cilist io___77 = { 0, 0, 0, 0, 0 };


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

    /* Function Body */
    ++execnt;
    io___58.ciunit = *k;
    s_wsle(&io___58);
    do_lio(&c__9, &c__1, "      double precision function loeval(z)", (ftnlen)
	    41);
    e_wsle();
    io___59.ciunit = *k;
    s_wsfe(&io___59);
    do_fio(&c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    e_wsfe();
    io___60.ciunit = *k;
    s_wsle(&io___60);
    do_lio(&c__9, &c__1, "      integer d,vc,nv,nc", (ftnlen)24);
    e_wsle();
    io___61.ciunit = *k;
    s_wsfe(&io___61);
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*vc), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    e_wsfe();
    io___62.ciunit = *k;
    s_wsfe(&io___62);
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    e_wsfe();
    io___63.ciunit = *k;
    s_wsfe(&io___63);
    do_fio(&c__1, (char *)&(*nv), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    e_wsfe();
    io___64.ciunit = *k;
    s_wsfe(&io___64);
    do_fio(&c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nv), (ftnlen)sizeof(integer));
    e_wsfe();
    io___65.ciunit = *k;
    s_wsfe(&io___65);
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    e_wsfe();
    io___66.ciunit = *k;
    s_wsfe(&io___66);
    e_wsfe();
    io___67.ciunit = *k;
    s_wsfe(&io___67);
    do_fio(&c__1, (char *)&(*d__), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*vc), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nv), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nc), (ftnlen)sizeof(integer));
    e_wsfe();
    i__1 = *nc;
    for (i__ = 1; i__ <= i__1; ++i__) {
	io___69.ciunit = *k;
	s_wsfe(&io___69);
	do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&a[i__], (ftnlen)sizeof(integer));
	e_wsfe();
	if (a[i__] != 0) {
	    io___70.ciunit = *k;
	    s_wsfe(&io___70);
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&hi[i__], (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&lo[i__], (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&xi[i__], (ftnlen)sizeof(doublereal));
	    e_wsfe();
	}
	i__2 = *vc;
	for (j = 1; j <= i__2; ++j) {
	    io___72.ciunit = *k;
	    s_wsfe(&io___72);
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&c__[j + i__ * c_dim1], (ftnlen)sizeof(
		    integer));
	    e_wsfe();
/* L4: */
	}
/* L3: */
    }
    i__1 = *nv;
    for (i__ = 1; i__ <= i__1; ++i__) {
	io___73.ciunit = *k;
	s_wsfe(&io___73);
	do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&vval[i__ * vval_dim1], (ftnlen)sizeof(
		doublereal));
	e_wsfe();
	i__2 = *d__;
	for (j = 1; j <= i__2; ++j) {
	    io___74.ciunit = *k;
	    s_wsfe(&io___74);
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&v[i__ + j * v_dim1], (ftnlen)sizeof(
		    doublereal));
	    e_wsfe();
	    io___75.ciunit = *k;
	    s_wsfe(&io___75);
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&vval[j + i__ * vval_dim1], (ftnlen)sizeof(
		    doublereal));
	    e_wsfe();
/* L6: */
	}
/* L5: */
    }
    io___76.ciunit = *k;
    s_wsle(&io___76);
    do_lio(&c__9, &c__1, "      loeval=ehg128(z,d,nc,vc,a,xi,lo,hi,c,v,nv,vv"
	    "al)", (ftnlen)53);
    e_wsle();
    io___77.ciunit = *k;
    s_wsle(&io___77);
    do_lio(&c__9, &c__1, "      end", (ftnlen)9);
    e_wsle();
    return 0;
} /* ehg170_ */

/* Subroutine */ int lofort_(integer *iunit, integer *iv, integer *liv, 
	integer *lv, doublereal *wv)
{
    /* Initialized data */

    static integer execnt = 0;

    extern /* Subroutine */ int ehg170_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *);

    /* Parameter adjustments */
    --wv;
    --iv;

    /* Function Body */
    ++execnt;
    ehg170_(iunit, &iv[2], &iv[4], &iv[6], &iv[14], &iv[5], &iv[17], &iv[iv[7]
	    ], &iv[iv[8]], &iv[iv[9]], &iv[iv[10]], &wv[iv[11]], &wv[iv[13]], 
	    &wv[iv[12]]);
    return 0;
} /* lofort_ */

