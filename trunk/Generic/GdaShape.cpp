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

#include "GdaShape.h"
#include <cmath> // for math abs and floor function
#include <cfloat>
#include <cfloat>
#include <wx/graphics.h>
#include "../logger.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include <boost/foreach.hpp>

GdaScaleTrans& GdaScaleTrans::operator=(const GdaScaleTrans& s)
{
	scale_x = s.scale_x;
	scale_y = s.scale_y;
	max_scale = GenUtils::max<double>(s.scale_x, s.scale_y);
	trans_x = s.trans_x;
	trans_y = s.trans_y;
	return *this;
}

/** x_min, y_min, x_max, y_max are the bonding box limits for the entire map.
 target_height and target_width are ignored if set to 0.  They are used
 when a target size smaller than the normal maximum working area size is
 desired, for example when the user continually zooms out.  The optional
 image_width_p and image_height_p parameters are for returning the
 resulting image width after the transformation is applied. */
void GdaScaleTrans::calcAffineParams(double x_min, double y_min,
									double x_max, double y_max,
									double top_marg, double bottom_marg,
									double left_marg, double right_marg,
									double screen_width, double screen_height,
									bool fixed_aspect_ratio,
									bool fit_to_window,
									double* scale_x_p, double* scale_y_p,
									double* trans_x_p, double* trans_y_p,
									double target_width, double target_height,
									double* image_width_p,
									double* image_height_p)
{
	//LOG_MSG("Entering GdaScaleTrans::calcAffineParams");
	double drawing_area_width = screen_width-(left_marg+right_marg);
	double drawing_area_height = screen_height-(top_marg+bottom_marg);
	
	//LOG(drawing_area_width);
	//LOG(target_width);
	//LOG(drawing_area_height);
	//LOG(target_height);
	
	if ( target_width > 0 && target_height > 0 &&
		target_width-1 <= drawing_area_width &&
		target_height-1 <= drawing_area_height ) {
		
		if (!fit_to_window && !fixed_aspect_ratio) {
			//LOG_MSG("margins and drawing area adjusted");
		}
		
		// increase the margins so that the working area is	equal
		// to the target_width and target_height
		double vert_delta = (drawing_area_height - target_height) / 2;
		double horiz_delta = (drawing_area_width - target_width) / 2;
		top_marg += vert_delta;
		bottom_marg += vert_delta;
		left_marg += horiz_delta;
		right_marg += horiz_delta;
		drawing_area_width -= (2 * horiz_delta);
		drawing_area_height -= (2 * vert_delta);
	} else {
		if (!fit_to_window && !fixed_aspect_ratio) {
			//LOG_MSG("margins not adjusted");
		}		
	}
	
	
	// drawing_area_ar represents the drawing area aspect ratio.
	double drawing_area_ar = drawing_area_width / drawing_area_height;
	
	// if fixed_aspect_ratio == true, we will maintain the original
	// aspect-ratio (width : height) of the input data.
	double data_width = x_max - x_min;
	double data_height = y_max - y_min;
	// data aspect ratio
	double data_ar = data_width / data_height;
	
	if ( fixed_aspect_ratio ) {
		if (drawing_area_ar >= data_ar ) {
			// scale (translated) data to fit within height of drawing area
			*scale_x_p = drawing_area_height / data_height;
			*scale_y_p = -(*scale_x_p);
			
			double slack =
				(drawing_area_width - ((*scale_x_p) * data_width))/2.0;
			*trans_x_p = slack + left_marg - (*scale_x_p)*x_min;
			*trans_y_p = screen_height - bottom_marg - (*scale_y_p)*y_min;
		} else { // drawing_area_ar < data_ar 
			// scale (translated) data to fit within width of drawing area
			*scale_x_p = drawing_area_width / data_width;
			*scale_y_p = -(*scale_x_p);
			
			*trans_x_p = left_marg - (*scale_x_p)*x_min;
			double slack =
				(drawing_area_height - ((*scale_x_p) * data_height))/2.0;
			*trans_y_p =
				screen_height - slack - bottom_marg - (*scale_y_p)*y_min;
		}
	} else { // fixed_aspect_ratio == false, fit_to_window == true/false
		*scale_x_p = drawing_area_width / data_width;
		*scale_y_p = -(drawing_area_height / data_height);
		
		*trans_x_p = left_marg - (*scale_x_p)*x_min;
		*trans_y_p = screen_height - bottom_marg - (*scale_y_p)*y_min;
	}
	
	if (image_width_p && image_height_p) {
		double new_x_min, new_y_min, new_x_max, new_y_max;
		new_x_min = x_min * (*scale_x_p) + (*trans_x_p);
		new_y_min = y_min * (*scale_y_p) + (*trans_y_p);
		new_x_max = x_max * (*scale_x_p) + (*trans_x_p);
		new_y_max = y_max * (*scale_y_p) + (*trans_y_p);
		*image_width_p = abs(new_x_min - new_x_max);
		*image_height_p = abs(new_y_min - new_y_max);
	}
	//LOG_MSG("Exiting GdaScaleTrans::calcAffineParams");
}

wxString GdaScaleTrans::GetString()
{
	wxString str("GdaScaleTrans: ");
	str << "scale_x=" << scale_x << ", scale_y=" << scale_y;
	str << "\n              trans_x=" << trans_x << ", trans_y=";
	str << trans_y;
	return str;
}

void GdaScaleTrans::transform_back(const wxPoint& src, wxRealPoint& result) const
{
	result.x = (src.x - trans_x) / scale_x ;
	result.y = (src.y - trans_y) / scale_y ;
}

void GdaScaleTrans::transform(const point_2d& src,
							 wxPoint* result) const
{
	result->x = (int) (src.x()*scale_x + trans_x);
	result->y = (int) (src.y()*scale_y + trans_y);
}

void GdaScaleTrans::transform(const wxRealPoint& src, wxPoint* result) const
{
	result->x = (int) (src.x * scale_x + trans_x);
	result->y = (int) (src.y * scale_y + trans_y);
}

void GdaScaleTrans::transform(const wxRealPoint& src, wxRealPoint* result) const
{
	result->x = src.x * scale_x + trans_x;
	result->y = src.y * scale_y + trans_y;
}

void GdaScaleTrans::transform(const wxPoint& src, wxPoint* result) const
{
	result->x = (int) (((double) src.x) * scale_x + trans_x);
	result->y = (int) (((double) src.y) * scale_y + trans_y);
}

void GdaScaleTrans::transform(const Shapefile::Point& src, wxPoint* result) const
{
	result->x = (int) (src.x * scale_x + trans_x);
	result->y = (int) (src.y * scale_y + trans_y);
}

void GdaScaleTrans::transform(const double& src, double* result) const
{
	*result = src * max_scale;
}

GdaShapeAttribs::GdaShapeAttribs(const GdaShapeAttribs& s)
: brush(s.brush), pen(s.pen), x_nudge(s.x_nudge), y_nudge(s.y_nudge)
{
}

GdaShapeAttribs::GdaShapeAttribs(const GdaShapeAttribs* s)
: brush(*GdaConst::default_myshape_brush),
pen(*GdaConst::default_myshape_pen), x_nudge(0), y_nudge(0)
{
	if (!s) return;
	brush = s->brush;
	pen = s->pen;
	x_nudge = s->x_nudge;
	y_nudge = s->y_nudge; 
}

GdaShapeAttribs& GdaShapeAttribs::operator=(const GdaShapeAttribs& s) {
	brush = s.brush;
	pen = s.pen;
	x_nudge = s.x_nudge;
	y_nudge = s.y_nudge;
	return *this;
}

GdaShape::GdaShape() :
	center(0,0), center_o(0.0,0.0), category(0), attribs(0), null_shape(false)
{
}

GdaShape::~GdaShape()
{
	if (attribs) delete attribs;
}

GdaShape::GdaShape(const GdaShape& s) :
	center(s.center), center_o(s.center_o), attribs(0),
	null_shape(s.null_shape)
{
	if (s.attribs) attribs = new GdaShapeAttribs(s.attribs);
}

GdaShape& GdaShape::operator=(const GdaShape& s)
{
	null_shape = s.null_shape;
	center = s.center;
	center_o = s.center_o;
	/** allocate optional attributes if not already allocated.  Attributes
	 will be deleted by destructor */
	if (s.attribs) {
		if (!attribs) attribs = new GdaShapeAttribs;
		attribs->operator=(*s.attribs);
	}
	return *this;
}

