//----------------------------------------------------------------------
//	File:		kd_split.h
//	Programmer:	Sunil Arya and David Mount
//	Last modified:	03/04/98 (Release 0.1)
//	Description:	Methods for splitting kd-trees
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

#ifndef ANN_KD_SPLIT_H
#define ANN_KD_SPLIT_H

#include "kd_tree.h"			// kd-tree definitions

//----------------------------------------------------------------------
//  External entry points
//	These are all splitting procedures for kd-trees.
//----------------------------------------------------------------------

void kd_split(				// standard (optimized) kd-splitter
    ANNpointArray	pa,		// point array (unaltered)
    ANNidxArray		pidx,		// point indices (permuted on return)
    const ANNorthRect	&bnds,		// bounding rectangle for cell
    int			n,		// number of points
    int			dim,		// dimension of space
    int			&cut_dim,	// cutting dimension (returned)
    ANNcoord		&cut_val,	// cutting value (returned)
    int			&n_lo);		// num of points on low side (returned)

void midpt_split(			// midpoint kd-splitter
    ANNpointArray	pa,		// point array (unaltered)
    ANNidxArray		pidx,		// point indices (permuted on return)
    const ANNorthRect	&bnds,		// bounding rectangle for cell
    int			n,		// number of points
    int			dim,		// dimension of space
    int			&cut_dim,	// cutting dimension (returned)
    ANNcoord		&cut_val,	// cutting value (returned)
    int			&n_lo);		// num of points on low side (returned)

void sl_midpt_split(			// sliding midpoint kd-splitter
    ANNpointArray	pa,		// point array (unaltered)
    ANNidxArray		pidx,		// point indices (permuted on return)
    const ANNorthRect	&bnds,		// bounding rectangle for cell
    int			n,		// number of points
    int			dim,		// dimension of space
    int			&cut_dim,	// cutting dimension (returned)
    ANNcoord		&cut_val,	// cutting value (returned)
    int			&n_lo);		// num of points on low side (returned)

void fair_split(			// fair-split kd-splitter
    ANNpointArray	pa,		// point array (unaltered)
    ANNidxArray		pidx,		// point indices (permuted on return)
    const ANNorthRect	&bnds,		// bounding rectangle for cell
    int			n,		// number of points
    int			dim,		// dimension of space
    int			&cut_dim,	// cutting dimension (returned)
    ANNcoord		&cut_val,	// cutting value (returned)
    int			&n_lo);		// num of points on low side (returned)

void sl_fair_split(			// sliding fair-split kd-splitter
    ANNpointArray	pa,		// point array (unaltered)
    ANNidxArray		pidx,		// point indices (permuted on return)
    const ANNorthRect	&bnds,		// bounding rectangle for cell
    int			n,		// number of points
    int			dim,		// dimension of space
    int			&cut_dim,	// cutting dimension (returned)
    ANNcoord		&cut_val,	// cutting value (returned)
    int			&n_lo);		// num of points on low side (returned)

#endif
