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

#ifndef __GEODA_CENTER_CAT_CLASSIF_MANAGER_H__
#define __GEODA_CENTER_CAT_CLASSIF_MANAGER_H__

#include <list>
#include <vector>
#include <map>
#include "../DataViewer/CustomClassifPtree.h"
#include "../DataViewer/TableStateObserver.h"
#include "CatClassification.h"
#include "CatClassifState.h"

class TableState;
class TableInterface;

class CatClassifManager : public TableStateObserver {
public:
	CatClassifManager(TableInterface* table_int,
					  TableState* table_state, CustomClassifPtree* cc_ptree);
	virtual ~CatClassifManager();
	void GetTitles(std::vector<wxString>& titles);
	CatClassifState* FindClassifState(const wxString& title);
	/** Create and return a new CatClassifState object and associate
	 with a default variable db field name.  If the name is blank,
	 then no default preview variable associated. */
	CatClassifState* CreateNewClassifState(const CatClassifDef& cc_data);
	void RemoveClassifState(CatClassifState* ccs);
	bool VerifyAgainstTable();
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
private:
	std::list<CatClassifState*> classif_states;
	TableInterface* table_int;
	TableState* table_state;
};

#endif
