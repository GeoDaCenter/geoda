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

#include <boost/uuid/nil_generator.hpp>
#include "WeightsManStateObserver.h"
#include "WeightsManState.h"

WeightsManState::WeightsManState()
: delete_self_when_empty(false), w_uuid(boost::uuids::nil_uuid()),
event_type(empty_evt)
{
}

WeightsManState::~WeightsManState()
{
}

void WeightsManState::closeAndDeleteWhenEmpty()
{
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
}

void WeightsManState::registerObserver(WeightsManStateObserver* o)
{
	observers.push_back(o);
}

void WeightsManState::removeObserver(WeightsManStateObserver* o)
{
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) delete this;
}

void WeightsManState::notifyObservers()
{
	for (std::list<WeightsManStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
	event_type = empty_evt;
}

void WeightsManState::closeObservers(boost::uuids::uuid id,
									 WeightsManStateObserver* o)
{
	// make a temporary list since list of observers might be modified
	// by calls to closeObserver
	std::list<WeightsManStateObserver*> to_close;
	for (std::list<WeightsManStateObserver*>::iterator i=observers.begin();
		 i != observers.end(); ++i)
	{
		if (o != (*i) && ((*i)->numMustCloseToRemove(id) > 0)) {
			to_close.push_back(*i);
		}
	}
	// call closeObserver for all observers where numMustCloseToRemove > 0
	for (std::list<WeightsManStateObserver*>::iterator i=to_close.begin();
		 i != to_close.end(); ++i)
	{
		(*i)->closeObserver(id);
	}
}

wxString WeightsManState::GetEventTypeStr()
{
	if (event_type == add_evt) return "add_evt";
	if (event_type == remove_evt) return "remove_evt";
	if (event_type == name_change_evt) return "name_change_evt";
	return "empty_evt";
}

void WeightsManState::SetAddEvtTyp(boost::uuids::uuid weights_id)
{
	event_type = add_evt;
	w_uuid = weights_id;
}

void WeightsManState::SetRemoveEvtTyp(boost::uuids::uuid weights_id)
{
	event_type = remove_evt;
	w_uuid = weights_id;	
}

void WeightsManState::SetNameChangeEvtTyp(boost::uuids::uuid weights_id)
{
	event_type = name_change_evt;
	w_uuid = weights_id;
}

int WeightsManState::NumBlockingRemoveId(boost::uuids::uuid id) const
{
	int n=0;
	for (std::list<WeightsManStateObserver*>::const_iterator it=observers.begin();
		 it != observers.end(); ++it) {
		n += (*it)->numMustCloseToRemove(id);
	}
	return n;
}
