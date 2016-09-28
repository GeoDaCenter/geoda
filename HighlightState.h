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

#ifndef __GEODA_CENTER_HIGHLIGHT_STATE_H__
#define __GEODA_CENTER_HIGHLIGHT_STATE_H__

#include <vector>
#include <list>
#include <wx/string.h>
#include "HLStateInt.h"

/**
 An instance of this class models the linked highlight state of all
 observations in the current project.  This is the means by which
 all of the views in GeoDa are linked.  All children of TemplateCanvas can
 be Observers of the HightlightState Observable class.  To be notified of
 state changes, an Observable registers itself by calling the
 registerObserver(Observer*) method.  The notifyObservers() method notifies
 all registered Observers of state changes.
*/

class HighlightState : public HLStateInt {
public:
	HighlightState();
	virtual ~HighlightState();
	
	/** Signal that HighlightState should be closed, but wait until
	 all observers have deregistered themselves. */
	virtual void closeAndDeleteWhenEmpty();
	
	virtual void SetSize(int n);
	virtual std::vector<bool>& GetHighlight() { return highlight; }
	virtual std::vector<int>& GetNewlyHighlighted() { return newly_highlighted; }
	virtual std::vector<int>& GetNewlyUnhighlighted() { return newly_unhighlighted; }
	virtual int GetHighlightSize() { return highlight.size(); }
	virtual int GetTotalNewlyHighlighted() { return total_newly_highlighted; }
	virtual int GetTotalNewlyUnhighlighted() { return total_newly_unhighlighted; }
	virtual void SetTotalNewlyHighlighted(int n) { total_newly_highlighted = n; }
	virtual void SetTotalNewlyUnhighlighted(int n) { total_newly_unhighlighted = n; }
	virtual bool IsHighlighted(int obs) { return highlight[obs]; }
	virtual EventType GetEventType() { return event_type; }
	virtual wxString GetEventTypeStr();
	virtual void SetEventType( EventType e ) { event_type = e; }
	virtual int GetTotalHighlighted() { return total_highlighted; }
	
	virtual void registerObserver(HighlightStateObserver* o);
	virtual void removeObserver(HighlightStateObserver* o);
	virtual void notifyObservers();
	/** Notify all observers excluding exclude. */
	virtual void notifyObservers(HighlightStateObserver* exclude);
	
private:
	/** The list of registered HighlightStateObserver objects. */
	std::list<HighlightStateObserver*> observers;
	/** This array of booleans corresponds to the highlight/not-highlighted
	 of each underlying SHP file observation. */
	std::vector<bool> highlight;
    
	/** total number of highlight[i] booleans set to true */
	int total_highlighted;
    
	/** When the highlight vector has changed values, this vector records
	 the observations indicies that have changed from false to true. */
	std::vector<int> newly_highlighted;
    
	/** We do not resize the newly_highlighted vector, rather it is used
	 more like a stack.  #total_newly_highlighted records the number of
	 valid entries on the newly_highlighted 'stack'. */
	int total_newly_highlighted;
    
	/** When the highlight vector has changed values, this vector records
	 the observations indicies that have changed from true to false. */
	std::vector<int> newly_unhighlighted;
    
	/** We do not resize the newly_unhighlighted vector, rather it is used
	 more like a stack.  #total_newly_unhighlighted records the number of
	 valid entries on the #newly_unhighlighted 'stack'. */
	int total_newly_unhighlighted;
    
	EventType event_type;
    
	void ApplyChanges(); // called by notifyObservers to update highlight vec
	
	/** When this is set to true and the list of observers is empty, the
	 class instance will automatically delete itself. */
	bool delete_self_when_empty;
};

#endif
