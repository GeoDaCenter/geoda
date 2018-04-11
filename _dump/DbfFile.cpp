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
#include <iomanip>
#include <wx/hashset.h>
#include <wx/strconv.h>
#include <wx/filename.h>
//#include <time.h> // for random number generator
#include <stdlib.h> // for random number generator and atoi
#include "GdaConst.h"
#include "logger.h"
#include "DbfFile.h"
#include "GenUtils.h"

// dBaseIII+ is only 128, but many others are much higher.  Some people
// have imperically discovered that 2046 is OK in practice for many
// implementations.
const int DbfFileReader::MAX_NUMBER_FIELDS = 2046;

DbfFileReader::DbfFileReader(const wxString& filename) :
  fname(filename), read_success(false)
{
  file.open(fname.fn_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) {
    read_success = false;
    return;
  }
  if (populateHeader() && populateFieldDescs()) read_success = true;
  if (file.is_open()) file.close();
}

DbfFileReader::~DbfFileReader()
{
  if (file.is_open()) file.close();
}

DbfFileHeader DbfFileReader::getFileHeader()
{
  return header;
}

std::vector<DbfFieldDesc> DbfFileReader::getFieldDescs()
{
  return fields;
}

void DbfFileReader::getFieldTypes(
	std::map<wxString, GdaConst::FieldType>& field_type_map)
{
	field_type_map.clear();
	for (int i=0, ie=fields.size(); i<ie; i++) {
		wxString name(fields[i].name);
		char type = fields[i].type;
		int length = fields[i].length;
		int decimals = fields[i].decimals;
		
		if (type == 'N' || type == 'F') {
			if (decimals > 0) {
				field_type_map[name] = GdaConst::double_type;
			} else {
				field_type_map[name] = GdaConst::long64_type;
			}
		} else if (type == 'D') {
			field_type_map[name] = GdaConst::date_type;
		} else {
			// assume (type == 'C')
			field_type_map[name] = GdaConst::string_type;
		}
	}
}

void DbfFileReader::getFieldList(std::vector<wxString>& field_list)
{
	field_list.resize(fields.size());
	for (int i=0, ie=fields.size(); i<ie; i++) {
		field_list[i] = fields[i].name.c_str();
	}
}

bool DbfFileReader::isDbfReadSuccess()
{
  return read_success;
}

bool DbfFileReader::isFieldExists(int field)
{
    return (field >= 0 && field < (int) fields.size());
}

bool DbfFileReader::isFieldExists(const wxString& f_name)
{
    for (int i=0; i< (int) fields.size(); i++)
        if (f_name == fields[i].name) return true;
    return false;
}

DbfFieldDesc DbfFileReader::getFieldDesc(int field)
{
	if (field >= 0 && field < (int) fields.size())
		return fields[field];
	else
		return DbfFieldDesc();
}

DbfFieldDesc DbfFileReader::getFieldDesc(const wxString& f_name)
{
	for (int i=0; i< (int) fields.size(); i++)
		if (f_name == fields[i].name)	return fields[i];
	return DbfFieldDesc();
}

bool DbfFileReader::isFieldValUnique(const wxString& f_name)
{
	for (int i=0; i< (int) fields.size(); i++) {
		if (f_name == fields[i].name)
			return isFieldValUnique(i);
	}
	return false;
}

WX_DECLARE_HASH_SET( wxString, wxStringHash, wxStringEqual, StringSet);

bool DbfFileReader::isFieldValUnique(int field)
{
  bool all_unique = false;
  read_success = false;
  if (!file.is_open())
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) return false;
 
  if (field >= (int) fields.size()) {
    return false;
  }

  // calculate field offset
  int record_offset = 1;  // the record deletion flag
  for (int i=0; i<field; i++) {
    record_offset += fields[i].length;
  }
  int field_length = fields[field].length;
 
  char* char_buf = new char[field_length+1];
  char_buf[field_length] = '\0';
 
  file.seekg(header.header_length + record_offset, std::ios::beg); 
  StringSet string_set(header.num_records);
  wxString val_str;
  for (int i=0; i< (int) header.num_records; i++) {
    file.read(char_buf, field_length);
    val_str = wxString(char_buf, wxConvUTF8);
    string_set.insert(val_str);
    // seek to next record in file
    file.seekg(header.length_each_record-field_length, std::ios::cur);
  }
  delete [] char_buf;

  read_success = true;
  //file.close();
  return string_set.size() == header.num_records;
}

