/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
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
        Lite - experimental design of templates and classes for
        the object oriented designed of the SpaceLite.
        1. containers;
        2. iterators for corresponding containers;
        3. container template functions;
 */

#ifndef __GEODA_CENTER_LITE2_H__
#define __GEODA_CENTER_LITE2_H__

#include <vector>
#include <ostream>

class SL_InputIterator {};
class SL_Iterator {};
class SL_OutputIterator {};

/*
        Definitions for the types used in defining spatial
        weights matrix.
 */
#define		CNT     int
#define		VALUE   double
#define		INDEX   size_t
#define     sl_max(x,y)     ( (x) < (y) ? (y) : (x) )
#define     sl_min(x,y)     ( (x) < (y) ? (x) : (y) )
#ifndef GDA_SWAP
#define GDA_SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
#endif

enum WeightsType  
{
    W_UNDEF,
    W_MAT,
    W_GWT,
    W_GAL
};

inline double ComputeMean(double *dt, int dim)
{
	double sdt = 0.0;
	for (int i=0; i<dim; i++) sdt += dt[i];
	sdt = sdt/((double) dim);
	return sdt;
}

inline double ComputeSdev(double *dt, int dim)
{
	double mean = ComputeMean(dt, dim);
	double sum= 0;
	double diff=0;
	for (int cnt=0; cnt<dim; ++cnt) {
		diff = dt[cnt]-mean;
		sum += (diff*diff);
	}
	return sqrt(sum / ((double) dim));
}


/*
        SL_function
  Template structure to provide skeleton for a SL function.
  The distinction between any SL function (like Sqr) and
  ordinary template function (like sqr) is that the first one
  can be seamlessly passed as a parameter to another templated
  function (see for_each), where one of the parameters is an
  SL function. This trick is necessarily since ordinary template
  function can not be passed as an argument. This works with SL
  function because this one is a structure. The function call
  is actually performed through overloaded operator ().
 */
template <class Arg>
struct SL_function {
        Arg arg;
        SL_function() {};
        SL_function(const Arg &param) : arg(param)  {};
};

/*
        Sqr
  Template SL function. Operator () overloaded to return square
  of an expression.
 */
template <class T>
struct Sqr : SL_function<T>  {
        T operator()(const T &x)  {  return x * x;  };
};

/*
        divide
  Template function that divides first argument on another.
 */
template <class T>
inline void divide(T &x, const T &y)  {
        x /= y;
         return;
}

/*
        Divide
  Template SL function. Operator () called with one parameter
  overloaded to divide that argument by value previously
  save at arg.
 */
//template <class T>
//struct Divide : SL_function<T>  {
//        Divide(const T &param) : SL_function<T>(param)  {};
//        void operator()(T &x)  {  divide(x, arg);  return;  };
//};

/*
        for_each
   Template function that calls function f for each element
   of the container.  The container can be of any type for
   which operations * and ++ are defined.
 */
template <class D, class F>
inline void for_each(D itr, F f)  {
  while (itr)
    f(*itr++);
  return;
}

/*
        operator >
  Generic operator introduced to avoid redundance in the
  class declarations through redefining using operator <.
 */

template <class T>
inline bool operator > (const T &x, const T &y)  
{
  return y < x;
}


/*
        ShortIterator
  Template class to describe basic iterator for which
  following are defined explicitly: dereference(*),
  forward iteration (++), backward iteration (--),
  operator < (less).
 */
template <class T>
class ShortIterator  {
  public :
    typedef T data;
    typedef T * pointer;
    typedef ShortIterator<T> self;
    ShortIterator(T * init= NULL) : ptr(init) {};
    ~ShortIterator()            {  ptr= NULL;  return;  };
    T & operator * ()           {  return *ptr;  };
    T & operator * () const     {  return *ptr;  };
    T * operator () () const    {  return ptr;  };
    T & operator [] (const size_t loc) const  {  return ptr[loc];  };
    T & operator [] (const size_t loc)  {  return ptr[loc];  };
    bool operator < (const ShortIterator &it)  {  return ptr < it.ptr;  };
    ShortIterator & operator ++ ()  {  ++ptr;  return *this;  };
    ShortIterator operator ++ (int)
      {  self  tmp= *this;  ++ptr;  return tmp;  };
    ShortIterator & operator -- ()  { --ptr;  return *this;  };
    ShortIterator operator -- (int)
      {  self  tmp= *this;  --ptr;  return tmp;  };
//    pointer operator -> ()  {  return ptr;  };
//    friend ShortIterator& operator -= (self & a, const size_t decrement);
    ShortIterator& operator -= (const size_t decrement)  { ptr -= decrement; return *this;  };
    
