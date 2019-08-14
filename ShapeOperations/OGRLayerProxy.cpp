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
#include <time.h>
#include <vector>
#include <ogrsf_frmts.h>
#include <climits>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>
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
using namespace boost;
namespace bt = boost::posix_time;

/**
 * Create a OGRLayerProxy from an existing OGRLayer
 */
OGRLayerProxy::OGRLayerProxy(wxString layer_name,
                             OGRLayer* _layer,
                             GdaConst::DataSourceType _ds_type,
                             bool isNew)
: mapContour(0), n_rows(0), n_cols(0), name(layer_name),ds_type(_ds_type),
layer(_layer), load_progress(0), stop_reading(false), export_progress(0)
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
                             OGRwkbGeometryType _eGType,
                             int _n_rows)
: mapContour(0), layer(_layer), name(_layer->GetName()), ds_type(_ds_type),
n_rows(_n_rows), eGType(_eGType), load_progress(0), stop_reading(false),
export_progress(0)
{
    if (n_rows == 0) {
        // sometimes the OGR returns 0 features (falsely)
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
    if (mapContour) {
        mapContour->empty();
        delete mapContour;
    }
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

bool OGRLayerProxy::IsFieldCaseSensitive(GdaConst::DataSourceType ds_type)
{
    if (ds_type == GdaConst::ds_sqlite ||
        ds_type == GdaConst::ds_gpkg || // based on sqlite
        ds_type == GdaConst::ds_mysql || // on windows only
        ds_type == GdaConst::ds_postgresql || // old version
        ds_type == GdaConst::ds_cartodb // based on postgresql
    ) {
        return false;
    }
    return true;
}
OGRwkbGeometryType OGRLayerProxy::GetShapeType()
{
    return eGType;
}

OGRLayer* OGRLayerProxy::GetOGRLayer()
{
    return layer;
}

int OGRLayerProxy::GetNumRecords()
{
    return n_rows;
}

int OGRLayerProxy::GetNumFields()
{
    return n_cols;
}

OGRSpatialReference* OGRLayerProxy::GetSpatialReference()
{
    return spatialRef;
}

void OGRLayerProxy::SetOGRLayer(OGRLayer* new_layer)
{
    layer = new_layer;
}

bool OGRLayerProxy::ReadFieldInfo()
{
    // check duplicated field names
    std::set<wxString> field_nms;
    for (int col_idx=0; col_idx < n_cols; col_idx++) {
        OGRFieldDefn *fieldDefn = featureDefn->GetFieldDefn(col_idx);
        field_nms.insert(fieldDefn->GetNameRef());
    }
    if (field_nms.size() < n_cols) {
        wxString msg = _("GeoDa can't load dataset with duplicate field names.");
        error_message << msg;
        throw GdaException(msg.mb_str());
    }
    
	for (int col_idx=0; col_idx<n_cols; col_idx++) {
		OGRFieldDefn *fieldDefn = featureDefn->GetFieldDefn(col_idx);
		OGRFieldProxy *fieldProxy = new OGRFieldProxy(fieldDefn);
		this->fields.push_back(fieldProxy);
	}
	return true;
}

vector<GdaConst::FieldType> OGRLayerProxy::GetFieldTypes()
{
    vector<GdaConst::FieldType> field_types;
	for( int i=0; i< n_cols; i++) {
		field_types.push_back(fields[i]->GetType());
	}
    return field_types;
}

vector<wxString> OGRLayerProxy::GetFieldNames()
{
    vector<wxString> field_names;
    for( int i=0; i< n_cols; i++) {
        wxString var = fields[i]->GetName();
        field_names.push_back(var);
    }
    return field_names;
}

wxString OGRLayerProxy::GetFieldName(int pos)
{
	return fields[pos]->GetName();
}

void OGRLayerProxy::SetFieldName(int pos, const wxString& new_fname)
{
    fields[pos]->SetName(new_fname, IsFieldCaseSensitive(ds_type));
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
    bool case_sensitive = IsFieldCaseSensitive(ds_type);
	for (size_t i=0, iend=fields.size(); i<iend; ++i) {
        wxString fname = fields[i]->GetName();
		if (fname.IsSameAs(field_name, case_sensitive))
            return i;
	}
    return -1;
}

OGRFieldProxy* OGRLayerProxy::GetField(int pos)
{
    return fields[pos];
}

OGRFieldProxy* OGRLayerProxy::GetField(const wxString& field_name)
{
    int pos = GetFieldPos(field_name);
    return fields[pos];
}

OGRFeature* OGRLayerProxy::GetFeatureAt(int rid)
{
    return data[rid];
}

bool OGRLayerProxy::IsUndefined(int rid, int cid)
{
    return !data[rid]->IsFieldSet(cid);
}

wxString OGRLayerProxy::GetValueAt(int rid, int cid, wxCSConv* m_wx_encoding)
{
    wxString rst;
    if (m_wx_encoding == NULL) {
        // following GDAL/OGR using UTF8 to read table data,
        // if no custom encoding specified
        rst = wxString(data[rid]->GetFieldAsString(cid), wxConvUTF8);
    } else {
        rst = wxString(data[rid]->GetFieldAsString(cid), *m_wx_encoding);
    }
    return rst;
}

void OGRLayerProxy::GetValueAt(int rid, int cid, GIntBig* val)
{
    *val = data[rid]->GetFieldAsInteger64(cid);
}

void OGRLayerProxy::GetValueAt(int rid, int cid, double* val)
{
    *val = data[rid]->GetFieldAsDouble(cid);
}

void OGRLayerProxy::SetValueAt(int rid, int cid, GIntBig val, bool undef)
{
    if (undef) data[rid]->UnsetField(cid);
    else data[rid]->SetField( cid, val);
    if (layer->SetFeature(data[rid]) != OGRERR_NONE){
        wxString msg = _("Set value to cell failed.");
        throw GdaException(msg.mb_str());
    }
}

void OGRLayerProxy::SetValueAt(int rid, int cid, double val, bool undef)
{
    if (undef) data[rid]->UnsetField(cid);
    else data[rid]->SetField( cid, val);
    if (layer->SetFeature(data[rid]) != OGRERR_NONE){
        wxString msg = _("Set value to cell failed.");
        throw GdaException(msg.mb_str());
    }
}

void OGRLayerProxy::SetValueAt(int rid, int cid, int year, int month, int day, bool undef)
{
    if (undef) data[rid]->UnsetField(cid);
    else data[rid]->SetField( cid, year, month, day);
    if (layer->SetFeature(data[rid]) != OGRERR_NONE){
        wxString msg = _("Set value to cell failed.");
        throw GdaException(msg.mb_str());
    }
}

void OGRLayerProxy::SetValueAt(int rid, int cid, int year, int month, int day, int hour, int minute, int second, bool undef)
{
    if (undef) data[rid]->UnsetField(cid);
    else data[rid]->SetField( cid, year, month, day, hour, minute, second);
    if (layer->SetFeature(data[rid]) != OGRERR_NONE) {
        wxString msg = _("Set value to cell failed.");
        throw GdaException(msg.mb_str());
    }
}

void OGRLayerProxy::SetValueAt(int rid, int cid, const char* val, bool is_new, bool undef)
{
    if (undef) data[rid]->UnsetField(cid);
    else data[rid]->SetField( cid, val);
    if (layer->SetFeature(data[rid]) != OGRERR_NONE){
        wxString msg = _("Set value to cell failed.");
        throw GdaException(msg.mb_str());
    }
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
    bool case_sensitive = IsFieldCaseSensitive(ds_type);
	vector<OGRFieldProxy*>::iterator it;
	for (it = fields.begin(); it!=fields.end(); it++){
        wxString name = (*it)->GetName();
		if (field_name.IsSameAs(name)){
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
        wxString tmp = _("Change field properties (%s) failed.\n\nDetails: %s");
        wxString msg = wxString::Format(tmp, name, CPLGetLastErrorMsg());
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
		wxString tmp = _("Field (%s) already exited.");
        wxString msg = wxString::Format(tmp, field_name);
		throw GdaException(msg.mb_str());
	}
	OGRFieldType  ogr_type = GetOGRFieldType(field_type);
	OGRFieldProxy *oField = new OGRFieldProxy(field_name, ogr_type, 
											  field_length, field_precision);
	if ( layer->CreateField( oField->GetFieldDefn() ) != OGRERR_NONE ) {
        wxString tmp = _("Internal Error: Add new field (%s) failed.\n\nDetails:%s");
        wxString msg = wxString::Format(tmp, field_name, CPLGetLastErrorMsg());
		throw GdaException(msg.mb_str());
	}					
	n_cols++;
	// Add this new field to OGRFieldProxy
	this->fields.push_back(oField);
	return n_cols-1;
}

void OGRLayerProxy::DeleteField(int pos)
{
    // remove this field in local OGRFeature vector
    for (size_t i=0; i < data.size(); ++i) {
        OGRFeature* my_feature = data[i];
#ifdef __linux__
	// move to official gdal on linux, so no need to call DeleteField()
#else
		my_feature->DeleteField(pos);
#endif
    }
	// delete field in actual datasource
	if( this->layer->DeleteField(pos) != OGRERR_NONE ) {
        wxString msg = _("Internal Error: Delete field failed.\n\nDetails:");
		msg << CPLGetLastErrorMsg();
		throw GdaException(msg.mb_str());
	}	
	n_cols--;
	// remove this field from OGRFieldProxy
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

void OGRLayerProxy::ApplyProjection(OGRCoordinateTransformation* poCT)
{
    if ( !data.empty() ) {
        for (int i=0; i<n_rows; i++) {
            OGRGeometry* geom = data[i]->GetGeometryRef();
            if (geom != NULL) {
                geom->transform(poCT);
            }
        }
    }
}

bool OGRLayerProxy::UpdateOGRFeature(OGRFeature* feature)
{
	if (layer->SetFeature(feature) == OGRERR_NONE)
        return true;
    return false;
}

bool OGRLayerProxy::AppendOGRFeature(vector<wxString>& content)
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
        wxString col_name = GetFieldName(col_idx);
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
        wxString col_name = GetFieldName(col_idx);
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
        wxString col_name = GetFieldName(col_idx);
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

vector<wxString> OGRLayerProxy::GetIntegerFieldNames()
{
    vector<wxString> names;
    for (int i=0; i<fields.size(); i++) {
        if (GdaConst::long64_type == fields[i]->GetType()) {
            names.push_back(GetFieldName(i));
        }
    }
    return names;
}

vector<wxString> OGRLayerProxy::GetNumericFieldNames()
{
    vector<wxString> names;
    for (int i=0; i<fields.size(); i++) {
        if (GdaConst::long64_type == fields[i]->GetType() ||
            GdaConst::double_type == fields[i]->GetType()) {
            names.push_back(GetFieldName(i));
        }
    }
    return names;
}

vector<wxString> OGRLayerProxy::GetIntegerAndStringFieldNames()
{
    vector<wxString> names;
    for (int i=0; i<fields.size(); i++) {
        if (GdaConst::long64_type == fields[i]->GetType() ||
            GdaConst::string_type == fields[i]->GetType()) {
            names.push_back(GetFieldName(i));
        }
    }
    return names;
}

Shapefile::ShapeType OGRLayerProxy::GetOGRGeometries(vector<OGRGeometry*>& geoms,
                                                OGRSpatialReference* dest_sr)
{
    OGRCoordinateTransformation *poCT = NULL;
    if (dest_sr && spatialRef) {
        poCT = OGRCreateCoordinateTransformation(spatialRef, dest_sr);
    }
    Shapefile::ShapeType shape_type;
    //read OGR geometry features
    int feature_counter =0;
    for ( int row_idx=0; row_idx < n_rows; row_idx++ ) {
        OGRFeature* feature = data[row_idx];
        OGRGeometry* geometry= feature->GetGeometryRef();
		if (geometry == NULL) {
			// in case of invalid geometry (e.g. rows with empty lat/lon in csv file)
			geoms.push_back(NULL);
			continue;
		}
        if (poCT) {
            geometry->transform(poCT);
        }
        geoms.push_back(geometry->clone());
        OGRwkbGeometryType eType = geometry ? wkbFlatten(geometry->getGeometryType()) : eGType;
        if (eType == wkbPoint) {
            shape_type = Shapefile::POINT_TYP;
        } else if (eType == wkbMultiPoint) {
            shape_type = Shapefile::POINT_TYP;
        } else if (eType == wkbPolygon || eType == wkbCurvePolygon ) {
            shape_type = Shapefile::POLYGON;
        } else if (eType == wkbMultiPolygon) {
            shape_type = Shapefile::POLYGON;
        }
    }
    return shape_type;
}

Shapefile::ShapeType OGRLayerProxy::GetGdaGeometries(vector<GdaShape*>& geoms,
                                                OGRSpatialReference* dest_sr)
{
    OGRCoordinateTransformation *poCT = NULL;
    if (dest_sr && spatialRef) {
        poCT = OGRCreateCoordinateTransformation(spatialRef, dest_sr);
    }
    Shapefile::ShapeType shape_type;
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
            shape_type = Shapefile::POINT_TYP;
            if (geometry) {
                OGRPoint* p = (OGRPoint *) geometry;
                double ptX = p->getX(), ptY = p->getY();
                if (poCT) {
                    poCT->Transform(1, &ptX, &ptY);
                }
                geoms.push_back(new GdaPoint(ptX, ptY));
            }
        } else if (eType == wkbMultiPoint) {
            shape_type = Shapefile::POINT_TYP;
            if (geometry) {
                OGRMultiPoint* mp = (OGRMultiPoint*) geometry;
                int n_geom = mp->getNumGeometries();
                for (size_t i = 0; i < n_geom; i++ )
                {
                    // only consider first point
                    OGRGeometry* ogrGeom = mp->getGeometryRef(i);
                    OGRPoint* p = static_cast<OGRPoint*>(ogrGeom);
                    double ptX = p->getX(), ptY = p->getY();
                    if (poCT) {
                        poCT->Transform(1, &ptX, &ptY);
                    }
                    geoms.push_back(new GdaPoint(ptX, ptY));
                }
            }
        } else if (eType == wkbPolygon || eType == wkbCurvePolygon ) {
            Shapefile::PolygonContents* pc = new Shapefile::PolygonContents();
            shape_type = Shapefile::POLYGON;
            if (geometry) {
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
                    if (pLinearRing) {
                        numPoints += pLinearRing->getNumPoints();
                    }
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
                        for (size_t k=0; k < pLinearRing->getNumPoints(); k++) {
                            double ptX = pLinearRing->getX(k);
                            double ptY = pLinearRing->getY(k);
                            if (poCT) {
                                poCT->Transform(1, &ptX, &ptY);
                            }
                            pc->points[i].x =  ptX;
                            pc->points[i++].y =  ptY;
                        }
                }
            }
            geoms.push_back(new GdaPolygon(pc));
        } else if (eType == wkbMultiPolygon) {
            Shapefile::PolygonContents* pc = new Shapefile::PolygonContents();
            shape_type = Shapefile::POLYGON;
            if (geometry) {
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
                            double ptX = pLinearRing->getX(k);
                            double ptY = pLinearRing->getY(k);
                            if (poCT) {
                                poCT->Transform(1, &ptX, &ptY);
                            }
                            pc->points[pidx].x =  ptX;
                            pc->points[pidx++].y =  ptY;
                        }
                    }
                }
            }
            geoms.push_back(new GdaPolygon(pc));
        } else {
            wxString msg = _("GeoDa does not support datasource with line data at this time.  Please choose a datasource with either point or polygon data.");
            throw GdaException(msg.mb_str());
        }
    }
    return shape_type;
}

