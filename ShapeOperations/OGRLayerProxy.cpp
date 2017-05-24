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
#include <ogrsf_frmts.h>
#include <climits>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "../ShpFile.h"
#include "../GdaException.h"
#include "../logger.h"
#include "../GeneralWxUtils.h"
#include "../GdaShape.h"
#include "../GdaCartoDB.h"
#include "../GdaException.h"

#include "OGRLayerProxy.h"
#include "OGRFieldProxy.h"


using namespace std;

/**
 * Create a OGRLayerProxy from an existing OGRLayer
 */
OGRLayerProxy::OGRLayerProxy(string layer_name,
                             OGRLayer* _layer,
                             GdaConst::DataSourceType _ds_type,
                             bool isNew)
: n_rows(0), n_cols(0), name(layer_name),ds_type(_ds_type), layer(_layer),
load_progress(0), stop_reading(false), export_progress(0)
{
    if (!isNew) n_rows = layer->GetFeatureCount(FALSE);
    is_writable = layer->TestCapability(OLCCreateField) != 0;
    
	eGType = layer->GetGeomType();
    spatialRef = layer->GetSpatialRef();
    
    // get feature definition
	featureDefn = layer->GetLayerDefn();
    n_cols = featureDefn->GetFieldCount();
	ReadFieldInfo();
}

/**
 * Create a OGRLayerProxy from an existing OGRLayer
 */
OGRLayerProxy::OGRLayerProxy(OGRLayer* _layer,
                             GdaConst::DataSourceType _ds_type,
                             OGRwkbGeometryType eGType,
                             int _n_rows)
: layer(_layer), name(_layer->GetName()), ds_type(_ds_type), n_rows(_n_rows),
eLayerType(eGType), load_progress(0), stop_reading(false)
{
    if (n_rows==0) {
        n_rows = layer->GetFeatureCount();
    }
    
    is_writable = layer->TestCapability(OLCCreateField) != 0;
    
    // get feature definition
	featureDefn = layer->GetLayerDefn();
    n_cols = featureDefn->GetFieldCount();
	ReadFieldInfo();
}

OGRLayerProxy::~OGRLayerProxy()
{
	// clean OGR features
    for ( size_t i=0; i < data.size(); ++i ) {
        OGRFeature::DestroyFeature(data[i]);
	}
	data.clear();
	// we don't need to clean OGR fields
    for ( size_t i=0; i < fields.size(); ++i ) {
        delete fields[i];
    }
	fields.clear();
}

void OGRLayerProxy::SetOGRLayer(OGRLayer* new_layer)
{
    layer = new_layer;
}

bool OGRLayerProxy::ReadFieldInfo()
{
	for (int col_idx=0; col_idx<n_cols; col_idx++) {
		OGRFieldDefn *fieldDefn = featureDefn->GetFieldDefn(col_idx);
		OGRFieldProxy *fieldProxy = new OGRFieldProxy(fieldDefn);
		this->fields.push_back(fieldProxy);
	}
	return true;
}

void OGRLayerProxy::GetVarTypeMap(vector<wxString>& var_list,
                                map<wxString,GdaConst::FieldType>& var_type_map)
{
	// Get field/variable list of current ogr_layer, store in var_list vector
	// Get field/variable:(its type) pair of current ogr_layer, store in map
	for( int i=0; i< n_cols; i++) {
        wxString var = fields[i]->GetName();
		var_list.push_back( var );
		var_type_map[ var ] = fields[i]->GetType();
	}
}

wxString OGRLayerProxy::GetFieldName(int pos)
{
	return fields[pos]->GetName();
}

void OGRLayerProxy::SetFieldName(int pos, const wxString& new_fname)
{
    fields[pos]->SetName(new_fname);
}

GdaConst::FieldType OGRLayerProxy::GetFieldType(int pos)
{
	return fields[pos]->GetType();
}

GdaConst::FieldType OGRLayerProxy::GetFieldType(const wxString& field_name)
{
    return GetFieldType( GetFieldPos(field_name));
}

int OGRLayerProxy::GetFieldLength(int pos)
{
	return fields[pos]->GetLength();
}

void OGRLayerProxy::SetFieldLength(int pos, int new_len)
{
    fields[pos]->SetLength(new_len);
}

void OGRLayerProxy::SetFieldDecimals(int pos, int new_dec)
{
    fields[pos]->SetDecimals(new_dec);
}

int OGRLayerProxy::GetFieldDecimals(int pos)
{
	return fields[pos]->GetDecimals();
}

int OGRLayerProxy::GetFieldPos(const wxString& field_name)
{
	for (size_t i=0, iend=fields.size(); i<iend; ++i) {
		if (fields[i]->GetName().CmpNoCase(field_name)==0) return i;
	}
    return -1;
}

