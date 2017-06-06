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

#ifndef __GEODA_CENTER_GDA_CACHE_H__
#define __GEODA_CENTER_GDA_CACHE_H__

#include <string>
#include <vector>
#include <map>

#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"

/**
 * GdaCache is a spatialite based cache sytem that
 * stores remote fetching data locally for better I/O performance.
 *
 * \code
 * // GeoDa Cache datasource name
 * string ds_name = GdaConst::cache_datasource_name;
 * // Layer name stored in GeoDa cache is composed by
 * // external datasource name plus layer name
 * // Ex: OCI:oracle/oracle@192.168.56.101:1521/xe_TEST5
 * string layer_name = ext_ds_name + "_" + ext_layer_name;
 * OGRDataAdapter::GetInstance()->ReadLayer(ds_name, layer_name);
 * \endcode
 */
class GdaCache  {
	
public:
	/**
	 * Constructor of GdaCache. 
	 *
	 * Connect to spatialite local cache database. Get related meta information.
	 */
	GdaCache();
	~GdaCache();
	
private:
    std::string cache_filename;
	OGRDatasourceProxy* cach_ds_proxy;
	std::vector<wxString> layer_names; //<! layer name in cache is composed
										  //<! by orginal ds_name and layer_name
	static const std::string HIST_TABLE_NAME;
	static const std::string DB_HOST_HIST;
	static const std::string DB_PORT_HIST;
	static const std::string DB_NAME_HIST;
	static const std::string DB_UNAME_HIST;
	static const std::string WS_URL_HIST;
	
	// GeoDa cache has a table called "history", which stores all user inputs
	// and used for autocompletion in some dialogs.
	OGRLayerProxy* history_table;
	
	// Since history_table is read only, then we'd better maintain the table
	// content in memory by using std::map, parameter_key:parameter_value
	std::vector<std::string> history_keys;
	std::vector<std::string> history_vals;
	
public:
    static wxString GetFullPath();
	/**
	 * Get history information for autocompletion.
	 * For example:
	 *     GetHistory( "database_host" );
	 */
	std::vector<std::string> GetHistory(std::string param_key);

	/**
	 * Add input to history table
	 */
	void AddHistory(std::string param_key, std::string param_val);
	
    void AddEntry(std::string param_key, std::string param_val);
    
	/**
	 * Clean the content of "history" table
	 */
	void CleanHistory();
	
	/**
	 * GdaCache is used to store OGR layer (in memory) to local Spatialite.
	 * This is done in a thread that will not impact the usage of main program.
	 *
	 * If layer has been cached before, update the layer in cache.
	 * Otherwise, create a new layer and store it in Geoda cache.
	 */
	bool CacheLayer(std::string ext_ds_name, 
					OGRLayerProxy* ext_layer_proxy);
	
	bool UpdateLayer(std::string ext_ds_name, 
					 OGRLayerProxy* ext_layer_proxy);
	
	bool IsLayerUpdated(std::string ext_ds_name, 
						OGRLayerProxy* ext_layer_proxy);
	
	bool IsLayerCached(std::string ext_ds_name, 
					   std::string ext_layer_name);
	
	OGRLayerProxy* GetLayerProxy(std::string ext_ds_name, 
								 std::string ext_layer_name);
};

#endif
