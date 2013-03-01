/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "shp.h"
#include <time.h>
#include "../logger.h"
#include "../GenUtils.h"
#include "../GeoDaConst.h"


long ReadBig(ifstream &input)
{
	long tmp;
	input.read((char *) &tmp, 4);
#ifdef WORDS_BIGENDIAN        // reverse bytes if Intel processor
	return tmp;
#else                         // don't reverse for Mac and Unix comps
	return GenUtils::Reverse(tmp);
#endif
}

/*
        MultiPoint::ComputeBox
 */
void MultiPoint::ComputeBox()  {
	if (NumPoints)  {
		bBox= Box(Points[0]);
		for (long cp= 1; cp < NumPoints; ++cp) bBox += Points[cp];
	} else {
		bBox= Box();
	}
}

BasePoint MultiPoint::MeanCenter() const
{
//	LOG_MSG("Entering MultiPoint::MeanCenter");
	double x_sum = 0;
	double y_sum = 0;
	for (long cnt= 0; cnt < NumPoints - 1; ++cnt) {
		x_sum += Points[cnt].x;
		y_sum += Points[cnt].y;
	}
	BasePoint bp(x_sum/(NumPoints-1), y_sum/(NumPoints-1));
//	LOG_MSG("Exiting MultiPoint::MeanCenter");
	return bp;
}

BasePoint MultiPoint::Centroid() const
{
//	LOG_MSG("Entering MultiPoint::Centroid");
	long cnt = 0;
	double area = 0, sum_x = 0, sum_y = 0;
	for (cnt = 0; cnt < NumPoints - 1; cnt++) {
		area += (Points[cnt].x * Points[cnt + 1].y -
				 Points[cnt + 1].x * Points[cnt].y);
		sum_x += (Points[cnt].x + Points[cnt + 1].x) *
		(Points[cnt].x * Points[cnt + 1].y - Points[cnt + 1].x * Points[cnt].y);
		sum_y += (Points[cnt].y + Points[cnt + 1].y) *
		(Points[cnt].x * Points[cnt + 1].y - Points[cnt + 1].x * Points[cnt].y);
	}
	BasePoint bp(sum_x / (3 * area), sum_y / (3 * area));
//	LOG_MSG("Exiting MultiPoint::Centroid");
	return bp;
}

/*
Output Shape in a text file
 */
ostream& Shape::WriteShape(ostream &s) const
{
	long int part, curr= 0, last= 0;
	if (Parts && Points)  {
		for (part= 0; part < NumParts; part++)  {
			last= (part+1 < NumParts) ? Parts[part+1] : NumPoints;
			WriteID(s, last-curr);
			do {
#ifdef WORDS_BIGENDIAN
				char p[16], t;
				memcpy(&p[0], &(Points[curr].x), sizeof(double));
				SWAP(p[0], p[7], t);
				SWAP(p[1], p[6], t);
				SWAP(p[2], p[5], t);
				SWAP(p[3], p[4], t);
				s.write(&p[0], sizeof(double) * 1);
				memcpy(&p[0], &(Points[curr].y), sizeof(double));
				SWAP(p[0], p[7], t);
				SWAP(p[1], p[6], t);
				SWAP(p[2], p[5], t);
				SWAP(p[3], p[4], t);
				s.write(&p[0], sizeof(double) * 1);
#else
				s << Points[curr] << endl;
#endif
			}
			while (++curr < last);
		}
	}
	return s;
}

/*
Input MultiPoint from a text file.
 */
istream& MultiPoint::ReadShape(istream &s)  {
  long int cp;
  ReadID(s);
	if (s.fail()) { Identify(0);  return s; };
	GenUtils::SkipTillNumber(s);
  #ifdef WORDS_BIGENDIAN
  char p[16], q[4], t;
  s.read((char *)q, sizeof(long));
  SWAP(q[0], q[3], t);
  SWAP(q[1], q[2], t);
  memcpy(&NumPoints, q, sizeof(long));
  #else
  s >> NumPoints;
  #endif
  Points= new BasePoint[NumPoints];
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
    double m1, m2;
    s.read((char *)p, sizeof(double) * 2);
    SWAP(p[0], p[7], t);
    SWAP(p[1], p[6], t);
    SWAP(p[2], p[5], t);
    SWAP(p[3], p[4], t);
    memcpy(&m1, &p[0], sizeof(double));
    SWAP(p[8], p[15], t);
    SWAP(p[9], p[14], t);
    SWAP(p[10], p[13], t);
    SWAP(p[11], p[12], t);
    memcpy(&m2, &p[8], sizeof(double));
    Points[cp] = BasePoint(m1, m2);
    #else
    s >> Points[cp];
    #endif
  }
  ComputeBox();
  return s;
}


