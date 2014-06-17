/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 Characteristic polynomial problem solving.
*/

#ifndef __GEODA_CENTER_POLYM_H__
#define __GEODA_CENTER_POLYM_H__

#include <cmath>
#include <algorithm>
using namespace std;

#define SL_SMALL_MATRIX 5
#define SL_SMALL    1e-14
#include "PowerSymLag.h"

//*** here go global variables for the characteristic polynomial algorithm

INDEX      SL_Max_Precision;        // precision -- length -- of the polynomial
INDEX      Max_Dense;            // performance: max size of dense matrix

//ofstream    output("poly.txt");     // performance: intermediate output
//static	WMatrix     Poly;                   // the container for the characteristic polynomial
WMatrix     Poly;                   // the container for the characteristic polynomial
static	bool        symmetric;       // don't assume symmetry of the original matrix

/* InitPoly
* determines the size of the polynomial to be computed, given user's requests:
* precis -- desired length of the characteristic polynomial;
* dim -- dimension of the spatial weights matrix.
* Initializes static variable Poly that will contain computed polynomial.
*/
void InitPoly(int precis, const int dim)  
{
    if (precis <= SL_SMALL_MATRIX)
        precis = SL_SMALL_MATRIX + 1;		// smaller precision is possible but who needs it?
    if (precis % 2 == 0) --precis;		// use odd numbers only, because last power stored must be even
    if (dim < precis) precis = dim + 1;		// complete characteristic polynomial has only dim+1 coefficients

    // save results in static variables
    SL_Max_Precision = precis;			// precision that the polynomials will be truncated to
    Max_Dense = 0;
    Poly.clear();
    symmetric = false;
    Poly.alloc(dim);				// initialize the result
};




/*
        PermuteMatrix
 */
//template <class R>
inline void PermuteMatrix(Iterator<WVector> mt, const INDEX i, const INDEX j)  {
  mt[i].Swap(mt[j]);                    // exchange i and j rows
  for ( ; mt; ++mt) swap((*mt)[i], (*mt)[j]);   // exchange i and j columns
  return;
}


/*
        DanilevskyPostMultiply
Multiplies matrix mt by a matrix that is different from I
by a single row and reduces last row of mt to that of Frobenius
form.
 */
void DanilevskyPostMultiply(Iterator<WVector> mt, const INDEX r1, WIterator its)  {
  WIterator             it, itr;
  VALUE         Row, Pivot= its[r1];
  for ( ; mt; ++mt)  {
    WIterator it((*mt).first(), r1);    // limit to 1st r1 elements
    Row= it[r1] / Pivot;                // pivot for the row
    if (Row != 0)  {                    // skip iteration if trivial transform
      itr= its;                         // itr is modifying row
      for ( ; it; ++it)
         (*it) -= Row * (*itr++);
      *it++ = Row;                      // (r1+1)th will be the Row pivot
      ++itr;
      for ( ; itr; ++itr)               // do the rest elements of the row
         (*it++) -= Row * (*itr);
    };
  };
  return;
}

/*
        DanilevskyPreMultiply
 */
void DanilevskyPreMultiply(Iterator<WVector> mt, const INDEX r1, WIterator its)  {
  WVector     row1(its.count(), 0);
  WIterator     it, itr;
  VALUE Row;
  for ( ; mt; ++mt)  {
    Row= *its++;
    if (Row != 0)
      for (it= (*mt)(), itr= row1(); it; ++it)
           (*itr++) += *it * Row;
  };
  for(itr= row1.first() + r1; its; ++its)
    (*itr++) += *its;
  --mt;
  row1.Swap(*mt);
  return;
}

inline void RemoveColumns(Iterator<WVector> mt, const INDEX num)  {
  for ( ; mt; ++mt)
    (*mt) -= num;
  return;
}


/*
        Danilevsky Iteration
 */
inline void DanilevskyIteration(Iterator<WVector> mt, const INDEX r1, const INDEX Remove)  {
  if (Remove) RemoveColumns(mt, Remove);
  WIterator     itsave(mt[r1+1]());
  mt.deflate();
  DanilevskyPostMultiply(mt, r1, itsave);
  DanilevskyPreMultiply(mt, r1, itsave);
  return;
}


/*
        FindAbsLargest
 */
