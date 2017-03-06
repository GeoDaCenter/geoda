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

#ifndef __GEODA_CENTER_DATA_CHANGE_TYPE_H__
#define __GEODA_CENTER_DATA_CHANGE_TYPE_H__

#include <vector>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/webview.h>
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "TableStateObserver.h"
#include "../FramesManagerObserver.h"
#include "../GenUtils.h"
#include "../GdaConst.h"

class GdaColListCtrl;
class TableInterface;
class Project;

class DataChangeTypeFrame : public TemplateFrame
{
public:
    DataChangeTypeFrame(wxFrame *parent, Project* project,
				const wxString& title = _("Change Variable Type"),
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = GdaConst::data_change_type_frame_default_size,
				const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~DataChangeTypeFrame();
	
	void OnActivate(wxActivateEvent& ev);
	
	void OnFromVarsSel(wxListEvent& ev);
	void OnFromDataSel(wxListEvent& ev);
	void OnAddVarBtn(wxCommandEvent& ev);
	void OnCopyBtn(wxCommandEvent& ev);
	void OnToVarsSel(wxListEvent& ev);
	void OnToDataSel(wxListEvent& ev);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
		
	static const long DATA_ID_COL = 0; // id column
	static const long DATA_VAL_COL = 1; // val column
	
private:
	void UpdateButtons();
	
	int GetFromVarSel(wxString& name, wxString& time);
	void RefreshFromVars();
	
	int GetToVarSel(wxString& name, wxString& time);
	void RefreshToVars();
	
	int GetVarSel(wxListCtrl* vars_list, wxString& name, wxString& time);
	void RefreshVars(wxListCtrl* vars_list, GdaColListCtrl* data_list);
	void RefreshData(GdaColListCtrl* data_list, const wxString& name,
					 const wxString& time);
	
	wxPanel* panel;

	wxListCtrl* from_vars; // ID_FROM_VARS
	GdaColListCtrl* from_data;	// ID_FROM_DATA
	
	wxButton* add_var_btn; // ID_ADD_VAR_BTN
	wxButton* copy_btn; // ID_COPY_BTN
	
	wxListCtrl* to_vars; // ID_TO_VARS
	GdaColListCtrl* to_data; // ID_TO_DATA

	static const long NAME_COL = 0; // variable name column
	static const long TYPE_COL = 1; // type column
	static const long TIME_COL = 2; // time column

	TableInterface* table_int;
	TableState* table_state;
	
	bool ignore_callbacks;
	
	DECLARE_EVENT_TABLE()
};

class GdaColListCtrl : public wxListCtrl
{
public:
	GdaColListCtrl(TableInterface* table_int,
				   wxWindow* parent, wxWindowID id,
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize);
	
	virtual wxString OnGetItemText(long item, long column) const;
	void SetColNumAndTime(int col, int time);
	
private:
	TableInterface* table_int;
	
	int col;
	int tm;
	
	// once everything is intialized, call SetItemCount(long count)
	// to indicate number of items in list.
};

#endif
