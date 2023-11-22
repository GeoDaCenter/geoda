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
#include <wx/wx.h>

#include "../GdaConst.h"
#include "../DataViewer/TableInterface.h"
#include "../GdaShape.h"

#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "GdaCache.h"

using namespace Shapefile;

/**
 * A singleton data adapter for communicating with all OGR data sources.
 * This class automatically maintains the connections to all OGR data sources.
 * This class also provides operations to OGR data sources
 * \code
 * \endcode
 */
class OGRDataAdapter {
    
    // Thread realted variables
    boost::thread* layer_thread;
    boost::thread* cache_thread;
    boost::thread* export_thread;
    
    // Configuration Cache
    GdaCache* gda_cache;
    
    // Store opened data source in memory
    // In multi-layer scenario, this ogr-datasource pool will automatically
    // manage ogr datasources and layers.
    std::map<wxString, OGRDatasourceProxy*> ogr_ds_pool;
    
    OGRDatasourceProxy* export_ds;
	
private:
	/**
	 * constructor
	 */
	OGRDataAdapter();
		
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
    
public:
	// export progress indicator: -1 means error, otherwise means progress
	// Note: this should be moved to OGRlayerProxy
	int  export_progress;
	bool stop_exporting;
    std::ostringstream error_message;
	
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

	/**
	 * Get OGR datasource. If there is one in the ogr_ds_pool, return it directly.
	 * Otherwise, create a new OGR datasource, store it in ogr_ds_pool,
	 * then return it.
	 */
	OGRDatasourceProxy* GetDatasourceProxy(const wxString& ds_name, GdaConst::DataSourceType ds_type);

    /**
     * Used by multi-layers, so users can remove a layer from cache
     *
     */
    void RemoveDatasourceProxy(const wxString& ds_name);

    /**
     * functions for cache.sqlite used by GeoDa
     *
     */
	std::vector<wxString> GetHistory(const wxString& param_key);
	void AddHistory(const wxString& param_key, const wxString& param_val);
    void AddEntry(const wxString& param_key, const wxString& param_val);
	void CleanHistory();
	
	/**
	 * get OGR layer names from a datasource
     *
	 * @param ds_name OGR data source name
	 * @param layer_names a reference to a string vector that stores layer names
	 */
	GdaConst::DataSourceType GetLayerNames(const wxString& ds_name, GdaConst::DataSourceType& ds_type, std::vector<wxString>& layer_names);
	
		
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
	OGRLayerProxy* T_ReadLayer(const wxString& ds_name,
                               GdaConst::DataSourceType ds_type,
                               const wxString& layer_name);
	
	void T_StopReadLayer(OGRLayerProxy* layer_proxy);

    
    /**
     * A threaded version of exporting OGRLayer to a data source.
     *
     * Create a OGR datasource that contains input geometries and table.
     */
    OGRLayerProxy* ExportDataSource(const wxString& o_ds_format,
                                    const wxString& o_ds_name,
                                    const wxString& o_layer_name,
                                    OGRwkbGeometryType geom_type,
                                    std::vector<OGRGeometry*> ogr_geometries,
                                    TableInterface* table,
                                    std::vector<int>& selected_rows,
                                    OGRSpatialReference* spatial_ref,
                                    bool is_update,
                                    wxString cpg_encode = wxEmptyString);
                                 
    void StopExport();
    
	void CancelExport(OGRLayerProxy* layer);

    /**
     * Create OGR geometries from internal GdaShapes
     */
	OGRwkbGeometryType MakeOGRGeometries(std::vector<GdaShape*>& geometries, 
								  Shapefile::ShapeType shape_type,
								  std::vector<OGRGeometry*>& ogr_geometries,
								  std::vector<int>& selected_rows);
};
#endif
