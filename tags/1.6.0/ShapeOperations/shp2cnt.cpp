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
 Functions to create a contiguity file (.gal) from polygon coverage (Shapefile).
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
#include "../GenUtils.h"
#include "../logger.h"
#include "ShapeFileHdr.h"
#include "ShapeFileTypes.h"

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
	int lower= (int)floor(lwr/step), upper= (int)floor(upr/step);
	if (lwr < 0 || upper > cells || incl < 0 || incl >= elements)  
	{
		//     cout << "PartM: incl= " << incl << " l= " << lwr << "  " << upr; 
		exit(1);  
	};
	if (lower < 0) lower= 0;
    else if (lower >= cells) lower= cells-1;
	if (upper >= cells) upper= cells-1;
    else if (upper < 0) upper= 0;
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

void ReadOffsets(const wxString& fname)  
{
	iShapeFile    shx(wxString(fname), "shx");
	char          hs[ 2*GdaConst::ShpHeaderSize ];
	shx.read((char *) &hs[0], 2*GdaConst::ShpHeaderSize);
	ShapeFileHdr        hd(hs);
	long          offset, contents;
	gRecords= (hd.Length() - GdaConst::ShpHeaderSize) / 4;
	gOffset= new long [ gRecords ];
	
	for (long rec= 0; rec < gRecords; ++rec)  
	{
		offset= ReadBig(shx);
		contents= ReadBig(shx);
		offset *= 2;
		gOffset[rec]= offset;
	};
	return;
}

bool IsLineShapeFile(const wxString& fname)
{
	iShapeFile   shp(fname, "shp");
	char         hs[ 2*GdaConst::ShpHeaderSize ];
	
	shp.read(hs, 2 * GdaConst::ShpHeaderSize);
	ShapeFileHdr       head(hs);
	
	shp.Recl(head.FileShape());
	return (head.FileShape() == ShapeFileTypes::ARC);
}

void ReadBoxes(const wxString& fname)  
{
	iShapeFile   shp(fname, "shp");
	char          hs[ 2*GdaConst::ShpHeaderSize ];
	shp.read(hs, 2*GdaConst::ShpHeaderSize);
	ShapeFileHdr hd(hs);
	if ((ShapeFileTypes::ShapeType(hd.FileShape())
		 != ShapeFileTypes::POLYGON) &&
		(ShapeFileTypes::ShapeType(hd.FileShape())
		 != ShapeFileTypes::POLYGON_Z) &&
		(ShapeFileTypes::ShapeType(hd.FileShape())
		 != ShapeFileTypes::POLYGON_M))
	{
		//      cout << hs << endl;
		//	  cout << hd.FileShape() << endl;
		//      cout << " expecting POLYGON shape type " << endl;  
		exit(1);  
	};
	gBigBox= hd.BoundingBox();
	
	gBox= new Box [ gRecords ];
	for (long rec= 0; rec < gRecords; ++rec)  {
		shp.seekg(gOffset[rec]+12, ios::beg);
#ifdef WORDS_BIGENDIAN
		char r[32], p;
		double m1, m2, n1, n2;
		shp.read((char *)r, sizeof(double) * 4);
		SWAP(r[0], r[7], p);
		SWAP(r[1], r[6], p);
		SWAP(r[2], r[5], p);
		SWAP(r[3], r[4], p);
		memcpy(&m1, &r[0], sizeof(double));
		SWAP(r[8], r[15], p);
		SWAP(r[9], r[14], p);
		SWAP(r[10], r[13], p);
		SWAP(r[11], r[12], p);
		memcpy(&m2, &r[8], sizeof(double));
		SWAP(r[16], r[23], p);
		SWAP(r[17], r[22], p);
		SWAP(r[18], r[21], p);
		SWAP(r[19], r[20], p);
		memcpy(&n1, &r[16], sizeof(double));
		SWAP(r[24], r[31], p);
		SWAP(r[25], r[30], p);
		SWAP(r[26], r[29], p);
		SWAP(r[27], r[28], p);
		memcpy(&n2, &r[24], sizeof(double));
		BasePoint p1 = BasePoint(m1, m2);
		BasePoint p2 = BasePoint(n1, n2);
		gBox[rec] = Box(p1, p2);
#else
		shp >> gBox[rec];
#endif
		gBigBox += gBox[rec];                // make sure BigBox covers all boxes
	};
	return;
}

