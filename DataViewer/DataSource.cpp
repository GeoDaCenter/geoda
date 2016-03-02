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

#include <set>
#include <boost/property_tree/ptree.hpp>
#include <wx/filename.h>
#include "../logger.h"
#include "../GdaException.h"
#include "../GdaConst.h"
#include "../GenUtils.h"
#include "DataSource.h"

using boost::property_tree::ptree;
using namespace std;


//------------------------------------------------------------------------------
// IDataSource static functions
//------------------------------------------------------------------------------
bool IDataSource::IsWritable(GdaConst::DataSourceType ds_type)
{
    if (ds_type == GdaConst::ds_shapefile ||
        ds_type == GdaConst::ds_csv ||
        ds_type == GdaConst::ds_dbf ||
        ds_type == GdaConst::ds_esri_file_geodb ||
        ds_type == GdaConst::ds_geo_json ||
        ds_type == GdaConst::ds_kml ||
        ds_type == GdaConst::ds_gml ||
        ds_type == GdaConst::ds_mapinfo ||
        ds_type == GdaConst::ds_sqlite ||
        ds_type == GdaConst::ds_mysql ||
        ds_type == GdaConst::ds_oci ||
        ds_type == GdaConst::ds_postgresql )
        return true;
    return false;
}

bool IDataSource::IsTableOnly(GdaConst::DataSourceType ds_type)
{
    if (ds_type == GdaConst::ds_csv ||
        ds_type == GdaConst::ds_dbf ||
        ds_type == GdaConst::ds_xls ||
        ds_type == GdaConst::ds_xlsx )
        return true;
    return false;
}

wxString IDataSource::GetDataTypeNameByExt(wxString ext)
{
    wxString ds_format;
    if (ext.CmpNoCase("shp")==0)
        ds_format = "ESRI Shapefile";
	else if(ext.CmpNoCase("dbf")==0)
		ds_format = "DBF";
    else if(ext.CmpNoCase("tab")==0 ||
            ext.CmpNoCase("mif")==0 || ext.CmpNoCase("mid")==0 )
        ds_format = "MapInfo File";
    else if(ext.CmpNoCase("csv")==0)
        ds_format = "CSV";
    else if(ext.CmpNoCase("mdb")==0)
        ds_format = "PGeo";
    else if(ext.CmpNoCase("gdb")==0)
        ds_format = "FileGDB";
    else if(ext.CmpNoCase("gml")==0)
        ds_format = "GML";
    else if(ext.CmpNoCase("gpx")==0)
        ds_format = "GPX";
    else if(ext.CmpNoCase("kml")==0)
        ds_format = "KML";
    else if(ext.CmpNoCase("json") ==0 || ext.CmpNoCase("geojson")==0)
        ds_format = "GeoJSON";
    else if(ext.CmpNoCase("sqlite")==0)
        ds_format = "SQLite";
    else if(ext.CmpNoCase("xls")==0)
        ds_format = "XLS";
    else if(ext.CmpNoCase("xlsx")==0)
        ds_format = "XLSX";
    else if(ext.CmpNoCase("dwg")==0)
        ds_format = "DWG";
    else if(ext.CmpNoCase("dxf")==0)
        ds_format = "DXF";
    else if(ext.CmpNoCase("ntf")==0)
        ds_format = "UK .NTF";
    else if(ext.CmpNoCase("sdts")==0)
        ds_format = "SDTS";
    else if(ext.CmpNoCase("tiger")==0)
        ds_format = "TIGER";
    else if(ext.CmpNoCase("s57")==0)
        ds_format = "S57";
    else if(ext.CmpNoCase("dgn")==0)
        ds_format = "DGN";
    else if(ext.CmpNoCase("vrt")==0)
        ds_format = "VRT";
    else if(ext.CmpNoCase("rec")==0)
        ds_format = "REC";
    else if(ext.CmpNoCase("memory")==0)
        ds_format = "Memory";
    else if(ext.CmpNoCase("bna")==0)
        ds_format = "BNA";
    else if(ext.CmpNoCase("nas")==0)
        ds_format = "NAS";
    /*
     else if(ext.CmpNoCase("itf")==0)
     ds_format = "Interlis 1";
     else if(ext.CmpNoCase("ili")==0)
     ds_format = "Interlis 2";
     */
    else if(ext.CmpNoCase("gmt")==0)
        ds_format = "GMT";
    else if(ext.CmpNoCase("georss")==0)
        ds_format = "GeoRSS";
    else if(ext.CmpNoCase("pgdump")==0)
        ds_format = "PGDump";
    else if(ext.CmpNoCase("svg")==0)
        ds_format = "SVG";
    else if(ext.CmpNoCase("couchdb")==0)
        ds_format = "CouchDB";
    else if(ext.CmpNoCase("vct")==0)
        ds_format = "Idrisi";
    else if(ext.CmpNoCase("ods")==0)
        ds_format = "ODS";

    //else
    //    ds_format = "Unknown";
    return ds_format;
}