void DbfFileReader::printFileHeader(wxTextOutputStream& outstrm)
{
  outstrm << "version: " << (int) header.version << endl;
  outstrm << "year: " << (int) header.year << endl;
  outstrm << "month: " << (int) header.month << endl;
  outstrm << "day: " << (int) header.day << endl;
  outstrm << "num_records: " << (int) header.num_records << endl;
  outstrm << "header_length: " << (int) header.header_length << endl;
  outstrm << "length_each_record: " << (int) header.length_each_record
	  << endl;
  outstrm << "num_fields: " << (int) header.num_fields << endl;
  outstrm << "(header_length - 33) >> 5 = "
	  << ((header.header_length - 33) >> 5) << endl;
}

void DbfFileReader::printFieldDesc(int field, wxTextOutputStream& outstrm)
{
  outstrm << "name: " << fields[field].name;
  outstrm << ", type: " << wxChar(fields[field].type);
  outstrm << ", length: " << fields[field].length;
  outstrm << ", decimals: " << fields[field].decimals << endl;
}

bool DbfFileReader::getFieldValsLong(const wxString& f_name,
									 std::vector<wxInt64>& vals)
{
    for (int i=0; i< (int) fields.size(); i++) {
        if (f_name == fields[i].name)
            return getFieldValsLong(i, vals);
    }
    return false;
}

bool DbfFileReader::getFieldValsLong(int field, std::vector<wxInt64>& vals)
{
    if (field < 0 || field >= (int) fields.size()) return false;
    if (vals.size() != header.num_records) return false;

    if (!file.is_open())
		file.open(fname.fn_str(), std::ios::in | std::ios::binary);
    if (!(file.is_open() && file.good())) return false;

     // calculate field offset
    int record_offset = 1;  // the record deletion flag
    for (int i=0; i<field; i++) {
        record_offset += fields[i].length;
    }
    int field_length = fields[field].length;

    char* char_buf = new char[field_length+1];
    char_buf[field_length] = '\0';

    file.seekg(header.header_length + record_offset, std::ios::beg);

    for (int i=0; i< (int) header.num_records; i++) {
        file.read(char_buf, field_length);
        // seek to next record in file
        file.seekg(header.length_each_record-field_length, std::ios::cur);
		DbfFileUtils::strToInt64(char_buf, &vals[i]);
    }
    delete [] char_buf;

    return true;
}

// Convert an ASCII string into a wxInt64 (or long long)
// Note this function is also in GenUtils, but we want DbfFile
// to have minimal dependencies.
void DbfFileUtils::strToInt64(const char *str, wxInt64 *val)
{
	wxInt64 total = 0;
	bool minus = 0;
	
	while (isspace(*str)) str++;
	if (*str == '+') {
		str++;
	} else if (*str == '-') {
		minus = true;
		str++;
	}
	while (isdigit(*str)) {
		total *= 10;
		total += (*str++ - '0');
	}
	*val = minus ? -total : total;
}

bool DbfFileReader::getFieldValsDouble(const wxString& f_name,
									   std::vector<double>& vals)
{
    for (int i=0; i< (int) fields.size(); i++) {
        if (f_name == fields[i].name)
            return getFieldValsDouble(i, vals);
    }
    return false;
}

