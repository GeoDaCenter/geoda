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

#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <wx/dir.h>
#include <wx/progdlg.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/regex.h>
#include <wx/bmpbuttn.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>
#include <wx/notebook.h>
#include <wx/hyperlink.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include <json_spirit/json_spirit_reader.h>

#include "../DialogTools/CsvFieldConfDlg.h"
#include "../DataViewer/DataSource.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../GdaCartoDB.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "../rc/GeoDaIcon-16x16.xpm"
#include "ConnectDatasourceDlg.h"

using namespace std;

DnDFile::DnDFile(ConnectDatasourceDlg *pOwner)
{
    m_pOwner = pOwner;
}

DnDFile::~DnDFile()
{
    
}

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
    size_t nFiles = filenames.GetCount();
    
    if (m_pOwner != NULL && nFiles > 0)
    {
        wxString fpath = filenames[0];
        wxFileName fn = wxFileName::FileName(fpath);

        // activate file/dir access permission for osx 10.15+
        wxFileName dirname = wxFileName::DirName(fpath);
        wxString dir_path = dirname.GetPath();
        wxDir dir;
        dir.Open(dir_path);
        if ( dir.IsOpened() ) {
            wxString filename;
            dir.GetFirst(&filename);
        }

        m_pOwner->ds_file_path = fn;
        wxCommandEvent ev;
        m_pOwner->OnOkClick(ev);
    }
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// Class RecentDatasource
////////////////////////////////////////////////////////////////////////////////

const int RecentDatasource::N_MAX_ITEMS = 10;
const std::string RecentDatasource::KEY_NAME_IN_GDA_HISTORY = "recent_ds";

RecentDatasource::RecentDatasource()
{
    n_ds =0;
    // get a latest input DB information
    std::vector<wxString> ds_infos = OGRDataAdapter::GetInstance().GetHistory(KEY_NAME_IN_GDA_HISTORY);
    
    if (ds_infos.size() > 0) {
        ds_json_str = ds_infos[0];
        Init(ds_json_str);
    }
}

RecentDatasource::~RecentDatasource()
{
    
}

void RecentDatasource::Init(wxString json_str_)
{
    if (json_str_.IsEmpty())
        return;
    
    // "recent_ds" : [{"ds_name":"/data/test.shp", "layer_name":"test", "ds_config":"...", "thumb":"..."}, ]
    std::wstring json_str = json_str_.ToStdWstring();
    json_spirit::wValue v;
    
    try {
        if (!json_spirit::read(json_str, v)) {
            throw GdaException("Could not parse recent ds string");
        }
        
        const json_spirit::wArray& ds_list = v.get_array();
        
        n_ds = ds_list.size();
        
        for (size_t i=0; i<n_ds; i++) {
            const json_spirit::wObject& o = ds_list[i].get_obj();
            wxString ds_name, ds_conf, layer_name, ds_thumb;
            
            for (json_spirit::wObject::const_iterator i=o.begin(); i!=o.end();
                 ++i)
            {
                json_spirit::wValue val;
                if (i->name_ == L"ds_name") {
                    val = i->value_;
					ds_name = val.get_str();
                }
                else if (i->name_ == "layer_name") {
                    val = i->value_;
                    layer_name = val.get_str();
                }
                else if (i->name_ == "ds_config") {
                    val = i->value_;
                    ds_conf = val.get_str();
                }
                else if (i->name_ == "ds_thumb") {
                    val = i->value_;
                    ds_thumb = val.get_str();
                }
            }
            ds_names.push_back(ds_name);
            ds_layernames.push_back(layer_name);
            ds_confs.push_back(ds_conf);
            ds_thumbnails.push_back(ds_thumb);
        }
    } catch (std::runtime_error e) {
        wxString msg;
        msg << "Get Latest DB infor: JSON parsing failed: ";
        msg << e.what();
        throw GdaException(msg.mb_str());
    }
}

void RecentDatasource::Save()
{
    // update ds_json_str from ds_names & ds_values
    json_spirit::wArray ds_list_obj;
    
    for (int i=0; i<n_ds; i++) {
        json_spirit::wObject ds_obj;
        //const char* ds_name = ds_names[i].mb_str(wxConvUTF8);
        //const char* layer_name = ds_layernames[i].mb_str(wxConvUTF8);
        //const char* ds_conf = ds_confs[i].mb_str(wxConvUTF8);
        //const char* ds_thumb = ds_thumbnails[i].mb_str(wxConvUTF8);
        std::wstring ds_name = ds_names[i].ToStdWstring();
        std::wstring layer_name = ds_layernames[i].ToStdWstring();
        std::wstring ds_conf = ds_confs[i].ToStdWstring();
        std::wstring ds_thumb = ds_thumbnails[i].ToStdWstring();
        //std::string ds_name( ds_names[i].mb_str(wxConvUTF8));
        //std::string layer_name( ds_layernames[i].mb_str(wxConvUTF8));
		//std::string ds_conf( ds_confs[i].mb_str(wxConvUTF8) );
        //std::string ds_thumb( ds_thumbnails[i].mb_str(wxConvUTF8) );
        ds_obj.push_back( json_spirit::wPair(L"ds_name", ds_name) );
        ds_obj.push_back( json_spirit::wPair(L"layer_name", layer_name) );
        ds_obj.push_back( json_spirit::wPair(L"ds_config", ds_conf) );
        ds_obj.push_back( json_spirit::wPair(L"ds_thumb", ds_thumb) );
        ds_list_obj.push_back( ds_obj);
    }
    
    ds_json_str = json_spirit::write(ds_list_obj);
    OGRDataAdapter::GetInstance().AddEntry(KEY_NAME_IN_GDA_HISTORY, ds_json_str);
}

