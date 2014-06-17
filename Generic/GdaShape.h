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

class GdaPolygon;

typedef boost::geometry::model::d2::point_xy<double> point_2d;

struct GdaScaleTrans {
	GdaScaleTrans() :
		scale_x(1.0), scale_y(1.0), max_scale(1.0), trans_x(0.0), trans_y(0.0){}
	GdaScaleTrans(double s_x, double s_y, double t_x, double t_y) :
		scale_x(s_x), scale_y(s_y), max_scale(GenUtils::max<double>(s_x, s_y)),
		trans_x(t_x), trans_y(t_y) {}
	virtual GdaScaleTrans& operator=(const GdaScaleTrans& s);
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
	void transform_back(const wxPoint& src, wxRealPoint& result) const;
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

namespace GdaShapeAlgs {
	void partsToCount(const std::vector<wxInt32>& parts,
					  int total_points, int* count);
	wxRealPoint calculateMeanCenter(GdaPolygon* poly);
	wxRealPoint calculateMeanCenter(int n, wxRealPoint* pts);
	wxRealPoint calculateMeanCenter(const std::vector<Shapefile::Point>& pts);
	wxRealPoint calculateCentroid(GdaPolygon* poly);
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
	void getBoundingBoxOrig(const GdaPolygon* p, double& xmin,
							double& ymin, double& xmax, double& ymax);
}

struct GdaShapeAttribs {
	GdaShapeAttribs() : brush(*wxBLUE_BRUSH), pen(*wxBLACK_PEN),
		x_nudge(0), y_nudge(0) {}
	GdaShapeAttribs(const GdaShapeAttribs& s);
	GdaShapeAttribs(const GdaShapeAttribs* s);
	virtual GdaShapeAttribs& operator=(const GdaShapeAttribs& s);
	virtual ~GdaShapeAttribs() {}
	
	wxBrush brush;
	wxPen pen;
	int x_nudge;
	int y_nudge;
};

class GdaShape {
public:
	GdaShape();
	GdaShape(const GdaShape& s);
	virtual ~GdaShape();
	virtual GdaShape* clone() = 0;
	virtual GdaShape& operator=(const GdaShape& s);
	
	virtual wxPoint getCentroid() { return center; }
	virtual wxPoint getMeanCenter() { return center; }
	virtual wxRealPoint getCentroidOrig() { return center_o; }
	virtual wxRealPoint getMeanCenterOrig() { return center_o; }
	
