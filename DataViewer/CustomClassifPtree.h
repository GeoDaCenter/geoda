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

#ifndef __GEODA_CENTER_CUSTOM_CLASSIF_PTREE_H__
#define __GEODA_CENTER_CUSTOM_CLASSIF_PTREE_H__

#include <list>
#include "../Explore/CatClassification.h"
#include "PtreeInterface.h"

class CatClassifManager;

class CustomClassifPtree : public PtreeInterface {
public:
    CustomClassifPtree();
	CustomClassifPtree(const CustomClassifPtree& o);
    CustomClassifPtree(const boost::property_tree::ptree& pt,
					   const wxString& proj_path);
    virtual ~CustomClassifPtree();
	
	/// PtreeInterface method.  Throws GdaException
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	/// PtreeInterface method.  Throws GdaException
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);

	const std::list<CatClassifDef>& GetCatClassifList() const;
	void SetCatClassifList(CatClassifManager* cc_manager);

	wxString ToStr() const;
	
    CustomClassifPtree* Clone();
    
private:
	std::list<CatClassifDef> cc;
};

#endif
