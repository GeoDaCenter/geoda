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

/*
 Functions to create spatial weights (.gwt) from any Shapefile.
 */

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "shp.h"
#include "shp2gwt.h"
#include "shp2cnt.h"
#include <math.h>
#include <stdio.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <time.h>
#include "../GenUtils.h"
#include "../GeoDaConst.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "ShapeFileTriplet.h"
#include "ShapeFileTypes.h"
#include "DbfFile.h"

const long HUGE_NUMBER = 99999999;

class PartitionGWT
{
	protected :
    int         elements, cells;
    int *       cell;
    int *       next;
    double      step;
	public :
    PartitionGWT(const int els= 0, const int cls= 0, const double range= 0);
	virtual ~PartitionGWT();
    void  alloc(const int els, const int cls, const double range);
    int         Cells() const  {  return cells;  };
    double      Step()  const  {  return step;  };
    int         Where(const double range)  {
        int where= (int)floor(range/step);
        if (where < 0) where= 0;
		else if (where >= cells) where= cells-1;
        return where;
    };
	
    void include(const int incl, const double range)  {
        int     where= Where( range );
        next [ incl ] = cell [ where ];
        cell [ where ] = incl;
        return;
    };
	
    int first(const int cl) const  {  return cell [ cl ];  };
    int tail(const int elt) const  {  return next [ elt ];  };
    void reset()  {
		
        for (int cnt= 0; cnt < cells; ++cnt)
			cell [ cnt ] = GeoDaConst::EMPTY;
        return;
    };
    void reset(const double range)  {
        cell [ Where(range) ] = GeoDaConst::EMPTY;
        return;
    };
};


typedef PartitionGWT * PartitionPtr;

void PartitionGWT::alloc(const int els, const int cls, const double range)  {
	
	elements= els;
	cells= cls;
	step= range / cls;
	cell= new int [ cells ];
	next= new int [ elements ];
	if (cell && next)
		reset();
    else elements= cells= 0;
	return;
	
}


PartitionGWT::PartitionGWT(const int els, const int cls, const double range)
: elements(els), cells(cls),
cell(NULL), next(NULL)  {
	if (elements > 0) alloc(els, cls, range);
	return;
}


PartitionGWT::~PartitionGWT()  
{
	if (cell != NULL) delete [] cell;
	cell = NULL;
	if (next != NULL) delete [] next;
	next = NULL;
	elements = cells = 0;
	return;
}


class Location  
{
	private :
	long    points;
	BasePoint * centroid;
	double threshold, distanceSquare;
	Box    BoundingBox;
	long   current;
	BasePoint  currPoint;
	int		method;
	public :
	double currDistance;
	Location(BasePoint * pts, const double distance, const long num, int mt);
	virtual ~Location();
	bool good()  const  {  return points > 0;  };
	long   x_cells()  const  {  return  (long)floor( BoundingBox.range_x(method)/threshold ) + 1;  };
	long   y_cells()  const  {  return (long)floor( BoundingBox.range_y(method)/threshold ) + 1;  };
	double x_range(int method)  const  {  return  BoundingBox.range_x(method);  };
	double y_range(int method)  const  {  return  BoundingBox.range_y(method);  };
	double x_range(const long elt, int method)  const {
		if (method==1) // 1:Eucledian Distance 2:Arc Distance
			return centroid[elt].x - BoundingBox._min().x;
		else
			return GenGeomAlgs::ComputeArcDist(BoundingBox._min().x,BoundingBox._min().y,centroid[elt].x,BoundingBox._min().y);
	};
	double y_range(const long elt, int method)  const {
		if (method==1)
			return centroid[elt].y - BoundingBox._min().y;
		else
			return GenGeomAlgs::ComputeArcDist(BoundingBox._min().x,BoundingBox._min().y,BoundingBox._min().x,centroid[elt].y);
	};
	void setCurrent(const long cp) {
        current= cp;
        currPoint= centroid[cp];
        return;
	};
	bool InTheSemiCircle(const long pt);
	bool InTheCircle(const long pt);
	void CheckParticle(PartitionGWT * Y, const long cell, long &BufferSize,
					   GwtNeighbor * buffer);
};

Location::Location(BasePoint * pts, 
				   const double distance, 
				   const long num, int mt) : 
