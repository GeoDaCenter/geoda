//----------------------------------------------------------------------
//	File:		kd_tree.cc
//	Programmer:	Sunil Arya and David Mount
//	Last modified:	03/04/98 (Release 0.1)
//	Description:	Basic methods for kd-trees.
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
#include "ANNx.h"
#include "kd_tree.h"			// kd-tree declarations
#include "kd_split.h"			// kd-tree splitting rules
#include "kd_util.h"			// kd-tree utilities
#include "ANNperf.h"		// performance evaluation

//----------------------------------------------------------------------
//  Global data
//
//  For some splitting rules, especially with small bucket sizes,
//  it is possible to generate a large number of empty leaf nodes.
//  To save storage we allocate a single trivial leaf node which
//  contains no points.  For messy coding reasons it is convenient
//  to have it reference a trivial point index.
//
//  KD_TRIVIAL is allocated when the first kd-tree is created.  It
//  must *never* deallocated (since it may be shared by more than
//  one tree).
//----------------------------------------------------------------------
static int  		IDX_TRIVIAL[] = {0};	// trivial point index
ANNkd_leaf		*KD_TRIVIAL = NULL;	// trivial leaf node

//----------------------------------------------------------------------
//  Printing the kd-tree 
//	These routines print a kd-tree in reverse inorder (high then
//	root then low).  (This is so that if you look at the output
//	from the right side it appear from left to right in standard
//	inorder.)  When outputting leaves we output only the point
//	indices rather than the point coordinates.  There is an option
//	to print the point coordinates separately.
//
//	The tree printing routine calls the printing routines on the
//	individual nodes of the tree, passing in the level or depth
//	in the tree.  The level in the tree is used to print indentation
//	for readability.
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//  Dumping a kd-tree
//	This routine dumps a kd-tree to a file.  The tree is printed
//	in preorder.  Eventually there should be a routine that loads
//	a tree that has been dumped.
//
//	Format:
//	#ANN <version number>
//	points <dim> <n_pts>		(point coordinates: this is optional)
//	0 <xxx> <xxx> ... <xxx>		(point indices and coordinates)
//	1 <xxx> <xxx> ... <xxx>
//	  ...
//	tree <dim> <n_pts> <bkt_size>
//	<xxx> <xxx> ... <xxx>		(lower end of bounding box)
//	<xxx> <xxx> ... <xxx>		(upper end of bounding box)
//		If the tree is null, then a single line "null" is
//		output.  Otherwise the nodes of the tree are printed
//		one per line in preorder.  Leaves and splitting nodes 
//		have the following formats:
//	Leaf node:
//		leaf <n_pts> <bkt[0]> <bkt[1]> ... <bkt[n-1]>
//	Splitting nodes:
//		split <cut_dim> <cut_val> <lo_bound> <hi_bound>
//
//	Note: Infinite lo_ and hi_bounds are printed as the special
//		values "-INF" and "+INF", respectively.
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//  kd_tree statistics (for performance evaluation)
//	This routine compute various statistics information for
//	a kd-tree.  It is used by the implementors for performance
//	evaluation of the data structure.
//----------------------------------------------------------------------

#define MAX(a,b)	((a) > (b) ? (a) : (b))
/*

//----------------------------------------------------------------------
//  kd_tree destructor
//	The destructor just frees the various elements that were
//	allocated in the construction process.
//----------------------------------------------------------------------
*/

ANNkd_tree::~ANNkd_tree()		// tree destructor
{
	if (root != NULL) delete root;
	root = NULL;
	if (pidx != NULL) delete [] pidx;
	pidx = NULL;
	if (bnd_box_lo != NULL) annDeallocPt(bnd_box_lo);
	bnd_box_lo = NULL;
	if (bnd_box_hi != NULL) annDeallocPt(bnd_box_hi);
	bnd_box_hi = NULL;
}

//----------------------------------------------------------------------
//  kd_tree constructors
//	There is a skeleton kd-tree constructor which sets up a
//	trivial empty tree.
//
//	The more interesting constructor is given a collection of
//	points and builds the entire tree.  It appears in the file
//	kd_split.cc.
//----------------------------------------------------------------------

void ANNkd_tree::SkeletonTree(		// construct skeleton tree
	int n,				// number of points
	int dd,				// dimension
	int bs)				// bucket size
{
    dim = dd;				// initialize basic elements
    n_pts = n;
    bkt_size = bs;
    root = NULL;			// no associated tree yet
    pts = NULL;				// no associated points yet

    pidx = new int[n];			// allocate space for point indices
    for (int i = 0; i < n; i++) {
	pidx[i] = i;			// initially identity
    }
    bnd_box_lo = bnd_box_hi = NULL;	// bounding box is nonexistent
    if (KD_TRIVIAL == NULL)		// no trivial leaf node yet?
	KD_TRIVIAL = new ANNkd_leaf(0, IDX_TRIVIAL);	// allocate it
}

ANNkd_tree::ANNkd_tree(			// basic constructor
	int n,				// number of points
	int dd,				// dimension
	int bs)				// bucket size
{  SkeletonTree(n, dd, bs);  }		// construct skeleton tree