void GdaShape::applyScaleTrans(const GdaScaleTrans& A)
{
	A.transform(center_o, &center);
}

void GdaShape::setNudge(int x_nudge, int y_nudge)
{
	if (!attribs) attribs = new GdaShapeAttribs;
	attribs->x_nudge = x_nudge;
	attribs->y_nudge = y_nudge;
}

void GdaShape::setPen(const wxPen& pen)
{
	if (!attribs) attribs = new GdaShapeAttribs;
	attribs->pen = pen;
}

void GdaShape::setBrush(const wxBrush& brush)
{
	if (!attribs) attribs = new GdaShapeAttribs;
	attribs->brush = brush;
}

const wxPen& GdaShape::getPen()
{
	if (!attribs) return *GdaConst::default_myshape_pen;
	return attribs->pen;
}

const wxBrush& GdaShape::getBrush()
{
	if (!attribs) return *GdaConst::default_myshape_brush;
	return attribs->brush;
}

int GdaShape::getXNudge()
{
	if (!attribs) return 0;
	return attribs->x_nudge;
}

int GdaShape::getYNudge()
{
	if (!attribs) return 0;
	return attribs->y_nudge;
}

void GdaShapeAlgs::partsToCount(const std::vector<wxInt32>& parts,
							   int total_points, int* count)
{
	if (total_points == 0 || parts.size() == 0) return;
	//LOG_MSG("Entering GdaShape::partsToCount");
	int last_ind = parts.size()-1;
	for (int i=0; i<last_ind; i++) {
		count[i] = parts[i+1]-parts[i];
		//LOG(count[i]);
		//LOG(parts[i]);
		//LOG(parts[i+1]);
	}
	//int prev = last_ind > 0 ? parts[last_ind-1] : 0;
	count[last_ind] = total_points - parts[last_ind];
	//LOG(total_points);
	//LOG(parts.size());
	//LOG(parts[last_ind]);
	//LOG(last_ind);
	//LOG(count[last_ind]);
	//LOG_MSG("Exiting GdaShape::partsToCount");
}

wxRealPoint GdaShapeAlgs::calculateMeanCenter(GdaPolygon* poly)
{
	if (poly->n_count < 1) return wxRealPoint(0,0);
	if (poly->points_o) {
		return calculateMeanCenter(poly->n, poly->points_o);
	} else {
		return calculateMeanCenter(poly->pc->points);
	}
}

wxRealPoint GdaShapeAlgs::calculateMeanCenter(int n, wxRealPoint* pts)
{
	wxRealPoint c(0.0, 0.0);
	if (pts) {
		for (int i=0; i<n; i++) {
			c.x += pts[i].x;
			c.y += pts[i].y;
		}
		c.x /= (double) n;
		c.y /= (double) n;
	}
	return c;
}

wxRealPoint GdaShapeAlgs::calculateMeanCenter(
	const std::vector<Shapefile::Point>& pts)
{
	wxRealPoint c(0.0, 0.0);
	int n = pts.size();
	for (int i=0; i<n; i++) {
		c.x += pts[i].x;
		c.y += pts[i].y;
	}
	c.x /= (double) n;
	c.y /= (double) n;
	return c;
}


// The only calculates the centroid for the first polygon in the
// description and does not take into acount holes in the polygon.
// also, polygons are assumed to be simple.
wxRealPoint GdaShapeAlgs::calculateCentroid(GdaPolygon* poly)
{
	if (poly->n_count < 1) return wxRealPoint(0,0);
	if (poly->points_o) {
		return calculateCentroid(poly->n, poly->points_o);
	} else {
		return calculateCentroid(poly->count[0], poly->pc->points);
	}
}

wxRealPoint GdaShapeAlgs::calculateCentroid(int n, wxRealPoint* pts)
{
	double area = GdaShapeAlgs::calculateArea(n, pts);
	if (area == 0) return wxRealPoint(pts[0].x, pts[0].y);
	// polygon is a p-gon. Handle case when polygon is not closed
	int p = (pts[0].x==pts[n-1].x && pts[0].y==pts[n-1].y) ? n-1 : n;
	double cx=0, cy=0, d;
	for (int i=0, j=1, k=0; k<p; i=(i+1)%p, j=(j+1)%p, k++) {
		d = (pts[i].x * pts[j].y) - (pts[j].x * pts[i].y);
		cx += (pts[i].x + pts[j].x)*d;
		cy += (pts[i].y + pts[j].y)*d;
	}
	cx /= area*6.0f;
	cy /= area*6.0f;
	return wxRealPoint(cx, cy);
}

wxRealPoint GdaShapeAlgs::calculateCentroid(int n,
	const std::vector<Shapefile::Point>& pts)
{
	double area = GdaShapeAlgs::calculateArea(n, pts);
	if (area == 0) return wxRealPoint(pts[0].x, pts[0].y);
	// polygon is a p-gon. Handle case when polygon is not closed
	int p = (pts[0].x==pts[n-1].x && pts[0].y==pts[n-1].y) ? n-1 : n;
	double cx=0, cy=0, d;
	for (int i=0, j=1, k=0; k<p; i=(i+1)%p, j=(j+1)%p, k++) {
		d = (pts[i].x * pts[j].y) - (pts[j].x * pts[i].y);
		cx += (pts[i].x + pts[j].x)*d;
		cy += (pts[i].y + pts[j].y)*d;
	}
	cx /= area*6.0f;
	cy /= area*6.0f;
	return wxRealPoint(cx, cy);
}

/** Note: if area is returned as negative, then this indicates that the
 polygon coordinates were given in reverse. When this is applied
 to by the calculateCentroid function, the negative area will
 automatically correct for reversed coordinates in the returned
 centroid point. */
double GdaShapeAlgs::calculateArea(int n, wxRealPoint* pts)
{
	if (n <= 2) return 0;
	double a = 0;
	int p = (pts[0].x==pts[n-1].x && pts[0].y==pts[n-1].y) ? n-1 : n;
	for (int i=0, j=1, k=0; k<p; i=(i+1)%p, j=(j+1)%p, k++) {
		a += (pts[i].x * pts[j].y - pts[j].x * pts[i].y);
	}
	return a/2.0f;
}

double GdaShapeAlgs::calculateArea(int n,
								  const std::vector<Shapefile::Point>& pts)
{
	if (n <= 2) return 0;
	double a = 0;
	int p = (pts[0].x==pts[n-1].x && pts[0].y==pts[n-1].y) ? n-1 : n;
	for (int i=0, j=1, k=0; k<p; i=(i+1)%p, j=(j+1)%p, k++) {
		a += (pts[i].x * pts[j].y - pts[j].x * pts[i].y);
	}
	return a/2.0f;
}


/** num_points is an optional parameter.  If num_points < 4, then a reasonable
 number of points to specify the circle is given depending on the radius.
 The program will either use it's own internal scratch wxPoints pnts_array
 is null, or will write to pnts_array if not null.  Note, the size of
 pnts_array needs to be sufficiently large.
 */
wxRegion GdaShapeAlgs::createCircleRegion(const wxPoint& center, double radius,
										 int num_points,
										 wxPoint* pnts_array,
										 int* pnts_array_size)
{
	static const int max_pts = 100;
	static wxPoint scratch_pts[max_pts];
	wxPoint* p = pnts_array ? pnts_array : scratch_pts;
	
	radius = fabs(radius); // ensure radius is non-zero
	if (radius < 1) radius = 1; // ensure radius is greater than 0
	if (radius > 5000) radius = 5000; // ensure radius is at most 5000
	if (num_points > max_pts) num_points = max_pts;
	if (num_points < 4) {
		if (radius <= 10) {
			num_points = 10;
		} else if (radius > 10 && radius <= 100) {
			num_points = 20;
		} else {  // radius > 100
			num_points = 40;
		}
	}
	double slice = ((double) 6.28318)/((double) num_points); // 2*pi/num_pts
	double theta = 0;
	for (int i=0; i<num_points; i++) {
		theta = i * slice;
		p[i].x = radius * cos(theta);
		p[i].y = radius * sin(theta);
		p[i] += center;
	}
	
	if (pnts_array_size) *pnts_array_size = num_points;
	
	return wxRegion(num_points, p);
}

