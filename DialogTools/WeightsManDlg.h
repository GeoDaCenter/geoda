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

#ifndef __GEODA_CENTER_WEIGHTS_MAN_DLG_H__
#define __GEODA_CENTER_WEIGHTS_MAN_DLG_H__

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
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../FramesManagerObserver.h"
#include "../GenUtils.h"
#include "../GdaConst.h"

class ConnectivityHistCanvas;
class ConnectivityMapCanvas;
class TableInterface;
class WeightsManState;
class WeightsManInterface;

class WeightsManFrame : public TemplateFrame, public WeightsManStateObserver
{
public:
    WeightsManFrame(wxFrame *parent, Project* project,
					const wxString& title = "Weights Manager",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::weights_man_dlg_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~WeightsManFrame();
	
	void OnActivate(wxActivateEvent& ev);
	
	void OnWListItemSelect(wxListEvent& ev);
	void OnWListItemDeselect(wxListEvent& ev);
	
	void OnCreateBtn(wxCommandEvent& ev);
	void OnLoadBtn(wxCommandEvent& ev);
	void OnRemoveBtn(wxCommandEvent& ev);
    void OnHistogramBtn(wxCommandEvent& ev);
    void OnConnectMapBtn(wxCommandEvent& ev);
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};
	
	void OnShowAxes(wxCommandEvent& event);
    void OnDisplayStatistics(wxCommandEvent& event);
	void OnHistogramIntervals(wxCommandEvent& event);
	void OnSaveConnectivityToTable(wxCommandEvent& event);
	void OnSelectIsolates(wxCommandEvent& event);
	
private:
	void InitWeightsList();
	void SetDetailsForId(boost::uuids::uuid id);
	void SetDetailsWin(const std::vector<wxString>& row_title,
					   const std::vector<wxString>& row_content);
	void SelectId(boost::uuids::uuid id);
	void HighlightId(boost::uuids::uuid id);
	boost::uuids::uuid GetHighlightId();
	void UpdateButtons();
	
	ConnectivityHistCanvas* conn_hist_canvas;
	ConnectivityMapCanvas* conn_map_canvas;

    Project* project_p;
	wxPanel* panel;
	wxButton* histogram_btn; //
    wxButton* connectivity_map_btn; //
	wxButton* create_btn; // ID_CREATE_BTN
	wxButton* load_btn; // ID_LOAD_BTN
	wxButton* remove_btn; // ID_REMOVE_BTN
	wxListCtrl* w_list;	// ID_W_LIST
	static const long TITLE_COL = 0;
	wxWebView* details_win;
    
    long cur_sel_item;

	std::vector<boost::uuids::uuid> ids;
	WeightsManInterface* w_man_int;
	WeightsManState* w_man_state;
	bool suspend_w_man_state_updates;
	TableInterface* table_int;
	
	DECLARE_EVENT_TABLE()
};


#endif