Box Shape::SetData(int nParts, int* Part, int nPoints, 
				   const std::vector<BasePoint>& Pts) 
{
	NumParts = nParts; NumPoints = nPoints;
	Parts = new long int[nParts];
	Points = new BasePoint[nPoints];
	int i = 0;
	for (i=0; i<nParts; i++) Parts[i] = Part[i];

	Box b(Pts.at(0),Pts.at(0));

	for (i=0; i<nPoints; i++) 
	{
		Points[i] = Pts.at(i);
		b += Points[i];
	}

	bBox = b;
	return bBox;
}

/*
Input Shape from Shapefile.
 */
iShapeFile& Shape::ReadShape(iShapeFile &s)  {

  long int cp;
  Identify(s.Record());
  #ifdef WORDS_BIGENDIAN
  char r[32], p[16], q[8], w[4], t;
  double m1, m2, n1, n2;
  s.read((char *)r, sizeof(double) * 4);
  SWAP(r[0], r[7], t);
  SWAP(r[1], r[6], t);
  SWAP(r[2], r[5], t);
  SWAP(r[3], r[4], t);
  memcpy(&m1, &r[0], sizeof(double));
  SWAP(r[8], r[15], t);
  SWAP(r[9], r[14], t);
  SWAP(r[10], r[13], t);
  SWAP(r[11], r[12], t);
  memcpy(&m2, &r[8], sizeof(double));
  SWAP(r[16], r[23], t);
  SWAP(r[17], r[22], t);
  SWAP(r[18], r[21], t);
  SWAP(r[19], r[20], t);
  memcpy(&n1, &r[16], sizeof(double));
  SWAP(r[24], r[31], t);
  SWAP(r[25], r[30], t);
  SWAP(r[26], r[29], t);
  SWAP(r[27], r[28], t);
  memcpy(&n2, &r[24], sizeof(double));
  BasePoint p1(m1, m2);
  BasePoint p2(n1, n2);
  bBox = Box(p1, p2);
  s.read((char *)q, sizeof(long) * 2);
  SWAP(q[0], q[3], t);
  SWAP(q[1], q[2], t);
  memcpy(&NumParts, &q[0], sizeof(long));
  SWAP(q[4], q[7], t);
  SWAP(q[5], q[6], t);
  memcpy(&NumPoints, &q[4], sizeof(long));
  #else
  s >> bBox >> NumParts >> NumPoints;
  #endif

	if (Parts) delete [] Parts; Parts=NULL;
	if (Points) delete [] Points; Points=NULL;

  Parts= new long int[NumParts];
  Points= new BasePoint[NumPoints];
  for (cp= 0; cp < NumParts; cp++) {
    #ifdef WORDS_BIGENDIAN
      s.read((char *)w, sizeof(long));
      SWAP(w[0], w[3], t);
      SWAP(w[1], w[2], t);
      memcpy(&Parts[cp], &w[0], sizeof(long));
    #else
      s >> Parts[cp];
    #endif
  }
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
      s.read((char *)p, sizeof(double) * 2);
      SWAP(p[0], p[7], t);
      SWAP(p[1], p[6], t);
      SWAP(p[2], p[5], t);
      SWAP(p[3], p[4], t);
      memcpy(&m1, &p[0], sizeof(double));
      SWAP(p[8], p[15], t);
      SWAP(p[9], p[14], t);
      SWAP(p[10], p[13], t);
      SWAP(p[11], p[12], t);
      memcpy(&m2, &p[8], sizeof(double));
      Points[cp] = BasePoint(m1, m2);
    #else
      s >> Points[cp];
    #endif
  }

  return s;
}

/*
Output Shape in a Shapefile.
 */