GdaConst::DataSourceType IDataSource::FindDataSourceType
(wxString data_type_name)
{
    if (GdaConst::datasrc_str_to_type.find(data_type_name.ToStdString()) ==
        GdaConst::datasrc_str_to_type.end()) {
        return GdaConst::ds_unknown;
    }
    
    return GdaConst::datasrc_str_to_type[data_type_name.ToStdString()];
}

wxString IDataSource::GetDataTypeNameByGdaDSType
(GdaConst::DataSourceType ds_type)
{
    return GdaConst::datasrc_type_to_str[ds_type];
}

IDataSource* IDataSource::CreateDataSource(wxString data_type_name,
                                           const ptree& subtree,
										   const wxString& proj_path)
{
    if (GdaConst::datasrc_str_to_type.find(data_type_name.ToStdString()) ==
        GdaConst::datasrc_str_to_type.end()) {
        stringstream ss;
        ss << "datasource.type " << data_type_name << " unknown.";
        throw GdaException(ss.str().c_str());
    }
    
    GdaConst::DataSourceType type =
        GdaConst::datasrc_str_to_type[data_type_name.ToStdString()];
    
    if (type == GdaConst::ds_esri_file_geodb ||
        type == GdaConst::ds_csv ||
        type == GdaConst::ds_dbf ||
        type == GdaConst::ds_gml ||
        type == GdaConst::ds_kml ||
        type == GdaConst::ds_mapinfo ||
        type == GdaConst::ds_shapefile ||
        type == GdaConst::ds_esri_personal_gdb ||
        type == GdaConst::ds_odbc ||
        type == GdaConst::ds_sqlite ||
        type == GdaConst::ds_xls ||
        type == GdaConst::ds_xlsx ||
        type == GdaConst::ds_geo_json )
    {
        // using <file>xxx</file> to create DataSource instance
        return new FileDataSource(subtree, type, proj_path);
        
    } else if (type == GdaConst::ds_oci ||
               type == GdaConst::ds_mysql ||
               type == GdaConst::ds_postgresql ||
               type == GdaConst::ds_esri_arc_sde )
    {
        // using <db_name>xxx</db_name>... to create DataSource instance
        return new DBDataSource(subtree, type, "");
        
    } else if (type == GdaConst::ds_wfs || type == GdaConst::ds_cartodb) {
        // using <url></url> to create Datasource instance
        return new WebServiceDataSource(subtree, type, "");
    }
	return NULL;
}

//------------------------------------------------------------------------------
// FileDataSource member functions
//------------------------------------------------------------------------------
FileDataSource::FileDataSource(const ptree& xml_tree,
                               GdaConst::DataSourceType _ds_type,
							   const wxString& proj_path)
{
	ds_type = _ds_type;
    ReadPtree(xml_tree, proj_path);
}

FileDataSource::FileDataSource(wxString file_rep_path)
{
    file_repository_path = file_rep_path;
	wxString ds_type_name;

	if ( file_rep_path.StartsWith("PGeo:") ) {
		ds_type_name = "PGeo";
	} else {
		wxString file_ext = wxFileName(file_rep_path).GetExt();
	    ds_type_name = IDataSource::GetDataTypeNameByExt(file_ext);
	}

    ds_type = IDataSource::FindDataSourceType(ds_type_name);
}

