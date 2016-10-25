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
#include "../HighlightState.h"
#include "../logger.h"
#include "CovSpHLStateProxy.h"

CovSpHLStateProxy::CovSpHLStateProxy(HighlightState* hl_state,
																		 const pairs_bimap_type& pairsBiMap)
: pbm(pairsBiMap)
{
	delete_self_when_empty = false;
	highlight_state = hl_state;
	
	Init();
	
	highlight_state->registerObserver(this);
	LOG_MSG("In CovSpHLStateProxy::CovSpHLStateProxy()");
	
}

CovSpHLStateProxy::~CovSpHLStateProxy()
{	
	LOG_MSG("In CovSpHLStateProxy::~CovSpHLStateProxy()");
	highlight_state->removeObserver(this);
}

void CovSpHLStateProxy::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering CovSpHLStateProxy::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
	LOG_MSG("Exiting CovSpHLStateProxy::closeAndDeleteWhenEmpty");
}

void CovSpHLStateProxy::SetSize(int n) {
	// do nothing
}


wxString CovSpHLStateProxy::GetEventTypeStr()
{
	if (event_type == delta) return "delta";
	if (event_type == unhighlight_all) return "unhighlight_all";
	if (event_type == invert) return "invert";
	return "empty";
}

/* Observable interface definitions */

void CovSpHLStateProxy::registerObserver(HighlightStateObserver* o)
{
	observers.push_front(o);
}

void CovSpHLStateProxy::removeObserver(HighlightStateObserver* o)
{
	LOG_MSG("Entering CovSpHLStateProxy::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0 && delete_self_when_empty) {
		delete this;
	}
	LOG_MSG("Exiting CovSpHLStateProxy::removeObserver");
}

void CovSpHLStateProxy::notifyObservers()
{
	ApplyChanges();
	if (event_type == empty) return;
	//LOG_MSG("In CovSpHLStateProxy::notifyObservers");
	//LOG(observers.size());
	// See section 18.4.4.2 of Stroustrup
	using namespace std;
	for_each(observers.begin(), observers.end(),
					 bind2nd(mem_fun(&HighlightStateObserver::update), this));
	notifyHighlightState();
}

void CovSpHLStateProxy::notifyObservers(HighlightStateObserver* exclude)
{
	ApplyChanges();
	if (event_type == empty) return;
	for (std::list<HighlightStateObserver*>::iterator i=observers.begin();
			 i != observers.end(); ++i)
	{
		if ((*i) == exclude) {
			//LOG_MSG("CovSpHLStateProxy::notifyObservers: skipping exclude");
		} else {
			(*i)->update(this);
		}
	}
	notifyHighlightState();
}

/** Implement HighlightStateObserver interface */
void CovSpHLStateProxy::update(HLStateInt* o)
{
	total_newly_highlighted = 0;
	total_newly_unhighlighted = 0;
	// translate update from HighlightState to CovSpHLStateProxy
	event_type = HLStateInt::empty;
	if (o->GetEventType() == HLStateInt::unhighlight_all) {
		event_type = HLStateInt::unhighlight_all;
		total_highlighted = 0;
		for (size_t i=0, sz=highlight.size(); i<sz; ++i) highlight[i] = false;
	} else if (o->GetEventType() == HLStateInt::delta ||
						 o->GetEventType() == HLStateInt::invert)
	{
		// For each pair (i,j) in pbm, if either i or j is sel in orig_hs,
		// then pair is now selected.
		const std::vector<bool>& orig_hs = highlight_state->GetHighlight();
		for (pairs_bimap_type::const_iterator iter = pbm.begin(), iend = pbm.end();
				iter != iend; ++iter)
		{
			// iter->left  : data : int
			// iter->right : data : UnOrdIntPair
			bool new_sel = orig_hs[iter->right.i] || orig_hs[iter->right.j];
			if (new_sel && !highlight[iter->left]) {
				highlight[iter->left] = true;
				newly_highlighted[total_newly_highlighted++] = iter->left;
				++total_highlighted;
			} else if (!new_sel && highlight[iter->left]) {
				highlight[iter->left] = false;
				newly_unhighlighted[total_newly_unhighlighted++] = iter->left;
				--total_highlighted;
			}
		}
		if (total_newly_highlighted > 0 || total_newly_unhighlighted > 0) {
			event_type = HLStateInt::delta;
		}
	}
	if (event_type != HLStateInt::empty) {
		using namespace std;
		for_each(observers.begin(), observers.end(),
						 bind2nd(mem_fun(&HighlightStateObserver::update), this));
	}
}

