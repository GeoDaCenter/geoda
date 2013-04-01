//----------------------------------------------------------------------
//      File:           pr_queue_k.h
//      Programmer:     Sunil Arya and David Mount
//      Last modified:  03/04/98 (Release 0.1)
//      Description:    Include file for priority queue with k items.
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

#ifndef PR_QUEUE_K_H
#define PR_QUEUE_K_H


//----------------------------------------------------------------------
//  Basic types
//----------------------------------------------------------------------
typedef	ANNdist		PQKkey;		// key field is distance
typedef	int		PQKinfo;	// info field is int

//----------------------------------------------------------------------
//  Constants
//----------------------------------------------------------------------

const PQKkey	PQ_NULL_KEY  =  ANN_DIST_INF;	// nonexistent key value
const PQKinfo	PQ_NULL_INFO = -1;		// nonexistent info value

//----------------------------------------------------------------------
//  ANNmin_k
//	An ANNmin_k structure is one which maintains the smallest
//	k values (of type PQKkey) and associated information (of type
//	PQKinfo).  The special info and key values PQ_NULL_INFO and
//	PQ_NULL_KEY means that thise entry is empty.
//
//	It is currently implemented using an array with k items.
//	Items are stored in increasing sorted order, and insertions
//	are made through standard insertion sort.  (This is quite
//	inefficient, but current applications call for small values
//	of k and relatively few insertions.)
//----------------------------------------------------------------------

class ANNmin_k 
{
    struct mk_node {			// node in min_k structure
    	PQKkey		key;		// key value
    	PQKinfo		info;		// info field (user defined)
    };

    int		k;			// max number of keys to store
    int		n;			// number of keys currently active
    mk_node	*mk;			// the list itself

public:
    ANNmin_k(int max)			// constructor (given max size)
	{
	    n = 0;			// initially no items
	    k = max;			// maximum number of items
	    mk = new mk_node[max+1];	// sorted array of keys
	}

    ~ANNmin_k()				// destructor
    	{ delete [] mk; }
    
  PQKkey ANNmin_key()			// return minimum key
	{ return (n > 0 ? mk[0].key : PQ_NULL_KEY); }
    
  PQKkey max_key()			// return maximum key
	{ return (n == k ? mk[k-1].key : PQ_NULL_KEY); }
    
  PQKkey ith_smallest_key(int i)	// ith smallest key (i in [0..n-1])
	{ return (i < n ? mk[i].key : PQ_NULL_KEY); }
    
  PQKinfo ith_smallest_info(int i)	// info for ith smallest (i in [0..n-1])
	{ return (i < n ? mk[i].info : PQ_NULL_INFO); }


  inline void insert(			// insert item (inlined for speed)
	PQKkey kv,			// key value
	PQKinfo inf)			// item info
	{
		register int i;
		// slide larger values up
		for (i = n; i > 0; i--) 
		{
			if (mk[i-1].key > kv)
				mk[i] = mk[i-1];
			else
				break;
		}
		mk[i].key = kv;		// store element here
		mk[i].info = inf;
		if (n < k) n++;		// increment number of items
		FLOP(k-i+1)			// increment floating ops
	}
};

#endif
