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

#include "logger.h"
#include "FramesManagerObserver.h"
#include "FramesManager.h"

FramesManager::FramesManager()
: delete_self_when_empty(false)
{
	LOG_MSG("In FramesManager::FramesManager");
}

FramesManager::~FramesManager()
{
	LOG_MSG("In FramesManager::~FramesManager");
}

void FramesManager::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering FramesManager::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		LOG_MSG("Deleting self now since no registered observers.");
		delete this;
	}
	LOG_MSG("Exiting FramesManager::closeAndDeleteWhenEmpty");
}

void FramesManager::registerObserver(FramesManagerObserver* o)
{
	observers.push_front(o);
}

void FramesManager::removeObserver(FramesManagerObserver* o)
{
	LOG_MSG("Entering FramesManager::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0) {
		if (delete_self_when_empty) {
			LOG_MSG("No more observers left and project closed, "
					"so deleting self.");
			delete this;
		} else {
			LOG_MSG("No more observers left, but project is still "
					"open, so not deleting self.");
		}
	}
	LOG_MSG("Exiting FramesManager::removeObserver");
}

void FramesManager::notifyObservers()
{
	for (std::list<FramesManagerObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