void GdaShapeAlgs::createCirclePolygon(const wxPoint& center, double radius,
									  int num_points, wxPoint* pnts_array,
									  int* pnts_array_size)
{
	static const int max_pts = 100;
	static wxPoint scratch_pts[max_pts];
	wxPoint* p = pnts_array ? pnts_array : scratch_pts;
	
	radius = fabs(radius); // ensure radius is non-zero
	if (radius < 1) radius = 1; // ensure radius is greater than 0
	if (radius > 5000) radius = 5000; // ensure radius is at most 5000
	if (num_points > max_pts) num_points = max_pts;
	if (num_points < 4) {
		if (radius <= 10) {
			num_points = 10;
		} else if (radius > 10 && radius <= 100) {
			num_points = 20;
		} else {  // radius > 100
			num_points = 40;
		}
	}
	double slice = ((double) 6.28318)/((double) num_points); // 2*pi/num_pts
	double theta = 0;
	for (int i=0; i<num_points; i++) {
		theta = i * slice;
		p[i].x = radius * cos(theta);
		p[i].y = radius * sin(theta);
		p[i] += center;
	}
	
	if (pnts_array_size) *pnts_array_size = num_points;
}

/** wxRegions need to have a non-zero area.  This function takes a line
 specifiation and converts it to a 3-pixels across parallelogram. */
wxRegion GdaShapeAlgs::createLineRegion(wxPoint a, wxPoint b)
{
	static wxPoint scratch_pts[4];
	if (a == b) {
		scratch_pts[0] = a + wxPoint(-1,-1);
		scratch_pts[0] = a + wxPoint(-1,1);
		scratch_pts[0] = a + wxPoint(1,1);
		scratch_pts[0] = a + wxPoint(1,-1);
	} else if (abs(a.y - b.y) >= abs(a.x - b.x)) {
		// line is closer to vertical than horizontal
		scratch_pts[0] = a + wxPoint(-1,0);
		scratch_pts[1] = a + wxPoint(1,0);
		scratch_pts[2] = b + wxPoint(1,0);
		scratch_pts[3] = b + wxPoint(-1,0);
	} else {
		// line is closer to horizontal than vertical
		scratch_pts[0] = a + wxPoint(0,-1);
		scratch_pts[1] = a + wxPoint(0,1);
		scratch_pts[2] = b + wxPoint(0,1);
		scratch_pts[3] = b + wxPoint(0,-1);		
	}
	return wxRegion(4, scratch_pts);
}

bool GdaShapeAlgs::pointInPolygon(const wxPoint& pt, int n, const wxPoint* pts)
{
	bool within = false;
	for (int i=0, j=n-1; i<n; j=i++) {
		if (((pts[i].y > pt.y) != (pts[j].y > pt.y)) &&
			(pt.x < (pts[j].x-pts[i].x) * (pt.y-pts[i].y) /
			 (pts[j].y-pts[i].y) + pts[i].x))
		{
			within = !within;
		}
	}
	return within;
}

void GdaShapeAlgs::getBoundingBoxOrig(const GdaPolygon* p, double& xmin,
									 double& ymin, double& xmax, double& ymax)
{
	if (p->pc) {
		xmin = p->pc->box[0];
		ymin = p->pc->box[1];
		xmax = p->pc->box[2];
		ymax = p->pc->box[3];
	} else {
		xmin = p->points_o[0].x;
		xmax = xmin;
		ymin = p->points_o[0].y;
		ymax = ymin;
		for (int i=1; i<p->n; i++) {
			if (p->points_o[i].x < xmin) {
				xmin = p->points_o[i].x;
			} else if (p->points_o[i].x > xmax) {
				xmax = p->points_o[i].x;
			}
			if (p->points_o[i].y < ymin) {
				ymin = p->points_o[i].y;
			} else if (p->points_o[i].y > ymax) {
				ymax = p->points_o[i].y;
			}
		}
	}
}

GdaPoint::GdaPoint()
{
	null_shape = true;
}

GdaPoint::GdaPoint(const GdaPoint& s)
	: GdaShape(s)
{
}

GdaPoint::GdaPoint(wxRealPoint point_o_s)
{
	center = wxPoint((int) point_o_s.x, (int) point_o_s.y); 
	center_o = point_o_s;
}

GdaPoint::GdaPoint(double x_orig, double y_orig)
{
	center = wxPoint((int) x_orig, (int) y_orig); 
	center_o = wxRealPoint(x_orig, y_orig);
}

double GdaPoint::GetX()
{
    return center_o.x;
}

double GdaPoint::GetY()
{
    return center_o.y;
}

bool GdaPoint::pointWithin(const wxPoint& pt)
{
	if (null_shape) return false;
	return ( fabs((double) (center.x - pt.x))
			<= GdaConst::my_point_click_radius &&
			fabs((double) (center.y - pt.y))
			<= GdaConst::my_point_click_radius );
}

bool GdaPoint::regionIntersect(const wxRegion& r)
{
	if (null_shape) return false;
	return r.Contains(center.x-1, center.y-1, 3, 3) != wxOutRegion;
}

//void GdaPoint::applyScaleTrans(const GdaScaleTrans& A)
//{
//	GdaShape::applyScaleTrans(A); // apply affine transform to base class
//	A.transform(center_o, &center);
//}

void GdaPoint::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	wxPoint n_center(center.x+getXNudge(), center.y+getYNudge()); 
	dc.DrawCircle(n_center, GdaConst::my_point_click_radius);
}

GdaCircle::GdaCircle()
{
	null_shape = true;
}

GdaCircle::GdaCircle(const GdaCircle& s)
	: GdaShape(s), radius(s.radius), radius_o(s.radius_o),
	scale_radius(s.scale_radius)
{
}

GdaCircle::GdaCircle(wxRealPoint center_o_s, double radius_o_s,
				   bool scale_radius_s)
	: radius_o(radius_o_s), radius(radius_o_s), scale_radius(scale_radius_s)
{
	center = wxPoint((int) center_o_s.x, (int) center_o_s.y);
	center_o = center_o_s;
}

bool GdaCircle::pointWithin(const wxPoint& pt)
{
	if (null_shape) return false;
	return GenUtils::distance(center, pt) <= radius;
}

bool GdaCircle::regionIntersect(const wxRegion& r)
{
	//long diam = (long) (2*radius);
	//if (r.Contains(center.x - radius, center.y - radius,
	//			   diam, diam) == wxOutRegion) {
	//	return false;
	//} else {
	//	wxRegion circ_reg = GdaShapeAlgs::createCircleRegion(center, radius);
	//	circ_reg.Intersect(r);
	//	return !circ_reg.IsEmpty();
	//}
	return false;
}

void GdaCircle::applyScaleTrans(const GdaScaleTrans& A)
{
	if (null_shape) return;
	GdaShape::applyScaleTrans(A); // apply affine transform to base class
	A.transform(center_o, &center);
	if (scale_radius) A.transform(radius_o, &radius);
}

void GdaCircle::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	wxPoint n_center(center.x+getXNudge(), center.y+getYNudge()); 
	dc.DrawCircle(n_center, radius);
}

GdaRectangle::GdaRectangle()
{
	null_shape = true;
}

GdaRectangle::GdaRectangle(const GdaRectangle& s)
: GdaShape(s), lower_left(s.lower_left), lower_left_o(s.lower_left_o),
upper_right_o(s.upper_right_o), upper_right(s.upper_right_o)
{
}

GdaRectangle::GdaRectangle(wxRealPoint lower_left_o_s,
						 wxRealPoint upper_right_o_s)
: lower_left_o(lower_left_o_s), upper_right_o(upper_right_o_s)
{
	center_o.x = (lower_left_o.x + upper_right_o.x)/2.0;
	center_o.y = (lower_left_o.y + upper_right_o.y)/2.0;
	center = wxPoint((int) center_o.x, (int) center_o.y);
}

bool GdaRectangle::pointWithin(const wxPoint& pt)
{
	if (null_shape) return false;
	return (pt.x >= lower_left.x && pt.x <= upper_right.x &&
			pt.y <= lower_left.y && pt.y >= upper_right.y);
}

bool GdaRectangle::regionIntersect(const wxRegion& r)
{
	return false;
}