/** Translate notify event to HighlightState */
void CovSpHLStateProxy::notifyHighlightState()
{
	using namespace std;
	vector<bool> hs(highlight_state->GetHighlight()); // make a copy
    bool selection_changed = false;
    
	highlight_state->SetEventType(HLStateInt::empty);
	if (event_type == HLStateInt::unhighlight_all) {
		highlight_state->SetEventType(HLStateInt::unhighlight_all);
        
	} else if (event_type == HLStateInt::delta || event_type == HLStateInt::invert) {
		// Notice that we must be careful not to highlight or unhighlight
		// the same observations multiple times.
		// We will the following logic: For observation i, if any of i
		// are selected in the (n-1) pairs, then i is considered selected.
		// otherwise, i is considered unselected.
		vector<bool> any_hl(hs.size(), false);
		for (pairs_bimap_type::const_iterator iter = pbm.begin(), iend = pbm.end();
				 iter != iend; ++iter)
		{
			// iter->left  : data : int
			// iter->right : data : UnOrdIntPair
			if (highlight[iter->left]) {
				any_hl[iter->right.i] = true;
				any_hl[iter->right.j] = true;
			}
		}
		for (size_t i=0, sz=hs.size(); i<sz; ++i) {
			bool sel_i = any_hl[i];
			if (sel_i && !hs[i]) {
				hs[i] = true;
                selection_changed = true;
			} else if (!sel_i && hs[i]) {
				hs[i] = false;
                selection_changed = true;
			}
		}
        
        if ( selection_changed ) {
			highlight_state->SetEventType(HLStateInt::delta);
		}
	}
	// notify HighlightState, but exclude self
	highlight_state->notifyObservers(this);
}

void CovSpHLStateProxy::Init()
{
	size_t n = pbm.size();
	total_highlighted = 0;
	highlight.resize(n);
	newly_highlighted.resize(n);
	newly_unhighlighted.resize(n);
	std::vector<bool>::iterator it;
	
	for ( it=highlight.begin(); it != highlight.end(); it++ ) (*it) = false;
	
	// For each pair (i,j) in pbm, if either i or j is sel in orig_hs,
	// then pair is selected.
	const std::vector<bool>& orig_hs = highlight_state->GetHighlight();
	for (pairs_bimap_type::const_iterator iter = pbm.begin(), iend = pbm.end();
			 iter != iend; ++iter)
	{
		// iter->left  : data : int
		// iter->right : data : UnOrdIntPair
		bool is_sel = orig_hs[iter->right.i] || orig_hs[iter->right.j];
		highlight[iter->left] = is_sel;
		if (is_sel) ++total_highlighted;
	}
}

void CovSpHLStateProxy::ApplyChanges()
{
	switch (event_type) {
		case delta:
		{
			for (int i=0; i<total_newly_highlighted; i++) {
				if (!highlight[newly_highlighted[i]]) {
					highlight[newly_highlighted[i]] = true;
				}
			}
			for (int i=0; i<total_newly_unhighlighted; i++) {
				if (highlight[newly_unhighlighted[i]]) {
					highlight[newly_unhighlighted[i]] = false;
				}
			}
			total_highlighted += total_newly_highlighted;
			total_highlighted -= total_newly_unhighlighted;
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
			int t_nh = 0;
			int t_nuh = 0;
			for (int i=0, iend=highlight.size(); i<iend; i++) {
				if (highlight[i]) {
					newly_unhighlighted[t_nuh++] = i;
					highlight[i] = false;
				} else {
					newly_highlighted[t_nh++] = i;
					highlight[i] = true;
				}
			}
			total_highlighted = highlight.size() - total_highlighted;
			total_newly_highlighted = t_nh;
			total_newly_unhighlighted = t_nuh;
		}
			break;
		default:
			break;
	}
}
