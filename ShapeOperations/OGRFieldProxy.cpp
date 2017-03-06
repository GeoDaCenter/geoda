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
#include <vector>
#include <ogrsf_frmts.h>

#include "OGRFieldProxy.h"

OGRFieldProxy::OGRFieldProxy(const wxString& _name,
							 OGRFieldType ogr_type,
							 size_t _length,
							 size_t _decimals)
{
    is_field_changed = false;
	name = _name;
    length = _length;
    decimals = _decimals;
	type = GdaConst::string_type; // default is string

	if (ogr_type == OFTString){
		type = GdaConst::string_type;
	}
	else if (ogr_type == OFTInteger64 || ogr_type == OFTInteger) {
		type = GdaConst::long64_type;
	}
	else if (ogr_type == OFTReal) {
		type = GdaConst::double_type;
	}
    else if (ogr_type == OFTDate ) {
		type = GdaConst::date_type;
        
    } else if (ogr_type == OFTTime) {
		type = GdaConst::time_type;
        
    } else if (ogr_type == OFTDateTime) {
		type = GdaConst::datetime_type;
	}
	
	// create a OGRFieldDefn instance
	ogr_fieldDefn = new OGRFieldDefn( name.c_str(), ogr_type );
	ogr_fieldDefn->SetWidth(length);
	ogr_fieldDefn->SetPrecision(decimals);
}

OGRFieldProxy::OGRFieldProxy(OGRFieldDefn *field_defn)
{
    is_field_changed = false;
	ogr_fieldDefn = field_defn;
	
	name = field_defn->GetNameRef();
	OGRFieldType ogr_type  = field_defn->GetType();
	length = field_defn->GetWidth();
    decimals = field_defn->GetPrecision();
	type = GdaConst::string_type; // default is string
	
	if (ogr_type == OFTString){
		type = GdaConst::string_type;
	}
	else if (ogr_type == OFTInteger64 || ogr_type == OFTInteger) {
		type = GdaConst::long64_type;
	}
	else if (ogr_type == OFTReal) {
		type = GdaConst::double_type;
	}
    else if (ogr_type == OFTDate ) {
        type = GdaConst::date_type;
        
    } else if (ogr_type == OFTTime) {
        type = GdaConst::time_type;
        
    } else if (ogr_type == OFTDateTime) {
        type = GdaConst::datetime_type;
    }
}

OGRFieldProxy::~OGRFieldProxy()
{
    // we don't need to free OGRFieldDefn
    /*
	if ( ogr_fieldDefn != NULL) {
		delete ogr_fieldDefn;
		ogr_fieldDefn = NULL;
	}*/
}

void OGRFieldProxy::SetName(const wxString &new_name)
{
    if ( name.CmpNoCase(new_name)!=0 ) {
        name = new_name;
        is_field_changed = true;
    }
}
void OGRFieldProxy::SetLength(int new_len)
{
    if ( length != new_len ) {
        length = new_len;
        is_field_changed = true;
    }
}

void OGRFieldProxy::SetDecimals(int new_deci)
{
    if ( decimals != new_deci ) {
        decimals = new_deci;
        is_field_changed = true;
    }
}

void OGRFieldProxy::Update()
{
    if ( is_field_changed ) {
        ogr_fieldDefn->SetName(name.c_str());
        ogr_fieldDefn->SetWidth(length);
        ogr_fieldDefn->SetPrecision(decimals);
    }
}
