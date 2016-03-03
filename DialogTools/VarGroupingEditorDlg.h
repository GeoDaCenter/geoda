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

#ifndef __GEODA_CENTER_TIME_VARIANT_IMPORT_DLG_H__
#define __GEODA_CENTER_TIME_VARIANT_IMPORT_DLG_H__

#include <list>
#include <map>
#include <set>
#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"
#include "../GdaConst.h"

class FramesManager;
class TableState;
class TableInterface;
class Project;
class WeightsManInterface;

class VarGroupingEditorDlg: public wxDialog, public FramesManagerObserver,
public TableStateObserver
{    
public:
    VarGroupingEditorDlg(Project* project, wxWindow* parent,
				   const wxString& title = "Time Editor", 
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
	virtual ~VarGroupingEditorDlg();

    void CreateControls();
	void InitAll();
	void InitUngroupedList(std::set<wxString>& excl_nms);
	void InitGroupedList();
	
	void OnClose(wxCloseEvent& event);
    void OnSaveSpaceTimeTableClick( wxCommandEvent& event );
    void OnCreateGrpClick( wxCommandEvent& event );
	void OnUngroupClick( wxCommandEvent& event );
	
	void OnMoveUpClick( wxCommandEvent& event );
	void OnMoveDownClick( wxCommandEvent& event );
	void OnSortClick( wxCommandEvent& event );
	void OnPlaceholderClick( wxCommandEvent& event );
	void OnAddToListClick( wxCommandEvent& event );
	void OnRemoveFrListClick( wxCommandEvent& event );
	void OnUngroupedListSelection( wxListEvent& event );
	void OnUngroupedListItemActivate( wxListEvent& event );
	
    void OnIncludeListSelection( wxListEvent& event );
	void OnIncludeListItemActivate( wxListEvent& event);
    void OnIncludeListEdit( wxListEvent& event );
    void OnIncludeListEditEnd( wxListEvent& event );
    void OnIncludeListColClick( wxListEvent& event );
    
    void OnIncludeListDblClicked( wxMouseEvent& event);
    void OnIncludeListRightUp( wxMouseEvent& event);
    void OnIncludePopupClick(wxCommandEvent &evt);
    
    void includeListAddNewTime();
    void includeListDeleteTime();
    void sortColumn(int col, bool asc=false);
    wxString GetNewAppendTimeLabel();

    
	void OnGroupedListSelection( wxCommandEvent& event );
	void OnNewGroupNameChange( wxCommandEvent& event );

	void OnSaveSTHelp( wxCommandEvent& event );
	void OnUngroupedVarsHelp( wxCommandEvent& event );
	void OnNewGroupHelp( wxCommandEvent& event );
	void OnCurGroupedHelp( wxCommandEvent& event );
    void OnLoadFromGda( wxCommandEvent& event );
	
	void UpdateGroupButton();
	void UpdateAddToListButton();
	void UpdateButtons();
	void ResetAllButtons();
	void UpdateCommonType();
	void UpdateTimeStepsTxt();
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	std::list<int> GetListSel(wxListCtrl* lc);
	std::list<wxString> GetListSelStrs(wxListCtrl* lc, int col=0);
	std::list<wxString> GetListAllStrs(wxListCtrl* lc, int col=0);
	void UnselectAll(wxListCtrl* lc);
	void SelectItem(wxListCtrl* lc, int i);
	void UnselectItem(wxListCtrl* lc, int i);
	bool IsItemSel(wxListCtrl* lc, int i);
    

	
protected:
	HighlightState* highlight_state;
    WeightsManInterface* wmi;
    
	int GetIncListNameCnt();
	int GetIncListNonPlaceholderCnt();
    wxString GenerateTimeLabel();
	
	bool all_init;
	wxButton* group_button;
	wxButton* ungroup_button;
	
	wxTextCtrl* new_group_name_txt_ctrl;
	wxStaticText* new_field_type_stat_txt;
	wxStaticText* include_list_stat_txt;

    wxButton* save_spacetime_button;
	wxButton* move_up_button;
	wxButton* move_down_button;
	wxButton* sort_button;
	wxButton* placeholder_button;
		
	wxButton* add_to_list_button;
	wxButton* remove_fr_list_button;
	
	wxListCtrl* ungrouped_list;
	wxListCtrl* include_list;
    bool sort_asc;
    
	wxListBox* grouped_list;
	
	FramesManager* frames_manager;
	TableState* table_state;
	TableInterface* table_int;
    Project* project;
    
	bool common_empty;
	wxString common_type;
	
	DECLARE_EVENT_TABLE()
};

#endif