void
OGRLayerProxy::AddFeatures(vector<OGRGeometry*>& geometries,
                           TableInterface* table,
                           vector<int>& selected_rows)
{
    export_progress = 0;
    stop_exporting = false;
    wxCSConv* encoding = table->GetEncoding();

    // Create features in memory first
    for (size_t i=0; i<selected_rows.size();++i) {
        if (stop_exporting) return;
        OGRFeature *poFeature = OGRFeature::CreateFeature(featureDefn);
        if ( !geometries.empty()) {
            poFeature->SetGeometryDirectly( geometries[i] );
        }
        data.push_back(poFeature);
    }
    
    int export_size = data.size();
    export_progress = export_size / 4;

    // Fill the feature with content
    if (table != NULL) {
        if (export_size == 0) export_size = table->GetNumberRows();
        export_progress = export_size / 4;
        // fields already have been created by OGRDatasourceProxy::CreateLayer()
        for (size_t j=0; j< fields.size(); j++) {
            wxString fname = fields[j]->GetName();
            GdaConst::FieldType ftype = fields[j]->GetType();
            // get underneath column position (no group and time =0)
            int col_pos = table->GetColIdx(fname);
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
                    if (stop_exporting) return;
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
                    if (stop_exporting) return;
                }
            } else if (ftype == GdaConst::date_type ||
                       ftype == GdaConst::time_type ||
                       ftype == GdaConst::datetime_type ) {
                
                vector<unsigned long long> col_data;
                table->GetDirectColData(col_pos, col_data);
                table->GetDirectColUndefined(col_pos, undefs);
               
                int year, month, day, hour, minute, second;
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                   
                    year = col_data[ orig_id ] / 10000000000;
                    month = (col_data[ orig_id ] % 10000000000) / 100000000;
                    day = (col_data[ orig_id ] % 100000000) / 1000000;
                    hour = (col_data[ orig_id ] % 1000000) / 10000;
                    minute = (col_data[ orig_id ] % 10000) / 100;
                    second = col_data[ orig_id ] % 100;
                    
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
                        undefs[m] = false; // no undefs in csv file
                        if (col_data[m].IsEmpty())
                            col_data[m] = " ";
                    }
                }
                for (size_t k=0; k<selected_rows.size();++k) {
                    int orig_id = selected_rows[k];
                    
                    if (undefs[orig_id]) {
                        data[k]->UnsetField(j);
                    } else {
                        char* val = NULL;
                        if (encoding == NULL)
                            val = (char*)col_data[orig_id].mb_str().data();
                        else
                            val = (char*)col_data[orig_id].mb_str(*encoding).data();
                        data[k]->SetField(j, val);
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
        if (stop_exporting) return;
        if ( (i+1)%2 ==0 ) export_progress++;
        if( layer->CreateFeature( data[i] ) != OGRERR_NONE ) {
            wxString msg = wxString::Format(" Failed to create feature (%d/%d).\n",
                                            i + 1, n_data);
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
    return error_message.IsEmpty() == false;
}

bool OGRLayerProxy::CheckIsTableOnly()
{
    OGRGeometry* my_geom = NULL;
    if (layer) {
        layer->ResetReading();
        OGRFeature *feature = layer->GetNextFeature();
        if (feature) {
            my_geom = feature->GetGeometryRef();
        }
    }
    return my_geom == NULL;
}

bool OGRLayerProxy::ReadData()
{
	if (n_rows > 0 && n_rows == data.size()) {
        // skip if data has already been read/loaded
        return true;
    }
    if (n_rows == 0) {
        // in some case  ArcSDE plugin can't return proper row number from
        // SDE engine. we will count it feature by feature
        n_rows = -1;
    }
	int row_idx = 0;
	OGRFeature *feature = NULL;
    unordered_map<int, OGRFeature*> feature_dict;
    layer->ResetReading();
	while ((feature = layer->GetNextFeature()) != NULL) {
        // thread feature: user can stop reading
		if (stop_reading) break;
        feature_dict[row_idx] = feature;
		load_progress = row_idx++;
	}
    if (row_idx == 0) {
        error_message << _("GeoDa can't read data from datasource. \n\nDetails: Datasource is empty.");
		error_message << CPLGetLastErrorMsg();
        return false;
    }
    if (stop_reading) {
        error_message << "Reading data was interrupted.";
        // clean just read OGRFeatures
        for (int i = 0; i < row_idx; i++) {
            OGRFeature::DestroyFeature(feature_dict[i]);
        }
        return false;
    }
	n_rows = row_idx;
    // check empty rows at the end of table -- this often occurs in a csv file
    // , then remove empty rows see issue#563
    for (int i = n_rows-1; i >= 0; --i) {
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
        } else {
            // visit starts from the bottom of the table, so interupt if
            // non-empty row is detected
            break;
        }
    }
    // create copies of OGRFeatures* from OGR,
    // and manage the copies in this class
    for (int i = 0; i < n_rows; i++) {
        OGRFeature* my_feature = feature_dict[i]->Clone();
        data.push_back(my_feature);
        OGRFeature::DestroyFeature(feature_dict[i]);
    }
    // Set load_progress 100% to continue
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
                        vector<wxInt32> startIndexes = poly->pc->parts;
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
        if (layer->SetFeature(data[id]) != OGRERR_NONE) {
            return false;
        }
    }
    return true;
}

