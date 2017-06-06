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

#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <boost/multi_array.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <ogrsf_frmts.h>
#include <ogr_api.h>

#include "OGRDataAdapter.h"
#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "GdaCache.h"

#include "../DataViewer/TableInterface.h"
#include "../DataViewer/DataSource.h"
#include "../DialogTools/FieldNameCorrectionDlg.h"
#include "../GdaShape.h"
#include "../ShpFile.h"
#include "../GdaConst.h"
#include "../Project.h"
#include "../GdaException.h"

using namespace Shapefile;
using namespace std;

OGRDataAdapter::OGRDataAdapter()
{
    CPLSetConfigOption("FGDB_BULK_LOAD", "YES");
	OGRRegisterAll();
	
	layer_thread = NULL;
	gda_cache = NULL;
	enable_cache = true;
}

OGRDataAdapter::OGRDataAdapter(bool enable_cache)
{
	OGRRegisterAll();
	
	layer_thread = NULL;
	gda_cache = NULL;
	this->enable_cache = enable_cache;
	//if (enable_cache && gda_cache==NULL) {
	//	gda_cache = new GdaCache();
	//}
}

void OGRDataAdapter::Close()
{
	// clean ogr_ds_pool
	map<wxString, OGRDatasourceProxy*>::iterator it;
	for(it=ogr_ds_pool.begin(); it!=ogr_ds_pool.end(); it++) {
        OGRDatasourceProxy* ds = it->second;
		if (ds) {
            delete ds;
            ds = NULL;
        }
	}
	ogr_ds_pool.clear();
	// clean gda_cache
	if ( !gda_cache) delete gda_cache;
}

OGRDatasourceProxy* OGRDataAdapter::GetDatasourceProxy(wxString ds_name, GdaConst::DataSourceType ds_type)
{
	OGRDatasourceProxy* ds_proxy;
	
	if (ogr_ds_pool.count(ds_name) > 0) {
		ds_proxy = ogr_ds_pool[ds_name];
	} else {
		ds_proxy = new OGRDatasourceProxy(ds_name, ds_type, true);
		ogr_ds_pool[ds_name] = ds_proxy;
	}
	
	return ds_proxy;
}

std::vector<std::string> OGRDataAdapter::GetHistory(string param_key)
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	return gda_cache->GetHistory(param_key);
}

void OGRDataAdapter::AddHistory(string param_key, string param_val)
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	gda_cache->AddHistory(param_key, param_val);
}
void OGRDataAdapter::AddEntry(string param_key, string param_val)
{
    if (gda_cache==NULL) gda_cache = new GdaCache();
    gda_cache->AddEntry(param_key, param_val);
}

void OGRDataAdapter::CleanHistory()
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	gda_cache->CleanHistory();
}

vector<wxString> OGRDataAdapter::GetLayerNames(wxString ds_name, GdaConst::DataSourceType ds_type)
{	
	OGRDatasourceProxy* ds_proxy = GetDatasourceProxy(ds_name, ds_type);
	return ds_proxy->GetLayerNames();
}

// Read OGR Layer using datasource name "ds_name" and layer name "layer_name"
// When read, related OGRDatasourceProxy instance and OGRLayerProxy instance
// will be created and stored in memory, or just get from memory if already
// there.
OGRLayerProxy* OGRDataAdapter::T_ReadLayer(wxString ds_name, GdaConst::DataSourceType ds_type, string layer_name)
{
	OGRLayerProxy* layer_proxy = NULL;
    
	//XXX: we don't cache layer in 1.5.x
	//if (enable_cache && gda_cache) {
	//	if (gda_cache->IsLayerCached(ds_name, layer_name)) {
	//		layer_proxy =  gda_cache->GetLayerProxy(ds_name,layer_name);
	//	}
	//}
    
	if (layer_proxy == NULL) {
		OGRDatasourceProxy* ds_proxy = GetDatasourceProxy(ds_name, ds_type);
		layer_proxy = ds_proxy->GetLayerProxy(layer_name);
	}

	// read actual data in a new thread
	if (layer_thread != NULL) {
		layer_thread->join();
		delete layer_thread;
		layer_thread = NULL;
	}
	
	layer_thread = new boost::thread( boost::bind(&OGRLayerProxy::ReadData, layer_proxy) );
	return layer_proxy;
}

