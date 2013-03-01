//----------------------------------------------------------------------
//	File:		kd_pr_search.h
//	Programmer:	Sunil Arya and David Mount
//	Last modified:	03/04/98 (Release 0.1)
//	Description:	Priority kd-tree search
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

#ifndef ANN_kd_pr_search_H
#define ANN_kd_pr_search_H

#include "kd_tree.h"			// kd-tree declarations
#include "kd_util.h"			// kd-tree utilities
#include "pr_queue.h"			// priority queue declarations
#include "pr_queue_k.h"			// k-element priority queue

//----------------------------------------------------------------------
//  Global variables
//	Active for the life of each call to Appx_Near_Neigh() or
//	Appx_k_Near_Neigh().
//----------------------------------------------------------------------

extern double		ANNprEps;	// the error bound
extern int		ANNprDim;	// dimension of space
extern ANNpoint		ANNprQ;		// query point
extern double		ANNprMaxErr;	// max tolerable squared error
extern ANNpointArray	ANNprPts;	// the points
extern ANNpr_queue	*ANNprBoxPQ;	// priority queue for boxes
extern ANNmin_k		*ANNprPointMK;	// set of k closest points

#endif
