//----------------------------------------------------------------------
//      File:           ANN.cc
//      Programmer:     Sunil Arya and David Mount
//      Last modified:  03/04/98 (Release 0.1)
//      Description:    Methods for ANN.h and ANNx.h
//----------------------------------------------------------------------
// Copyright (c) 1997-1998 University of Maryland and Sunil Arya and David
// Mount.  All Rights Reserved.
// 
// This software and related documentation is part of the 
// Approximate Nearest Neighbor Library (ANN).
// 
// Permission to use, copy, and distribute this software and its 
// documentation is hereby granted free of charge, provided that 
// (1) it is not a component of a commercial product, and 
// (2) this notice appears in all copies of the software and
//     related documentation. 
// 
// The University of Maryland (U.M.) and the authors make no representations
// about the suitability or fitness of this software for any purpose.  It is
// provided "as is" without express or implied warranty.
//----------------------------------------------------------------------

#include "ANN.h"			// ANN includes
#include "ANNx.h"			// all ANN includes

//----------------------------------------------------------------------
//  Point methods
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//  Distance utility.
//	(Note: In the nearest neighbor search, most distances are
//	computed using partial distance calculations, not this
//	procedure.)
//----------------------------------------------------------------------

ANNdist annDist(			// interpoint squared distance
    int			dim,
    ANNpoint		p,
    ANNpoint		q)
{
    register int d;
    register ANNcoord diff;
    register ANNcoord dist;

    dist = 0;
    for (d = 0; d < dim; d++) {
	diff = p[d] - q[d];
	dist = ANN_SUM(dist, ANN_POW(diff));
    }
    return dist;
}

//----------------------------------------------------------------------
//  annPrintPoint() prints a point to a given output stream.
//----------------------------------------------------------------------

void annPrintPt(			// print a point
    ANNpoint		pt,		// the point
    int			dim,		// the dimension
    ostream		&out)		// output stream
{
    for (int j = 0; j < dim; j++) {
       	out << pt[j];
	if (j < dim-1) out << " ";
    }
}

//----------------------------------------------------------------------
//  Point allocation/deallocation:
//
//	Because points (somewhat like strings in C) are stored
//	as pointers.  Consequently, creating and destroying
//	copies of points may require storage allocation.  These
//	procedures do this.
//
//	annAllocPt() and annDeallocPt() allocate a deallocate
//	storage for a single point, and return a pointer to it.
//
//	annAllocPts() allocates an array of points as well a place
//	to store their coordinates, and initializes the points to
//	point to their respective coordinates.  It allocates point
//	storage in a contiguous block large enough to store all the
//	points.  It performs no initialization.
//
// 	annDeallocPts() should only be used on point arrays allocated
//	by annAllocPts since it assumes that points are allocated in
//	a block.
//
//	annCopyPt() copies a point taking care to allocate storage
//	for the new point.
//
//	annAssignRect() assigns the coordinates of one rectangle to
//	another.  The two rectangles must have the same dimension
//	(and it is not possible to test this here).
//----------------------------------------------------------------------

ANNpoint annAllocPt(int dim, ANNcoord c)	// allocate 1 point
{
    ANNpoint p = new ANNcoord[dim];
    for (int i = 0; i < dim; i++) p[i] = c;
    return p;
}
   
ANNpointArray annAllocPts(int n, int dim)	// allocate n pts in dim
{
    ANNpointArray pa = new ANNpoint[n];		// allocate points
    ANNpoint	  p  = new ANNcoord[n*dim];	// allocate space for coords
    for (int i = 0; i < n; i++) {
			pa[i] = &(p[i*dim]);
    }
    return pa;
}

void annDeallocPt(ANNpoint &p)			// deallocate 1 point
{
    delete [] p;
    p = NULL;
}
   
void annDeallocPts(ANNpointArray &pa)		// deallocate points
{
    delete [] pa[0];				// dealloc coordinate storage
    delete [] pa;				// dealloc points
    pa = NULL;
}
   
ANNpoint annCopyPt(int dim, ANNpoint source)	// copy point
{
    ANNpoint p = new ANNcoord[dim];
    for (int i = 0; i < dim; i++) p[i] = source[i];
    return p;
}
   
						// assign one rect to another
void annAssignRect(int dim, ANNorthRect &dest, const ANNorthRect &source)
{
    for (int i = 0; i < dim; i++) {
	dest.lo[i] = source.lo[i];
	dest.hi[i] = source.hi[i];
    }
}

						// is point inside rectangle?
ANNbool ANNorthRect::inside(int dim, ANNpoint p)
{
    for (int i = 0; i < dim; i++) {
	if (p[i] < lo[i] || p[i] > hi[i]) return ANNfalse;
    }
    return ANNtrue;
}

//----------------------------------------------------------------------
//  Error handler
//----------------------------------------------------------------------

void annError(char *msg, ANNerr level)
{
    if (level == ANNabort) {
	cerr << "ANN: ERROR------->" << msg << "<-------------ERROR\n";
    	exit(1);
    }
    else {
	cerr << "ANN: WARNING----->" << msg << "<-------------WARNING\n";
    }
}

//----------------------------------------------------------------------
//  Limit on number of points visited
//	We have an option for terminating the search early if the
//	number of points visited exceeds some threshold.  If the
//	threshold is 0 (its default)  this means there is no limit
//	and the algorithm applies its normal termination condition.
//	This is for applications where there are real time constraints
//	on the running time of the algorithm.
//----------------------------------------------------------------------

int		ANNmaxPtsVisited = 0;	// maximum number of pts visited
int		ANNptsVisited;		// number of pts visited in search

//----------------------------------------------------------------------
//  Global function declarations
//----------------------------------------------------------------------

void annMaxPtsVisit(		// set limit on max. pts to visit in search
    int                 maxPts)		// the limit
{
    ANNmaxPtsVisited = maxPts;
}