void RecentDatasource::Add(wxString ds_name, wxString ds_conf, wxString layer_name,
                           wxString ds_thumb)
{
    // remove existed one
    n_ds = ds_names.size();
    int search_idx = -1;
    
    for (int i=0; i<n_ds; i++) {
        if (ds_names[i] == ds_name) {
            search_idx = i;
            break;
        }
    }
    
    if (search_idx >= 0) {
        ds_names.erase(ds_names.begin() + search_idx);
        ds_confs.erase(ds_confs.begin() + search_idx);
        ds_layernames.erase(ds_layernames.begin() + search_idx);
        
        wxString thumbnail_name = ds_thumbnails[search_idx];
        wxString file_path_str;
        file_path_str << GenUtils::GetSamplesDir() << thumbnail_name;
       
        if (wxFileExists(file_path_str) ) {
            wxRemoveFile(file_path_str);
        }
        
        ds_thumbnails.erase(ds_thumbnails.begin() + search_idx);
    }
    
    n_ds = ds_names.size();
    
    if (n_ds < N_MAX_ITEMS) {
        ds_names.push_back(ds_name);
        ds_confs.push_back(ds_conf);
        ds_layernames.push_back(layer_name);
        ds_thumbnails.push_back(ds_thumb);
        
        n_ds = ds_names.size();
    } else {
        ds_names.erase(ds_names.begin());
        ds_confs.erase(ds_confs.begin());
        ds_layernames.erase(ds_layernames.begin());
        
        wxString thumbnail_name = ds_thumbnails[0];
        wxString file_path_str;
        file_path_str << GenUtils::GetSamplesDir() << thumbnail_name;
      
        if (wxFileExists(file_path_str) ) {
            wxRemoveFile(file_path_str);
        }
        
        ds_thumbnails.erase(ds_thumbnails.begin());
        
        ds_names.push_back(ds_name);
        ds_confs.push_back(ds_conf);
        ds_layernames.push_back(layer_name);
        ds_thumbnails.push_back(ds_thumb);
    }
    
    Save();
}

void RecentDatasource::Add(IDataSource* ds, const wxString& layer_name, wxString ds_thumb)
{
    wxString ds_name = ds->GetOGRConnectStr();
    wxString ds_conf = ds->GetJsonStr();
    
    Add(ds_name, ds_conf, layer_name, ds_thumb);
}

void RecentDatasource::Delete(int idx)
{
    if (idx >= 0) {
        ds_names.erase(ds_names.begin() + idx);
        ds_confs.erase(ds_confs.begin() + idx);
        ds_layernames.erase(ds_layernames.begin() + idx);
        
        wxString thumbnail_name = ds_thumbnails[idx];
        wxString file_path_str;
        file_path_str << GenUtils::GetSamplesDir() << thumbnail_name;
       
        if (wxFileExists(file_path_str) ) {
            wxRemoveFile(file_path_str);
        }
        
        ds_thumbnails.erase(ds_thumbnails.begin() + idx);
        
        n_ds = ds_names.size();
        Save();
    }
}

void RecentDatasource::DeleteLastRecord()
{
    if (n_ds > 0) {
        Delete(n_ds -1);
    }
}

wxString RecentDatasource::GetLastIndex()
{
    int last_idx = ds_names.size() - 1;
    if (last_idx < 0)
        last_idx = 0;
    wxString str;
    str << last_idx;
    return str;
}

wxString RecentDatasource::GetLastLayerName()
{
    int last_idx = ds_layernames.size() - 1;
    if (last_idx < 0)
        return "";
    wxString str;
    str << ds_layernames[last_idx];
    return str;
}

wxString RecentDatasource::GetLastDSName()
{
    int last_idx = ds_names.size() - 1;
    if (last_idx < 0)
        return "";
    wxString str;
    str << ds_names[last_idx];
    return str;
}

void RecentDatasource::UpdateLastThumb(wxString ds_thumb)
{
    int last_idx = ds_names.size() - 1;
    if (last_idx >= 0) {
        
        ds_thumbnails[last_idx] = ds_thumb;
    }
    Save();
}

void RecentDatasource::Clear()
{
    OGRDataAdapter::GetInstance().AddEntry(KEY_NAME_IN_GDA_HISTORY, "");
}

std::vector<wxString> RecentDatasource::GetList()
{
    return ds_names;
}

IDataSource* RecentDatasource::GetDatasource(wxString ds_name)
{
    for (int i=0; i<n_ds; i++) {
        if (ds_names[i] == ds_name) {
            wxString ds_conf = ds_confs[i];
            try {
                return IDataSource::CreateDataSource(ds_confs[i]);
            } catch(GdaException& e){
                return NULL;
            }
        }
    }
    return NULL;
}

wxString RecentDatasource::GetLayerName(wxString ds_name)
{
    for (int i=0; i<n_ds; i++) {
        if (ds_names[i] == ds_name) {
            wxString ds_layername = ds_layernames[i];
            return ds_layername;
        }
    }
    return wxEmptyString;
}

////////////////////////////////////////////////////////////////////////////////
//
// Class ConnectDatasourceDlg
////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( ConnectDatasourceDlg, wxDialog )
    EVT_BUTTON(XRCID("IDC_OPEN_IASC"), ConnectDatasourceDlg::OnBrowseDSfileBtn)
	EVT_BUTTON(XRCID("ID_BTN_LOOKUP_TABLE"), ConnectDatasourceDlg::OnLookupDSTableBtn)
	//EVT_BUTTON(XRCID("ID_BTN_LOOKUP_WSLAYER"), ConnectDatasourceDlg::OnLookupWSLayerBtn)
    EVT_BUTTON(wxID_OK, ConnectDatasourceDlg::OnOkClick )
    //EVT_BUTTON(wxID_CANCEL, ConnectDatasourceDlg::OnCancelClick )
    //EVT_BUTTON(wxID_EXIT, ConnectDatasourceDlg::OnCancelClick )
    //EVT_MENU(wxID_EXIT, ConnectDatasourceDlg::OnCancelClick )
