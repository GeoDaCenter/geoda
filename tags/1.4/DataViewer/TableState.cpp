/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include "../logger.h"
#include "TableStateObserver.h"
#include "TableState.h"

TableState::TableState()
: delete_self_when_empty(false)
{
	LOG_MSG("In TableState::TableState");
}

TableState::~TableState()
{
	LOG_MSG("In TableState::~TableState");
}

void TableState::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering TableState::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		LOG_MSG("Deleting self now since no registered observers.");
		delete this;
	}
	LOG_MSG("Exiting TableState::closeAndDeleteWhenEmpty");
}

void TableState::registerObserver(TableStateObserver* o)
{
	observers.push_front(o);
}

void TableState::removeObserver(TableStateObserver* o)
{
	LOG_MSG("Entering TableState::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0 && delete_self_when_empty) delete this;
	LOG_MSG("Exiting TableState::removeObserver");
}

void TableState::notifyObservers()
{
	std::list<TableStateObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); it++) {
		(*it)->update(this);
	}
}