template <class R>
inline INDEX FindAbsLargest(Iterator<R> vt, INDEX ro)  {
  R     init= *vt;
  INDEX initx(0), ix(0);
  while(++ix <= ro)  {
    ++vt;
    if (fabs(*vt) > init)  { init= fabs(*vt);  initx= ix;  };
  };
  return initx;
}

/*
        PolyMultiply
c is a product of two polynomials, determined by a and b.
a is a polynomial (including leading 1);
The second polynomial is {1, -b}.
 */
void PolyMultiply(WVector &a, WIterator b, WVector &c)  {
  c.alloc(a.count() + b.count());
  Copy(c, a());
  for (INDEX cp= b.count(); cp; --cp) c << 0;
  WIterator   ita= a(), itr= c(), itr2, itb;
  ++itr;
  for ( ; ita; ++ita, ++itr)  {
    VALUE       vita= *ita;
    if (vita != 0)
        for (itb= b, itr2= itr; itb; ++itb)
            *itr2++ -= vita * (*itb);
  };
  return;
}

/*
        Danilevsky Method
 */
void DanilevskyMethod(Iterator<WVector> mt, WVector &poly)  {
  const VALUE   Dsmall= 0.001;
  INDEX dim= mt.count(), col, ro= dim, C2Remove= 0;
  WVector     tpoly(1, 1), tp;
//  PrintM(mt);
  while (--ro)  {
    if (fabs(mt[ro][ro-1]) > Dsmall)  { // regular iteration
            DanilevskyIteration(mt, ro-1, C2Remove);
            C2Remove= 0;
            }
       else  {                          // permute & iterate
            col= FindAbsLargest(mt[ro](), ro-1);
            if (mt[ro][col] != 0)  {
                PermuteMatrix(mt, col, ro-1);
                DanilevskyIteration(mt, ro-1, C2Remove);
                C2Remove= 0;
              }
              else  {
                tp= tpoly;
                PolyMultiply(tp, WIterator(&mt[ro][ro], dim-ro), tpoly);
                C2Remove += dim-ro;
                dim= ro;
                };
        };
   mt.deflate();
  };
  PolyMultiply(tpoly, WIterator(mt[0].first(), dim), poly);
  return;
}

/*
        LevelStructure
Assumes order has dim(mt)+1 in all its elements.
On the exit: perm -- contains the ordering,
order -- contains contiguity order (0,1,2,...) for each block
sequence is determined by perm;
start -- the pseudoperipheral;
Contig -- length of the levelstructure (if single block);
returns the number of blocks.
 */
//template <class M>
INDEX LevelStructure(Iterator<WMap> mt, INDEX &start, INDEX &Contig,
        const Iterator<INDEX> perm, INDEX * order)  
{
  typedef WMap::input_iterator iROW;
  INDEX        Dim1= mt.count()+1, conty(0), pieces(0), init(start);
  INDEX        *first, *last, *current, *next;

	next = perm();
  do  {
    order[init]= 0;
    ++pieces;
    last= next++;
    *last= init;
    do  {
       first= last;
       last= next;
       ++conty;
       for (current= first; current < last; ++current)
         for (iROW r= mt[*current](); r; ++r)  {
           INDEX tid= (*r).first;
           if (order[tid] == Dim1)
             order[(*next++)= tid]= conty;
         };
    }
    while (last != next);     // no new elements added to permutation table
    if (next <= perm.last())
        while(order[init] < Dim1) ++init;
  }
  while (next <= perm.last());
  order[init]= 0;               // mark the last element
  if (pieces == 1)  {           // find element with min degree
    init= Dim1;
//    cout << "ordering " << setw(4) << mt.count() << "  elts,  conty: " << setw(4) << conty << "  has " << last-first << endl;
    for (current= first; current < last; ++current)  {
      INDEX cnt= mt[*current].count();
      if (cnt < init)  { init= cnt;  next= current;  };
    };
  };
  Contig= conty;
  start= next - perm();
  return pieces;
}

/*
        PolyProduct

 */
