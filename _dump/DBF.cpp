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

#include <stdio.h>
#include <iomanip>
#include "DBF.h"
#include <wx/msgdlg.h>
#include "../GenUtils.h"

DBF_field::DBF_field(const char* buf)
	: Type(buf[11]), Width(buf[16]), Precision(buf[17])
{  
	memcpy(Name, buf, 11);
	Name[11]= '\x0';
	char* ch = Name;
	while (*ch)  {
		*ch = toupper(*ch);
		++ch;
	}
	if (Width <=0) {
		Width = 256 + Width;
	}
}
	
DBF_field::DBF_field(const wxString& nme, const char type,
					 const int wdth, const int prec)
	: Type(type), Width(wdth), Precision(prec)
{  
	strcpy(Name, nme.mb_str());
	Width = wdth;
}
	
void DBF_field::MakeBuffer(char *buf)  
{
	for (int cp= 0; cp < 32; ++cp) buf[cp]= '\x0';
	strcpy(buf, Name);
	buf[11]= Type;
	buf[16]= (char) Width;
	buf[17]= (char) Precision;
}

DBF::DBF(const wxString& fname, DBF_descr *ptr, const long nr, const int nf)
	: NumOfFields(nf), NumOfRecords(nr), Field(ptr), record(1), pos(-1)
{
	fn = GenUtils::swapExtension(fname, "dbf");
}

DBF::DBF(const wxString& fname, const wxString& dir)
	: record(1), pos(0), Field(NULL)
{
	fn = GenUtils::swapExtension(fname, "dbf");
}

DBF::~DBF()  {
	int cp= 0;
	/*
	 if (Field) {
	 while(cp < NumOfFields && Field[cp] != NULL)
	 delete Field[cp++];
	 delete [] Field;
	 Field= NULL;
	 };
	 */
}

DBF_descr* DBF::InitField(const wxString& nme, int ncols)
{
	DBF_descr *df= new DBF_descr[ncols];
	for (int cp= 0; cp < ncols; cp++)
		df[cp]= new DBF_field(nme, 'N');
	strcat(df[0]->Name, "ID");
	return df;
}

iDBF::iDBF(const wxString& fname, const wxString& dir)
	: std::ifstream(),
	DBF(fname, dir)  
{
	char sym;
	char buffer[32];
	int cp, maxfield= 19, HeaderSz= 0, RecordLength= 0;
	open(GET_ENCODED_FILENAME(fname), std::ios::in | std::ios::binary);
	connectedToFile = true;
	if (fail())	{
		wxString msg("iDBF::iDBF Error: wasn't able to open DBF file: ");
		msg << fname;
		connectedToFile = false;
		return;
	}
	get(sym);
	get(sym);             /* =year */
	get(sym);             /* =month */
	get(sym);             /* =day */
	read((char *) &NumOfRecords, 4);
#ifdef WORDS_BIGENDIAN
	NumOfRecords = GenUtils::Reverse(NumOfRecords);
#endif
	read((char *) &HeaderSz, 2);
#ifdef WORDS_BIGENDIAN
	HeaderSz = GenUtils::Reverse(HeaderSz);
#endif
	read((char *) &RecordLength, 2);
#ifdef WORDS_BIGENDIAN
	RecordLength = GenUtils::Reverse(RecordLength);
#endif
	read(buffer, 20);                             // skip 20 characters
	NumOfFields= (HeaderSz - 33) >> 5;
	//	wxString xx;xx.Format("File: %s\nNum fileds:%d",fname, NumOfFields);wxMessageBox(xx);
	Field = new DBF_descr[NumOfFields];
	for (cp= 0; cp < NumOfFields; cp++) {
		read(buffer, 32);
		Field[cp] = new DBF_field(buffer);
		if (Field[cp]->Width > maxfield) maxfield= Field[cp]->Width;
	}
	for(cp= HeaderSz-32*NumOfFields-32; cp > 0; cp--) get(sym);
	get(sym);
	
	if (sym == '\x0D') get(sym);
	// buf= new char[maxfield+1];
	// buf= new char[512];
}

