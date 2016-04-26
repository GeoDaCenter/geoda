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

#ifndef __GEODA_CENTER_VAR_ORDER_PTREE_H__
#define __GEODA_CENTER_VAR_ORDER_PTREE_H__

#include <map>
#include <vector>
#include "PtreeInterface.h"
#include "TableInterface.h"
#include "VarGroup.h"

class VarOrderPtree : public PtreeInterface {
public:
    VarOrderPtree();
	VarOrderPtree(const VarOrderPtree& vo);
    VarOrderPtree(const boost::property_tree::ptree& pt,
				  const wxString& proj_path);
    virtual ~VarOrderPtree();
	
	/// PtreeInterface method.  Throws GdaException
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	/// PtreeInterface method.  Throws GdaException
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
	
	const std::vector<wxString>& GetTimeIdsRef() const;
	const VarGroup_container& GetVarGroupsRef() const;
	
	bool CorrectVarGroups(const std::map<wxString,
						  GdaConst::FieldType>& ds_var_type_map,
						  const std::vector<wxString>& ds_var_list);
	/**
	 * This function clears all data and then reinitializes from the
	 * TableInterface.  The order of the variables ultimately comes
	 * from the displayed order according to wxGrid.  Therefore, this
	 * should only be used just before saving to disk.
	 */
    void ReInitFromTableInt(TableInterface* table);

	wxString VarOrderToStr() const;
    
    VarOrderPtree* Clone();
	
private:
	bool RemoveFromVarGroups(const wxString& v);
	static bool IsTypeCompatible(const std::vector<wxString>& vars,
								 const std::map<wxString,
								 GdaConst::FieldType>& ds_var_type_map);
	
	// Data
    std::vector<wxString> time_ids;
	VarGroup_container var_grps;
};

#endif