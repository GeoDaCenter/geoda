/**
 * OpenGeoDa TM, Copyright (C) 2011 by Luc Anselin - all rights reserved
 *
 * This file is part of OpenGeoDa.
 * 
 * OpenGeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenGeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// GalStorage.h: interface for the GalStorage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GALSTORAGE_H__E5F835AD_9128_4461_876F_3F06F499E144__INCLUDED_)
#define AFX_GALSTORAGE_H__E5F835AD_9128_4461_876F_3F06F499E144__INCLUDED_


class GalStorageReg 
{
  private :
		long  obs;
    long	size;        // current size of the creature
    long	*data;
		int		contiguityType; //0: rook, 1: queen
  public :
    GalStorageReg(const long sz= 0, const int type = 0) : 
		 size(0)  
		 {
				obs = 0;
        if (sz == 0) data= NULL;
				else data= new long [ sz ];
        return;
    };
    ~GalStorageReg()  {
        if (data) delete [] data;
        data= NULL;
        size= 0;
    };
    int alloc(const int sz, const int obs)  {
        data= new long [ sz ];
        return !empty();
    };
    bool empty()  const  {  return data == NULL;  };
    void Push(const long val)  {
        data [ size++ ]= val;
        return;
    };
    long Pop()  {
        if (!size)  return -1;
        return data [ --size ];
    };
    long Size() const  {  return size;  };
    int GetConType() const  {  return contiguityType;  };
    long elt(const long where) const  {  return data[where];  };
    long * dt() const  {  return data;  };
    double SpatialLag(const double *x, const bool std= true)  const;
    double SpatialLag(const DataPoint *x, const bool std= true)  const;
    double SpatialLag(const DataPoint *x, const int * perm, const bool std= true) const;
    double SpatialLag(const double *x, const int * perm, const bool std= true) const;
    double SpatialSubLag(const DataPoint *x, const int * perm, const bool std= true) const;
    void Read(ifstream &in);
		int ReadTxt(ifstream &in, long ob);
		int ReadTxt(int dim, long* dt, long ob);
};

#endif // !defined(AFX_GALSTORAGE_H__E5F835AD_9128_4461_876F_3F06F499E144__INCLUDED_)
