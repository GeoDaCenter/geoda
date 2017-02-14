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

#ifndef __GEODA_CENTER_MERGE_TO_TALBE_DLG_H__
#define __GEODA_CENTER_MERGE_TO_TALBE_DLG_H__

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

#include "DataSource.h"
#include "../FramesManagerObserver.h"
#include "../ShapeOperations/OGRLayerProxy.h"
#include "../ShapeOperations/OGRDatasourceProxy.h"
#include "../DataViewer/TableInterface.h"

class ConnectDatasourceDlg;
class FramesManager;

class MergeTableDlg: public wxDialog, public FramesManagerObserver
{    
public:
    MergeTableDlg(wxWindow* parent,
                  TableInterface* _table_int,
                  FramesManager* frames_manager,
                  const wxPoint& pos = wxDefaultPosition);
	virtual ~MergeTableDlg();

    void CreateControls();
	void Init();
    
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
    
	void OnKeyValRB( wxCommandEvent& ev );
	void OnRecOrderRB( wxCommandEvent& ev );
	void OnOpenClick( wxCommandEvent& ev );
	void OnIncAllClick( wxCommandEvent& ev );
	void OnIncOneClick( wxCommandEvent& ev );
	void OnIncListDClick(wxCommandEvent& ev );
	void OnExclAllClick( wxCommandEvent& ev );
	void OnExclOneClick( wxCommandEvent& ev );
	void OnExclListDClick(wxCommandEvent& ev );
    void OnMergeClick( wxCommandEvent& ev );
	void OnKeyChoice( wxCommandEvent& ev );
	void OnCloseClick( wxCommandEvent& ev );
    void OnClose( wxCloseEvent& ev);
	void UpdateMergeButton();
	//void RemoveDbfReader();
	//void UpdateIncListItems();
	
	wxTextCtrl* m_input_file_name;
	wxRadioButton* m_key_val_rb;
	wxRadioButton* m_rec_order_rb;
	wxChoice* m_current_key;
	wxChoice* m_import_key;
	wxListBox* m_exclude_list;
	wxListBox* m_include_list;
	
	//TableBase* table_base;
	TableInterface* table_int;
	OGRLayerProxy* merge_layer_proxy;
    OGRDatasourceProxy* merge_datasource_proxy;
	std::set<wxString> table_fnames;
	
private:
    ConnectDatasourceDlg* connect_dlg;
	FramesManager* frames_manager;
    
	std::map<wxString, int> dedup_to_id;
	std::set<wxString> dups;
	// a mapping from displayed col order to actual col ids in table
	// Eg, in underlying table, we might have A, B, C, D, E, F,
	// but because of user wxGrid col reorder operaions might see these
	// as C, B, A, F, D, E.  In this case, the col_id_map would be
	// 0->2, 1->1, 2->0, 3->5, 4->3, 5->4
	//std::vector<int> col_id_map;
    
private:
    void CheckKeys(wxString key_name, std::vector<wxString>& key_vec,
                   std::map<wxString, int>& key_map);
    
    vector<wxString>
    GetSelectedFieldNames(map<wxString,wxString>& merged_fnames_dict);
    
    void AppendNewField(wxString field_name, wxString real_field_name,
                        int n_rows, std::map<int,int>& rowid_map);
    
    
	DECLARE_EVENT_TABLE()
};

#endif