void OGRDataAdapter::T_StopReadLayer(OGRLayerProxy* layer_proxy)
{
	layer_proxy->stop_reading = TRUE;
	layer_thread->join();
	delete layer_thread;
}
				
void OGRDataAdapter::SaveLayer(OGRLayerProxy* layer_proxy)
{
	if (layer_proxy != NULL) layer_proxy->Save();
}

void OGRDataAdapter::CacheLayer
(string ds_name, string layer_name, OGRLayerProxy* layer_proxy)
{
	//XXX: we don't cache layer in 1.5.x
	// cache current layer in a thread
	if (enable_cache && !gda_cache->IsLayerCached(ds_name, layer_name)) {
		boost::thread cache_thread(boost::bind(
			&GdaCache::CacheLayer, gda_cache, ds_name, layer_proxy));
	}
}

void OGRDataAdapter::StopExport()
{
    export_thread->join();
    delete export_thread;
    delete export_ds;
}

void OGRDataAdapter::CancelExport(OGRLayerProxy* layer)
{
    layer->stop_exporting = true;
    export_thread->join();
	delete export_thread;
    delete export_ds;
}

OGRwkbGeometryType
OGRDataAdapter::MakeOGRGeometries(vector<GdaShape*>& geometries, 
								  Shapefile::ShapeType shape_type,
								  vector<OGRGeometry*>& ogr_geometries,
								  vector<int>& selected_rows)
{
	OGRwkbGeometryType eGType = wkbNone;
    int n = selected_rows.size();
    
    // check geometry type: in case of Postgresql, OGRPolygon and OGRMultiPolygon
    // can NOT be created at the same time
    for (size_t i = 0; i < n; i++ ) {
        int id = selected_rows[i];
        if ( shape_type == Shapefile::POINT_TYP ) {
            eGType = wkbPoint;
            break;
        } else if ( shape_type == Shapefile::POLYGON ) {
            GdaPolygon* poly = (GdaPolygon*) geometries[id];
            if (poly->isNull()) {
                
            } else {
                int numParts     = poly->n_count;
                if ( numParts == 1 ) {
                    if (eGType != wkbMultiPolygon)
                        eGType = wkbPolygon;
                } else {
                    eGType = wkbMultiPolygon;
                    break;
                }
            }
        }
    }
    
    // make geometries to OGRGeometry
    for (size_t i = 0; i < n; i++ ) {
        int id = selected_rows[i];
        if ( shape_type == Shapefile::POINT_TYP ) {
            GdaPoint* pc = (GdaPoint*) geometries[id];
            OGRPoint* pt = (OGRPoint*)OGRGeometryFactory::createGeometry(wkbPoint);
            if (!pc->isNull()) {
                pt->setX(pc->GetX());
                pt->setY(pc->GetY());
            }
            ogr_geometries.push_back(pt);
            
        } else if ( shape_type == Shapefile::POLYGON ) {
            
            GdaPolygon* poly = (GdaPolygon*) geometries[id];
            if (poly->isNull()) {
				OGRPolygon* polygon = (OGRPolygon*)OGRGeometryFactory::createGeometry(eGType);
                ogr_geometries.push_back(polygon);
                
            } else {
                int numParts = poly->n_count;
                int numPoints = poly->n;
                double x, y;
                if ( numParts == 1 ) {
    				OGRPolygon* polygon = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
    				OGRLinearRing* ring = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
                    for ( int j = 0; j < numPoints; j++ ) { 
                        if ( poly->points_o != NULL ) {
                            // for created centroids or other geometries, the actual
                            // points are stored in points_o wxRealPoint[].
                            // Note: GdaPolygon::count[] constantly has size = 1 in
                            // current design, see GdaPolygon class
                            x = poly->points_o[j].x;
                            y = poly->points_o[j].y;
                        } else {
                            x = poly->pc->points[j].x;
                            y = poly->pc->points[j].y;
                        }
                        ring->addPoint(x,y);
                    }
                    ring->closeRings();
                    polygon->addRingDirectly(ring);
                    
                    if (eGType == wkbMultiPolygon) {
                        OGRMultiPolygon* multi_polygon = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
                        multi_polygon->addGeometryDirectly(polygon);
                        ogr_geometries.push_back(multi_polygon);
                    } else {
                        ogr_geometries.push_back(polygon);
                    }
                
                } else if ( numParts > 1 ) {
    				OGRMultiPolygon* multi_polygon = (OGRMultiPolygon*)OGRGeometryFactory::createGeometry(wkbMultiPolygon);
                    for ( int num_part = 0; num_part < numParts; num_part++ ) {
    					OGRPolygon* polygon = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
                        OGRLinearRing* ring = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
                        vector<wxInt32> startIndexes = poly->pc->parts;
                        startIndexes.push_back(numPoints);
                        for ( size_t j = startIndexes[num_part];
                              j < startIndexes[num_part+1]; j++ )
                        {
                            
                            x = poly->pc->points[j].x;
                            y = poly->pc->points[j].y;
                            ring->addPoint(x,y);
                        }
                        ring->closeRings();
                        polygon->addRingDirectly(ring);
                        multi_polygon->addGeometryDirectly(polygon);
                    }
                    ogr_geometries.push_back(multi_polygon);
                }
            }
        }
    }
    return eGType;
}