// tests if two boxes intersect each other
inline bool Intersect(const Box &a, const Box &b)  
{
    if (a._max().x < b._min().x) return false;      // a is left to b
    if (a._min().x > b._max().x) return false;      // a is right to b
    if (a._max().y < b._min().y) return false;      // a is down to b
    if (a._min().y > b._max().y) return false;      // a is up to b
    return true;
}

GalElement* MakeContiguity(Shapefile::Main& main, const int crit,
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
			 curr= gMinX.tail(curr)) gY->include(curr);
		
		// test each element in xmax[step]
		for (curr= gMaxX.first(step); curr != GdaConst::EMPTY;
			 curr= gMaxX.tail(curr))  {
			PolygonContents* ply = dynamic_cast<PolygonContents*> (
												main.records[curr].contents_p);
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
				PolygonContents* nbr_ply = dynamic_cast<PolygonContents*> (
												main.records[nbr].contents_p);
				if (ply->intersect(nbr_ply)) {
					
					PolygonPartition nbrPoly(nbr_ply);
					//shp.seekg(gOffset[nbr]+12, ios::beg);
					//nbrPoly.ReadShape(shp);
					
					if (curr == 0 && nbr == 0) {
						
					}
					
					// run sweep with testPoly as a host and nbrPoly as a guest
					int related = testPoly.sweep(nbrPoly, crit,
                                                 precision_threshold);
					if (related) Related.Push(nbr);
				}
			}
			
			if (Related.Size() && gl[curr].alloc(Related.Size())) {
				while (Related.Size()) gl[curr].Push(Related.Pop());
			}
			
			gY->remove(curr);       // remove from the partition
		}
	}
	
	return gl;
}

void ValueSort(const long * value, const int lower, const int upper)  
{
	int   i= lower, j= upper, t = 0;
	long    pivot= value[(lower+upper) >> 1];
	do  {
		while (value[i] < pivot) ++i;
		while (value[j] > pivot) --j;
		if (i < j)  SWAP(i, j, t);
		if (i <= j)  { ++i;  --j;  };
	}
	while (j >= i);
	if (j > lower) ValueSort(value, lower, j);
	if (i < upper) ValueSort(value, i, upper);
	return;
}


inline void Write(ofstream &out, const double val)  
{
    out.write((char *) &val, sizeof(double));
    return;
}

inline void Write(ofstream &out, const long val)  
{
	out.write((char *) &val, sizeof(long));
	return;
}


inline long Read(ifstream &in)  
{
	long   val;
	in.read( (char *) &val, sizeof(long) );
	return val;
}


inline double ReadD(ifstream &in)  
{
	double   val;
	in.read( (char *) &val, sizeof(double) );
	return val;
}



GalElement * MakeFull(GalElement *half)  
{
    long * Count= new long [ gRecords ], nbr, cnt;
    for (cnt= 0; cnt < gRecords; ++cnt)
		Count[ cnt ]= half[cnt].Size();
    for (cnt= 0; cnt < gRecords; ++cnt)
        for (nbr= half[cnt].Size()-1; nbr >= 0; --nbr)
			++Count [ half[cnt].elt(nbr) ];
    GalElement * full= new GalElement [ gRecords ];
	
    for (cnt= 0; cnt < gRecords; ++cnt)
		full[cnt].alloc( Count[cnt] );
    for (cnt= 0; cnt < gRecords; ++cnt)
		for (nbr= half[cnt].Size()-1; nbr >= 0; --nbr)  {
			long val= half[cnt].elt(nbr);
			full[cnt].Push(val);
			full[val].Push(cnt);
		};
    bool isGalEmpty = false;
    for (cnt= 0; cnt < gRecords; ++cnt)  {
		if (full[cnt].Size() > 1) {
            isGalEmpty = true;
			ValueSort(full[cnt].dt(), 0, full[cnt].Size()-1);
        }
    };
    if (isGalEmpty == false) {
        delete [] full;
        full = NULL;
    }
	delete [] Count;
	Count = NULL;
    return full;
};


bool SaveGal(const GalElement *full, 
			 const wxString& layer_name, 
			 const wxString& ofname, 
			 const wxString& vname,
			 const std::vector<wxInt64>& id_vec)
{
	LOG_MSG("Entering SaveGal");
	if (full == NULL || ofname.empty() || vname.empty() || id_vec.size() == 0) {
		return false;
	}
	int Obs = (int) id_vec.size();
	
	wxFileName galfn(ofname);
    galfn.SetExt("gal");
    wxString gal_ofn(galfn.GetFullPath());
    std::ofstream out;
	out.open(GET_ENCODED_FILENAME(gal_ofn));
	
    if (!(out.is_open() && out.good())) {
        return false;
    }
	
	LOG_MSG(layer_name);
	
	out << "0 " << Obs << " " << layer_name.c_str();
	out << " " << vname.c_str() << endl;
	
	for (int cnt= 0; cnt < Obs; ++cnt) {
		out << id_vec[cnt];
		out << " " << full[cnt].Size() << endl;
		for (int cp= full[cnt].Size(); --cp >= 0;) {
			out << id_vec[full[cnt].elt(cp)];			
			if (cp > 0) out << " ";
		}
		out << endl;
	}

	LOG_MSG("Exiting SaveGal");
	return true;
}


