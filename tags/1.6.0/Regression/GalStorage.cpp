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

// GalStorage.cpp: implementation of the GalStorage class.
//
//////////////////////////////////////////////////////////////////////
#include <wx/wxprec.h>

#ifdef __BORLANDC__    
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/shp2gwt.h"
#include "../ShapeOperations/shp2cnt.h"

#include "GalStorage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*** compute spatial lag for a contiguity weights matrix
//*** optionally (default) performs standardization of the result
double GalStorageReg::SpatialLag(const double *x, const bool std) const  
{
  double    lag= 0;
  for (int cnt= Size(); cnt > 0; )
    lag += x[ data[--cnt] ];
  if (std && Size() > 1)
    lag /= Size();
  return lag;
}

//*** compute spatial lag for a contiguity weights matrix
//*** optionally (default) performs standardization of the result
double GalStorageReg::SpatialLag(const DataPoint *x, const bool std) const {
  double    lag= 0;
  for (int cnt= Size(); cnt > 0; )
    lag += x[ data[--cnt] ].horizontal;
  if (std && Size() > 1)
    lag /= Size();
  return lag;
}

//*** compute spatial lag for a contiguity matrix, with a given permutation
//*** optionally (default) performs standardization
double GalStorageReg::SpatialLag(const DataPoint *x, const int * perm,
								 const bool std) const  {
  double    lag = 0;
  for (int cnt = Size(); cnt > 0; )
    lag += x[ perm[ data[--cnt] ] ].horizontal;
  if (std && Size() > 1)
    lag /= Size();
  return lag;
}

double GalStorageReg::SpatialLag(const double *x, const int * perm,
								 const bool std) const  
{
  double    lag = 0;
  for (int cnt = Size(); cnt > 0; )
    lag += x[ perm[ data[--cnt]]];
  if (std && Size() > 1)
    lag /= Size();
  return lag;
}

//*** compute spatial lag for a contiguity matrix, with a given permutation, on a subset
//*** optionally (default) performs standardization
double GalStorageReg::SpatialSubLag(const DataPoint *x, const int * perm,
									const bool std) const  {
  double lag = 0;
  int sz= Size();
  for (int cnt= Size(); cnt > 0; )  {
    int cp= perm[ data[--cnt] ];
    if (cp != -1) lag += x[ cp ].horizontal;
      else --sz;
   };
   if (std && sz)
     lag /= sz;
   return lag;
}


int GalStorageReg::ReadTxt(ifstream &in, long ob)  
{

	// Notes on return:
	// -1 : wrong # of neighbors, too big or negative
	//  0 : doesn't have neighbor
	//  1 : good

	long id; int dim; long mydt;
	in >> id >> dim;

	
	size = dim;
	obs = ob;

	if (size == 0) return 0;
	if (size > obs-1 || size < 0) return -1;

	data = new long [ size ];
	if (data == NULL)  
	{
		size = 0;
		return 0;
	}
	for (int i=0;i< size;i++) 
	{
		in >> mydt; 
		data[i] = mydt-1;

		if (mydt > obs || mydt < 0) 
		{
			delete [] data;
			data = NULL;
			size = 0;
			return -1;
		}
	}
	return 1;
}



int GalStorageReg::ReadTxt(int dim, long* dt, long ob)  
{
	size = dim;
	obs = ob;
	if (size == 0) return 0;
	if (size > obs-1) return -1;
	data = new long [ size ];
	if (data == NULL)  
	{
		size = 0;
		return 0;
	}

	for (int i=0;i< size;i++) 
	{
		data[i] = dt[i];
		if (data[i] > obs || data[i] < 0) 
		{
			delete [] data;
			data = NULL;
			size = 0;
			return -1;
		}
	}

	return 1;
}

void GalStorageReg::Read(ifstream &in)  
{
	
  in.read((char *) size, sizeof(long));
  if (size == 0) return;
  data = new long [ size ];
  if (data == NULL)  
	{
    size = 0;
    return;
  };
  in.read( (char *) data, sizeof(long) * Size() );
  return;
}

