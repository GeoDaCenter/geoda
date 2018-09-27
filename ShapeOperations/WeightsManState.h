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

#ifndef __GEODA_CENTER_WEIGHTS_MAN_STATE_H__
#define __GEODA_CENTER_WEIGHTS_MAN_STATE_H__

#include <list>
#include <utility>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <wx/string.h>

class WeightsManStateObserver; // forward declaration

class WeightsManState {
public:
	enum EventType {
		empty_evt, // an empty event, observers should not be notified
		add_evt, // weights entry removed
		remove_evt, // new weights entry added
		name_change_evt // title change
	};
	WeightsManState();
	virtual ~WeightsManState();
	
	/** Signal that WeightsManState should be closed, but wait until
	 all observers have deregistered themselves. */
	void closeAndDeleteWhenEmpty();
	
	void registerObserver(WeightsManStateObserver* o);
	void removeObserver(WeightsManStateObserver* o);
	void notifyObservers();
    /** Notify all observers excluding exclude. */
    virtual void notifyObservers(WeightsManStateObserver* exclude);
    
	/** For any Observers where WeightsManStateObserver::numMustCloseToRemove
	 is non-zero, closeObservers will call
	 WeightsManStateObserver::closeObserver.
	 If o is not null, this observer will be skipped. */
	void closeObservers(boost::uuids::uuid id,
						WeightsManStateObserver* o = NULL);
	
	EventType GetEventType() { return event_type; }
	wxString GetEventTypeStr();
	boost::uuids::uuid GetWeightsId() { return w_uuid; }
	void SetAddEvtTyp(boost::uuids::uuid weights_id);
	void SetRemoveEvtTyp(boost::uuids::uuid weights_id);
	void SetNameChangeEvtTyp(boost::uuids::uuid weights_id);
	
	int NumBlockingRemoveId(boost::uuids::uuid id) const;
	
private:
	/** The list of registered WeightsManStateObserver objects. */
	std::list<WeightsManStateObserver*> observers;
	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the WeightsManState instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
	
	/** event details */
	EventType event_type;
	boost::uuids::uuid w_uuid; // UUID of modified weights
	
	
};

#endif