bool OGRLayerProxy::GetExtent(double& minx, double& miny,
                              double& maxx, double& maxy,
                              OGRSpatialReference* dest_sr)
{
	OGREnvelope pEnvelope;
    if (layer->GetExtent(&pEnvelope) != OGRERR_NONE) return false;
	minx = pEnvelope.MinX;
	miny = pEnvelope.MinY;
	maxx = pEnvelope.MaxX;
	maxy = pEnvelope.MaxY;
    
    OGRCoordinateTransformation *poCT = NULL;
    if (dest_sr && spatialRef) {
        poCT = OGRCreateCoordinateTransformation(spatialRef, dest_sr);
    }
    if (poCT) {
        OGRPoint pt1(minx, miny);
        pt1.transform(poCT);
        minx = pt1.getX();
        miny = pt1.getY();
        OGRPoint pt2(maxx, maxy);
        pt2.transform(poCT);
        maxx = pt2.getX();
        maxy = pt2.getY();
    }
    
    if ( minx == miny && maxx == maxy && minx == 0 && maxx==0) {
        return false;
    }
    return true;
}

void OGRLayerProxy::GetCentroids(vector<GdaPoint*>& centroids)
{
    if (centroids.size() == 0 && n_rows > 0) {
        // if centroids is empty
        double x, y;
        for ( int row_idx=0; row_idx < n_rows; row_idx++ ) {
            OGRFeature* feature = data[row_idx];
            OGRGeometry* geometry= feature->GetGeometryRef();
            if (geometry) {
                OGRPoint poPoint;
                geometry->Centroid(&poPoint);
                x = poPoint.getX();
                y = poPoint.getY();
                centroids.push_back(new GdaPoint(x, y));
            } else {
                centroids.push_back(new GdaPoint(0,0)); // no geomeetry
            }
        }
    }
}

