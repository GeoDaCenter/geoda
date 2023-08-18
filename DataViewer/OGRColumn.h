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

#ifndef __GEODA_CENTER_OGR_COLUMN_H__
#define __GEODA_CENTER_OGR_COLUMN_H__

#include <vector>
#include <map>
#include <iostream>
#include <boost/date_time.hpp>

#include "../GdaConst.h"
#include "../DataViewer/VarOrderPtree.h"
#include "../DataViewer/VarOrderMapper.h"
#include "../ShapeOperations/OGRLayerProxy.h"

namespace bt = boost::posix_time;

/**
 *
 */
class OGRColumn
{
protected:
    wxString name;
    int  idx; // idx in ogr_layer
    int  length;
    int  decimals;
    bool is_new;
    bool is_deleted;
    int  rows;
    OGRLayerProxy* ogr_layer;
    // markers for a new column if the cell has ben assigned a value
    std::vector<bool> undef_markers;
    int get_date_format(std::string& s);
    
public:
    // Constructor for in-memory column
    OGRColumn(wxString name, int field_length, int decimals, int n_rows);
    OGRColumn(OGRLayerProxy* _ogr_layer,
              wxString name, int field_length, int decimals);
    OGRColumn(OGRLayerProxy* _ogr_layer, int idx);
    virtual ~OGRColumn(){}
   
    // Get column index from loaded ogr_layer
    int GetColIndex();
   
    void SetUndefinedMarkers(std::vector<bool>& undefs) {undef_markers = undefs;}
    std::vector<bool> GetUndefinedMarkers() { return undef_markers;}
    
    //  When SaveAs current datasource to a new datasource, the underneath OGRLayer will be replaced.
    void UpdateOGRLayer(OGRLayerProxy* new_ogr_layer);
    
    OGRLayerProxy* GetOGRLayerProxy() { return ogr_layer;}
    
    bool IsNewColumn() { return is_new;}
    
    void SetInserted() { is_new = false; }
    
    bool IsCellUpdated(int row);
    
    bool IsDeleted() { return is_deleted;}
    
    void SetDeletion(bool is_delete) { is_deleted = is_delete;}
    
    wxString GetName();
    void Rename(const wxString& new_name);
    
    int GetLength();
    void SetLength(int new_length);
    
    int GetDecimals();
    void SetDecimals(int new_decimals);
    
    int GetNumRows() { return rows; }
    
    // only for adding new column, and rollback() operation
    //void SetColIndex(int idx) { col_idx = idx;}
    // virtual functions that need to be overwritten
    virtual bool IsUndefined(int row);
    virtual GdaConst::FieldType GetType() {return GdaConst::unknown_type;}
    
    
    virtual void UpdateData(const std::vector<wxString>& data);
    virtual void UpdateData(const std::vector<wxInt64>& data);
    virtual void UpdateData(const std::vector<double>& data);
    virtual void UpdateData(const std::vector<unsigned long long>& data);
   
    // following UpdateData will be used for any undefined/null values
    virtual void UpdateData(const std::vector<double>& data,
                            const std::vector<bool>& undef_markers);
    virtual void UpdateData(const std::vector<wxInt64>& data,
                            const std::vector<bool>& undef_markers);
    virtual void UpdateData(const std::vector<wxString>& data,
                            const std::vector<bool>& undef_markers);
    virtual void UpdateData(const std::vector<unsigned long long>& data,
                            const std::vector<bool>& undef_markers);
    
    // Should return true, unless if a undefined value is found
	virtual bool GetCellValue(int row, wxInt64& val);
	virtual bool GetCellValue(int row, double& val);
	virtual bool GetCellValue(int row, wxString& val);
    
    virtual void UpdateNullMarkers(const std::vector<bool>& set_markers);
    
    // interfaces for TableInterface
    virtual void FillData(std::vector<double>& data);
    virtual void FillData(std::vector<wxInt64>& data);
    virtual void FillData(std::vector<wxString>& data,
                          wxCSConv* m_wx_encoding = NULL);
    virtual void FillData(std::vector<unsigned long long>& data);
    
    virtual void FillData(std::vector<double>& data,
                          std::vector<bool>& undef_markers);
    virtual void FillData(std::vector<wxInt64>& data,
                          std::vector<bool>& undef_markers);
    virtual void FillData(std::vector<wxString>& datam,
                          std::vector<bool>& undef_markers,
                          wxCSConv* m_wx_encoding = NULL);
    virtual void FillData(std::vector<unsigned long long>& datam,
                          std::vector<bool>& undef_markers);
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL) = 0;
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL) = 0;
    virtual void SetValueAt(int row_idx, wxInt64 value){}
    virtual void SetValueAt(int row_idx, double value){}
};

/**
 *
 */
class OGRColumnInteger : public OGRColumn
{
private:
    std::vector<wxInt64> new_data;
    void InitMemoryData();
    
public:
    OGRColumnInteger(wxString name, int field_length, int decimals, int n_rows);
    