centroid(pts),
threshold(distance), 
points(num),
method(mt)
{
    if (threshold < 0) 
		threshold = - threshold;
	
    distanceSquare= threshold * threshold;
    if (centroid == NULL)  
	{ 
		points= 0; 
		return;  
	};
    BoundingBox= Box(centroid[0]);
    for(long cnt= 1; cnt < points; ++cnt) BoundingBox += centroid[ cnt ];
}

Location::~Location()  {
	delete [] centroid;
	centroid = NULL;
	points = 0;
}

inline bool Location::InTheCircle(const long pt)  
{
    BasePoint   work= centroid[pt];
	double d2;
	if (method == 2)
	{
		d2 = GenGeomAlgs::ComputeArcDist(currPoint.x,currPoint.y,work.x,work.y);
		d2 = d2 * d2;
	}
	else {
		double dx = work.x-currPoint.x;
		double dy = work.y-currPoint.y;
		d2= dx*dx + dy*dy;
	}
	
    bool test= (d2 <= distanceSquare);
    if (test) currDistance= sqrt( d2 );
    return test;
}



inline bool Location::InTheSemiCircle(const long pt)  
{
    BasePoint   work= centroid[pt];
    if (work.x > currPoint.x) return false;
    if (work.x == currPoint.x)
		if (work.y > currPoint.y) return false;
		else if (work.y == currPoint.y)  // identically located points
		{
			currDistance= threshold * 0.000001;  // set 'small' distance
			return (pt < current);               // use id to introduce assymetry
		}
	
    double d2=-1;
	if (method == 2) 
	{
		d2 = GenGeomAlgs::ComputeArcDist(currPoint.x,currPoint.y,work.x,work.y);
		d2 = d2 * d2;
	}
	else {
		double dx = work.x-currPoint.x;
		double dy = work.y-currPoint.y;
		d2= dx*dx + dy*dy;
	}	
	
    bool test= (d2 <= distanceSquare);
    if (test) currDistance= sqrt( d2 );
    return test;
}

void Location::CheckParticle(PartitionGWT * Y, 
							 const long cell, 
							 long &BufferSize,
							 GwtNeighbor * buffer)  
{
    for ( long cnt= Y->first(cell); cnt != GeoDaConst::EMPTY; cnt= Y->tail(cnt)) {
		if (InTheSemiCircle( cnt)) {
			buffer[ BufferSize++ ] = GwtNeighbor(cnt, currDistance);
		}
	}
}

double ComputeMaxDistance(const std::vector<double>& x,
						  const std::vector<double>& y, int method) 
{
	int Records = x.size();
	BasePoint minmax[2];
	
	BasePoint *centroid= new BasePoint [ Records ];
	int rec = 0;
	for (rec =0; rec < Records; rec++) centroid[rec].setXY(x.at(rec),y.at(rec));
	
	minmax[0].setXY(centroid[0].x, centroid[0].y);
	minmax[1].setXY(centroid[0].x, centroid[0].y);
	
	for (rec = 1; rec < Records; ++rec) {
		if (centroid[rec].x > minmax[1].x) 
			minmax[1].setXY(centroid[rec].x, minmax[1].y);
		else if (centroid[rec].x < minmax[0].x) 
			minmax[0].setXY(centroid[rec].x, minmax[0].y);
		if (centroid[rec].y > minmax[1].y) 
			minmax[1].setXY(minmax[1].x, centroid[rec].y);
		else if (centroid[rec].y < minmax[0].y) 
			minmax[0].setXY(minmax[0].x, centroid[rec].y);
	}
	
	double dist = sqrt(geoda_sqr(minmax[1].x - minmax[0].x)
					   + geoda_sqr(minmax[1].y - minmax[0].y));
	if (method == 2) {
		// Arc Distance
		dist = GenGeomAlgs::ComputeArcDist(minmax[0].x, minmax[0].y,
										   minmax[1].x, minmax[1].y);
	}
	return dist;
}

DBF_descr* CreateDBFDesc4Grid(wxString ID)
{
	
	DBF_descr *dbfdesc;
	if (ID==wxEmptyString)
	{
		dbfdesc			= new DBF_descr [3];
		dbfdesc[0]	= new DBF_field("POLYID", 'N',8,0);
		dbfdesc[1]	= new DBF_field("AREA", 'N',16,6);
		dbfdesc[2]	= new DBF_field("PERIMETER", 'N',16,6);
	}
	else
	{
		dbfdesc			= new DBF_descr [4];
		dbfdesc[0]	= new DBF_field(ID.wx_str(), 'N',8,0);
		dbfdesc[1]	= new DBF_field("AREA", 'N',16,6);
		dbfdesc[2]	= new DBF_field("PERIMETER", 'N',16,6);
		dbfdesc[3]	= new DBF_field("RECORD_ID", 'N',8,0);
	}
	
	return dbfdesc;
	
}

