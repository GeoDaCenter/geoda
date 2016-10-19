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

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <vector>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "AbstractShape.h"
#include "BasePoint.h"
#include "Box.h"
#include "../ShpFile.h"
#include "ShapeFile.h"
#include "ShapeFileHdr.h"

#include "../logger.h"
#include "../GenUtils.h"
#include "PolysToContigWeights.h"

using namespace std;

#ifndef GDA_SWAP
#define GDA_SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
#endif


/**
 MultiPoint - Multipoint Shape
 Class for shapes with multiple points.
 Corresponds to multipoint shape in the Shapefile.
 */
class MultiPoint : public virtual AbstractShape  {
public:
	MultiPoint() : NumPoints(0), Points(NULL) {};
	MultiPoint(char *nme, const long &points) :  AbstractShape(nme),
	Points(new BasePoint[ points ]), NumPoints(points) {};
	virtual ~MultiPoint()  {  if (Points) delete [] Points; return;  };
	virtual Box ShapeBox() const  {  return bBox;  };
	virtual BasePoint MeanCenter() const;
	virtual BasePoint Centroid() const;
	virtual long ContentsLength() const  {  return 20 + 8 * NumPoints;  };
	virtual ostream& WriteShape(ostream &s) const;
	virtual istream& ReadShape(istream &s);
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual Box SetData(int nParts, int* Part, int nPoints,
											const std::vector<BasePoint>& P) {
		Box b; return b;
	}
	virtual void AssignPointData(double x, double y) {};
	protected :
	long          NumPoints;
	BasePoint         *Points;
	Box           bBox;
	void ComputeBox();
};

/**
 Shape
 Class for shapes with multiple parts.
 */
class Shape : public virtual MultiPoint  {
	public :
	virtual long ContentsLength() const {
		return 22 + 2 * NumParts + 8 * NumPoints;  }
	virtual ostream& WriteShape(ostream &s) const;
	virtual istream& ReadShape(istream &s) {
		MultiPoint::ReadShape(s);  NumParts= 1;
		Parts= new long int[1];  Parts[0]= 0;
		return s;
	};
	virtual oShapeFile& WriteShape(oShapeFile &s) const;
	virtual iShapeFile& ReadShape(iShapeFile &s);
	virtual Box SetData(int nParts, int* Part, int nPoints,
											const std::vector<BasePoint>& P);
	protected :
	long int      NumParts;
	long int      *Parts;
	Box			oBox; // FileBox: from data
	Shape() : Parts(NULL), NumParts(0)  {};
	Shape(char *nme, const long int &parts, const long int &points) :
	MultiPoint(nme, points), NumParts(parts), Parts(new long[ parts ]) {
		Parts[0]= 0;
		return;
	}
	virtual ~Shape()  {  if (Parts) delete [] Parts;  return; };
	virtual void AssignPointData(double x, double y) {};
};

/**
 PolygonShape
 Class for Polygon shapes - corresponds to the Polygon shape
 in the Shapefile.
 */
class PolygonShape : public virtual Shape {
public:
  PolygonShape &  operator+=(const PolygonShape &a);
  virtual void    SeparateParts();
};

/**
 BasePartition
 */
class BasePartition  {
	protected :
	int         elements, cells;
	int *       cell;
	int *       next;
	double      step;
	public :
	BasePartition(const int els= 0, const int cls= 0, const double range= 0);
	virtual ~BasePartition();
	void virtual alloc(const int els, const int cls, const double range);
	int         Cells() const  {  return cells;  };
	double      Step() const  {  return step;  };
	virtual void include(const int incl, const double range)  {
		int where = (int) floor(range/step);
		// if (where < -1 || where > cells || incl < 0 || incl >= elements)
		//     cout << " BasePartition: incl= " << incl << " location= "
		//          << where << " els= " << elements << " cells= "
		//          << cells << endl;
		if (where < 0) where= 0;
		else if (where >= cells) where= cells-1;
		next [ incl ] = cell [ where ];
		cell [ where ] = incl;
		return;
	};
	
	int first(const int cl) const  {  return cell [ cl ];  };
	int tail(const int elt) const  {  return next [ elt ];  };
};