IDataSource* FileDataSource::Clone()
{
    return new FileDataSource(file_repository_path);
}

void FileDataSource::ReadPtree(const ptree& pt,
							   const wxString& proj_path)
{
	try {
		//const ptree& subtree = pt.get_child("datasource");
		string type_str = pt.get<string>("type");
        ds_type = IDataSource::FindDataSourceType(type_str);
        
		if (ds_type == GdaConst::ds_unknown) {
			stringstream ss;
			ss << "datasource.type " << type_str << " unknown.";
			throw GdaException(ss.str().c_str());
		}
		
        file_repository_path = pt.get<string>("path");
		file_repository_path = GenUtils::RestorePath(proj_path,
													 file_repository_path);
        
        if (!wxFileExists(file_repository_path)) {
            wxString msg;
            msg << "GeoDa cannot find one or more associated datasource defined in GeoDa project file.\n\n";
            msg << "Details: Datasource not exists: " << file_repository_path;
            msg << "\n\nTip: You can open the .gda project file in a text editor to modify the path(s) of the datasource associated with your project.";
            
            throw GdaException(msg.mb_str());
        }
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

void FileDataSource::WritePtree(ptree& pt,
								const wxString& proj_path)
{
    try{
        pt.put("type", GdaConst::datasrc_type_to_str[ds_type]);
        pt.put("path", GenUtils::SimplifyPath(proj_path, file_repository_path));
    } catch (std::exception &e) {
        throw GdaException(e.what());
    }
}

wxString FileDataSource::GetOGRConnectStr()
{
    if (wxFileExists(file_repository_path) ||
		file_repository_path.StartsWith("PGeo:") ||
        (file_repository_path.EndsWith(".gdb") &&
		 wxDirExists(file_repository_path)))
    {
        return file_repository_path;
    } else {
		return file_repository_path;
	}
    
    wxString error_msg;
    error_msg << "Datasource (" << file_repository_path << ") doesn't "
              << "exist. Please check the project configuration file.";
    throw GdaException(error_msg.mb_str());
}
//------------------------------------------------------------------------------
// WebServiceDataSource member functions
//------------------------------------------------------------------------------
WebServiceDataSource::WebServiceDataSource(const ptree& xml_tree,
										   GdaConst::DataSourceType _ds_type,
										   const wxString& proj_path)
{
	ds_type = _ds_type;
    ReadPtree(xml_tree, proj_path);
}

WebServiceDataSource::WebServiceDataSource(wxString ws_url,
                                           GdaConst::DataSourceType _ds_type)
{
    webservice_url = ws_url;
    ds_type = _ds_type;
}

IDataSource* WebServiceDataSource::Clone()
{
    return new WebServiceDataSource(webservice_url, ds_type);
}

void WebServiceDataSource::ReadPtree(const ptree& pt,
									 const wxString& proj_path)
{
	try {
		string type_str = pt.get<string>("type");
        ds_type = IDataSource::FindDataSourceType(type_str);
        
		if (ds_type == GdaConst::ds_unknown) {
			stringstream ss;
			ss << "datasource.type " << type_str << " unknown.";
			throw GdaException(ss.str().c_str());
		}
		
        webservice_url = pt.get<string>("url");
		
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

void WebServiceDataSource::WritePtree(ptree& pt,
									  const wxString& proj_path)
{
    try{
        pt.put("type", GdaConst::datasrc_type_to_str[ds_type]);
        pt.put("url", webservice_url);
    } catch (std::exception &e) {
        throw GdaException(e.what());
    }
}

//------------------------------------------------------------------------------
// DBDataSource member functions
//------------------------------------------------------------------------------
DBDataSource::DBDataSource(const ptree& xml_tree,
						   GdaConst::DataSourceType _ds_type,
						   const wxString& proj_path)
{
	ds_type = _ds_type;
    ReadPtree(xml_tree, proj_path);
}

DBDataSource::DBDataSource(wxString _db_name, wxString _db_host, 
						   wxString _db_port, wxString _db_user, 
						   wxString _db_pwd, GdaConst::DataSourceType _ds_type)
{
	db_name = _db_name;
	db_host = _db_host;
	db_port = _db_port;
	db_user = _db_user;
	db_pwd  = _db_pwd;
	ds_type = _ds_type;
}

IDataSource* DBDataSource::Clone()
{
    return new DBDataSource(db_name, db_host, db_port, db_user, db_pwd, ds_type);
}

wxString DBDataSource::GetOGRConnectStr()
{
	if (!ogr_conn_str.IsEmpty()) return ogr_conn_str;
	
	if (ds_type == GdaConst::ds_oci) {
		// Oracle examples
		//ds_str = "OCI:oracle/abcd1234@ORA11";
		//ds_str = "OCI:oracle/abcd1234@129.219.93.200:1521/xe";
		//ds_str = "OCI:oracle/oracle@192.168.56.101:1521/xe:table1,table2";
		ogr_conn_str << GdaConst::datasrc_type_to_prefix[ds_type];
		ogr_conn_str << db_user << "/" << db_pwd << "@";
		ogr_conn_str << db_host << ":" << db_port << "/";
		ogr_conn_str << db_name;
		
	} else if (ds_type == GdaConst::ds_postgresql) {
		// postgis: PG:"dbname='databasename' host='addr' port='5432'
		//   user='x' password='y'
		ogr_conn_str << GdaConst::datasrc_type_to_prefix[ds_type];
		ogr_conn_str << "dbname='" << db_name << "' ";
		ogr_conn_str << "host='" << db_host << "' ";
		ogr_conn_str << "port='" << db_port << "' ";
		ogr_conn_str << "user='" << db_user << "' ";
		ogr_conn_str << "password='" << db_pwd << "'";
		
	} else if (ds_type == GdaConst::ds_esri_arc_sde) {
		// ArcSDE: SDE:server,instance,database,username,password[,layer]
		ogr_conn_str << GdaConst::datasrc_type_to_prefix[ds_type];
		ogr_conn_str << db_host << "," << db_port << ",";
		ogr_conn_str << db_name << ","; 
		ogr_conn_str << db_user << "," << db_pwd;
        
	} else if (ds_type == GdaConst::ds_mysql) {
		// MYSQL:dbname,host=server,user=root,password=pwd,port=3306,table=test
		ogr_conn_str << GdaConst::datasrc_type_to_prefix[ds_type];
		ogr_conn_str << db_name<< ",host=" << db_host<< ",port=";
		ogr_conn_str << db_port<< ",user=";
		ogr_conn_str << db_user << ",password=" << db_pwd;
        
	}
    /*
    else if (ds_type == GdaConst::ds_ms_sql) {
		// MSSQL:server=.\MSSQLSERVER2008;database=dbname;trusted_connection=yes
		ogr_conn_str << GdaConst::datasrc_type_to_prefix[ds_type];
		ogr_conn_str << db_name<< ",host=" << db_host<< ",port=";
		ogr_conn_str << db_port<< ",user=";
		ogr_conn_str << db_user << ",password=" << db_pwd;
        
    }
    */
    return ogr_conn_str;
}

void DBDataSource::ReadPtree(const ptree& pt,
							 const wxString& proj_path)
{
    try{
        string type_str = pt.get<string>("type");
        ds_type = IDataSource::FindDataSourceType(type_str);
        
        if (ds_type == GdaConst::ds_unknown) {
            stringstream ss;
            ss << "datasource.type " << type_str << " unknown.";
            throw GdaException(ss.str().c_str());
        }
        
        db_name = pt.get<string>("db_name");
        db_host = pt.get<string>("host");
        db_port = pt.get<string>("port");
        db_user = pt.get<string>("user");
        db_pwd  = pt.get<string>("pwd");
    } catch (std::exception &e) {
        throw GdaException(e.what());
    }
}

void DBDataSource::WritePtree(ptree& pt,
							  const wxString& proj_path)
{
    try {
        pt.put("type", GdaConst::datasrc_type_to_str[ds_type]);
        pt.put("db_name", db_name);
        pt.put("host", db_host);
        pt.put("port", db_port);
        pt.put("user", db_user);
        pt.put("pwd", db_pwd);
    } catch (std::exception &e) {
        throw GdaException(e.what());
    }
}