OGRGeometry* OGRLayerProxy::GetGeometry(int idx)
{
    OGRFeature* feature = data[idx];
    OGRGeometry* geometry= feature->GetGeometryRef();
    return geometry;
}

GdaPolygon* OGRLayerProxy::DissolvePolygons(vector<OGRGeometry*>& geoms)
{
    OGRMultiPolygon geocol;
    for (size_t i=0; i < geoms.size(); i++ ) {
        OGRGeometry* geometry= geoms[i];
        OGRwkbGeometryType etype = wkbFlatten(geometry->getGeometryType());
        if (IsWkbSinglePolygon(etype)) {
            geocol.addGeometry(geometry);
        } else if (IsWkbMultiPolygon(etype)) {
            OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geometry;
            for (size_t j = 0; j < mpolygon->getNumGeometries(); j++ ){
                OGRGeometry* ogrGeom = mpolygon->getGeometryRef(j);
                geocol.addGeometry(ogrGeom);
            }
        }
    }
    OGRGeometry* ogr_contour = geocol.UnionCascaded();
    if (ogr_contour) {
        return OGRGeomToGdaShape(ogr_contour);
    }
    return NULL;
}

GdaPolygon* OGRLayerProxy::GetMapBoundary()
{
    if (mapContour == NULL) {
        OGRMultiPolygon geocol;
        for (size_t row_idx=0; row_idx < n_rows; row_idx++ ) {
            OGRFeature* feature = data[row_idx];
            OGRGeometry* geometry= feature->GetGeometryRef();
            OGRwkbGeometryType etype = wkbFlatten(geometry->getGeometryType());
            if (IsWkbSinglePolygon(etype)) {
                geocol.addGeometry(geometry);
                
            } else if (IsWkbMultiPolygon(etype)) {
                OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geometry;
                int n_geom = mpolygon->getNumGeometries();
                // if there is more than one polygon, then we need to count 
                // which part is processing accumulatively
                for (size_t i = 0; i < n_geom; i++ ){
                    OGRGeometry* ogrGeom = mpolygon->getGeometryRef(i);
                    geocol.addGeometry(ogrGeom);
                }
            }
        }
        mapContour = geocol.UnionCascaded();
    }
    
    if (mapContour) {
        return OGRGeomToGdaShape(mapContour);
    }
    return NULL;
}