/**
 PartitionP
 */
class PartitionP : public BasePartition  {
	private :
	int *       cellIndex;
	int *       previous;
	public :
	PartitionP(const int els= 0, const int cls= 0, const double range= 0);
	~PartitionP();
	void alloc(const int els, const int cls, const double range);
	
	void include(const int incl);
	void initIx(const int incl, const double range)  {
		int cl= (int) floor(range / step);
		// if (cl < -1 || cl > cells || incl < 0 || incl >= elements)
		//     cout << "PartitionP: incl= " << incl << " at " << cl << endl;
		if (cl < 0) cl= 0;
		else if (cl >= cells) cl= cells-1;
		cellIndex[ incl ] = cl;
		return;
	};
	int inTheRange(const double range) const  
	{
		if (range < 0 || range/step > cells) return -1;
		int where= (int) floor(range / step);
		if (where < 0) where= 0;
		else if (where >= cells) --where;
		return where;
	}
	void remove(const int del);
	void cleanup(const BasePartition &p, const int cl)  {
		for (int cnt= p.first(cl); cnt != GdaConst::EMPTY; cnt= p.tail(cnt))
			remove(cnt);
	}
};

class PolygonPartition 
{
	protected :
	Shapefile::PolygonContents     *poly;	
	
	BasePartition       pX;
	PartitionP          pY;
	int *               nbrPoints;
	
	int prev(const int pt) const  
	{
		int ix= nbrPoints[pt];
		return (ix >= 0) ? pt-1 : -ix;
	}
	int succ(const int pt) const  
	{
		int ix= nbrPoints[pt];
		return (ix >= 0) ? ix : pt+1;
	}
	
	public :	
	int                 NumPoints;
	int                 NumParts;
	
	PolygonPartition(Shapefile::PolygonContents* _poly)
	: pX(), pY(), nbrPoints(NULL) {
		poly = _poly;
		NumPoints = poly->num_points;
		NumParts = poly->num_parts;
	}
	~PolygonPartition();
	
	Shapefile::Point* GetPoint(const int i){ return &poly->points[i];}
	int GetPart(int i){ return (int)poly->parts[i]; }
	double GetMinX(){ return (double)poly->box[0]; }
	double GetMinY(){ return (double)poly->box[1]; }
	double GetMaxX(){ return (double)poly->box[2]; }
	double GetMaxY(){ return (double)poly->box[3]; }
	