OGRFieldType OGRLayerProxy::GetOGRFieldType(GdaConst::FieldType field_type)
{
	OGRFieldType ogr_type = OFTString; // default OFTString
	if (field_type == GdaConst::string_type){
		ogr_type = OFTString;
	}
	else if (field_type == GdaConst::long64_type) {
		ogr_type = OFTInteger64;
	}
	else if (field_type == GdaConst::double_type) {
		ogr_type = OFTReal;
	}
	else if (field_type == GdaConst::date_type) {
		ogr_type = OFTDate;
	}
	else if (field_type == GdaConst::time_type) {
		ogr_type = OFTTime;
	}
	else if (field_type == GdaConst::datetime_type) {
		ogr_type = OFTDateTime;
	}
	return ogr_type;
}

bool OGRLayerProxy::IsFieldExisted(const wxString& field_name)
{
	// check if field existed by given field name
	std::vector<OGRFieldProxy*>::iterator it;
	for (it = fields.begin(); it!=fields.end(); it++){
		if (field_name.CmpNoCase((*it)->GetName()) == 0 ){
            return true;
		}
	}
	return false;
}

void OGRLayerProxy::UpdateFieldProperties(int col)
{
	OGRFieldProxy *field_proxy = fields[col];
    if ( !field_proxy->IsChanged()) return;
    
    field_proxy->Update();
	if ( layer->AlterFieldDefn(col, field_proxy->GetFieldDefn(),
							   ALTER_WIDTH_PRECISION_FLAG)!= OGRERR_NONE ) {
		wxString msg;
		msg << "Change field properties (" << name <<") failed.";
		msg << "\n\nDetails:" << CPLGetLastErrorMsg();
		throw GdaException(msg.mb_str());
	}
}

int OGRLayerProxy::AddField(const wxString& field_name,
							GdaConst::FieldType field_type,
							int field_length,
							int field_precision)
{
	// check if field existed
	if (IsFieldExisted(field_name)) {
		wxString msg;
		msg << "Field (" << field_name <<") already exited.";
		throw GdaException(msg.mb_str());
	}
	OGRFieldType  ogr_type = GetOGRFieldType(field_type);
	OGRFieldProxy *oField = new OGRFieldProxy(field_name, ogr_type, 
											  field_length, field_precision);
	if ( layer->CreateField( oField->GetFieldDefn() ) != OGRERR_NONE ) {
		wxString msg;
		msg << "Internal Error: Add new field (" << field_name <<") failed."
            << "\n\nDetails:" << CPLGetLastErrorMsg();
		throw GdaException(msg.mb_str());
	}					
	n_cols++;
	// Add this new field to OGRFieldProxy
	this->fields.push_back(oField);
	return n_cols-1;
}

void OGRLayerProxy::DeleteField(int pos)
{
    /*
    // remove this field in local OGRFeature vector
    for (size_t i=0; i < data.size(); ++i) {
        OGRFeature* my_feature = data[i];
		my_feature->DeleteField(pos);
    }
	// delete field in actual datasource
	if( this->layer->DeleteField(pos) != OGRERR_NONE ) {
		wxString msg;
		msg << "Internal Error: Delete field failed."
        << "\n\nDetails:" << CPLGetLastErrorMsg();
		throw GdaException(msg.mb_str());
	}	
	n_cols--;
	// remove this field from OGRFieldProxy
	this->fields.erase( fields.begin() + pos ); 
    */
	this->fields.erase( fields.begin() + pos ); 
}

void OGRLayerProxy::DeleteField(const wxString& field_name)
{
	int pos = this->GetFieldPos(field_name);
	DeleteField(pos);
}

bool OGRLayerProxy::IsTableOnly()
{
    if ( !data.empty() ) {
        OGRGeometry* geom = data[0]->GetGeometryRef();
        if (geom != NULL)
            return false;
    }
    return true;
}

bool OGRLayerProxy::UpdateOGRFeature(OGRFeature* feature)
{
	if (layer->SetFeature(feature) == OGRERR_NONE)
        return true;
    return false;
}

bool OGRLayerProxy::AppendOGRFeature(std::vector<std::string>& content)
{
	OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
	feature->SetFrom( data[0]);
	for (size_t i=0; i < content.size(); i++){
		feature->SetField(i, content[i].c_str());
	}
	if(layer->CreateFeature( feature ) != OGRERR_NONE) {
		return false;
	}
	n_rows++;
	return true;
}


bool OGRLayerProxy::CallCartoDBAPI(wxString url)
{
    return true;
}

