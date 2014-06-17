#include <map>

#include "QuadTree.h"
#include "../Generic/GdaShape.h"

QuadTree::QuadTree(const GdaRealRect& Bounds)
{
	bounds = Bounds;
	for (int i = 0; i < QT_NODE_CAPACITY; i++) {
		points[i] = NULL;
		pids[i] = 0;
	}
	sz = 0;
	ne = 0;
	se = 0;
	nw = 0;
	sw = 0;
}

QuadTree::~QuadTree()
{
    // points will be deleted outside
	for (int i = 0; i < QT_NODE_CAPACITY; i++) {
		//delete points[i];
		points[i] = NULL;
	}
}

bool QuadTree::Insert(GdaShape* p, int id)
{
	//Ignore objects which are outside
	if (!bounds.Contains(p))
		return false;

	//If there is space in this quad tree, add the object here
	if (sz < QT_NODE_CAPACITY)
	{
		points[sz] = p;
		pids[sz++] = id;
		return true;
	}

	//Otherwise, we need to subdivide then add the point to whichever node will accept it
	if (nw == 0)
		Subdivide();

	if (nw->Insert(p,id) || ne->Insert(p,id) || sw->Insert(p,id) || se->Insert(p,id))
		return true;

	//Otherwise, the point cannot be inserted for some unknown reason (which should never happen)
	return false;
}

bool QuadTree::Subdivide()
{
	if (ne != 0 || nw != 0 || sw != 0 || se != 0)
		return false;

	ne = new QuadTree(GdaRealRect(bounds.x+(bounds.width/2.0), 
							 bounds.y, bounds.width/2.0,
							 bounds.height/2.0));
	se = new QuadTree(GdaRealRect(bounds.x+(bounds.width/2.0), 
							 bounds.y+(bounds.height/2.0), 
							 bounds.width/2.0, bounds.height/2.0));
	nw = new QuadTree(GdaRealRect(bounds.x, bounds.y, bounds.width/2.0,
								  bounds.height/2.0));
	sw = new QuadTree(GdaRealRect(bounds.x, bounds.y + (bounds.height/2.0), 
								  bounds.width/2.0, bounds.height/2.0));
	return true;
}

void QuadTree::DelChildren()
{
	if (nw != 0)
	{
		nw->DelChildren();
		delete nw;
		nw = 0;
	}
	if (ne != 0)
	{
		ne->DelChildren();
		delete ne;
		ne = 0;
	}
	if (sw != 0)
	{
		sw->DelChildren();
		delete sw;
		sw = 0;
	}
	if (se != 0)
	{
		se->DelChildren();
		delete se;
		se = 0;
	}
}

void QuadTree::Resize(const GdaRealRect& New)
{
	bounds = New;
	if (ne != 0) ne->Resize(GdaRealRect(bounds.x + (bounds.width/2.0), bounds.y, 
										bounds.width/2.0, bounds.height/2.0));
	if (nw != 0) nw->Resize(GdaRealRect(bounds.x + (bounds.width/2.0), 
										bounds.y + (bounds.height/2.0), 
										bounds.width/2.0, bounds.height/2.0));
	if (se != 0) se->Resize(GdaRealRect(bounds.x, bounds.y, bounds.width/2.0, 
										bounds.height/2.0));
	if (sw != 0) sw->Resize(GdaRealRect(bounds.x,bounds.y + (bounds.height/2.0), 
										bounds.width/2.0, bounds.height/2.0));
}

void AddGroup(std::vector<GdaShape*>& vtr, std::vector<GdaShape*>& toAdd)
{
	for (unsigned int i = 0; i < toAdd.size(); i++)
		vtr.push_back(toAdd[i]);
}

void QuadTree::QueryRange(GdaRealRect& range, std::vector<GdaShape*>& shapesInRange)
{
	// Automatically abort if the range does not collide with this quad
	if (!bounds.Intersects(range))
		return; // empty list

	// Check objects at this quad level
	for (int p = 0; p < sz; p++)
	{
		if (range.Contains(points[p])) {
			shapesInRange.push_back(points[p]);
		}
	}

	// Terminate here, if there are no children
	if (nw == 0)
		return;

	// Otherwise, add the points from the children
	nw->QueryRange(range, shapesInRange);
	ne->QueryRange(range, shapesInRange);
	sw->QueryRange(range, shapesInRange);
	se->QueryRange(range, shapesInRange);

	return;
}

void QuadTree::QueryRange(GdaRealRect& range, std::map<int,bool>& shapesInRange)
{
	// Automatically abort if the range does not collide with this quad
	if (!bounds.Intersects(range))
		return; // empty list
	
	// Check objects at this quad level
	for (int p = 0; p < sz; p++)
	{
		if (range.Contains(points[p])) {
			shapesInRange[pids[p]] = true;
		}
	}
	
	// Terminate here, if there are no children
	if (nw == 0)
		return;
	
	// Otherwise, add the points from the children
	nw->QueryRange(range, shapesInRange);
	ne->QueryRange(range, shapesInRange);
	sw->QueryRange(range, shapesInRange);
	se->QueryRange(range, shapesInRange);
	
	return;
}