bool DbfFileReader::getFieldValsDouble(int field, std::vector<double>& vals)
{
    if (field < 0 || field >= (int) fields.size()) return false;
    if (vals.size() != header.num_records) return false;
	
    if (!file.is_open())
		file.open(fname.fn_str(), std::ios::in | std::ios::binary);
    if (!(file.is_open() && file.good())) return false;
	
	// calculate field offset
    int record_offset = 1;  // the record deletion flag
    for (int i=0; i<field; i++) {
        record_offset += fields[i].length;
    }
    int field_length = fields[field].length;
	
    char* char_buf = new char[field_length+1];
    char_buf[field_length] = '\0';
	
    file.seekg(header.header_length + record_offset, std::ios::beg);
	
    for (int i=0; i< (int) header.num_records; i++) {
        file.read(char_buf, field_length);
        // seek to next record in file
        file.seekg(header.length_each_record-field_length, std::ios::cur);
        //vals[i] = atof(char_buf);
		wxString::Format("%s", char_buf).ToCDouble(&vals[i]);
    }
    delete [] char_buf;
	
    return true;
}

bool DbfFileReader::getFieldValsString(int field, std::vector<wxString>& vals)
{
    if (field < 0 || field >= (int) fields.size()) return false;
    if (vals.size() != header.num_records) return false;
	
    if (!file.is_open())
		file.open(fname.fn_str(), std::ios::in | std::ios::binary);
    if (!(file.is_open() && file.good())) return false;
	
	// calculate field offset
    int record_offset = 1;  // the record deletion flag
    for (int i=0; i<field; i++) {
        record_offset += fields[i].length;
    }
    int field_length = fields[field].length;
	
    char* char_buf = new char[field_length+1];
    char_buf[field_length] = '\0';
	
    file.seekg(header.header_length + record_offset, std::ios::beg);
	
    for (int i=0; i< (int) header.num_records; i++) {
        file.read(char_buf, field_length);
        // seek to next record in file
        file.seekg(header.length_each_record-field_length, std::ios::cur);
		vals[i] = wxString(char_buf);
    }
    delete [] char_buf;
	
    return true;
}


void DbfFileReader::printFieldValues(int field, wxTextOutputStream& outstrm)
{
  read_success = false;
  if (!file.is_open())
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) return;

  if (field >= (int) fields.size()) {
    outstrm << "Error: field does not exist: field = "
	    << field << ", fields.size() = " << (int) fields.size()
	    << endl;
    return;
  }
  // calculate field offset
  int record_offset = 1;  // the record deletion flag
  for (int i=0; i<field; i++) {
    record_offset += fields[i].length;
  }
  int field_length = fields[field].length;
  outstrm << "fields[" << field << "].length = "
	  << field_length << endl;
  outstrm << "record_offset = " << record_offset << endl;

  char* char_buf = new char[field_length+1];
  char_buf[field_length] = '\0';
 
  file.seekg(header.header_length + record_offset, std::ios::beg);

  for (int i=0; i< (int) header.num_records; i++) {
    file.read(char_buf, field_length);
    // seek to next record in file
    file.seekg(header.length_each_record-field_length, std::ios::cur);
    wxString val_str = wxString(char_buf, wxConvUTF8);
    outstrm << "\"" << val_str << "\"" << endl;
  }
  delete [] char_buf;

  read_success = true;
  //file.close();
}

bool DbfFileReader::populateHeader()
{
  read_success = false;
  if (!file.is_open())
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) return false;

  wxUint32 u_int32;
  wxUint32* u_int32p = &u_int32;
  wxUint16 u_int16;
  wxUint16* u_int16p = &u_int16;
  wxUint8 u_int8;
  wxUint8* u_int8p = &u_int8;
  char membyte;

  file.seekg(0, std::ios::beg);
  file.get(membyte);
  header.version = membyte;

  file.get(membyte);
  header.year = 1900 + (int) membyte;

  file.get(membyte);
  header.month = (int) membyte;

  file.get(membyte);
  header.day = (int) membyte;

  file.read((char*) u_int32p, 4);
  u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
  header.num_records = u_int32;

  file.read((char*) u_int16p, 2);
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  header.header_length = (int) u_int16;

  // Read in sum of length of all fields (+ 1 for deletion flag)
  file.read((char*) u_int16p, 2);
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  header.length_each_record = (int) u_int16;

  // discover number of fields. MAX_NUMBER_FIELDS is the max
  // Could also calculate (header_length - 33) / 32, assuming
  // that the Visual FoxPro database container section doesn't exist.
  file.seekg(32, std::ios::beg);
  header.num_fields = 0;
  char first_byte;
  file.read(&first_byte, 1);
  while (first_byte != 0xd) { // && header.num_fields < MAX_NUMBER_FIELDS) {
    header.num_fields++;
    file.seekg(31, std::ios::cur);
    file.read(&first_byte, 1);
  }

  read_success = true;
  //file.close();
  return true;
}

