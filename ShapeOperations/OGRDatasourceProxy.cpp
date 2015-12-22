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

#include <string>
#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <cpl_port.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "../GdaException.h"

using namespace std;

//------------------------------------------------------------------------------
OGRDatasourceProxy::OGRDatasourceProxy(GDALDataset* _ds, string _ds_name)
: ds(_ds), ds_name(_ds_name)
{
}

OGRDatasourceProxy::OGRDatasourceProxy(string _ds_name, bool bUpdate)
: ds_name(_ds_name)
{	
	//ds = OGRSFDriverRegistrar::Open( ds_name.c_str(), bUpdate);
    const char* pszDsPath = ds_name.c_str();
	ds = (GDALDataset*) GDALOpenEx(pszDsPath, GDAL_OF_VECTOR|GDAL_OF_UPDATE, NULL, NULL, NULL);
    is_writable = true;
	if (!ds) {
        // try without UPDATE
        ds = (GDALDataset*) GDALOpenEx(pszDsPath, GDAL_OF_VECTOR, NULL, NULL, NULL);
        if (!ds) {
            // raise open fialed
            string error_detail = CPLGetLastErrorMsg();
            ostringstream msg;
            if ( error_detail.length() == 0 || error_detail == "Unknown"){
                msg << "Failed to open data source. Please check if the data is valid and its data type/format is supported by GeoDa.\n\nTip: you can setup necessary GeoDa driver by following the instructions at:\n https://geodacenter.asu.edu/geoda/formats";
            } else {
                msg << error_detail;
            }

            throw GdaException(msg.str().c_str());
        }
        is_writable = false;
	}
	ds_type = GetGdaDataSourceType();
	// deprecated by above logic
    //is_writable = ds->TestCapability( ODsCCreateLayer );
	layer_count = ds->GetLayerCount();
}

OGRDatasourceProxy::OGRDatasourceProxy(string format, string dest_datasource)
: ds_name(dest_datasource)
{
	// create a OGRDatasourceProxy with a geometry layer
	ostringstream error_message;
	const char* pszFormat = format.c_str();
	const char* pszDestDataSource = ds_name.c_str();
	//OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
	//OGRSFDriver *poDriver = poR->GetDriverByName(pszFormat);
	GDALDriver *poDriver;
	poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
	
	if( poDriver == NULL ){
		error_message << "Current data source format " << format << " is not ";
		error_message << "supprted by GeoDa.\n" << CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
	}
	
	// get datasource type
	const char* drv_name = GDALGetDriverLongName(poDriver);
	string ogr_ds_type(drv_name);
	if (GdaConst::datasrc_str_to_type.find(ogr_ds_type) ==
		GdaConst::datasrc_str_to_type.end()) {
		ds_type = GdaConst::ds_unknown;
	} else {
		ds_type = GdaConst::datasrc_str_to_type[ogr_ds_type];
	}
	
	// create the output data source.
	char *papszLCO[50] = {"OVERWRITE=yes"};
	//ds = poDriver->CreateDataSource( pszDestDataSource, papszLCO);
	ds = poDriver->Create( pszDestDataSource, 0,0,0,GDT_Unknown, NULL);
	if(ds == NULL ) {
		// driver failed to load
		error_message << "Internal Error: GeoDa can't create output driver. Please contact GeoDa admin."
		<<"\n\nDetails: "<< CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
	}
    if(!ds->TestCapability(ODsCCreateLayer)) {
		// driver failed to load
		error_message << "GeoDa can't write layer to this datasource. Please contact GeoDa admin."
		<<"\n\nDetails: "<< CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
    }
	layer_count = ds->GetLayerCount();
}

//------------------------------------------------------------------------------
OGRDatasourceProxy::~OGRDatasourceProxy()
{
	// clean map of layer_pool
	map<string, OGRLayerProxy*>::iterator it;
	for (it=layer_pool.begin(); it!=layer_pool.end(); it++) {
		if (it->second)
            delete it->second;
	}
	layer_pool.clear();
	// clean ogr data sources
	//OGRDataSource::DestroyDataSource(ds);
	GDALClose(ds);
}

//------------------------------------------------------------------------------
GdaConst::DataSourceType
OGRDatasourceProxy::GetGdaDataSourceType()
{
	string ogr_ds_type(GDALGetDriverLongName(ds));
   
    if (ogr_ds_type.empty()) ogr_ds_type = GDALGetDriverShortName(ds);
    
    if (ogr_ds_type.find("CartoDB") != std::string::npos) {
       return GdaConst::datasrc_str_to_type["CartoDB"];
        
    } else if (GdaConst::datasrc_str_to_type.find(ogr_ds_type) ==
		GdaConst::datasrc_str_to_type.end()) {
		return GdaConst::ds_unknown;
	} else {
		return GdaConst::datasrc_str_to_type[ogr_ds_type];
	}
}

