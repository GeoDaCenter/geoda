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

#ifndef __GEODA_CENTER_DBF_COL_CONTAINER_H__
#define __GEODA_CENTER_DBF_COL_CONTAINER_H__

#include <map>
#include <vector>
#include <wx/filename.h>
#include <wx/grid.h>
#include "TableStateObserver.h"
#include "../GdaConst.h"
#include "../HighlightStateObserver.h"
#include "../DbfFile.h"

/**
 DbfColContainer notes: when a DBF file is read from disk, we initially
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
class DbfColContainer
{
public:
	DbfColContainer();
	virtual ~DbfColContainer();
	bool Init(int size,
			  const GdaConst::FieldInfo& field_info,
			  bool alloc_raw_data,
			  bool alloc_vector_data,
			  bool mark_all_defined);
	
	int size; // number of rows
	
	void AllocRawData();
	bool IsRawDataAlloc();
	void FreeRawData();
	
	void AllocVecData();
	bool IsVecDataAlloc();
	void FreeVecData();
	
	// raw character data directly from the DBF file but with null-terminated
	// strings.  If raw_data is valid, then the pointer is non-null,
	// otherwise it is set to zero.
	char* raw_data;
	double* d_vec;
	wxInt64* l_vec;
	wxString* s_vec;
	// for use by d_vec and l_vec to denote either empty or undefined values.
	std::vector<bool> undefined;
	// when reading in a large DBF file, we do not take the time to
	// check that all numbers are valid.  When a data column is read
	// for the first time, we will take the time to properly set the
	// undefined vector.  If the vector has not been initialized, then
	// TableInterface::GetValue should not rely the values in the
	// undefined vector.
	bool undefined_initialized;
	
	wxString GetName();
	wxString GetDbfColName();
	GdaConst::FieldType GetType();
	int GetFieldLen();
	int GetDecimals();
	
	void GetMinMaxVals(double& min_val, double& max_val);
	
	// Function to change properties.
	bool ChangeProperties(int new_len, int new_dec=0);
	bool ChangeName(const wxString& new_name);
	
	void GetVec(std::vector<double>& vec);
	void GetVec(std::vector<wxInt64>& vec);
	void GetVec(std::vector<wxString>& vec);
	
	// note: the following two functions only have an
	// effect on numeric fields currently.
	void SetFromVec(const std::vector<double>& vec);
	void SetFromVec(const std::vector<wxInt64>& vec);
	void SetFromVec(const std::vector<wxString>& vec);
	void CheckUndefined();
	void SetUndefined(const std::vector<bool>& undef_vec);
	void GetUndefined(std::vector<bool>& undef_vec);
	
	void CopyRawDataToVector();
	void CopyVectorToRawData();
	
	void UpdateMinMaxVals();
	bool stale_min_max_val;
	
private:
	GdaConst::FieldInfo info;
	
	double min_val;
	double max_val;	
	
	void raw_data_to_vec(std::vector<double>& vec);
	void raw_data_to_vec(double* vec);
	void raw_data_to_vec(std::vector<wxInt64>& vec);
	void raw_data_to_vec(wxInt64* vec);
	void raw_data_to_vec(std::vector<wxString>& vec);
	void raw_data_to_vec(wxString* vec);
	
	void d_vec_to_raw_data();
	void l_vec_to_raw_data();
	void s_vec_to_raw_data();
	
public:
	static bool sprintf_period_for_decimal();
};


#endif
