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

#ifndef __GEODA_CENTER_SELECT_WEIGHTS_DLG_H__
#define __GEODA_CENTER_SELECT_WEIGHTS_DLG_H__

#include <vector>
#include <boost/uuid/uuid.hpp>
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

class Project;
class WeightsManInterface;

class SelectWeightsDlg: public wxDialog
{    
public:
	SelectWeightsDlg(Project* project,
					 wxWindow* parent,
                     const wxString& caption = _("Choose Weights"),
					 wxWindowID id = wxID_ANY,
					 const wxPoint& pos = wxDefaultPosition,
					 const wxSize& size = wxDefaultSize,
					 long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

	void OnWListItemDblClick(wxListEvent& ev);
	void OnWListItemSelect(wxListEvent& ev);
	void OnWListItemDeselect(wxListEvent& ev);
	void OnOkClick(wxCommandEvent& ev);
	void OnCancelClick(wxCommandEvent& ev);
	
	boost::uuids::uuid GetSelWeightsId();
	
private:
	void InitWeightsList();
	void SetDetailsForId(boost::uuids::uuid id);
	void SetDetailsWin(const std::vector<wxString>& row_title,
					   const std::vector<wxString>& row_content);
	void SelectId(boost::uuids::uuid id);
	void HighlightId(boost::uuids::uuid id);
	void UpdateButtons();
	
	void InitNoWeights();
	void InitNormal();
	
	wxPanel* panel;
	wxButton* ok_btn;
	wxButton* cancel_btn;
	wxListCtrl* w_list;	// ID_W_LIST
	static const long TITLE_COL = 0;
	wxWebView* details_win;
	
	std::vector<boost::uuids::uuid> ids;
	WeightsManInterface* w_man_int;
	bool no_weights;
};

#endif
