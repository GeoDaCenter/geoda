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

#ifndef __GEODA_CENTER_HL_STATE_INT_H__
#define __GEODA_CENTER_HL_STATE_INT_H__

#include <vector>
#include <list>
#include <wx/string.h>

class HighlightStateObserver;

/**
 An instance of this class models the linked highlight state of all
 observations in the current project.  This is the means by which
 all of the views in GeoDa are linked.  All children of TemplateCanvas can
 be Observers of the HightlightState Observable class.  To be notified of
 state changes, an Observable registers itself by calling the
 registerObserver(Observer*) method.  The notifyObservers() method notifies
 all registered Observers of state changes.
 */

class HLStateInt {
public:
	enum EventType {
		empty, // an empty event, observers should not be notified
		delta, // check both newly_highlighted and newly_unhighlighted
		unhighlight_all, // unhighlight everything
		invert, // flip highlight state for all observations
        transparency // transparency change
	};
	
	/** Signal that HighlightState should be closed, but wait until
	 all observers have deregistered themselves. */
	virtual void closeAndDeleteWhenEmpty() = 0;
	
	virtual void SetSize(int n) = 0;
	virtual std::vector<bool>& GetHighlight() = 0;
	virtual std::vector<int>& GetNewlyHighlighted() = 0;
	virtual std::vector<int>& GetNewlyUnhighlighted() = 0;
	virtual int GetHighlightSize() = 0;
	virtual int GetTotalNewlyHighlighted() = 0;
	virtual int GetTotalNewlyUnhighlighted() = 0;
	virtual void SetTotalNewlyHighlighted(int n) = 0;
	virtual void SetTotalNewlyUnhighlighted(int n) = 0;
	virtual bool IsHighlighted(int obs) = 0;
	virtual EventType GetEventType() = 0;
	virtual wxString GetEventTypeStr() = 0;
	virtual void SetEventType( EventType e ) = 0;
	virtual int GetTotalHighlighted() = 0;
	
	virtual void registerObserver(HighlightStateObserver* o) = 0;
	virtual void removeObserver(HighlightStateObserver* o) = 0;
	virtual void notifyObservers() = 0;
	/** Notify all observers excluding exclude. */
	virtual void notifyObservers(HighlightStateObserver* exclude) = 0;
};

#endif