bool OGRLayerProxy::UpdateColumn(int col_idx, vector<double> &vals)
{
    if (ds_type == GdaConst::ds_cartodb) {
        // update column using CARTODB_API directly, avoid single UPDATE clause
        string col_name(GetFieldName(col_idx).mb_str());
        CartoDBProxy::GetInstance().UpdateColumn(name, col_name, vals);
        
        // update memory still
        for (int rid=0; rid < n_rows; rid++) {
            data[rid]->SetField(col_idx, vals[rid]);
        }
        
    } else {
        for (int rid=0; rid < n_rows; rid++) {
            SetValueAt(rid, col_idx, vals[rid]);
        }
    }
	return true;
    
}
bool OGRLayerProxy::UpdateColumn(int col_idx, vector<wxInt64> &vals)
{
    if (ds_type == GdaConst::ds_cartodb) {
        // update column using CARTODB_API directly, avoid single UPDATE clause
        string col_name(GetFieldName(col_idx).mb_str());
        CartoDBProxy::GetInstance().UpdateColumn(name, col_name, vals);
        
        // update memory still
        for (int rid=0; rid < n_rows; rid++) {
            data[rid]->SetField(col_idx, (GIntBig)vals[rid]);
        }
        
    } else {
        for (int rid=0; rid < n_rows; rid++) {
            SetValueAt(rid, col_idx, (GIntBig)vals[rid]);
        }
    }
	return true;
}

bool OGRLayerProxy::UpdateColumn(int col_idx, vector<wxString> &vals)
{
    if (ds_type == GdaConst::ds_cartodb) {
        // update column using CARTODB_API directly, avoid single UPDATE clause
        string col_name(GetFieldName(col_idx).mb_str());
        CartoDBProxy::GetInstance().UpdateColumn(name, col_name, vals);
        
        // update memory still
        for (int rid=0; rid < n_rows; rid++) {
            data[rid]->SetField(col_idx, vals[rid].mb_str());
        }
        
    } else {
        for (int rid=0; rid < n_rows; rid++) {
            SetValueAt(rid, col_idx, vals[rid].mb_str());
        }
    }
	return true;
}

void
OGRLayerProxy::AddFeatures(vector<OGRGeometry*>& geometries,
                           TableInterface* table,
                           vector<int>& selected_rows)
{
    export_progress = 0;
    stop_exporting = false;

    // Create features in memory first
    for (size_t i=0; i<selected_rows.size();++i) {
        if (stop_exporting) {
            return;
        }
        OGRFeature *poFeature = OGRFeature::CreateFeature(featureDefn);
        if ( !geometries.empty()) {
            poFeature->SetGeometryDirectly( geometries[i] );
        }
        data.push_back(poFeature);
    }
    
    int export_size = data.size()==0 ? table->GetNumberRows() : data.size();
    export_progress = export_size / 4;
   
    bool ignore_case = false;
    
    if (ds_type == GdaConst::ds_postgresql ||
        ds_type == GdaConst::ds_sqlite) {
        ignore_case = true;
    }
    
    // Fill the feature with content
    if (table != NULL) {
        // fields already have been created by OGRDatasourceProxy::CreateLayer()
        for (size_t j=0; j< fields.size(); j++) {
            
            wxString fname = fields[j]->GetName();
            GdaConst::FieldType ftype = fields[j]->GetType();
           
            // get underneath column position (no group and time =0)
            int col_pos = table->GetColIdx(fname, ignore_case);
          
            if (col_pos < 0) {
                //wxString msg = wxString::Format(_(" Save column %s failed. Please check your data, or contact GeoDa team."), fname);
                //error_message << msg;
                //export_progress = -1;
                //return;
                continue;
            }
            
            vector<bool> undefs;
            
            if ( ftype == GdaConst::long64_type) {
                
                vector<wxInt64> col_data;
                table->GetDirectColData(col_pos, col_data);
                table->GetDirectColUndefined(col_pos, undefs);
                
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                    
                    if (undefs[orig_id]) {
                        data[k]->UnsetField(j);
                    } else {
                        data[k]->SetField(j, (GIntBig)(col_data[orig_id]));
                    }
                    if (stop_exporting)
                        return;
                }
                
            } else if (ftype == GdaConst::double_type) {
                
                vector<double> col_data;
                table->GetDirectColData(col_pos, col_data);
                table->GetDirectColUndefined(col_pos, undefs);
                
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                    
                    if (undefs[orig_id]) {
                        data[k]->UnsetField(j);
                    } else {
                        data[k]->SetField(j, col_data[orig_id]);
                    }
                    if (stop_exporting)
                        return;
                }
                
            } else if (ftype == GdaConst::date_type ||
                       ftype == GdaConst::time_type ||
                       ftype == GdaConst::datetime_type ) {
                
                vector<wxInt64> col_data;
                table->GetDirectColData(col_pos, col_data);
                table->GetDirectColUndefined(col_pos, undefs);
                
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                    
                    wxInt64 val = col_data[ orig_id ];
                    int year = val / 10000000000;
                    val = val % 10000000000;
                    int month = val / 100000000;
                    val = val % 100000000;
                    int day = val  / 1000000;
                    val = val % 1000000;
                    int hour = val / 10000;
                    val = val % 10000;
                    int minute = val / 100;
                    int second = val % 100;
                    
                    if (undefs[orig_id]) {
                        data[k]->UnsetField(j);
                    } else {
                        data[k]->SetField(j, year, month, day, hour, minute, second);
                    }
                    if (stop_exporting) return;
                }
                
            } else if (ftype == GdaConst::placeholder_type) {
                // KML case: there are by default two fields:
                // [Name, Description], so if placeholder that
                // means table is empty. Then do nothing
                
            } else {
                // others are treated as string_type
                // XXX encodings
                vector<wxString> col_data;
                table->GetDirectColData(col_pos, col_data);
                table->GetDirectColUndefined(col_pos, undefs);
                
                if (ds_type == GdaConst::ds_csv) {
                    for (int m=0; m<col_data.size(); m++) {
                        if (col_data[m].IsEmpty())
                            col_data[m] = " ";
                    }
                }
                
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                    
                    if (undefs[orig_id]) {
                        data[k]->UnsetField(j);
                    } else {
                        data[k]->SetField(j, col_data[orig_id].mb_str());
                    }
                    if (stop_exporting) return;
                }
            }
            if (stop_exporting) return;
        }
    }
    export_progress = export_size / 2;
   
    int n_data = data.size();
    for (int i=0; i<n_data; i++) {
        if (stop_exporting)
            return;
        if ((i+1)%2==0) {
            export_progress++;
        }
        if( layer->CreateFeature( data[i] ) != OGRERR_NONE ) {
            wxString msg = wxString::Format(" Failed to create feature (%d/%d).", i + 1, n_data);
            error_message << msg << CPLGetLastErrorMsg();
            export_progress = -1;
			return;
        }
    }
    Save();
    export_progress = export_size;
}