void GdaRectangle::applyScaleTrans(const GdaScaleTrans& A)
{
	if (null_shape) return;
	GdaShape::applyScaleTrans(A); // apply affine transform to base class
	A.transform(lower_left_o, &lower_left);
	A.transform(upper_right_o, &upper_right);
}

void GdaRectangle::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	dc.DrawRectangle(lower_left.x+getXNudge(), lower_left.y+getYNudge(),
					 upper_right.x - lower_left.x,
					 upper_right.y - lower_left.y);
}

GdaPolygon::GdaPolygon() : points(0), points_o(0), count(0)
{
	null_shape = true;
}

GdaPolygon::GdaPolygon(const GdaPolygon& s)
	: GdaShape(s), //region(s.region),
	n(s.n), pc(s.pc), points_o(s.points_o),
	n_count(s.n_count), all_points_same(s.all_points_same),
	bb_ll_o(s.bb_ll_o), bb_ur_o(s.bb_ur_o), count(0)
{
	if (null_shape) return;
	points = new wxPoint[n];
	for (int i=0; i<n; i++) {
		points[i].x = s.points[i].x;
		points[i].y = s.points[i].y;
	}
	if (s.points_o) {
		points_o = new wxRealPoint[n];
		for (int i=0; i<n; i++) {
			points_o[i].x = s.points_o[i].x;
			points_o[i].y = s.points_o[i].y;
		}
	}
	count = new int[s.n_count];
	for (int i=0; i<s.n_count; i++) {
		count[i] = s.count[i];
	}
}

/** This constructs a polygon with no holes and only one region.  The
 memory for the original set of points is also maintained internally and
 will be deleted when the constructor is called. */
GdaPolygon::GdaPolygon(int n_s, wxRealPoint* points_o_s)
	: n(n_s), points_o(0), pc(0), points(0), n_count(1),
	all_points_same(false), count(0)
{
	if (points_o_s == 0 || n == 0) {
		null_shape = true;
		return;
	}
	count = new int[1];
	count[0] = n_s;
	points = new wxPoint[n_s];
	points_o = new wxRealPoint[n_s];
	n = points && points_o_s ? n_s : 0;
	for (int i=0; i<n; i++) {
		points_o[i].x = points_o_s[i].x;
		points_o[i].y = points_o_s[i].y;
	}
	for (int i=0; i<n; i++) {
		points[i].x = (int) points_o_s[i].x;
		points[i].y = (int) points_o_s[i].y;
	}
	center_o = GdaShapeAlgs::calculateMeanCenter(n, points_o);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
	bb_ll_o = center_o;
	bb_ur_o = center_o;
	for (int i=0; i<n; i++) {
		if (points_o[i].x < bb_ll_o.x) bb_ll_o.x = points_o[i].x;
		if (points_o[i].x > bb_ur_o.x) bb_ur_o.x = points_o[i].x;
		if (points_o[i].y < bb_ll_o.y) bb_ll_o.y = points_o[i].y;
		if (points_o[i].y > bb_ur_o.y) bb_ur_o.y = points_o[i].y;
	}
	//region = wxRegion(n, points);
}

/** This constructs a potentially multi-part polygon where each polygon
 part might contain holes.  Only a pointer to the original data is
 kept, and this memory is not deleted in the destructor. */
GdaPolygon::GdaPolygon(Shapefile::PolygonContents* pc_s)
  : n(0), points_o(0), pc(pc_s), points(0), all_points_same(false), count(0)
{
	assert(pc);
	if (pc->shape_type == 0 || pc->num_points == 0) {
		null_shape = true;
		return;
	}
	count = new int[pc->num_parts];
	// initialize count array
	GdaShapeAlgs::partsToCount(pc->parts, pc->num_points, count);
	n_count = pc->num_parts;
	n = pc->num_points;
	points = new wxPoint[n];
    //points_o = new wxRealPoint[n];
	for (int i=0; i<n; i++) {
		points[i].x = (int) pc->points[i].x;
		points[i].y = (int) pc->points[i].y;
        //points_o[i].x = pc->points[i].x;
        //points_o[i].y = pc->points[i].y;
	}
	center_o = GdaShapeAlgs::calculateMeanCenter(pc->points);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
	bb_ll_o = center_o;
	bb_ur_o = center_o;
	for (int i=0; i<n; i++) {
		if (pc->points[i].x < bb_ll_o.x) bb_ll_o.x = pc->points[i].x;
		if (pc->points[i].x > bb_ur_o.x) bb_ur_o.x = pc->points[i].x;
		if (pc->points[i].y < bb_ll_o.y) bb_ll_o.y = pc->points[i].y;
		if (pc->points[i].y > bb_ur_o.y) bb_ur_o.y = pc->points[i].y;
	}
	//region = wxRegion(n, points);
}


GdaPolygon::~GdaPolygon()
{
	if (points) {
		delete [] points;
		points = 0;
	}
	if (points_o) {
		delete [] points_o;
		points_o = 0;
	}
	if (count) {
		delete [] count;
		count = 0;
	}
}

bool GdaPolygon::pointWithin(const wxPoint& pt)
{
	if (null_shape) return false;
	if (all_points_same) {
		return pt == center;
	} else {
		return GdaShapeAlgs::pointInPolygon(pt, n, points);
	}
	//return region.Contains(pt) != wxOutRegion;
}

bool GdaPolygon::regionIntersect(const wxRegion& r)
{
	//wxRegion reg(region);
	//reg.Intersect(r);
	//return !reg.IsEmpty();
	return false;
}

void GdaPolygon::applyScaleTrans(const GdaScaleTrans& A)
{
	if (null_shape) return;
	GdaShape::applyScaleTrans(A); // apply affine transform to base class
	all_points_same = true;
	wxPoint tpt;
	A.transform(bb_ll_o, &tpt);
	if (tpt == center) A.transform(bb_ur_o, &tpt);
	if (tpt == center) return;
	if (points_o) {
		for (int i=0; i<n; i++) {
			A.transform(points_o[i], &(points[i]));
			if (points[i] != center) all_points_same = false;
		}
		//region = wxRegion(n, points);
	} else {
		for (int i=0; i<n; i++) {
			A.transform(pc->points[i], &(points[i]));
			if (points[i] != center) all_points_same = false;
		}
		//region = wxRegion(n, points);  // MMM: needs to support multi-part
	}
}

wxRealPoint GdaPolygon::CalculateCentroid(int n, wxRealPoint* pts)
{
	double area = 0;
	double sum_x = 0;
	double sum_y = 0;
	for (int cnt = 0; cnt < n-1; cnt++) {
		area += (pts[cnt].x * pts[(cnt+1)%n].y - pts[(cnt+1)%n].x * pts[cnt].y);
		sum_x += (pts[cnt].x + pts[(cnt+1)%n].x) *
		(pts[cnt].x * pts[(cnt+1)%n].y - pts[(cnt+1)%n].x * pts[cnt].y);
		sum_y += (pts[cnt].y + pts[(cnt+1)%n].y) *
		(pts[cnt].x * pts[(cnt+1)%n].y - pts[(cnt+1)%n].x * pts[cnt].y);
	}
	return wxRealPoint(sum_x / (3 * area), sum_y / (3 * area));
}

void GdaPolygon::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	if (n_count > 1) {
		dc.DrawPolyPolygon(n_count, count, points);
	} else {
		dc.DrawPolygon(n, points);
	}
}


GdaPolyLine::GdaPolyLine()
	: n(2), pc(0), n_count(1), count(0), points(0), points_o(0)
{
	null_shape = true;
	return;
	
	/*
	double x1=0, y1=0, x2=100, y2=100; 
	count = new int[1];
	count[0] = n;
	points = new wxPoint[n];
	points_o = new wxRealPoint[n];
	points_o[0].x = x1;
	points_o[0].y = y1;
	points_o[1].x = x2;
	points_o[1].y = y2;
	points[0].x = (int) x1;
	points[0].y = (int) y1;
	points[1].x = (int) x2;
	points[1].y = (int) y2;
	
	center_o = GdaShapeAlgs::calculateMeanCenter(n, points_o);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
	
	//region = GdaShapeAlgs::createLineRegion(points[0], points[1]);
	//for (int i=1; i<n-1; i++) {
	//	region.Union(GdaShapeAlgs::createLineRegion(points[i], points[i+1]));
	//}
	 */
}