    ShortIterator& operator += (size_t increment)  {
        ptr += increment;
        return this;
    }
    
  protected :
    T * ptr;
};

/*
        ShortIterator  -- operator -= and +=
Operator += advances forward the iterator by an arbitrary
number of elements. Operator -= advances the iteratyor backward.
 */
//template <class T>
//inline ShortIterator<T>& operator += (ShortIterator<T> &a, size_t increment)  {
//  a.ptr += increment;
//  return a;
//}



/*
        Iterator
  Template class for a 'full' iterator, which in addition to
  ShortIterator can be checked on validity (operator void *)
  and has defined operator >>.
 */
template <class T>
class Iterator : public ShortIterator<T>  {
  public :
    typedef SL_Iterator iterator_type;
    typedef ShortIterator<T> parent;
    Iterator(T * init= NULL, T * last= NULL) : ShortIterator<T> (init), end(last) {};
    Iterator(T * init, const size_t sz) : ShortIterator<T> (init)
                {  end= &init[sz];  return;  };
    operator void * ()  const         {  return (void *) (ShortIterator<T>::ptr < end);  };
    Iterator & operator >> (T & val)  {  val= *(ShortIterator<T>::ptr)++;  return *this;  };
    size_t      count() const   {  return end - ShortIterator<T>::ptr;  };
    void       deflate()        {  --end;  return;  };
    T *         first() const   {  return ShortIterator<T>::ptr;  };
    void       inflate()        {  ++end;  return;  };
    T *         last() const    {  return end-1;  };
  protected :
    T *   end;
};


/*
       pairstruct 
   Template structure constructing an object consisting of
   two classes.
   Function template provided to alleviate its creation.
   pair can be compared to another pair - operator <;
 */
template <class First, class Second>
struct pairstruct  {
  First         first;
  Second        second;
  pairstruct(const First init1, const Second init2) : first(init1), second(init2)  {};
  pairstruct() : first(0), second(0)  {};
  bool operator < ( const pairstruct<First, Second> &x)
     {  return first < x.first; };
//  friend INPUTstream & operator >> (INPUTstream &s, pairstruct<First, Second> & v);
};

template <class F, class S>
std::ostream & operator << (std::ostream &s, pairstruct<F, S> &v)  {
  return s << "f: " << v.first << " s: " << v.second;
}


template <class T>
inline Iterator<T> & operator -= (Iterator<T> &it1, Iterator<T> it2)  {
    Iterator<T>     tp= it1;
    for ( ; tp; ++tp, ++it2)
      *tp -= *it2;
    return it1;
}

/*
        Vector
  Template class to define container in the form of vector.
  Is explicitly derived from ShortIterator. In addition,
  supports memory allocation and deallocation, validity
  check (operator void *), add-to-the-container function
  (operator <<), current size (function count()),
  adaptor to a corresponding input iterator.
  Elements can not be taken from the container other way but
  destroying it, rather they can be copied (all or part) using
  input iterator adapter. Elements can be added to the
  container at any time and only at the end by applying
  operator <<.
 */
template <class T>
class Vector : public ShortIterator<T>  
{
  public :
    typedef Vector<T> self;
    typedef SL_Iterator iterator_type;
    typedef Iterator<T> input_iterator;

    Vector(const size_t initsize= 0)    {  alloc(initsize);  return;  };
    Vector(const size_t initsize, const T initvalue)
        {  alloc(initsize, initvalue);  return;  };
    Vector(const Vector<T> &from)       {  copy(from());  return;  };
    Vector(T *array, const int size)  {
        begin = array;
        sz = size;
        ShortIterator<T>::ptr = end = array + sz;
    };
    ~Vector()   {  if (begin) delete [] begin;  begin= NULL;  return;  };

