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

#include "SimplePoint.h"
#include "../GenUtils.h"

#ifndef GDA_SWAP
#define GDA_SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
#endif

/*
 Input SimplePoint from a text file.
 */
istream& SimplePoint::ReadShape(istream &s)  {
	long int skip;
	ReadID(s);
	GenUtils::SkipTillNumber(s);
#ifdef WORDS_BIGENDIAN
	s >> skip;
	char q[16], t;
	double m1, m2;
	s.read((char*)q, sizeof(double) * 2);
	GDA_SWAP(q[0], q[7], t);
	GDA_SWAP(q[1], q[6], t);
	GDA_SWAP(q[2], q[5], t);
	GDA_SWAP(q[3], q[4], t);
	memcpy(&m1, &q[0], sizeof(double));
	GDA_SWAP(q[8], q[15], t);
	GDA_SWAP(q[9], q[14], t);
	GDA_SWAP(q[10], q[13], t);
	GDA_SWAP(q[11], q[12], t);
	memcpy(&m2, &q[8], sizeof(double));
	p = BasePoint(m1, m2);
#else
	s >> skip >> p;
#endif
	return s;
}

oShapeFile& SimplePoint::WriteShape(oShapeFile &s) const
{  
#ifdef WORDS_BIGENDIAN
	char q[16], t;
	memcpy(&q[0], &(p.x), sizeof(double));
	GDA_SWAP(q[0], q[7], t);
	GDA_SWAP(q[1], q[6], t);
	GDA_SWAP(q[2], q[5], t);
	GDA_SWAP(q[3], q[4], t);
	s.write(&q[0], sizeof(double) * 1);
	memcpy(&q[8], &(p.y), sizeof(double));
	GDA_SWAP(q[8], q[15], t);
	GDA_SWAP(q[9], q[14], t);
	GDA_SWAP(q[10], q[13], t);
	GDA_SWAP(q[11], q[12], t);
	s.write(&q[8], sizeof(double) * 1);
	return s;
#else
	return s << p;
#endif
}

iShapeFile& SimplePoint::ReadShape(iShapeFile &s)
{
	Identify(s.Record());  
#ifdef WORDS_BIGENDIAN
	char q[16], t;
	double m1, m2;
	s.read((char*)q, sizeof(double) * 2);
	GDA_SWAP(q[0], q[7], t);
	GDA_SWAP(q[1], q[6], t);
	GDA_SWAP(q[2], q[5], t);
	GDA_SWAP(q[3], q[4], t);
	memcpy(&m1, &q[0], sizeof(double));
	GDA_SWAP(q[8], q[15], t);
	GDA_SWAP(q[9], q[14], t);
	GDA_SWAP(q[10], q[13], t);
	GDA_SWAP(q[11], q[12], t);
	memcpy(&m2, &q[8], sizeof(double));
	p = BasePoint(m1, m2);
	return s;
#else
	return s >> p;
#endif
};
