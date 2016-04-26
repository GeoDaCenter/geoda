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

#ifndef __GEODA_CENTER_COV_SP_HL_STATE_PROXY_H__
#define __GEODA_CENTER_COV_SP_HL_STATE_PROXY_H__

#include <vector>
#include <list>
#include <wx/string.h>
#include "DistancesCalc.h"
#include "../HLStateInt.h"
#include "../HighlightStateObserver.h"

class HighlightState;

/**
 A Proxy to the global shared HighlightState instance, highlight_state.
 - For a Project with N observations, highlight_state has a vector of
   N ids (booleans) 0, 1, ..., N-1.
 - Assuming we're interested in distances between all pairs of observations,
   CovSpHLStateProxy has a vector of N(N-1)/2 ids (booleans).  Each of these
   ids corresponds to 1 id in highlight_state, and each id in highlight_state
   corresponds to N ids in this class.
 - In this way, TemplateCanvas only needs to use the HLStateInt interface
 */

class CovSpHLStateProxy : public HLStateInt, public HighlightStateObserver {
public:
	CovSpHLStateProxy(HighlightState* hl_state,
										const pairs_bimap_type& pairsBiMap);
	virtual ~CovSpHLStateProxy();
	
	/** Signal that CovSpHLStateProxy should be closed, but wait until
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
	
	/** Implement HighlightStateObserver interface */
	virtual void update(HLStateInt* o);
	
	const pairs_bimap_type& GetBiMap() const { return pbm; }
	
private:
	void notifyHighlightState();
	void Init();
	HighlightState* highlight_state;
	const pairs_bimap_type& pbm;
	
	/** The list of registered HighlightStateObserver objects. */
	std::list<HighlightStateObserver*> observers;
	/** This array of booleans corresponds to the highlight/not-highlighted
	 selectable_shps. */
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