oShapeFile& Shape::WriteShape(oShapeFile &s) const {
  long int cp;
  #ifdef WORDS_BIGENDIAN
  char r[32], p[16], q[8], w[4], t;
  double m1, m2, n1, n2;
  m1 = bBox._min().x;
  memcpy(&r[0], &m1, sizeof(double));
  SWAP(r[0], r[7], t);
  SWAP(r[1], r[6], t);
  SWAP(r[2], r[5], t);
  SWAP(r[3], r[4], t);
  m2 = bBox._min().y;
  memcpy(&r[8], &m2, sizeof(double));
  SWAP(r[8], r[15], t);
  SWAP(r[9], r[14], t);
  SWAP(r[10], r[13], t);
  SWAP(r[11], r[12], t);
  n1 = bBox._max().x;
  memcpy(&r[16], &n1, sizeof(double));
  SWAP(r[16], r[23], t);
  SWAP(r[17], r[22], t);
  SWAP(r[18], r[21], t);
  SWAP(r[19], r[20], t);
  n2 = bBox._max().y;
  memcpy(&r[24], &n2, sizeof(double));
  SWAP(r[24], r[31], t);
  SWAP(r[25], r[30], t);
  SWAP(r[26], r[29], t);
  SWAP(r[27], r[28], t);
  s.write(&r[0], sizeof(double) * 4);
  memcpy(&q[0], &NumParts, sizeof(long));
  SWAP(q[0], q[3], t);
  SWAP(q[1], q[2], t);
  s.write(&q[0], sizeof(long));
  memcpy(&q[4], &NumPoints, sizeof(long));
  SWAP(q[4], q[7], t);
  SWAP(q[5], q[6], t);
  s.write(&q[4], sizeof(long));
  for (cp = 0; cp < NumParts; cp++) {
      memcpy(&w[0], &Parts[cp], sizeof(long));
      SWAP(w[0], w[3], t);
      SWAP(w[1], w[2], t);
      s.write(&w[0], sizeof(long));
  }
  for (cp = 0; cp < NumPoints; cp++) {
      memcpy(&p[0], &(Points[cp].x), sizeof(double));
      SWAP(p[0], p[7], t);
      SWAP(p[1], p[6], t);
      SWAP(p[2], p[5], t);
      SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[cp].y), sizeof(double));
      SWAP(p[8], p[15], t);
      SWAP(p[9], p[14], t);
      SWAP(p[10], p[13], t);
      SWAP(p[11], p[12], t);
      s.write(&p[8], sizeof(double));
  }
  #else
  s << bBox << NumParts << NumPoints;
  for (cp= 0; cp < NumParts; cp++) s << Parts[cp];
  for (cp= 0; cp < NumPoints; cp++) s << Points[cp];
  #endif
  return s;
}


/*
 BasePartition
 */
void BasePartition::alloc(const int els, const int cls, const double range)
{
	elements= els;
	cells= cls;
	step= range / cls;
	cell= new int [ cells ];
	next= new int [ elements ];
	if (cell && next)
		for (int cnt= 0; cnt < cells; ++cnt) cell [ cnt ] = GeoDaConst::EMPTY;
	else elements= cells= 0;
}

/*
 BasePartition
 */
BasePartition::BasePartition(const int els, const int cls, const double range)
: elements(els), cells(cls), cell(0), next(0)
{
	if (elements > 0) alloc(els, cls, range);
}


/*
 BasePartition
 */
BasePartition::~BasePartition()
{
	if (cell) delete [] cell; cell = 0;
	if (next) delete [] next; next = 0;
	elements = 0;
	cells = 0;
}


/*
 PartitionP
 */
void PartitionP::alloc(const int els, const int cls, const double range)  {
	BasePartition::alloc(els, cls, range);
	cellIndex= new int [elements ];
	previous= new int [ elements ];
	if (!cellIndex || !previous)  elements= cells= 0;
}

/*
 PartitionP
 */
PartitionP::PartitionP(const int els, const int cls, const double range) :
BasePartition(els, cls), cellIndex(NULL), previous(NULL)
{
	if (elements > 0)  alloc(els, cls, range);
}

/*
 PartitionP
 */
PartitionP::~PartitionP()
{
	if (cellIndex) delete [] cellIndex;
	if (previous) delete [] previous;
	cellIndex= previous= NULL;
}

