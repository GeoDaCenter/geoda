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

#ifndef __GEODA_CENTER_TIME_STATE_H__
#define __GEODA_CENTER_TIME_STATE_H__

#include <list>
#include <vector>
#include <wx/string.h>

class TimeStateObserver; // forward declaration

class TimeState {
public:
	TimeState();
	TimeState(const std::vector<wxString>& time_ids);
	virtual ~TimeState();
	
	/** Signal that TimeState should be closed, but wait until
	 all observers have deregistered themselves. */
	void closeAndDeleteWhenEmpty();
	
	void registerObserver(TimeStateObserver* o);
	void removeObserver(TimeStateObserver* o);
	void notifyObservers();
	void notifyObservers(TimeStateObserver* exclude);
	
	int GetCurrTime();
	wxString GetCurrTimeString();
	void SetCurrTime(int t);
	int GetTimeSteps();
    wxString GetTimeString(int idx) { return time_ids[idx];}
	
	void SetTimeIds(const std::vector<wxString>& time_ids);
	
private:
	/** The list of registered TimeStateObserver objects. */
	std::list<TimeStateObserver*> observers;
	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the TimeState instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
	
	int curr_time_step;
	std::vector<wxString> time_ids;
};

#endif
