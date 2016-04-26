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

#ifndef __GEODA_CENTER_WEIGHTS_MAN_PTREE_H__
#define __GEODA_CENTER_WEIGHTS_MAN_PTREE_H__

#include <list>
#include "../DataViewer/PtreeInterface.h"
#include "../VarCalc/WeightsMetaInfo.h"

struct WeightsPtreeEntry {
	WeightsPtreeEntry() : is_default(false) {}
	WeightsPtreeEntry(const WeightsMetaInfo& w) : wmi(w), is_default(false) {}
	WeightsMetaInfo wmi;
	wxString title;
	bool is_default;
	
	wxString ToStr() const;
};

class WeightsManPtree : public PtreeInterface {
public:
    WeightsManPtree();
	WeightsManPtree(const WeightsManPtree& o);
    WeightsManPtree(const boost::property_tree::ptree& pt,
					const wxString& proj_path);
    virtual ~WeightsManPtree();
	
	/// PtreeInterface method.  Throws GdaException
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	/// PtreeInterface method.  Throws GdaException
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);

	const std::list<WeightsPtreeEntry>& GetWeightsMetaInfoList() const;
	void SetWeightsMetaInfoList(const std::list<WeightsPtreeEntry>& w_list);

	wxString ToStr() const;
    
    WeightsManPtree* Clone();
	
private:
	std::list<WeightsPtreeEntry> weights_list;
};

#endif
