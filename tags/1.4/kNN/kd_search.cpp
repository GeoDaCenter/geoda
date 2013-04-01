//----------------------------------------------------------------------
//	File:		kd_search.cc
//	Programmer:	Sunil Arya and David Mount
//	Last modified:	03/04/98 (Release 0.1)
//	Description:	Standard kd-tree search
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

#include "ANN.h"
#include "ANNx.h"			// all ANN includes
#include "ANNperf.h"
#include "../GenGeomAlgs.h"

#include "kd_search.h"			// kd-search declarations

//----------------------------------------------------------------------
//  Approximate nearest neighbor searching by kd-tree search
//	The kd-tree is searched for an approximate nearest neighbor.
//	The point is returned through one of the arguments, and the
//	distance returned is the squared distance to this point.
//
//	The method used for searching the kd-tree is an approximate
//	adaptation of the search algorithm described by Friedman,
//	Bentley, and Finkel, ``An algorithm for finding best matches
//	in logarithmic expected time,'' ACM Transactions on Mathematical
//	Software, 3(3):209-226, 1977).
//
//	The algorithm operates recursively.  When first encountering a
//	node of the kd-tree we first visit the child which is closest to
//	the query point.  On return, we decide whether we want to visit
//	the other child.  If the box containing the other child exceeds
//	1/(1+eps) times the current best distance, then we skip it (since
//	any point found in this child cannot be closer to the query point
//	by more than this factor.)  Otherwise, we visit it recursively.
//	The distance between a box and the query point is computed exactly
//	(not approximated as is often done in kd-tree), using incremental
//	distance updates, as described by Arya and Mount in ``Algorithms
//	for fast vector quantization,'' Proc.  of DCC '93: Data Compression
//	Conference, eds. J. A. Storer and M. Cohn, IEEE Press, 1993,
//	381-390.
//
//	The main entry points is annkSearch() which sets things up and
//	then call the recursive routine ann_search().  This is a recursive
//	routine which performs the processing for one node in the kd-tree.
//	There are two versions of this virtual procedure, one for splitting
//	nodes and one for leaves.  When a splitting node is visited, we
//	determine which child to visit first (the closer one), and visit
//	the other child on return.  When a leaf is visited, we compute
//	the distances to the points in the buckets, and update information
//	on the closest points.
//
//	Some trickery is used to incrementally update the distance from
//	a kd-tree rectangle to the query point.  This comes about from
//	the fact that which each successive split, only one component
//	(along the dimension that is split) of the squared distance to
//	the child rectangle is different from the squared distance to
//	the parent rectangle.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	To keep argument lists short, a number of global variables
//	are maintained which are common to all the recursive calls.
//	These are given below.
//----------------------------------------------------------------------

int						ANNkdDim;				// dimension of space
ANNpoint			ANNkdQ;					// query point
double				ANNkdMaxErr;		// max tolerable squared error
ANNpointArray	ANNkdPts;				// the points
ANNmin_k		 *ANNkdPointMK;		// set of k closest points

//----------------------------------------------------------------------
//  annkSearch - search for the k nearest neighbors
//----------------------------------------------------------------------

void ANNkd_tree::annkSearch(
    ANNpoint			q,			// the query point
    int						k,			// number of near neighbors to return
    ANNidxArray		nn_idx,	// nearest neighbor indices (returned)
    ANNdistArray	dd,			// the approximate nearest neighbor
    double				eps,		// the error bound
		int						method) // 1 == Euclidean Dist, 2== Arc Dist

{

	ANNkdDim = dim;			// copy arguments to static equivs
	ANNkdQ	 = q;
	ANNkdPts = pts;
	ANNptsVisited = 0;			// initialize count of points visited

	if (k > n_pts) 
	{			// too many near neighbors?
		annError("Requesting more near neighbors than data points", ANNabort);
	}

	ANNkdMaxErr = ANN_POW(1.0 + eps);
	FLOP(2)				// increment floating op count

	ANNkdPointMK = new ANNmin_k(k);	// create set for closest k points
	// search starting at the root
	root->ann_search(annBoxDistance(q, bnd_box_lo, bnd_box_hi, dim), method);

	for (int i = 0; i < k; i++) 
	{	// extract the k-th closest points
		dd[i] = ANNkdPointMK->ith_smallest_key(i);
		nn_idx[i] = ANNkdPointMK->ith_smallest_info(i);
	}
	delete ANNkdPointMK;		// deallocate closest point set
	ANNkdPointMK = NULL;
}

//----------------------------------------------------------------------
//  kd_split::ann_search - search a splitting node
//----------------------------------------------------------------------