GdaPolyLine::GdaPolyLine(const GdaPolyLine& s)
	: GdaShape(s), //region(s.region),
	n(s.n), pc(s.pc), points_o(s.points_o),
	n_count(s.n_count), points(0), count(0)
{
	if (null_shape) return;
	points = new wxPoint[n];
	for (int i=0; i<n; i++) {
		points[i].x = s.points[i].x;
		points[i].y = s.points[i].y;
	}
	if (s.points_o) {
		points_o = new wxRealPoint[n];
		for (int i=0; i<n; i++) {
			points_o[i].x = s.points_o[i].x;
			points_o[i].y = s.points_o[i].y;
		}
	}
	count = new int[s.n_count];
	for (int i=0; i<s.n_count; i++) {
		count[i] = s.count[i];
	}
}

/** This constructs a single polyline rather than (possibly) several disjoint
 polylines like the PolyLineContents data structure allows for. The
 memory for the original set of points is also maintained internally and
 will be deleted when the destructor is called. */
GdaPolyLine::GdaPolyLine(int n_s, wxRealPoint* points_o_s)
	: n(n_s), pc(0), n_count(1), count(0), points_o(0), points(0)
{
	if (n == 0 || points_o_s == 0) {
		null_shape = true;
		return;
	}
	count = new int[1];
	count[0] = n;
	points = new wxPoint[n];
	points_o = new wxRealPoint[n];
	for (int i=0; i<n; i++) {
		points_o[i].x = points_o_s[i].x;
		points_o[i].y = points_o_s[i].y;
	}	
	for (int i=0; i<n; i++) {
		points[i].x = (int) points_o_s[i].x;
		points[i].y = (int) points_o_s[i].y;
	}
	
	center_o = GdaShapeAlgs::calculateMeanCenter(n, points_o);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
	
	//if (n>1) {
	//	region = GdaShapeAlgs::createLineRegion(points[0], points[1]);
	//	for (int i=1; i<n-1; i++) {
	//		region.Union(GdaShapeAlgs::createLineRegion(points[i], points[i+1]));
	//	}
	//}
}

GdaPolyLine::GdaPolyLine(double x1, double y1, double x2, double y2)
	: n(2), pc(0), n_count(1), points_o(0), points(0), count(0)
{
	count = new int[1];
	count[0] = n;
	points = new wxPoint[n];
	points_o = new wxRealPoint[n];
	points_o[0].x = x1;
	points_o[0].y = y1;
	points_o[1].x = x2;
	points_o[1].y = y2;
	points[0].x = (int) x1;
	points[0].y = (int) y1;
	points[1].x = (int) x2;
	points[1].y = (int) y2;

	center_o = GdaShapeAlgs::calculateMeanCenter(n, points_o);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
	
	//region = GdaShapeAlgs::createLineRegion(points[0], points[1]);
	//for (int i=1; i<n-1; i++) {
	//	region.Union(GdaShapeAlgs::createLineRegion(points[i], points[i+1]));
	//}
}

/** This constructs a potentially multi-part polyline. Only a pointer to the
 original data is kept, and this memory is not deleted in the destructor. */
GdaPolyLine::GdaPolyLine(Shapefile::PolyLineContents* pc_s)
	: n(0), points_o(0), pc(pc_s), points(0), count(0)
{
	assert(pc);
	if (pc->shape_type == 0 || pc->num_points == 0) {
		null_shape = true;
		return;
	}
	count = new int[pc->num_parts];
	// initialize count array
	GdaShapeAlgs::partsToCount(pc->parts, pc->num_points, count);
	n_count = pc->num_parts;
	n = pc->num_points;

	points = new wxPoint[n];
	for (int i=0; i<n; i++) {
		points[i].x = (int) pc->points[i].x;
		points[i].y = (int) pc->points[i].y;
	}
	
	//int chunk_index = 0;  // will have the initial index of each part
	//for (int h=0; h<n_count; h++) {
	//	if (count[h] > 1) {  // ensure this is a valid part
	//		region.Union(GdaShapeAlgs::createLineRegion(points[chunk_index],
	//												   points[chunk_index+1]));
	//		for (int i=1; i<n-1; i++) {
	//			region.Union(GdaShapeAlgs::createLineRegion(points[chunk_index+i],
	//													   points[chunk_index+i+1]));
	//		}
	//	}
	//	chunk_index += count[h]; // increment to next part
	//}
	
	center_o = GdaShapeAlgs::calculateMeanCenter(pc->points);
	center.x = (int) center_o.x;
	center.y = (int) center_o.y;
}

GdaPolyLine::~GdaPolyLine()
{
	if (points) delete [] points; points = 0;
	if (points_o) delete [] points_o; points_o = 0;
	if (count) delete [] count; count = 0;
}

GdaPolyLine& GdaPolyLine::operator=(const GdaPolyLine& s)
{
	//LOG_MSG("Entering GdaPolyLine::operator=");
	GdaShape::operator=(s);
	if (points) delete [] points; points = 0;
	if (points_o) delete [] points_o; points_o = 0;
	if (count) delete [] count; count = 0;
	if (s.null_shape) return *this;
	if (s.points) {
		points = new wxPoint[s.n];
		for (int i=0, iend=s.n; i<iend; i++) {
			points[i] = s.points[i];
		}
	}
	if (s.points_o) {
		points_o = new wxRealPoint[n];
		for (int i=0, iend=s.n; i<iend; i++) {
			points_o[i] = s.points_o[i];
		}
	}
	if (s.count) {
		count = new int[s.n_count];
		for (int i=0, iend=s.n_count; i<iend; i++) {
			count[i] = s.count[i];
		}
	}
	n = s.n;
	n_count = s.n_count;
	pc = s.pc;
	//region = s.region;
	return *this;
	//LOG_MSG("Exiting GdaPolyLine::operator=");
}

bool GdaPolyLine::pointWithin(const wxPoint& pt)
{
	if (null_shape) return false;
	const double r = 3.0; // point radius
	wxRealPoint hp;
	double hp_rad;
	for (int j=0, its=n-1; j<its; j++) {
		hp.x = (points[j].x + points[j+1].x)/2.0;
		hp.y = (points[j].y + points[j+1].y)/2.0;
		hp_rad = GenUtils::distance(points[j],
									points[j+1])/2.0;
		if ((GenUtils::pointToLineDist(pt, points[j], points[j+1]) <= r) &&
			(GenUtils::distance(hp, pt) <= hp_rad + r)) return true;
	}
	return false;
}

bool GdaPolyLine::regionIntersect(const wxRegion& r)
{
	//wxRegion reg(region);
	//reg.Intersect(r);
	//return !reg.IsEmpty();
	return false;
}

void GdaPolyLine::applyScaleTrans(const GdaScaleTrans& A)
{
	if (null_shape) return;
	GdaShape::applyScaleTrans(A); // apply affine transform to base class
	if (points_o) {
		for (int i=0; i<n; i++) {
			A.transform(points_o[i], &(points[i]));
		}
		//if (n>1) {
		//	region = GdaShapeAlgs::createLineRegion(points[0], points[1]);
		//	for (int i=1; i<n-1; i++) {
		//		region.Union(GdaShapeAlgs::createLineRegion(points[i], points[i+1]));
		//	}
		//}
	} else {
		for (int i=0; i<n; i++) {
			A.transform(pc->points[i], &(points[i]));
		}
		//region = wxRegion(); // create an empty initial region
		//int chunk_index = 0;  // will have the initial index of each part
		//for (int h=0; h<n_count; h++) {
		//	if (count[h] > 1) {  // ensure this is a valid part
		//		region.Union(GdaShapeAlgs::createLineRegion(points[chunk_index],
		//												   points[chunk_index+1]));
		//		for (int i=1; i<n-1; i++) {
		//			region.Union(GdaShapeAlgs::createLineRegion(points[chunk_index+i],
		//													   points[chunk_index+i+1]));
		//		}
		//	}
		//	chunk_index += count[h]; // increment to next part
		//}
	}
	
	//region = wxRegion(n, points);
}

