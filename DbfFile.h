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

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <utility>
#include <vector>
#include <wx/string.h>
#include <wx/txtstrm.h>
#include <wx/sstream.h>
#include "GdaConst.h"

#ifndef __GEODA_CENTER_DBF_FILE_H__
#define __GEODA_CENTER_DBF_FILE_H__

struct DbfFieldDesc
{
  wxString name; // 10 ASCII character string (with 11th char as 0x00).
  char type; // either C, D, F or N
  int length; // unsigned 8-bit <= 255
  int decimals; // unsigned 8-bit <= 15
};

struct DbfFileHeader
{
  char version; // 8-bit bitmask in DBF
  int year; // unsigned 8 bits little endian in DBF (-1900)
  int month; // unsigned 8 bits little endian in DBF
  int day; // unsigned 8 bits little endian in DBF
  wxUint32 num_records;  // unsigned 32 bits little endian in DBF
  int header_length; // unsigned 16 bits little endian in DBF
  int length_each_record;  // unsigned 16 bits little endian in DBF
  int num_fields; // calculated
};

class DbfFileReader
{
  public:
  DbfFileReader(const wxString& filename);
  virtual ~DbfFileReader();
  bool isDbfReadSuccess();
  DbfFileHeader getFileHeader();
  std::vector<DbfFieldDesc> getFieldDescs();
  DbfFieldDesc getFieldDesc(int field);
  DbfFieldDesc getFieldDesc(const wxString& f_name);
  void getFieldTypes(std::map<wxString,GdaConst::FieldType>& field_type_map);
  void getFieldList(std::vector<wxString>& field_list);
  bool getFieldValsLong(int field, std::vector<wxInt64>& vals);
  bool getFieldValsLong(const wxString& f_name, std::vector<wxInt64>& vals);
  bool getFieldValsDouble(int field, std::vector<double>& vals);
  bool getFieldValsDouble(const wxString& f_name, std::vector<double>& vals);
  bool getFieldValsString(int field, std::vector<wxString>& vals);
  int getNumFields() const { return header.num_fields; }
  int getNumRecords() const { return header.num_records; }
  void printFileHeader(wxTextOutputStream& outstrm);
  void printFieldDesc(int field, wxTextOutputStream& outstrm);
  void printFieldValues(int field, wxTextOutputStream& outstrm);
  bool isFieldExists(int field);
  bool isFieldExists(const wxString& f_name);
  bool isFieldValUnique(int field);
  bool isFieldValUnique(const wxString& f_name);
  wxString getFileName() { return fname; }

private:
  bool populateHeader();
  bool populateFieldDescs();
  bool read_success;

public:	
  std::ifstream file;
  wxString fname;
  std::vector<DbfFieldDesc> fields;
  DbfFileHeader header;
  static const int MAX_NUMBER_FIELDS;
};

namespace DbfFileUtils
{
  bool insertIdFieldDbf(const wxString& in_fname,
			const wxString& out_fname,
			const wxString& id_fld_name,
			int id_fld_pos,
			wxString& err_msg);
  bool isValidFieldName(const wxString& n);
  bool isAlphabetic(const wxString& n); // looks at first char in string
  bool isDigit(const wxString& n); // looks at first char in string
  bool isAlphaNum(const wxString& n); // looks at first char in string
  int getNumRecords(const wxString& fname);
  int getNumFields(const wxString& fname);
  wxInt64 GetMaxInt(int length);
  wxString GetMaxIntString(int length);
  wxInt64 GetMinInt(int length);
  wxString GetMinIntString(int length);
  void SuggestDoubleParams(int length, int decimals,
						   int* suggest_len, int* suggest_dec);
  double GetMaxDouble(int length, int decimals,
					  int* suggest_len=0, int* suggest_dec=0);
  wxString GetMaxDoubleString(int length, int decimals);
  double GetMinDouble(int length, int decimals,
					  int* suggest_len=0, int* suggest_dec=0);
  wxString GetMinDoubleString(int length, int decimals);
  void strToInt64(const char *str, wxInt64 *val);
}

#endif