void ANNkd_split::ann_search(ANNdist box_dist, int method)
{
	// check dist calc termination condition
	if (ANNmaxPtsVisited && ANNptsVisited > ANNmaxPtsVisited) return;

	// distance to cutting plane
	ANNcoord cut_diff = ANNkdQ[cut_dim] - cut_val;

	if (cut_diff < 0) 
	{			// left of cutting plane
		child[LO]->ann_search(box_dist, method);// visit closer child first

		ANNcoord box_diff = cd_bnds[LO] - ANNkdQ[cut_dim];
		if (box_diff < 0)		// within bounds - ignore
		box_diff = 0;
		// distance to further box
		box_dist = (ANNdist) ANN_SUM(box_dist,
		            ANN_DIFF(ANN_POW(box_diff), ANN_POW(cut_diff)));

		// visit further child if close enough
		if (box_dist * ANNkdMaxErr < ANNkdPointMK->max_key())
			child[HI]->ann_search(box_dist, method);

	}
	else 
	{				// right of cutting plane
		child[HI]->ann_search(box_dist, method);// visit closer child first

		ANNcoord box_diff = ANNkdQ[cut_dim] - cd_bnds[HI];
		if (box_diff < 0)		// within bounds - ignore
		box_diff = 0;
		// distance to further box
		box_dist = (ANNdist) ANN_SUM(box_dist,
		            ANN_DIFF(ANN_POW(box_diff), ANN_POW(cut_diff)));

		// visit further child if close enough
		if (box_dist * ANNkdMaxErr < ANNkdPointMK->max_key())
			child[LO]->ann_search(box_dist, method);
	}
	FLOP(10)				// increment floating ops
	SPL(1)				// one more splitting node visited
}


//----------------------------------------------------------------------
//  kd_leaf::ann_search - search points in a leaf node
//	Note: The unreadability of this code is the result of
//	some fine tuning to replace indexing by pointer operations.
//----------------------------------------------------------------------

// adding ArcDist computation

void ANNkd_leaf::ann_search(ANNdist box_dist, int method)
{
	register ANNdist dist;		// distance to data point
	register ANNcoord* pp;		// data coordinate pointer
	register ANNcoord* qq;		// query coordinate pointer
	register ANNdist min_dist;		// distance to k-th closest point
	register ANNcoord t;
	register int d;

	min_dist = ANNkdPointMK->max_key();	// k-th smallest distance so far
  int i = 0;
	switch (method)
	{
		case 1: // Euclidean Distance
			for (i = 0; i < n_pts; i++) 
			{	// check points in bucket

				pp = ANNkdPts[bkt[i]];		// first coord of next data point
				qq = ANNkdQ; 	    		// first coord of query point
				dist = 0;

				for(d = 0; d < ANNkdDim; d++) 
				{
					COORD(1)			// one more coordinate hit
					FLOP(4)			// increment floating ops

					t = *(qq++) - *(pp++);	// compute length and adv coordinate
					// exceeds dist to k-th smallest?
					if( (dist = ANN_SUM(dist, ANN_POW(t))) > min_dist) 
					{
						break;
					}
				}

				if (d >= ANNkdDim &&			// among the k best?
				(ANN_ALLOW_SELF_MATCH || dist!=0)) 
				{	// and no self-match problem
					// add it to the list
					ANNkdPointMK->insert(dist, bkt[i]);
					min_dist = ANNkdPointMK->max_key();
				}
			}
			break;
		case 2: // Arc Distance
			for (i = 0; i < n_pts; i++) 
			{	// check points in bucket

				pp = ANNkdPts[bkt[i]];		// first coord of next data point
				qq = ANNkdQ; 	    		// first coord of query point
				dist = 0;
				double x[4];

				for(d = 0; d < ANNkdDim; d++) 
				{
					COORD(1)			// one more coordinate hit
					FLOP(4)			// increment floating ops

					x[d] = *(qq++);
					x[d+2] = *(pp++);	// compute length and adv coordinate
				}

				dist = GenGeomAlgs::ComputeArcDist(x[0], x[1], x[2], x[3]);
				dist = dist * dist;

				if (dist > min_dist) 
					d = 0;

				if (ANN_ALLOW_SELF_MATCH || dist!=0) 
				{	// and no self-match problem
					// add it to the list
					ANNkdPointMK->insert(dist, bkt[i]);
					min_dist = ANNkdPointMK->max_key();
				}
			}
			
			break;
		default:
			break;
	}

	LEAF(1)				// one more leaf node visited
	PTS(n_pts)				// increment points visited
	ANNptsVisited += n_pts;		// increment number of points visited

	char buf[333];
	sprintf(buf,"Visited: %d, %f\n",n_pts,min_dist);
}

