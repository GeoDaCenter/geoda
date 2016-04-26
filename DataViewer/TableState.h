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

#ifndef __GEODA_CENTER_TABLE_STATE_H__
#define __GEODA_CENTER_TABLE_STATE_H__

#include <list>
#include <utility>
#include <vector>
#include <wx/string.h>
#include "../GdaConst.h"

class TableStateObserver; // forward declaration

struct TableDeltaEntry {
	TableDeltaEntry() : pos_at_op(-1), pos_final(-1), decimals(0),
		displayed_decimals(0), length(0), type(GdaConst::unknown_type),
		change_to_db(false) {}
	TableDeltaEntry(const wxString& _group_name, bool _insert, int _pos_at_op)
		: group_name(_group_name), insert(_insert), pos_at_op(_pos_at_op),
		pos_final(-1), decimals(0), displayed_decimals(0), length(0),
		type(GdaConst::unknown_type), change_to_db(false) {}
	
	wxString group_name;
	bool insert; // if false, then remove
	/** Position when operation performed.  For example, if colA was
	 first inserted in pos 0, then colB was inserted in pos 0, the
	 pos value for both should be 0, not 1 and 0.  This should allow
	 wxGrid to be notified of an entire sequence of table column
	 changes.
	 */
	int pos_at_op;
	/** Final position.  This is only valid for inserted columns since
	 removed columns are gone after the end of the sequence of operations. */
	int pos_final;
	GdaConst::FieldType type; // only needed for insert
	int decimals; // only needed for numerical insert
	int displayed_decimals; // only needed for numerical insert
	int length; // only needed for insert
	bool change_to_db; // state of database, not just meta-data changed
	
	wxString ToString() const;
};

typedef std::list<TableDeltaEntry> TableDeltaList_type;

class TableState {
public:
	enum EventType {
		empty, // an empty event, observers should not be notified
		cols_delta, // a series of column insert and deletions
		col_rename, // group or db_field rename
		col_data_change, // change to values in variable
		col_disp_decimals_change, // change to number of displayed decimals
		col_properties_change, // change of type, length or precision
		col_order_change, // visual order of columns changed in Table
		time_ids_add_remove,
		time_ids_rename,
		time_ids_swap,
        refresh
	};
	TableState();
	virtual ~TableState();
	
	/** Signal that TableState should be closed, but wait until
	 all observers have deregistered themselves. */
	void closeAndDeleteWhenEmpty();
	
	/** registerTableBase is a special case of registerObserver
	 that should only be called by the TableBase class.  This is
	 to ensure TableBase is always notified first. */
	void registerTableBase(TableStateObserver* o);
	void registerObserver(TableStateObserver* o);
	void removeObserver(TableStateObserver* o);
	void notifyObservers();
	
	EventType GetEventType() { return event_type; }
	wxString GetOldColName() { return old_col_name; }
	wxString GetNewColName() { return new_col_name; }
	bool IsSimpleGroupRename() { return is_simple_group_rename; }
	wxString GetModifiedColName() { return modified_col_name; }
	int GetModifiedColPos() { return modified_col_pos; }
	const TableDeltaList_type& GetTableDeltaListRef() { return tdl; }
	
	void SetColsDeltaEvtTyp(const TableDeltaList_type& tdl);
	void SetColRenameEvtTyp(const wxString& old_name,
							const wxString& new_name,
							bool simple_group);
	void SetColDataChangeEvtTyp(const wxString& name, int pos);
	void SetColOrderChangeEvtTyp();
	void SetColDispDecimalsEvtTyp(const wxString& name, int pos);
	void SetColPropertiesChangeEvtTyp(const wxString& name, int pos);
	void SetTimeIdsAddRemoveEvtTyp();
	void SetTimeIdsRenameEvtTyp();
	void SetTimeIdsSwapEvtTyp();
    void SetRefreshEvtTyp();
	
	/** Number of TableStateObserver instances for which 
	 TableStateObserver::AllowTimelineChanges() returns false.  If
	 number is zero, then ok to proceed with add/remove, rename
	 and swap time ids operations */
	int GetNumDisallowTimelineChanges();
	wxString GetDisallowTimelineChangesMsg();
	
	/** Number of TableStateObserver instance for which
	 TableStateObserver::AllowGroupModify(grp_nm) returns false.  If
	 number is zero, then ok to proceed with data or property changes */
	int GetNumDisallowGroupModify(const wxString& grp_nm);
	wxString GetDisallowGroupModifyMsg(const wxString& grp_nm);
	
	/** Number of TableStateObserver instance for which
	 TableStateObserver::AllowObservationAddDelete returns false.  If
	 number is zero, then ok to proceed. */
	int GetNumDisallowObservationAddDelete();
	wxString GetDisallowObservationAddDeleteMsg();
	
private:
	/** The list of registered TableStateObserver objects. */
	std::list<TableStateObserver*> observers;
	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the TableState instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
	
	// event details
	EventType event_type;
	wxString modified_col_name;
	int modified_col_pos;
	wxString old_col_name;
	wxString new_col_name;
	bool is_simple_group_rename;
	TableDeltaList_type tdl; /// Set by SetColsDeltaEvtTyp()
};

#endif
