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

#include <algorithm>
#include <functional>
#include "HighlightStateObserver.h"
#include "logger.h"
#include "HighlightState.h"

HighlightState::HighlightState()
{
	delete_self_when_empty = false;
	LOG_MSG("In HighlightState::HighlightState()");
}

HighlightState::~HighlightState()
{	
	LOG_MSG("In HighlightState::~HighlightState()");
}

void HighlightState::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering HighlightState::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
	LOG_MSG("Exiting HighlightState::closeAndDeleteWhenEmpty");
}

void HighlightState::SetSize(int n) {
	total_highlighted = 0;
	highlight.resize(n);
	newly_highlighted.resize(n);
	newly_unhighlighted.resize(n);
	std::vector<bool>::iterator it;
	for ( it=highlight.begin(); it != highlight.end(); it++ ) (*it) = false;
}


wxString HighlightState::GetEventTypeStr()
{
	if (event_type == delta) return "delta";
	if (event_type == unhighlight_all) return "unhighlight_all";
	if (event_type == invert) return "invert";
	return "empty";
}

/* Observable interface definitions */

void HighlightState::registerObserver(HighlightStateObserver* o)
{
	observers.push_front(o);
}

void HighlightState::removeObserver(HighlightStateObserver* o)
{
	LOG_MSG("Entering HighlightState::removeObserver");
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) {
		delete this;
	}
	LOG_MSG("Exiting HighlightState::removeObserver");
}

void HighlightState::notifyObservers()
{
	ApplyChanges();
	if (event_type == empty) return;
    if (observers.empty()) return;
	// See section 18.4.4.2 of Stroustrup
	//std::for_each(observers.begin(), observers.end(),
	//		 std::bind2nd(std::mem_fun(&HighlightStateObserver::update),this));
   
    std::list<HighlightStateObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        HighlightStateObserver* obj = *it;
        obj->update(this);
    }
    
}

void HighlightState::notifyObservers(HighlightStateObserver* exclude)
{
	ApplyChanges();
	if (event_type == empty) return;
	for (std::list<HighlightStateObserver*>::iterator i=observers.begin();
		 i != observers.end(); ++i)
	{
		if ((*i) == exclude) {
            
		} else {
			(*i)->update(this);
		}
	}
}

void HighlightState::ApplyChanges()
{
	switch (event_type) {
		case delta:
		{
            total_highlighted = 0;
            for (int i=0; i<highlight.size(); i++) {
                if (highlight[i] == true) {
                    total_highlighted += 1;
                }
            }
            
		}
			break;
		case unhighlight_all:
		{
			if (total_highlighted == 0) {
				//MMM: figure out why this short-cut isn't always working
				//event_type = empty;
			}
			for (int i=0, iend=highlight.size(); i<iend; i++) {
				highlight[i] = false;
			}
			total_highlighted = 0;
		}
			break;
		case invert:
		{
            
            total_highlighted = 0;
            for (int i=0; i<highlight.size(); i++) {
                if (highlight[i] == false) {
                    total_highlighted += 1;
                }
                highlight[i] = !highlight[i];
            }
            
		}
			break;
		default:
			break;
	}
}
