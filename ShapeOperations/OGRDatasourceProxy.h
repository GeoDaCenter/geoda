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

#ifndef __GEODA_CENTER_OGR_DATASOURCE_PROXY_H__
#define __GEODA_CENTER_OGR_DATASOURCE_PROXY_H__

#include <string>
#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include <boost/thread.hpp>

#include "OGRLayerProxy.h"
#include "../GdaShape.h"
#include "../DataViewer/TableInterface.h"

using namespace std;

/**
 * A proxy class for OGR data source. It will parallel run threaded 
 * OGRLayerProxy instances to fetch data from different data resources 
 * simutaneously. 
 */
class OGRDatasourceProxy {
public:
	OGRDatasourceProxy(){};
	/**
	 * Construct function. Create a data source proxy according to the 
	 * formated ds_name (datasource name).
	 */
	OGRDatasourceProxy(wxString _ds_name, GdaConst::DataSourceType _ds_type, bool bUpdate=true);
	/**
	 */
	OGRDatasourceProxy(GDALDataset* _ds, wxString _ds_name);
	/**
	 * Create a new OGRDatasourceProxy by given OGR format and ds name
	 */
	OGRDatasourceProxy(wxString format, wxString dest_datasources);
	/**
	 * Deconstructor. Will clean the layer proxies (OGRLayerProxy) in the layer
	 * pool (map)
	 */
	~OGRDatasourceProxy();

private:
    boost::thread* create_layer_thread;
	
public:
    bool is_writable;
   
    GdaConst::DataSourceType ds_type;
    
    wxString ds_name; //!< formated name of data source
    
    GDALDataset* ds; //!< maintain the datasource after connecting to it
    
    size_t layer_count;	//!< number of layer in this data source
    
	map<wxString, OGRLayerProxy*> layer_pool; //!< dict for all opened layers
    
	vector<wxString> layer_names;
    
public:
	/**
	 * This function clean the memory (geometies and table) of contained layers
	 */
	void Clean();
	
	/**
	 * This function create a new datasource (file or database).
	 * Note: this function now is working in GdaCache only, so the created
	 * datasource will be added to layer_pool.
	 */
	static void CreateDataSource(string format, wxString dest_datasource);
	
	/**
	 * Return OGR data source type as string
	 */
    GdaConst::DataSourceType GetGdaDataSourceType(GDALDriver *poDriver);
    
	/**
	 * Get all layer names in this data source
	 * Return the number of layers, in case there is no layer in datasource.
	 * (e.g. an empty spatialite file db).
	 */
	vector<wxString> GetLayerNames();
	
	/**
	 * Return layer proxy according to the layer_name
	 * Note: 
	 * this function also needs to prepare memory for reading an ogr_layer. 
	 * All other layers, if real OGR Features were read into memory, have to be
	 * cleaned. This makes sure that this program wont crash when large datasets
	 * (layers) were read.
	 */
	OGRLayerProxy* GetLayerProxy(string layer_name);
	
	OGRLayerProxy* GetLayerProxyBySQL(string sql);
	
	OGRLayerProxy* ExecuteSQL(string sql);

    OGRLayerProxy* CreateLayer(wxString layer_name,
                               OGRwkbGeometryType eGType,
                               vector<OGRGeometry*>& geometries,
                               TableInterface* table,
                               vector<int>& selected_rows,
                               OGRSpatialReference* spatial_ref);
    
    void StopCreateLayer();
    bool DeleteLayer(string layer_name);
};


#endif