bool DbfFileReader::populateFieldDescs()
{
  read_success = false;
  if (!file.is_open())
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
  if (!(file.is_open() && file.good())) return false;

  wxUint32 u_int32;
  wxUint32* u_int32p = &u_int32;
  wxUint16 u_int16;
  wxUint16* u_int16p = &u_int16;
  wxUint8 u_int8;
  wxUint8* u_int8p = &u_int8;
  char type;
  char ascii_name[11];

  // Read in number of records
  file.seekg(4, std::ios::beg);
  file.read((char*) u_int32p, 4);
  u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
  wxUint32 num_recs = u_int32;

  // Read in length of header structure
  file.read((char*) u_int16p, 2);
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  wxUint16 header_len = u_int16;

  // Read in sum of length of all fields (+ 1)
  file.read((char*) u_int16p, 2);
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  wxUint16 sum_field_len = u_int16;

  // discover number of fields. MAX_NUMBER_FIELDS is the max
  file.seekg(32, std::ios::beg);
  int num_fields = 0;
  char first_byte;
  file.read(&first_byte, 1);
  while (first_byte != 0xd ) { // || num_fields != MAX_NUMBER_FIELDS) {
    num_fields++;
    file.seekg(31, std::ios::cur);
    file.read(&first_byte, 1);
  }
  fields.resize(num_fields);
 
  // Read in all header descriptors.  Each desc is 32 bytes long.
  file.seekg(32, std::ios::beg);
  for (int i=0; i<num_fields; i++) {
    file.read(ascii_name, 11);
    fields[i].name = wxString(ascii_name, wxCSConv("utf-8"));
	fields[i].name.Trim(false); // remove left spaces
	fields[i].name.Trim(true); // remove right spaces
    file.read(&type, 1);
    fields[i].type = type;
    file.seekg(4, std::ios::cur); // skip 4 bytes
    file.read((char*) u_int8p, 1);
    fields[i].length = u_int8;
    file.read((char*) u_int8p, 1);
    fields[i].decimals = u_int8;
    file.seekg(14, std::ios::cur); // skip 14 bytes to next descriptor
  }

  read_success = true;
  //file.close();
  return true;
}