iDBF::~iDBF()  
{
	
	if (Field) {
		for (int i=0; i<NumOfFields;i++) {
			if (Field[i]) delete Field[i]; // deleting DBF_field items in Field array.
			Field[i] = NULL;
		}
		delete [] Field;
		Field = NULL;
	}
	
	if (is_open()) {
		close();
	}
	
};

bool iDBF::ReOpen()
{

	if (is_open()) close();
	
	char sym;
	char  buffer[32];
	int cp, maxfield= 19, HeaderSz= 0, RecordLength= 0;
    //open(fn.mb_str(), std::ios::binary | std::ios::in);
	open(GET_ENCODED_FILENAME(fn), std::ios::in | std::ios::binary);
	connectedToFile = true;
	if (fail())
	{
		connectedToFile = false;
		return 0;
	}
	
	get(sym);
	get(sym);             /* =year */
	get(sym);             /* =month */
	get(sym);             /* =day */
	read((char *) &NumOfRecords, 4);
#ifdef WORDS_BIGENDIAN
    NumOfRecords = GenUtils::Reverse(NumOfRecords);
#endif
	read((char *) &HeaderSz, 2);
#ifdef WORDS_BIGENDIAN
    HeaderSz = GenUtils::Reverse(HeaderSz);
#endif
	read((char *) &RecordLength, 2);
#ifdef WORDS_BIGENDIAN
    RecordLength = GenUtils::Reverse(RecordLength);
#endif
	read(buffer, 20);                             // skip 20 characters
	NumOfFields= (HeaderSz - 33) >> 5;
	Field= new DBF_descr[NumOfFields];
	for (cp= 0; cp < NumOfFields; cp++) 
	{
		read(buffer, 32);
		Field[cp] = new DBF_field(buffer);
		if (Field[cp]->Width > maxfield) maxfield= abs(Field[cp]->Width);
	};
	for(cp= HeaderSz-32*NumOfFields-32; cp > 0; cp--) get(sym);
	get(sym);
	if (sym == '\x0D') get(sym);
	//  buf= new char[maxfield+1];
	//buf= new char[512];
	
	return 1;
}

int iDBF::prefix() 
{
	int width= Field[pos]->Width;
	read(buf, width);
	buf[width]= '\x0';
	if (fail()) readerr();
	return width;
}

void iDBF::postfix() 
{
	char sym;
	if (++pos == NumOfFields) {
		get(sym);
		++record;
		pos= 0;
	}
}

void iDBF::readerr()  
{
	//wxMessageBox("Error reading the dbf file");
	return;
}

char* iDBF::QuickReadDouble()  
{
	prefix();
	postfix();
	return buf;
}

char* iDBF::QuickReadLong()
{
	prefix();
	postfix();
	return buf;
}

char* iDBF::QuickReadString(int len)  
{
	int ln=prefix();
	if (len < ln) buf[len]= '\x0';
	postfix();
	return buf;
}


void iDBF::Read(double &v)  
{
	prefix();
	//v=atof(buf);
	double t;
	wxString::Format("%s", buf).ToCDouble(&t);
	v=t;
	postfix();
}

void iDBF::Read(long &v)  
{
	prefix();
	//v=atol(buf);
	long t;
	wxString::Format("%s", buf).ToCLong(&t);
	v=t;
	postfix();
}

void iDBF::Read(char *v, const int &len)  
{
	int ln= prefix();
	if (len < ln) buf[len]= '\x0';
	strcpy(v, buf);
	postfix();
}

/*
 FindField
 Returns column number which name contains sequence nme.
 If such sequence is not found, returns -1.
  */
int iDBF::FindField(const wxString& nme) {
	int cp = -1;
	for (cp= 0; cp < NumOfFields; cp++)
		if ( nme == wxString(Field[cp]->Name, wxConvUTF8) ) break;
	if (cp == NumOfFields) {
		cp = -1;
		wxString msg(nme);
		msg += " not found. Returning -1.";
	}
	return cp;
}

