
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

const std::string GdaCache::HIST_TABLE_NAME = "history";
const std::string GdaCache::DB_HOST_HIST	= "db_host";
const std::string GdaCache::DB_PORT_HIST	= "db_port";
const std::string GdaCache::DB_NAME_HIST	= "db_name";
const std::string GdaCache::DB_UNAME_HIST	= "db_uname";
const std::string GdaCache::WS_URL_HIST		= "ws_url";

wxString GdaCache::GetFullPath()
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFile(exePath);
    wxString exeDir = exeFile.GetPathWithSep();
    exeDir = exeDir + "cache.sqlite";
    return exeDir;
}

GdaCache::GdaCache()
{
    wxString exePath = GdaCache::GetFullPath();
    
	// if cache file not exists, create one
    if (!wxFileExists(exePath)) {
        OGRDatasourceProxy::CreateDataSource("SQLite", exePath);
    }
    
	// connect to cache file
    try {
        
        cach_ds_proxy = new OGRDatasourceProxy(exePath, GdaConst::ds_sqlite, true);
        layer_names = cach_ds_proxy->GetLayerNames();
        std::string sql = "SELECT * FROM history";
        history_table = cach_ds_proxy->GetLayerProxyBySQL(sql);
        
        if (history_table == NULL) {
            sql = "CREATE TABLE history (param_key TEXT, param_val TEXT)";
            cach_ds_proxy->ExecuteSQL(sql);
            sql = "SELECT * FROM history";
            history_table = cach_ds_proxy->GetLayerProxyBySQL(sql);
        }
        
        history_table->ReadData();
        
        for ( int i=0; i< history_table->n_rows; i++){
            history_keys.push_back( history_table->GetValueAt(i, 0).ToStdString() );
            history_vals.push_back( history_table->GetValueAt(i, 1).ToStdString() );
        }
    }catch(GdaException& e) {
        //XXX
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

std::vector<std::string> GdaCache::GetHistory(std::string param_key)
{
	std::vector<std::string> hist_rst;
	int n = history_keys.size();
	for ( int i=0; i< n; i++){
		if ( param_key == history_keys[i] ){
			hist_rst.push_back( history_vals[i] );
		}
	}
	return hist_rst;
}

void GdaCache::AddHistory(std::string param_key, std::string param_val)
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
	std::string sql = "INSERT INTO history VALUES('"
						+ param_key +"','"+param_val + "')";
	cach_ds_proxy->ExecuteSQL(sql);
}

void GdaCache::AddEntry(std::string param_key, std::string param_val)
{
    for ( size_t i=0; i< history_keys.size(); i++){
        if ( param_key == history_keys[i] ){
            // update existing Entry
            history_vals[i] = param_val;
            std::string sql = "UPDATE history SET param_val='" + param_val +"' WHERE param_key='" + param_key + "'";
            cach_ds_proxy->ExecuteSQL(sql);
            return;
        }
    }
    // add new entry to current memory
    history_keys.push_back( param_key );
    history_vals.push_back( param_val );
    // add to spatialite table
    std::string sql = "INSERT INTO history VALUES('" + param_key +"','"+param_val + "')";
    //cach_ds_proxy->ExecuteSQL(sql);
	OGRLayer* tmp_layer = cach_ds_proxy->ds->ExecuteSQL(sql.c_str(),  0, "SQLITE");
	cach_ds_proxy->ds->ReleaseResultSet(tmp_layer);
}

void GdaCache::CleanHistory()
{
	std::string sql = "DELETE FROM history";
	cach_ds_proxy->ExecuteSQL(sql);
}

bool GdaCache::UpdateLayer(std::string ext_ds_name, 
						   OGRLayerProxy* ext_layer_proxy)
{
	return false;
}

bool GdaCache::IsLayerUpdated(std::string ext_ds_name, 
							  OGRLayerProxy* ext_layer_proxy)
{
	return false;
}

bool GdaCache::IsLayerCached(std::string ext_ds_name, 
							 std::string ext_layer_name)
{
    bool caseSensitive = false;
	for (size_t i=0; i<layer_names.size(); i++) {
		wxString query_layer_name = ext_ds_name + "_" + ext_layer_name;
		if (layer_names[i].IsSameAs(query_layer_name, caseSensitive))
            return true;
	}
	return false;
}

OGRLayerProxy* GdaCache::GetLayerProxy(std::string ext_ds_name, 
									   std::string ext_layer_name)
{
	std::string query_layer_name = ext_ds_name + "_" + ext_layer_name;
	std::transform(query_layer_name.begin(), query_layer_name.end(), 
				   query_layer_name.begin(), ::tolower);
	
	return cach_ds_proxy->GetLayerProxy(query_layer_name);
}

// This function does NOT work for now
bool GdaCache::CacheLayer(std::string ext_ds_name, 
						  OGRLayerProxy* ext_layer_proxy)
{
	OGRLayer* poSrcLayer = ext_layer_proxy->layer;
	
	// get information from current layer: geomtype, layer_name
    // (NOTE: we don't consider coodinator system and translation here)
    OGRFeatureDefn *poSrcFDefn = poSrcLayer->GetLayerDefn();
    int eGType = poSrcFDefn->GetGeomType();
    const char* pszNewLayerName = poSrcLayer->GetName();
    int bForceToPolygon = FALSE;
    int bForceToMultiPolygon = FALSE;
    int bForceToMultiLineString = FALSE;
	
    if( wkbFlatten(eGType) == wkbPolygon )
        bForceToPolygon = TRUE;
    else if( wkbFlatten(eGType) == wkbMultiPolygon )
        bForceToMultiPolygon = TRUE;
    else if( wkbFlatten(eGType) == wkbMultiLineString )
        bForceToMultiLineString = TRUE;
	
	//Setup coordinate transformation if we need it.
	OGRCoordinateTransformation *poCT = NULL;
	bool bTransform                   = FALSE;
	OGRSpatialReference *poSourceSRS = NULL;
	// todo
	OGRSpatialReference *poOutputSRS = new OGRSpatialReference("EPSG:4326");

	// Cache
	char *papszLCO[] = {"OVERWRITE=yes","FORMAT=Spatialite"};
	std::string cache_layer_name = ext_ds_name + "_"+ext_layer_proxy->name;
	GDALDataset *poDstDS = cach_ds_proxy->ds;
	OGRLayer *poDstLayer = poDstDS->CreateLayer(cache_layer_name.c_str(), 
												poOutputSRS, 
												(OGRwkbGeometryType)eGType, 
												papszLCO);
	if (poDstLayer == NULL) {
		// raise create cache failed.
		return false;
	}
    // Process Layer style table
    poDstLayer->SetStyleTable( poSrcLayer->GetStyleTable () );
	
    // Add fields. here to copy all field.
    int nSrcFieldCount = poSrcFDefn->GetFieldCount();
    int iField;
    OGRFeatureDefn *poDstFDefn = poDstLayer->GetLayerDefn();
	
    for( iField = 0; iField < nSrcFieldCount; iField++ )
    {    
        OGRFieldDefn* poSrcFieldDefn = poSrcFDefn->GetFieldDefn(iField);
        OGRFieldDefn oFieldDefn( poSrcFieldDefn );
		
        // The field may have been already created at layer creation 
		if (poDstLayer->CreateField( &oFieldDefn ) == OGRERR_NONE)
        {    
            // now that we've created a field, GetLayerDefn() won't return NULL
            if (poDstFDefn == NULL)
                poDstFDefn = poDstLayer->GetLayerDefn();
        }    
    }    
	
    // Transfer feature from Source Layer to Dest Layer
    OGRFeature  *poFeature;
    GIntBig      nFeaturesWritten = 0;
    poSrcLayer->ResetReading();
	
    while (poFeature = poSrcLayer->GetNextFeature())
    {
        OGRFeature *poDstFeature = OGRFeature::CreateFeature(
										poDstLayer->GetLayerDefn() );
        poDstFeature->SetFrom(poFeature);
		
        OGRGeometry *poDstGeometry = poDstFeature->GetGeometryRef();
        if (poDstGeometry != NULL)
        {
            if( bForceToPolygon )
            {
                poDstFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToPolygon(
						poDstFeature->StealGeometry()));
            }
            else if( bForceToMultiPolygon )
            {
                poDstFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToMultiPolygon(
					    poDstFeature->StealGeometry() ) );
            }
            else if ( bForceToMultiLineString )
            {
                poDstFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToMultiLineString(
						poDstFeature->StealGeometry() ) );
            }
        }
        if( poDstLayer->CreateFeature( poDstFeature ) == OGRERR_NONE )
        {
            nFeaturesWritten ++;
        }            
        OGRFeature::DestroyFeature( poDstFeature );
        OGRFeature::DestroyFeature( poFeature );
    }
    //OGRDataSource::DestroyDataSource(poDstDS);
	GDALClose(poDstDS);
    // XXX
    // delete poDstLayer;
	return true;
}
