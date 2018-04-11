#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include <time.h>
#include <boost/multi_array.hpp>
#include <wx/filefn.h> // ::wxFileExists, ::wxCopyFile, etc.
#include <wx/log.h>
#include <wx/string.h>
#include <wx/textfile.h> // wxTextFile
#include <wx/tokenzr.h>
#include <wx/txtstrm.h> // wxTextInputStream
#include <wx/wfstream.h> // wxFileInputStream
#include "../../ShapeOperations/DbfFile.h"

using namespace std; // cout, cerr, clog
typedef boost::multi_array<wxString, 2> s_array_type;
typedef boost::multi_array<char*, 2> c_ptr_array_type;

void WriteDbfHeader(std::ofstream& out_file, const DbfFileHeader& header,
					const std::vector<DbfFieldDesc>& fd)
{
	// assumes that file is already open for reading
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
	u_int16 = header.header_length;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 10-11
	u_int16 = header.length_each_record;
	u_int16 = wxUINT16_SWAP_ON_BE(u_int16);
	out_file.write((char*) u_int16p, 2);
	
	// byte 12-13 (0x0000)
	u_int16 = 0x0;
	out_file.write((char*) u_int16p, 2);
	
	// bytes 14-31: write 0 
	membyte = 0;
	for (int i=0; i<(31-14)+1; i++) out_file.put(membyte);
	
	// out_file now points to byte 32, which is the beginning of the list
	// of fields.  There must be at least one field.  Each field descriptor
	// is 32 bytes long with byte 0xd following the last field descriptor.
	char* byte32_buff = new char[32];
	for (int i=0; i<header.num_fields; i++) {
		for (int j=0; j<32; j++) byte32_buff[j] = 0;
		strncpy(byte32_buff,
				(const char*)fd[i].name.mb_str(wxConvUTF8), 11);
		byte32_buff[11] = fd[i].type;
		byte32_buff[16] = (wxUint8) fd[i].length;
		byte32_buff[17] = (wxUint8) fd[i].decimals;
		out_file.write(byte32_buff, 32);
	}
	delete [] byte32_buff;
	// mark end of field descriptors with 0x0D
	out_file.put((char) 0x0D);	
}

