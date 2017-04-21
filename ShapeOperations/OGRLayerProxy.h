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

#ifndef __GEODA_CENTER_OGR_LAYER_PROXY_H__
#define __GEODA_CENTER_OGR_LAYER_PROXY_H__

#include <sstream> 
#include <string>
#include <vector>
#include <ogrsf_frmts.h>
#include <wx/string.h>

// This is for Shapfile/DBF direct operation
#include "../DataViewer/TableInterface.h"
#include "../ShpFile.h"
#include "../GdaShape.h"
#include "../GdaException.h"
#include "OGRFieldProxy.h"
#include "OGRLayerProxy.h"


using namespace std;

/**
 * A threaded proxy class for OGR layer. It will read all meta information, such
 * as field properties, and data from OGR data soruce.
 *
 * Note: OGR read data source row by row. But the data will be stored column
 * by column, so that it can be used by OGRTable and wxGrid easily.
 */
class OGRLayerProxy {
public:
	OGRLayerProxy(std::string layer_name,
                  OGRLayer* _layer,
                  GdaConst::DataSourceType _ds_type,
                  bool isNew=false);
    
    OGRLayerProxy(OGRLayer* _layer,
                  GdaConst::DataSourceType _ds_type,
                  OGRwkbGeometryType eGType,
                  int _n_rows = 0);
    
	~OGRLayerProxy();
	
private:
    OGRFeatureDefn* featureDefn;
    OGRSpatialReference* spatialRef;
    
public:
    GdaConst::DataSourceType ds_type;
	std::ostringstream error_message;
	// progress indicator: -1 means error, otherwise means progress
	int         load_progress;
	bool        stop_reading;
	int         export_progress;
	bool        stop_exporting;
	bool        is_writable;
	std::string name;
	int			n_rows;
	int			n_cols;
	OGRLayer*	layer;
    
    //!< Geometry type of OGRLayer
    OGRwkbGeometryType eLayerType;
    //!< Fields and the meta data are stored in OGRFieldProxy.
	std::vector<OGRFieldProxy*> fields;
    //!< OGR will read data sources and store the he content in many OGRFeature
    //!< objects. The OGRLayerProxy will maintain these objects until the proxy
    //!< is dismissed.
	std::vector<OGRFeature*> data;
    //!< number of time steps.  If time_steps=1, then not time-series data
	int time_steps;
    //!< OGR layer GeomType
    OGRwkbGeometryType eGType;
    
private:
    void GetExtent(Shapefile::Main& p_main, Shapefile::PointContents* pc,
                   int row_idx);
    
    void GetExtent(Shapefile::Main& p_main, Shapefile::PolygonContents* pc,
                   int row_idx);
    
    void CopyEnvelope(OGRPolygon* p, Shapefile::PolygonContents* pc);
	
    /**
	 * Read field information and save to OGRFieldProxy array.
	 */
	bool ReadFieldInfo();
    
public:
	static OGRFieldType GetOGRFieldType(GdaConst::FieldType field_type);
    OGRwkbGeometryType  GetShapeType(){ return eGType;}
    void      SetOGRLayer(OGRLayer* new_layer);
    OGRLayer* GetOGRLayer()  { return layer; }
    int       GetNumRecords(){ return n_rows; }
    int       GetNumFields() { return n_cols; }
    bool      HasError();
    bool      GetExtent(double& minx, double& miny, double& maxx, double& maxy);
    OGRSpatialReference* GetSpatialReference() {return spatialRef;}
	/**
	 * Save() function tries to save any changes to original data source.
	 * It may return failure because the layer doesn't support writeback.
	 */
	void Save();
	/**
	 * Export current ogr layer to  layer in other ogr data source
	 * @param format exported driver name (OGR style)
	 * @param dest_datasource exported data source name (OGR style)
	 */
	void Export(string format, string dest_datasource, string new_layer_name,
                bool is_update);
	void T_Export(string format, string dest_datasource, string new_layer_name,
                  bool is_update);
	void T_StopExport();

    /**
     * Add new features to an empty OGRLayer
     * This function should be only used when create a new OGRLayer
     */
    void AddFeatures(std::vector<OGRGeometry*>& geometries,
                     TableInterface* table,
                     std::vector<int>& selected_rows);
	/**
	 * Read geometries and save to Shapefile::Main data structure.
	 */
	bool ReadGeometries(Shapefile::Main& p_main);
    bool AddGeometries(Shapefile::Main& p_main);