void GdaPolyLine::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	int nx = getXNudge();
	int ny = getYNudge();
	if (n > 1) {
		for (int i=0, its=n-1; i<its; i++) {
			//LOG(i);
			//LOG(points[i].x);
			//LOG(points[i].y);
			//LOG(points[i+1].x);
			//LOG(points[i+1].y);
			//dc.DrawLine(45, 50, 200, 16);
			//dc.DrawLine(200, 16, 200, 70);
			//dc.DrawLine(25, 235, 250, 130);
			//dc.DrawLine(250, 130, 475, 25);
			dc.DrawLine(points[i].x+nx, points[i].y+ny,
						points[i+1].x+nx, points[i+1].y+ny);
		}
	} else {
		dc.DrawPoint(points[0].x+nx, points[0].y+ny);
	}
}

wxString GdaPolyLine::printDetails()
{
	wxString s;
	s << "GdaPolyLine attribs: ";
	s << "\n  null_shape : " << null_shape;
	s << "\n  n : " << n;
	s << "\n  n_count : " << n_count;
	s << "\n  count[0] : " << count[0];
	s << "\n  points[0].x : " << points[0].x;
	s << "\n  points[0].y : " << points[0].y;
	return s;
}

GdaRay::GdaRay()
{
	null_shape = true;
}

GdaRay::GdaRay(const GdaRay& s)
	: GdaShape(s), degs_rot_cc_from_horiz(s.degs_rot_cc_from_horiz),
	length(s.length)
{
}

GdaRay::GdaRay(wxRealPoint center_o_s, double degs_rot_cc_from_horiz_s,
			 int length_s)
: degs_rot_cc_from_horiz(degs_rot_cc_from_horiz_s), length(length_s)
{
	center = wxPoint((int) center_o_s.x, (int) center_o_s.y);
	center_o = center_o_s;
}

bool GdaRay::pointWithin(const wxPoint& pt)
{
	// should use line intersection instead
	if (null_shape) return false;
	return GenUtils::distance(center, pt) <= length;
}

bool GdaRay::regionIntersect(const wxRegion& r)
{
	return false;
}

void GdaRay::applyScaleTrans(const GdaScaleTrans& A)
{
	if (null_shape) return;
	GdaShape::applyScaleTrans(A); // apply affine transform to base class
	A.transform(center_o, &center);
}

void GdaRay::paintSelf(wxDC& dc)
{
	if (null_shape) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	
	double dx = length;
	double dy = 0;
	
	// now rotate the vector (dx,dy) by deg_rot_cc_from_horiz
	double rad = degs_rot_cc_from_horiz * 0.0174532925; // pi/180 = 0.1745...
	double c_rad = cos(rad);
	double s_rad = sin(rad);
	double rot_dx = c_rad*dx - s_rad*dy;
	double rot_dy = -(s_rad*dx + c_rad*dy);
	
	wxPoint n_center(center.x+getXNudge(), center.y+getYNudge());
	dc.DrawLine(center.x+getXNudge(), center.y+getYNudge(),
				center.x+getXNudge()+floor(rot_dx+0.5),
				center.y+getYNudge()+floor(rot_dy+0.5));
}


GdaShapeText::GdaShapeText()
: text(""), font(*GdaConst::medium_font),
	ref_pt(0,0), ref_pt_o(0,0),
	degs_rot_cc_from_horiz(0),
	degs_rot_cc_from_horiz_o(0),
	horiz_align(h_center), vert_align(v_center),
	hidden(false)
{
	for (int i=0; i<5; i++) { bb_poly[i].x = 0; bb_poly[i].y = 0; }
}

GdaShapeText::GdaShapeText(wxString text_s, wxFont font_s, const wxRealPoint& ref_pt_s,
			   double degs_rot_cc_from_horiz_s,
			   HorizAlignment h_align, VertAlignment v_align,
			   int x_nudge, int y_nudge)
	: text(text_s), font(font_s),
	ref_pt(ref_pt_s), ref_pt_o(ref_pt_s),
	degs_rot_cc_from_horiz(degs_rot_cc_from_horiz_s),
	degs_rot_cc_from_horiz_o(degs_rot_cc_from_horiz_s),
	horiz_align(h_align), vert_align(v_align), hidden(false)
{
	setNudge(x_nudge, y_nudge);
	for (int i=0; i<5; i++) { bb_poly[i].x = 0; bb_poly[i].y = 0; }
}

GdaShapeText::GdaShapeText(const GdaShapeText& s)
	: GdaShape(s), text(s.text), font(s.font),
	ref_pt(s.ref_pt), ref_pt_o(s.ref_pt_o),
	degs_rot_cc_from_horiz(s.degs_rot_cc_from_horiz),
	degs_rot_cc_from_horiz_o(s.degs_rot_cc_from_horiz_o),
	horiz_align(s.horiz_align), vert_align(s.vert_align),
	hidden(s.hidden)
{
	for (int i=0; i<5; i++) bb_poly[i] = s.bb_poly[i];
}

GdaShapeText& GdaShapeText::operator=(const GdaShapeText& s)
{
	GdaShape::operator=(s);
	text = s.text;
	font = s.font;
	ref_pt = s.ref_pt;
	ref_pt_o = s.ref_pt_o;
	degs_rot_cc_from_horiz = s.degs_rot_cc_from_horiz;
	degs_rot_cc_from_horiz_o = s.degs_rot_cc_from_horiz_o;
	horiz_align = s.horiz_align;
	vert_align = s.vert_align;
	hidden = s.hidden;
	for (int i=0; i<5; i++) bb_poly[i] = s.bb_poly[i];
	
	return *this;
}

bool GdaShapeText::pointWithin(const wxPoint& pt)
{
	return GdaShapeAlgs::pointInPolygon(pt, 5, bb_poly);
}

void GdaShapeText::paintSelf(wxDC& dc)
{
	//LOG_MSG("Entering GdaShapeText::paintSelf");
	if (hidden) return;
		
	wxPen pen = getPen();
	pen.SetWidth(1);
	dc.SetPen(pen);
	dc.SetBrush(getBrush());
	dc.SetFont(font);
	dc.SetTextForeground(getPen().GetColour());

	//wxString text("This is a very big test.");
	//wxSize extent(dc.GetTextExtent(text));
	//wxRealPoint t_ref_pt(300,200);
	//dc.DrawCircle(wxPoint(t_ref_pt.x, t_ref_pt.y), 10);
	//dc.DrawPoint(t_ref_pt.x, t_ref_pt.y);	
	
	wxPoint text_pos;
	wxRealPoint nudged_ref_pt(ref_pt.x+getXNudge(), ref_pt.y+getYNudge());
	text_pos = wxPoint(GdaShapeText::calcRefPoint(dc, text, font, nudged_ref_pt,
											degs_rot_cc_from_horiz,
											horiz_align, vert_align));
	dc.DrawRotatedText(text, text_pos.x, text_pos.y,
					   degs_rot_cc_from_horiz);
	
	// Calculate the bounding polygon.  This is needed for pointWithin
	wxSize extent(dc.GetTextExtent(text));
	double x = extent.GetWidth();
	double y = extent.GetHeight();
	// rotate vector (x,0) by deg_rot_cc_from _horiz
	double theta = degs_rot_cc_from_horiz / 57.2957796;
	double xp_x = x * cos(theta);
	double yp_x = x * sin(theta);
	// now calculate upper left and upper right points
	wxPoint ul(text_pos);
	wxPoint ur(ul.x+xp_x, ul.y-yp_x);
	
	// rotate vector (0,y) by deg_rot_cc_from_horiz
	double xp_y = -y * sin(theta);
	double yp_y = y * cos(theta);
	// now calculate lower left point and lower right point
	wxPoint ll(ul.x-xp_y, ul.y+yp_y);
	wxPoint lr(ll.x+xp_x, ll.y-yp_x);
	
	bb_poly[0] = ul;
	bb_poly[1] = ur;
	bb_poly[2] = lr;
	bb_poly[3] = ll;
	bb_poly[4] = ul;
	
	// draw the full text bounding box for debugging purposes
	//dc.SetBrush(*wxTRANSPARENT_BRUSH);
	//dc.DrawPolygon(5, bb_poly);
	//dc.DrawLine(ul, ur);
	//dc.DrawLine(ur, lr);
	//dc.DrawLine(lr, ll);
	//dc.DrawLine(ll, ul);
	
	//wxPoint t_ref_pt(ref_pt.x+getXNudge(), ref_pt.y+getYNudge());
	//dc.DrawCircle(t_ref_pt, 6);
	//dc.DrawPoint(t_ref_pt);
	
	dc.SetTextForeground(*wxBLACK);
	//LOG_MSG("Exiting GdaShapeText::paintSelf");
}