END_EVENT_TABLE()

ConnectDatasourceDlg::ConnectDatasourceDlg(wxWindow* parent, const wxPoint& pos,
                                           const wxSize& size,
                                           bool showCsvConfigure_,
                                           bool showRecentPanel_,
                                           int _dialogType)
:DatasourceDlg(), datasource(0), scrl(0), recent_panel(0),
showCsvConfigure(showCsvConfigure_),
showRecentPanel(showRecentPanel_),
m_wx_encoding(NULL)
{
    dialogType = _dialogType;

    base_xrcid_recent_thumb = 7000;
    base_xrcid_sample_thumb = 7500;
    
    // init controls defined in parent class
    DatasourceDlg::Init();
    ds_names.Add("GeoDa Project File (*.gda)|*.gda");
	SetParent(parent);
	CreateControls();
	SetPosition(pos);

    if (dialogType == 1) {
        // in case of Merge, show encoding choice ctrl
        m_encodings->Show();
        m_encoding_lbl->Show();
    }

    if (showRecentPanel) {
        RecentDatasource recent_ds;
        if (recent_ds.GetRecords() > 0) {
            wxBoxSizer* sizer;
            sizer = new wxBoxSizer( wxVERTICAL );
            
            InitRecentPanel();
            sizer->Add( scrl, 1, wxEXPAND | wxRIGHT, 5 );
            
            recent_panel->SetSizer( sizer );
            recent_panel->Layout();
            sizer->Fit( recent_panel );
        }
        InitSamplePanel();
    }

    m_dnd = new DnDFile(this);
    m_drag_drop_box->SetDropTarget(m_dnd);
   
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));

    Bind(wxEVT_COMMAND_MENU_SELECTED, &ConnectDatasourceDlg::BrowseDataSource,
         this, GdaConst::ID_CONNECT_POPUP_MENU,
         GdaConst::ID_CONNECT_POPUP_MENU + ds_names.Count());
    
	Centre();
    Move(pos);
    
    GetSizer()->Fit(this);
    Restore();
    Raise();
}

ConnectDatasourceDlg::~ConnectDatasourceDlg()
{
    if (datasource) {
        delete datasource;
        datasource = NULL;
    }
    if (m_wx_encoding) {
        delete m_wx_encoding;
    }
}



void ConnectDatasourceDlg::AddRecentItem(wxBoxSizer* sizer, wxScrolledWindow* scrl,
                                         wxString ds_name, wxString ds_layername,
                                         wxString ds_thumb, int id)
{
	wxString file_path_str;
    if (ds_thumb.IsEmpty()) {
        ds_thumb = "no_map.png";
    }
    file_path_str = GenUtils::GetSamplesDir() + ds_thumb;
    
    wxImage img;
    if (!wxFileExists(file_path_str)) {
#ifdef __linux__
        file_path_str = GenUtils::GetUserSamplesDir() + ds_thumb;
        if (!wxFileExists(file_path_str)) {
            ds_thumb = "no_map.png";
            file_path_str = GenUtils::GetSamplesDir() + ds_thumb;
        }
#else
        ds_thumb = "no_map.png";
        file_path_str = GenUtils::GetSamplesDir() + ds_thumb;
#endif
    }
    img.LoadFile(file_path_str);
    if (!img.IsOk()) {
        ds_thumb = "no_map.png";
        file_path_str = GenUtils::GetSamplesDir() + ds_thumb;
        img.LoadFile(file_path_str);
    }
    img.Rescale(100,66,wxIMAGE_QUALITY_HIGH );
    wxBitmap bmp(img);
    
    wxBitmapButton* thumb;
    thumb = new wxBitmapButton(scrl, id, bmp);
    thumb->Bind(wxEVT_BUTTON, &ConnectDatasourceDlg::OnRecent, this);
    

    wxBoxSizer* text_sizer;
    text_sizer = new wxBoxSizer( wxVERTICAL );
    
    wxString lbl_ds_layername = ds_layername;
    lbl_ds_layername = GenUtils::PadTrim(lbl_ds_layername, 28, false);
    
    wxBoxSizer* title_sizer;
    title_sizer = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* layername;
    layername = new wxStaticText(scrl, wxID_ANY,  lbl_ds_layername.Trim());
    layername->SetFont(*GdaConst::medium_font);
    layername->SetToolTip(ds_layername);
    layername->SetForegroundColour(wxColour(100,100,100));
   
#ifdef __WIN32__
    int pad_remove_btn = 10;
	wxButton *remove = new wxButton(scrl, id, _("Delete"), wxDefaultPosition, wxSize(36,18), wxBORDER_NONE|wxBU_EXACTFIT);
	remove->SetFont(*GdaConst::extra_small_font); 
#else
    int pad_remove_btn = 0;
	wxBitmap remove_bitmap(GdaConst::delete_icon_xpm);
    wxBitmapButton* remove;
    remove = new wxBitmapButton(scrl, id, remove_bitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE|wxBU_EXACTFIT);
#endif
	remove->Bind(wxEVT_BUTTON, &ConnectDatasourceDlg::OnRecentDelete, this);
    
    title_sizer->Add(layername, 1, wxALIGN_LEFT |wxALIGN_CENTER_VERTICAL | wxRIGHT, pad_remove_btn);
    title_sizer->Add(remove, 0, wxALIGN_LEFT |wxALIGN_CENTER_VERTICAL| wxALIGN_TOP | wxLEFT, 5);
    
    text_sizer->Add(title_sizer, 1, wxALIGN_LEFT | wxALL, 5);
    
    wxString lbl_ds_name = ds_name;
    lbl_ds_name = GenUtils::PadTrim(lbl_ds_name, 50, false);
    wxStaticText* filepath;
    filepath = new wxStaticText(scrl, wxID_ANY, lbl_ds_name);
    filepath->SetFont(*GdaConst::extra_small_font);
    filepath->SetToolTip(ds_name);
    filepath->SetForegroundColour(wxColour(70,70,70));
    text_sizer->Add(filepath, 1, wxALIGN_LEFT | wxALL, 10);
  
    
    wxBoxSizer* row_sizer;
    row_sizer = new wxBoxSizer( wxHORIZONTAL );
    row_sizer->Add(thumb, 0, wxALIGN_CENTER | wxALL, 0);
    row_sizer->Add(text_sizer, 1, wxALIGN_LEFT | wxALIGN_TOP | wxEXPAND | wxTOP, 5);
    
    sizer->Add(row_sizer, 0, wxALIGN_LEFT | wxALL, 2);
}