std::vector<GdaShape*> OGRLayerProxy::DissolveMap(const std::map<wxString, std::vector<int> >& cids)
{
    std::vector<GdaShape*> results;

    if (data.empty()) return results;

    if (IsWkbPoint(eGType) || IsWkbLine(eGType)) return results;

    std::map<wxString, std::vector<int> >::const_iterator it;
    for (it = cids.begin(); it != cids.end(); ++it) {
        std::vector<OGRGeometry*> geom_set;
        for (size_t j=0; j<it->second.size(); j++) {
            int rid = it->second[j];
            OGRFeature* feature = data[rid];
            OGRGeometry* geometry= feature->GetGeometryRef();
            geom_set.push_back(geometry);
        }
        GdaPolygon* new_poly = DissolvePolygons(geom_set);
        results.push_back((GdaShape*)new_poly);
    }
    return results;
}

bool OGRLayerProxy::IsWkbSinglePolygon(OGRwkbGeometryType etype)
{
    if (etype == wkbPolygon || etype == wkbCurvePolygon ||
        etype == wkbPolygon25D || etype == wkbCurvePolygonZ ) {
        return true;
    }
    return false;
}

bool OGRLayerProxy::IsWkbMultiPolygon(OGRwkbGeometryType etype)
{
    if (etype == wkbMultiPolygon || etype == wkbMultiPolygon25D ) {
        return true;
    }
    return false;
}