	int  MakePartition(int mX= 0, int mY= 0);
	void MakeSmallPartition(const int mX, const double Start,
													const double Stop);
	void MakeNeighbors();
	bool edge(PolygonPartition &p, const int host, const int guest, double precision_threshold);
	int sweep(PolygonPartition & guest, bool is_queen,
						double precision_threshold=0.0);
};





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
				GDA_SWAP(p[0], p[7], t);
				GDA_SWAP(p[1], p[6], t);
				GDA_SWAP(p[2], p[5], t);
				GDA_SWAP(p[3], p[4], t);
				s.write(&p[0], sizeof(double) * 1);
				memcpy(&p[0], &(Points[curr].y), sizeof(double));
				GDA_SWAP(p[0], p[7], t);
				GDA_SWAP(p[1], p[6], t);
				GDA_SWAP(p[2], p[5], t);
				GDA_SWAP(p[3], p[4], t);
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
  GDA_SWAP(q[0], q[3], t);
  GDA_SWAP(q[1], q[2], t);
  memcpy(&NumPoints, q, sizeof(long));
  #else
  s >> NumPoints;
  #endif
  Points= new BasePoint[NumPoints];
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
    double m1, m2;
    s.read((char *)p, sizeof(double) * 2);
    GDA_SWAP(p[0], p[7], t);
    GDA_SWAP(p[1], p[6], t);
    GDA_SWAP(p[2], p[5], t);
    GDA_SWAP(p[3], p[4], t);
    memcpy(&m1, &p[0], sizeof(double));
    GDA_SWAP(p[8], p[15], t);
    GDA_SWAP(p[9], p[14], t);
    GDA_SWAP(p[10], p[13], t);
    GDA_SWAP(p[11], p[12], t);
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
  GDA_SWAP(r[0], r[7], t);
  GDA_SWAP(r[1], r[6], t);
  GDA_SWAP(r[2], r[5], t);
  GDA_SWAP(r[3], r[4], t);
  memcpy(&m1, &r[0], sizeof(double));
  GDA_SWAP(r[8], r[15], t);
  GDA_SWAP(r[9], r[14], t);
  GDA_SWAP(r[10], r[13], t);
  GDA_SWAP(r[11], r[12], t);
  memcpy(&m2, &r[8], sizeof(double));
  GDA_SWAP(r[16], r[23], t);
  GDA_SWAP(r[17], r[22], t);
  GDA_SWAP(r[18], r[21], t);
  GDA_SWAP(r[19], r[20], t);
  memcpy(&n1, &r[16], sizeof(double));
  GDA_SWAP(r[24], r[31], t);
  GDA_SWAP(r[25], r[30], t);
  GDA_SWAP(r[26], r[29], t);
  GDA_SWAP(r[27], r[28], t);
  memcpy(&n2, &r[24], sizeof(double));
  BasePoint p1(m1, m2);
  BasePoint p2(n1, n2);
  bBox = Box(p1, p2);
  s.read((char *)q, sizeof(long) * 2);
  GDA_SWAP(q[0], q[3], t);
  GDA_SWAP(q[1], q[2], t);
  memcpy(&NumParts, &q[0], sizeof(long));
  GDA_SWAP(q[4], q[7], t);
  GDA_SWAP(q[5], q[6], t);
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
      GDA_SWAP(w[0], w[3], t);
      GDA_SWAP(w[1], w[2], t);
      memcpy(&Parts[cp], &w[0], sizeof(long));
    #else
      s >> Parts[cp];
    #endif
  }
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
      s.read((char *)p, sizeof(double) * 2);
      GDA_SWAP(p[0], p[7], t);
      GDA_SWAP(p[1], p[6], t);
      GDA_SWAP(p[2], p[5], t);
      GDA_SWAP(p[3], p[4], t);
      memcpy(&m1, &p[0], sizeof(double));
      GDA_SWAP(p[8], p[15], t);
      GDA_SWAP(p[9], p[14], t);
      GDA_SWAP(p[10], p[13], t);
      GDA_SWAP(p[11], p[12], t);
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
  GDA_SWAP(r[0], r[7], t);
  GDA_SWAP(r[1], r[6], t);
  GDA_SWAP(r[2], r[5], t);
  GDA_SWAP(r[3], r[4], t);
  m2 = bBox._min().y;
  memcpy(&r[8], &m2, sizeof(double));
  GDA_SWAP(r[8], r[15], t);
  GDA_SWAP(r[9], r[14], t);
  GDA_SWAP(r[10], r[13], t);
  GDA_SWAP(r[11], r[12], t);
  n1 = bBox._max().x;
  memcpy(&r[16], &n1, sizeof(double));
  GDA_SWAP(r[16], r[23], t);
  GDA_SWAP(r[17], r[22], t);
  GDA_SWAP(r[18], r[21], t);
  GDA_SWAP(r[19], r[20], t);
  n2 = bBox._max().y;
  memcpy(&r[24], &n2, sizeof(double));
  GDA_SWAP(r[24], r[31], t);
  GDA_SWAP(r[25], r[30], t);
  GDA_SWAP(r[26], r[29], t);
  GDA_SWAP(r[27], r[28], t);
  s.write(&r[0], sizeof(double) * 4);
  memcpy(&q[0], &NumParts, sizeof(long));
  GDA_SWAP(q[0], q[3], t);
  GDA_SWAP(q[1], q[2], t);
  s.write(&q[0], sizeof(long));
  memcpy(&q[4], &NumPoints, sizeof(long));
  GDA_SWAP(q[4], q[7], t);
  GDA_SWAP(q[5], q[6], t);
  s.write(&q[4], sizeof(long));
  for (cp = 0; cp < NumParts; cp++) {
      memcpy(&w[0], &Parts[cp], sizeof(long));
      GDA_SWAP(w[0], w[3], t);
      GDA_SWAP(w[1], w[2], t);
      s.write(&w[0], sizeof(long));
  }
  for (cp = 0; cp < NumPoints; cp++) {
      memcpy(&p[0], &(Points[cp].x), sizeof(double));
      GDA_SWAP(p[0], p[7], t);
      GDA_SWAP(p[1], p[6], t);
      GDA_SWAP(p[2], p[5], t);
      GDA_SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[cp].y), sizeof(double));
      GDA_SWAP(p[8], p[15], t);
      GDA_SWAP(p[9], p[14], t);
      GDA_SWAP(p[10], p[13], t);
      GDA_SWAP(p[11], p[12], t);
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
		for (int cnt= 0; cnt < cells; ++cnt) cell [ cnt ] = GdaConst::EMPTY;
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
	if (old != GdaConst::EMPTY)
		previous [ old ] = incl;
	next [ incl ] = old;    // OLD becomes the 2nd element in the list
	previous [ incl ] = GdaConst::EMPTY;       // there are no elements prior to incl
	return;
}