std::vector<wxString> iDBF::GetFieldNames() 
{
	std::vector<wxString> names(NumOfFields);
    for (int i=0; i < NumOfFields; i++)
		names[i] = wxString(Field[i]->Name, wxCSConv("utf-8"));
	return names;
}

charPtr iDBF::GetFieldName(int i) 
{
	return Field[i]->Name;
}

int* iDBF::GetFieldSizes() 
{	
	int* size = new int[NumOfFields];
	if (!size) return NULL;
    for (int cp= 0; cp < NumOfFields; ++cp)
		size[cp] = Field[cp]->Width;
	return size;
}

int iDBF::GetFieldSize(int i) 
{
	return Field[i]->Width;
}

int* iDBF::GetFieldPrecisions() 
{	
	int* prec = new int[NumOfFields];
	if (!prec) return NULL;
    for (int cp= 0; cp < NumOfFields; ++cp)
		prec[cp] = Field[cp]->Precision;
	return prec;
}

int iDBF::GetFieldPrecision(int i) 
{
	return Field[i]->Precision;
}

char* iDBF::GetFieldTypes() 
{	
	char* ty = new char [NumOfFields];
	if (!ty) return NULL;
	for (int cp= 0; cp < NumOfFields; ++cp)
		ty[cp] = Field[cp]->Type;
	return ty;
}

char iDBF::GetFieldType(int i) 
{
	return Field[i]->Type;
}

int iDBF::GetIndexField(const wxString& st) 
{
	return FindField(st);
}

bool iDBF::GetDblDataArray(const wxString& fieldname, double* dt) 
{
	
	int rows = NumOfRecords;
	int fld = FindField(fieldname);
	if (fld < 0 || fld > NumOfFields-1) 
		return 0;
	
	pos =0;
	record=0;
	//ReOpen();  //MMM this must be a hack, temporarily disable and eventually remove it!
	
	if (Field[fld]->Type == 'F')
	{
		
		for ( int xIndex = 0; xIndex < rows; ++xIndex) 
		{
			while (Pos() != fld) Read();
			int len = Field[fld]->Width;
			char* result = new char[len+1];
			Read(result, len);
			//dt[xIndex] = atof(result);
			wxString::Format("%s", result).ToCDouble(&dt[xIndex]);
			delete [] result;
			result = NULL;
		}
		
	}
	else
	{
		for ( int xIndex = 0; xIndex < rows; ++xIndex) 
		{
			while (Pos() != fld) Read();
			double result;
			Read(result);
			dt[xIndex] = result;
		}
	}
	
	return 1;
}

bool iDBF::GetStrDataArray(const wxString& fieldname, charPtr* dt) 
{
	int rows = NumOfRecords;
	int fld = FindField(fieldname);
	if (fld < 0 || fld > NumOfFields-1 || Field[fld]->Type != 'C') 
		return false;
	
	for ( int xIndex = 0; xIndex < rows; ++xIndex) 
	{
		while (Pos() != fld) Read();
		int len = Field[fld]->Width;
		char* result = new char[len+1];
		Read(result, len);
		dt[xIndex] = result;
	}
	
	return true;
}


wxString iDBF::PrintDescription()
{
	const int nf = GetNumOfField();
	wxString xx=wxEmptyString, x;
	for (int i=0;i<nf;i++)
	{
		char *nm = GetFieldName(i);
		char ty  = GetFieldType(i);
		int  sz  = GetFieldSize(i);
		int  pr  = GetFieldPrecision(i);
		x.Format("%12s  %c  %3d   %2d\n",nm,ty,sz,pr);
		xx += x;
	}
	return wxString(xx);
}

oDBF::oDBF(const wxString& fname, DBF_descr *descr, const long recs, const int fds)
	: std::ofstream(),
	DBF(fname, descr, recs, fds)
{
	fail = (OpenDBF() == -1);  
}

/*
 oDBF -- OpenDBF for output DBF
  */
