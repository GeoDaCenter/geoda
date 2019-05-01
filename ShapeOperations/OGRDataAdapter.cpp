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
#include <wx/wx.h>

#include <wx/wx.h>

#include "OGRDataAdapter.h"
#include "OGRDatasourceProxy.h"
#include "OGRLayerProxy.h"
#include "GdaCache.h"

#include "../DataViewer/TableInterface.h"
#include "../DataViewer/DataSource.h"
#include "../DialogTools/FieldNameCorrectionDlg.h"
#include "../GdaShape.h"

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

OGRDatasourceProxy* OGRDataAdapter::GetDatasourceProxy(const wxString& ds_name,
                                                       GdaConst::DataSourceType ds_type)
{
	OGRDatasourceProxy* ds_proxy;
	
	if (ogr_ds_pool.find(ds_name) != ogr_ds_pool.end()) {
		ds_proxy = ogr_ds_pool[ds_name];
	} else {
		ds_proxy = new OGRDatasourceProxy(ds_name, ds_type, true);
        // if OGRDatasourceProxy is failed to initialized, an exception will
        // be thrown and catched, so the line below will not be executed.
        // There is no case that ds_proxy = NULL
		ogr_ds_pool[ds_name] = ds_proxy;
	}
	
	return ds_proxy;
}

void OGRDataAdapter::RemoveDatasourceProxy(const wxString& ds_name)
{
    if (ogr_ds_pool.find(ds_name) != ogr_ds_pool.end()) {
        OGRDatasourceProxy* ds_proxy = ogr_ds_pool[ds_name];
        if (ds_proxy) delete ds_proxy;
        ogr_ds_pool.erase(ogr_ds_pool.find(ds_name));
    }
}

std::vector<wxString> OGRDataAdapter::GetHistory(const wxString& param_key)
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	return gda_cache->GetHistory(param_key);
}

void OGRDataAdapter::AddHistory(const wxString& param_key, const wxString& param_val)
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	gda_cache->AddHistory(param_key, param_val);
}
void OGRDataAdapter::AddEntry(const wxString& param_key, const wxString& param_val)
{
    if (gda_cache==NULL) gda_cache = new GdaCache();
    gda_cache->AddEntry(param_key, param_val);
}

void OGRDataAdapter::CleanHistory()
{
	if (gda_cache==NULL) gda_cache = new GdaCache();
	gda_cache->CleanHistory();
}

GdaConst::DataSourceType OGRDataAdapter::GetLayerNames(const wxString& ds_name, GdaConst::DataSourceType& ds_type, vector<wxString>& layer_names)
{	
	OGRDatasourceProxy* ds_proxy = GetDatasourceProxy(ds_name, ds_type);
    ds_type = ds_proxy->ds_type;
	layer_names = ds_proxy->GetLayerNames();
    
    return ds_type;
}

// Read OGR Layer using datasource name "ds_name" and layer name "layer_name"
// When read, related OGRDatasourceProxy instance and OGRLayerProxy instance
// will be created and stored in memory, or just get from memory if already
// there.
OGRLayerProxy* OGRDataAdapter::T_ReadLayer(const wxString& ds_name,
                                           GdaConst::DataSourceType ds_type,
                                           const wxString& layer_name)
{
	OGRLayerProxy* layer_proxy = NULL;
    
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
	
	layer_thread = new boost::thread( boost::bind(&OGRLayerProxy::ReadData,
                                                  layer_proxy) );
	return layer_proxy;
}

void OGRDataAdapter::T_StopReadLayer(OGRLayerProxy* layer_proxy)
{
	layer_proxy->stop_reading = TRUE;
	layer_thread->join();
	delete layer_thread;
    layer_thread = NULL;
}
				
void OGRDataAdapter::SaveLayer(OGRLayerProxy* layer_proxy)
{
	if (layer_proxy != NULL) layer_proxy->Save();
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
            } else {
                pt->empty();
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
                    for ( size_t j = 0; j < numPoints; j++ ) {
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
OGRDataAdapter::ExportDataSource(const wxString& o_ds_format,
								 const wxString& o_ds_name,
                                 const wxString& o_layer_name,
                                 OGRwkbGeometryType geom_type,
                                 vector<OGRGeometry*> ogr_geometries,
                                 TableInterface* table,
								 vector<int>& selected_rows,
                                 OGRSpatialReference* spatial_ref,
								 bool is_update)
{
    wxLogMessage("In OGRDataAdapter::ExportDataSource()");
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
                        throw GdaException(msg.mb_str(),
                                           GdaException::FIELD_NAME_EMPTY);
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
                    throw GdaException(msg.mb_str(),
                                       GdaException::FIELD_NAME_EMPTY);
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
    if (is_update) {
        // update layer in datasources, e.g. Sqlite
        bool bUpdate = true;
        export_ds = new OGRDatasourceProxy(o_ds_name, ds_type, bUpdate);
    } else {
        export_ds = new OGRDatasourceProxy(o_ds_format, o_ds_name);
    }
    OGRLayerProxy* new_layer_proxy;
    new_layer_proxy = export_ds->CreateLayer(o_layer_name, geom_type,
                                             ogr_geometries, table,
                                             selected_rows, spatial_ref);

    wxLogMessage("start OGRLayerProxy::AddFreatures()");
    export_thread = new boost::thread(boost::bind(&OGRLayerProxy::AddFeatures,
                                                  new_layer_proxy,
                                                  ogr_geometries,
                                                  table, selected_rows));
   
    wxLogMessage("Out OGRDataAdapter::ExportDataSource()");
    return new_layer_proxy;
}
