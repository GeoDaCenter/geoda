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

#ifndef __GEODA_CENTER_TEST_TABLE_VIEW_H__
#define __GEODA_CENTER_TEST_TABLE_VIEW_H__

#include "HighlightState.h"
#include "../TemplateFrame.h"
#include <wx/grid.h>

class TestTableFrame : public TemplateFrame, public HighlightStateObserver {
	DECLARE_CLASS(TestTableFrame)
public:
	TestTableFrame(wxFrame *parent, Project* project, const wxString& title,
				   const wxPoint& pos, const wxSize& size,
				   const long style);
	virtual ~TestTableFrame();
	
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	
	/** This is the implementation of the HighlightStateObserver interface
	 update function.  It is called whenever the observable's state has changed.
	 In this case, the observable is an instance of the HighlightState, which
	 keeps track of the hightlighted/selected state for every SHP file
	 observation. */
	virtual void update(HighlightState* o);
	
private:
	wxGrid* my_grid;
	HighlightState* highlight_state;
	
	DECLARE_EVENT_TABLE()
};

#endif