bool DbfFileUtils::insertIdFieldDbf(const wxString& in_fname,
				    const wxString& out_fname,
				    const wxString& id_fld_name,
				    int id_fld_pos,
					wxString& err_msg)
{
  DbfFileHeader header;
  std::vector<DbfFieldDesc> fields;
  DbfFileReader* reader_ptr = new DbfFileReader(in_fname);
  header = reader_ptr->getFileHeader();
  fields = reader_ptr->getFieldDescs();
  bool file_valid = reader_ptr->isDbfReadSuccess();
  delete reader_ptr;
  if (!file_valid) {
    err_msg += "Error: could not read \"" + in_fname + "\"";
    return false;
  }

  std::ifstream in_file;
  in_file.open(in_fname.fn_str(), std::ios::in | std::ios::binary);

  if (!(in_file.is_open() && in_file.good())) {
	err_msg += "Error: Problem opening \"" + in_fname + "\"";
    return false;
  }

  std::ofstream out_file;
  wxString temp_out_fname = out_fname;
  if (in_fname == out_fname) {
	  wxFileName fn(out_fname);
	  wxString just_name = fn.GetName();
	  fn.SetName("1-_" + just_name);
	  temp_out_fname = fn.GetPathWithSep() + fn.GetFullName();
  }
  //out_file.open(temp_out_fname.fn_str(),std::ios::out | std::ios::binary);
  out_file.open(GET_ENCODED_FILENAME(temp_out_fname), std::ios::out | std::ios::binary);

  if (!(out_file.is_open() && out_file.good())) {
	err_msg += "Error: Problem opening \"" + temp_out_fname + "\"";
    return false;
  }
  
  //if (header.num_fields == DbfFileReader::MAX_NUMBER_FIELDS) {
  //  err_msg += "Error: adding a new field will exceed ";
  //  err_msg << DbfFileReader::MAX_NUMBER_FIELDS;
  //  err_msg +=  " field max ";
  //  err_msg += "for a DBF file.";
  //  return false;
  //}
	
  // make sure the new id_fld_pos is valid:
  if (id_fld_pos < 0) id_fld_pos = 0;
  if (id_fld_pos > header.num_fields) id_fld_pos = header.num_fields;
  if (id_fld_name.length() > 10) {
    err_msg += "Error: the new field name is greater than 10 characters ";
	err_msg += "in length.";
    return false;
  }

  wxUint32 u_int32;
  wxUint32* u_int32p = &u_int32;
  wxUint16 u_int16;
  wxUint16* u_int16p = &u_int16;
  wxUint8 u_int8;
  wxUint8* u_int8p = &u_int8;
  char membyte;

  // byte 0
  membyte = header.version;
  out_file.put(membyte);

  // byte 1
  membyte = (char) (header.year - 1900);
  out_file.put(membyte);
  
  // byte 2
  membyte = (char) header.month;
  out_file.put(membyte);

  // byte 3
  membyte = (char) header.day;
  out_file.put(membyte);
 
  // byte 4-7
  u_int32 = header.num_records;
  u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
  out_file.write((char*) u_int32p, 4);

  // byte 8-9
  // Since we are adding a new field, the header length grows by 32 bytes.
  u_int16 = header.header_length + 32;
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  out_file.write((char*) u_int16p, 2);

  // byte 10-11
  // Since we are adding a new field of size 10, the length of each record
  // grows by 10 bytes.
  u_int16 = header.length_each_record + 10;
  u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
  out_file.write((char*) u_int16p, 2);

  // byte 12-13 (0x0000)
  u_int16 = 0x0;
  out_file.write((char*) u_int16p, 2);

  // copy whatever is in source for
  // bytes 14 through 31.
  in_file.seekg(14, std::ios::beg);
  for (int i=0; i< (31-14)+1; i++) {
    in_file.get(membyte);
    out_file.put(membyte);
  }

  // in_file and out_file now both point to byte 32, which is the beginning of
  // the list of fields.  There must be at least one field.  Each field
  // descriptor is 32 bytes long with byte 0xd following the last field
  // descriptor.  We will insert our new field at the head of the list and
  // then copy the in_file field descriptors after that.
  
  char* byte32_buff = new char[32];
  for (int i=0; i<header.num_fields+1; i++) {
    if (i == id_fld_pos) {
      // Insert the new field descriptor here.  The total number of
      // records is at most 2^32 = 4 294 967 296, so the new id field
      // needs to be at most 10 decimal digits in length.
      for (int j=0; j<32; j++) byte32_buff[j] = 0;
      std::string c_str(id_fld_name.mb_str(wxConvUTF8));
      for (int j=0; j< (int) id_fld_name.length(); j++)
		  byte32_buff[j] = c_str[j];
      byte32_buff[11] = 'N'; // type is numeric
      byte32_buff[16] = 10; // length is 10
      out_file.write(byte32_buff, 32);
    } else {
      in_file.read(byte32_buff, 32);
      out_file.write(byte32_buff, 32);
    }
  }
  delete byte32_buff;

  // move in_file pointer past 0x0D terminator
  in_file.get(membyte);
  // 0x0D marks the end of the header structure
  out_file.put((char) 0xD);

  // We will now write out the records, one at a time.
  // Calculate the source split position.  The split position denotes the
  // number of characters to copy before the new field. For example, if
  // the new field is at the beginning, then split is 0.  If it is at the
  // end then split = header.length_each_record.

  int split = 1;  // we always start after the record deletion flag
  int pre_split_mode = true;
  {
    int i=0;
    while (i< (int) fields.size() && pre_split_mode) {
      if (id_fld_pos == i) pre_split_mode = false;
      if ( pre_split_mode ) split += fields[i].length;
      i++;
    }
  }

  int src_rec_buff_len = header.length_each_record;
  char* src_rec_buff = new char[src_rec_buff_len];
  int targ_rec_buff_len = src_rec_buff_len + 10;
  char* targ_rec_buff = new char[targ_rec_buff_len];
  //srand(time(NULL));
  //int random_integer;
  for (int i=0; i< (int) header.num_records; i++) {
    in_file.read(src_rec_buff, src_rec_buff_len);
    for (int j=0; j<split; j++) {
      targ_rec_buff[j] = src_rec_buff[j];
    }
    std::ostringstream ost("");
    ost.width(10);
    ost << i+1;
	//random_integer = 1000 + rand()%30011;
    //ost << random_integer;
    std::string id_str = ost.str();
    for (int j=0; j<10; j++) {
      targ_rec_buff[split+j] = id_str[j];
    }
    for (int j=split; j<src_rec_buff_len; j++) {
      targ_rec_buff[j+10] = src_rec_buff[j];
    }
    out_file.write(targ_rec_buff, targ_rec_buff_len);
  }
  delete targ_rec_buff;
  delete src_rec_buff;

  // 0x1A is the EOF marker
  membyte = (char) 0x1A;
  out_file.put(membyte);

  out_file.close();
  in_file.close();

  if (out_fname == in_fname) {
    // we know that temp_out_fname was used and now we need
    // to move temp_out_fname to in_fname.
    if (!wxRenameFile(temp_out_fname, in_fname, true)) {
		err_msg += "Error: Failed to overwrite DBF file with ";
		err_msg += "updated temporary DBF file.";
    }
  }

  return true;
}

