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

#ifndef __GEODA_CENTER_MY_SHAPE_H__
#define __GEODA_CENTER_MY_SHAPE_H__

#undef check // macro undefine needed for Xcode compilation with Boost.Geometry
#include <wx/gdicmn.h>
#include <wx/brush.h>
#include <wx/pen.h>
#include <wx/region.h>
#include <wx/string.h>
#include <wx/dc.h>
#include "../ShapeOperations/ShpFile.h"
#include <cmath>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include "../GenUtils.h"

class MyPolygon;

typedef boost::geometry::model::d2::point_xy<double> point_2d;

struct MyScaleTrans {
	MyScaleTrans() :
		scale_x(1.0), scale_y(1.0), max_scale(1.0), trans_x(0.0), trans_y(0.0){}
	MyScaleTrans(double s_x, double s_y, double t_x, double t_y) :
		scale_x(s_x), scale_y(s_y), max_scale(GenUtils::max<double>(s_x, s_y)),
		trans_x(t_x), trans_y(t_y) {}
	virtual MyScaleTrans& operator=(const MyScaleTrans& s);
	static void calcAffineParams(double x_min, double y_min,
								 double x_max, double y_max,
								 double top_marg, double bottom_marg,
								 double left_marg, double right_marg,
								 double screen_width, double screen_height,
								 bool fixed_aspect_ratio,
								 bool fit_to_window,
								 double* scale_x_p, double* scale_y_p,
								 double* trans_x_p, double* trans_y_p,
								 double target_width=0, double target_height=0,
								 double* image_width_p=0,
								 double* image_height_p=0);
	wxString GetString();
	void transform(const point_2d& pt, wxPoint* result) const;
	void transform(const wxRealPoint& src, wxPoint* result) const;
	void transform(const wxRealPoint& src, wxRealPoint* result) const;
	void transform(const wxPoint& src, wxPoint* result) const;
	void transform(const Shapefile::Point& src, wxPoint* result) const;
	void transform(const double& src, double* result) const;
	void transform(const double& src, int* result) const;
	
	double scale_x;
	double scale_y;
	double max_scale; // max of scale_x, scale_y
	double trans_x;
	double trans_y;
};

namespace MyShapeAlgs {
	void partsToCount(const std::vector<wxInt32>& parts,
					  int total_points, int* count);
	wxRealPoint calculateMeanCenter(MyPolygon* poly);
	wxRealPoint calculateMeanCenter(int n, wxRealPoint* pts);
	wxRealPoint calculateMeanCenter(const std::vector<Shapefile::Point>& pts);
	wxRealPoint calculateCentroid(MyPolygon* poly);
	wxRealPoint calculateCentroid(int n, wxRealPoint* pts);
	wxRealPoint calculateCentroid(int n,
								  const std::vector<Shapefile::Point>& pts);
	double calculateArea(int n, wxRealPoint* pts);
	double calculateArea(int n, const std::vector<Shapefile::Point>& pts);
	void createCirclePolygon(const wxPoint& center, double radius,
							 int num_points = 0,
							 wxPoint* pnts_array = 0,
							 int* pnts_array_size = 0);	
	wxRegion createCircleRegion(const wxPoint& center, double radius,
								int num_points = 0,
								wxPoint* pnts_array = 0,
								int* pnts_array_size = 0);
	wxRegion createLineRegion(wxPoint a, wxPoint b);
	bool pointInPolygon(const wxPoint& pt, int n, const wxPoint* pts);
	void getBoundingBoxOrig(const MyPolygon* p, double& xmin,
							double& ymin, double& xmax, double& ymax);
}

struct MyShapeAttribs {
	MyShapeAttribs() : brush(*wxBLUE_BRUSH), pen(*wxBLACK_PEN),
		x_nudge(0), y_nudge(0) {}
	MyShapeAttribs(const MyShapeAttribs& s);
	MyShapeAttribs(const MyShapeAttribs* s);
	virtual MyShapeAttribs& operator=(const MyShapeAttribs& s);
	virtual ~MyShapeAttribs() {}
	
	wxBrush brush;
	wxPen pen;
	int x_nudge;
	int y_nudge;
};

class MyShape {
public:
	MyShape();
	MyShape(const MyShape& s);
	virtual ~MyShape();
	virtual MyShape* clone() = 0;
	virtual MyShape& operator=(const MyShape& s);
	
	virtual wxPoint getCentroid() { return center; }
	virtual wxPoint getMeanCenter() { return center; }
	virtual wxRealPoint getCentroidOrig() { return center_o; }
	virtual wxRealPoint getMeanCenterOrig() { return center_o; }
	