	virtual bool pointWithin(const wxPoint& pt) { return false; };
	virtual bool regionIntersect(const wxRegion& region) { return false; };
	virtual void applyScaleTrans(const GdaScaleTrans& A);
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
	bool isNull() { return null_shape; }
	wxPoint center;
	// for selectable shapes, indicates which category shape belongs to
	int category;
//protected:
	wxRealPoint center_o;
	wxPoint bb_poly[5];
protected:
	bool null_shape;  // flag for an placeholder or empty shape
	GdaShapeAttribs* attribs; // optional extra attributes
};


class GdaPoint: public GdaShape {
public:
	GdaPoint(); // creates a null shape
	GdaPoint(const GdaPoint& s); 
	GdaPoint(wxRealPoint point_o_s);
	GdaPoint(double x_orig, double y_orig);
	virtual ~GdaPoint() {}
	virtual GdaPoint* clone() { return new GdaPoint(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	//virtual void applyScaleTrans(const GdaScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
    double GetX();
    double GetY();
};


class GdaCircle: public GdaShape {
public:
	GdaCircle(); // creates a null shape
	GdaCircle(const GdaCircle& s);
	GdaCircle(wxRealPoint center_o_s, double radius_o_s,
			 bool scale_radius = false);
	virtual ~GdaCircle() {}
	virtual GdaCircle* clone() { return new GdaCircle(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	//wxPoint center; // inherited from GdaShape
	double radius;
	bool scale_radius; // does radius change when affine trans applied?
protected:
	//wxRealPoint center_o; // inherited from GdaShape
	double radius_o;
};

class GdaRectangle: public GdaShape {
public:
	GdaRectangle(); // creates a null shape
	GdaRectangle(const GdaRectangle& s);
	GdaRectangle(wxRealPoint lower_left_o_s, wxRealPoint upper_right_o_s);
	virtual ~GdaRectangle() {}
	virtual GdaRectangle* clone() { return new GdaRectangle(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	wxPoint lower_left;
	wxPoint upper_right;
protected:
	wxRealPoint lower_left_o;
	wxRealPoint upper_right_o;
};


class GdaPolygon: public GdaShape {
public:
	GdaPolygon(); // creates a null shape
	GdaPolygon(const GdaPolygon& s);
	GdaPolygon(int n_s, wxRealPoint* points_o_s);
	GdaPolygon(Shapefile::PolygonContents* pc_s);
	virtual ~GdaPolygon();
	virtual GdaPolygon* clone() { return new GdaPolygon(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
	
	static wxRealPoint CalculateCentroid(int n, wxRealPoint* pts);
	virtual void paintSelf(wxDC& dc);
public:
	// All values in points array are the same.  Can render render
	// as a single point at points[0]
	bool all_points_same;
	
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
	wxRealPoint bb_ll_o; // bounding box lower left
	wxRealPoint bb_ur_o; // bounding box upper right
	//wxRegion region;
};


class GdaPolyLine: public GdaShape {
public:
	GdaPolyLine();
	GdaPolyLine(const GdaPolyLine& s);
	GdaPolyLine(int n_s, wxRealPoint* points_o_s);
	GdaPolyLine(double x1, double y1, double x2, double y2);
	GdaPolyLine(Shapefile::PolyLineContents* pc_s);
	virtual GdaPolyLine& operator=(const GdaPolyLine& s);
	virtual ~GdaPolyLine();
	virtual GdaPolyLine* clone() { return new GdaPolyLine(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
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


class GdaRay: public GdaShape {
public:
	GdaRay(); // creates a null shape
	GdaRay(const GdaRay& s);
	GdaRay(wxRealPoint center_o_s, double degs_rot_cc_from_horiz_s,
		  int length_s);
	virtual ~GdaRay() {}
	virtual GdaRay* clone() { return new GdaRay(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual bool regionIntersect(const wxRegion& r);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
	virtual void paintSelf(wxDC& dc);
public:
	//wxPoint center; // inherited from GdaShape
	double degs_rot_cc_from_horiz;
	int length; // length in pixels
protected:
	//wxRealPoint center_o; // inherited from GdaShape
};


class GdaShapeText: public GdaShape {
public:
	enum VertAlignment{ top, v_center, bottom };
	enum HorizAlignment{ left, h_center, right };
	
	GdaShapeText();
	GdaShapeText(wxString text_s, wxFont font_s, const wxRealPoint& ref_pt_s,
		   double degs_rot_cc_from_horiz_s = 0,
		   HorizAlignment h_align = h_center, VertAlignment v_align = v_center,
		   int x_nudge = 0, int y_nudge = 0);
	GdaShapeText(const GdaShapeText& s);
	virtual GdaShapeText& operator=(const GdaShapeText& s); 
	virtual ~GdaShapeText() {}
	virtual GdaShapeText* clone() { return new GdaShapeText(*this); }
	
	virtual bool pointWithin(const wxPoint& pt);
	virtual void applyScaleTrans(const GdaScaleTrans& A);
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

class GdaShapeTable : public GdaShape {
public:
	struct CellAttrib {
		wxColour color;
	};
	
	GdaShapeTable();
	GdaShapeTable(const std::vector<wxString>& vals,
			const std::vector<CellAttrib>& attributes,
			int rows, int cols, wxFont font,
			const wxRealPoint& ref_pt,
			GdaShapeText::HorizAlignment horiz_align = GdaShapeText::h_center,
			GdaShapeText::VertAlignment vert_align = GdaShapeText::v_center,
			GdaShapeText::HorizAlignment cell_h_align = GdaShapeText::h_center,
			GdaShapeText::VertAlignment cell_v_align = GdaShapeText::v_center,
			int row_gap = 3, int col_gap = 10,
			int x_nudge = 0, int y_nudge = 0);
	GdaShapeTable(const GdaShapeTable& s);
	virtual GdaShapeTable& operator=(const GdaShapeTable& s); 
	virtual ~GdaShapeTable() {}
	virtual GdaShapeTable* clone() { return new GdaShapeTable(*this); }
	
	virtual void applyScaleTrans(const GdaScaleTrans& A);
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
	GdaShapeText::HorizAlignment horiz_align;
	GdaShapeText::VertAlignment vert_align;
	GdaShapeText::HorizAlignment cell_h_align;
	GdaShapeText::VertAlignment cell_v_align;
protected:
	wxRealPoint ref_pt_o;
};

class GdaAxis: public GdaShape {
public:
	GdaAxis();
	GdaAxis(const GdaAxis& s);
	GdaAxis(const wxString& caption_s, const AxisScale& s,
		   const wxRealPoint& a_s, const wxRealPoint& b_s,
		   int x_nudge = 0, int y_nudge = 0);
	virtual ~GdaAxis() {}
	virtual GdaAxis* clone() { return new GdaAxis(*this); }
	
	virtual void applyScaleTrans(const GdaScaleTrans& A);
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

class GdaSelRegion {
public:
	enum Type { rectangle, circle, line };
	Type regionType;
	int sel1;
	int sel2;
};

/**
 This class is used for pointer comparisons between GdaShape pointers.  If
 p1 and p2 are pointers to GdaShape instances, then p1 < p2 is true
 if and only if p1->z < p2->z, where z is the z-value integer in each GdaShape
 object.
 This is only used to specify the std::multiset<GdaShape*, my_shp_ptr_comp>
 template instance so that a multiset of GdaShape points has a well-defined
 partial order.
 */
//class my_shp_ptr_comp { // used by multiset template for comparison
//public:
//	bool operator() (const GdaShape* lhs, const GdaShape* rhs) const
//	{ return lhs->z < rhs->z; }
//};

#endif