bool OGRLayerProxy::InsertOGRFeature()
{
	wxString msg;
	msg << "GeoDa does not support InsertOGRFeature yet.";
    throw GdaException(msg);
    return false;
}

void OGRLayerProxy::Save()
{
	if(layer->SyncToDisk() != OGRERR_NONE){
		wxString msg;
		msg << "Internal Error: Save OGR layer (" << name <<") failed."
		    << "\n\nDetails:"<< CPLGetLastErrorMsg();
		throw GdaException(msg.mb_str());
	}
}

bool OGRLayerProxy::HasError()
{
    return !error_message.str().empty();
}

bool OGRLayerProxy::ReadData()
{
	if (n_rows > 0 && n_rows == data.size()) {
        // if data already been read, skip
        return true;
    }
    if (n_rows == 0) {
        // in some case  ArcSDE plugin can't return proper row number from
        // SDE engine. we will count it feature by feature
        n_rows = -1;
    }
	int row_idx = 0;
	OGRFeature *feature = NULL;
    map<int, OGRFeature*> feature_dict;
    
    layer->ResetReading();
	while ((feature = layer->GetNextFeature()) != NULL) {
        if (feature == NULL) {
            error_message << "GeoDa can't read data from datasource."
		    << "\n\nDetails:"<< CPLGetLastErrorMsg();
            return false;
        }
        
        // thread feature: user can stop reading
		if (stop_reading)
            break;
        
        //long fid = feature->GetFID();
        feature_dict[row_idx] = feature;
        
        // keep load_progress not 100%, so that it can finish this function
		load_progress = row_idx++;
	}
    if (row_idx == 0) {
		error_message << "GeoDa can't read data from datasource."
		    << "\n\nDetails: Datasource is empty. "<< CPLGetLastErrorMsg();
        
        return false;
    }
	n_rows = row_idx;
    
    // check empty rows at the end of table, remove empty rows #563
    for (int i = n_rows-1; i>=0; i--) {
        OGRFeature* my_feature = feature_dict[i];
        bool is_empty = true;
        for (int j= 0; j<n_cols; j++) {
            if (my_feature->IsFieldSet(j)) {
                is_empty = false;
                break;
            }
        }
        if (is_empty) {
            OGRGeometry* my_geom = my_feature->GetGeometryRef();
            if (my_geom == NULL) {
                n_rows -= 1;
            }
        }
    }
    // create copies of OGRFeatures
    for (int i = 0; i < n_rows; i++) {
        OGRFeature* my_feature = feature_dict[i]->Clone();
        data.push_back(my_feature);
        OGRFeature::DestroyFeature(feature_dict[i]);
    }
    
    load_progress = row_idx;
    feature_dict.clear();
    
	return true;
}

