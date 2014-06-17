#pragma once
#include <vector>
#include "../Generic/GdaShape.h"

class GdaRealRect
{
public:
	double x;
    double y;
    double width;
    double height;
    
	GdaRealRect() : x(.0), y(.0), width(.0), height(.0) { }
	
	GdaRealRect(double X, double Y, double W, double H) 
	: x(X), y(Y), width(W), height(H) { 
	}
	
    GdaRealRect& operator=(const GdaRealRect& rect) {
		x = rect.x; y=rect.y; 
		width=rect.width; 
		height=rect.height; 
		return *this;
	}
	
	/**
	 * This function is used for constructing quad-tree using centers of
	 * GdaShapes.
	 */
	bool Contains(GdaShape* p) { 
		return (p->center_o.x >= x && p->center_o.y >= y 
				&& p->center_o.x < x + width && p->center_o.y < y + height); 
	}

	bool Intersects(GdaRealRect& r) { 
		return !(r.x > (x + width) || (r.x + r.width) < x || r.y > (y + height) 
				 || (r.y + r.height) < y); 
	}
};

//A single layer of a quad tree
class QuadTree
{
public:
	// Arbitrary constant to indicate how many elements can be stored in this quad tree node
	static const int QT_NODE_CAPACITY = 8;

	// Axis-aligned bounding box stored as a center with half-dimensions
	// to represent the boundaries of this quad tree
	GdaRealRect bounds;

	//data inside
	GdaShape* points[QT_NODE_CAPACITY];
	int pids[QT_NODE_CAPACITY];
	int sz;

	QuadTree* nw;
	QuadTree* ne;
	QuadTree* sw;
	QuadTree* se;

	//Create a new quadtree
	QuadTree(const GdaRealRect& bounds);
    ~QuadTree();
	
	bool Insert(GdaShape* p, int id);
	bool Subdivide();
	void DelChildren();
	void Resize(const GdaRealRect& NewBounds);

	void QueryRange(GdaRealRect& range, std::vector<GdaShape*>& shapesInRange);
	void QueryRange(GdaRealRect& range, std::map<int,bool>& shapesInRange);
};