	/**
	 * Read table data from ogr OGRFeatures.
	 * Note: Geometries are saved as raw "wkb" format. Developer needs to call
	 * ReadGeometries() function to retrieve/phrase geometries into memory.
	 */
	bool ReadData();
    /**
     * Get OGRFieldProxy by an in put field position
     */
    OGRFieldProxy* GetField(int pos) { return fields[pos]; }
    /**
     * Get OGRFieldProxy by a input field name.
     */
    OGRFieldProxy* GetField(const wxString& field_name)
    {
        int pos = GetFieldPos(field_name);
        return fields[pos];
    }
	/**
	 * AddField() function tries to add a new field in OGRLayer.
	 * NOTE: this may has no impact on orignal data source. It only works in 
	 * memory. If ogr data source doesn't support add field, user can "export"
	 * the new field(s) in memory to a new file.
	 */
	int  AddField(const wxString& field_name, GdaConst::FieldType field_type,
				  int field_length, int field_precision);
	/**
	 *
	 */
	void UpdateFieldProperties(int col);
    /**
	 * Get field name by an input field position.
	 */
	wxString GetFieldName(int pos);
    /**
	 * Set field name at an input field position.
	 */
    void SetFieldName(int pos, const wxString& new_fname);
	/**
	 *
	 */
	void DeleteField(int pos);
	/**
	 *
	 */
	void DeleteField(const wxString& field_name);
	/**
	 *
	 */
	int GetFieldPos(const wxString& field_name);
    /**
     */
	GdaConst::FieldType GetFieldType(int pos);
    /**
     */
    GdaConst::FieldType GetFieldType(const wxString& field_name);
	/**
	 *
	 */
	int GetFieldLength(int pos);
    void SetFieldLength(int pos, int new_len);
	/**
	 *
	 */
	int GetFieldDecimals(int pos);
    void SetFieldDecimals(int pos, int new_dec);
	/**
	 *
	 */
	bool UpdateColumn();
    bool UpdateColumn(int col_idx, vector<double> &vals);
    bool UpdateColumn(int col_idx, vector<wxInt64> &vals);
    bool UpdateColumn(int col_idx, vector<wxString> &vals);
    
	/**
	 *
	 */
	bool IsTableOnly();
	/**
	 *
	 */
	bool UpdateOGRFeature(OGRFeature* feature);
	/**
	 *
	 */
	bool AppendOGRFeature(std::vector<std::string>& content);
	/**
	 *
	 */
	bool InsertOGRFeature();
	
	/**
	 * var_list:  variable/column/field list
	 * var_type_map: variable/column/field -- field type
	 */
	void GetVarTypeMap(std::vector<wxString>& var_list,
					   std::map<wxString, GdaConst::FieldType>& var_type_map);
	/**
	 *
	 */
    OGRFeature* GetFeatureAt(int rid) { return data[rid];}
    
    bool IsUndefined(int rid, int cid)
    {
        return !data[rid]->IsFieldSet(cid);
    }
    
	wxString GetValueAt(int rid, int cid)
    {
        wxString rst(data[rid]->GetFieldAsString(cid));
        return rst;
    }
    
    void SetValueAt(int rid, int cid, GIntBig val)
    {
        data[rid]->SetField( cid, val);
        if (layer->SetFeature(data[rid]) != OGRERR_NONE){
            throw GdaException(wxString("Set value to cell failed.").mb_str());
        }
    }
    
    void SetValueAt(int rid, int cid, double val)
    {
        data[rid]->SetField( cid, val);
        if (layer->SetFeature(data[rid]) != OGRERR_NONE){
            throw GdaException(wxString("Set value to cell failed.").mb_str());
        }
    }
    
    void SetValueAt(int rid, int cid, int year, int month, int day)
    {
        data[rid]->SetField( cid, year, month, day);
        if (layer->SetFeature(data[rid]) != OGRERR_NONE){
            throw GdaException(wxString("Set value to cell failed.").mb_str());
        }
    }
    
    void SetValueAt(int rid, int cid, int year, int month, int day, int hour, int minute, int second)
    {
        data[rid]->SetField( cid, year, month, day, hour, minute, second);
        if (layer->SetFeature(data[rid]) != OGRERR_NONE){
            throw GdaException(wxString("Set value to cell failed.").mb_str());
        }
    }
    
    void SetValueAt(int rid, int cid, const char* val, bool is_new=true)
    {
        data[rid]->SetField( cid, val);
        if (layer->SetFeature(data[rid]) != OGRERR_NONE){
            throw GdaException(wxString("Set value to cell failed.").mb_str());
        }
    }
    
private:
	bool IsFieldExisted(const wxString& field_name);
    
    bool CallCartoDBAPI(wxString url);
};

#endif