void OGRLayerProxy::GetExtent(Shapefile::Main& p_main,
                              Shapefile::PointContents* pc, int row_idx)
{
    if (row_idx==0) {
        p_main.header.bbox_x_min = pc->x;
        p_main.header.bbox_y_min = pc->x;
        p_main.header.bbox_x_max = pc->y;
        p_main.header.bbox_y_max = pc->y;
    } else {
        if (pc->x < p_main.header.bbox_x_min)
            p_main.header.bbox_x_min = pc->x;
        if (pc->x > p_main.header.bbox_x_max)
            p_main.header.bbox_x_max = pc->x;
        if (pc->y < p_main.header.bbox_y_min)
            p_main.header.bbox_y_min = pc->y;
        if (pc->y > p_main.header.bbox_y_max)
            p_main.header.bbox_y_max = pc->y;
    }
}

void OGRLayerProxy::GetExtent(Shapefile::Main& p_main,
                              Shapefile::PolygonContents* pc, int row_idx)
{
    if (row_idx==0) {
        p_main.header.bbox_x_min = pc->box[0];
        p_main.header.bbox_y_min = pc->box[1];
        p_main.header.bbox_x_max = pc->box[2];
        p_main.header.bbox_y_max = pc->box[3];
    } else {
        if (pc->box[0] < p_main.header.bbox_x_min)
            p_main.header.bbox_x_min = pc->box[0];
        if (pc->box[2] > p_main.header.bbox_x_max)
            p_main.header.bbox_x_max = pc->box[2];
        if (pc->box[1] < p_main.header.bbox_y_min)
            p_main.header.bbox_y_min = pc->box[1];
        if (pc->box[3] > p_main.header.bbox_y_max)
            p_main.header.bbox_y_max = pc->box[3];
    }
}

void OGRLayerProxy::CopyEnvelope(OGRPolygon* p, Shapefile::PolygonContents* pc)
{
    OGREnvelope pEnvelope;
    p->getEnvelope(&pEnvelope);
    pc->box[0] = pEnvelope.MinX;
    pc->box[1] = pEnvelope.MinY;
    pc->box[2] = pEnvelope.MaxX;
    pc->box[3] = pEnvelope.MaxY;
}

bool OGRLayerProxy::AddGeometries(Shapefile::Main& p_main)
{
    // NOTE: OGR/GDAL 2.0 is still implementing addGeomField feature.
    // So, we only support limited datasources for adding geometries.
    if ( !(ds_type == GdaConst::ds_geo_json ||
           ds_type == GdaConst::ds_gml ||
           ds_type == GdaConst::ds_kml ||
           ds_type == GdaConst::ds_sqlite ||
           ds_type == GdaConst::ds_gpkg) )
    {
        return false;
    }

    //create geometry field
    int n_geom = p_main.records.size();
    if (n_geom < n_rows)
        return false;
    
    vector<GdaShape*> geometries;
    Shapefile::ShapeType shape_type = Shapefile::NULL_SHAPE;
    int num_geometries = p_main.records.size();
    if ( p_main.header.shape_type == Shapefile::POINT_TYP) {
        Shapefile::PointContents* pc;
        for (int i=0; i<num_geometries; i++) {
            pc = (Shapefile::PointContents*)p_main.records[i].contents_p;
            //xxx
            geometries.push_back(new GdaPoint(wxRealPoint(pc->x, pc->y)));
        }
        shape_type = Shapefile::POINT_TYP;
        
    } else if (p_main.header.shape_type == Shapefile::POLYGON) {
        Shapefile::PolygonContents* pc;
        for (int i=0; i < num_geometries; i++) {
            pc = (Shapefile::PolygonContents*)p_main.records[i].contents_p;
            geometries.push_back(new GdaPolygon(pc));
        }
        shape_type = Shapefile::POLYGON;
    }
    
    for (int id=0; id < n_geom; id++) {
        if ( shape_type == Shapefile::POINT_TYP ) {
            OGRwkbGeometryType eGType = wkbPoint;
            GdaPoint* pc = (GdaPoint*) geometries[id];
            OGRPoint pt;
            if (!pc->isNull()) {
                pt.setX( pc->GetX() );
                pt.setY( pc->GetY() );
            }
            data[id]->SetGeometry( &pt);
        } else if ( shape_type == Shapefile::POLYGON ) {
            GdaPolygon* poly = (GdaPolygon*) geometries[id];
            if (poly->isNull()) {
                // special case for null polygon
                OGRPolygon polygon;
                data[id]->SetGeometry(&polygon);
            } else {
                int numParts = poly->n_count;
                int numPoints = poly->n;
                // for shp/dbf reading, GdaPolygon still use "pc", which is from
                // main data, see Shapefile::Main
                if ( numParts == 1 ) {
                    OGRwkbGeometryType eGType = wkbPolygon;
                    OGRPolygon polygon;
                    OGRLinearRing ring;
                    double x, y;
                    for ( int j = 0; j < numPoints; j++ ) {
                        if ( poly->points_o != NULL ) {
                            x = poly->points_o[j].x;
                            y = poly->points_o[j].y;
                        } else {
                            x = poly->pc->points[j].x;
                            y = poly->pc->points[j].y;
                        }
                        ring.addPoint(x,y);
                    }
                    ring.closeRings();
                    polygon.addRing(&ring);
                    data[id]->SetGeometry(&polygon);
                } else if ( numParts > 1 ) {
                    OGRwkbGeometryType eGType = wkbMultiPolygon;
                    OGRMultiPolygon multi_polygon;
                    for ( int num_part = 0; num_part < numParts; num_part++ ) {
                        OGRPolygon polygon;
                        OGRLinearRing ring;
                        std::vector<wxInt32> startIndexes = poly->pc->parts;
                        startIndexes.push_back(numPoints);
                        for ( int j = startIndexes[num_part];
                             j < startIndexes[num_part+1]; j++ ) {
                            double x = poly->pc->points[j].x;
                            double y = poly->pc->points[j].y;
                            ring.addPoint(x,y);
                        }
                        ring.closeRings();
                        polygon.addRing(&ring);
                        multi_polygon.addGeometry(&polygon);
                    }
                    data[id]->SetGeometry(&multi_polygon);
                }
            }
        }
        layer->SetFeature(data[id]);
    }
    return true;
}