int main(int argc, char **argv)
{
	cout << "An GeoDa utility for converting DBF to space-time DBFs\n";
	wxLog* logger = new wxLogStream(&std::cout);
	wxLog::SetActiveTarget(logger);
	//wxLogMessage("Sample Log Message, works with threads!");
	
	if (argc < 2) {
		cout << "Usage: sp_tm_conv <text config file>" << endl;
		delete logger;
		return 1;
	}
	// array variables:
	// vector<int> time_ids;
	//    list of time ids
	//
	// std::vector<DbfFieldDesc> orig_fd
	//    list of original DBF field descriptors
	//    name values are converted to uppercase
	//
	// std::map<wxString, DbfFieldDesc> orig_fd_nm_to_desc;
	//    map from original DBF field names to descriptors
	//
	// std::map<wxString, int> o_nm_to_col;
	//    map from orininal DBF field names to col id
	//
	// std::vector<wxString> time_fld_nms;
	//    list of new time field names, excluding space and time ids 
	//
	// int tm_fld_cnt
	//    number of new time fields exlcuding space and time fields
	//
	// s_array_type sp_tm_flds;
	//    boost::extents[tm_fld_cnt][time_steps]
	//    list of list of old name time fields
	//
	// std::map<wxString, bool> is_in_time_tbl;
	//    map from orig dbf field names to boolean
	//    does not include space_id_name, even though it is both tables
	//
	// std::map<wxString, int> tm_nm_to_col;
	// std::map<wxString, int> tm_nm_to_tm_step;
	// std::map<wxString, wxString> tm_nm_to_nm;
	//    maps every time field name from orig to a column, time_step val,
	//    and new column name
	//    excludes entries for space_id_name and time_id_name
	//    column mapping starts from 2 so that there is room for
	//    space and time columns
	//
	// std::map<wxString, int> sp_nm_to_col;
	//    maps every non space field name from orig to a column.
	//    excludes entry fro space_id.  space_id will always be the first
	//    column, so mapping starts from 1
	//
	// std::vector<wxString> new_sp_nm_col;
	//
    //
	// std::vector<char*> sp_data(sp_nm_to_col.size());
	//
	// c_ptr_array_type tm_data(boost::extents[time_steps][tm_nm_to_col.size()]);
	//
	
	wxString input_dbf;
	wxString output_dbf_sp;
	wxString output_dbf_tm; // derived from output_dbf_sp
	wxString space_id_name;
	wxString time_id_name;
	DbfFieldDesc space_id_desc;
	DbfFieldDesc time_id_desc;
	bool is_time_int = true; // otherwise, DBF date type
	int time_steps = 0;
	vector<int> time_ids;
	
	wxString fname(argv[1]);	
	wxFileInputStream file_is(fname);
	wxTextInputStream text_is(file_is);
	wxString line;
	wxString word;
	bool found_input = false;
	bool found_output = false;
	bool found_space_id = false;
	bool found_time_id = false;
	while (!file_is.Eof() && !(found_input && found_output &&
							   found_space_id && found_time_id)) {
		text_is >> word;
		if (word == "input:") {
			text_is >> input_dbf;
			cout << "input_dbf: " << input_dbf << endl;
			found_input = true;
		} else if (word == "output:") {
			text_is >> output_dbf_sp;
			output_dbf_tm = output_dbf_sp.BeforeLast('.');
			output_dbf_tm += "_time.dbf";
			cout << "output_dbf_sp: " << output_dbf_sp << endl;
			cout << "output_dbf_tm: " << output_dbf_tm << endl;
			found_output = true;
		} else if (word == "space-id:") {
			text_is >> space_id_name;
			space_id_name.MakeUpper();
			cout << "space_id_name: " << space_id_name << endl;
			found_space_id = true;
		} else if (word == "time-id:") {
			text_is	>> time_id_name;
			time_id_name.MakeUpper();
			text_is >> word;
			is_time_int = (word.CmpNoCase("integer") == 0);
			cout << "time_id_name: " << time_id_name << endl;
			cout << "is_time_int: " << (is_time_int ? "true" : "false") << endl;
			wxStringTokenizer tkns(text_is.ReadLine());
			// tkns contains a sequence of space-seperated ints or dates
			time_steps = tkns.CountTokens();
			time_ids.resize(time_steps);
			int tkn_cnt = 0;
			while (tkns.HasMoreTokens()) {
				long val;
				tkns.GetNextToken().ToLong(&val);
				time_ids[tkn_cnt++] = (int) val;
			}
			cout << "time_steps: " << time_steps << endl;
			cout << "Time ids: ";
			for (int i=0; i<time_steps; i++) {
				cout << time_ids[i] << " ";
			}
			cout << endl;
			found_time_id = true;
		}
	}
	
	DbfFileReader orig_reader(input_dbf);
	if (!orig_reader.isDbfReadSuccess()) {
		wxLogMessage("Error: could not read " + input_dbf);
		return 1;
	}
	DbfFileHeader orig_header = orig_reader.getFileHeader();
	std::vector<DbfFieldDesc> orig_fd = orig_reader.getFieldDescs();
	for (size_t i=0, iend=orig_fd.size(); i<iend; i++) {
		orig_fd[i].name.MakeUpper();
	}
	
	std::map<wxString, DbfFieldDesc> orig_fd_nm_to_desc;
	for (int i=0, iend=orig_fd.size(); i<iend; i++) {
		orig_fd_nm_to_desc[orig_fd[i].name] = orig_fd[i];
	}
	
	if (orig_fd_nm_to_desc.find(space_id_name) == orig_fd_nm_to_desc.end()) {
		wxString msg;
		msg << "Error, specified space-id " << space_id_name << " does not";
		msg << " exist in the input DBF file.  Aborting operation.";
		wxLogMessage(msg);
		return 1;
	} else {
		//wxString msg;
		//msg << "space-id " << space_id_name << " found in " << input_dbf;
		//msg << " with type " << orig_fd_nm_to_desc[space_id_name].type;
		//wxLogMessage(msg);
		space_id_desc = orig_fd_nm_to_desc[space_id_name];
	}
	
	std::map<wxString, int> o_nm_to_col;
	bool duplicate = false;
	for (size_t i=0, iend=orig_fd.size(); i<iend && !duplicate; i++) {
		if (o_nm_to_col.find(orig_fd[i].name) == o_nm_to_col.end()) {
			o_nm_to_col[orig_fd[i].name] = (int) i;
		} else {
			duplicate = true;
		}
	}
	if (duplicate) {
		wxLogMessage("Error: DBF file has duplicate field names.");
		return 1;
	}
	
	std::vector<wxString> time_fld_nms;
	s_array_type sp_tm_flds(boost::extents[orig_header.num_fields][time_steps]);
	
	int tm_fld_cnt = 0;
	while (!file_is.Eof()) {
		text_is >> word;
		if (!word.IsEmpty()) {
			time_fld_nms.push_back(word.Upper());
			wxStringTokenizer tkns(text_is.ReadLine());
			// tkns contains a sequence of space-seperated field names
			int tkn_cnt = 0;
			while (tkns.HasMoreTokens() && tkn_cnt < time_steps) {
				sp_tm_flds[tm_fld_cnt][tkn_cnt] = tkns.GetNextToken().Upper();
				if (orig_fd_nm_to_desc.find(sp_tm_flds[tm_fld_cnt][tkn_cnt])
					== orig_fd_nm_to_desc.end()) {
					wxString msg;
					msg << "Error, field name ";
					msg << sp_tm_flds[tm_fld_cnt][tkn_cnt] << " not found ";
					msg << "in input DBF file. Aborting operation.";
					wxLogMessage(msg);
					exit(1);
				}
				tkn_cnt++;
			}
			tm_fld_cnt++;
		}
	}
	
	sp_tm_flds.resize(boost::extents[tm_fld_cnt][time_steps]);
	std::map<wxString, bool> is_in_time_tbl;
	
	const int MAX_NUM_FLD_LEN = 18;
	time_id_desc.name = time_id_name;
	time_id_desc.type = 'N';
	time_id_desc.length = MAX_NUM_FLD_LEN;
	time_id_desc.decimals = 0;
	
	//wxLogMessage(wxString::Format("(%d, %d)", tm_fld_cnt, time_steps));
	for (int i=0; i<tm_fld_cnt; i++) {
		//wxString msg;
		//msg << time_fld_nms[i] << ": ";
		for (int j=0; j<time_steps; j++) {
			//msg << sp_tm_flds[i][j] << " ";
			is_in_time_tbl[sp_tm_flds[i][j]] = true;
		}
		//wxLogMessage(msg);
	}
	
	for (int i=0, iend=orig_fd.size(); i<iend; i++) {
		if (is_in_time_tbl.find(orig_fd[i].name) == is_in_time_tbl.end()) {
			is_in_time_tbl[orig_fd[i].name] = false;
		}
	}
	
	std::vector<DbfFieldDesc> space_fd;
	std::vector<DbfFieldDesc> time_fd;
	
	space_fd.push_back(space_id_desc);
	time_fd.push_back(space_id_desc);
	time_fd.push_back(time_id_desc);
	
	// map every time field name to the correct column number
	std::map<wxString, int> tm_nm_to_col;
	std::map<wxString, int> tm_nm_to_tm_step;
	std::map<wxString, wxString> tm_nm_to_nm;
	bool field_desc_missmatch = false;
	wxString missmatch_fld1;
	wxString missmatch_fld2;
	for (int i=0; i<tm_fld_cnt; i++) {
		//wxLogMessage(sp_tm_flds[i][0]);
		char type = orig_fd_nm_to_desc[sp_tm_flds[i][0]].type;
		int length = orig_fd_nm_to_desc[sp_tm_flds[i][0]].length;
		int decimals = orig_fd_nm_to_desc[sp_tm_flds[i][0]].decimals;
		time_fd.push_back(orig_fd_nm_to_desc[sp_tm_flds[i][0]]);
		for (int j=0; j<time_steps; j++) {
			tm_nm_to_col[sp_tm_flds[i][j]] = 2 + i; // need room for spc/tm col
			tm_nm_to_tm_step[sp_tm_flds[i][j]] = j;
			tm_nm_to_nm[sp_tm_flds[i][j]] = time_fld_nms[i];
			//wxLogMessage(orig_fd_nm_to_desc[sp_tm_flds[i][0]].name);
			if ( type != orig_fd_nm_to_desc[sp_tm_flds[i][j]].type ||
				length != orig_fd_nm_to_desc[sp_tm_flds[i][j]].length ||
				decimals != orig_fd_nm_to_desc[sp_tm_flds[i][j]].decimals ) {
				field_desc_missmatch = true;
				missmatch_fld1 = orig_fd_nm_to_desc[sp_tm_flds[i][0]].name;
				missmatch_fld2 = orig_fd_nm_to_desc[sp_tm_flds[i][j]].name;
			}
		}
	}
	if (field_desc_missmatch) {
		wxString msg;
		msg << "Field attributes for " << missmatch_fld1 << " and ";
		msg << missmatch_fld2 << " do not match.\n";
		msg << missmatch_fld1 << " type: ";
		msg << orig_fd_nm_to_desc[missmatch_fld1].type << ", length: ";
		msg << orig_fd_nm_to_desc[missmatch_fld1].length << ", decimals: ";
		msg << orig_fd_nm_to_desc[missmatch_fld1].decimals << "\n";
		msg << missmatch_fld2 << " type: ";
		msg << orig_fd_nm_to_desc[missmatch_fld2].type << ", length: ";
		msg << orig_fd_nm_to_desc[missmatch_fld2].length << ", decimals: ";
		msg << orig_fd_nm_to_desc[missmatch_fld2].decimals << "\n";
		" Aborting operation.";
		wxLogMessage(msg);
		exit(1);
	}
	
	for (int i=0; i<time_fld_nms.size(); i++) {
		time_fd[i+2].name = time_fld_nms[i].Upper();
	}
	
	// need to map every space field name to the correct column number
	std::map<wxString, int> sp_nm_to_col;
	std::vector<wxString> new_sp_nm_col;
	int sp_col_cnt = 1; // first column is always for space_id
	for (int i=0, iend=orig_fd.size(); i<iend; i++) {
		if (!is_in_time_tbl[orig_fd[i].name] &&
			orig_fd[i].name != space_id_name)
		{
			new_sp_nm_col.push_back(orig_fd[i].name);
			sp_nm_to_col[orig_fd[i].name] = sp_col_cnt++;
			space_fd.push_back(orig_fd[i]);
		}
	}
	
	//{wxString msg;
	//msg << "new_sp_nm_col.size() = " << new_sp_nm_col.size() << ", ";
	//msg << "sp_nm_to_col.size() = " << sp_nm_to_col.size();
	//wxLogMessage(msg);}
		
	/*
	for (int i=0, iend=orig_fd.size(); i<iend; i++) {
		wxString cur_nm = orig_fd[i].name;
		wxString msg;
		if (cur_nm == space_id_name) {
			msg << "col " << i << ", " << cur_nm << " is the space_id col";
			msg << " and maps to col 0";
			wxLogMessage(msg);
		} else if (is_in_time_tbl[cur_nm]) {
			msg << "col " << i << ", " << cur_nm;
			msg << " is in time and maps to col " << tm_nm_to_col[cur_nm];
			msg << ", name " << tm_nm_to_nm[cur_nm] << " and time offset ";
			msg << tm_nm_to_tm_step[cur_nm];
			wxLogMessage(msg);
		} else {
			msg << "col " << i << ", " << cur_nm;
			msg << " is in space and maps to col " << sp_nm_to_col[cur_nm];
			wxLogMessage(msg);
		}
	}
	 */
	
	// We now have fast mapping to space / time tbl and to
	// column id and to row offset for table.
	
	// make a temporary space row structure and time row structure
	// time row will be time_steps by time cols.  Will make this
	// as a char arrays to avoid any loss of info.  Also need to
	// make field length table.  Make sure that length is max of
	// all colums in a column group.  Decimals places must be compatible,
	// will otherwise truncate to fit given allowable length.
	
	std::vector<char*> sp_data(sp_nm_to_col.size() + 1);
	c_ptr_array_type tm_data(boost::extents[time_steps][tm_nm_to_col.size() + 2]);
	
	sp_data[0] = new char[space_id_desc.length];
	for (int i=0; i<time_steps; i++) {
		tm_data[i][0] = new char[space_id_desc.length];
		tm_data[i][1] = new char[time_id_desc.length];
	}
	
	for (int i=0, iend=orig_fd.size(); i<iend; i++) {
		wxString cur_nm = orig_fd[i].name;
		wxString msg;
		if (cur_nm == space_id_name) {
			// already allocated, so skip
		} else if (is_in_time_tbl[cur_nm]) {
			tm_data[tm_nm_to_tm_step[cur_nm]][tm_nm_to_col[cur_nm]] =
				new char[orig_fd_nm_to_desc[cur_nm].length];
		} else {
			sp_data[sp_nm_to_col[cur_nm]] =
				new char[orig_fd_nm_to_desc[cur_nm].length];
		}
	}
	
	// Write new time and space DBFs
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	DbfFileHeader space_header;
	space_header.version = 3; // dBASE III+
	space_header.year = timeinfo->tm_year+1900;
	space_header.month = timeinfo->tm_mon+1;
	space_header.day = timeinfo->tm_mday;
	space_header.num_records = orig_header.num_records;
	space_header.num_fields = space_fd.size();
	space_header.header_length = 32 + space_header.num_fields*32 + 1;
	space_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	space_header.length_each_record += space_id_desc.length;
	for (int i=0; i<space_fd.size(); i++) {
		space_header.length_each_record += space_fd[i].length;
	}
		
	DbfFileHeader time_header;
	time_header.version = 3; // dBASE III+
	time_header.year = timeinfo->tm_year+1900;
	time_header.month = timeinfo->tm_mon+1;
	time_header.day = timeinfo->tm_mday;
	time_header.num_records = orig_header.num_records * time_steps;
	time_header.num_fields = time_fd.size();
	time_header.header_length = 32 + time_header.num_fields*32 + 1;
	time_header.length_each_record = 1; // first byte is either 0x20 or 0x2A
	for (int i=0; i<time_fd.size(); i++) {
		time_header.length_each_record += time_fd[i].length;
	}
	
	if (!orig_reader.file.is_open()) {
		orig_reader.file.open(input_dbf.mb_str(wxConvUTF8),
							  std::ios::in | std::ios::binary);
	}
	if (!(orig_reader.file.is_open() && orig_reader.file.good())) {
		wxLogMessage("Could not open input DBF for reading");
		exit(1);
	}

	int max_fld_len = 255;
	for (int col=0; col<orig_header.num_fields; col++) {
		if (orig_fd[col].length > max_fld_len) {
			max_fld_len = orig_fd[col].length;
		}
	}
	char temp_buf[max_fld_len+1];
		
	// time ids never change, so fill in now
	for (int tm=0; tm<time_steps; tm++) {
		sprintf(temp_buf, "%*d", MAX_NUM_FLD_LEN, (int) time_ids[tm]);
		memcpy(tm_data[tm][1], temp_buf, MAX_NUM_FLD_LEN);
		//wxString msg;
		//msg << "\"" << wxString(temp_buf) << "\"";
		//wxLogMessage(msg);
	}
	
	// Open space and time dbf files for writing.  Will simply overwrite
	// if already exist
	if (wxFileExists(output_dbf_sp) && !wxRemoveFile(output_dbf_sp)) {
		wxString msg("Error: unable to overwrite ");
		msg << output_dbf_sp;
		exit(1);
	}
	if (wxFileExists(output_dbf_tm) && !wxRemoveFile(output_dbf_tm)) {
		wxString msg("Error: unable to overwrite ");
		msg << output_dbf_tm;
		exit(1);
	}
	
	std::ofstream out_file_sp;
	out_file_sp.open(output_dbf_sp.mb_str(wxConvUTF8),
				  std::ios::out | std::ios::binary);
	if (!(out_file_sp.is_open() && out_file_sp.good())) {
		wxString msg("Error: Problem opening ");
		msg << output_dbf_sp;
		exit(1);
	}

	std::ofstream out_file_tm;
	out_file_tm.open(output_dbf_tm.mb_str(wxConvUTF8),
				  std::ios::out | std::ios::binary);
	if (!(out_file_tm.is_open() && out_file_tm.good())) {
		wxString msg("Error: Problem opening ");
		msg << output_dbf_tm;
		exit(1);
	}
	
	WriteDbfHeader(out_file_sp, space_header, space_fd);
	WriteDbfHeader(out_file_tm, time_header, time_fd);
	
	// Note: first byte of every DBF row is the record deletion flag, so
	// we always skip this.
	int del_flag_len = 1;  // the record deletion flag
	orig_reader.file.seekg(orig_header.header_length, std::ios::beg);
	for (int row=0; row<orig_header.num_records; row++) {
		//orig_reader.file.seekg(del_flag_len, std::ios::cur);
		orig_reader.file.read((char*) temp_buf, del_flag_len);
		for (int col=0; col<orig_header.num_fields; col++) {
			char* buf = 0;
			int field_len = orig_fd[col].length;
			wxString cur_nm = orig_fd[col].name;
			
			wxString msg;
			if (cur_nm == space_id_name) {
				orig_reader.file.read(sp_data[0], field_len);
				for (int tm=0; tm<time_steps; tm++) {
					memcpy(tm_data[tm][0], sp_data[0], field_len);
				}
			} else if (is_in_time_tbl[cur_nm]) {
				buf = tm_data[tm_nm_to_tm_step[cur_nm]][tm_nm_to_col[cur_nm]];
				orig_reader.file.read(buf, field_len);
			} else {
				buf = sp_data[sp_nm_to_col[cur_nm]];
				orig_reader.file.read(buf, field_len);
			}
		}
		
		// Write out row of space dbf and time_steps rows of time dbf

		// each record starts with a space character
		out_file_sp.put((char) 0x20);
		for (int col=0, cols=space_fd.size(); col<cols; col++) {
			out_file_sp.write(sp_data[col], space_fd[col].length);
		}
		
		for (int tm=0; tm<time_steps; tm++) {
			// each record starts with a space character
			out_file_tm.put((char) 0x20);
			for (int col=0, cols=time_fd.size(); col<cols; col++) {
				//cout << "tm_data[" << tm << "]["<< col << "] = ";
				//cout << tm_data[tm][col] << endl;
				out_file_tm.write(tm_data[tm][col], time_fd[col].length);
			}
		}		
	}
	
	// 0x1A is the EOF marker
	out_file_sp.put((char) 0x1A);
	out_file_sp.close();
	out_file_tm.put((char) 0x1A);
	out_file_tm.close();
	
	/*
	for (int col=0, cols=space_fd.size(); col<cols; col++) {
		wxString msg;
		memcpy(temp_buf, sp_data[col], space_fd[col].length);
		temp_buf[space_fd[col].length] = 0;
		msg << space_fd[col].name << ": " << wxString(temp_buf);
		wxLogMessage(msg);
	}

	for (int col=0, cols=time_fd.size(); col<cols; col++) {
		for (int tm=0; tm<time_steps; tm++) {
			wxString msg;
			memcpy(temp_buf, tm_data[tm][col], time_fd[col].length);
			temp_buf[time_fd[col].length] = 0;
			msg << time_fd[col].name << ": " << wxString(temp_buf);
			wxLogMessage(msg);
		}
	}
	 */
	
	wxLogMessage("Success.");
	
	for (int i=0; i<tm_data.shape()[0]; i++) {
		for (int j=0; j<tm_data.shape()[1]; j++) {
			delete [] tm_data[i][j];
		}
	}
	for (int i=0; i<sp_data.size(); i++) delete [] sp_data[i];
	delete logger;

	return 0;
}
