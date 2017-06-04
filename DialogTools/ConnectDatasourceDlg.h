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

#ifndef __GEODA_CENTER_CONNECT_DATASOURCE_DLG_H__
#define __GEODA_CENTER_CONNECT_DATASOURCE_DLG_H__


#include <vector>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>

#include <wx/checkbox.h>
#include "../DataViewer/DataSource.h"
#include "AutoCompTextCtrl.h"
#include "DatasourceDlg.h"

using namespace std;

//
// Class RecentDatasource
//
//
class RecentDatasource
{
public:
    RecentDatasource();
    virtual ~RecentDatasource();
   
    void Add(wxString ds_name, wxString ds_conf, wxString ds_layername,
             wxString ds_thumb = "");
    void Add(IDataSource* ds, const wxString& layer_name, wxString ds_thumb="");
    void Clear();
    void Save();
    void Delete(int idx);
    void DeleteLastRecord();
  
    int GetRecords() {return n_ds;}
    wxString GetLastIndex();
    wxString GetLastLayerName();
    wxString GetLastDSName();
    void UpdateLastThumb(wxString ds_thumb);
    vector<wxString> GetList();
   
    IDataSource* GetDatasource(wxString ds_name);
    wxString GetLayerName(wxString ds_name);
   
    wxString GetDSName(int idx) {return ds_names[idx];}
    wxString GetDSLayerName(int idx) {return ds_layernames[idx];}
    wxString GetDSThumbnail(int idx) {return ds_thumbnails[idx];}
protected:
    static const int N_MAX_ITEMS;
    static const string KEY_NAME_IN_GDA_HISTORY;
    
    int n_ds;
    wxString ds_json_str;
   
    vector<wxString> ds_names;
    vector<wxString> ds_layernames;
    vector<wxString> ds_confs;
    vector<wxString> ds_thumbnails;
    
    void Init(wxString json_str);
};



////////////////////////////////////////////////////////////////////////////////
//
// Class ConnectDatasourceDlg
// 
////////////////////////////////////////////////////////////////////////////////
class ConnectDatasourceDlg: public DatasourceDlg
{
public:
	ConnectDatasourceDlg(wxWindow* parent,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize,
                         bool showCsvConfigure=true,
                         bool showRecentPanel=GdaConst::show_recent_sample_connect_ds_dialog,
                         int dialogType = 0);
    virtual ~ConnectDatasourceDlg();

    void TriggerOKClick();
    virtual void OnOkClick( wxCommandEvent& event );
    void CreateControls();
	void OnLookupWSLayerBtn( wxCommandEvent& event );
	void OnLookupDSTableBtn( wxCommandEvent& event );
	void OnLookupCartoDBTableBtn( wxCommandEvent& event );
	IDataSource* GetDataSource(){ return datasource; }
    
    
protected:
    bool showCsvConfigure;
    bool showRecentPanel;
    
    wxStaticBitmap* m_drag_drop_box;
	wxBitmapButton* m_database_lookup_table;
	wxBitmapButton* m_database_lookup_wslayer;
    wxTextCtrl*   m_database_table;
	AutoTextCtrl*  m_webservice_url;
	IDataSource*   datasource;
    wxPanel* recent_panel;
    wxPanel* smaples_panel;
    wxScrolledWindow* scrl;
    wxNotebook* recent_nb;
    wxCheckBox* noshow_recent;
    wxChoice* m_web_choice;
   
    int base_xrcid_recent_thumb;
    int base_xrcid_sample_thumb;
    void AddRecentItem(wxBoxSizer* sizer, wxScrolledWindow* scrl,
                       wxString ds_name, wxString ds_layername,
                       wxString ds_thumb, int id);
    void AddSampleItem(wxBoxSizer* sizer, wxScrolledWindow* scrl,
                       wxString name, 
                       wxString ds_name, wxString ds_layername,
                       wxString ds_thumb, int id);
    void InitRecentPanel();
    void InitSamplePanel();
    IDataSource* CreateDataSource();
    void SaveRecentDataSource(IDataSource* ds, const wxString& layer_name);
    
    void OnRecent(wxCommandEvent& event);
    void OnSample(wxCommandEvent& event);
    void OnRecentDelete(wxCommandEvent& event);
    
    void OnNoShowRecent(wxCommandEvent& event);
   
    
	DECLARE_EVENT_TABLE()
};

#endif
