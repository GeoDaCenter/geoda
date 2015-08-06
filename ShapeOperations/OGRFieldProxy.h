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

#ifndef __GEODA_CENTER_OGR_FIELD_PROXY_H__
#define __GEODA_CENTER_OGR_FIELD_PROXY_H__

#include <vector>
#include <ogrsf_frmts.h>
#include <wx/string.h>
#include "../GdaConst.h"

/**
 * A proxy class for a OGR column/field. It will store the meta data of a OGR 
 * field that read from a OGR data source, 
 */
class OGRFieldProxy {
public:
	/**
	 * Constuctor function. Construct OGRFieldProxy instance from an existed 
	 * OGRFieldDefn. Usually used when read a OGR Field from existing OGR
	 * data source.
	 */
	OGRFieldProxy(OGRFieldDefn *field_defn);
	
	/**
	 * Constuctor function. Construct OGRFieldProxy instance from related meta
	 * information. Usually used when create a new OGR Field
	 */
	OGRFieldProxy(const wxString& _name,
				  OGRFieldType ogr_type,
				  size_t _length,
				  size_t _decimals);
	
	~OGRFieldProxy();
	
public:
    wxString GetName() { return name; }
    
    void SetName(const wxString& new_name);
    
    OGRFieldType GetOGRType() { return ogr_fieldDefn->GetType(); }
    
    GdaConst::FieldType GetType() { return type; }
    
    size_t GetLength() {return length;}
    
    void SetLength(int new_len);
    
    size_t GetDecimals() { return decimals;}
   
    void SetDecimals(int new_deci);
    
    OGRFieldDefn* GetFieldDefn() {return ogr_fieldDefn;}
  
    bool IsChanged() { return is_field_changed; }
    
    void Update();
    
private:
	OGRFieldDefn* ogr_fieldDefn;
	wxString name;
	GdaConst::FieldType type;
    int length;
    int decimals;
    bool is_field_changed;
};



#endif