template <class P>
void PolyProduct(Vector<P> &v1, Iterator<P> v2)  {
  Iterator<P>           it2;
  ShortIterator<P>      it1(v1.last()), First, work;
  P     Current;
  bool          Restricted= 0;
  size_t        Add= v2.count() - 1, AddRestricted= v1.size() - v1.count();
  if (Add > AddRestricted)  {
      Restricted= true;
      Add= AddRestricted;
  };
  for (size_t AddIn= 0; AddIn < Add; ++AddIn)   // adds Add zeros to the vector
     v1 << 0;

  if (Restricted)  {
    ++Add;
    First= ShortIterator<P> (it1() - v2.count() + Add);
    v2= Iterator<P> (v2.first(), Add);
    for ( ; First < it1; --it1)  {
     if ((Current = *it1) != 0)
      for ( *it1=0, it2= v2, work= it1; it2; ++it2)
        (*work++) += *it2 * Current;
      v2.inflate();
    };
  };

  First= ShortIterator<P> (v1.first());
  for ( ; !(it1 < First); --it1)
     for (Current= *it1, *it1= 0, it2= v2, work= it1; it2; ++it2)
         (*work++) += *it2 * Current;

  return;
}


/*
        SparseSubMatrix
 */
//template <class R>
void SparseSubMatrix(Iterator<WMap> mt, Iterator<INDEX> perm, INDEX * flag, Vector<WMap> &v)  
{

  INDEX Length= 0, *IxPerm = perm(), FlagId= flag[*perm];
  Vector<INDEX> InversePermute(mt.count());
  while (perm && flag[*perm] == FlagId)  {
    InversePermute[*perm++] = Length;
    ++Length;
  };
  v.alloc(Length);

  for ( ; v; ++v)  
	{
    WMap::input_iterator   OneRow= mt[*IxPerm++]();
    (*v).alloc(OneRow.count());
    for ( ; OneRow; ++OneRow)
      if (flag[(*OneRow).first] == FlagId)
          *v << WMap::data(InversePermute[(*OneRow).first], (*OneRow).second);
  };
  return;
}

template <class IR>
void SavePerm(Iterator<IR> it, INDEX * perm)  {
    for ( ; it; ++it)
        (*it).first = perm[ (*it).first ];
}

/*
        Rename
 */
template <class R>
void Rename(Iterator<R> mt, INDEX * Perm)  {
  for (Iterator<R> it= mt; it; ++it)  {
    SavePerm((*it)(), Perm);
    HeapSort((*it)());
  };
  INDEX         Dim= mt.count(), IxOld, IxNew;
  for (IxOld= 0; IxOld < Dim; ++IxOld)  {
    IxNew= Perm[IxOld];
    while (IxOld != IxNew)  {
      mt[IxNew].Swap(mt[IxOld]);
      swap(IxNew, Perm[IxNew]);
    };
  };
    return;
}

/*
        SelectMatrix
 */
inline void SelectMatrix(Iterator<WMap> mt, const INDEX start, const INDEX stop,
                Vector<WMap> &v)  {
  Iterator<WMap>   it(mt.first()+start, stop-start);
  v.alloc(stop-start);
  for ( ; it; ++it, ++v)  {
      Iterator<WPair> itr= (*it)();
      while (itr && (*itr).first < start) ++itr;
      (*v).alloc(itr.count());
      while (itr && (*itr).first < stop)  {
          *v << WPair((*itr).first - start, (*itr).second);
          ++itr;
      };
  };
  return;
}

/*
        RemoveTheBelt
 */
inline void RemoveTheBelt(Iterator<WMap> it, INDEX InTheBelt)  {
  INDEX Limit= 0;
  for ( ; it; ++it)  {           // truncate the belt from the weights matrix
     if (InTheBelt < Limit) ++InTheBelt;
     ++Limit;
     while (!(*it).empty())  {
       --(*it);
       if ((*(*it)).first < InTheBelt) {  ++(*it);  break;  };
     };
  };
  return;
}

/*
        SparsePolyBordering
 */