    operator void * () const  {  return (void *) (ShortIterator<T>::ptr < end);  };
    Vector & operator << (const T &x)  
		{
        if (!(ShortIterator<T>::ptr < end)) realloc((sz+1) << 1);
			if (ShortIterator<T>::ptr) {
        *(ShortIterator<T>::ptr)++ = x;
			}
        return *this;
    };
    input_iterator operator ()() const {  return  input_iterator (begin, ShortIterator<T>::ptr);  };
    input_iterator operator ()(const size_t dim) const  {
      const size_t rdim= sl_min(dim, count());
      return input_iterator (begin, rdim);
    };
    T & operator [] (const size_t loc)  {  return begin[loc];  };
    T & operator [] (const size_t loc)  const  {  return begin[loc];  };
    self & operator = (self &from);
    self & operator = (const self &from);  // MMM: something is fishy with this overloaded operator
	                                      //but don't touch these!  Commenting out one or the other
									     // produces strange errors (either seg fault, or wrong values).
    self & operator = (self * from);

    void        alloc(const size_t initsize= 0);
    void        alloc(const size_t initsize, const T initvalue);
    void        clear()  {
        if (begin) delete [] begin;
        begin= ShortIterator<T>::ptr= NULL;
        sz= 0;
        end= NULL;
        return;
    };
    size_t      copy(Iterator<T> from);
    size_t      count() const   {  return ShortIterator<T>::ptr - begin;  };
    void        destroy()  {  if (begin) delete [] begin;  begin= NULL;   return;  };
    bool        empty() const   {  return ShortIterator<T>::ptr == begin;  };
    T *         first() const   {  return begin;  };
    T *         last() const    {  return ShortIterator<T>::ptr-1;  };
    void        reset()         {  ShortIterator<T>::ptr= begin;  return;  };
    void	reset(int s)  {
            if (s < 0) s = 0;
            if (s > (int) sz) s = sz;
            ShortIterator<T>::ptr = &begin[s];
    };
    void	fill(const size_t initsize, const T initvalue);	
    size_t      size() const    {  return sz;  };
    void        Swap(self &with);
  protected :
    size_t      sz;
    T *         begin;
    T *         end;
  private   :
    void realloc(const size_t newsize);
};

//***
//***
template <class T>
void Vector<T>::fill(const size_t initsize, const T initvalue)  {
  ShortIterator<T>::ptr= begin;
  for (size_t cnt= initsize; cnt; --cnt)
    *(ShortIterator<T>::ptr)++ = initvalue;
  return;
}
    
/*
 */
template <class T>
inline Vector<T> & Vector<T>::operator = (Vector<T> &from)  {
  if (this != &from)  {
    memcpy(this, &from, sizeof(from));
    from.begin= NULL;
  };
  return *this;
}

/*
 */
template <class T>
inline Vector<T> & Vector<T>::operator = (const Vector<T> &from)  {
  if (this != &from)  {
    if (begin) delete [] begin;
    copy(from());
  };
  return *this;
}

/*
 */
template <class T>
inline Vector<T> & Vector<T>::operator = (Vector<T> * from)  {
  if (this != from)  {
    if (begin) delete [] begin;			// release whatever we might have had before
    memcpy(this, from, sizeof(*from));		// move all relevant data : (1) copy and
    from->begin = NULL;				// (2) mark the source as empty.
  };
  return *this;
}

/*
 */
template <class T>
size_t Vector<T>::copy(Iterator<T> from)  {
  const size_t        dim= from.count();
  if (dim)  {
    alloc(dim);
    do
      *(ShortIterator<T>::ptr)++ = *from++;
    while (from);
  };
  return dim;
}

template <class T>
void Vector<T>::alloc(const size_t initsize, const T initvalue)  {
  alloc(initsize);
  for (size_t cnt= initsize; cnt; --cnt)
     *(ShortIterator<T>::ptr)++ = initvalue;
  return;
}

/*
 */