void GdaShapeText::applyScaleTrans(const GdaScaleTrans& A)
{
	A.transform(ref_pt_o, &ref_pt);
	// adjust degs_rot_cc_from_horiz according to A.scale_x and A.scale_y
	// begin by calculating the unit vector that represents
	// the rotation from horizontal
	double rads = degs_rot_cc_from_horiz_o / 57.2957796;
	double x_o = cos(rads);
	double y_o = sin(rads);
	double x = x_o * A.scale_x;
	double y = y_o * -A.scale_y;
	double dist_x_y = GenUtils::distance(wxRealPoint(0,0), wxRealPoint(x,y));
	if (dist_x_y <= 4*DBL_MIN) return;
	// renormalize to unit length
	x /= dist_x_y;
	y /= dist_x_y;
	// now calculate cc degrees of rotation from (1,0) for (x,y)
	degs_rot_cc_from_horiz = acos(x) * 57.2957796;
	if (y < 0) degs_rot_cc_from_horiz = 360.0 - degs_rot_cc_from_horiz;
}

/** degs_rot_cc_from_horiz = degrees of rotation counter clockwise from
 horizontal. */
wxPoint GdaShapeText::calcRefPoint(wxDC& dc, const wxString& text,
							 const wxFont& font,
							 const wxRealPoint& ref_pt,
							 double degs_rot_cc_from_horiz,
							 HorizAlignment h_align,
							 VertAlignment v_align)
{
	//wxFont orig_font = dc.GetFont();
	//dc.SetFont(font);
	wxSize extent(dc.GetTextExtent(text));
	double dx, dy;
	if (h_align == left ) {
		dx = 0;
	} else if ( h_align == h_center ) {
		dx = - (double) extent.GetWidth() / 2.0;
	} else {
		dx = - (double) extent.GetWidth();
	}
	
	if ( v_align == top ) {
		dy = 0;
	} else if ( v_align == v_center ) {
		dy = (double) extent.GetHeight() / 2.0;
	} else {
		dy = (double) extent.GetHeight();
	}	
	//dc.DrawLine(ref_pt.x, ref_pt.y, ref_pt.x + dx, ref_pt.y + dy);
	//dc.DrawCircle(wxPoint(ref_pt.x + dx, ref_pt.y + dy), 5);
	
	// now rotate the vector (dx,dy) by deg_rot_cc_from_horiz
	double rad = degs_rot_cc_from_horiz * 0.0174532925; // pi/180 = 0.1745...
	double c_rad = cos(rad);
	double s_rad = sin(rad);
	double rot_dx = c_rad*dx - s_rad*dy;
	double rot_dy = -(s_rad*dx + c_rad*dy);
	
	// add this to the original ref_pt.
	wxPoint ret_pt(ref_pt.x + rot_dx, ref_pt.y + rot_dy);
	//dc.DrawLine(ref_pt.x, ref_pt.y, ret_pt.x, ret_pt.y);
	//dc.DrawCircle(ret_pt, 5);

	//dc.SetFont(orig_font);
	return ret_pt;
}


GdaShapeTable::GdaShapeTable()
: vals(0), attributes(0), rows(0), cols(0), font(*GdaConst::small_font),
	ref_pt(0,0), ref_pt_o(0,0),
	horiz_align(GdaShapeText::h_center), vert_align(GdaShapeText::v_center),
	cell_h_align(GdaShapeText::h_center), cell_v_align(GdaShapeText::v_center),
	row_gap(3), col_gap(10), hidden(false)
{
}

GdaShapeTable::GdaShapeTable(const std::vector<wxString>& vals_s,
				 const std::vector<CellAttrib>& attributes_s,
				 int rows_s, int cols_s, wxFont font_s,
				 const wxRealPoint& ref_pt_s,
				 GdaShapeText::HorizAlignment horiz_align_s,
				 GdaShapeText::VertAlignment vert_align_s,
				 GdaShapeText::HorizAlignment cell_h_align_s,
				 GdaShapeText::VertAlignment cell_v_align_s,
				 int row_gap_s, int col_gap_s,
				 int x_nudge, int y_nudge)
	: vals(vals_s), attributes(attributes_s),
	rows(rows_s), cols(cols_s), font(font_s),
	ref_pt(ref_pt_s), ref_pt_o(ref_pt_s),
	horiz_align(horiz_align_s), vert_align(vert_align_s),
	cell_h_align(cell_h_align_s), cell_v_align(cell_v_align_s),
	row_gap(row_gap_s), col_gap(col_gap_s),
	hidden(false)
{
	setNudge(x_nudge, y_nudge);
}

GdaShapeTable::GdaShapeTable(const GdaShapeTable& s)
	: GdaShape(s), vals(s.vals), attributes(s.attributes),
	rows(s.rows), cols(s.cols), font(s.font),
	ref_pt(s.ref_pt), ref_pt_o(s.ref_pt_o),
	horiz_align(s.horiz_align), vert_align(s.vert_align),
	cell_h_align(s.cell_h_align), cell_v_align(s.cell_v_align),
	row_gap(s.row_gap), col_gap(s.col_gap),
	hidden(s.hidden)
{
}

GdaShapeTable& GdaShapeTable::operator=(const GdaShapeTable& s)
{
	GdaShape::operator=(s);
	vals = s.vals;
	attributes = s.attributes;
	rows = s.rows;
	cols = s.cols;
	font = s.font;
	ref_pt = s.ref_pt;
	ref_pt_o = s.ref_pt_o;
	horiz_align = s.horiz_align;
	vert_align = s.vert_align;
	cell_h_align = s.cell_h_align;
	cell_v_align = s.cell_v_align;
	row_gap = s.row_gap;
	col_gap = s.col_gap;
	hidden = s.hidden;
	
	return *this;
}

void GdaShapeTable::paintSelf(wxDC& dc)
{
	using namespace std;
	//LOG_MSG("Entering GdaShapeTable::paintSelf");
	if (hidden || vals.size() == 0 || rows*cols != vals.size()) return;
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	dc.SetFont(font);
	dc.SetTextForeground(getPen().GetColour());
	
	// we know that rows>0 and cols>0 and that rows*cols == vals.size()
	// let's find the max vertical extent and the max horizontal
	// extent for each row and each column
	vector<int> row_h(rows, 0);
	vector<int> col_w(cols, 0);
	vector<wxSize> extents(rows*cols); 
	for (int i=0; i<rows; i++) {
		for (int j=0; j<cols; j++) {
			int ij = i*cols+j;
			extents[ij] = dc.GetTextExtent(vals[ij]);
			//wxString msg("extents[");
			//msg << i << "," << j << "] = " << extents[ij].GetWidth();
			//msg << "," << extents[ij].GetHeight();
			//LOG_MSG(msg);			
			
			if (row_h[i] < extents[ij].GetHeight()) {
				row_h[i] = extents[ij].GetHeight();
			}
			if (col_w[j] < extents[ij].GetWidth()) {
				col_w[j] = extents[ij].GetWidth();
			}
		}
	}
	//for (int i=0; i<rows; i++) {
		//wxString msg("row_h[");
		//msg << i << "] = " << row_h[i];
		//LOG_MSG(msg);
	//}
	//for (int i=0; i<cols; i++) {
	//	wxString msg("col_w[");
	//	msg << i << "] = " << col_w[i];
	//	LOG_MSG(msg);		
	//}
	
	vector<wxPoint> d(rows*cols);
	for (int i=0; i<rows; i++) {
		for (int j=0; j<cols; j++) {
			int ij = i*cols+j;
			if (cell_h_align == GdaShapeText::left) {
				d[ij].x = 0;
			} else if (cell_h_align == GdaShapeText::h_center) {
				d[ij].x = (col_w[j]-extents[ij].GetWidth())/2;
			} else {
				d[ij].x = col_w[j]-extents[ij].GetWidth();
			}
			if (cell_v_align == GdaShapeText::top) {
				d[ij].y = 0;
			} else if (cell_v_align == GdaShapeText::v_center) {
				d[ij].y = (row_h[i]-extents[ij].GetHeight())/2;
			} else {
				d[ij].y = row_h[i]-extents[ij].GetHeight();
			}
			//wxString msg("d[");
			//msg << i << "," << j << "] = " << d[ij].x;
			//msg << "," << d[ij].y;
			//LOG_MSG(msg);			
		}
	}
	vector<wxPoint> pos(rows*cols);
	int y_offset = 0;
	for (int i=0; i<rows; i++) {
		int x_offset = 0;
		for (int j=0; j<cols; j++) {
			int ij = i*cols+j;
			pos[ij] = wxPoint(x_offset + d[ij].x, y_offset + d[ij].y);
			x_offset += col_gap + col_w[j];
		}
		y_offset += row_gap + row_h[i];
	}
	
	int table_width = 0;
	int table_height = 0;
	for (int i=0; i<cols; i++) table_width += col_w[i];
	for (int i=0; i<rows; i++) table_height += row_h[i];
	table_width += (cols-1)*col_gap;
	table_height += (rows-1)*row_gap;
	
	wxPoint n_ref_pt;
	n_ref_pt.x = ref_pt.x;
	n_ref_pt.y = ref_pt.y;
	if (horiz_align == GdaShapeText::h_center) {
		n_ref_pt.x -= table_width/2;
	} else if (horiz_align == GdaShapeText::right) {
		n_ref_pt.x -= table_width;
	}
	if (vert_align == GdaShapeText::v_center) {
		n_ref_pt.y += table_height/2;
	} else if (vert_align == GdaShapeText::bottom) {
		n_ref_pt.y += table_height;
	}
	n_ref_pt.x += getXNudge();
	n_ref_pt.y += getYNudge();
	
	bool attribs_defined = (attributes.size() == vals.size());
	
	for (int i=0; i<rows; i++) {
		for (int j=0; j<cols; j++) {
			int ij = i*cols+j;
			if (attribs_defined) {
				dc.SetTextForeground(attributes[ij].color);
			}
			dc.DrawText(vals[ij], pos[ij]+n_ref_pt);
			//wxString msg(vals[ij]);
			//msg << " = " << pos[ij].x << "," << pos[ij].y;
			//LOG_MSG(msg);
		}
	}
	
	dc.SetTextForeground(*wxBLACK);
	//LOG_MSG("Exiting GdaShapeTable::paintSelf");
}