void SparsePolyBordering(Iterator<WMap> mt, const INDEX BeltSize, WMatrix &Poly)  {
  INDEX         Dim= mt.count(), InTheBelt= Dim - BeltSize;
  RemoveTheBelt(mt, InTheBelt);
  Iterator<WMap> it(mt.first(), InTheBelt);
  for ( ; InTheBelt < Dim; ++InTheBelt)  {
     (*Poly).alloc(sl_min(InTheBelt+2, SL_Max_Precision));
     VALUE  Diag= 0;
     if (mt[InTheBelt])
       if ((*(mt[InTheBelt])).first == InTheBelt) Diag= (*(mt[InTheBelt])).second;
     if (symmetric)  {
        PowerSymLag    SparseLag(it, InTheBelt);
        VALUE       Lag= SparseLag.Init();
        *Poly << 1 << -Diag << -Lag;
        while (*Poly)
          *Poly << -SparseLag.ComputeLag();
     }  else  {     // matrix is not symmetric, yet has symmetric structure

        PowerLag        SparseLag(it, InTheBelt);
        VALUE      Lag= SparseLag.Init();
        *Poly << 1 << -Diag << -Lag;
        while (*Poly)  {
         Lag= SparseLag.ComputeLag();
         *Poly << -Lag;
        };
     };
     ++Poly;
     it.inflate();
     Iterator<WPair> itr = (*it.last())();
     for ( ; itr; ++itr)  ++it[(*itr).first];
     if (Diag) ++mt[InTheBelt];
  };
  return;
}

/*
        SparsePolyBordering
 */
void SparsePolyBorderingNE(Iterator<WMap> mt, const INDEX BeltSize, WMatrix &Poly)  {
  const		INDEX         Dim= mt.count();
  INDEX		InTheBelt;
  Iterator<WMap>	it = mt;
  for (InTheBelt= Dim-1; InTheBelt >= Dim-BeltSize; --InTheBelt)  {
     it.deflate();
     Iterator<WPair> itr = it[InTheBelt]();	// iterate over the just bordered row
     for ( ; itr; ++itr)  {
        --it[(*itr).first];	// remove these elements from the matrix
      };
//     if (Diag) --it[InTheBelt];	-- don't need , since the whole row goes away
//     it.deflate();						// remove the row from the matrix

     (*Poly).alloc(sl_min(InTheBelt+2, SL_Max_Precision));
     VALUE  Diag= 0;
     if (mt[InTheBelt])
       if ((*(mt[InTheBelt])).first == InTheBelt) Diag= (*(mt[InTheBelt])).second;
     if (symmetric)  {
        PowerSymLag    SparseLag(it, InTheBelt);
        VALUE       Lag= SparseLag.Init();
        *Poly << 1 << -Diag << -Lag;
        while (*Poly)
          *Poly << -SparseLag.ComputeLag();
     }  else  {     // matrix is not symmetric, yet has symmetric structure

        PowerLag        SparseLag(it, InTheBelt);
        VALUE      Lag= SparseLag.Init();
        *Poly << 1 << -Diag << -Lag;
        while (*Poly)  {
         Lag= SparseLag.ComputeLag();
         *Poly << -Lag;
        };
     };
     ++Poly;
  };
  int ct= Poly.count() - 1;
  int cr = 0;
  while (cr < ct)  {
    Poly[cr].Swap(Poly[ct]);
    --ct;
    ++cr;
  };
  return;
}

const INDEX	kDenseMax = 1;

// static WMatrix	BufferDense;
WMatrix	BufferDense;

/*
        DensePolySolving
 */
void DensePolySolving(Iterator<WMap> mt)  
{

  INDEX dim= mt.count();
  Iterator<WPair>      itr;
  if (Max_Dense < dim) Max_Dense= dim;
  if (dim <= 1)  
	{
       VALUE val= 0;
       (*Poly).alloc(2);
       if (!(*mt).empty())
         val= (*(*mt)).second;
       *Poly << 1 << -val;
  }  
	else  if (dim <= kDenseMax)  
	{
  		Iterator<WVector>	tmp;
        BufferDense.reset();
        for ( ; mt; ++mt, ++BufferDense)  {
          (*BufferDense).fill(dim, 0);
          for (itr= (*mt)(); itr; ++itr)
            (*BufferDense)[(*itr).first] = (*itr).second;
        };
        tmp= BufferDense();
        DanilevskyMethod(tmp, *Poly);
  }  else  {
       WMatrix     Dense(dim);
       for ( ; mt; ++mt, ++Dense)  {
         (*Dense).alloc(dim, 0);
         for (itr= (*mt)(); itr ; ++itr)
           (*Dense)[(*itr).first]= (*itr).second;
       };
       DanilevskyMethod(Dense(), *Poly);
  };
  ++Poly;
  return;
};