/*
 PartitionP::include
 Overloaded function to include an element in the partition.
 Assumes that cellIndex has been initialized.
 */
void PartitionP::include(const int incl)  {
	int where = cellIndex [ incl ];
	//        if (where < 0 || where >= cells || incl < 0 || incl >= elements)
	//          cout << "including " << incl << " at " << where << endl;
	int old= cell [ where ];
	cell [ where ] = incl;
	if (old != GeoDaConst::EMPTY)
		previous [ old ] = incl;
	next [ incl ] = old;    // OLD becomes the 2nd element in the list
	previous [ incl ] = GeoDaConst::EMPTY;       // there are no elements prior to incl
	return;
}

/*
 PartitionP
 */
void PartitionP::remove(const int del)  {
	int   thePrevious= previous[ del ], theNext= next[ del ];
	if ( thePrevious == GeoDaConst::EMPTY )                // this is the 1st element in the list
		cell [ cellIndex[del] ] = theNext;
    else
		next[ thePrevious ] = theNext;
	if ( theNext != GeoDaConst::EMPTY )                   // this is not the last element in thelist
		previous[ theNext ] = thePrevious;
	previous[ del ] = next [ del ] = GeoDaConst::EMPTY;  // probably this is not necessary
	return;
}



/*
 PolygonPartition:: destructor
 */
PolygonPartition::~PolygonPartition()   {
	if (nbrPoints)  {  delete [] nbrPoints;  nbrPoints= NULL;  };
	return;
}

wxString getPointStr(const BasePoint& point)
{
	wxString s;
	s << "(" << point.x << "," << point.y << ")";
	return s;
}

/** Method for detecting if an edge is shared between a host and guest polygon.
 */
bool PolygonPartition::edge(const PolygonPartition &p, const int host,
							const int guest)  
{
	BasePoint guestPrev = p.Points[ p.prev(guest) ]; 
	BasePoint hostPoint = Points[ succ(host) ];
	if (hostPoint == guestPrev) {
		//LOG_MSG("PolygonPartition::edge: hostPoint == guestPrev true");
		//wxString msg;
		//msg << "  hostPoint" << getPointStr(hostPoint);
		//msg << ", guestPrev" << getPointStr(guestPrev);
		//LOG_MSG(msg);
		return true;
	}
	
	BasePoint guestSucc= p.Points[ p.succ(guest) ];
	if (hostPoint == guestSucc) {
		//LOG_MSG("PolygonPartition::edge: hostPoint == guestSucc true");
		//wxString msg;
		//msg << "  hostPoint" << getPointStr(hostPoint);
		//msg << ", guestSucc" << getPointStr(guestSucc);
		//LOG_MSG(msg);
		return true;
	}
	
	hostPoint= Points[ prev(host) ];
	
	if (hostPoint == guestSucc) {
		//LOG_MSG("PolygonPartition::edge: hostPoint(prev) == guestSucc true");
		//wxString msg;
		//msg << "  hostPoint" << getPointStr(hostPoint);
		//msg << ", guestSucc" << getPointStr(guestSucc);
		//LOG_MSG(msg);
		return true;
	}
	
	if (hostPoint == guestPrev) {
		//LOG_MSG("PolygonPartition::edge: hostPoint(prev) == guestPrev true");
		//wxString msg;
		//msg << "  hostPoint" << getPointStr(hostPoint);
		//msg << ", guestPrev" << getPointStr(guestPrev);
		//LOG_MSG(msg);
		return true;
	}
	
	return false;
}

/*
 PolygonPartition
 */
int PolygonPartition::MakePartition(int mX, int mY)  {
	if (mX == 0) mX = NumPoints/4 + 2;
	if (mY == 0) mY = (int)(sqrt((long double)NumPoints) + 2);
	pX.alloc(NumPoints, mX, bBox._max().x - bBox._min().x);
	pY.alloc(NumPoints, mY, bBox._max().y - bBox._min().y);
	double        xStart= bBox._min().x, yStart= bBox._min().y;
	for (int cnt= 0; cnt < NumPoints; ++cnt)  {
		pX.include(cnt, Points[cnt].x - xStart);
		pY.initIx(cnt, Points[cnt].y - yStart);
	};
	MakeNeighbors();
	return 0;
}