bool CreateSHPfromBoundary(wxString ifl, wxString otfl)
{
	// Open the Boundary file
	ifstream ias;
	ias.open(ifl.mb_str());
	int nRows;
	char name[1000];
	ias.getline(name,100);
	wxString tok = wxString(name, wxConvUTF8);
	wxString ID_name=wxEmptyString;
	
	int pos = tok.Find(',');
	if( pos >= 0)
	{
		//nRows = wxAtoi(tok.Left(pos));
		long temp;
		tok.Left(pos).ToCLong(&temp);
		nRows = (int) temp;
		ID_name = tok.Right(tok.Length()-pos-1);
	}
	else
	{
		wxMessageBox("Wrong format!");
		return false;
	}
	
	ID_name.Trim(false);
	ID_name.Trim(true);
	
	if (nRows < 1 || ID_name == wxEmptyString)
	{
		wxMessageBox("Wrong format!");
		return false;
	}
	
	double const EPS = 1e-10;
	BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
	BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
	Box fBox(a,b);
	
	
	// Define the output file
	DBF_descr *dbfdesc = CreateDBFDesc4Grid(ID_name);
	oShapeFileTriplet Triple(otfl,fBox, dbfdesc, 4,ShapeFileTypes::POLYGON);
	AbstractShape* shape = new PolygonShape;
	if (shape == NULL )
		return false;
	int nParts=1;
	int* Parts;
	Parts = new int[nParts];
	Parts[0]=0;
	
	// Assuming max # of points in a polygon is 10000
	int const MAX_POINT = 10000;
	std::vector<BasePoint> Points(MAX_POINT);
	double* x = new double[MAX_POINT];
	double* y = new double[MAX_POINT];
	
	long polyid = 0, ID;
	int nPoint;
	for (long row = nRows; row >= 1; row--) 
	{
		ias.getline(name,100);
		sscanf(name,"%ld, %d", &ID, &nPoint);
		if (nPoint < 1)
		{
			wxString xx= wxString::Format("at polygon-%d",ID);
			wxMessageBox(wxT("Fail in reading the Boundary file: "+xx));
			delete [] x;
			x = NULL;
			delete [] y;
			y = NULL;
			delete [] Parts;
			Parts = NULL;
			delete shape;
			shape = NULL;
			return false;
		}
		
		polyid += 1;
		
		
		long pt = 0;
		for (pt=0; pt < nPoint; pt++) 
		{
			double xt,yt;
			
			ias.getline(name, 100);
			tok = wxString(name,wxConvUTF8);
			//tok = wxString::Format("%100s",name);
			int pos = tok.Find(',');
			if( pos >= 0)
			{
				//xt = wxAtof(tok.Left(pos));
				tok.Left(pos).ToCDouble(&xt);
				tok = tok.Right(tok.Length()-pos-1);
				//yt = wxAtof(tok);
				tok.ToCDouble(&yt);
			}
			else
			{
				wxMessageBox("Wrong format!");
				return false;
			}
			x[pt] = xt; y[pt]= yt;
			Points.at(pt).setXY(x[pt],y[pt]);
		}
		Points.at(pt).setXY(x[0],y[0]);
		
		const double Area  = GenGeomAlgs::ComputeArea2D(nPoint,x,y); 
		const double Perim = GenGeomAlgs::ComputePerimeter2D(nPoint,x,y); 
		
		Box rBox(a,b);
		rBox = shape->SetData(nParts, Parts, nPoint+1, Points);
		fBox += rBox;
		Triple << *shape;
		Triple << ID;
		Triple << Area;
		Triple << Perim;
		Triple << polyid;
		
	}
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;
	delete [] Parts;
	Parts = NULL;
	delete shape;
	shape = NULL;
	return true;
}