/** A valid field name is between 1 and 10 chars long
 It has no spaces.  The first char must be a letter,
 and the following can be alphanumerics or '_'. */
bool DbfFileUtils::isValidFieldName(const wxString& n)
{
	if ( n.size() < 1 || n.size() > 10 ) return false;
	if ( !isAlphabetic(n.Mid(0,1)) || n.Mid(0,1) == "_" ) return false;
	for (int i=0, iend=n.size(); i<iend; i++)
		if ( !(isAlphaNum(n.Mid(i,1)) || n.Mid(i,1) == "_")
			|| n.Mid(i,1) == " " ) return false;
	return true;
}

bool DbfFileUtils::isAlphabetic(const wxString& n)
{
	wxUint32 n_val = (wxUint32) n.GetChar(0).GetValue();
	wxUint32 A = (wxUint32) wxString("A").GetChar(0).GetValue();
	wxUint32 z = (wxUint32) wxString("z").GetChar(0).GetValue();
	return (A <= n_val && n_val <= z);
}

bool DbfFileUtils::isDigit(const wxString& n)
{
	wxUint32 n_val = (wxUint32) n.GetChar(0).GetValue();
	wxUint32 zero = (wxUint32) wxString("0").GetChar(0).GetValue();
	wxUint32 nine = (wxUint32) wxString("9").GetChar(0).GetValue();
	return (zero <= n_val && n_val <= nine);
}

bool DbfFileUtils::isAlphaNum(const wxString& n)
{
	return isAlphabetic(n) || isDigit(n);
}

int DbfFileUtils::getNumRecords(const wxString& fname)
{
	std::ifstream file;
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
	if (!(file.is_open() && file.good())) return 0;
	
	wxUint32 u_int32;
	wxUint32* u_int32p = &u_int32;
	char membyte;
	
	file.seekg(0, std::ios::beg);
	file.get(membyte); // read version
	file.get(membyte); // read year
	file.get(membyte); // read month
	file.get(membyte); // read day
	file.read((char*) u_int32p, 4); // read number of records
	u_int32 = wxUINT32_SWAP_ON_BE(u_int32);
	int num_records = u_int32;
	
	file.close();
	return num_records;
}