OGRLayerProxy*
OGRDataAdapter::ExportDataSource(string o_ds_format, 
								 wxString o_ds_name,
                                 wxString o_layer_name,
                                 OGRwkbGeometryType geom_type,
                                 vector<OGRGeometry*> ogr_geometries,
                                 TableInterface* table,
								 vector<int>& selected_rows,
                                 OGRSpatialReference* spatial_ref,
								 bool is_update)
{
    GdaConst::DataSourceType ds_type = IDataSource::FindDataSourceType(o_ds_format);
    
    // field identifier: a pair value <column pos, time step> to indicate how to
    // retreive real field name and cell value for time-enabled table
    typedef pair<int, int> field_idn;
    
    vector<field_idn> field_idn_s;
    vector<wxString> field_name_s;
    
    
    // check field names first
    if ( table != NULL ) {
        // get all field names for FieldNameCorrectionDlg
        
        vector<wxString> all_fnames;
        int time_steps = table->GetTimeSteps();
        
        for ( int id=0; id < table->GetNumberCols(); id++ ) {
            if (table->IsColTimeVariant(id)) {
                for ( int t=0; t < time_steps; t++ ) {
                    wxString fname = table->GetColName(id, t);
                    if (fname.IsEmpty()) {
                        wxString msg;
                        msg << "Field name is empty at position: " << id << " and time " << t;
                        throw GdaException(msg.mb_str(), GdaException::FIELD_NAME_EMPTY);
                    }
                    all_fnames.push_back(fname);
                    field_idn_s.push_back(make_pair(id, t));
                    field_name_s.push_back(fname);
                }
            } else {
                wxString fname = table->GetColName(id);
                if (fname.IsEmpty()) {
                    wxString msg;
                    msg << "Field name is empty at position: " << id ;
                    throw GdaException(msg.mb_str(), GdaException::FIELD_NAME_EMPTY);
                }
                all_fnames.push_back(fname);
                field_idn_s.push_back(make_pair(id, 0));
                field_name_s.push_back(fname);
            }
        }
        
        // try to correct
        FieldNameCorrectionDlg fname_correct_dlg(ds_type, all_fnames);
        if ( fname_correct_dlg.NeedCorrection()) {
            if (fname_correct_dlg.ShowModal() != wxID_OK) {
                // cancel at Field Name Correction
                return NULL;
            }
            
            vector<wxString> new_field_name_s = fname_correct_dlg.GetNewFieldNames();
            
            for (size_t i=0; i<new_field_name_s.size(); i++) {
                wxString new_fname = new_field_name_s[i];
                
                if (new_fname == field_name_s[i]) {
                    // don't have to change field name
                    continue;
                }
                field_idn fld_idn = field_idn_s[i];
                table->RenameSimpleCol(fld_idn.first, fld_idn.second, new_fname);
            }
        }
    }
    
	// create new OGRLayerProxy
    OGRLayerProxy* new_layer_proxy = NULL;

    if (is_update) {
        // update layer in datasources, e.g. Sqlite
        export_ds = new OGRDatasourceProxy(o_ds_name, ds_type, true);
        new_layer_proxy = export_ds->CreateLayer(o_layer_name,
                                                 geom_type,
                                                 ogr_geometries,
                                                 table,
                                                 selected_rows,
                                                 spatial_ref);
    } else {
        export_ds = new OGRDatasourceProxy(o_ds_format, o_ds_name);
        new_layer_proxy = export_ds->CreateLayer(o_layer_name,
                                                 geom_type,
                                                 ogr_geometries,
                                                 table,
                                                 selected_rows,
                                                 spatial_ref);
    }

    export_thread = new boost::thread(boost::bind(&OGRLayerProxy::AddFeatures,
                                                  new_layer_proxy,
                                                  ogr_geometries,
                                                  table, selected_rows));
   
    return new_layer_proxy;
}