/*
        DensePolySolving -- old
 */
void DensePolySolvingOld(Iterator<WMap> mt)  {
  INDEX dim= mt.count();
  if (Max_Dense < dim) Max_Dense= dim;
  if (dim > 1)  {
       WMatrix     Dense(dim);
       Iterator<WPair>      itr;
       for ( ; mt; ++mt, ++Dense)  {
         (*Dense).alloc(dim, 0);
         for (itr= (*mt)(); itr ; ++itr)
           (*Dense)[(*itr).first]= (*itr).second;
       };
       DanilevskyMethod(Dense(), *Poly);
   }  else  {
       VALUE val= 0;
       (*Poly).alloc(2);
       if (!(*mt).empty())
         val= (*(*mt)).second;
       *Poly << 1 << -val;
   };
  ++Poly;
  return;
};



//**********************************************************
//        PolyOrganize
//**********************************************************
void PolyOrganize(const WVector * First, const INDEX Precision, Iterator<WVector> Border)  {
  if (Precision < SL_Max_Precision)  {
    INDEX Cnt= Poly.last() + 1 - First;
    if (Cnt > 1)  {
      WVector  acc(Precision);
      acc << 1;
      while (Cnt-- > 0)  {
        --Poly;
        PolyProduct(acc, (*Poly)());
        (*Poly).clear();

      };
      (*Poly).Swap(acc);
    };

    while (Border && (*Border).count() < SL_Max_Precision)  {
//      cout << " org: " << (*Border).count() << " + " << (*Poly).count() << endl;
      PolyProduct(*Border, (*Poly)());
      (*Poly).destroy();
      (*Poly).Swap(*Border);
      ++Border;
    };
    ++Poly;
  };
  for ( ; Border; ++Border, ++Poly)  {
//    cout << " org, copy: " << (*Border).count() << endl;
    *Poly = *Border;
  };
  return;
};

INDEX SplitSparse(Iterator<WMap> mt, INDEX Pseudo,  INDEX & Second, INDEX & PseudoCount)  {
  const INDEX     dim = mt.count();
  INDEX    n1, n2= 1, StartBest= 0, localLevel, oldPseudo;
  Vector<INDEX> perm(dim, 0), order(dim);
                                                // find pseudoperipheral
//  cout << "ps: " << Pseudo << "  level: " << Level;
  do  {
     for (n1= 0; n1 < dim; ++n1)
        order[n1] = dim+1;
      oldPseudo= Pseudo;
     LevelStructure(mt, Pseudo, localLevel, perm(), order.first());
  }
  while (oldPseudo == 0);
//   cout << "  psF: " << Pseudo << "  levelF: " << Level << endl;

  VALUE             OpBest= dim * geoda_sqr((double) dim), OpLevel;
  for (INDEX lev= 1; lev < localLevel-1; ++lev)  {
     n1= n2;
     while (order[perm[n2]] == lev) ++n2;
     OpLevel= (n2-n1) * dim;
     if (dim < 300)  OpLevel *= 2;	// adjust for a 'big' matrix
     OpLevel += geoda_sqr((double) n1) + geoda_sqr((double) dim-n2);
     if (OpLevel < OpBest)  {  OpBest= OpLevel;  StartBest= n1;  };
  };
  if (StartBest == 0)  {
      DensePolySolving(mt);
      return 0;
  };

  localLevel= order[perm[StartBest]];              // the level of the belt

  //   find the new Pseudo for the 1st half
  n2= StartBest;
  INDEX	count= 0;
  if (localLevel == 1 ) count= 1;
    else
	  while (order[perm[--n2]] == localLevel-1)
	      ++count;

   //  find the start of the second half
  n2= StartBest;
  while (order[perm[n2]] == localLevel) ++n2;

  Vector<INDEX> InvPerm(dim);
  for (n1= 0; n1 < dim; ++n1)
    InvPerm[perm[n1]] = n1;
      									// now n2 indicates beginning of the 2nd submatrix
  for (n1= StartBest; n1 < n2; ++n1)  {		// for each elt on the belt
    Iterator<WPair>	itr= mt[ perm[n1] ]();
    bool	NoNbrsInSecond = true;
    for ( ; itr; ++itr)
      if (InvPerm[(*itr).first] >= n2)  {
         NoNbrsInSecond= false;
         break;
      };
    if (NoNbrsInSecond)  {		// this elt on the belt does not have nbrs in the second submatrix ->
    							// move it from the belt to the first submatrix
      const CNT	tmp= perm[ StartBest ];
      perm[ StartBest ]= perm[ n1 ];
      perm[ n1 ]= tmp;
      ++StartBest;
      ++count;
    };
  };

  for (n1= 0; n1 < StartBest; ++n1) InvPerm[perm[n1]] = n1;     // 1st half
  n2= dim-1;
  for ( ; order[perm[n1]] == localLevel; ++n1, --n2)       // place the belt at the end
    InvPerm[perm[n1]]= n2;
  n2= n1 - StartBest;                   // keep width of the belt
  for ( ; n1 < dim; ++n1)               // 2nd half between them
    InvPerm[perm[n1]]= n1 - n2;

  Rename(mt, InvPerm.first());
  Second= n2;

  PseudoCount= count;
  return StartBest;
}

