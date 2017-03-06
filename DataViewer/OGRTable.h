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

#ifndef __GEODA_CENTER_OGR_TABLE_BASE_H__
#define __GEODA_CENTER_OGR_TABLE_BASE_H__

#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <wx/filename.h>

#include "OGRColumn.h"
#include "OGRTableOperation.h"
#include "TableInterface.h"
#include "../DataViewer/VarOrderPtree.h"
#include "../DataViewer/VarOrderMapper.h"
#include "../ShapeOperations/OGRLayerProxy.h"

using namespace std;

class OGRTable : public TableInterface, TableStateObserver
{
public:
	OGRTable(OGRLayerProxy* _ogr_layer, GdaConst::DataSourceType ds_type,
             TableState* table_state, TimeState* time_state,
             const VarOrderPtree& var_order_ptree);
    
    OGRTable(int n_rows);
    
    virtual ~OGRTable();

private:
    GdaConst::DataSourceType datasource_type;
    OGRLayerProxy* ogr_layer;
    vector<OGRColumn*> columns;
	VarOrderMapper var_order;
    
    // var_map will be deprecate in 1.8.8, and replace by _var_names
	map<wxString, int> var_map;
    vector<wxString> org_var_names;
    vector<wxString> new_var_names;
    
    // queues of table operations
    queue<OGRTableOperation*> operations_queue;
    stack<OGRTableOperation*> completed_stack;
	
private:
	void AddTimeIDs(int n);
	int  FindOGRColId(int wxgrid_col_pos, int time);
    int  FindOGRColId(const wxString& name);
	OGRColumn* FindOGRColumn(int col, int time=0);
    OGRColumn* FindOGRColumn(const wxString& name);
    
    void AddOGRColumn(OGRLayerProxy* ogr_layer_proxy, int idx);
	
public:
    /** Implementation of TableStateObserver interface */
    virtual void update(TableState* o);
    virtual bool AllowTimelineChanges() { return true; }
    virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
    virtual bool AllowObservationAddDelete() { return true; }
    
    // Public methods specific to OGRTable
	OGRLayerProxy* GetOGRLayer() { return ogr_layer; }
    //void ChangeOGRLayer(OGRLayerProxy* new_ogr_layer);

public:
    // These functions for in-memory table
    void AddOGRColumn(OGRColumn* ogr_col);
    OGRColumn* GetOGRColumn(int idx);
    
    
	// Implementation of TableInterface pure virtual methods
    virtual void Update(const VarOrderPtree& var_order_ptree);
    
	virtual GdaConst::DataSourceType GetDataSourceType();
	virtual void GetTimeStrings(std::vector<wxString>& tm_strs);
	virtual void GetColNonPlaceholderTmStrs(int col,
											std::vector<wxString>& tm_strs);
	virtual wxString GetTimeString(int time);
	virtual int GetTimeInt(const wxString& tm_string);
	virtual bool IsTimeVariant();
	virtual int GetTimeSteps();	
	virtual wxString GetTableName();
	
	virtual bool Save(wxString& err_msg);
	
	virtual bool IsReadOnly();
	virtual bool PermitRenameSimpleCol();
	virtual bool HasFixedLengths();
	virtual bool PermitChangeLength();
	virtual bool HasFixedDecimals();
	virtual bool PermitChangeDecimals();
	virtual bool PermitChangeDisplayedDecimals();
	
	virtual bool DbColNmToColAndTm(const wxString& name, int& col, int& tm);
	virtual int  FindColId(const wxString& name);
    virtual int  GetColIdx(const wxString& name, bool ignore_case=false);
	virtual void FillColIdMap(std::vector<int>& col_map);
	virtual void FillNumericColIdMap(std::vector<int>& col_map);
	virtual void FillIntegerColIdMap(std::vector<int>& col_map);
	virtual void FillNumericNameList(std::vector<wxString>& num_names);
	virtual int  GetNumberCols();
	virtual int  GetNumberRows();
	virtual bool IsColTimeVariant(int col);
	virtual int  GetColTimeSteps(int col);
	virtual bool IsColNumeric(int col);
	virtual GdaConst::FieldType GetColType(int col);
	virtual std::vector<GdaConst::FieldType> GetColTypes(int col);
	virtual GdaConst::FieldType GetColType(int col, int time);
	virtual bool DoesNameExist(const wxString& name, bool case_sensitive) const;
	virtual wxString GetColName(int col);
	virtual wxString GetColName(int col, int time);
	virtual int  GetColLength(int col, int time=0);
	virtual int  GetColDecimals(int col, int time=0);
	virtual int  GetColDispDecimals(int col);
	virtual void GetColData(int col, GdaFlexValue& data);
	virtual void GetColData(int col, d_array_type& data);
	virtual void GetColData(int col, l_array_type& data);
	virtual void GetColData(int col, s_array_type& data);
	virtual void GetColData(int col, int time, std::vector<double>& data);
	virtual void GetColData(int col, int time, std::vector<wxInt64>& data);
	virtual void GetColData(int col, int time, std::vector<wxString>& data);
    
	virtual void GetDirectColData(int col, std::vector<double>& data);
	virtual void GetDirectColData(int col, std::vector<wxInt64>& data);
	virtual void GetDirectColData(int col, std::vector<wxString>& data);
	virtual bool GetDirectColUndefined(int col, std::vector<bool>& undefined);
    
	virtual bool GetColUndefined(int col, b_array_type& undefined);
	virtual bool GetColUndefined(int col, int time,
								 std::vector<bool>& undefined);
	virtual void GetMinMaxVals(int col, std::vector<double>& min_vals,
							   std::vector<double>& max_vals);
	virtual void GetMinMaxVals(int col, int time,
							   double& min_val, double& max_val);
	virtual void SetColData(int col, int time,
                            const std::vector<double>& data);
	virtual void SetColData(int col, int time,
                            const std::vector<wxInt64>& data);
	virtual void SetColData(int col, int time,
                            const std::vector<wxString>& data);
	virtual void SetColUndefined(int col, int time,
								 const std::vector<bool>& undefined);
	virtual bool ColChangeProperties(int col, int time,
									 int new_len, int new_dec=0);
	virtual bool ColChangeDisplayedDecimals(int col, int new_disp_dec);
	virtual bool RenameGroup(int col, const wxString& new_name);
	virtual bool RenameSimpleCol(int col, int time, const wxString& new_name);
	virtual wxString GetCellString(int row, int col, int time=0);
	virtual bool SetCellFromString(int row, int col, int time,
								   const wxString &value);
	virtual int  InsertCol(GdaConst::FieldType type, const wxString& name,
						   int pos=-1, int time_steps=1,
						   int field_len=-1, int decimals=-1);
	virtual bool DeleteCol(int pos);
	virtual void UngroupCol(int col);
	virtual void GroupCols(const vector<int>& cols,
						   const wxString& name, int pos=0);
	virtual void InsertTimeStep(int time, const wxString& name);
	virtual void RemoveTimeStep(int time);
	virtual void SwapTimeSteps(int time1, int time2);
	virtual void RenameTimeStep(int time, const wxString& new_name);
	virtual bool IsValidDBColName(const wxString& col_nm,
								  wxString* fld_warn_msg=0);
};

#endif