//----------------------------------------------------------------------
//  rkd_tree - recursive procedure to build a kd-tree
//
//	Builds a kd-tree for points in pa as indexed through the
//	array pidx[0..n-1] (typically a subarray of the array used in
//	the top-level call).  This routine permutes the array pidx,
//	but does not alter pa[].
//
//	The construction is based on a standard algorithm for constructing
//	the kd-tree (see Friedman, Bentley, and Finkel, ``An algorithm for
//	finding best matches in logarithmic expected time,'' ACM Transactions
//	on Mathematical Software, 3(3):209-226, 1977).  The procedure
//	operates by a simple divide-and-conquer strategy, which determines
//	an appropriate orthogonal cutting plane (see below), and splits
//	the points.  When the number of points falls below the bucket size,
//	we simply store the points in a leaf node's bucket.
//
//	One of the arguments is a pointer to a splitting routine,
//	whose prototype is:
//	
//		void split(
//			ANNpointArray pa,  // complete point array
//			ANNidxArray pidx,  // point array (permuted on return)
//			ANNorthRect &bnds, // bounds of current cell
//			int n,		   // number of points
//			int dim,	   // dimension of space
//			int &cut_dim,	   // cutting dimension
//			ANNcoord &cut_val, // cutting value
//			int &n_lo)	   // no. of points on low side of cut
//
//	This procedure selects a cutting dimension and cutting value,
//	partitions pa about these values, and returns the number of
//	points on the low side of the cut.
//----------------------------------------------------------------------

ANNkd_ptr rkd_tree(		// recursive construction of kd-tree
    ANNpointArray	pa,		// point array
    ANNidxArray		pidx,		// point indices to store in subtree
    int			n,		// number of points
    int			dim,		// dimension of space
    int			bsp,		// bucket space
    ANNorthRect		&bnd_box,	// bounding box for current node
    ANNkd_splitter	splitter)	// splitting routine
{
    if (n <= bsp) {			// n small, make a leaf node
	if (n == 0)			// empty leaf node
	    return KD_TRIVIAL;		// return (canonical) empty leaf
	else				// construct the node and return
	    return new ANNkd_leaf(n, pidx); 
    }
    else {				// n large, make a splitting node
	int cd;				// cutting dimension
	ANNcoord cv;			// cutting value
	int n_lo;			// number on low side of cut
	ANNkd_node *lo, *hi;		// low and high children

					// invoke splitting procedure
	(*splitter)(pa, pidx, bnd_box, n, dim, cd, cv, n_lo);

	ANNcoord lv = bnd_box.lo[cd];	// save bounds for cutting dimension
	ANNcoord hv = bnd_box.hi[cd];

	bnd_box.hi[cd] = cv;		// modify bounds for left subtree
	lo = rkd_tree(			// build left subtree
		pa, pidx, n_lo,		// ...from pidx[0..n_lo-1]
		dim, bsp, bnd_box, splitter);
	bnd_box.hi[cd] = hv;		// restore bounds

	bnd_box.lo[cd] = cv;		// modify bounds for right subtree
	hi = rkd_tree(			// build right subtree
		pa, pidx + n_lo, n-n_lo,// ...from pidx[n_lo..n-1]
		dim, bsp, bnd_box, splitter);
	bnd_box.lo[cd] = lv;		// restore bounds

					// create the splitting node
	ANNkd_split *ptr = new ANNkd_split(cd, cv, lv, hv, lo, hi);

	return ptr;			// return pointer to this node
    }
} 

//----------------------------------------------------------------------
// kd-tree constructor
//	This is the main constructor for kd-trees given a set of points.
//	It first builds a skeleton tree, then computes the bounding box
//	of the data points, and then invokes rkd_tree() to actually
//	build the tree, passing it the appropriate splitting routine.
//----------------------------------------------------------------------

ANNkd_tree::ANNkd_tree(			// construct from point array
    ANNpointArray	pa,		// point array (with at least n pts)
    int			n,		// number of points
    int			dd,		// dimension
    int			bs,		// bucket size
    ANNsplitRule	split)		// splitting method
{
    SkeletonTree(n, dd, bs);		// set up the basic stuff
    pts = pa;				// where the points are
    if (n == 0) return;			// no points--no sweat

    ANNorthRect bnd_box(dd);		// bounding box for points
    annEnclRect(pa, pidx, n, dd, bnd_box);// construct bounding rectangle
					// copy to tree structure
    bnd_box_lo = annCopyPt(dd, bnd_box.lo);
    bnd_box_hi = annCopyPt(dd, bnd_box.hi);

    switch (split) {			// build by rule
    case ANN_KD_STD:			// standard kd-splitting rule
	root = rkd_tree(pa, pidx, n, dd, bs, bnd_box, kd_split);
	break;
    case ANN_KD_MIDPT:			// midpoint split
	root = rkd_tree(pa, pidx, n, dd, bs, bnd_box, midpt_split);
	break;
    case ANN_KD_FAIR:			// fair split
	root = rkd_tree(pa, pidx, n, dd, bs, bnd_box, fair_split);
	break;
    case ANN_KD_SUGGEST:		// best (in our opinion)
    case ANN_KD_SL_MIDPT:		// sliding midpoint split
	root = rkd_tree(pa, pidx, n, dd, bs, bnd_box, sl_midpt_split);
	break;
    case ANN_KD_SL_FAIR:		// sliding fair split
	root = rkd_tree(pa, pidx, n, dd, bs, bnd_box, sl_fair_split);
	break;
    default:
	annError("Illegal splitting method", ANNabort);
    }
}