int DbfFileUtils::getNumFields(const wxString& fname)
{
	std::ifstream file;
	file.open(fname.fn_str(), std::ios::in | std::ios::binary);
	if (!(file.is_open() && file.good())) return 0;

	// discover number of fields. MAX_NUMBER_FIELDS is the max
	file.seekg(32, std::ios::beg);
	int num_fields = 0;
	char first_byte;
	file.read(&first_byte, 1);
	while (first_byte != 0xd) { // && num_fields < 
		                        // DbfFileReader::MAX_NUMBER_FIELDS) {
		num_fields++;
		file.seekg(31, std::ios::cur);
		file.read(&first_byte, 1);
	}
	
	file.close();
	return num_fields;
}

wxInt64 DbfFileUtils::GetMaxInt(int length)
{
	// We want to allow the user to enter a string of
	// all 9s for the largest value reported.  So, we must
	// limit the length of the string to be floor(log(2^63)) = 18
	if (length < 1) return 0;
	if (length > 18) length = 18;
	wxInt64 r=0;
	for (int i=0; i<length; i++) r = r*10 + 9;
	return r;
}

wxString DbfFileUtils::GetMaxIntString(int length)
{
	return wxString::Format("%lld", GetMaxInt(length));
}

wxInt64 DbfFileUtils::GetMinInt(int length)
{
	// This is generally the -GetMaxInt(length-1), because we must
	// allow one character for the minus sign unless the length
	// is greater than 18;
	if (length > 19) length = 19;
	return -GetMaxInt(length-1);
}

wxString DbfFileUtils::GetMinIntString(int length)
{
	return wxString::Format("%lld", GetMinInt(length));
}

void DbfFileUtils::SuggestDoubleParams(int length, int decimals,
									   int* suggest_len, int* suggest_dec)
{
	// doubles have 52 bits for the mantissa, so we can allow at most
	// floor(log(2^52)) = 15 digits of precision.
	// We require that there length-2 >= decimals to allow for "x." . when
	// writing to disk, and when decimals = 15, require length >= 17 to
	// allow for "0." prefex. If length-2 == decimals, then negative numbers
	// are not allowed since there is not room for the "-0." prefix.
	if (GdaConst::max_dbf_double_len < length) {
		length = GdaConst::max_dbf_double_len;
	}
	if (length < 3) length = 3;
	if (decimals < 1) decimals = 1;
	if (decimals > 15) decimals = 15;
	if (length-2 < decimals) length = decimals + 2;
	
	*suggest_len = length;
	*suggest_dec = decimals;
}

double DbfFileUtils::GetMaxDouble(int length, int decimals,
								  int* suggest_len, int* suggest_dec)
{
	// make sure that length and decimals have legal values
	SuggestDoubleParams(length, decimals, &length, &decimals);

	int len_inter = length - (1+decimals);
	if (len_inter + decimals > 15) len_inter = 15-decimals;
	double r = 0;
	for (int i=0; i<len_inter+decimals; i++) r = r*10 + 9;
	for (int i=0; i<decimals; i++) r /= 10;
	
	if (suggest_len) *suggest_len = length;
	if (suggest_dec) *suggest_dec = decimals;
	return r;
}

wxString DbfFileUtils::GetMaxDoubleString(int length, int decimals)
{
	double x = GetMaxDouble(length, decimals, &length, &decimals);
	return wxString::Format("%.*f", decimals, x);
}

double DbfFileUtils::GetMinDouble(int length, int decimals,
								  int* suggest_len, int* suggest_dec)
{
	SuggestDoubleParams(length, decimals, &length, &decimals);
	if (length-2 == decimals) return 0;
	if (suggest_len) *suggest_len = length;
	if (suggest_dec) *suggest_dec = decimals;
	return -GetMaxDouble(length-1, decimals);
}

wxString DbfFileUtils::GetMinDoubleString(int length, int decimals)
{
	double x = GetMinDouble(length, decimals, &length, &decimals);
	if (length-2 == decimals) {
		wxString s("0.");
		for (int i=0; i<decimals; i++) s += "0";
		return s;
	}
	return wxString::Format("%.*f", decimals, x);
}