	virtual bool pointWithin(const wxPoint& pt) { return false; };
	virtual bool regionIntersect(const wxRegion& region) { return false; };
	virtual void applyScaleTrans(const MyScaleTrans& A);

	virtual void paintSelf(wxDC& dc) = 0;
	
public:
	// calls allocAttribs if needed, a convenience function.
	virtual void setNudge(int x_nudge, int y_nudge);
	virtual void setPen(const wxPen& pen);
	virtual void setBrush(const wxBrush& brush);
	const wxPen& getPen();
	const wxBrush& getBrush();
	int getXNudge();
	int getYNudge();
	wxPoint center;
	// for selectable shapes, indicates which category shape belongs to
	int category;
//protected:
	wxRealPoint center_o;
	wxPoint bb_poly[5];
protected:
	MyShapeAttribs* attribs; // optional extra attributes
};


class MyPoint: public MyShape {
public:
	MyPoint(const MyPoint& s); 
	MyPoint(wxRealPoint point_o_s);
	MyPoint(double x_orig, double y_orig);
	virtual ~MyPoint() {}
	virtual MyPoint* clone() { return new MyPoint(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	//virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
};


class MyCircle: public MyShape {
public:
	MyCircle(const MyCircle& s);
	MyCircle(wxRealPoint center_o_s, double radius_o_s,
			 bool scale_radius = false);
	virtual ~MyCircle() {}
	virtual MyCircle* clone() { return new MyCircle(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	//wxPoint center; // inherited from MyShape
	double radius;
	bool scale_radius; // does radius change when affine trans applied?
protected:
	//wxRealPoint center_o; // inherited from MyShape
	double radius_o;
};

class MyRectangle: public MyShape {
public:
	MyRectangle() : lower_left(0,0), upper_right(0,0),
		lower_left_o(0,0), upper_right_o(0,0) {}
	MyRectangle(const MyRectangle& s);
	MyRectangle(wxRealPoint lower_left_o_s, wxRealPoint upper_right_o_s);
	virtual ~MyRectangle() {}
	virtual MyRectangle* clone() { return new MyRectangle(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	wxPoint lower_left;
	wxPoint upper_right;
protected:
	wxRealPoint lower_left_o;
	wxRealPoint upper_right_o;
};


class MyPolygon: public MyShape {
public:
	MyPolygon(const MyPolygon& s);
	MyPolygon(int n_s, wxRealPoint* points_o_s);
	MyPolygon(Shapefile::PolygonContents* pc_s);
	virtual ~MyPolygon();
	virtual MyPolygon* clone() { return new MyPolygon(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const MyScaleTrans& A);

	static wxRealPoint CalculateCentroid(int n, wxRealPoint* pts);
	virtual void paintSelf(wxDC& dc);
public:
	wxPoint* points;
	int n; // size of points array
	int n_count; // size of count array
	// Note: count array is different than PolygonContents::parts array
	//   count stores the number of points in each polygon part
	//   parts stores the index of the first point for each polygon
	int* count;
//protected:
	// (pc == 0 && points_o !=0 ) || (pc != 0 && points_o ==0 )
	Shapefile::PolygonContents* pc;
	wxRealPoint* points_o;
	//wxRegion region;
};


class MyPolyLine: public MyShape {
public:
	MyPolyLine();
	MyPolyLine(const MyPolyLine& s);
	MyPolyLine(int n_s, wxRealPoint* points_o_s);
	MyPolyLine(double x1, double y1, double x2, double y2);
	MyPolyLine(Shapefile::PolyLineContents* pc_s);
	virtual MyPolyLine& operator=(const MyPolyLine& s);
	virtual ~MyPolyLine();
	virtual MyPolyLine* clone() { return new MyPolyLine(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
	virtual wxString printDetails();
	
	static wxRealPoint CalculateCentroid(int n, wxRealPoint* pts)
	{ return wxRealPoint(0,0); }
public:
	wxPoint* points;
	int n; // size of points array
	int n_count; // size of count array
	int* count; // index into various parts of points array
//protected:
	// (pc == 0 && points_o !=0 ) || (pc != 0 && points_o ==0 )
	Shapefile::PolyLineContents* pc;
	wxRealPoint* points_o;
	//wxRegion region;
};


class MyRay: public MyShape {
public:
	MyRay(const MyRay& s);
	MyRay(wxRealPoint center_o_s, double degs_rot_cc_from_horiz_s,
		  int length_s);
	virtual ~MyRay() {}
	virtual MyRay* clone() { return new MyRay(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	//wxPoint center; // inherited from MyShape
	double degs_rot_cc_from_horiz;
	int length; // lenght in pixels
protected:
	//wxRealPoint center_o; // inherited from MyShape
};


class MyText: public MyShape {
public:
	enum VertAlignment{ top, v_center, bottom };
	enum HorizAlignment{ left, h_center, right };
	
	MyText();
	MyText(wxString text_s, wxFont font_s, const wxRealPoint& ref_pt_s,
		   double degs_rot_cc_from_horiz_s = 0,
		   HorizAlignment h_align = h_center, VertAlignment v_align = v_center,
		   int x_nudge = 0, int y_nudge = 0);
	MyText(const MyText& s);
	virtual MyText& operator=(const MyText& s); 
	virtual ~MyText() {}
	virtual MyText* clone() { return new MyText(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
	
	static wxPoint calcRefPoint(wxDC& dc, const wxString& text,
								const wxFont& font,
								const wxRealPoint& ref_pt,
								double degs_rot_cc_from_horiz = 0,
								HorizAlignment h_align = h_center,
								VertAlignment v_align = v_center);
public:
	double getDegsRotCcFromHoriz() { return degs_rot_cc_from_horiz; }
	wxString getText() { return text; }
	void setText(wxString t) { text = t; }
	wxString text;
	wxFont font;
	wxRealPoint ref_pt;
	HorizAlignment horiz_align;
	VertAlignment vert_align;
	bool hidden;
protected:
	double degs_rot_cc_from_horiz;
	double degs_rot_cc_from_horiz_o;
	wxRealPoint ref_pt_o;
};

class MyTable : public MyShape {
public:
	struct CellAttrib {
		wxColour color;
	};
	
	MyTable();
	MyTable(const std::vector<wxString>& vals,
			const std::vector<CellAttrib>& attributes,
			int rows, int cols, wxFont font,
			const wxRealPoint& ref_pt,
			MyText::HorizAlignment horiz_align = MyText::h_center,
			MyText::VertAlignment vert_align = MyText::v_center,
			MyText::HorizAlignment cell_h_align = MyText::h_center,
			MyText::VertAlignment cell_v_align = MyText::v_center,
			int row_gap = 3, int col_gap = 10,
			int x_nudge = 0, int y_nudge = 0);
	MyTable(const MyTable& s);
	virtual MyTable& operator=(const MyTable& s); 
	virtual ~MyTable() {}
	virtual MyTable* clone() { return new MyTable(*this); }
	
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
	virtual void GetSize(wxDC& dc, int& w, int& h);
	
public:
	bool hidden;
	std::vector<wxString> vals;
	std::vector<CellAttrib> attributes;
	wxFont font;
	int rows;
	int cols;
	int row_gap;
	int col_gap;
	wxRealPoint ref_pt;
	MyText::HorizAlignment horiz_align;
	MyText::VertAlignment vert_align;
	MyText::HorizAlignment cell_h_align;
	MyText::VertAlignment cell_v_align;
protected:
	wxRealPoint ref_pt_o;
};

class MyAxis: public MyShape {
public:
	MyAxis();
	MyAxis(const MyAxis& s);
	MyAxis(const wxString& caption_s, const AxisScale& s,
		   const wxRealPoint& a_s, const wxRealPoint& b_s,
		   int x_nudge = 0, int y_nudge = 0);
	virtual ~MyAxis() {}
	virtual MyAxis* clone() { return new MyAxis(*this); }
	
	virtual void applyScaleTrans(const MyScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
	
public:
	wxString getCaption() { return caption; }
	void setCaption(const wxString& s) { caption = s; } 
	bool isHorizontal() { return is_horizontal; }
	void hideScaleValues(bool hide) { hide_scale_values = hide; }
	AxisScale scale;
	wxPoint a, b;
	wxString caption;
	bool hidden;
protected:
	bool is_horizontal;
	wxRealPoint a_o;
	wxRealPoint b_o;
	wxFont font;
	wxFont caption_font;
	bool hide_scale_values;
};

class MySelRegion {
public:
	enum Type { rectangle, circle, line };
	Type regionType;
	int sel1;
	int sel2;
};

/**
 This class is used for pointer comparisons between MyShape pointers.  If
 p1 and p2 are pointers to MyShape instances, then p1 < p2 is true
 if and only if p1->z < p2->z, where z is the z-value integer in each MyShape
 object.
 This is only used to specify the std::multiset<MyShape*, my_shp_ptr_comp>
 template instance so that a multiset of MyShape points has a well-defined
 partial order.
 */
//class my_shp_ptr_comp { // used by multiset template for comparison
//public:
//	bool operator() (const MyShape* lhs, const MyShape* rhs) const
//	{ return lhs->z < rhs->z; }
//};

#endif
