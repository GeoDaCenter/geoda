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

#ifndef __GEODA_CENTER_TEST_MAP_VIEW_H__
#define __GEODA_CENTER_TEST_MAP_VIEW_H__

#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"

class TestMapCanvas;
class TestMapFrame;

class TestMapCanvas : public TemplateCanvas {
	DECLARE_CLASS(TestMapCanvas)
public:
	TestMapCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size);
	virtual ~TestMapCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);

	DECLARE_EVENT_TABLE()
};

class TestMapFrame : public TemplateFrame {
	DECLARE_CLASS(TestMapFrame)
public:
	TestMapFrame(wxFrame *parent, Project* project, const wxString& title,
				 const wxPoint& pos, const wxSize& size,
				 const long style);
	virtual ~TestMapFrame() {}

public:
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
		
	DECLARE_EVENT_TABLE()
};

#endif