void OGRDataAdapter::Export(OGRLayerProxy* source_layer_proxy,
                            std::string format,
                            std::string dest_datasource,
                            std::string new_layer_name,
                            bool is_update)
{
    OGRLayer* poSrcLayer = source_layer_proxy->layer;
    OGRwkbGeometryType eGType = poSrcLayer->GetGeomType();
    OGRFeatureDefn *poSrcFDefn = poSrcLayer->GetLayerDefn();
    int number_rows = source_layer_proxy->GetNumRecords();
    
	const char* pszFormat = format.c_str();
	const char* pszDestDataSource = dest_datasource.c_str();
	const char* pszNewLayerName = new_layer_name.c_str();
	char** papszDSCO = NULL;
	char*  pszOutputSRSDef = NULL;
	char**  papszLCO = NULL;
	papszLCO = CSLAddString(papszLCO, "OVERWRITE=yes");
	
    // get information from current layer: geomtype, layer_name
    // (don't consider coodinator system and translation here)
	int bForceToPoint = FALSE;
    int bForceToPolygon = FALSE;
    int bForceToMultiPolygon = FALSE;
    int bForceToMultiLineString = FALSE;
	
	if( wkbFlatten(eGType) == wkbPoint ) bForceToPoint = TRUE;
    else if(wkbFlatten(eGType) == wkbPolygon)  bForceToPolygon = TRUE;
    else if(wkbFlatten(eGType) == wkbMultiPolygon) bForceToMultiPolygon = TRUE;
    else if(wkbFlatten(eGType) == wkbMultiLineString) {
		bForceToMultiLineString = TRUE;
	} else { // not supported geometry type
		export_progress = -1;
		return;
	}
    
	// Try opening the output datasource as an existing, writable
	GDALDataset *poODS = NULL;
    
    if (is_update == true) {
        //poODS = OGRSFDriverRegistrar::Open( dest_datasource.c_str(), true);
        poODS = (GDALDataset*)GDALOpenEx( dest_datasource.c_str(), 
					GDAL_OF_VECTOR, NULL, NULL, NULL);
    } else {
        // Find the output driver.
        GDALDriver *poDriver;
        poDriver  = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL ) {
            // raise driver not supported failure
            error_message << "Current OGR dirver " + format + " is not "
            << "supported by GeoDa.\n" << CPLGetLastErrorMsg();
            export_progress = -1;
            return;
        }

        // Create the output data source.
        poODS = poDriver->Create( pszDestDataSource, 0, 0,0, GDT_Unknown, NULL);
    }
    
	if( poODS == NULL ) {
		// driver failed to create
		//throw GdaException("Can't create output OGR driver.");
		error_message << "Can't create output OGR driver."
        <<"\n\nDetails:"<< CPLGetLastErrorMsg();
		export_progress = -1;
		return;
	}
    
	// Parse the output SRS definition if possible.
	OGRSpatialReference *poOutputSRS = NULL;
	if( pszOutputSRSDef != NULL ) {
		poOutputSRS = (OGRSpatialReference*)OSRNewSpatialReference(NULL);
        if( poOutputSRS->SetFromUserInput( pszOutputSRSDef ) != OGRERR_NONE){
			// raise failed to process SRS definition:
			error_message << "Can't setup SRS spatial definition.";
			export_progress = -1;
			return;
		}
	}
	if( !poODS->TestCapability( ODsCCreateLayer ) ){
		// "Layer %s not found, and CreateLayer not supported by driver.",
		error_message << "Current OGR driver does not support layer creation."
        <<"\n" << CPLGetLastErrorMsg();
		export_progress = -1;
		return;
	}
    
	// Create Layer
	OGRLayer *poDstLayer = poODS->CreateLayer( pszNewLayerName, poOutputSRS,
											  (OGRwkbGeometryType) eGType,
											  papszLCO );
	if( poDstLayer == NULL ){
		//Layer creation failed.
		error_message << "Creating layer field.\n:" << CPLGetLastErrorMsg();
		export_progress = -1;
		return;
	}
	// Process Layer style table
	poDstLayer->SetStyleTable( poSrcLayer->GetStyleTable() );
	OGRFeatureDefn *poDstFDefn = poDstLayer->GetLayerDefn();
	int nSrcFieldCount = poSrcFDefn->GetFieldCount();
	// Add fields. here to copy all field.
    for( int iField = 0; iField < nSrcFieldCount; iField++ ){
        OGRFieldDefn* poSrcFieldDefn = poSrcFDefn->GetFieldDefn(iField);
        OGRFieldDefn oFieldDefn( poSrcFieldDefn );
        // The field may have been already created at layer creation
        if (poDstLayer->CreateField( &oFieldDefn ) == OGRERR_NONE){
            // now that we've created a field, GetLayerDefn() won't return NULL
            if (poDstFDefn == NULL) {
                poDstFDefn = poDstLayer->GetLayerDefn();
			}
        }
    }
    
	// Create OGR geometry features
	for(int row=0; row< number_rows; row++){
		if(stop_exporting) return;
		export_progress++;
		OGRFeature *poFeature;
		poFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());
		poFeature->SetFrom( source_layer_proxy->data[row] );
        if (poFeature != NULL){
            if(bForceToPoint) {
                poFeature->SetGeometryDirectly(
                    source_layer_proxy->data[row]->StealGeometry() );
            }
			else if( bForceToPolygon ) {
                poFeature->SetGeometryDirectly(
                    OGRGeometryFactory::forceToPolygon(
                        source_layer_proxy->data[row]->StealGeometry() ) );
            }
            else if( bForceToMultiPolygon ) {
                poFeature->SetGeometryDirectly(
                    OGRGeometryFactory::forceToMultiPolygon(
                        source_layer_proxy->data[row]->StealGeometry() ) );
            }
            else if ( bForceToMultiLineString ){
                poFeature->SetGeometryDirectly(
                    OGRGeometryFactory::forceToMultiLineString(
                        source_layer_proxy->data[row]->StealGeometry() ) );
            }
        }
        if( poDstLayer->CreateFeature( poFeature ) != OGRERR_NONE ){
			// raise "Failed to create feature in shapefile.\n"
			error_message << "Creating feature (" <<row<<") failed."
            << "\n" << CPLGetLastErrorMsg();
			export_progress = -1;
			return;
        }
		OGRFeature::DestroyFeature( poFeature );
	}
	// Clean
	//OGRDataSource::DestroyDataSource( poODS );
	GDALClose(poODS);
}
