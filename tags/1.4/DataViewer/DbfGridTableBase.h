/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_GRID_TABLE_BASE_H__
#define __GEODA_CENTER_GRID_TABLE_BASE_H__

#include <map>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/filename.h>
#include <wx/grid.h>
#include "TableStateObserver.h"
#include "../GeoDaConst.h"
#include "../Generic/HighlightStateObserver.h"
#include "../ShapeOperations/DbfFile.h"


/** DbfColContainer notes: when a DBF file is read from disk, we initially
 just read its data into the raw_data array in raw form to minimize table
 loading time.  The corrseponding data vector d_vec, l_vec or s_vec are
 not filled.  When a new column is created, only d_vec, l_vec, or s_vec are
 created and raw_data is left as empty.  When data is written out to disk,
 the data in d_vec, l_vec, s_vec is converted into raw_data unless only
 raw_data only exists.
 
 So, let's allow both raw_data and the vector data to potentially exist
 together.  When a cell is written, it is written to both.  When
 an entire column is written, it is only written into to the vector, and
 raw_data is deleted.  When an entire column is read in (for example by
 Scatter Plot), then we must first create the vector if it doesn't already
 exist.
 
 So, in summary, whenever both raw_data and corresponding vector exist,
 single cell updates are written to both, but entire column updates
 only go to the vector and the raw_data is deleted.  When data is
 written to disk, the raw_data is created once again.  At any given time
 it is therefore possible to have just the raw_data, just the vector, or
 both raw_data and vector.  When providing values to wxGrid, we will
 always pull the value from vector first, and then raw_data.  In either
 case, we must check the undefined flag. When writing a column of data,
 we will likely also pass in an optional boolean vector of undefined flags.
 
 For d_vec and l_vec, we might want the potential to specify empty or
 undefined values.  The IEEE double standard gives us several special
 values for this purpose, but for integers, there are no special reserved
 values.  We must therefore provide some sort of integer status flags.
 Perhaps for both we can just specify defined or undefined?  Then it
 would be sufficient to maintain a common bit-vector called to flag
 empty values (either because no value was provided, or because the value
 was undefined).
 */
class DbfGridTableBase;

typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<wxInt64, 2> l_array_type;
typedef boost::multi_array<wxString, 2> s_array_type;
typedef boost::multi_array<bool, 2> b_array_type;
typedef boost::multi_array<std::string, 2> std_str_array_type;

class DbfColContainer
{
public:
	DbfColContainer(DbfGridTableBase* grid_base);
	DbfColContainer(DbfFileReader& dbf, int field, DbfGridTableBase* grid_base);
	virtual ~DbfColContainer();
	bool Init(int size, int time_steps, GeoDaConst::FieldType type,
			  const wxString& name,
			  int field_len, int decimals, int displayed_decimals,
			  bool alloc_raw_data,
			  bool alloc_vector_data,
			  bool mark_all_defined);
	int size; // number of rows
	int time_steps; // number of time steps.  If time_steps=1, then
	// not time-series data.
	GeoDaConst::FieldType type;
	wxString name;
	int field_len; // number of chars in DBF field
	int decimals;
	int displayed_decimals; // number of decimal places shown in wxGrid
	
	void AllocRawData();
	bool IsRawDataAlloc();
	void FreeRawData();
	
	// raw character data directly from the DBF file but with null-terminated
	// strings.  If raw_data is valid, then the pointer is non-null,
	// otherwise it is set to zero.
	// When vector_valid is false, size of corresponding vector should be
	// zero to save memory.
	std::vector<char*> raw_data;
	// true if one of d_vec, l_vec, or s_vec is valid, depending on data type
	bool vector_valid;
	d_array_type d_vec;
	l_array_type l_vec;
	s_array_type s_vec;
	// for use by d_vec and l_vec to denote either empty or undefined values.
	b_array_type undefined;
	// when reading in a large DBF file, we do not take the time to
	// check that all numbers are valid.  When a data column is read
	// for the first time, we will take the time to properly set the
	// undefined vector.  If the vector has not been initialized, then
	// DbfGridTableBase::GetValue should not rely the values in the
	// undefined vector.
	bool undefined_initialized;
	
	void GetMinMaxVals(std::vector<double>& min_vals,
					   std::vector<double>& max_vals);
	std::vector<bool> stale_min_max_val;
	std::vector<double> min_val;
	std::vector<double> max_val;
	void UpdateMinMaxVals();
	