void ConnectDatasourceDlg::OnRecentDelete(wxCommandEvent& event)
{
    int xrcid = event.GetId();
    int recent_idx = xrcid - base_xrcid_recent_thumb;
    
    RecentDatasource recent_ds;
    recent_ds.Delete(recent_idx);
   
    InitRecentPanel();
}

void ConnectDatasourceDlg::OnRecent(wxCommandEvent& event)
{
    int xrcid = event.GetId();
    int recent_idx = xrcid - base_xrcid_recent_thumb;
    
    RecentDatasource recent_ds;
    wxString ds_name = recent_ds.GetDSName(recent_idx); // UTF-8 decoded
        
    if (ds_name.EndsWith(".gda")) {
        GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
        gda_frame->OpenProject(ds_name);
        Project* project = gda_frame->GetProject();
        wxString layer_name;
        if (project) {
            layer_name = project->layername;
        }
        recent_ds.Add(ds_name, ds_name, layer_name);
        EndDialog(wxID_CANCEL);
    } else {
        // For csv file, if no csvt file,
        // pop-up a field definition dialog and create a csvt file
        if (ds_name.EndsWith("csv") && showCsvConfigure) {
            if (wxFileExists(ds_name)) {
                CsvFieldConfDlg csvDlg(this, ds_name, m_wx_encoding);
                csvDlg.ShowModal();
            }
        }
        IDataSource* ds = recent_ds.GetDatasource(ds_name);
        if (ds == NULL) {
            // raise message dialog show can't connect to datasource
            wxString msg = _("Can't connect to datasource: ") + ds_name;
            wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return;
        } else {
            wxString layername = recent_ds.GetLayerName(ds_name);
            SaveRecentDataSource(ds, layername);
            layer_name = layername;
            datasource = ds;
            EndDialog(wxID_OK);
        }
    }
}

void ConnectDatasourceDlg::InitRecentPanel()
{
    if (scrl)
        scrl->Destroy();
    
    scrl = new wxScrolledWindow(recent_panel, wxID_ANY, wxDefaultPosition,
                                wxSize(420,200), wxVSCROLL );
    scrl->SetScrollRate( 5, 5 );
#ifdef __WIN32__
    scrl->SetBackgroundColour(*wxWHITE);
#endif
    
    wxBoxSizer* sizer;
    sizer = new wxBoxSizer( wxVERTICAL );
    
    RecentDatasource recent_ds;
    int n_records = recent_ds.GetRecords();
   
    if (n_records > 0) {
        recent_nb->SetSelection(0);
    }
    for (int i=n_records-1; i>=0; i--) {
        wxString ds_name = recent_ds.GetDSName(i);
        wxString ds_layername = recent_ds.GetDSLayerName(i);
        wxString ds_thumb = recent_ds.GetDSThumbnail(i);
        AddRecentItem(sizer, scrl, ds_name, ds_layername, ds_thumb, base_xrcid_recent_thumb+i);
    }
    
    scrl->SetSizer( sizer );
    scrl->Layout();
    sizer->Fit( scrl );
}

void ConnectDatasourceDlg::CreateControls()
{
    if (showRecentPanel) {
        wxXmlResource::Get()->LoadDialog(this, GetParent(),"IDD_CONNECT_DATASOURCE");
    	recent_nb = XRCCTRL(*this, "IDC_DS_LIST",  wxNotebook);
        recent_nb->SetSelection(1);
        recent_panel = XRCCTRL(*this, "dsRecentListSizer", wxPanel);
        smaples_panel = XRCCTRL(*this, "dsSampleList", wxPanel);

    } else {
        wxXmlResource::Get()->LoadDialog(this, GetParent(),"IDD_CONNECT_DATASOURCE_SIMPLE");
        m_encodings = XRCCTRL(*this, "IDC_CDS_ENCODING_CHOICE",wxChoice);
        m_encoding_lbl = XRCCTRL(*this, "IDC_CDS_ENCODING_LABEL",wxStaticText);
    }
    
    FindWindow(XRCID("wxID_OK"))->Enable(true);
    // init db_table control that is unique in this class
    m_drag_drop_box = XRCCTRL(*this, "IDC_DRAG_DROP_BOX",wxStaticBitmap);
	m_webservice_url = XRCCTRL(*this, "IDC_CDS_WS_URL",AutoTextCtrl);
	m_database_lookup_table = XRCCTRL(*this, "ID_BTN_LOOKUP_TABLE",  wxBitmapButton);
    m_database_lookup_table->Hide();
	//m_database_lookup_wslayer = XRCCTRL(*this, "ID_BTN_LOOKUP_WSLAYER", wxBitmapButton);
	m_database_table = XRCCTRL(*this, "IDC_CDS_DB_TABLE", wxTextCtrl);
    m_database_table->Hide(); // don't need this
    
    XRCCTRL(*this, "IDC_STATIC_DB_TABLE", wxStaticText)->Hide();
    
    m_web_choice =  XRCCTRL(*this, "ID_CDS_WEB_CHOICE", wxChoice);
    
    noshow_recent = XRCCTRL(*this, "IDC_NOSHOW_RECENT_SAMPLES", wxCheckBox);
    noshow_recent->Bind(wxEVT_CHECKBOX, &ConnectDatasourceDlg::OnNoShowRecent, this);
    if (!showRecentPanel) {
        noshow_recent->Hide();
    }
    
    // create controls defined in parent class
    DatasourceDlg::CreateControls();

    // setup WSF auto-completion
	std::vector<wxString> ws_url_cands = OGRDataAdapter::GetInstance().GetHistory("ws_url");
	m_webservice_url->SetAutoList(ws_url_cands);
}

