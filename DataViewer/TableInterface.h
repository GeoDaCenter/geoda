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

#ifndef __GEODA_CENTER_TABLE_INTERFACE_H__
#define __GEODA_CENTER_TABLE_INTERFACE_H__

#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/multi_array.hpp>
#include "TableState.h"
#include "TimeState.h"
#include "TableStateObserver.h"

#include "../GdaConst.h"
#include "../VarCalc/GdaFlexValue.h"

class TimeState;
class VarOrderPtree;

typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<wxInt64, 2> l_array_type;
typedef boost::multi_array<wxString, 2> s_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class TableInterface 
{
public:
	TableInterface(TableState* table_state, TimeState* time_state);
	virtual ~TableInterface();

    virtual void update(TableState* o) = 0;
    
	virtual bool IsValid();
	virtual wxString GetOpenErrorMessage();
	
	virtual GdaConst::DataSourceType GetDataSourceType() = 0;
    
	virtual void GetTimeStrings(std::vector<wxString>& tm_strs) = 0;
	virtual void GetColNonPlaceholderTmStrs(int col,
											std::vector<wxString>& tm_strs) = 0;
	virtual wxString GetTimeString(int time) = 0;
	virtual int GetTimeInt(const wxString& tm_str);
	virtual bool IsTimeVariant() = 0;
	virtual int GetTimeSteps() = 0;
	virtual wxString GetTableName() = 0;
    
	/** Has Table data changed since last save. */
	virtual bool ChangedSinceLastSave();
	/** Indicate whether Table data is unsaved. */
	virtual void SetChangedSinceLastSave(bool chg);
    
	/** Has Table data changed since last save. */
	virtual bool ProjectChangedSinceLastSave();
	/** Indicate whether Table data is unsaved. */
	virtual void SetProjectChangedSinceLastSave(bool chg);
    
	/** Save table data to data source associated with this table.  True
	 is returned on success.  On failure false is returned along with
	 an error message in err_msg reference. */
	virtual bool Save(wxString& err_msg) = 0;

	/** Is DB read only */
	virtual bool IsReadOnly() = 0;
	/** Indicate whether DB field rename is supported.  Group names
	 are stored as meta-data, and so renaming them is always supported. */
	virtual bool PermitRenameSimpleCol() = 0;
	/** For most data sources this is false, but for dBase this is true. */
	virtual bool HasFixedLengths() = 0;
	/** Not all data sources permit field property changes.  This method
	 indicates if length property change is supported. */
	virtual bool PermitChangeLength() = 0;
	/** Only dBase supports fixed decimals places generally. */
	virtual bool HasFixedDecimals() = 0;
	/** Not all data sources permit field property changes.  This method
	 indicates if decimals property change is supported. */
	virtual bool PermitChangeDecimals() = 0;
	/** This method indicates if displayed decimals property change
	 is supported.  This is saved the project file meta-data, so should
	 generally be supported. */
	virtual bool PermitChangeDisplayedDecimals() = 0;
    
    virtual void Update(const VarOrderPtree& var_order_ptree) = 0;
    
	/** Searches for database field/col name and returns true if found
	 as well as col and time in.  If not found, returns false and col
	 and tm are set to -1 */
	virtual bool DbColNmToColAndTm(const wxString& name,
								   int& col, int& tm) = 0;
	virtual bool ColNameExists(const wxString& name);
	virtual int FindColId(const wxString& name) = 0;
	
	virtual void FillColIdMap(std::vector<int>& col_map) = 0;
	virtual void FillNumericColIdMap(std::vector<int>& col_map) = 0;
	virtual void FillIntegerColIdMap(std::vector<int>& col_map) = 0;
	virtual void FillNumericNameList(std::vector<wxString>& num_names) = 0;
	
	virtual int GetNumberCols() = 0;
	virtual int GetNumberRows() = 0;
	
	virtual bool IsColTimeVariant(const wxString& name);
	virtual bool IsColTimeVariant(int col) = 0;
	virtual int GetColTimeSteps(int col) = 0;	

	/** Returns true if name found as group or as a database column */
	virtual bool DoesNameExist(const wxString& name,
							   bool case_sensitive) const = 0;
	virtual wxString GetColName(int col) = 0; // returns group name if group
	virtual bool IsColNumeric(int col) = 0;
    
	/** Returns type for entire group, cannot be a placeholder type */
	virtual GdaConst::FieldType GetColType(int col) = 0;
	
	/** Returns types for each timestep in group.
	 Can include placeholder types */
	virtual std::vector<GdaConst::FieldType> GetColTypes(int col) = 0;
    
	/** Returns type for each specific column.  Can be a placeholder type */
	virtual GdaConst::FieldType GetColType(int col, int time) = 0;
    
	virtual wxString GetColName(int col, int time) = 0;
	virtual int GetColLength(int col, int time=0) = 0;
	virtual int GetColDecimals(int col, int time=0) = 0;
	virtual int GetColDispDecimals(int col) = 0;
	
	virtual void GetColData(int col, GdaFlexValue& data) = 0;
	virtual void GetColData(int col, d_array_type& data) = 0;
	virtual void GetColData(int col, l_array_type& data) = 0;
	virtual void GetColData(int col, s_array_type& data) = 0;
    
	virtual void GetColData(int col, int time, std::vector<double>& data) = 0;
	virtual void GetColData(int col, int time, std::vector<wxInt64>& data) = 0;
	virtual void GetColData(int col, int time, std::vector<wxString>& data) = 0;
   
	virtual void GetColData(int col, int time, std::vector<double>& data,
                            std::vector<bool>& undefs);
	virtual void GetColData(int col, int time, std::vector<wxInt64>& data,
                            std::vector<bool>& undefs);
	virtual void GetColData(int col, int time, std::vector<wxString>& data,
                            std::vector<bool>& undefs);
    
	virtual bool GetColUndefined(int col, b_array_type& undefined) = 0;
	virtual bool GetColUndefined(int col, int time,
								 std::vector<bool>& undefined) = 0;
    
    // using underneath columns, not vargroup
	virtual void GetDirectColData(int col, std::vector<double>& data) =0;
	virtual void GetDirectColData(int col, std::vector<wxInt64>& data)=0;
	virtual void GetDirectColData(int col, std::vector<wxString>& data)=0;
	virtual bool GetDirectColUndefined(int col, std::vector<bool>& undefs)=0;
    
	virtual void GetMinMaxVals(int col, std::vector<double>& min_vals,
							   std::vector<double>& max_vals) = 0;
	virtual void GetMinMaxVals(int col, int time,
							   double& min_val, double& max_val) = 0;

	virtual void SetColData(int col, int time,
							const std::vector<double>& data) = 0;
	virtual void SetColData(int col, int time,
							const std::vector<wxInt64>& data) = 0;
	virtual void SetColData(int col, int time,
							const std::vector<wxString>& data) = 0;
    
	virtual void SetColData(int col, int time,
							const std::vector<double>& data,
                            const std::vector<bool>& undefs);
	virtual void SetColData(int col, int time,
							const std::vector<wxInt64>& data,
                            const std::vector<bool>& undefs);
	virtual void SetColData(int col, int time,
							const std::vector<wxString>& data,
                            const std::vector<bool>& undefs);
    
	virtual void SetColUndefined(int col, int time,
								 const std::vector<bool>& undefined) = 0;
	
	/** Changes non-group column properties */
	virtual bool ColChangeProperties(int col, int time,
									 int new_len, int new_dec=0) = 0;
	/** Changes displayed decimals for a group or simple variable. */
	virtual bool ColChangeDisplayedDecimals(int col, int new_disp_dec=0) = 0;
	/** Renames a Group name at column or calls
	 RenameSimpleCol(col, 0, new_name) if not a group */
	virtual bool RenameGroup(int col, const wxString& new_name) = 0;
	/** Renames a simple column name.  Returns true on success. */
	virtual bool RenameSimpleCol(int col, int time,
								 const wxString& new_name) = 0;
    /** wxGrid will call this function to fill data in displayed part 
     automatically. Returns formated string (e.g. nummeric numbers) */
	virtual wxString GetCellString(int row, int col, int time=0) = 0;
	/** Attempts to set the wxGrid cell from a user-entered value.  Returns
	 true on success. If failured, then IsCellFromStringFail() returns false
	 and GetSetCellFromStringFailMsg() retuns a meaningful failure message
	 that can be displayed to the user. */
	virtual bool SetCellFromString(int row, int col, int time,
								   const wxString &value) = 0;
	virtual bool IsSetCellFromStringFail();
	virtual wxString GetSetCellFromStringFailMsg();
	/** Inserts a new Table column.  The return value is the same as the
	 * pos argument on success, and -1 on failure.  field_len and decimals
	 * are only used by certain older formats such as dBase that do not
	 * have variable-width fields, or do not have floating-point numbers.
	 * Values of -1 for these indicate to ignore or use a default. */
	virtual int InsertCol(GdaConst::FieldType type, const wxString& name,
						  int pos=-1, int time_steps=1,
						  int field_len=-1, int decimals=-1) = 0;
	virtual bool DeleteCol(int col) = 0;
	virtual void UngroupCol(int col) = 0;
	/** Columns to group in specified order. A placeholder column is
	 * indicated with -1 in cols vector. */	
	virtual void GroupCols(const std::vector<int>& cols,
						   const wxString& name, int pos=0) = 0;
	virtual void InsertTimeStep(int time, const wxString& name) = 0;
	
	/** This is a potentially very complicated operation because it can
	 * result in some combination of multiple new columns being added
	 * and removed.  When a time step is removed, all groups
	 * have the corresponding time step removed.  For each group, if the
	 * column at that time is a non-placeholder, the column will be removed
	 * from the group and added back into the Table at the current group
	 * position.  If there is only a placeholder
	 * at that position, then the placeholder is removed.  If the resulting
	 * group only contains placeholders, then the group itself is removed
	 * from the Table. */
	virtual void RemoveTimeStep(int time) = 0;
	virtual void SwapTimeSteps(int time1, int time2) = 0;
	virtual void RenameTimeStep(int time, const wxString& new_name) = 0;
    
    /** Checks is name is a valid database column name given naming
     * restrctions for database as specified by cols_max_length,
     * cols_case_sensitive and cols_ascii_only or other encoding restrictions.
     * This is generally much more strict than space-time group names.
     * Does not verify that name is unique. */
    virtual bool IsValidDBColName(const wxString& col_nm,
                                  wxString* fld_warn_msg=0) =0;

	
    /**
     * belows are non-virtual functions
     *
     */
    virtual std::vector<wxString> GetGroupNames();
    virtual int GetColIdx(const wxString& name, bool ignore_case=false);
    
	/** Sets encoding of string column data.  Can possibly use for data base
	 * column name encoding, but for now remain more restrictive */
	virtual void SetEncoding(wxFontEncoding enc_type);
	virtual wxFontEncoding GetFontEncoding() { return encoding_type; }
	
	/** Suggests a group name based on the member names listed in cols.
	 * Returned value is a unique, valid group name: it is different than
	 * all other groups and DB fields.  Since group names are stored as
	 * meta-data, they do not need to follow naming, case or encoding
	 * restrictions.  Generally the longest common substring will be used
	 * as the group name (excluding spaces, underscores, etc).  If there
	 * is no common prefix, then the first name in cols will be used, but
	 * modified to be unique. */
	virtual wxString SuggestGroupName(std::vector<wxString> cols) const;
	
	/** Suggest a sequence of of n column names for use in a newly
	 * created space-time group.  The column names can be written to the
	 * data-base will conform to DB restrictions:
	 *   cols_case_sensitive (true/false)
	 *   cols_max_length
	 *   cols_ascii_only (true/false)
	 * All column names are unique amongst all other data base names and
	 * group names including new_grp_name.  At attempt will be made
	 * to use prefix (could be the new_grp_name) as a prefix for the column
	 * names.  The prefix might be modified as needed. */
	virtual std::vector<wxString> SuggestDBColNames(wxString new_grp_name,
													   wxString prefix,
													   int n) const;
	
	/** Returns a valid group name based on grp_nm string.  Returned value
	 * different than all existing group and database field names.  If grp_nm
	 * is already unique, then grp_nm is returned unmodified.  If an empty
	 * value is passed, then a group name of the form "Group x" is returned
	 * where x is a non-negative integer. */
	virtual wxString GetUniqueGroupName(wxString grp_nm) const;
	
	/** Returns n valid database column name based on col_nm string.
	 * Returned value different than all existing group and database
	 * column names and conforms to various column name restrictions as
	 * defined in this class.  If col_nm is already unique and n=1,
	 * then col_nm is returned unmodified.  If an empty value is passed,
	 * then a col name of the form "VAR_X" is returned where x is a
	 * non-negative integer. */
	virtual std::vector<wxString> GetUniqueColNames(wxString col_nm,
													int n) const;
	
	/** Checks is name is a valid group name given any naming restrctions
	 * for groups.  This is generally much less strict than database
	 * column names.  Does not verify that name is unique. */
	virtual bool IsValidGroupName(const wxString&  grp_nm) const;
    
    
		
protected:
	
	TableState* table_state;
	TimeState*  time_state;

	bool is_valid;
	wxString open_err_msg;
	int rows;
	bool changed_since_last_save;
    bool project_changed_since_last_save;
	
	bool is_set_cell_from_string_fail;
	wxString set_cell_from_string_fail_msg;
	
	/** Table Attributes. Should reflect limitations of underlying database. */
	
	wxFontEncoding encoding_type;
	wxCSConv* m_wx_encoding;  // can be NULL.  Should match encoding_type
	
	bool cols_case_sensitive;
	bool cols_max_length;
	bool cols_ascii_only;
};

#endif