INDEX SplitSparseOld(Iterator<WMap> mt, INDEX Pseudo, INDEX & Level, INDEX & Second)  {
  INDEX     dim = mt.count(), OldLevel;
  Vector<INDEX> perm(dim, 0), order(dim);
                                                // find pseudoperipheral
  do  {
     Vector<INDEX>      InitOrder(dim, dim+1);
     OldLevel= Level;
     LevelStructure(mt, Pseudo, Level, perm(), InitOrder.first());
     order.Swap(InitOrder);
  }
  while (Level > OldLevel);
  OldLevel= Level;

  INDEX    n1, n2= 1, StartBest= 0;
  VALUE             OpBest= dim * geoda_sqr((double) dim), OpLevel;
  for (INDEX lev= 1; lev < OldLevel-1; ++lev)  {
     n1= n2;
     while (order[perm[n2]] == lev) ++n2;
     OpLevel= (n2-n1) * dim;
     if (dim < 300)  OpLevel *= 2;
     OpLevel += geoda_sqr((double) n1) + geoda_sqr((double) dim-n2);
     if (OpLevel < OpBest)  {  OpBest= OpLevel;  StartBest= n1;  };
  };
  if (StartBest == 0)  {
      DensePolySolving(mt);
      return 0;
  };
  Level= order[perm[StartBest]];              // the level of the belt
  Vector<INDEX> InvPerm(dim);
  for (n1= 0; n1 < StartBest; ++n1) InvPerm[perm[n1]] = n1;     // 1st half
  n2= dim-1;
  for ( ; order[perm[n1]] == Level; ++n1, --n2)       // place the belt at the end
    InvPerm[perm[n1]]= n2;
  n2= n1 - StartBest;                   // keep width of the belt
  for ( ; n1 < dim; ++n1)               // 2nd half between them
    InvPerm[perm[n1]]= n1 - n2;
  Rename(mt, InvPerm.first());
  Second= n2;
  return StartBest;
}

void SparsePoly(Iterator<WMap> mt);
void SparseBlockPoly(Iterator<WMap> mt, INDEX Pseudo);

void SelectBlockPolyNE(Iterator<WMap> mt, const INDEX StartBest, const INDEX PseudoCount)  {
//*** this routine alters the sparse matrix -> does not require extra memory
  Iterator<WMap>	it( mt.first(), StartBest );	// select the submatrix to work with

  INDEX	count= StartBest, Pseudo= 0;
  
  for (INDEX cnt= 1; cnt <= PseudoCount; ++cnt)  {
    const INDEX		thisCount= mt[StartBest-cnt].count();
    if (thisCount <= count)  {
      count= thisCount;
      Pseudo= StartBest-cnt;
    };
  };

  SparseBlockPoly(it, Pseudo);
  return;
}

// procedure to rename the references in the sparse matrix by deducting Deduct
// (thus taking offset out from the submatrix)
void PartialRename(Iterator<WMap> it, const INDEX Deduct)  {
  for ( ; it; ++it)
    for (Iterator<WPair> itr= (*it)(); itr; ++itr)
      (*itr).first -= Deduct;
  return;
}

