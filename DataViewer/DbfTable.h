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

#ifndef __GEODA_CENTER_DBF_TABLE_BASE_H__
#define __GEODA_CENTER_DBF_TABLE_BASE_H__

#include <map>
#include <vector>
#include <wx/filename.h>
#include <wx/string.h>
#include "TableInterface.h"
#include "../DbfFile.h"
#include "../DataViewer/VarOrderPtree.h"
#include "../DataViewer/VarOrderMapper.h"

class DbfColContainer;

class DbfTable : public TableInterface
{
public:
	DbfTable(TableState* table_state, TimeState* time_state,
			 DbfFileReader& dbf, const VarOrderPtree& var_order_ptree);
	virtual ~DbfTable();
    
    virtual void update(TableState* o);
	
	// Implementation of TableInterface pure virtual methods
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
	virtual bool PermitRenameSimpleCol() { return true; }
	virtual bool HasFixedLengths() { return true; }
	virtual bool PermitChangeLength() { return true; }
	virtual bool HasFixedDecimals() { return true; }
	virtual bool PermitChangeDecimals() { return true; }
	virtual bool PermitChangeDisplayedDecimals() { return true; }
	
    virtual void Update(const VarOrderPtree& var_order_ptree);
    
	virtual bool DbColNmToColAndTm(const wxString& name, int& col, int& tm);
	virtual int FindColId(const wxString& name);
	virtual void FillColIdMap(std::vector<int>& col_map);
	virtual void FillNumericColIdMap(std::vector<int>& col_map);
	virtual void FillIntegerColIdMap(std::vector<int>& col_map);
	virtual void FillNumericNameList(std::vector<wxString>& num_names);
	virtual int GetNumberCols();
	virtual int GetNumberRows();
	virtual bool IsColTimeVariant(int col);
	virtual int GetColTimeSteps(int col);	
	virtual bool IsColNumeric(int col);
	virtual GdaConst::FieldType GetColType(int col);
	virtual std::vector<GdaConst::FieldType> GetColTypes(int col);
	virtual GdaConst::FieldType GetColType(int col, int time);
	virtual bool DoesNameExist(const wxString& name, bool case_sensitive) const;
	virtual wxString GetColName(int col);
	virtual wxString GetColName(int col, int time);
	virtual int GetColLength(int col, int time=0);
	virtual int GetColDecimals(int col, int time=0);
	virtual int GetColDispDecimals(int col);
	virtual void GetColData(int col, GdaFlexValue& data);
	virtual void GetColData(int col, d_array_type& data);
	virtual void GetColData(int col, l_array_type& data);
	virtual void GetColData(int col, s_array_type& data);
	virtual void GetColData(int col, int time, std::vector<double>& data);
	virtual void GetColData(int col, int time, std::vector<wxInt64>& data);
	virtual void GetColData(int col, int time, std::vector<wxString>& data);
	virtual void GetColUndefined(int col, b_array_type& undefined);
	virtual void GetColUndefined(int col, int time,
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
	virtual bool ColChangeProperties(int col, int time, int new_len,
									 int new_dec);
	virtual bool ColChangeDisplayedDecimals(int col, int new_disp_dec);
	
	virtual bool RenameGroup(int col, const wxString& new_name);
	virtual bool RenameSimpleCol(int col, int time, const wxString& new_name);
	
	virtual wxString GetCellString(int row, int col, int time=0);
	virtual bool SetCellFromString(int row, int col, int time,
								   const wxString &value);
	virtual int InsertCol(GdaConst::FieldType type, const wxString& name,
						  int pos=-1, int time_steps=1,
						  int field_len=-1, int decimals=-1);
	virtual bool DeleteCol(int pos);
    
	virtual void UngroupCol(int col);
	virtual void GroupCols(const std::vector<int>& cols,
						   const wxString& name, int pos=0);
	virtual void InsertTimeStep(int time, const wxString& name);
	virtual void RemoveTimeStep(int time);
	virtual void SwapTimeSteps(int time1, int time2);
	virtual void RenameTimeStep(int time, const wxString& new_name);
	virtual bool IsValidDBColName(const wxString& col_nm,
								  wxString* fld_warn_msg=0);
	
	// Public methods specific to DbfTable
	wxFileName GetDbfFileName() { return dbf_file_name; }
	void SetDbfFileName(const wxFileName& d) { dbf_file_name = d; }
		
	bool WriteToDbf(const wxString& fname, wxString& err_msg);

private:
	static void FillFieldInfoFromDesc(GdaConst::FieldInfo& fi,
									  const DbfFieldDesc& desc);
	DbfColContainer* FindDbfCol(int col, int time=0);
	void GetDbfCols(int col, std::vector<DbfColContainer*>& cols);
	void GetAllSimpleDbfCols(std::vector<DbfColContainer*>& cols);
	wxFileName dbf_file_name;
	
	VarOrderMapper var_order;
	std::map<wxString, DbfColContainer*> var_map;
	DbfFileHeader orig_header;
};


#endif
