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

#ifndef __GEODA_CENTER_NEW_TABLE_VIEWER_H__
#define __GEODA_CENTER_NEW_TABLE_VIEWER_H__

#include "DataViewer/DataViewer.h"
#include "TemplateFrame.h"

class NewTableViewerFrame : public TemplateFrame
{
public:
	NewTableViewerFrame(wxFrame *parent, Project* project,
						const wxString& title,
						const wxPoint& pos, const wxSize& size,
						const long style);
	virtual ~NewTableViewerFrame();

	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
		
	void OnColSizeEvent( wxGridSizeEvent& ev );
	void OnColMoveEvent( wxGridEvent& ev );
	void OnRightClickEvent( wxGridEvent& ev );
	void OnLabelLeftClickEvent( wxGridEvent& ev );
	void OnLabelLeftDClickEvent( wxGridEvent& ev );
	void OnCellChanged( wxGridEvent& ev );	
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
private:
	wxGrid* grid;
	DbfGridTableBase* grid_base;
	wxString cur_dbf_fname;
	
	DECLARE_EVENT_TABLE()
};


#endif