int oDBF::OpenDBF()  
{
	char buffer[32];
	int cp, HeaderSz(32*NumOfFields+33), RecordLength(1);
	for (cp= 0; cp < NumOfFields; cp++)
		RecordLength += Field[cp]->Width;
	//int year = wxAtoi(wxDateTime::Now().FormatISODate().SubString(0, 3));
	long l_year;
	wxDateTime::Now().FormatISODate().SubString(0, 3).ToCLong(&l_year);
	int year = (int) l_year;
	//int month = wxAtoi(wxDateTime::Now().FormatISODate().SubString(5, 6));
	long l_month;
	wxDateTime::Now().FormatISODate().SubString(5, 6).ToCLong(&l_month);
	int month = (int) l_month;
	//int day = wxAtoi(wxDateTime::Now().FormatISODate().SubString(8, 9));
	long l_day;
	wxDateTime::Now().FormatISODate().SubString(8, 9).ToCLong(&l_day);
	int day = (int) l_day;
	open(GET_ENCODED_FILENAME(fn), std::ios::out | std::ios::binary);	
	//	_finddata_t *info = new _finddata_t;
	//	long s = _findfirst(fn, info);
	//	if (info->attrib == 33)
	//		return -1; // Read Only
	
	PutChar(3);
	PutChar(year);  PutChar(month);  PutChar(day);
	// see .\wxWidgets-2.8.7\configure(22485):#define WORDS_BIGENDIAN 1
#ifdef WORDS_BIGENDIAN
    NumOfRecords = GenUtils::Reverse(NumOfRecords);
#endif
	write((char *) &NumOfRecords, 4);
#ifdef WORDS_BIGENDIAN
    NumOfRecords = GenUtils::Reverse(NumOfRecords);
#endif
#ifdef WORDS_BIGENDIAN
    HeaderSz = GenUtils::Reverse(HeaderSz);
#endif
	write((char *) &HeaderSz, 2);
#ifdef WORDS_BIGENDIAN
    HeaderSz = GenUtils::Reverse(HeaderSz);
#endif
#ifdef WORDS_BIGENDIAN
    RecordLength = GenUtils::Reverse(RecordLength);
#endif
	write((char *) &RecordLength, 2);
#ifdef WORDS_BIGENDIAN
    RecordLength = GenUtils::Reverse(RecordLength);
#endif
	
	for (cp= 0; cp < 20; cp++) PutChar(0);
	for (cp= 0; cp < NumOfFields; cp++) 
	{
		Field[cp]->MakeBuffer(buffer);
		write(buffer, 32);
	};
	PutChar(13);
	PutChar(32);
	std::setprecision(9);
	return 1;
}

oDBF::~oDBF()  
{
	if (Field && good()) PutChar(26);
	close();
};

void oDBF::prefix() {
	++pos;
	if (pos == NumOfFields) 
	{
		++record;
		PutChar(32);
		pos= 0;
	};
	return;
}

void oDBF::Write(const wxInt32 v) 
{
	wxInt32 width;
	prefix();
	width= Field[pos]->Width;
	*this << std::setw(width) << v;
}

void oDBF::Write(const long int v) 
{
	int width;
	prefix();
	width= Field[pos]->Width;
	*this << std::setw(width) << v;
}

void oDBF::Write(const double v) 
{
	int width, prec;
	prefix();
	width= Field[pos]->Width;
	prec = Field[pos]->Precision;
	char c_buf[64];
	sprintf(c_buf, "%*.*f", width, prec, v);
	c_buf[width] = '\0';
	for (int i=0; i<width; i++) {
		if (c_buf[i] == '.') break;
		if (c_buf[i] == ',') {
			c_buf[i] = '.';
			break;
		}
	}
	*this << c_buf;
	//*this << std::setw(width) << std::setprecision(prec) << v;
}

void oDBF::Write(const char *v)  
{
	int width;
	prefix();
	width= Field[pos]->Width;
	*this << std::setw(width) << v;
}

void oDBF::Write(const wxString v)  
{
	int width;
	prefix();
	width= Field[pos]->Width;
	*this << std::setw(width) << v;
}
