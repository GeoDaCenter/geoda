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

#include <wx/wx.h>
#include <wx/string.h>

#include "../logger.h"
#include "TableStateObserver.h"
#include "TableState.h"

wxString TableDeltaEntry::ToString() const
{
	wxString s;
	s << "TableDeltaEntry: nm=" << group_name << ", ";
	s << (insert ? "insert" : "remove") << ", pos_at_op=" << pos_at_op;
	if (insert) s << ", pos_final= " << pos_final;
	return s;
}

TableState::TableState()
: delete_self_when_empty(false), event_type(TableState::empty),
	modified_col_pos(-1)
{
}

TableState::~TableState()
{
}

void TableState::closeAndDeleteWhenEmpty()
{
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		//LOG_MSG("Deleting self now since no registered observers.");
		delete this;
	}
}

void TableState::registerTableBase(TableStateObserver* o)
{
	observers.push_front(o);
}

void TableState::registerObserver(TableStateObserver* o)
{
	observers.push_back(o);
}

void TableState::removeObserver(TableStateObserver* o)
{
	LOG_MSG("Entering TableState::removeObserver");
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) {
		delete this;
	}
	LOG_MSG("Exiting TableState::removeObserver");
}

void TableState::notifyObservers()
{
	for (std::list<TableStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
	// reset event details
	event_type = TableState::empty;
	modified_col_name = wxEmptyString;
	modified_col_pos = -1;
}

void TableState::SetColsDeltaEvtTyp(const TableDeltaList_type& _tdl)
{
	event_type = TableState::cols_delta;
	tdl = _tdl;
}

void TableState::SetColRenameEvtTyp(const wxString& old_name,
									const wxString& new_name,
									bool simple_group)
{
	old_col_name = old_name;
	new_col_name = new_name;
	is_simple_group_rename = simple_group;
	event_type = TableState::col_rename;
}

void TableState::SetColDataChangeEvtTyp(const wxString& name, int pos)
{
	event_type = TableState::col_data_change;
	modified_col_name = name;
	modified_col_pos = pos;
}

void TableState::SetColOrderChangeEvtTyp()
{
	event_type = TableState::col_order_change;
}

void TableState::SetTimeIdsAddRemoveEvtTyp()
{
	event_type = TableState::time_ids_add_remove;
}

void TableState::SetTimeIdsRenameEvtTyp()
{
	event_type = TableState::time_ids_rename;
}

void TableState::SetTimeIdsSwapEvtTyp()
{
	event_type = TableState::time_ids_swap;
}

void TableState::SetRefreshEvtTyp()
{
    event_type = TableState::refresh;
}

void TableState::SetColDispDecimalsEvtTyp(const wxString& name, int pos)
{
	event_type = TableState::col_disp_decimals_change;
	modified_col_name = name;
	modified_col_pos = pos;
}

void TableState::SetColPropertiesChangeEvtTyp(const wxString& name, int pos)
{
	event_type = TableState::col_properties_change;
	modified_col_name = name;
	modified_col_pos = pos;
}

int TableState::GetNumDisallowTimelineChanges()
{
	int n=0;
	for (std::list<TableStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		if ( !(*it)->AllowTimelineChanges() ) ++n;
	}
	return n;
}

wxString TableState::GetDisallowTimelineChangesMsg()
{
	wxString s;
	int n = GetNumDisallowTimelineChanges();
	s = wxString::Format(_("Before proceed with operation (add/remove, move, or rename), please close %d views that depend on it."), n);
	
	return s;
}

int TableState::GetNumDisallowGroupModify(const wxString& grp_nm)
{
	int n=0;
	for (std::list<TableStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		if ( !(*it)->AllowGroupModify(grp_nm) ) ++n;
	}
	wxString msg;
	msg << n << " of " << observers.size() << " TableStateObservers ";
	msg << "disallow changes to group: " << grp_nm;
	return n;
}

wxString TableState::GetDisallowGroupModifyMsg(const wxString& grp_nm)
{
	wxString s;
	int n = GetNumDisallowGroupModify(grp_nm);
	s = wxString::Format(_("Before you can modify the variable %s, please close the %d view(s) that depend on it."), grp_nm, n);
	return s;
}

int TableState::GetNumDisallowObservationAddDelete()
{
	int n=0;
	for (std::list<TableStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		// if ( !(*it)->AllowObservationAddDelete() ) ++n;
	}
	wxString msg;
	msg << n << " of " << observers.size() << " TableStateObservers ";
	msg << "disallow observation add/delete";
	return n;
}

wxString TableState::GetDisallowObservationAddDeleteMsg()
{
	wxString s;
	int n = GetNumDisallowObservationAddDelete();
	s = wxString::Format(_("Before add/delete observations, please close the %d view(s) that depend on it."), n);

	return s;
}