GalElement* shp2gal(Shapefile::Main& main, int criteria, bool save,
                    double precision_threshold)
{
	using namespace Shapefile;
	
	GalElement * full;
	//ReadOffsets(fname);
	//ReadBoxes(fname);
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
		PolygonContents* ply = dynamic_cast<PolygonContents*> (
											main.records[cnt].contents_p);
		
		gMinX.include( cnt, ply->box[0] - shp_min_x );
		gMaxX.include( cnt, ply->box[2] - shp_min_x );
	}
	
	gy= (int)(sqrt((long double)gRecords) + 2);
	do {
		gY= new PartitionM(gRecords, gy, shp_y_len );
		for (cnt= 0; cnt < gRecords; ++cnt) {
			PolygonContents* ply = dynamic_cast<PolygonContents*> (
											main.records[cnt].contents_p);
			gY->initIx( cnt, ply->box[1] - shp_min_y, ply->box[3] - shp_min_y );
		}
		total= gY->Sum();
		if (total > gRecords * 8) {
			delete gY;
			gy = gy/2 + 1;
			total= 0;
		}
	} while ( total == 0);
	
	GalElement * gl= MakeContiguity(main, criteria, precision_threshold);
    
	if (gY) delete gY; gY = 0;
	if (gOffset) delete [] gOffset; gOffset = 0;
	if (gBox) delete [] gBox; gBox = 0;
	
	full = MakeFull(gl);
	if (gl) delete [] gl; gl = 0;
	return full;
}

// Lag: True; otherwise Cumulative
GalElement *HOContiguity(const int p, long obs, GalElement *W, bool Lag)
{	
	if (obs	< 1 || p <= 1 || p > obs-1) return NULL;
	
	int j, QueueCnt, c, irow, CurrIx, LastIx;
	int* nLag				= new int [p];
	GalElement *HO	= new GalElement[obs];
	long *Queue			= new long [obs];
	long *dt;
	int *OC					= new int [obs];
	
	if (W == NULL || nLag == NULL || HO == NULL ||
		Queue == NULL || OC == NULL) return NULL;
	
	for (j=0; j < obs; j++) OC[j] = 0;
	for (irow = 0; irow < obs; irow++) {
		OC[irow] = -1;
		dt = W[irow].dt(); // neighbors of irow
		QueueCnt = W[irow].Size();
		for (j=0; j<QueueCnt; j++) {
			Queue[j] = dt[j];
			OC[Queue[j]] = 1;
		}
		CurrIx = 0; int k;
		k= LastIx = QueueCnt;
		for (c=2;c<=p; c++) {
			nLag[c-2] = 0;
			LastIx = k;
			for (;CurrIx <LastIx; CurrIx++) {
				dt = W[Queue[CurrIx]].dt();
				int Nbrs = W[Queue[CurrIx]].Size();
				for ( j=0; j< Nbrs; j++) {
					if (OC[dt[j]] == 0) {
						OC[dt[j]] = c;
						Queue[k] = dt[j];
						k++;
						nLag[c-2]++;
					}
				}
			}
		}
		
		LastIx = k;
		if (!Lag) {
			HO[irow].alloc(nLag[p-2]);
			k = nLag[p-2];
		}
		else HO[irow].alloc(LastIx);
		
		for (j=0;j<LastIx;j++) {
			if (Lag) {
				HO[irow].Push(Queue[j]);
			} else {
				
				if (OC[Queue[j]] == p) {
					HO[irow].Push(Queue[j]);
				}
				
			}
			OC[Queue[j]] = 0;
		}
		OC[irow] = 0;
	}
	return HO;
}

void DevFromMean(int nObs, double* RawData)
{
	double sumX = 0.0;
	int cnt = 0;
	for (cnt= 0; cnt < nObs; ++cnt) 
	{
		sumX += RawData[cnt];
	}
	const double  meanX = sumX / nObs;
	for (cnt= 0; cnt < nObs; ++cnt)
	{
		RawData[cnt] -= meanX;
	}
}
