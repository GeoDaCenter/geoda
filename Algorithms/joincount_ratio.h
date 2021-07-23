#ifndef __GEODA_CENTER_JOINCOUNT_RATIO_H__
#define __GEODA_CENTER_JOINCOUNT_RATIO_H__

#include <wx/wx.h>
#include <vector>
#include "../ShapeOperations/GeodaWeight.h"

struct JoinCountRatio {
	std::string cluster;
	int n;
	int totalNeighbors;
	int totalJoinCount;
	double ratio;
    JoinCountRatio(): n(0), totalNeighbors(0), totalJoinCount(0),ratio(0) {}
};

std::vector<JoinCountRatio> joincount_ratio(const std::vector<wxString>& clusters, GeoDaWeight* w);

JoinCountRatio all_joincount_ratio(const std::vector<JoinCountRatio>& items);

#endif