void ConnectDatasourceDlg::OnNoShowRecent( wxCommandEvent& event)
{
    //recent_nb->Hide();
    //noshow_recent->Hide();
    //GetSizer()->Fit(this);
    
    showRecentPanel = false;
    GdaConst::show_recent_sample_connect_ds_dialog = false;
    OGRDataAdapter::GetInstance().AddEntry("show_recent_sample_connect_ds_dialog", "0");
}

/**
 * This functions handles the event of user click the "lookup" button in
 * Web Service tab
 */
void ConnectDatasourceDlg::OnLookupWSLayerBtn( wxCommandEvent& event )
{
    //XXX handling invalid wfs url:
    //http://walkableneighborhoods.org/geoserver/wfs
	OnLookupDSTableBtn(event);
}

/**
 * This functions handles the event of user click the "lookup" button in
 * Database Tab.
 * When click "lookup" button in Datasource Table, this will call
 * PromptDSLayers() function
 */
void ConnectDatasourceDlg::OnLookupDSTableBtn( wxCommandEvent& event )
{
	try {
        CreateDataSource();
        PromptDSLayers(datasource);
        m_database_table->SetValue(layer_name );
	} catch (GdaException& e) {
		wxString msg;
		msg << e.what();
		wxMessageDialog dlg(this, msg , _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
        if( datasource!= NULL &&
            msg.StartsWith("Failed to open data source") ) {
            if ( datasource->GetType() == GdaConst::ds_oci ){
               wxExecute("open http://geodacenter.github.io/setup-oracle.html");
            } else if ( datasource->GetType() == GdaConst::ds_esri_arc_sde ){
               wxExecute("open http://geodacenter.github.io/etup-arcsde.html");
			} else if ( datasource->GetType() == GdaConst::ds_esri_file_geodb ){
				wxExecute("open http://geodacenter.github.io/setup-esri-fgdb.html");
            } else {
				wxExecute("open http://geodacenter.github.io/formats.html");
			}
        }
	}
}

void ConnectDatasourceDlg::OnLookupCartoDBTableBtn( wxCommandEvent& event )
{
	try {
        CreateDataSource();
        PromptDSLayers(datasource);
    } catch(GdaException& e) {
        
    }
}

/**
 * This function handles the event of user click OK button.
 * When user chooses a data source, validate it first,
 * then create a Project() that will be used by the
 * main program.
 */
void ConnectDatasourceDlg::OnOkClick( wxCommandEvent& event )
{
	wxLogMessage("Entering ConnectDatasourceDlg::OnOkClick");
	try {
        // Open GeoDa project file direclty
        if (ds_file_path.GetExt().Lower() == "gda") {
            if (dialogType == 1) {
                // in Merge/Stack (when dialogType == 1), user can't open gda file
                wxString msg = _("Please open a data file rather than a project file (*.gda).");
                wxMessageDialog dlg(this, msg, _("Info"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                return;
            }
            GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
            if (gda_frame) {
                gda_frame->OpenProject(ds_file_path.GetFullPath());
                wxLogMessage(_("Open project file:") + ds_file_path.GetFullPath());
                try {
                    Project* project = gda_frame->GetProject();
                    wxString layer_name;
                    if (project) layer_name = project->layername;
                    
                    RecentDatasource recent_ds;
                    recent_ds.Add(ds_file_path.GetFullPath(), ds_file_path.GetFullPath(), layer_name);
                } catch( GdaException ex) {
                    wxLogMessage(ex.what());
                }
                EndDialog(wxID_CANCEL);
            }
            return;
        }

        if (dialogType == 1) {
            // case of Merge
            m_wx_encoding = GetEncoding();
        }

        // For csv file, if no csvt file,
        // pop-up a field definition dialog and create a csvt file
        if (ds_file_path.GetExt().Lower() == "csv" && showCsvConfigure) {
            wxString csv_path = ds_file_path.GetFullPath();
            CsvFieldConfDlg csvDlg(this, csv_path, m_wx_encoding);
            csvDlg.ShowModal();
        }
        
		CreateDataSource();
        
        // Check to make sure to get a layer name
        wxString layername;
		int datasource_type = m_ds_notebook->GetSelection();
		if (datasource_type == 0) {
            // File table is selected
            
            if ( wxDirExists(ds_file_path.GetFullPath()) ) {
                // dra-n-drop a directory
                PromptDSLayers(datasource);
                layername = layer_name;
            } else {
                if (layer_name.IsEmpty()) {
                    layername = ds_file_path.GetName();
                } else {
                    // user may select a layer name from Popup dialog that displays
                    // all layer names, see PromptDSLayers()
                    layername = layer_name;
                }
            }
            
		} else if (datasource_type == 1) {
            // Database tab is selected
			layername = m_database_table->GetValue();
            if (layername.IsEmpty()) PromptDSLayers(datasource);
			layername = layer_name;
            
		} else if (datasource_type == 2) {
            // Web Service tab is selected
            if (layer_name.IsEmpty()) PromptDSLayers(datasource);
			layername = layer_name;
            
		} else {
            // Should never be here
			return;
		}
        
        if (layername.IsEmpty())
            return;
        
		// At this point, there is a valid datasource and layername.
        if (layer_name.IsEmpty())
            layer_name = layername;
      
        wxLogMessage(_("Open Datasource:") + datasource->ToString());
        wxLogMessage(_("Open Layer:") + layername);
        
        SaveRecentDataSource(datasource, layer_name);
        
        EndDialog(wxID_OK);
		
	} catch (GdaException& e) {
		wxString msg;
		msg << e.what();
		wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
        
	} catch (...) {
		wxString msg = _("Unknow exception. Please contact GeoDa support.");
		wxMessageDialog dlg(this, msg , _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
	}
	wxLogMessage("Exiting ConnectDatasourceDlg::OnOkClick");
}

/**
 * After user click OK, create a data source connection string based on user
 * inputs
 * Throw GdaException()
 */
IDataSource* ConnectDatasourceDlg::CreateDataSource()
{
	wxLogMessage("ConnectDatasourceDlg::CreateDataSource()");
	if (datasource) {
		delete datasource;
		datasource = NULL;
	}
    
	int datasource_type = m_ds_notebook->GetSelection();
	
	if (datasource_type == 0) {
        // File  tab selected
		wxString fn = ds_file_path.GetFullPath();
		if (fn.IsEmpty()) {
			throw GdaException(
                wxString("Please select a datasource file.").mb_str());
		}
#if defined(_WIN64) || defined(__amd64__)
        if (m_ds_filepath_txt->GetValue().StartsWith("PGeo:")) {
			fn = "PGeo:" + fn;
		}
#endif
        datasource = new FileDataSource(fn);
        
		// a special case: sqlite is a file based database, so we need to get
		// avalible layers and prompt for user selecting
        if (datasource->GetType() == GdaConst::ds_sqlite ||
            datasource->GetType() == GdaConst::ds_gpkg||
            datasource->GetType() == GdaConst::ds_osm ||
            datasource->GetType() == GdaConst::ds_esri_personal_gdb||
            datasource->GetType() == GdaConst::ds_esri_file_geodb)
        {
            PromptDSLayers(datasource);
            if (layer_name.IsEmpty()) {
                throw GdaException(
                    wxString("Layer/Table name could not be empty. Please select a layer/table.").mb_str());
            }
        }
		
	} else if (datasource_type == 1) {
        // Database tab selected
		//int cur_sel = m_database_type->GetSelection();
        wxString cur_sel = m_database_type->GetStringSelection();
		wxString dbname = m_database_name->GetValue().Trim();
		wxString dbhost = m_database_host->GetValue().Trim();
		wxString dbport = m_database_port->GetValue().Trim();
		wxString dbuser = m_database_uname->GetValue().Trim();
		wxString dbpwd  = m_database_upwd->GetValue().Trim();
        
        GdaConst::DataSourceType ds_type = GdaConst::ds_unknown;
        if (cur_sel == DBTYPE_ORACLE) ds_type = GdaConst::ds_oci;
        else if (cur_sel == DBTYPE_ARCSDE) ds_type = GdaConst::ds_esri_arc_sde;
        else if (cur_sel == DBTYPE_POSTGIS) ds_type = GdaConst::ds_postgresql;
        else if (cur_sel == DBTYPE_MYSQL) ds_type = GdaConst::ds_mysql;
        //else if (cur_sel == 4) ds_type = GdaConst::ds_ms_sql;
        else {
            wxString msg = _("The selected database driver is not supported on this platform. Please check GeoDa website for more information about database support and connection.");
            throw GdaException(msg.mb_str());
        }
        
        // save user inputs to history table
        OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
        if (!dbhost.IsEmpty())
            ogr_adapter.AddHistory("db_host", dbhost.ToStdString());
        
        if (!dbname.IsEmpty())
            ogr_adapter.AddHistory("db_name", dbname.ToStdString());
        
        if (!dbport.IsEmpty())
            ogr_adapter.AddHistory("db_port", dbport.ToStdString());
        
        if (!dbuser.IsEmpty())
            ogr_adapter.AddHistory("db_user", dbuser.ToStdString());
        
        // check if empty, prompt user to input
        wxRegEx regex;
        regex.Compile("[0-9]+");
        if (!regex.Matches( dbport )){
            wxString msg = _("Database port is empty. Please input one.");
            throw GdaException(msg.mb_str());
        }
		wxString error_msg;
		if (dbhost.IsEmpty()) error_msg = _("Please input database host.");
		else if (dbname.IsEmpty()) error_msg = _("Please input database name.");
		else if (dbport.IsEmpty()) error_msg = _("Please input database port.");
		else if (dbuser.IsEmpty()) error_msg = _("Please input user name.");
        else if (dbpwd.IsEmpty()) error_msg = _("Please input password.");
		if (!error_msg.IsEmpty()) {
			throw GdaException(error_msg.mb_str() );
		}
        
        // save current db info
        json_spirit::Object ret_obj;
        ret_obj.push_back(json_spirit::Pair("db_host", dbhost.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_port", dbport.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_name", dbname.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_user", dbuser.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_pwd", dbpwd.ToStdString()));
        ret_obj.push_back(json_spirit::Pair("db_type", cur_sel.ToStdString()));
        
        std::string json_str = json_spirit::write(ret_obj);
        OGRDataAdapter::GetInstance().AddEntry("db_info", json_str);
        
        datasource = new DBDataSource(ds_type, dbname, dbhost, dbport, dbuser, dbpwd);
        
	} else if ( datasource_type == 2 ) {
        // Web Service tab selected
        wxString ws_url = m_webservice_url->GetValue().Trim();
        // detect if it's a valid url string
        wxRegEx regex;
        regex.Compile("^(https|http)://");
        if (!regex.Matches( ws_url )){
            wxString msg = _("Please input a valid url address.");
            throw GdaException(msg.mb_str());
        }
        if (ws_url.IsEmpty()) {
            wxString msg = _("Please input a valid url.");
            throw GdaException(msg.mb_str());
        } else {
            OGRDataAdapter::GetInstance().AddHistory("ws_url", ws_url.ToStdString());
        }
        
        if (m_web_choice->GetSelection() == 0 ) {
            datasource = new FileDataSource(ws_url);
        } else {
            if ((!ws_url.StartsWith("WFS:") || !ws_url.StartsWith("wfs:"))
                && !ws_url.EndsWith("SERVICE=WFS"))
            {
                ws_url = "WFS:" + ws_url;
            }
            datasource = new WebServiceDataSource(GdaConst::ds_wfs, ws_url);
            // prompt user to select a layer from WFS
            //if (layer_name.IsEmpty()) PromptDSLayers(datasource);
        }
	}
    
	return datasource;
}
                                                
void ConnectDatasourceDlg::SaveRecentDataSource(IDataSource* ds,
                                                const wxString& layer_name)
{
    wxLogMessage("Entering ConnectDatasourceDlg::SaveRecentDataSource");
    try {
        RecentDatasource recent_ds;
        recent_ds.Add(ds, layer_name);
    } catch( GdaException ex) {
        LOG_MSG(ex.what());
    }
	wxLogMessage("Exiting ConnectDatasourceDlg::SaveRecentDataSource");
}

void ConnectDatasourceDlg::InitSamplePanel()
{
	wxLogMessage("ConnectDatasourceDlg::InitSamplePanel()");
    wxBoxSizer* sizer;
    sizer = new wxBoxSizer( wxVERTICAL );
    
    wxScrolledWindow* sample_scrl;
    sample_scrl = new wxScrolledWindow(smaples_panel, wxID_ANY,
                                       wxDefaultPosition, wxSize(420,200),
                                       wxVSCROLL );
    sample_scrl->SetScrollRate( 5, 5 );
#ifdef __WIN32__
    sample_scrl->SetBackgroundColour(*wxWHITE);
#endif
    {
        wxBoxSizer* sizer;
        sizer = new wxBoxSizer( wxVERTICAL );
        int n = 11; // number of sample dataset
        for (int i=0; i<n; i++) {
            wxString sample_name = GdaConst::sample_names[i];
            wxString sample_meta_url = GdaConst::sample_meta_urls[i];
            wxString ds_layername = GdaConst::sample_layer_names[i];
            wxString ds_thumb = GdaConst::sample_layer_names[i];
            AddSampleItem(sizer, sample_scrl, sample_name, sample_meta_url,
                          ds_layername, ds_thumb, base_xrcid_sample_thumb+i);
        }
        sample_scrl->SetSizer( sizer );
        sample_scrl->Layout();
        sizer->Fit( sample_scrl );
    }
    
    sizer->Add( sample_scrl, 1, wxEXPAND | wxRIGHT, 5 );
    
    smaples_panel->SetSizer( sizer );
    smaples_panel->Layout();
    sizer->Fit( smaples_panel );
}

void ConnectDatasourceDlg::AddSampleItem(wxBoxSizer* sizer,
                                         wxScrolledWindow* scrl,
                                         wxString name,
                                         wxString ds_url,
                                         wxString ds_layername,
                                         wxString ds_thumb, int id)
{
    wxBoxSizer* text_sizer;
    text_sizer = new wxBoxSizer( wxVERTICAL );
    
    wxString lbl_ds_layername = ds_layername;
    lbl_ds_layername = GenUtils::PadTrim(lbl_ds_layername, 30, false);
    
    wxStaticText* layername;
    layername = new wxStaticText(scrl, wxID_ANY,  lbl_ds_layername.Trim());
    layername->SetFont(*GdaConst::medium_font);
    layername->SetForegroundColour(wxColour(100,100,100));
    layername->SetToolTip(ds_layername);
    text_sizer->Add(layername, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 5);
    
    wxString lbl_name = name;
    lbl_name = GenUtils::PadTrim(lbl_name, 60, false);
    
    wxStaticText* obs_txt;
    obs_txt = new wxStaticText(scrl, wxID_ANY, lbl_name);
    obs_txt->SetFont(*GdaConst::extra_small_font);
    obs_txt->SetForegroundColour(wxColour(70,70,70));
    obs_txt->SetToolTip(name);
    text_sizer->Add(obs_txt, 0, wxALIGN_LEFT | wxALL, 5);
    
    wxString lbl_ds_name = ds_url;
    lbl_ds_name = GenUtils::PadTrim(lbl_ds_name, 50, false);
    wxHyperlinkCtrl* filepath;
    filepath = new wxHyperlinkCtrl(scrl, wxID_ANY, ds_url, ds_url, wxDefaultPosition, wxDefaultSize, wxHL_ALIGN_LEFT);
    filepath->SetFont(*GdaConst::extra_small_font);
    filepath->SetForegroundColour(wxColour(70,70,70));
    filepath->SetToolTip(ds_url);
    text_sizer->Add(filepath, 1, wxALIGN_LEFT | wxALL, 5);
    
    wxString file_path_str;
    ds_thumb.Replace(" ", "");
    file_path_str = GenUtils::GetSamplesDir() + ds_thumb + ".png";
    if (!wxFileExists(file_path_str)) {
        file_path_str = GenUtils::GetSamplesDir() + "no_map.png";
    }
    wxImage img(file_path_str);
    if (!img.IsOk()) {
        file_path_str = GenUtils::GetSamplesDir() + "no_map.png";
        img.LoadFile(file_path_str);
    }
    img.Rescale(100,66,wxIMAGE_QUALITY_HIGH );
    wxBitmap bmp(img);
    
    wxBitmapButton* thumb;
    thumb = new wxBitmapButton(scrl, id, bmp);
    thumb->Bind(wxEVT_BUTTON, &ConnectDatasourceDlg::OnSample, this);
    
    wxBoxSizer* row_sizer;
    row_sizer = new wxBoxSizer( wxHORIZONTAL );
    row_sizer->Add(thumb, 0, wxALIGN_CENTER | wxALL, 0);
    row_sizer->Add(text_sizer, 1, wxALIGN_LEFT | wxALIGN_TOP | wxEXPAND | wxTOP, 5);
    
    sizer->Add(row_sizer, 0, wxALIGN_LEFT | wxALL, 2);
}

void ConnectDatasourceDlg::OnSample(wxCommandEvent& event)
{
	wxLogMessage("ConnectDatasourceDlg::OnSample()");
    int xrcid = event.GetId();
    int sample_idx = xrcid - base_xrcid_sample_thumb;
   
    // '{"ds_type":"ds_shapefile", "ds_path": "/test.shp", "db_name":"test"..}'
    wxString ds_json;
    wxString layername = GdaConst::sample_layer_names[sample_idx];
    wxString ds_name = GdaConst::sample_datasources[sample_idx];
    if (ds_name == "samples.sqlite") {
        ds_name = GenUtils::GetSamplesDir() + ds_name;
		ds_name.Replace("\\", "\\\\");
        //ds_json = wxString::Format("{\"ds_type\":\"SQLite\", \"ds_path\": \"%s\"}", ds_name);
        ds_json = "{\"ds_type\":\"SQLite\", \"ds_path\": \""+ds_name+"\"}";
    } else {
        //ds_json =  wxString::Format("{\"ds_type\":\"GeoJSON\", \"ds_path\": \"%s\"}", ds_name);
        ds_json = "{\"ds_type\":\"GeoJSON\", \"ds_path\": \""+ds_name+"\"}";
    }
    
    IDataSource* ds = IDataSource::CreateDataSource(ds_json);
    if (ds == NULL) {
        // raise message dialog show can't connect to datasource
        wxString msg = _("Can't connect to datasource: ") + ds_name;
        wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    } else {
        datasource = ds;
        layer_name = layername;
        EndDialog(wxID_OK);
    }

}

wxCSConv* ConnectDatasourceDlg::GetEncoding()
{
    if (m_encodings && m_encodings->IsShown()) {
		
        wxFontEncoding encoding_type = wxFONTENCODING_SYSTEM;
        int sel = m_encodings->GetSelection();
        wxString encode_str = m_encodings->GetString(sel);

        if (sel == 0) return NULL;

		//encode_str = wxString(encode_str.wc_str(), wxCSConv(wxFONTENCODING_DEFAULT));
		if (sel == 1) {
            encoding_type = wxFONTENCODING_UTF8;
        } else if (sel == 2) {
            encoding_type = wxFONTENCODING_UTF16LE;

		} else if (sel == 3) {
            encoding_type = wxFONTENCODING_CP1256;

        } else if (sel == 4 ) {
            encoding_type = wxFONTENCODING_ISO8859_2;

        } else if (sel == 5) {
            encoding_type = wxFONTENCODING_CP1250;

        } else if (sel == 6) {
            encoding_type = wxFONTENCODING_CP852;

        } else if (sel == 7) {
            encoding_type = wxFONTENCODING_GB2312;

        } else if (sel == 8) {
            encoding_type = wxFONTENCODING_BIG5;
     
        } else if (sel == 9) {
            encoding_type = wxFONTENCODING_ISO8859_5;

        } else if (sel == 10) {
            encoding_type = wxFONTENCODING_KOI8;

        } else if (sel == 11) {
            encoding_type = wxFONTENCODING_CP1251;

        } else if (sel == 12) {
            encoding_type = wxFONTENCODING_CP866;

        } else if (sel == 13 ) {
            encoding_type = wxFONTENCODING_ISO8859_7;

        } else if (sel == 14) {
            encoding_type = wxFONTENCODING_ISO8859_8;

        } else if (sel == 15) {
            encoding_type = wxFONTENCODING_CP1255;

        } else if (sel == 16) {
            encoding_type = wxFONTENCODING_SHIFT_JIS;
        } else if (sel == 17) {
            encoding_type = wxFONTENCODING_EUC_JP;
        } else if (sel == 18) {
            encoding_type = wxFONTENCODING_EUC_KR;
    
        } else if (sel == 19 ) {
            encoding_type = wxFONTENCODING_ISO8859_10;

        } else if (sel == 20) {
            encoding_type = wxFONTENCODING_ISO8859_3;

        } else if (sel == 21) {
            encoding_type = wxFONTENCODING_ISO8859_9;
        } else if (sel == 22) {
            encoding_type = wxFONTENCODING_CP1254;
        } else if (sel == 23) {
            encoding_type = wxFONTENCODING_CP1258;
        } else if (sel == 24) {
            encoding_type = wxFONTENCODING_ISO8859_1;

        } else if (sel == 25) {
            encoding_type = wxFONTENCODING_ISO8859_15;
		}

        if (m_wx_encoding) delete m_wx_encoding;
        m_wx_encoding = new wxCSConv(encoding_type);
    }
    return m_wx_encoding;
}