//------------------------------------------------------------------------------
vector<string> OGRDatasourceProxy::GetLayerNames()
{
	// GetLayerNames can happen before actually read data from layer
	// , so this provide us a chance to store all OGRLayer instance
	// in this datasource proxy for future use.
	if (layer_count == layer_pool.size() && layer_count > 0) {
		// return layer names from pool directly
		//map<string, OGRLayerProxy*>::iterator it;
		//for (it = layer_pool.begin(); it!= layer_pool.end(); it++) {
		//	layer_names.push_back(it->first);
		//}
    } else if (layer_count == 0){
       // try to read (by default) first layer
       // Note: http://ogi.state.ok.us/geoserver/wfs?VERSION=1.1.0&REQUEST=GetFeature&typename=okcounties
       // this WFS return 0 layer count, which is unusual
		OGRLayerProxy* layer_proxy = NULL;
        OGRLayer* layer = ds->GetLayer(0);
        if (layer) {
            string layer_name(layer->GetName());
            this->layer_names.push_back(layer_name);
            layer_pool[layer_name] = new OGRLayerProxy(layer_name,layer,ds_type);
        }
	} else {
		// read and store layer one by one, get the layer name
		int system_layers = 0;
		OGRLayer* layer = NULL;
		for (int i=0; i<layer_count; i++)
		{
			layer = ds->GetLayer(i);
			string layer_name(layer->GetName());
            // don't show some system tables in postgres
            if ( layer_name == "raster_columns" ||
                 layer_name == "raster_overviews" ||
                 layer_name == "topology.topology"  ||
                 layer_name == "topology.layer" )
            {
                system_layers++;
                continue;
            }
			this->layer_names.push_back(layer_name);
			layer_pool[layer_name] = new OGRLayerProxy(layer_name,layer,ds_type);
		}
        layer_count = layer_count - system_layers;
	}
	return this->layer_names;
}

//------------------------------------------------------------------------------
OGRLayerProxy* OGRDatasourceProxy::ExecuteSQL(string sql)
{
	ds->ExecuteSQL(sql.c_str(), 0, 0);
	return NULL;
}

//------------------------------------------------------------------------------

OGRLayerProxy* OGRDatasourceProxy::GetLayerProxyBySQL(string sql)
{
    // Note: layer is not managed here. Memory leak is possible.
	OGRLayer* layer = ds->ExecuteSQL(sql.c_str(), 0, 0);
	if (layer == NULL) return NULL;
	OGRLayerProxy* layer_proxy = new OGRLayerProxy(sql, layer, ds_type);
	return layer_proxy;
}

