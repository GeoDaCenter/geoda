
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <wx/stdpaths.h>

#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "GdaCache.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"

const wxString GdaCache::HIST_TABLE_NAME = "history";
const wxString GdaCache::DB_HOST_HIST	= "db_host";
const wxString GdaCache::DB_PORT_HIST	= "db_port";
const wxString GdaCache::DB_NAME_HIST	= "db_name";
const wxString GdaCache::DB_UNAME_HIST	= "db_uname";
const wxString GdaCache::WS_URL_HIST    = "ws_url";

wxString GdaCache::GetFullPath()
{
    wxString cache_path = GenUtils::GetCachePath();
    return cache_path;
}

GdaCache::GdaCache()
{
    wxString cache_path = GdaCache::GetFullPath();
    
	// if cache file not exists, create one
    if (!wxFileExists(cache_path)) {
        OGRDatasourceProxy::CreateDataSource("SQLite", cache_path);
    }
    
	// connect to cache file
    try {
        cach_ds_proxy = new OGRDatasourceProxy(cache_path, GdaConst::ds_sqlite, true);
        layer_names = cach_ds_proxy->GetLayerNames();
        wxString sql = "SELECT * FROM history";
        history_table = cach_ds_proxy->GetLayerProxyBySQL(sql);
        
        if (history_table == NULL) {
            sql = "CREATE TABLE history (param_key TEXT, param_val TEXT)";
            cach_ds_proxy->ExecuteSQL(sql);
            sql = "SELECT * FROM history";
            history_table = cach_ds_proxy->GetLayerProxyBySQL(sql);
        }
        
        history_table->ReadData();
        
        for ( int i=0; i< history_table->n_rows; i++) {
            // all strings are encoded and stored using UTF8, so use
            // wxString::FromUTF8 to read from char*
            wxString key = history_table->GetValueAt(i, 0);
            wxString val = history_table->GetValueAt(i, 1);
            history_keys.push_back(key);
            history_vals.push_back(val);
        }
    } catch(GdaException& e) {
        throw GdaException("Construct GdaCache Failed.");
    }
}

GdaCache::~GdaCache()
{
	history_table->Save();

	delete history_table;
    history_table = NULL;
	delete cach_ds_proxy;
	cach_ds_proxy = NULL;
}						

std::vector<wxString> GdaCache::GetHistory(wxString param_key)
{
	std::vector<wxString> hist_rst;
	int n = history_keys.size();
	for ( int i=0; i< n; i++){
		if ( param_key == history_keys[i] ){
			hist_rst.push_back( history_vals[i] );
		}
	}
	return hist_rst;
}

void GdaCache::AddHistory(wxString param_key, wxString param_val)
{
	for ( size_t i=0; i< history_keys.size(); i++){
		if ( param_key == history_keys[i] ){
			if ( param_val == history_vals[i] )
                return; // already existed
		}
	}
	// add to current memory
	history_keys.push_back( param_key );
	history_vals.push_back( param_val );
	// add to spatialite table
	wxString _sql = "INSERT INTO history VALUES('"
						+ param_key +"','"+param_val + "')";
    const char * sql = (const char*) _sql.mb_str(wxConvUTF8);
	cach_ds_proxy->ExecuteSQL(sql);
}

void GdaCache::AddEntry(wxString param_key, wxString param_val)
{
    for ( size_t i=0; i< history_keys.size(); i++){
        if ( param_key == history_keys[i] ){
            // update existing Entry
            history_vals[i] = param_val;
            wxString _sql = "UPDATE history SET param_val='" + param_val +"' WHERE param_key='" + param_key + "'";
            const char * sql = (const char*) _sql.mb_str(wxConvUTF8);
            cach_ds_proxy->ExecuteSQL(sql);
            return;
        }
    }
    // add new entry to current memory
    history_keys.push_back( param_key );
    history_vals.push_back( param_val );
    // add to spatialite table
    wxString _sql = "INSERT INTO history VALUES('" + param_key +"','"+param_val + "')";
    const char * sql = (const char*) _sql.mb_str(wxConvUTF8);
	OGRLayer* tmp_layer = cach_ds_proxy->ds->ExecuteSQL(sql,  0, "SQLITE");
	cach_ds_proxy->ds->ReleaseResultSet(tmp_layer);
}

void GdaCache::CleanHistory()
{
	wxString sql = "DELETE FROM history";
	cach_ds_proxy->ExecuteSQL(sql);
}

OGRLayerProxy* GdaCache::GetLayerProxy(wxString ext_ds_name,
									   wxString ext_layer_name)
{
	wxString query_layer_name = ext_ds_name + "_" + ext_layer_name;
	std::transform(query_layer_name.begin(), query_layer_name.end(), 
				   query_layer_name.begin(), ::tolower);
	
	return cach_ds_proxy->GetLayerProxy(query_layer_name);
}
