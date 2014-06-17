/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

class HighlightStateObserver;

/**
 An instance of this class models the linked highlight state of all
 observations in the currentl project.  This is the means by which
 all of the views in GeoDa are linked.  All children of TemplateCanvas can
 be Observers of the HightlightState Observable class.  To be notified of
 state changes, an Observable registers itself by calling the
 registerObserver(Observer*) method.  The notifyObservers() method notifies
 all registered Observers of state changes.
*/

class HighlightState {
public:
	enum EventType {
		empty, // an empty event, observers should not be notified
		delta, // check both newly_highlighted and newly_unhighlighted
		unhighlight_all, // unhighlight everything
		invert // flip highlight state for all observations
	};

	HighlightState();
	virtual ~HighlightState();
	void SetSize(int n);
	std::vector<bool>& GetHighlight() { return highlight; }
	std::vector<int>& GetNewlyHighlighted() { return newly_highlighted; }
	/** To add a single obs to the newly_highlighted list, set pos=0, and
	 val to the obs number to highlight. */
	void SetNewlyHighlighted(int pos, int val) { newly_highlighted[pos] = val; }
	std::vector<int>& GetNewlyUnhighlighted() { return newly_unhighlighted; }
	/** To add a single obs to the newly_unhighlighted list, set pos=0, and
	 val to the obs number to unhighlight. */
	void SetNewlyUnhighlighted(int pos, int val) {
		newly_unhighlighted[pos] = val; }
	int GetHighlightSize() { return highlight.size(); }
	int GetTotalNewlyHighlighted() { return total_newly_highlighted; }
	int GetTotalNewlyUnhighlighted() { return total_newly_unghighlighted; }
	void SetTotalNewlyHighlighted(int n) { total_newly_highlighted = n; }
	void SetTotalNewlyUnhighlighted(int n) { total_newly_unghighlighted = n; }
	bool IsHighlighted(int obs) { return highlight[obs]; }
	EventType GetEventType() { return event_type; }
	void SetEventType( EventType e ) { event_type = e; }
	int GetTotalHighlighted() { return total_highlighted; }
	
	void registerObserver(HighlightStateObserver* o);
	void removeObserver(HighlightStateObserver* o);
	void notifyObservers();
	
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
	int total_newly_unghighlighted;
	EventType event_type;
	void ApplyChanges(); // called by notifyObservers to update highlight vec	
};

#endif