bool CreateGridShapeFile(wxString otfl, int nRows, int nCols,
						 double *xg, double *yg, myBox myfBox)
{
	
	BasePoint p1(myfBox.p1.x,myfBox.p1.y);
	BasePoint p2(myfBox.p2.x,myfBox.p2.y);
	Box xoBox(p1,p2);
	double const EPS = 1e-10;
	
	int const nPolygon = nRows * nCols;
	
	DBF_descr *dbfdesc = CreateDBFDesc4Grid(wxEmptyString);
	oShapeFileTriplet Triple(otfl,xoBox, dbfdesc, 3,ShapeFileTypes::POLYGON);
	
	AbstractShape* shape = new PolygonShape;
	
	int nParts=1;
	int* Parts;
	Parts = new int[nParts];
	Parts[0]=0;
	
	BasePoint a(HUGE_NUMBER,HUGE_NUMBER);
	BasePoint b(-HUGE_NUMBER,-HUGE_NUMBER);
	Box fBox(a,b);
	
	std::vector<BasePoint> Points(6);
	
	if (shape == NULL)
		return false;
	
	long const nPoint = 4;
	double *x = new double[nPoint+2];
	double *y = new double[nPoint+2];
	
	long polyid = 0;
	for (long row = nRows; row >= 1; row--) 
	{
		
		for (long col=1; col <= nCols; col++) 
		{
			polyid += 1;
			
			x[0] = xg[col-1];
			y[0] = yg[row];
			x[1] = xg[col];
			y[1] = yg[row];
			x[2] = xg[col];
			y[2] = yg[row-1];
			x[3] = xg[col-1];
			y[3] = yg[row-1];
			Points[0].setXY(x[0],y[0]);
			Points[1].setXY(x[1],y[1]);
			Points[2].setXY(x[2],y[2]);
			Points[3].setXY(x[3],y[3]);
			Points[4].setXY(x[0],y[0]);
			
			const double Area  = GenGeomAlgs::ComputeArea2D(nPoint,x,y); 
			const double Perim = GenGeomAlgs::ComputePerimeter2D(nPoint,x,y); 
			
			Box rBox(a,b);
			rBox = shape->SetData(nParts, Parts, nPoint+1, Points);
			fBox += rBox;
			Triple << *shape;
			Triple << polyid;
			Triple << Area;
			Triple << Perim;
		}
	}
	
	Triple.SetFileBox(fBox);
	Triple.CloseTriplet();
	delete [] x;
	x = NULL;
	delete [] y;
	y = NULL;
	delete [] Parts;
	Parts = NULL;
	delete shape;
	shape = NULL;
	return true;
}


bool WriteGwt(const GwtElement *g, 
			  const wxString& ifname, 
			  const wxString& ofname, 
			  const wxString& vname,
			  const std::vector<wxInt64>& id_vec,
			  const int degree_flag,
			  bool geodaL)  
{
    if (g == NULL || ifname.IsEmpty() || ofname.IsEmpty() || vname.IsEmpty()
		|| id_vec.size() == 0) {
        return false;
	}
	int Obs = (int) id_vec.size();
	
    wxFileName galfn(ofname);
    galfn.SetExt("gwt");
    wxString gal_ofn(galfn.GetFullPath());
    std::ofstream out(gal_ofn.mb_str(wxConvUTF8), std::ios::out);
    if (!(out.is_open() && out.good())) {
        return false;
    }
	
    int degree_fl = geodaL ? 0 : degree_flag ;
    wxFileName local(ifname);
    std::string local_name(local.GetName().mb_str(wxConvUTF8));
    out << degree_fl << " " << Obs << " " << local_name;
    out << " " << vname.mb_str() << endl;
    
    for (int i=0; i < Obs; i++) {
        for (long nbr= 0; nbr < g[i].Size(); ++nbr) {
            GwtNeighbor  current= g[i].elt(nbr);
            double w = current.weight;
            out << id_vec[i] << ' ' << id_vec[current.nbx];
			out << ' ' << setprecision(9) << setw(18) << w << '\n';
        }
    }
	
    return true;
}

