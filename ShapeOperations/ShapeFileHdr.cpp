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

#include "ShapeFileHdr.h"
#include "../GdaConst.h"
#include "../GenUtils.h"

#ifndef GDA_SWAP
#define GDA_SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
#endif

ShapeFileHdr::ShapeFileHdr(const ShapeFileTypes::ShapeType FileShape)
	: FileCode(kFileCode), Version(kVersion),
	  FileLength(GdaConst::ShpHeaderSize), fShape(FileShape)
{
}

ShapeFileHdr::ShapeFileHdr(const char* s)
{
    //MMM: very dangerous.  This is where the problems begin
    // on 64 bit builds!

	HdrRecord *hr= (HdrRecord *) s;
#ifdef WORDS_BIGENDIAN
	FileCode = hr->f[0];
#else
	FileCode = GenUtils::Reverse(hr->f[0]);
#endif
#ifdef WORDS_BIGENDIAN
	FileLength = hr->f[6];
#else
	FileLength = GenUtils::Reverse(hr->f[6]);
        HdrRecord64 *hr64=(HdrRecord64 *) s;
        wxInt32 x = GenUtils::ReverseInt(hr64->f[6]);
#endif
#ifdef WORDS_BIGENDIAN
	Version = GenUtils::Reverse(hr->f[7]);
#else
	Version = hr->f[7];
#endif
#ifdef WORDS_BIGENDIAN
	fShape = GenUtils::Reverse(hr->f[8]);
#else
	fShape = hr->f[8];
#endif
#ifdef WORDS_BIGENDIAN
	char r[32], t;
	memcpy(&r[0], &s[36], sizeof(double) * 4);
	double m1, m2, n1, n2;
	GDA_SWAP(r[0], r[31], t);
	GDA_SWAP(r[1], r[30], t);
	GDA_SWAP(r[2], r[29], t);
	GDA_SWAP(r[3], r[28], t);
	GDA_SWAP(r[4], r[27], t);
	GDA_SWAP(r[5], r[26], t);
	GDA_SWAP(r[6], r[25], t);
	GDA_SWAP(r[7], r[24], t);
	GDA_SWAP(r[8], r[23], t);
	GDA_SWAP(r[9], r[22], t);
	GDA_SWAP(r[10], r[21], t);
	GDA_SWAP(r[11], r[20], t);
	GDA_SWAP(r[12], r[19], t);
	GDA_SWAP(r[13], r[18], t);
	GDA_SWAP(r[14], r[17], t);
	GDA_SWAP(r[15], r[16], t);
	memcpy(&m1, &r[24], sizeof(double));
	memcpy(&m2, &r[16], sizeof(double));
	memcpy(&n1, &r[8], sizeof(double));
	memcpy(&n2, &r[0], sizeof(double));
	BasePoint p1 = BasePoint(m1, m2);
	BasePoint p2 = BasePoint(n1, n2);
	FileBox = Box(p1, p2);
	hr->b = FileBox;
#else
	memcpy(&FileBox, &s[36], sizeof(double)*4);
	hr->b = FileBox;
#endif
}

void ShapeFileHdr::SetFileBox(const Box& fBox) 
{
	//LOG_MSG("Entering ShapeFileHdr::SetFileBox");	
	FileBox = fBox;
	//LOG(FileBox.Bmin.x);
	//LOG(FileBox.Bmin.y);
	//LOG(FileBox.Bmax.x);
	//LOG(FileBox.Bmax.y);
	//LOG_MSG("Exiting ShapeFileHdr::SetFileBox");
}

void ShapeFileHdr::SetFileLength(wxInt32 fl) 
{
	FileLength = fl;
}