bool OGRLayerProxy::GetExtent(double& minx, double& miny,
                              double& maxx, double& maxy)
{
	OGREnvelope pEnvelope;
	layer->GetExtent(&pEnvelope);
	minx = pEnvelope.MinX;
	miny = pEnvelope.MinY;
	maxx = pEnvelope.MaxX;
	maxy = pEnvelope.MaxY;
    
    if ( minx == miny && maxx == maxy && minx == 0 && maxx==0) return false;
    return true;
}

bool OGRLayerProxy::ReadGeometries(Shapefile::Main& p_main)
{
	// get geometry envelope
	OGREnvelope pEnvelope;
    if (layer->GetExtent(&pEnvelope) == OGRERR_NONE) {
        p_main.header.bbox_x_min = pEnvelope.MinX;
        p_main.header.bbox_y_min = pEnvelope.MinY;
        p_main.header.bbox_x_max = pEnvelope.MaxX;
        p_main.header.bbox_y_max = pEnvelope.MaxY;

    }
    p_main.header.bbox_z_min = 0;
	p_main.header.bbox_z_max = 0;
	p_main.header.bbox_m_min = 0;
	p_main.header.bbox_m_max = 0;
    
	// resize geometry records
	p_main.records.resize(n_rows);
	bool noExtent = (pEnvelope.MinX == pEnvelope.MaxX) &&
                    (pEnvelope.MinY == pEnvelope.MaxY);
    noExtent = false;
    
	//read OGR geometry features
	int feature_counter =0;
	for ( int row_idx=0; row_idx < n_rows; row_idx++ ) {
		OGRFeature* feature = data[row_idx];
		OGRGeometry* geometry= feature->GetGeometryRef();
		OGRwkbGeometryType eType = geometry ? wkbFlatten(geometry->getGeometryType()) : eGType;
		// sometime OGR can't return correct value from GetGeomType() call
		if (eGType == wkbUnknown)
            eGType = eType;
        
		if (eType == wkbPoint) {
			Shapefile::PointContents* pc = new Shapefile::PointContents();
			pc->shape_type = Shapefile::POINT_TYP;
            if (geometry) {
                if (feature_counter==0)
                    p_main.header.shape_type = Shapefile::POINT_TYP;
                OGRPoint* p = (OGRPoint *) geometry;
                pc->x = p->getX();
                pc->y = p->getY();
                if (noExtent)
                    GetExtent(p_main, pc, row_idx);
            }
			p_main.records[feature_counter++].contents_p = pc;
			
		} else if (eType == wkbMultiPoint) {
			Shapefile::PointContents* pc = new Shapefile::PointContents();
			pc->shape_type = Shapefile::POINT_TYP;
			if (geometry) {
                if (feature_counter==0)
                    p_main.header.shape_type = Shapefile::POINT_TYP;
                OGRMultiPoint* mp = (OGRMultiPoint*) geometry;
				int n_geom = mp->getNumGeometries();
				for (size_t i = 0; i < n_geom; i++ )
                {	
					// only consider first point
                    OGRGeometry* ogrGeom = mp->getGeometryRef(i);
                    OGRPoint* p = static_cast<OGRPoint*>(ogrGeom);
					pc->x = p->getX();
					pc->y = p->getY();
					if (noExtent)
						GetExtent(p_main, pc, row_idx);
					
				}
            }
			p_main.records[feature_counter++].contents_p = pc;
			
		} else if (eType == wkbPolygon || eType == wkbCurvePolygon ) {
			Shapefile::PolygonContents* pc = new Shapefile::PolygonContents();
			pc->shape_type = Shapefile::POLYGON;
            if (geometry) {
                if (feature_counter==0)
                    p_main.header.shape_type = Shapefile::POLYGON;
                OGRPolygon* p = (OGRPolygon *) geometry;
                CopyEnvelope(p, pc);
                OGRLinearRing* pLinearRing = NULL;
                int numPoints= 0;
                // interior rings
                int ni_rings = p->getNumInteriorRings();
                // resize parts memory, 1 is for exterior ring,
                pc->num_parts = ni_rings + 1;
                pc->parts.resize(pc->num_parts);
                for (size_t j=0; j < pc->num_parts; j++ ) {
                    pLinearRing = j==0 ? 
                        p->getExteriorRing() : p->getInteriorRing(j-1);
                    pc->parts[j] = numPoints;
                    if (pLinearRing)
                        numPoints += pLinearRing->getNumPoints();
                }
                // resize points memory					
                pc->num_points = numPoints;
                pc->points.resize(pc->num_points);
                // read points
                int i=0;
                for (size_t j=0; j < pc->num_parts; j++) {
                    pLinearRing = j==0 ?
                        p->getExteriorRing() : p->getInteriorRing(j-1);
                    if (pLinearRing)
                        for (size_t k=0; k < pLinearRing->getNumPoints(); k++){
                            pc->points[i].x =  pLinearRing->getX(k);
                            pc->points[i++].y =  pLinearRing->getY(k);
                        }
                }
                if (noExtent)
                    GetExtent(p_main, pc, row_idx);
            }
			p_main.records[feature_counter++].contents_p = pc;
            
		} else if (eType == wkbMultiPolygon) {
			Shapefile::PolygonContents* pc = new Shapefile::PolygonContents();
			pc->shape_type = Shapefile::POLYGON;
            if (geometry) {
                if (feature_counter==0)
                    p_main.header.shape_type = Shapefile::POLYGON;
                OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geometry;
                int n_geom = mpolygon->getNumGeometries();
                // if there is more than one polygon, then we need to count which
                // part is processing accumulatively
                int part_idx = 0, numPoints = 0;
                OGRLinearRing* pLinearRing = NULL;
                int pidx =0;
                for (size_t i = 0; i < n_geom; i++ )
                {	
                    OGRGeometry* ogrGeom = mpolygon->getGeometryRef(i);
                    OGRPolygon* p = static_cast<OGRPolygon*>(ogrGeom);
                    if ( i == 0 ) {
                        CopyEnvelope(p, pc);
                    } else {
                        OGREnvelope pBox;
                        p->getEnvelope(&pBox);
                        if ( pc->box[0] > pBox.MinX ) pc->box[0] = pBox.MinX;
                        if ( pc->box[1] > pBox.MinY ) pc->box[1] = pBox.MinY;
                        if ( pc->box[2] < pBox.MaxX ) pc->box[2] = pBox.MaxX;
                        if ( pc->box[3] < pBox.MaxY ) pc->box[3] = pBox.MaxY;
                    }
                    // number of interior rings + 1 exterior ring
                    int ni_rings = p->getNumInteriorRings()+1;
                    pc->num_parts += ni_rings;
                    pc->parts.resize(pc->num_parts);
                    
                    for (size_t j=0; j < ni_rings; j++) {
                        pLinearRing = j==0 ? 
                            p->getExteriorRing() : p->getInteriorRing(j-1);
                        pc->parts[part_idx++] = numPoints;
                        numPoints += pLinearRing->getNumPoints();
                    }
                    // resize points memory					
                    pc->num_points = numPoints;
                    pc->points.resize(pc->num_points);
                    // read points
                    for (size_t j=0; j < ni_rings; j++) {
                        pLinearRing = j==0 ? 
                            p->getExteriorRing() : p->getInteriorRing(j-1);
                        for (int k=0; k < pLinearRing->getNumPoints(); k++) {
                            pc->points[pidx].x = pLinearRing->getX(k);
                            pc->points[pidx++].y = pLinearRing->getY(k);
                        }
                    }
                    if (noExtent)
                        GetExtent(p_main, pc, row_idx);
                }
            }
			p_main.records[feature_counter++].contents_p = pc;
            
        } else {
            std::string open_err_msg = "GeoDa does not support datasource with line data at this time.  Please choose a datasource with either point or polygon data.";
            throw GdaException(open_err_msg.c_str());
        }
	}
    
	return true;
}