/*
 PartitionP
 */
void PartitionP::remove(const int del)  {
	int   thePrevious= previous[ del ], theNext= next[ del ];
	if ( thePrevious == GdaConst::EMPTY )                // this is the 1st element in the list
		cell [ cellIndex[del] ] = theNext;
    else
		next[ thePrevious ] = theNext;
	if ( theNext != GdaConst::EMPTY )                   // this is not the last element in thelist
		previous[ theNext ] = thePrevious;
	previous[ del ] = next [ del ] = GdaConst::EMPTY;  // probably this is not necessary
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
bool PolygonPartition::edge(PolygonPartition &p, const int host,
							const int guest, double precision_threshold)
{
	using namespace Shapefile;

	
	Point* guestPrev = p.GetPoint(p.prev(guest));
	//BasePoint hostPoint = Points[ succ(host) ];
	Point* hostPoint = this->GetPoint(succ(host));
	
	if (hostPoint->equals(guestPrev, precision_threshold)) return true;
	
	//BasePoint guestSucc= p.Points[ p.succ(guest) ];
	Point* guestSucc= p.GetPoint(p.succ(guest));
	if (hostPoint->equals( guestSucc, precision_threshold) ) return true;
	
	hostPoint= this->GetPoint( prev(host) );
	
	if (hostPoint->equals( guestSucc, precision_threshold )) return true;
	
	if (hostPoint->equals( guestPrev, precision_threshold )) return true;
	
	return false;
}

/*
 PolygonPartition
 */
int PolygonPartition::MakePartition(int mX, int mY)  {
	if (mX == 0) mX = NumPoints/4 + 2;
	if (mY == 0) mY = (int)(sqrt((long double)NumPoints) + 2);
	pX.alloc(NumPoints, mX, GetMaxX() - GetMinX());// bBox._max().x - bBox._min().x);
	pY.alloc(NumPoints, mY, GetMaxY() - GetMinY());//bBox._max().y - bBox._min().y);
	double xStart= GetMinX(), yStart= GetMinY();
	for (int cnt= 0; cnt < NumPoints; ++cnt)  {
		pX.include(cnt, GetPoint(cnt)->x - xStart);
		pY.initIx(cnt, GetPoint(cnt)->y - yStart);
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
		last= (part == NumParts) ? NumPoints : GetPart(part);
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
		Shapefile::Point* pt= GetPoint(cnt);
		if (pt->x >= Start && pt->x <= Stop) pX.include(cnt, pt->x - Start);
	}
	MakeNeighbors();
}

/*
 PolygonPartition::sweep
 Determines if two polygons are neighbors. The host is assumed to be
 partitioned.
 Uses two criteria to establish neighborhood:
 is_queen == true then: common point, else: common boundary
 */
int PolygonPartition::sweep(PolygonPartition & guest, bool is_queen,
                            double precision_threshold)
{
	int       host, dot, cly, cell;
	double    yStart= GetMinY(), yStop= GetMaxY();
	Shapefile::Point* pt;
	guest.MakeSmallPartition(pX.Cells(), GetMinX(), GetMaxX());
	for (cell= 0; cell < pX.Cells(); ++cell) {
		for (host= pX.first(cell); host != GdaConst::EMPTY; host= pX.tail(host))
            pY.include(host);
		for (dot=guest.pX.first(cell); dot != GdaConst::EMPTY; dot=guest.pX.tail(dot))
        {
			pt= guest.GetPoint(dot);
			cly= pY.inTheRange(pt->y - yStart);
			if (cly != -1) {
				for (host= pY.first(cly); host != GdaConst::EMPTY;
					 host= pY.tail(host))
                {
					if (pt->equals( GetPoint(host), precision_threshold) )
                    {
						if (is_queen || edge(guest, host, dot, precision_threshold)) {
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
      GDA_SWAP(p[0], p[7], t);
      GDA_SWAP(p[1], p[6], t);
      GDA_SWAP(p[2], p[5], t);
      GDA_SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[point].y), sizeof(double));
      GDA_SWAP(p[8], p[15], t);
      GDA_SWAP(p[9], p[14], t);
      GDA_SWAP(p[10], p[13], t);
      GDA_SWAP(p[11], p[12], t);
      s.write(&p[8], sizeof(double));
      s << endl;
    #else
    s << Points[point] << endl;
    #endif
  }
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
  GDA_SWAP(r[0], r[7], t);
  GDA_SWAP(r[1], r[6], t);
  GDA_SWAP(r[2], r[5], t);
  GDA_SWAP(r[3], r[4], t);
  memcpy(&m1, &r[0], sizeof(double));
  GDA_SWAP(r[8], r[15], t);
  GDA_SWAP(r[9], r[14], t);
  GDA_SWAP(r[10], r[13], t);
  GDA_SWAP(r[11], r[12], t);
  memcpy(&m2, &r[8], sizeof(double));
  GDA_SWAP(r[16], r[23], t);
  GDA_SWAP(r[17], r[22], t);
  GDA_SWAP(r[18], r[21], t);
  GDA_SWAP(r[19], r[20], t);
  memcpy(&n1, &r[16], sizeof(double));
  GDA_SWAP(r[24], r[31], t);
  GDA_SWAP(r[25], r[30], t);
  GDA_SWAP(r[26], r[29], t);
  GDA_SWAP(r[27], r[28], t);
  memcpy(&n2, &r[24], sizeof(double));
  BasePoint o1(m1, m2);
  BasePoint o2(n1, n2);
  bBox = Box(o1, o2);
  GDA_SWAP(p[0], p[3], t);
  GDA_SWAP(p[1], p[2], t);
  memcpy(&NumPoints, &p[0], sizeof(long));
  #else
  s >> bBox >> NumPoints;
  #endif

  if (Points) delete [] Points;
  Points= new BasePoint[NumPoints];
  for (cp= 0; cp < NumPoints; cp++) {
    #ifdef WORDS_BIGENDIAN
      s.read((char *)q, sizeof(double) * 2);
      GDA_SWAP(q[0], q[7], t);
      GDA_SWAP(q[1], q[6], t);
      GDA_SWAP(q[2], q[5], t);
      GDA_SWAP(q[3], q[4], t);
      memcpy(&m1, &q[0], t);
      GDA_SWAP(q[8], q[15], t);
      GDA_SWAP(q[9], q[14], t);
      GDA_SWAP(q[10], q[13], t);
      GDA_SWAP(q[11], q[12], t);
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
  GDA_SWAP(r[0], r[7], t);
  GDA_SWAP(r[1], r[6], t);
  GDA_SWAP(r[2], r[5], t);
  GDA_SWAP(r[3], r[4], t);
  m2 = bBox._min().y;
  memcpy(&r[8], &m2, sizeof(double));
  GDA_SWAP(r[8], r[15], t);
  GDA_SWAP(r[9], r[14], t);
  GDA_SWAP(r[10], r[13], t);
  GDA_SWAP(r[11], r[12], t);
  n1 = bBox._max().x;
  memcpy(&r[16], &n1, sizeof(double));
  GDA_SWAP(r[16], r[23], t);
  GDA_SWAP(r[17], r[22], t);
  GDA_SWAP(r[18], r[21], t);
  GDA_SWAP(r[19], r[20], t);
  n2 = bBox._max().y;
  memcpy(&r[24], &n2, sizeof(double));
  GDA_SWAP(r[24], r[31], t);
  GDA_SWAP(r[25], r[30], t);
  GDA_SWAP(r[26], r[29], t);
  GDA_SWAP(r[27], r[28], t);
  s.write(&r[0], sizeof(double) * 4);
  memcpy(&q[0], &NumPoints, sizeof(long));
  GDA_SWAP(q[0], q[3], t);
  GDA_SWAP(q[1], q[2], t);
  s.write(&q[0], sizeof(long));
  for (cp = 0; cp < NumPoints; cp++) {
      memcpy(&p[0], &(Points[cp].x), sizeof(double));
      GDA_SWAP(p[0], p[7], t);
      GDA_SWAP(p[1], p[6], t);
      GDA_SWAP(p[2], p[5], t);
      GDA_SWAP(p[3], p[4], t);
      s.write(&p[0], sizeof(double));
      memcpy(&p[8], &(Points[cp].y), sizeof(double));
      GDA_SWAP(p[8], p[15], t);
      GDA_SWAP(p[9], p[14], t);
      GDA_SWAP(p[10], p[13], t);
      GDA_SWAP(p[11], p[12], t);
      s.write(&p[8], sizeof(double));
  }
  #else
  s << bBox << NumPoints;
  for (cp= 0; cp < NumPoints; cp++) s << Points[cp];
  #endif
  return s;
}

typedef struct Ref  {
	int next, prev;
	Ref(const int nxt= -1, const int prv= -1) : next(nxt), prev(prv) {};
} RefStruct;

typedef RefStruct* RefPtr;

/*
 PartitionM
 */
class PartitionM  {
	private :
	double      step;
	int         elements, cells;
	int *       cell;
	int *       cellIndex;
	int *       lastIndex;
	RefPtr *    Refs;
	public :
	PartitionM(const int els, const int cls, const double range);
	virtual ~PartitionM();
	
	void include(const int incl);
	void remove(const int del);
	void initIx(const int incl, const double lwr, const double upr);
	int lowest (const int el) const  {  return cellIndex [ el ];  };
	int upmost(const int el) const  {  return lastIndex [ el ];  };
	int first(const int cl) const  {  return cell[ cl ];  };
	int tail(const int elt, const int cl) const  {
		return Refs[elt][cl-cellIndex[elt]].next;
	};
	int Sum() const;
};

/*
 PartitionM
 Constructor to initialize the partition
 */
PartitionM::PartitionM(const int els, const int cls, const double range) :
elements(els), cells(cls)  {
	cell= new int[ cells ];
	cellIndex= new int [ elements ];
	lastIndex= new int [ elements ];
	int cnt;
	for (cnt= 0; cnt < cells; ++cnt)
		cell [ cnt ] = GdaConst::EMPTY;
	Refs= new RefPtr [ elements ];
	for (cnt= 0; cnt < elements; ++cnt)
		Refs[cnt]= NULL;
	step= range / cells;
	return;
}

PartitionM::~PartitionM()  {
	if (cell)  {
		delete [] cell;
		cell= NULL;
	};
	if (cellIndex)  {
		delete [] cellIndex;
		cellIndex= NULL;
	};
	if (lastIndex)  {
		delete [] lastIndex;
		lastIndex= NULL;
	};
	if (Refs)  {
		for (int ref= 0; ref < elements; ++ref)
			if (Refs[ref]) delete [] Refs[ref];
		delete [] Refs;
		Refs= NULL;
	};
	cells= 0;
	elements= 0;
	return;
}

int PartitionM::Sum() const {
	int sum= 0;
	for (int cnt= 0; cnt < elements; ++cnt)
		sum += (lastIndex[cnt] - cellIndex[cnt] + 1);
	return sum;
};

void PartitionM::initIx(const int incl, const double lwr, const double upr)  {
    int lower= (int)floor(lwr/step);
    int upper= (int)floor(upr/step);
    
    
    if (lower < 0) {
        lower= 0;
    } else if (lower >= cells) {
        lower= cells-1;
    }
    
    if (upper >= cells) {
        upper= cells-1;
    }
    else if (upper < 0) {
        upper= 0;
    }
    
    if (lower < 0 || upper > cells || incl < 0 || incl >= elements)
    {
        //     cout << "PartM: incl= " << incl << " l= " << lwr << "  " << upr;
        exit(1);
    }

    
	cellIndex [ incl ] = lower;
	lastIndex [ incl ] = upper;
	return;
}

/*
 PartitionM::include
 Include element incl with duration [lower, upper] into the partition.
 */

void PartitionM::include(const int incl)  {
	int cnt, lower= cellIndex [ incl ], upper= lastIndex [ incl ];
	RefPtr rptr= new RefStruct[ upper - lower + 1 ];
	Refs[incl]= rptr;
	for (cnt= upper-lower; cnt >= 0; --cnt)
		rptr[cnt]= Ref();
	for (cnt= lower; cnt <= upper; ++cnt)  {
		int old= cell [ cnt ];            // first element in the cell
		cell [ cnt ] = incl;              // new first element in the cell
		if (old != GdaConst::EMPTY)  {  // the cell was not empty
			rptr[cnt-lower].next = old; // OLD is the next element after incl in the list
			Refs[old][cnt-cellIndex[old]].prev= incl;    // incl is preceeding OLD in the list
		};
	};
	return;
};

/*
 PartitionM::remove
 Remove an element del from the partition.
 */
void PartitionM::remove(const int del)  {
	int   lower= cellIndex[ del ], upper= lastIndex [ del ], cnt;
	for (cnt= lower; cnt <= upper; ++cnt)  {
		RefStruct  cRef= Refs[del][cnt-lower];
		if (cRef.prev < 0)       // this is the first element in the list
			cell[ cnt ]= cRef.next;
		else
			Refs[cRef.prev][ cnt-cellIndex[cRef.prev] ].next= cRef.next;
		if (cRef.next != GdaConst::EMPTY)  // this is not the last element in the list
			Refs[cRef.next][cnt-cellIndex[cRef.next]].prev= cRef.prev;
	};
	delete [] Refs[del];
	Refs[del]= NULL;
	return;
}



// # of records in the Shapefile == dimesion of the weights matrix
static long         gRecords= 0;
// locations of the polygon records in the shp file
static long *       gOffset= NULL;
// bounding boxes for each polygon
static Box *        gBox= NULL;
// bounding box for the entire map
static Box          gBigBox;
// partition constructed on lower(x) and upper(x) for each polygon
static BasePartition  gMinX, gMaxX;
// partition constructed on y for each polygon
static PartitionM * gY;


GalElement* MakeContiguity(Shapefile::Main& main, bool is_queen,
                           double precision_threshold=0.0)
{
	using namespace Shapefile;
	int curr;
	GalElement * gl= new GalElement [ gRecords ];
	
	if (!gl) return NULL;
	GeoDaSet   Neighbors(gRecords), Related(gRecords);
	//  cout << "total steps= " << gMinX.Cells() << endl;
	for (int step= 0; step < gMinX.Cells(); ++step) {
		// include all elements from xmin[step]
		for (curr= gMinX.first(step); curr != GdaConst::EMPTY;
             curr= gMinX.tail(curr))
        {
            gY->include(curr);
        }
		
		// test each element in xmax[step]
		for (curr= gMaxX.first(step); curr != GdaConst::EMPTY;
             curr= gMaxX.tail(curr))
        {
            RecordContents* rec = main.records[curr].contents_p;
			PolygonContents* ply = dynamic_cast<PolygonContents*> (rec);
			PolygonPartition testPoly(ply);
			testPoly.MakePartition();
			
			// form a list of neighbors
			for (int cell=gY->lowest(curr); cell <= gY->upmost(curr); ++cell) {
				int potential = gY->first( cell );
				while (potential != GdaConst::EMPTY) {
					if (potential != curr) Neighbors.Push( potential );
					potential = gY->tail(potential, cell);
				}
			}
			
			// test each potential neighbor
			for (int nbr = Neighbors.Pop(); nbr != GdaConst::EMPTY;
					 nbr = Neighbors.Pop()) {
                RecordContents* nbr_rec = main.records[nbr].contents_p;
                PolygonContents* nbr_ply = dynamic_cast<PolygonContents*>(nbr_rec);
                
				if (ply->intersect(nbr_ply)) {
					
					PolygonPartition nbrPoly(nbr_ply);
					//shp.seekg(gOffset[nbr]+12, ios::beg);
					//nbrPoly.ReadShape(shp);
					
					if (curr == 0 && nbr == 0) {
						
					}
					// run sweep with testPoly as a host and nbrPoly as a guest
					int related = testPoly.sweep(nbrPoly, is_queen, precision_threshold);
					if (related) Related.Push(nbr);
				}
			}
			
			
			if (size_t sz = Related.Size()) {
				gl[curr].SetSizeNbrs(sz);
                for (size_t i=0; i<sz; ++i) {
                    gl[curr].SetNbr(i, Related.Pop());
                }
			}
			
			gY->remove(curr);       // remove from the partition
		}
	}
	
	return gl;
}


void MakeFull(GalElement* W, size_t obs)
{
	using namespace std;
	vector<set<long> > G(obs);
	for (size_t i=0; i<obs; ++i) {
		for (size_t j=0, sz=W[i].Size(); j<sz; ++j) {
			G[i].insert(W[i][j]);
			G[W[i][j]].insert(i);
		}
	}
	for (size_t i=0; i<obs; ++i) {
		if (W[i].Size() == G[i].size()) continue;
		W[i].SetSizeNbrs(G[i].size());
		size_t cnt = 0;
		for (set<long>::iterator it=G[i].begin(); it!=G[i].end(); ++it) {
			W[i].SetNbr(cnt++, *it);
		}
		W[i].SortNbrs();
	}
}



GalElement* PolysToContigWeights(Shapefile::Main& main, bool is_queen,
                                 double precision_threshold)
{
	using namespace Shapefile;
	
	gRecords = main.records.size();
	double shp_min_x = (double)main.header.bbox_x_min;
	double shp_max_x = (double)main.header.bbox_x_max;
	double shp_min_y = (double)main.header.bbox_y_min;
	double shp_max_y = (double)main.header.bbox_y_max;
	double shp_x_len = shp_max_x - shp_min_x;
	double shp_y_len = shp_max_y - shp_min_y;
	
	long gx, gy, cnt, total=0;
	gx= gRecords / 8 + 2;
	
	gMinX.alloc(gRecords, gx, shp_x_len );
	gMaxX.alloc(gRecords, gx, shp_x_len );
	
	for (cnt= 0; cnt < gRecords; ++cnt) {
        RecordContents* rec = main.records[cnt].contents_p;
		PolygonContents* ply = dynamic_cast<PolygonContents*>(rec);
	
		gMinX.include( cnt, ply->box[0] - shp_min_x );
		gMaxX.include( cnt, ply->box[2] - shp_min_x );
	}
	
	gy= (int)(sqrt((long double)gRecords) + 2);
	do {
		gY= new PartitionM(gRecords, gy, shp_y_len );
		for (cnt= 0; cnt < gRecords; ++cnt) {
            RecordContents* rec = main.records[cnt].contents_p;
			PolygonContents* ply = dynamic_cast<PolygonContents*>(rec);
            double lwr = ply->box[1] - shp_min_y;
            double upr = ply->box[3] - shp_min_y;
			gY->initIx(cnt, lwr, upr);
		}
		total= gY->Sum();
		if (total > gRecords * 8) {
			delete gY;
			gy = gy/2 + 1;
			total= 0;
		}
	} while ( total == 0);
	
	GalElement * gl= MakeContiguity(main, is_queen, precision_threshold);
	
	if (gY) delete gY; gY = 0;
	if (gOffset) delete [] gOffset; gOffset = 0;
	if (gBox) delete [] gBox; gBox = 0;
	
	MakeFull(gl, gRecords);
	return gl;
}