/*
 PolygonPartition
 */
void PolygonPartition::MakeNeighbors()  
{
	nbrPoints= new int [ NumPoints ];
	if (nbrPoints == NULL) return;
	for (int cnt= 0; cnt < NumPoints; ++cnt) {
		nbrPoints [ cnt ] = cnt+1;
	}
	int first= 0, last;
	for (int part= 1; part <= NumParts; ++part) {
		last= (part == NumParts) ? NumPoints : Parts[part];
		nbrPoints [ first ] = -(last-2);
		nbrPoints [ last-1 ] = first+1;
		first= last;
	}
}

/*
 PolygonPartition
 */
void PolygonPartition::MakeSmallPartition(const int mX, const double Start,
										  const double Stop)
{
	pX.alloc(NumPoints, mX, Stop-Start);
	for (int cnt= 0; cnt < NumPoints; ++cnt) {
		BasePoint pt= Points[cnt];
		if (pt.x >= Start && pt.x <= Stop) pX.include(cnt, pt.x - Start);
	}
	MakeNeighbors();
}

/*
 PolygonPartition::sweep
 Determines if two polygons are neighbors. The host is assumed to be
 partitioned.
 Uses two criteria to establish neighborhood:
 0 -- common point;  1 -- common boundary.
 */
int PolygonPartition::sweep(PolygonPartition & guest, const int criteria)  
{
	int       host, dot, cly, cell;
	double    yStart= bBox._min().y, yStop= bBox._max().y;
	BasePoint pt;
	guest.MakeSmallPartition(pX.Cells(), bBox._min().x, bBox._max().x);
	for (cell= 0; cell < pX.Cells(); ++cell) {
		for (host= pX.first(cell); host != GeoDaConst::EMPTY;
			 host= pX.tail(host)) pY.include(host);
		for (dot=guest.pX.first(cell); dot != GeoDaConst::EMPTY;
			 dot=guest.pX.tail(dot)) {
			pt= guest.Points[dot];
			cly= pY.inTheRange(pt.y - yStart);
			if (cly != -1) {
				for (host= pY.first(cly); host != GeoDaConst::EMPTY;
					 host= pY.tail(host)) {
					if (pt == Points[host]) {
						if (criteria == 0 || edge(guest, host, dot)) { 
							pY.cleanup(pX, cell);  
							return 1;  
						}
					}
				}
			}
		}
		pY.cleanup(pX, cell);
	}
	return 0;
}



/*
Output MultiPoint in a text file.
 */
ostream & MultiPoint::WriteShape(ostream &s) const {
  WriteID(s);
  for (long point= 0; point < NumPoints; point++) {
    #ifdef WORDS_BIGENDIAN
      char p[16], t;
      memcpy(&p[0], &(Points[point].x), sizeof(double));
      SWAP(p[0], p[7], t);
      SWAP(p[1], p[6], t);
      SWAP(p[2], p[5], t);
      SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[point].y), sizeof(double));
      SWAP(p[8], p[15], t);
      SWAP(p[9], p[14], t);
      SWAP(p[10], p[13], t);
      SWAP(p[11], p[12], t);
      s.write(&p[8], sizeof(double));
      s << endl;
    #else
    s << Points[point] << endl;
    #endif
  }
  return s;
}

/*
Input Ppoint from a text file.
 */
istream& Ppoint::ReadShape(istream &s)  {
	long int skip;
	ReadID(s);
	GenUtils::SkipTillNumber(s);
	#ifdef WORDS_BIGENDIAN
	s >> skip;
	char q[16], t;
	double m1, m2;
	s.read((char*)q, sizeof(double) * 2);
	SWAP(q[0], q[7], t);
	SWAP(q[1], q[6], t);
	SWAP(q[2], q[5], t);
	SWAP(q[3], q[4], t);
	memcpy(&m1, &q[0], sizeof(double));
	SWAP(q[8], q[15], t);
	SWAP(q[9], q[14], t);
	SWAP(q[10], q[13], t);
	SWAP(q[11], q[12], t);
	memcpy(&m2, &q[8], sizeof(double));
	p = BasePoint(m1, m2);
	#else
	s >> skip >> p;
	#endif
	return s;
}


/*
 */