	// Function to change properties.
	bool ChangeProperties(const wxString& new_name, int new_len, int new_dec=0,
						  int new_disp_dec=0);

	void GetVec(std::vector<double>& vec, int time=0);
	void GetVec(std::vector<wxInt64>& vec, int time=0);
	void GetVec(std::vector<wxString>& vec, int time=0);
	void GetVec(d_array_type& data);
	void GetVec(l_array_type& data);
	
	// note: the following two functions only have an
	// effect on numeric fields currently.
	void SetFromVec(std::vector<double>& vec, int time=0);
	void SetFromVec(std::vector<wxInt64>& vec, int time=0);
	void SetFromVec(std::vector<std::string>& vec, int time=0);
	void CheckUndefined();
	void SetUndefined(const std::vector<bool>& undef_vec, int time=0);
	void GetUndefined(std::vector<bool>& undef_vec, int time=0);
	void GetUndefined(b_array_type& data);
	
	bool IsVecItemDefined(int i, int time=0);
	bool IsRawItemDefined(int i, int time=0);
	
	void CopyRawDataToVector();
	void CopyVectorToRawData();
protected:
	void raw_data_to_vec(d_array_type& vec);
	void raw_data_to_vec(std::vector<double>& vec, int time);
	void raw_data_to_vec(l_array_type& vec);
	void raw_data_to_vec(std::vector<wxInt64>& vec, int time);
	void raw_data_to_vec(s_array_type& vec);
	void raw_data_to_vec(std::vector<wxString>& vec, int time);
	
	void d_vec_to_raw_data();
	void l_vec_to_raw_data();
	void s_vec_to_raw_data();
	
	DbfGridTableBase* grid_base; // ref used to notify when changes made
public:
	static bool sprintf_period_for_decimal();
};

class DbfGridCellAttrProvider : public wxGridCellAttrProvider
{
public:
	DbfGridCellAttrProvider(std::vector<int>& row_order,
							std::vector<bool>& selected);
	virtual ~DbfGridCellAttrProvider();
	
	virtual wxGridCellAttr *GetAttr(int row, int col,
									wxGridCellAttr::wxAttrKind kind) const;
	
private:
	wxGridCellAttr* attrForSelectedRows;
	std::vector<int>& row_order;
	std::vector<bool>& selected;
};

