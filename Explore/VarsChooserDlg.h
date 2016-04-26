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

#ifndef __GEODA_CENTER_VARS_CHOOSER_DLG_H__
#define __GEODA_CENTER_VARS_CHOOSER_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/webview.h>
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "VarsChooserObservable.h"

class Project;

class VarsChooserFrame : public wxFrame, public VarsChooserObservable
{
public:
	VarsChooserFrame(GdaVarTools::Manager& var_man,
									 Project* project,
									 bool allow_duplicates = false,
									 bool specify_times = false,
									 const wxString& help_html = wxEmptyString,
									 const wxString& help_title = wxEmptyString,
									 const wxString& title = "Variables Add/Remove",
									 const wxPoint& pos = wxDefaultPosition,
									 const wxSize& size = wxSize(-1, 170));
	virtual ~VarsChooserFrame();
	
	void OnVarsListDClick(wxCommandEvent& ev);
	void OnIncludeListDClick(wxCommandEvent& ev);
	void OnIncludeBtn(wxCommandEvent& ev);
	void OnRemoveBtn(wxCommandEvent& ev);
	void OnUpBtn(wxCommandEvent& ev);
	void OnDownBtn(wxCommandEvent& ev);
	void OnHelpBtn(wxCommandEvent& ev);
	void OnCloseBtn(wxCommandEvent& ev);
	
	/** Validates current list against table.
	 New variables are added, order is updated, and missing variables are removed.
	 If any changes to GdaVarTools::Manager are made, an notify event is
	 generated. */
	void UpdateFromTable();
	
	/** Override VarsChooserObservable::closeAndDeleteWhenEmpty */
	virtual void closeAndDeleteWhenEmpty();
	
private:
	void UpdateLists();
	/** Add item selected in vars_list to include_list */
	void IncludeFromVarsListSel(int sel);
	/** Remove item selected in include_list from include_list */
	void RemoveFromIncludeListSel(int sel);
	wxString PrintState();
	
	Project* project;
	wxListBox* vars_list; // ID_VARS_LIST
	wxListBox* include_list; // ID_INCLUDE_LIST
	
	bool allow_duplicates;
	bool specify_times;
	wxString help_html;
	wxString help_title;
};

#endif