template <class T>
void Vector<T>::Swap(Vector<T> &with)  {
  T *   buffer= begin; int t;
  begin= with.begin;  with.begin= buffer;                   // swap begin
  buffer= ShortIterator<T>::ptr;    ShortIterator<T>::ptr= with.ptr;      with.ptr= buffer;     // swap ptr
  buffer= end;    end= with.end;      with.end= buffer;     // swap end
  GDA_SWAP(sz, with.sz, t);
  return;
}

/*
        alloc
   Allocates Vector of given size and initializes all
   iterators. - Functionally implemented is that of a constructor,
   though it is not a constructor.
   Enables one to have separate declaration and allocation of
   a vector - a necessity for dynamic structures.
 */
template <class T>
inline void Vector<T>::alloc(const size_t initsize)  {
  sz= initsize;
  if (sz)  {
//    const int saveCount= newCount;
    begin= new T[sz];
//    newCount= saveCount+1;
    end= begin + sz;
    }
    else {  begin= NULL;  end= NULL;  };
  ShortIterator<T>::ptr= begin;
  return;
}

/*
        realloc
  Replaces existing Vector with one of given size (or adjusted
  to accomodate existing data). All data are copied from the old vector
  to the new one and memory from the old one is released. Safegurded
  against allocating too small vector to avoid possible loss of data.
 */
template <class T>
void Vector<T>::realloc(const size_t newsize)  {
  self NewVector(sl_max(count(), newsize));     // create a new Vector of given size
  Copy(NewVector, (*this)());           // copy the contents of existing one
  destroy();                            // release memory from the old one
  *this= NewVector;                     // new one replaces the old one
//  NewVector= self(NULL);                // 'nullify' the temporary object
  return;
}

/*
        HeapSort
   Sorts vector in the ascending order, using HeapSort.
   Assumes operator < is defined.
   For basic idea see Numerical Recipes in C., p.247.
 */
template <class T>
void HeapSort(Iterator<T> v)  {
  const size_t  half= v.count() >> 1;
  if (half == 0) return;        // nothing to sort;
  T * start = v.first();
  T * hire= start + half, * boss, * empl;
// hire will be decremented down to begin during the 'hiring' (heap creation).
  T * ir= v.last();
  T rra;
  while (ir > start)  {
    if (hire > start)  rra= *(--hire);  // still hiring
     else  {                    // retirement and promotion
        rra= *ir;               // clear space at end of vector
        *ir-- = *start;         // retire the top of the heap into it
       };
    boss= hire;            // set up to sift down rra to its proper level
    empl= boss + (boss - start);
    while (++empl <= ir)  {
       if (empl < ir && *empl < *(empl+1)) ++empl;
       if (rra < *empl)  {         // demote rra
             *boss= *empl;
             boss= empl;
             empl= boss + (boss - start);
           }
         else  empl= ir;   // rra is already in place
    };
    *boss= rra;            // i is right place for rra
  };
  return;
}

/*
        accumulator - for iterator
   Generic function of two arguments, first is a container
   over which the iterative accumulation procedure is performed,
   the second is a valid expression that can be interpreted
   as a function of one argument. Accumulator computes the
   sum of results derived by applying function f() to each
   element of the container. The total is returned as result.
   The type of result is the same as type of elements of a
   container.
 */
template <class D, class F>
D accumulator(Iterator<D> v, F f)  {
  D     acc(0);
  while (v)
    acc += f(*v++);
  return acc;
}


/*
        Destroy
  Template function to destroy all elements of a vector.
 */
template <class T>
inline void Destroy(Iterator<T> it)  {
  while (it)  {           // have more elements in the vector
    (*it).destroy();            // call a destructor
    ++it;
  };
  return;
}

template <class X>
inline void CopyInput(Vector<X> & dest, const std::vector<long>& nbl,
					  INDEX size)  
{
	X   v;
	for(INDEX i=0;i<size;i++) {
		v = nbl[i];
		dest << v;
	}
}


template <class X>
inline void CopyInput(Vector<X> & dest, const std::vector<long>& nbl,
					  const std::vector<double>& wtl, INDEX size)  
{
	X   v;
	for(INDEX i=0;i<size;i++)
	{
		v = pairstruct<INDEX, VALUE>(nbl[i], wtl[i]);
		dest << v;
	}
}

/*
 */
template <class X, class Y>
inline void Copy(X & dest, Y src)  {
  while (src)
    dest << *src++;
  return;
};



