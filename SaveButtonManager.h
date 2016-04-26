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

#ifndef __GEODA_CENTER_SAVE_MANAGER_H__
#define __GEODA_CENTER_SAVE_MANAGER_H__

#include "DataViewer/TableStateObserver.h"
#include "ShapeOperations/WeightsManStateObserver.h"

class TableState;
class WeightsManState;

/**
 * SaveButtonManager is responsible for keeping keeping track of when there
 * are changes that need to be saved to the Project meta-data file and
 * also to the database.  It determines this by observing TableState
 * events.
 */

class SaveButtonManager : public TableStateObserver,
	public WeightsManStateObserver
{
public:
	SaveButtonManager(TableState* table_state, WeightsManState* w_man_state);
	virtual ~SaveButtonManager();
	
	/** SetAllowEnableSave is used to notify the project that the Save
	 menu items can be enabled.  It does not mean that save items should
	 be enabled, only that they can be when changes have been made to
	 the meta-data or db data. When a new project is opened, the project
	 file name has not been specified, and therefore only Save As should
	 be enabled until the project is saved for the first time.  For the case of
	 data-only files such as dBase, CSV or Excel, if a point layer
	 is added, then only Save As should be enabled since it is not
	 possible to save the point layer in the data-only text file. */
	void SetAllowEnableSave(bool enable);
	bool IsAllowEnableSave();
	
	bool IsSaveNeeded();
	bool IsMetaDataSaveNeeded();
	bool IsDbSaveNeeded();
	void SetMetaDataSaveNeeded(bool save_needed);
	/** This should only be called immedately after creating or opening
	 a new Project instance.  After that, the SaveButtonManager will watch
	 for TableState events to determine when a change has been made. */
	void SetDbSaveNeeded(bool save_needed);
	
	void NotifyAllSaved();
	void NotifyMetaDataSaved();
	void NotifyDbSaved();
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }

	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};
	
private:
	void UpdateSaveMenuItems();
	
	TableState* table_state;
	WeightsManState* w_man_state;
	bool allow_enable_save;
	bool metadata_chgs_since_last_save;
	bool db_chgs_since_last_save;
};

#endif
