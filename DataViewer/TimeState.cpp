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

#include "../logger.h"
#include "../GenUtils.h"
#include "TimeStateObserver.h"
#include "TimeState.h"

TimeState::TimeState()
: delete_self_when_empty(false), curr_time_step(0), time_ids(1)
{
	LOG_MSG("In TimeState::TimeState");
}

TimeState::TimeState(const std::vector<wxString>& time_ids)
: delete_self_when_empty(false), curr_time_step(0)
{
	SetTimeIds(time_ids);
}

TimeState::~TimeState()
{
	LOG_MSG("In TimeState::~TimeState");
}

int TimeState::GetCurrTime()
{
	return curr_time_step;
}

wxString TimeState::GetCurrTimeString()
{
	return time_ids[curr_time_step];
}

void TimeState::SetCurrTime(int t)
{
	if (t >= 0 && t < time_ids.size()) curr_time_step = t;
}

int TimeState::GetTimeSteps()
{
	return time_ids.size();
}

void TimeState::SetTimeIds(const std::vector<wxString>& time_ids_s)
{
	time_ids.resize(time_ids_s.size());
	for (int i=0, ii=time_ids_s.size(); i<ii; i++) {
		time_ids[i] = time_ids_s[i];
	}
	if (curr_time_step >= time_ids.size()) curr_time_step = time_ids.size()-1;
}

void TimeState::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering TimeState::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		LOG_MSG("Deleting self now since no registered observers.");
		delete this;
	}
	LOG_MSG("Exiting TimeState::closeAndDeleteWhenEmpty");
}

void TimeState::registerObserver(TimeStateObserver* o)
{
	observers.push_back(o);
}

void TimeState::removeObserver(TimeStateObserver* o)
{
	LOG_MSG("Entering TimeState::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0 && delete_self_when_empty) delete this;
	LOG_MSG("Exiting TimeState::removeObserver");
}

void TimeState::notifyObservers()
{
	for (std::list<TimeStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

void TimeState::notifyObservers(TimeStateObserver* exclude)
{
	for (std::list<TimeStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it)
	{
		if ((*it) == exclude) {
			LOG_MSG("TimeState::notifyObservers: skipping exclude");
		} else {
			(*it)->update(this);
		}
	}
}