void OGRLayerProxy::T_Export(std::string format,
                             std::string dest_datasource,
							 std::string new_layer_name,
                             bool is_update)
{
	export_progress = 0;
	stop_exporting = FALSE;
	boost::thread export_thread(boost::bind(&OGRLayerProxy::Export, this, 
											format, dest_datasource,
											new_layer_name, is_update));
}

void OGRLayerProxy::T_StopExport()
{
	stop_exporting = TRUE;
	export_progress = 0;	
}

void OGRLayerProxy::Export(std::string format,
                           std::string dest_datasource,
                           std::string new_layer_name,
                           bool is_update)
{
	const char* pszFormat = format.c_str();
	const char* pszDestDataSource = dest_datasource.c_str();
	const char* pszNewLayerName = new_layer_name.c_str();
	char** papszDSCO = NULL;
	char*  pszOutputSRSDef = NULL;
	char**  papszLCO = NULL;
	papszLCO = CSLAddString(papszLCO, "OVERWRITE=yes");
	OGRLayer* poSrcLayer = this->layer;
	OGRFeatureDefn *poSrcFDefn = poSrcLayer->GetLayerDefn();
    
	// get information from current layer: geomtype, layer_name
    // (don't consider coodinator system and translation here)
	int bForceToPoint = FALSE;
    int bForceToPolygon = FALSE;
    int bForceToMultiPolygon = FALSE;
    int bForceToMultiLineString = FALSE;
	
	if( wkbFlatten(eGType) == wkbPoint ) 
        bForceToPoint = TRUE;
    else if(wkbFlatten(eGType) == wkbPolygon)  
        bForceToPolygon = TRUE;
    else if(wkbFlatten(eGType) == wkbMultiPolygon) 
        bForceToMultiPolygon = TRUE;
    else if(wkbFlatten(eGType) == wkbMultiLineString) {
		bForceToMultiLineString = TRUE;
	} else { // not supported geometry type
		export_progress = -1;
		return;
	}
	// Try opening the output datasource as an existing, writable
    GDALDataset  *poODS = NULL;
    
    if (is_update == true) {
        poODS = (GDALDataset*) GDALOpenEx( pszDestDataSource,
                                          GDAL_OF_VECTOR, NULL, NULL, NULL );
    } else {
        // Find the output driver.
        GDALDriver *poDriver;
        poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        
        if( poDriver == NULL ) {
            // raise driver not supported failure
            error_message << "Current OGR dirver " + format + " is not "
                          << "supported by GeoDa.\n" << CPLGetLastErrorMsg();
            export_progress = -1;
            return;
        }

        // Create the output data source.  
        poODS = poDriver->Create(pszDestDataSource, 0, 0, 0, GDT_Unknown, NULL);
    }
    
	if( poODS == NULL ) {
		// driver failed to create
		// throw GdaException("Can't create output OGR driver.");
		error_message << "Can't create output OGR driver."
                      <<"\n\nDetails:"<< CPLGetLastErrorMsg();
		export_progress = -1;
		return;
	}

    // Parse the output SRS definition if possible.
	OGRSpatialReference *poOutputSRS = this->GetSpatialReference();
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
	for(int row=0; row< this->n_rows; row++){
		if(stop_exporting) return;
		export_progress++;
		OGRFeature *poFeature;
		poFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());		
		poFeature->SetFrom( this->data[row] );
        if (poFeature != NULL){   
            if(bForceToPoint) {   
                poFeature->SetGeometryDirectly(
					this->data[row]->StealGeometry() );
            }   
			else if( bForceToPolygon ) {
                poFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToPolygon(
						this->data[row]->StealGeometry() ) );
            }
            else if( bForceToMultiPolygon ) {   
                poFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToMultiPolygon(
						this->data[row]->StealGeometry() ) );
            }   
            else if ( bForceToMultiLineString ){   
                poFeature->SetGeometryDirectly(
					OGRGeometryFactory::forceToMultiLineString(
						this->data[row]->StealGeometry() ) );
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
    GDALClose(poODS);
}