GwtElement * MakeFullGwt(GwtElement * half, const long dim, int degree,
						 bool standardize)  
{
	long * Count = new long [ dim ], cnt, nbr, Nz= 0;
	for (cnt= 0; cnt < dim; ++cnt)
		Count[cnt] = half[cnt].Size();
	for (cnt= 0; cnt < dim; ++cnt)
		for (nbr= 0; nbr < half[cnt].Size(); ++nbr)
			++Count[ half[cnt].elt(nbr).nbx ];
	GwtElement *tmp = new GwtElement [dim], * full= new GwtElement [ dim ];
	bool good= true;
	double min = 1e10, *sum = new double [dim];
	
	for (cnt= 0; cnt < dim; ++cnt)  {
		if (Count[cnt]) {
			good &= full[cnt].alloc( Count[cnt] );
			good &= tmp[cnt].alloc(Count[cnt]);
		}
		Nz += Count[cnt];
		for (nbr = 0; nbr < half[cnt].Size(); nbr++)
		{
			if (min > half[cnt].elt(nbr).weight)
			{
				min = half[cnt].elt(nbr).weight;
			}
		}
	}
	
	if (good)  {
		for (cnt= 0; cnt < dim; ++cnt)
			for (nbr= 0; nbr < half[cnt].Size(); ++nbr)  {
				GwtNeighbor cx= half[cnt].elt(nbr);
				if (degree < 0) cx.weight = pow(cx.weight / min, degree);
				else cx.weight = pow(cx.weight, degree);
				tmp[cnt].Push( cx );
				tmp[cx.nbx].Push( GwtNeighbor(cnt, cx.weight) );
			}
		
		if (standardize) {
			for (cnt = 0; cnt < dim; cnt++)
			{
				sum[cnt] = 0;
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					sum[cnt] += cx.weight * cx.weight;
				}
			}
			
			for (cnt = 0; cnt < dim; cnt++)
			{
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					cx.weight /= sqrt(sum[cnt]);
					full[cnt].Push(cx);
				}
			}
			
		}
		else
		{
			for (cnt = 0; cnt < dim; cnt++)
			{
				for (nbr = 0; nbr < tmp[cnt].Size(); nbr++)
				{
					GwtNeighbor cx = tmp[cnt].elt(nbr);
					full[cnt].Push(cx);
				}
			}
		}
	}  else {
		delete [] full;
		full = NULL;
	}
	delete [] tmp;
	tmp = NULL;
	delete [] Count;
	Count = NULL;
	// cout << " nonzero elements: " << Nz << endl;
	return full;
}

inline void swap(PartitionPtr &x, PartitionPtr &y)  {
	PartitionPtr z= x;
	x= y;
	y= z;
	return;
}

GwtElement* shp2gwt(int Obs, 
					std::vector<double>& x,
					std::vector<double>& y,
					const double threshold, 
					const int degree,
					int	method) // 0: Euclidean dist, 1:Arc
{
	long Records = Obs, cnt;
	
	BasePoint *m_pt = new BasePoint[Obs];
	if (m_pt == NULL) return NULL;
	
	for (int i=0;i < Obs; i++)
		m_pt[i].setXY(x.at(i),y.at(i));
	
	Location Center(m_pt, threshold, Records, method);
	
	if (!Center.good()) return NULL;  
	
	long gx, gy, part, curr;
	gx = Center.x_cells();
	gy = Center.y_cells();
	
	if (gx > 4*Records || gy > 4*Records) return NULL;
	
	PartitionGWT gX(Records, gx, Center.x_range(method));
	for (cnt= 0; cnt < Records; ++cnt)       // fill in x-partition
		gX.include ( cnt, Center.x_range(cnt, method) );
	
	PartitionGWT *A	= new PartitionGWT(Records, gy, Center.y_range(method)),
	*B	= new PartitionGWT(Records, gy, Center.y_range(method));
	
	GwtNeighbor * buffer	= new GwtNeighbor[ Records];
	GwtElement * GwtHalf	= new GwtElement[ Records];
	long BufferSize= 0, included;
	
	for (part= 0; part < gx; ++part)  
	{      // processing all elements along (part, y)
		included= 0;
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY;
			 curr= gX.tail(curr), ++included)
			A->include( curr, Center.y_range(curr, method) );
		for (curr= gX.first(part); curr != GeoDaConst::EMPTY;
			 curr= gX.tail(curr))  
		{
			long cell= A->Where( Center.y_range(curr, method) );
			Center.setCurrent(curr);
			if (cell > 0)  
			{
				Center.CheckParticle(B, cell-1, BufferSize, buffer);
				Center.CheckParticle(A, cell-1, BufferSize, buffer);
			};
			Center.CheckParticle(B, cell, BufferSize, buffer);
			Center.CheckParticle(A, cell, BufferSize, buffer);
			if (++cell < gy)  
			{
				Center.CheckParticle(A, cell, BufferSize, buffer);
				Center.CheckParticle(B, cell, BufferSize, buffer);
			};
			GwtHalf[curr].alloc(BufferSize);
			while (BufferSize)
				GwtHalf[curr].Push(buffer[--BufferSize]);
		};
		if (part > 0 && included > 0)  
		{
			if (4*included > gy) // it's less expensive to reset all cells in the partition
				B->reset();
			else               // it's less expensive to reset only used cells
				for(curr= gX.first(part); curr != GeoDaConst::EMPTY;
					curr= gX.tail(curr))
					B->reset(Center.y_range(curr, method));
		};
		swap(A, B); // swap A and B -- sweep by; now A is empty, B contains current line
	};
	delete  A;
	delete  B;
	GwtElement * GwtFull= MakeFullGwt(GwtHalf, Records, degree, false);
	delete [] GwtHalf;
	GwtHalf = NULL;
	delete [] buffer;
	buffer = NULL;
	
	return GwtFull;
}

