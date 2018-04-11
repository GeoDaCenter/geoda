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

#ifndef __GEODA_CENTER_DBF_H__
#define __GEODA_CENTER_DBF_H__

#include <fstream>
#include <wx/string.h>
#include <vector>

/*
 DBF_field
 Descriptor for a single field in the DBF file.
 Name - name of the field; Type - its type ('C' for text, 'N' for
 number, 'D' is for date); Width - width of the field (in bytes);
 Precision - precision (0 for 'C' format).
  */
struct DBF_field {
	char Name[20];
	char Type;
	int Width, Precision;

	DBF_field(const char *buf);
	DBF_field(const wxString& nme, const char type,
		const int wdth=8, const int prec= 0);
	void MakeBuffer(char *buf);
};

typedef DBF_field * DBF_descr;

/*
 DBF
 Describes DBF common features of DBF files.
 NumOfFields     - the number of fields (columns) in the DBF table;
 pos             - current position (column) in the table;
 NumOfRecords    - the total number of records (rows) in the DBF file;
 record          - current position in the table;
 fn              - file name;
 Field - a vector[NumOfFields] that contains descriptors for each field.
  */
class DBF {
protected:
	long int      NumOfRecords, record;
	int           NumOfFields, pos;
	wxString      fn;
	DBF_descr* Field;
public:
	DBF(const wxString& fname, DBF_descr* ptr, const long nr, const int nf); 
	DBF(const wxString& fname, const wxString& dir = wxEmptyString);
	virtual ~DBF();
	int Pos() const { return pos; };
	long Records() const { return NumOfRecords; }
	DBF_descr field(int ff) const { return Field[ff]; }
	static DBF_descr* InitField(const wxString& nme, int ncols); 
};

typedef char* charPtr;
/*
 iDBF
 Describes DBF files opened for input.
 buf - unformatted contents of the field;
  */
class iDBF : public std::ifstream, public DBF {
private :
	char buf[512];
	// housekeeping functions (can not be called from outside)
	int prefix();                         // prepares next field for reading
	void readerr();                       // generates an error message if one occurs
	void postfix();                       // completes reading for the record
	bool ReOpen();
	FILE _dbf;
	bool connectedToFile;  // true if was able to open DBF file.
public :
	wxString PrintDescription();
	iDBF(const wxString& fname, const wxString& dir = wxEmptyString);
	virtual ~iDBF();
	void Read() { prefix(); postfix(); return; };
	char* QuickReadDouble();
	char* QuickReadLong();
	char* QuickReadString(int len);	
	void Read(double& v);
	void Read(long& v);
	void Read(char* v, const int& len);
	int FindField(const wxString& nme);
	std::vector<wxString> GetFieldNames();
	int*		GetFieldPrecisions();
	char*		GetFieldTypes();
	int*		GetFieldSizes();
	int			GetNumOfField() { return NumOfFields; };
	int			GetNumOfRecord() { return NumOfRecords; };
	bool		GetDblDataArray(const wxString& fieldname, double* dt);
	bool		GetStrDataArray(const wxString& fieldname, charPtr* dt);
	bool		IsConnectedToFile() { return connectedToFile; }
	charPtr		GetFieldName(int i);
	int			GetFieldPrecision(int i);
	char		GetFieldType(int i);
	int			GetFieldSize(int i);
	int			GetIndexField(const wxString& st);	
};

/*
 oDBF
 Class to create a DBF file.
  */
class oDBF : public std::ofstream, public DBF {
	// housekeeping functions (can not be called from outside)
	void PutChar(const int &v)  { put((char) v); return; };
	void prefix();
	int OpenDBF();
	
public :
	bool fail;
	oDBF(const wxString& fname, DBF_descr *descr,
		 const long recs=1, const int fds=2);
	virtual ~oDBF();
	void Write(const wxInt32 v);	
	void Write(const long int v);
	void Write(const double v);
	void Write(const char *v);
	void Write(const wxString v);
};

#endif