bool OGRDatasourceProxy::DeleteLayer(string layer_name)
{
    int tmp_layer_count = layer_count;
    for (int i=0; i < tmp_layer_count; i++)
	{
        OGRLayer* layer = ds->GetLayer(i);
		string tmp_layer_name(layer->GetName());
        if ( tmp_layer_name.compare(layer_name) == 0) {
            if ( ds->DeleteLayer(i) == OGRERR_NONE ) {
                map<string, OGRLayerProxy*>::iterator it =
                    layer_pool.find(layer_name);
                if ( it != layer_pool.end()) {
                    layer_pool.erase(it);
                }
                for (int j=0; j<layer_names.size(); j++) {
                    if (layer_name.compare(layer_names[i]) == 0) {
                        layer_names.erase(layer_names.begin() + j);
                        break;
                    }
                }
                layer_count--;
                return true;
            }
            else return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
OGRLayerProxy* OGRDatasourceProxy::GetLayerProxy(string layer_name)
{
	OGRLayerProxy* layer_proxy;
	
	if (layer_pool.count(layer_name) > 0) {
		// find it from pool and return it
		layer_proxy = layer_pool[layer_name];
	} else {
		// otherwise, create one and store it in pool
		OGRLayer* layer = ds->GetLayerByName(layer_name.c_str());
		if (!layer) {
			// for some files, there's no layer name. Just get the first one
			layer = ds->GetLayer(0);
		}
		if (layer == NULL) {
            ostringstream error_message;
            error_message << "No layer was found in this datasource.";
			throw GdaException(error_message.str().c_str());
		}
		//bool is_thread_safe = layer->TestCapability(OLCRandomRead);
		layer_proxy = new OGRLayerProxy(layer_name, layer, ds_type);
		//todo: if there is one already existed, clean/delete the old first
		layer_pool[layer_name] = layer_proxy;
	}
	
	return layer_proxy;
}

//------------------------------------------------------------------------------
// static function
void OGRDatasourceProxy::CreateDataSource(string format,
										  string dest_datasource)
{
	ostringstream error_message;
	const char* pszFormat = format.c_str();
	const char* pszDestDataSource = dest_datasource.c_str();
	//OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
	//OGRSFDriver *poDriver = poR->GetDriverByName(pszFormat);
	GDALDriver *poDriver;
	poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
	
	if( poDriver == NULL ){
		error_message << "Current OGR dirver " + format + " is not supprted "
			<< "by GeoDa.\n" << CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
	}
	//if( !poDriver->TestCapability( ODrCCreateDataSource ) ){
  //      error_message << "Current OGR driver does not support data source"
  //      " creation.";
	//	throw GdaException(error_message.str().c_str());
	//}
	
	// Create the output data source.  
	//OGRDataSource *poODS = poDriver->CreateDataSource( pszDestDataSource, NULL);
	GDALDataset *poODS = poDriver->Create( pszDestDataSource, 0,0,0,GDT_Unknown, NULL);
	if( poODS == NULL ) {
		// driver failed to load
		error_message << "Can't create output OGR driver."
		<<"\n\nDetails:"<< CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
	}
	//OGRDataSource::DestroyDataSource( poODS );
	GDALClose(poODS);
}


// Create a geom only layer, the table will added ID column automatically
OGRLayerProxy*
OGRDatasourceProxy::CreateLayer(string layer_name,
                                OGRwkbGeometryType eGType,
                                vector<OGRGeometry*>& geometries,
                                TableInterface* table,
                                map<wxString, pair<int, int> >& field_dict,
                                vector<int>& selected_rows,
                                OGRSpatialReference* spatial_ref)
{
    ostringstream  error_message;
    if(!ds->TestCapability(ODsCCreateLayer)) {
		// driver failed to load
		error_message << "GeoDa can't create a layer."
		<<"\n\nDetails: "<< CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
    }
    
    OGRSpatialReference *poOutputSRS = spatial_ref;
    
    // PRECISION is for database e.g. MSSQL
    // LAUNDER is for database: rename desired field name
    char* papszLCO[50] = {"OVERWRITE=yes", "PRECISION=no", "LAUNDER=no"};
    
    OGRLayer *poDstLayer = ds->CreateLayer(layer_name.c_str(), poOutputSRS, eGType, papszLCO);
    
    if( poDstLayer == NULL ) {
        error_message << "Can't write/create layer \"" << layer_name << "\"."
                      <<"\n\nDetails: Attemp to write a readonly database, or "
                      << CPLGetLastErrorMsg();
		throw GdaException(error_message.str().c_str());
    }
    
    map<wxString, pair<int, int> >::iterator field_it;
    
    // create fields using TableInterface:table
    if ( table != NULL ) {
        
        std::vector<int> col_id_map;
        table->FillColIdMap(col_id_map);
        
        int time_steps = table->GetTimeSteps();
        for ( int id=0; id < table->GetNumberCols(); id++ ) {
            for ( int t=0; t < time_steps; t++ ) {
                wxString fname = table->GetColName(col_id_map[id], t);
                if (fname.empty()) {
                    error_message << "Can't create layer \"" << layer_name
                    << "\" with empty field name.";
                    throw GdaException(error_message.str().c_str());
                }
                field_it = field_dict.find(fname);
                if (field_it == field_dict.end()) {
                    // a unique field
                    OGRFieldType ogr_type;
                    int ogr_fwidth            = table->GetColLength(id, t);
                    int ogr_fprecision        = table->GetColDecimals(id, t);
                    GdaConst::FieldType ftype = table->GetColType(id, t);
                    if (ftype == GdaConst::string_type){
                        ogr_type = OFTString;
                    } else if (ftype == GdaConst::long64_type){
                        ogr_type = OFTInteger64;
                    } else if (ftype == GdaConst::double_type){
                        ogr_type = OFTReal;
                    } else if (ftype == GdaConst::date_type){
                        ogr_type = OFTDate;
                    } else {
                        ogr_type = OFTString;
                    }
                    OGRFieldDefn oField(fname, ogr_type);
                    oField.SetWidth(ogr_fwidth);
                    if ( ogr_fprecision>0 ) {
                        oField.SetPrecision(ogr_fprecision);
                    }
                    if( poDstLayer->CreateField( &oField ) != OGRERR_NONE ) {
                        error_message << "Creating Name field failed.\n";
                        throw GdaException(error_message.str().c_str());
                    }
                    // record in field_dict
                    field_dict[fname] = make_pair(id, t);
                }
            }
        }
    }
    
    OGRLayerProxy* layer =  new OGRLayerProxy(poDstLayer, ds_type, eGType);
    //layer->AddFeatures(geometries, table, field_dict, selected_rows);
    
    layer_pool[layer_name] = layer;
    return layer;
}
