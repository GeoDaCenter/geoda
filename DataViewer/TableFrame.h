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

#ifndef __GEODA_CENTER_TABLE_FRAME_H__
#define __GEODA_CENTER_TABLE_FRAME_H__

#include <wx/grid.h>
#include "../TemplateFrame.h"

class TableBase;

class TableFrame : public TemplateFrame
{
public:
	TableFrame(wxFrame *parent, Project* project,
			   const wxString& title,
			   const wxPoint& pos, const wxSize& size,
			   const long style);
	virtual ~TableFrame();
	
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnActivate(wxActivateEvent& event);
	virtual void MapMenus();
	
	void DisplayPopupMenu( wxGridEvent& ev );
	void SetEncodingCheckmarks(wxMenu* menu, wxFontEncoding enc_type);
	void OnColSizeEvent( wxGridSizeEvent& ev );
	void OnColMoveEvent( wxGridEvent& ev );
	void OnRightClickEvent( wxGridEvent& ev );
	void OnLabelLeftClickEvent( wxGridEvent& ev );
	void OnLabelLeftDClickEvent( wxGridEvent& ev );
	void OnCellChanged( wxGridEvent& ev );
    
    void OnMouseEvent(wxMouseEvent& event);
	
	TableBase* GetTableBase() { return table_base; }
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	/** Override of TableStateObserver interface */
	virtual bool AllowTimelineChanges() { return true; }

    void OnGroupVariables(wxCommandEvent& event);
    void OnUnGroupVariable(wxCommandEvent& event);
	void OnRenameVariable(wxCommandEvent& event);
	wxString PromptRenameColName(TableInterface* ti, int curr_col,
								 const wxString& initial_name);
	
private:
	wxGrid* grid;
	// TableBase contains the instance of table_interface, 
	// e.g. table_base->GetTableInt()
	TableBase* table_base;
	int popup_col;
	
	DECLARE_EVENT_TABLE()
};

#endif