/*
 */
template <class T>
size_t Values(Iterator<T> it, const T threshold)  {
  size_t        Cnt= 0;
  for ( ; it; ++it)
    if (*it > threshold) ++Cnt;
  return Cnt;
}

#ifdef __SC__
 	#define MAP(K,T)  Vector<pairstruct<K, T>>
 #else
	template <class K, class T>
  	struct Map_types  {
    	typedef pairstruct<K, T> map_data;
    	typedef Vector<map_data> map_container;
 	 };

  	#define MAP(K,T) Map_types<K,T>::map_container
#endif

typedef pairstruct<INDEX, VALUE>      WPair;
typedef Vector<WPair>           WMap;
typedef Vector<WMap>            GWT;
typedef Vector<VALUE>           WVector;
typedef Iterator<VALUE>         WIterator;
typedef Vector<WVector>         WMatrix;

VALUE inline Product(Iterator<VALUE> it1, Iterator<VALUE> it2)  {
   VALUE    sum(0);
   for( ; it1; ++it1, ++it2)
     sum += *it1 * *it2;
   return sum;
}

template <class T>
inline T Sum(Iterator<T> it)  {
  T     sum(0);
  for ( ; it; ++it)
    sum += *it;
  return sum;
}

VALUE inline Product(Iterator<WPair> it1, Iterator<VALUE> it2)  {
    VALUE    sum(0);
    for( ; it1; ++it1)  {
        sum += (*it1).second * it2[ (*it1).first ];
    }  
    return sum;
}

template <class T>
inline Vector<T> & operator -= (Vector<T> &v1, const Vector<T> &v2)  {
   Iterator<T>      it1= v1(), it2= v2();
   for ( ; it1; ++it1, ++it2)
     *it1 -= *it2;
   return v1;
}

template <class T>
inline Vector<T> & operator += (Vector<T> &v1, const Vector<T> &v2)  {
   Iterator<T>      it1= v1(), it2= v2();
   for ( ; it1; ++it1, ++it2)
     *it1 += *it2;
   return v1;
}

template <class T>
inline Vector<T> & operator *= (Vector<T> &v1, const T v2)  {
   Iterator<T>      it1= v1();
   for ( ; it1; ++it1)
     *it1 *= v2;
   return v1;
}

/*
        Find
   Finds an element with a given key. Performs binary
   search. Assumes the vector has been previously sorted.
   Returns NULL if not found.
 */
template <class Pair, class Key>
Pair * Find(Iterator<Pair> it, const Key What)  {
   Pair * Sta= it(), * Sto= it.last(), *Median;
   do  {
     Median= Sta + (Sto - Sta)/2;
     if (Median->first < (int) What) Sta= Median + 1;
       else if (Median->first > (int) What) Sto= Median - 1;
         else  Sta= Sto= Median;
   }
   while (Sta < Sto);
   return (Sto->first == (int) What) ? Sto : NULL;
}

template <class Pair, class Key>
Pair * FindW(Iterator<Pair> it, const Key What)  {
   Pair * Sta= it(), * Sto= it.last(), *Median;
   do  {
     Median= Sta + (Sto - Sta)/2;
     if (Median->first < (int) What.first) Sta= Median + 1;
       else if (Median->first > (int) What.first) Sto= Median - 1;
         else  Sta= Sto= Median;
   }
   while (Sta < Sto);
   return (Sto->first == (int) What.first) ? Sto : NULL;
}



/*
        ColumnMultiply
product is a column-vector, that results from matrix (mt)
and vector (column) multiplication.
Assumes all objects already exist.
 */
// template <class R>
inline void ColumnMultiply(Iterator<VALUE> column, Iterator<WMap> mt,
						   Vector<VALUE> &product)  
{
  product.reset();
  for ( ; mt; ++mt)  {
         VALUE  TmpResult= 0;
         Iterator<WPair> it= (*mt)();
         for ( ; it; ++it)
           TmpResult += column[(*it).first] * (*it).second;
         product << TmpResult;
  };
  return;
}

inline int NeighborId(const Iterator<WPair> it)  {
   return (it) ? (*it).first : INT_MAX;
}    

#endif