bool OGRLayerProxy::IsWkbPoint(OGRwkbGeometryType etype)
{
    if (etype == wkbPoint || etype == wkbMultiPoint ) {
        return true;
    }
    return false;
}

bool OGRLayerProxy::IsWkbLine(OGRwkbGeometryType etype)
{
    if (etype == wkbLineString || etype == wkbMultiLineString ) {
        return true;
    }
    return false;
}

GdaPolygon* OGRLayerProxy::OGRGeomToGdaShape(OGRGeometry* geom)
{
    OGRwkbGeometryType eType = wkbFlatten(geom->getGeometryType());
    Shapefile::PolygonContents* pc = new Shapefile::PolygonContents();
    if (IsWkbSinglePolygon(eType)) {
        pc->shape_type = Shapefile::POLYGON;
        OGRPolygon* p = (OGRPolygon *)geom;
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
            pLinearRing = j==0 ? p->getExteriorRing() : p->getInteriorRing(j-1);
            if (pLinearRing) {
                for (size_t k=0; k < pLinearRing->getNumPoints(); k++) {
                    pc->points[i].x =  pLinearRing->getX(k);
                    pc->points[i++].y =  pLinearRing->getY(k);
                }
            }
        }
    } else if (IsWkbMultiPolygon(eType)) {
        pc->shape_type = Shapefile::POLYGON;
        OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geom;
        int n_geom = mpolygon->getNumGeometries();
        // if there is more than one polygon, then we need to count which
        // part is processing accumulatively
        int part_idx = 0, numPoints = 0;
        OGRLinearRing* pLinearRing = NULL;
        int pidx =0;
        for (size_t i = 0; i < n_geom; i++ ){
            OGRGeometry* ogrGeom = mpolygon->getGeometryRef(i);
            OGRPolygon* p = static_cast<OGRPolygon*>(ogrGeom);
            // number of interior rings + 1 exterior ring
            int ni_rings = p->getNumInteriorRings() + 1;
            pc->num_parts += ni_rings;
            pc->parts.resize(pc->num_parts);
            
            for (size_t j=0; j < ni_rings; j++) {
                pLinearRing = j==0 ? p->getExteriorRing() : p->getInteriorRing(j-1);
                pc->parts[part_idx++] = numPoints;
                numPoints += pLinearRing->getNumPoints();
            }
            // resize points memory
            pc->num_points = numPoints;
            pc->points.resize(pc->num_points);
            // read points
            for (size_t j=0; j < ni_rings; j++) {
                pLinearRing = j==0 ? p->getExteriorRing() : p->getInteriorRing(j-1);
                for (int k=0; k < pLinearRing->getNumPoints(); k++) {
                    pc->points[pidx].x = pLinearRing->getX(k);
                    pc->points[pidx++].y = pLinearRing->getY(k);
                }
            }
        }
    }
    return new GdaPolygon(pc);
}