class DbfGridTableBase : public wxGridTableBase, public HighlightStateObserver,
	public TableStateObserver
{
public:
	DbfGridTableBase(const std_str_array_type& string_table,
					 const std::vector<std::string>& field_names,
					 const std::string file_name,
					 HighlightState* highlight_state,
					 TableState* table_state);
	DbfGridTableBase(DbfFileReader& dbf, HighlightState* highlight_state,
					 TableState* table_state);
	DbfGridTableBase(DbfFileReader& dbf_sp, DbfFileReader& dbf_tm,
					 int sp_tbl_sp_col, int tm_tbl_sp_col, int tm_tbl_tm_col,
					 HighlightState* highlight_state,
					 TableState* table_state);
	DbfGridTableBase(int rows, int cols, HighlightState* highlight_state,
					 TableState* table_state);
	virtual ~DbfGridTableBase();
	
	int time_steps; // 1 for a non space-time DBF.
	int curr_time_step; // value between 0 and time_steps-1;
	std::vector<wxInt64> time_ids;
	std::vector<DbfColContainer*> col_data;
	std::vector<int> row_order;

	bool IsSelected(int row);
	void Select(int row);
	void Deselect(int row);
	bool FromGridIsSelected(int row);
	void FromGridSelect(int row);
	void FromGridDeselect(int row);
	void SelectAll();
	void DeselectAll();
	void InvertSelection();
	void SortByDefaultAscending();
	void SortByDefaultDecending();
	void SortByCol(int col, bool ascending);
	void MoveSelectedToTop();
	bool IsSortedAscending() { return sorting_ascending; }
	int GetSortingCol() { return sorting_col; }
	int sorting_col;
	bool sorting_ascending;
	int rows;
	wxFileName GetDbfFileName() { return dbf_file_name; }
	wxString GetDbfNameNoExt() { return dbf_file_name_no_ext; }
	wxFileName dbf_file_name;
	wxString dbf_file_name_no_ext;
	wxFileName dbf_tm_file_name;
	wxString dbf_tm_file_name_no_ext;
	wxFileName GetSpaceDbfFileName() { return dbf_file_name; }
	wxFileName GetTimeDbfFileName() { return dbf_tm_file_name; }
	DbfFileHeader orig_header;
	DbfFileHeader orig_header_tm;
	bool ChangedSinceLastSave() { return changed_since_last_save; }
	void SetChangedSinceLastSave(bool chg) { changed_since_last_save = chg; }
	bool ColNameExists(const wxString& name);
	int FindColId(const wxString& name);
	
	void PrintTable();
	bool WriteToDbf(const wxString& fname, wxString& err_msg);
	bool WriteToSpaceTimeDbf(const wxString& space_fname,
							 const wxString& time_fname, wxString& err_msg);

	void ReorderCols(const std::vector<int>& col_order);
	void FillColIdMap(std::vector<int>& col_map);
	void FillNumericColIdMap(std::vector<int>& col_map);
	void FillIntegerColIdMap(std::vector<int>& col_map);
	void FillNumericNameList(std::vector<wxString>& num_names);
	
	bool IsSpaceTimeIdField(const wxString& name);
	wxString GetSpTblSpColName() { return sp_tbl_sp_col_name; }
	int GetNumberColsSpace();
	int GetNumberColsTime();
	
	int GetCurrTime() { return curr_time_step; }
	bool IsTimeVariant() { return time_steps > 1; }
	int GetTimeSteps() { return time_steps; }
	wxString GetTimeString(int time);
	void GetTimeStrings(std::vector<wxString>& times);
	
	bool IsColTimeVariant(const wxString& name);
	bool IsColTimeVariant(int col);
	int GetColTimeSteps(int col);
	bool IsColNumeric(int col);
	GeoDaConst::FieldType GetColType(int col);
	wxString GetColName(int col);
	int GetColLength(int col);
	int GetColDecimals(int col);
	void GetColData(int col, d_array_type& dbl_data);
	void GetColData(int col, int time, std::vector<double>& data);
	void GetColUndefined(int col, b_array_type& undefined);
	void GetColUndefined(int col, int time, std::vector<bool>& undefined);
	void GetMinMaxVals(int col, std::vector<double>& min_vals,
					   std::vector<double>& max_vals);
	
	bool ConvertToSpTime(const wxString& sp_dbf_name,
						 const wxString& tm_dbf_name,
						 int space_col, const wxString& tm_field_name,
						 const std::vector<wxInt64>& new_time_ids,
						 wxString& err_msg);
	
	bool IsDuplicateColNames(wxString& dup_ret_str);
	bool CopySpColRawToTmColRaw(int sp_col, int tm_col, int time);
	
	// pure virtual method implementation for wxGridTableBase
	virtual int GetNumberRows();
	virtual int GetNumberCols();
	virtual wxString GetValue(int row, int col);
	virtual void SetValue(int row, int col, const wxString &value);
	virtual bool IsEmptyCell(int row, int col) { return false; }
	
	virtual bool InsertCol(int pos,
						   int time_steps, GeoDaConst::FieldType type,
						   const wxString& name,
						   int field_len, int decimals=0,
						   int displayed_decimals=0,
						   bool alloc_raw_data = false,
						   bool mark_all_defined = true);
	virtual bool DeleteCol(int pos);
	// methods called by wxGrid
	virtual wxString GetRowLabelValue(int row);
	virtual wxString GetColLabelValue(int col);
	
	/** This is the implementation of the HighlightStateObserver interface
	 update function. It is called whenever the HighlightState's state has
	 changed.  In this case, the observable is HighlightState, which keeps
	 track of the hightlighted/selected state for every SHP file observation. */
	virtual void update(HighlightState* o);
	
	/** This is the implementation of the TableStateObserver interface
	 update function. It is called whenever the table associated with
	 TableState's state has changed. Since this is the table, the update
	 method will do nothing. */
	virtual void update(TableState* o);
	void notifyColMove();
	
private:
	wxString sp_tbl_sp_col_name;
	wxString tm_tbl_sp_col_name;
	wxString tm_tbl_tm_col_name;
	HighlightState* highlight_state;
	TableState* table_state;
	std::vector<bool>& hs; //shortcut to HighlightState::highlight read only!
	bool changed_since_last_save;
};

#endif