void ShapeFileHdr::MakeBuffer(char* s) const
{
	HdrRecord * hr= (HdrRecord *) s;
	
	wxInt32 *ptr= (wxInt32 *) s;
	int cp;
	for (cp= 0; cp < GdaConst::ShpHeaderSize/2; ++cp) ptr[cp]= 0;
#ifdef WORDS_BIGENDIAN
	hr->f[0] = FileCode;
	hr->f[6] = FileLength;
	hr->f[7] = GenUtils::Reverse(Version);
	hr->f[8] = GenUtils::Reverse(fShape);
	hr->b = FileBox;
	char r[32], t;
	// MMM: This can't be correct.  Verify this!
	GDA_SWAP(r[0], r[31], t);
	GDA_SWAP(r[1], r[30], t);
	GDA_SWAP(r[2], r[29], t);
	GDA_SWAP(r[3], r[28], t);
	GDA_SWAP(r[4], r[27], t);
	GDA_SWAP(r[5], r[26], t);
	GDA_SWAP(r[6], r[25], t);
	GDA_SWAP(r[7], r[24], t);
	GDA_SWAP(r[8], r[23], t);
	GDA_SWAP(r[9], r[22], t);
	GDA_SWAP(r[10], r[21], t);
	GDA_SWAP(r[11], r[20], t);
	GDA_SWAP(r[12], r[19], t);
	GDA_SWAP(r[13], r[18], t);
	GDA_SWAP(r[14], r[17], t);
	GDA_SWAP(r[15], r[16], t);
	memcpy(&r[0], &s[36], sizeof(double) * 4);
#else
	hr->f[0]= GenUtils::Reverse(FileCode);
	hr->f[6]= GenUtils::Reverse(FileLength);
	hr->f[7]= Version;
	hr->f[8]= fShape;
	hr->b = FileBox;
	memcpy(&s[36], &hr->b, sizeof(double)*4);
#endif
}

void ShapeFileHdr::Replace (const wxString& fname, const wxInt32& recs)  
{
	// may have problems when writing
	HdrRecord buf;
	MakeBuffer((char *) &buf);
	
	// update *.shp file
	FILE *shp = fopen(
		GenUtils::swapExtension(fname, "shp"), "rb+");
	fseek(shp, 0, SEEK_SET);
	fwrite((char*)&buf, sizeof(char), GdaConst::ShpHeaderSize * 2, shp);
	fclose(shp);
	
	// update *.shx file
	FILE *shx = fopen(
		GenUtils::swapExtension(fname, "shx"), "rb+");
#ifdef WORDS_BIGENDIAN
	buf.f[6] = GdaConst::ShpHeaderSize + 4 * recs;
#else
	buf.f[6]= GenUtils::Reverse(GdaConst::ShpHeaderSize + 4 * recs);
#endif
	fseek(shx, 0, SEEK_SET);
	fwrite((char*)&buf, sizeof(char), GdaConst::ShpHeaderSize * 2, shx);
	fclose(shx);
	
	// update *.dbf file
	FILE *dbf = fopen(
		GenUtils::swapExtension(fname, "dbf"), "rb+");
	wxInt32 nrec;
	fseek(dbf, 4, SEEK_SET);
#ifdef WORDS_BIGENDIAN
	nrec = GenUtils::Reverse(recs);
#else
	nrec = recs;
#endif
	fwrite(&nrec, sizeof(wxInt32), 1, dbf);
#ifdef WORDS_BIGENDIAN
	nrec = GenUtils::Reverse(recs);
#endif
	fclose(dbf);
}

ShapeFileHdr& operator<<(ShapeFileHdr& hd, const AbstractShape& s)  
{
	Box bo;
	bo = hd.BoundingBox();
	if (hd.Length() == GdaConst::ShpHeaderSize) { 
		hd.SetFileBox(s.ShapeBox());
	} else {
		bo += s.ShapeBox();
		hd.SetFileBox(bo);
	}
	wxInt32 fl = hd.Length();
	fl += 4 + s.ContentsLength();
	hd.SetFileLength(fl);
	return hd;
}

oShapeFile& operator<<(oShapeFile& s, const ShapeFileHdr& hd)
{
	char buf[GdaConst::ShpHeaderSize*2];
	hd.MakeBuffer(buf);
	s.write(buf, 2*GdaConst::ShpHeaderSize);
	return s;
}