    OGRColumnInteger(OGRLayerProxy* ogr_layer,
                     wxString name, int field_length, int decimals);
    
    OGRColumnInteger(OGRLayerProxy* ogr_layer, int idx);
    
    ~OGRColumnInteger();
    
    virtual GdaConst::FieldType GetType() {return GdaConst::long64_type;}
    
    virtual void FillData(std::vector<double>& data);
    
    virtual void FillData(std::vector<wxInt64>& data);
                          
    virtual void FillData(std::vector<wxString>& data, wxCSConv* m_wx_encoding=NULL);
    
    virtual void UpdateData(const std::vector<wxInt64>& data);
    
    virtual void UpdateData(const std::vector<double>& data);
    
	virtual bool GetCellValue(int row, wxInt64& val);
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);
    
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL);
    void SetValueAt(int row_idx, wxInt64 value);
    
};

/**
 *
 */
class OGRColumnDouble : public OGRColumn
{
private:
    std::vector<double> new_data;
    void InitMemoryData();
    
public:
    OGRColumnDouble(wxString name, int field_length, int decimals, int n_rows);
    
    OGRColumnDouble(OGRLayerProxy* ogr_layer,
                    wxString name, int field_length, int decimals);
    
    OGRColumnDouble(OGRLayerProxy* ogr_layer, int idx);
    
    ~OGRColumnDouble();
    
    virtual GdaConst::FieldType GetType() {return GdaConst::double_type;}
    
    virtual void FillData(std::vector<double>& data);
    
    virtual void FillData(std::vector<wxInt64>& data);
    
    virtual void FillData(std::vector<wxString>& data, wxCSConv* m_wx_encodin = NULL);
    
    virtual void UpdateData(const std::vector<wxInt64>& data);
    
    virtual void UpdateData(const std::vector<double>& data);
    
	virtual bool GetCellValue(int row, double& val);
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);
    
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL);
    void SetValueAt(int row_idx, double value);
    
};

/**
 *
 */
class OGRColumnString : public OGRColumn
{
private:
    std::vector<wxString> new_data;
    
    void InitMemoryData();
    
    
public:
    OGRColumnString(wxString name, int field_length, int decimals, int n_rows);
    
    OGRColumnString(OGRLayerProxy* ogr_layer,
                    wxString name, int field_length, int decimals);
    
    OGRColumnString(OGRLayerProxy* ogr_layer, int idx);
    
    ~OGRColumnString();
    
    virtual GdaConst::FieldType GetType() {return GdaConst::string_type;}
    
    virtual void FillData(std::vector<double>& data);
    
    virtual void FillData(std::vector<wxInt64>& data);
    
    virtual void FillData(std::vector<wxString>& data, wxCSConv* m_wx_encodin = NULL);
    
    virtual void FillData(std::vector<unsigned long long>& data);
    
    virtual void UpdateData(const std::vector<wxString>& data);
    
    virtual void UpdateData(const std::vector<wxInt64>& data);
    
    virtual void UpdateData(const std::vector<double>& data);
    
	virtual bool GetCellValue(int row, wxString& val);
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);

    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding);
};

/**
 *
 */
class OGRColumnDate: public OGRColumn
{
protected:
    std::vector<unsigned long long> new_data;
    void InitMemoryData();
    
public:
    //OGRColumnDate(int rows);
    OGRColumnDate(OGRLayerProxy* ogr_layer, int idx);
    OGRColumnDate(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals);
    virtual ~OGRColumnDate();
    
    virtual void FillData(std::vector<wxInt64>& data);
    
    virtual void FillData(std::vector<wxString>& data, wxCSConv* m_wx_encoding = NULL);
    
    virtual void FillData(std::vector<unsigned long long>& data);
  
    virtual void UpdateData(const std::vector<unsigned long long>& data);
    
    virtual GdaConst::FieldType GetType() {return GdaConst::date_type;}
    
	virtual bool GetCellValue(int row, wxInt64& val);
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);
    
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL);
};

class OGRColumnTime: public OGRColumnDate
{
public:
    OGRColumnTime(OGRLayerProxy* ogr_layer, int idx);
    OGRColumnTime(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals);
    virtual ~OGRColumnTime();
    
    virtual GdaConst::FieldType GetType() {return GdaConst::time_type;}
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);
 
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL);
};

class OGRColumnDateTime: public OGRColumnDate
{
public:
    OGRColumnDateTime(OGRLayerProxy* ogr_layer, int idx);
    OGRColumnDateTime(OGRLayerProxy* ogr_layer, wxString name, int field_length, int decimals);
    virtual ~OGRColumnDateTime();
    
    virtual GdaConst::FieldType GetType() {return GdaConst::datetime_type;}
    
    virtual wxString GetValueAt(int row_idx,
                                int disp_decimals=0,
                                wxCSConv* m_wx_encoding=NULL);
    
    virtual void SetValueAt(int row_idx, const wxString& value,
                            wxCSConv* m_wx_encoding=NULL);
};
#endif
