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

#ifndef __GEODA_CENTER_DATA_VIEWER_EDIT_FIELD_PROPERTIES_DLG_H__
#define __GEODA_CENTER_DATA_VIEWER_EDIT_FIELD_PROPERTIES_DLG_H__

#include <map>
#include <vector>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include "../FramesManagerObserver.h"
#include "TableStateObserver.h"

class FramesManager;
class TableInterface;
class TableState;
class Project;

class DataViewerEditFieldPropertiesDlg: public wxDialog,
	public FramesManagerObserver, public TableStateObserver
{
public:
    DataViewerEditFieldPropertiesDlg(Project* project,
									 const wxPoint &pos=wxDefaultPosition,
									 const wxSize &size=wxDefaultSize );
	virtual ~DataViewerEditFieldPropertiesDlg();
	
    void CreateControls();
	void InitTable();
	void OnCloseButton( wxCommandEvent& ev );
	void OnClose( wxCloseEvent& ev );
	void OnCellChanging( wxGridEvent& ev );
	void OnCellEditorCreated( wxGridEditorCreatedEvent& ev );
    void OnCellEditorShown( wxGridEvent& ev );
    void OnCellEditorHidden( wxGridEvent& ev );
	void UpdateMinMax(int row);
    
    void OnGridComboBox(wxCommandEvent& ev );
    
    void OnFieldSelected( wxCommandEvent& ev );
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	void UpdateTmStrMap();
	
private:
    int combo_selection;
    
	wxGrid* field_grid;
	Project* project;
	TableInterface* table_int;
	TableState* table_state;
		
	std::map<wxString, int> fn_freq;
	bool cell_editor_open;
	
	std::map<wxString, int> tm_str_map;
	
	int COL_N; // field name
	int COL_T; // type
	int COL_L; // length
	int COL_D; // decimals
	int COL_DD; // displayed decimals
	int COL_PG; // parent group name
	int COL_TM; // time period
	int COL_MIN; // min value possible
	int COL_MAX; // max value possible
	int NUM_COLS;
	
	DECLARE_EVENT_TABLE()
};

#endif