void GdaShapeTable::GetSize(wxDC& dc, int& w, int& h)
{
	using namespace std;
	if (hidden || vals.size() == 0 || rows*cols != vals.size()) return;
	dc.SetPen(getPen());
	dc.SetFont(font);
	
	vector<int> row_h(rows, 0);
	vector<int> col_w(cols, 0);
	vector<wxSize> extents(rows*cols); 
	for (int i=0; i<rows; i++) {
		for (int j=0; j<cols; j++) {
			int ij = i*cols+j;
			extents[ij] = dc.GetTextExtent(vals[ij]);
			if (row_h[i] < extents[ij].GetHeight()) {
				row_h[i] = extents[ij].GetHeight();
			}
			if (col_w[j] < extents[ij].GetWidth()) {
				col_w[j] = extents[ij].GetWidth();
			}
		}
	}
	
	w = 0;
	h = 0;
	for (int i=0; i<cols; i++) w += col_w[i];
	for (int i=0; i<rows; i++) h += row_h[i];
	w += (cols-1)*col_gap;
	h += (rows-1)*row_gap;
}

void GdaShapeTable::applyScaleTrans(const GdaScaleTrans& A)
{
	A.transform(ref_pt_o, &ref_pt);
}

GdaAxis::GdaAxis()
	: caption(wxEmptyString), is_horizontal(true),
	caption_font(*GdaConst::small_font), font(*GdaConst::small_font),
	hidden(false), hide_scale_values(false)
{
}	

GdaAxis::GdaAxis(const GdaAxis& s)
	: GdaShape(s), caption(s.caption), scale(s.scale),
	is_horizontal(s.is_horizontal), caption_font(s.caption_font), font(s.font),
	a(s.a), b(s.b), a_o(s.a_o), b_o(s.b_o), hidden(s.hidden),
	hide_scale_values(s.hide_scale_values)
{
}

GdaAxis::GdaAxis(const wxString& caption_s, const AxisScale& s,
			   const wxRealPoint& a_s, const wxRealPoint& b_s,
			   int x_nudge, int y_nudge)
	: caption(caption_s), scale(s), is_horizontal(a_s.y == b_s.y),
	a(a_s), b(b_s), a_o(a_s), b_o(b_s),
	font(*GdaConst::small_font), caption_font(*GdaConst::medium_font),
	hidden(false), hide_scale_values(false)
{
	setNudge(x_nudge, y_nudge);
}


void GdaAxis::applyScaleTrans(const GdaScaleTrans& A)
{
	A.transform(a_o, &a);
	A.transform(b_o, &b);
}

void GdaAxis::paintSelf(wxDC& dc)
{
	if (hidden) return;
	wxPoint aa = a;
	aa.x += getXNudge();
	aa.y += getYNudge();
	wxPoint bb = b;
	bb.x += getXNudge();
	bb.y += getYNudge();
	dc.SetPen(getPen());
	dc.SetBrush(getBrush());
	dc.SetFont(font);
	double my_tic_inc = (GenUtils::distance(aa,bb) / scale.scale_range) *
							scale.tic_inc;
	const int tic_length = 4;
	const double label_offset = 8;
	const double caption_offset = 26;
	if (isHorizontal()) {
		dc.DrawLine(aa.x, aa.y, bb.x, bb.y);
		for (int i=0, iend=scale.tics.size(); i<iend; i++) {
			int my_x = ((double) aa.x) + (((double) i) * my_tic_inc);
			int tl_delta = scale.tics_str_show[i] ? 1 : -1;
			dc.DrawLine(my_x, aa.y, my_x, aa.y + (tic_length+tl_delta));
			wxString text(scale.tics_str[i].c_str(), wxConvUTF8);
			wxRealPoint ref_pt(my_x, aa.y+label_offset);
			double cc_rot_degs = 0;
			wxPoint text_pos = GdaShapeText::calcRefPoint(dc, text, font, ref_pt,
													cc_rot_degs,
													GdaShapeText::h_center,
													GdaShapeText::top);
			if (scale.tics_str_show[i] && !hide_scale_values) {
				dc.DrawRotatedText(text, text_pos.x, text_pos.y, cc_rot_degs);
			}
		}
		wxRealPoint ref_pt(aa.x + (bb.x - aa.x)/2, aa.y + caption_offset);
		double	cc_rot_degs = 0;
		wxPoint text_pos = GdaShapeText::calcRefPoint(dc, caption, caption_font,
												ref_pt, cc_rot_degs,
												GdaShapeText::h_center,
												GdaShapeText::top);
		if (!hide_scale_values) {
			dc.DrawRotatedText(caption, text_pos.x, text_pos.y, cc_rot_degs);
		}
	} else {
		dc.DrawLine(aa.x, aa.y, bb.x, bb.y);
		for (int i=0, iend=scale.tics.size(); i<iend; i++) {
			int my_y = ((double) aa.y) - (((double) i) * my_tic_inc);
			int tl_delta = scale.tics_str_show[i] ? 1 : -1;
			dc.DrawLine(aa.x - (tic_length+tl_delta), my_y, aa.x, my_y);
			wxString text(scale.tics_str[i].c_str(), wxConvUTF8);
			wxRealPoint ref_pt(aa.x-label_offset, my_y);
			double cc_rot_degs = 90;
			wxPoint text_pos = GdaShapeText::calcRefPoint(dc, text, font, ref_pt,
													cc_rot_degs,
													GdaShapeText::h_center,
													GdaShapeText::bottom);
			if (scale.tics_str_show[i] && !hide_scale_values) {
				dc.DrawRotatedText(text, text_pos.x, text_pos.y, cc_rot_degs);
			}
		}
		wxRealPoint ref_pt(aa.x - caption_offset, aa.y + (bb.y - aa.y)/2);
		double	cc_rot_degs = 90;
		wxPoint text_pos = GdaShapeText::calcRefPoint(dc, caption, caption_font,
												ref_pt, cc_rot_degs,
												GdaShapeText::h_center,
												GdaShapeText::bottom);
		if (!hide_scale_values) {
			dc.DrawRotatedText(caption, text_pos.x, text_pos.y, cc_rot_degs);
		}
	}
}
