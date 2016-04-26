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

#ifndef __GEODA_CENTER_FIELD_NEW_CALC_SHEET_DLG_H__
#define __GEODA_CENTER_FIELD_NEW_CALC_SHEET_DLG_H__

#include <wx/notebook.h>
#include <wx/panel.h>
#include "FieldNewCalcSpecialDlg.h"
#include "FieldNewCalcBinDlg.h"
#include "FieldNewCalcLagDlg.h"
#include "FieldNewCalcRateDlg.h"
#include "FieldNewCalcUniDlg.h"
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"

class FramesManager;
class TableState;
class Project;
class WeightsManState;

class FieldNewCalcSheetDlg: public wxDialog, public FramesManagerObserver,
	public TableStateObserver, public WeightsManStateObserver
{    
    DECLARE_EVENT_TABLE()

public:
    FieldNewCalcSheetDlg(Project* project,
						 wxWindow* parent, wxWindowID id = wxID_ANY,
						 const wxString& caption = "Var Calc Container",
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize,
						 long style = wxDEFAULT_DIALOG_STYLE );
	virtual ~FieldNewCalcSheetDlg();

    bool Create( wxWindow* parent, wxWindowID id = -1,
				const wxString& caption = "Var Calc Container",
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxDEFAULT_DIALOG_STYLE );

    void CreateControls();

	void OnPageChange( wxBookCtrlEvent& event );
    void OnApplyClick( wxCommandEvent& event );
	void OnClose(wxCloseEvent& event);
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};

private:
    wxNotebook* m_note;
	FieldNewCalcSpecialDlg* pSpecial;
	FieldNewCalcBinDlg* pBin;
	FieldNewCalcLagDlg* pLag;
	FieldNewCalcRateDlg* pRate;
	FieldNewCalcUniDlg* pUni;
	
	FramesManager* frames_manager;
	TableState* table_state;
	WeightsManState* w_man_state;
	Project* project;
};

#endif