bool OGRLayerProxy::ReadGeometries(Shapefile::Main& p_main)
{
    bool has_null_geometry = false;

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
		if (eGType == wkbUnknown) eGType = eType;
        
		if (eType == wkbPoint) {
			Shapefile::PointContents* pc = new Shapefile::PointContents();
			pc->shape_type = Shapefile::POINT_TYP;
            if (geometry) {
                if (feature_counter==0)
                    p_main.header.shape_type = Shapefile::POINT_TYP;
                
                OGRPoint* p = (OGRPoint *) geometry;
                if (p->IsEmpty()) {
                    pc->shape_type = Shapefile::NULL_SHAPE;
                } else {
                    pc->x = p->getX();
                    pc->y = p->getY();
                    
                    if (noExtent)
                        GetExtent(p_main, pc, row_idx);
                }
            } else {
                has_null_geometry = true;
                pc->shape_type = Shapefile::NULL_SHAPE;
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
					if (noExtent) GetExtent(p_main, pc, row_idx);
				}
            } else {
                has_null_geometry = true;
                pc->shape_type = Shapefile::NULL_SHAPE;
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
                        for (size_t k=0; k < pLinearRing->getNumPoints(); k++) {
                            pc->points[i].x =  pLinearRing->getX(k);
                            pc->points[i++].y =  pLinearRing->getY(k);
                        }
                }
                if (noExtent) GetExtent(p_main, pc, row_idx);
            } else {
                has_null_geometry = true;
                pc->shape_type = Shapefile::NULL_SHAPE;
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
                // if there is more than one polygon, then we need to count
                // which part is processing accumulatively
                int part_idx = 0, numPoints = 0;
                OGRLinearRing* pLinearRing = NULL;
                int pidx =0;
                for (size_t i = 0; i < n_geom; i++ ) {
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
                        pLinearRing = j==0 ? p->getExteriorRing() : p->getInteriorRing(j-1);
                        pc->parts[part_idx++] = numPoints;
                        numPoints += pLinearRing->getNumPoints();
                    }
                    // resize points memory					
                    pc->num_points = numPoints;
                    pc->points.resize(pc->num_points);
                    // read points
                    for (size_t j=0; j < ni_rings; j++) {
                        pLinearRing = j==0 ? p->getExteriorRing() : p->getInteriorRing(j-1);
                        for (int k=0; k < pLinearRing->getNumPoints(); k++) {
                            pc->points[pidx].x = pLinearRing->getX(k);
                            pc->points[pidx++].y = pLinearRing->getY(k);
                        }
                    }
                    if (noExtent) GetExtent(p_main, pc, row_idx);
                }
            }  else {
                has_null_geometry = true;
                pc->shape_type = Shapefile::NULL_SHAPE;
            }
			p_main.records[feature_counter++].contents_p = pc;
            
        } else {
            string open_err_msg = "GeoDa does not support datasource with line data at this time.  Please choose a datasource with either point or polygon data.";
            throw GdaException(open_err_msg.c_str());
        }
	}
    
	return has_null_geometry;
}