PolygonShape & PolygonShape::operator +=(const PolygonShape &a)  {
	long int cp, o_NumPoints= NumPoints, o_NumParts= NumParts, *o_Parts= Parts;
	BasePoint * o_Points= Points;
	Points= new BasePoint[ NumPoints += a.NumPoints ];
	for (cp= 0; cp < o_NumPoints; cp++) Points[cp]= o_Points[cp];
	for (cp= 0; cp < a.NumPoints; cp++) Points[cp+o_NumPoints]= a.Points[cp];
	delete [] o_Points;
	o_Points = NULL;
	Parts= new long int[ NumParts += a.NumParts ];
	for (cp= 0; cp < o_NumParts; cp++) Parts[cp]= o_Parts[cp];
	for (cp= 0; cp < a.NumParts; cp++) {
		Parts[cp+o_NumParts]= a.Parts[cp] + o_NumPoints;
	}
	delete [] o_Parts;
	o_Parts = NULL;
	bBox += a.bBox;
	return *this;
}

/*
 */
void PolygonShape::SeparateParts()  {
  long int cPoint, cPart, tParts= 0;
  BasePoint fPoint, *nPoints;
  if (NumPoints < 4) return;
  fPoint= Points[0];
  for (cPoint= 1; cPoint < NumPoints; ++cPoint)
    if (Points[cPoint] == fPoint) ++tParts;
  if (tParts == 1) return;              // one part polygon
    if (Parts) delete [] Parts; Parts=NULL;
    NumParts= tParts;
    Parts= new long[NumParts];
    Parts[0]= 0;
    cPoint= 0;
    NumPoints -= (NumParts - 1);
    nPoints= new BasePoint[NumPoints];
    for (cPart= 1; cPart < NumParts; cPart++)  {
        do
          ++cPoint;
        while (fPoint != Points[cPoint]);
        Parts[cPart] = cPoint - cPart + 2;
    };
    for (cPart= 0; cPart < NumParts; cPart++)  {
       tParts= Parts[cPart] + cPart;
       if (cPart) tParts--;
       cPoint= (cPart == NumParts-1) ? NumPoints : Parts[cPart+1];
       memcpy(&nPoints[Parts[cPart]], &Points[tParts],
                 (cPoint - Parts[cPart]) * sizeof(BasePoint));
    };
    if (Points) delete [] Points;
    Points= nPoints;
  return;
}

iShapeFile & MultiPoint::ReadShape(iShapeFile &s)  {
  long int cp;
  Identify(s.Record());
  #ifdef WORDS_BIGENDIAN
  char r[32], q[16], p[4], t;
  double m1, m2, n1, n2;
  s.read((char *)r, sizeof(double) * 4);
  s.read((char *)p, sizeof(long));
  SWAP(r[0], r[7], t);
  SWAP(r[1], r[6], t);
  SWAP(r[2], r[5], t);
  SWAP(r[3], r[4], t);
  memcpy(&m1, &r[0], sizeof(double));
  SWAP(r[8], r[15], t);
  SWAP(r[9], r[14], t);
  SWAP(r[10], r[13], t);
  SWAP(r[11], r[12], t);
  memcpy(&m2, &r[8], sizeof(double));
  SWAP(r[16], r[23], t);
  SWAP(r[17], r[22], t);
  SWAP(r[18], r[21], t);
  SWAP(r[19], r[20], t);
  memcpy(&n1, &r[16], sizeof(double));
  SWAP(r[24], r[31], t);
  SWAP(r[25], r[30], t);
  SWAP(r[26], r[29], t);
  SWAP(r[27], r[28], t);
  memcpy(&n2, &r[24], sizeof(double));
  BasePoint o1(m1, m2);
  BasePoint o2(n1, n2);
  bBox = Box(o1, o2);
  SWAP(p[0], p[3], t);
  SWAP(p[1], p[2], t);
  memcpy(&NumPoints, &p[0], sizeof(long));
  #else
  s >> bBox >> NumPoints;
  #endif

  if (Points) delete [] Points;
  Points= new BasePoint[NumPoints];
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
      s.read((char *)q, sizeof(double) * 2);
      SWAP(q[0], q[7], t);
      SWAP(q[1], q[6], t);
      SWAP(q[2], q[5], t);
      SWAP(q[3], q[4], t);
      memcpy(&m1, &q[0], t);
      SWAP(q[8], q[15], t);
      SWAP(q[9], q[14], t);
      SWAP(q[10], q[13], t);
      SWAP(q[11], q[12], t);
      memcpy(&m2, &q[8], t);
      Points[cp] = BasePoint(m1, m2);
    #else
      s >> Points[cp];
    #endif
  } 
  return s;
}


