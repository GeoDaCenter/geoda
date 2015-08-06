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

#ifndef __GEODA_CENTER_TIME_EDITOR_DLG_H__
#define __GEODA_CENTER_TIME_EDITOR_DLG_H__

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"

class FramesManager;
class TableState;
class TableInterface;

class TimeEditorDlg : public wxDialog, public FramesManagerObserver,
public TableStateObserver
{
public:
	TimeEditorDlg(wxWindow* parent, FramesManager* frames_manager,
				  TableState* table_state, TableInterface* table_int);
	virtual ~TimeEditorDlg();
	void InitFromTable();
	void OnClose(wxCloseEvent& ev);
	void OnNewButton(wxCommandEvent& ev);
	void OnBackButton(wxCommandEvent& ev);
	void OnForwardButton(wxCommandEvent& ev);
	void OnDeleteButton(wxCommandEvent& ev);
	void OnSize(wxSizeEvent& ev);
	void OnEndEditItem(wxListEvent& ev);
	void OnItemActivate(wxListEvent& ev);
	void OnItemSelection(wxListEvent& ev);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
private:
	wxString GetNewPrependTimeLabel();
	wxString GetNewAppendTimeLabel();
	int GetListSel();
	void SelectItem(int i);
	void UpdateButtons();
	wxListCtrl* lc;
	
	FramesManager* frames_manager;
	TableState* table_state;
	TableInterface* table_int;
	bool all_init;
	bool ignore_time_id_update;
	
	DECLARE_EVENT_TABLE()
};

#endif
