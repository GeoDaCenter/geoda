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

#ifndef __GEODA_CENTER_OGR_DATA_ADAPTER_H__
#define __GEODA_CENTER_OGR_DATA_ADAPTER_H__

#include <string>
#include <vector>
#include <map>
#include <boost/multi_array.hpp>
#include <boost/thread.hpp>

#include "../ShpFile.h"
#include "../GdaConst.h"
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/OGRTable.h"
#include "../GdaShape.h"

#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "GdaCache.h"

using namespace Shapefile;
using namespace std;

/**
 * A singleton data adapter for communicating with all OGR data sources.
 * This class automatically maintains the connections to all OGR data sources.
 * This class also provides operations to OGR data sources, which include:
 *  GetHistory(), AddHistory(), GetDataSourceType(), GetLayerNames(),
 *  ReadLayer(),StopReadLayer(),SaveLayer(),T_Export(),T_StopExport()
 * \code
 * \endcode
 */
class OGRDataAdapter {
public:
	static OGRDataAdapter& GetInstance() {
		static OGRDataAdapter instance;
		return instance;
	}
    
	/**
	 * Call to destroy OGRDataAdapter instance.
	 * Free all allocated memory, disconnect OGR data sources,
	 * and execute a safe exit.
	 */
	void Close();
	
private:
	/**
	 * constructor
	 */
	OGRDataAdapter();
	
    /**
     */
	OGRDataAdapter(bool enable_cache);
		
	/**
	 * dummy constructor. Not implemented.
	 * To prevent construction by copying
	 */
	OGRDataAdapter(OGRDataAdapter const&);
	
	/**
	 * dummy operator =. Not implemented.
	 * To prevent assignment
	 */
	void operator = (OGRDataAdapter const&);		
	
private:
	// Thread realted variables
	//todo: we should have a thread pool that manage their lifecycle
	boost::thread* layer_thread;
	boost::thread* cache_thread;
    boost::thread* export_thread;
	
	// Cache realted variables
	bool enable_cache;
	GdaCache* gda_cache;
	
	// Store opened data source in memory
	// In multi-layer scenario, this ogr-datasource pool will automatically
	// manage ogr datasources and layers.
	map<wxString, OGRDatasourceProxy*> ogr_ds_pool;

	OGRDatasourceProxy* export_ds;
    
public:
	// export progress indicator: -1 means error, otherwise means progress
	// Note: this should be moved to OGRlayerProxy
	int  export_progress;
	bool stop_exporting;
	ostringstream error_message;
	
public:
	/**
	 * Get OGR datasource.
	 * If there is one in the ogr_ds_pool, return it directly.
	 * Otherwise, create a new OGR datasource, store it in ogr_ds_pool,
	 * then return it.
	 */
	OGRDatasourceProxy* GetDatasourceProxy(wxString ds_name, GdaConst::DataSourceType ds_type);
	
	vector<string> GetHistory(string param_key);

	void AddHistory(string param_key, string param_val);
    void AddEntry(string param_key, string param_val);
	void CleanHistory();
	
	/**
	 * get OGR layer names from a datasource
	 * @param ds_name OGR data source name
	 * @param layer_names a reference to a string vector that stores layer names
	 */
	vector<wxString> GetLayerNames(wxString ds_name, GdaConst::DataSourceType ds_type);

	/**
	 * cacher existing layer (memory) to local spatialite
	 */
	void CacheLayer(string ds_name, string layer_name,
                    OGRLayerProxy* layer_proxy);
	
		
	/**
	 * A threaded version of reading OGRLayer from data source.
	 *
	 * If the layer has been cached locally and has not been updated, read it
	 * directly from cache. Otherwise, read layer from external datasource and
	 * cache it when enable_cache flag setted.
	 *
	 * @param ds_name OGR data source name
	 * @param layer_name OGR table name
	 */
	OGRLayerProxy* T_ReadLayer(wxString ds_name, GdaConst::DataSourceType ds_type, string layer_name);
	
	void T_StopReadLayer(OGRLayerProxy* layer_proxy);
	
    /**
     * 
     */
	void SaveLayer(OGRLayerProxy* layer_proxy);
    
    /**
     * Create a OGR datasource that contains input geometries and table.
     */
    OGRLayerProxy* ExportDataSource(string o_ds_format, 
                                    wxString o_ds_name,
                                    wxString o_layer_name,
                                    OGRwkbGeometryType geom_type,
                                    vector<OGRGeometry*> ogr_geometries,
                                    TableInterface* table,
                                    vector<int>& selected_rows,
                                    OGRSpatialReference* spatial_ref,
                                    bool is_update);
                                 
    void StopExport();
	void CancelExport(OGRLayerProxy* layer);
    
    void Export(OGRLayerProxy* source_layer_proxy,
                std::string format,
                std::string dest_datasource,
                std::string new_layer_name,
                bool is_update);

	OGRwkbGeometryType MakeOGRGeometries(vector<GdaShape*>& geometries, 
								  Shapefile::ShapeType shape_type,
								  vector<OGRGeometry*>& ogr_geometries,
								  vector<int>& selected_rows);
};
#endif