oShapeFile& MultiPoint::WriteShape(oShapeFile &s) const {
  long int cp;
  #ifdef WORDS_BIGENDIAN
  char r[32], p[16], q[8], w[4], t;
  double m1, m2, n1, n2;
  m1 = bBox._min().x;
  memcpy(&r[0], &m1, sizeof(double));
  SWAP(r[0], r[7], t);
  SWAP(r[1], r[6], t);
  SWAP(r[2], r[5], t);
  SWAP(r[3], r[4], t);
  m2 = bBox._min().y;
  memcpy(&r[8], &m2, sizeof(double));
  SWAP(r[8], r[15], t);
  SWAP(r[9], r[14], t);
  SWAP(r[10], r[13], t);
  SWAP(r[11], r[12], t);
  n1 = bBox._max().x;
  memcpy(&r[16], &n1, sizeof(double));
  SWAP(r[16], r[23], t);
  SWAP(r[17], r[22], t);
  SWAP(r[18], r[21], t);
  SWAP(r[19], r[20], t);
  n2 = bBox._max().y;
  memcpy(&r[24], &n2, sizeof(double));
  SWAP(r[24], r[31], t);
  SWAP(r[25], r[30], t);
  SWAP(r[26], r[29], t);
  SWAP(r[27], r[28], t);
  s.write(&r[0], sizeof(double) * 4);
  memcpy(&q[0], &NumPoints, sizeof(long));
  SWAP(q[0], q[3], t);
  SWAP(q[1], q[2], t);
  s.write(&q[0], sizeof(long));
  for (cp = 0; cp < NumPoints; cp++) {
      memcpy(&p[0], &(Points[cp].x), sizeof(double));
      SWAP(p[0], p[7], t);
      SWAP(p[1], p[6], t);
      SWAP(p[2], p[5], t);
      SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[cp].y), sizeof(double));
      SWAP(p[8], p[15], t);
      SWAP(p[9], p[14], t);
      SWAP(p[10], p[13], t);
      SWAP(p[11], p[12], t);
      s.write(&p[8], sizeof(double));
  }
  #else
  s << bBox << NumPoints;
  for (cp= 0; cp < NumPoints; cp++) s << Points[cp];
  #endif
  return s;
}



oShapeFile& Ppoint::WriteShape(oShapeFile &s) const
{  
#ifdef WORDS_BIGENDIAN
	char q[16], t;
	memcpy(&q[0], &(p.x), sizeof(double));
	SWAP(q[0], q[7], t);
	SWAP(q[1], q[6], t);
	SWAP(q[2], q[5], t);
	SWAP(q[3], q[4], t);
	s.write(&q[0], sizeof(double) * 1);
	memcpy(&q[8], &(p.y), sizeof(double));
	SWAP(q[8], q[15], t);
	SWAP(q[9], q[14], t);
	SWAP(q[10], q[13], t);
	SWAP(q[11], q[12], t);
	s.write(&q[8], sizeof(double) * 1);
	return s;
#else
	return s << p;
#endif
}

iShapeFile& Ppoint::ReadShape(iShapeFile &s)
{
	Identify(s.Record());  
#ifdef WORDS_BIGENDIAN
	char q[16], t;
	double m1, m2;
	s.read((char*)q, sizeof(double) * 2);
	SWAP(q[0], q[7], t);
	SWAP(q[1], q[6], t);
	SWAP(q[2], q[5], t);
	SWAP(q[3], q[4], t);
	memcpy(&m1, &q[0], sizeof(double));
	SWAP(q[8], q[15], t);
	SWAP(q[9], q[14], t);
	SWAP(q[10], q[13], t);
	SWAP(q[11], q[12], t);
	memcpy(&m2, &q[8], sizeof(double));
	p = BasePoint(m1, m2);
	return s;
#else
	return s >> p;
#endif
};





