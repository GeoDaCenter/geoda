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

#ifndef __GEODA_CENTER_DISSOLVE_DLG_H__
#define __GEODA_CENTER_DISSOLVE_DLG_H__

#include <set>
#include <map>
#include <vector>
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/grid.h>

#include "GdaListBox.h"
#include "GdaChoice.h"
#include "../DataViewer/DataSource.h"
#include "../FramesManagerObserver.h"
#include "../ShapeOperations/OGRLayerProxy.h"
#include "../ShapeOperations/OGRDatasourceProxy.h"
#include "../DataViewer/TableInterface.h"

class FramesManager;
class Project;
class ExportDataDlg;
class OGRColumn;

class DissolveDlg: public wxDialog, public FramesManagerObserver
{    
public:
    DissolveDlg(wxWindow* parent,
                  Project* project_p,
                  const wxPoint& pos = wxDefaultPosition);
	virtual ~DissolveDlg();

    void CreateControls();
	void Init();
    
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
    
	void OnIncAllClick( wxCommandEvent& ev );
	void OnIncOneClick( wxCommandEvent& ev );
	void OnIncListDClick(wxCommandEvent& ev );
	void OnExclAllClick( wxCommandEvent& ev );
	void OnExclOneClick( wxCommandEvent& ev );
	void OnExclListDClick(wxCommandEvent& ev );
    
    void OnOKClick( wxCommandEvent& ev );
	void OnKeyChoice( wxCommandEvent& ev );
    
	void OnCloseClick( wxCommandEvent& ev );
    void OnClose( wxCloseEvent& ev);
	void OnLeftJoinClick( wxCommandEvent& ev );
	void OnOuterJoinClick( wxCommandEvent& ev );
	void UpdateMergeButton();
	void OnMethodSelect( wxCommandEvent& ev );

	GdaChoice* m_current_key;
	GdaListBox* m_exclude_list;
	GdaListBox* m_include_list;
	wxRadioButton* m_count;
    wxRadioButton* m_sum;
    wxRadioButton* m_avg;
    wxRadioButton* m_max;
    wxRadioButton* m_min;
    wxButton* m_inc_all;
    wxButton* m_inc_one;
    wxButton* m_exc_all;
    wxButton* m_exc_one;

    ExportDataDlg* export_dlg;
    
    Project* project_s;
	TableInterface* table_int;
	
private:
	FramesManager* frames_manager;
    
	std::map<wxString, int> dedup_to_id;
	std::set<wxString> dups;
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	//std::vector<int> col_id_map;
    
    bool CheckKeys(wxString key_name, std::vector<wxString>& key_vec,
                   std::map<int, std::vector<int> >& key_map);
    
    OGRColumn* CreateNewOGRColumn(int new_rows, int col_id, int tm_id,
                                  std::map<wxString, std::vector<int> >& key_map);
    
    double ComputeAgg(std::vector<double>& vals, std::vector<bool>& undefs, std::vector<int>& ids);
    
	DECLARE_EVENT_TABLE()
};

#endif