inline void SelectPolyNE(Iterator<WMap> mt, const INDEX StartBest, const INDEX Stop)  {
//*** this routine works on the original sparse matrix
  Iterator<WMap>	it( mt.first()+StartBest, Stop-StartBest);
  PartialRename(it, StartBest);
  SparsePoly(it);
  return;
}


/*
        SparseBlockPoly
mt - sparse matrix of connected elements;
 */
void SparseBlockPoly(Iterator<WMap> mt, INDEX Pseudo)  
{
  const INDEX 	dim= mt.count();
  if (dim <= SL_SMALL_MATRIX)  {
       DensePolySolving(mt);
       return;
  };
  INDEX     StartBest, n2, PseudoCount= 0;
  StartBest= SplitSparse(mt, Pseudo, n2, PseudoCount);
  if (StartBest == 0) return;
  WVector * First= Poly.last()+1; // first poly introduced in this routine

  Vector<WVector>  Bordering(n2);

  SparsePolyBorderingNE(mt, n2, Bordering);

  SelectBlockPolyNE(mt, StartBest, PseudoCount);

  SelectPolyNE(mt, StartBest, dim-n2);

  PolyOrganize(First, dim-n2+1, Bordering());
  return;
}

/*
        SparsePoly
mt - sparse matrix;
Precision - number of charactersitic coeffs required;
Poly -  container with blocks of characteristic
polynomial (should be previously allocated);
 */
void SparsePoly(Iterator<WMap> mt)  
{
	INDEX         dim= mt.count(), Level, Blocks, Pseudo(0);
	Vector<INDEX> perm(dim, 0), order(dim+1, dim+1);
	Blocks= LevelStructure(mt, Pseudo, Level, perm(), order.first());
	if (Blocks > 1) {
		Iterator<INDEX>     iperm= perm();
		INDEX               Block= 0;
		for ( ; iperm; ++iperm)  
		{
			if (order[*iperm] == 0) ++Block;
			order[*iperm] = Block;
		}                  // order[x] shows to which block x belongs to
		for (Block= 0, iperm= perm(); Block < Blocks; ++Block) {
			while (order[*iperm] <= Block) ++iperm;
			GWT         SubMatrix;
			SparseSubMatrix(mt, iperm, order.first(), SubMatrix);
			SparseBlockPoly(SubMatrix(), 0);
		}
	} else {
		perm.destroy();
		order.destroy();
		SparseBlockPoly(mt, Pseudo);
	}
}

// computes log of the polynomial stored in Poly:
// as a sum of logs of each polynomial
VALUE OnePoly(Iterator<WVector> Poly, const VALUE Val, const INDEX Prec, INDEX Pr)  {
  Iterator<WVector>   it;
  Vector<VALUE> Power(Prec);
  VALUE         S= 1, LogJ= 0;
  bool		first = true;
  while(Power)  {
      Power << S;
      S *= Val;
  };
  for ( ; Pr < Prec; ++Pr)
    Power[Pr]= 0;
  for (it= Poly; it; ++it)  {
     WIterator 	itr= (*it)();
     Iterator<VALUE>       Po= Power();
     S= 0;
     for ( ; itr; ++itr, ++Po)
       S += *itr * *Po;
     if (S < 1.0e-16)  { 
         if (first)  {
//            cout << " S is small= " << S << "  val: " << Val << endl;
            first= false;
         };
         S= 1.0e-16;
     };
     LogJ += log(S);
  };
  return LogJ;
}

VALUE OnePrime(Iterator<WVector> Poly, const VALUE Val, const INDEX Prec,  INDEX Pr)  {
  Iterator<WVector>         it;
  Vector<VALUE> Power(Prec);
  VALUE         S= 1, Sum= 0;
  long          cnt;
  while (Power)  {
    Power << S;
    S *= Val;
  };
  for ( ; Pr < Prec; ++Pr)
    Power[Pr]= 0;
  for (it= Poly; it; ++it)  {
    WIterator 		itr= (*it)();
    Iterator<VALUE>     Po= Power();
    VALUE       S1= 0;
    S= 1;
    cnt= 1;
    ++itr;
    for (; itr; ++itr, ++cnt)  {
        S1 += *itr * cnt * *Po;
        ++Po;
        S += *itr * *Po;
    };
    Sum += S1/S;
  };
  return Sum;
}


#endif