#include "../kNN/ANN.h"			// ANN declarations
GwtElement* DynKNN(const std::vector<double>& x, const std::vector<double>& y,
				   int k, int method)
{
	int obs = x.size();
	if (obs	< 3 || k < 1 || k > obs || x.size() != y.size()) return NULL;
	
	int		dim		= 2;		// dimension
	ANNpointArray	data_pts;		// data points
	ANNpoint			query_pt;		// query point
	ANNidxArray		nn_idx;			// near neighbor indices
	ANNdistArray	dists;			// near neighbor distances
	ANNkd_tree		*the_tree;		// search structure
	
	GwtElement *gwt = new GwtElement[obs];
	if (gwt == NULL)
		return NULL;
	
	query_pt	= annAllocPt(dim);			// allocate query point
	data_pts	= annAllocPts(obs, dim);	// allocate data points
	nn_idx		= new ANNidx[k];			// allocate near neigh indices
	dists		= new ANNdist[k];			// allocate near neighbor dists
	int i = 0;
	for (i=0;i<obs; i++) {
		data_pts[i][0] = x.at(i);
		data_pts[i][1] = y.at(i);
	}
	
	the_tree = new ANNkd_tree(data_pts,obs,dim);
	
	for (i=0; i<obs; i++) {
		the_tree->annkSearch(data_pts[i], k, nn_idx, dists,0.0, method);
		gwt[i].alloc(k-1);
		for (int j=1; j<k; j++) {
			GwtNeighbor e;
			e.nbx = nn_idx[j];
			e.weight = sqrt(dists[j]);  // annkSearch returns each distance
			// squared, so take sqrt.
			gwt[i].Push(e);
		}
	}
	return gwt;
}


double ComputeCutOffPoint(const std::vector<double>& x,
						  const std::vector<double>& y,
						  int method) // 1 == Euclidean Dist, 2== Arc Dist
{
	int obs = x.size();
	if (obs < 3 || x.size() != y.size()) return 0.0;
		
	int	dim	= 2;		// dimension
	ANNpointArray	data_pts;		// data points
	ANNpoint		query_pt;		// query point
	ANNidxArray		nn_idx;			// near neighbor indices
	ANNdistArray	dists;			// near neighbor distances
	ANNkd_tree		*the_tree;		// search structure
	
	int k = 2;
	query_pt = annAllocPt(dim);			// allocate query point
	data_pts = annAllocPts(obs, dim);	// allocate data points
	nn_idx	 = new ANNidx[k];			// allocate near neigh indices
	dists	 = new ANNdist[k];			// allocate near neighbor dists
	
	for (int i=0; i<obs; i++) {
		data_pts[i][0] = x[i];
		data_pts[i][1] = y[i];
	}
	
	the_tree = new ANNkd_tree(data_pts, obs, dim);
	
	the_tree->annkSearch(data_pts[0], k, nn_idx, dists, 0.0, method);
	double minDist = sqrt(dists[1]);
	
	int p1 = 0, p2 = 1;
	for (int i=1; i<obs; i++) {
		the_tree->annkSearch(data_pts[i], k, nn_idx, dists, 0.0, method);
		if (minDist <  sqrt(dists[1])) {
			minDist = sqrt(dists[1]);
			p1 = i;
			p2 = nn_idx[1];
		}
	}
	if (method==1) {
		minDist = GenGeomAlgs::ComputeEucDist(data_pts[p1][0], data_pts[p1][1],
											  data_pts[p2][0], data_pts[p2][1]);
	} else {
		minDist = GenGeomAlgs::ComputeArcDist(data_pts[p1][0], data_pts[p1][1],
											  data_pts[p2][0],data_pts[p2][1]);
	}
	
	delete the_tree;
	
	return minDist;
}

