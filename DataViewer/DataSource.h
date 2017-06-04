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

#ifndef __GEODA_CENTER_DATA_SOURCE_H__
#define __GEODA_CENTER_DATA_SOURCE_H__

#include <map>
#include <string>
#include "PtreeInterface.h"
#include "../GdaConst.h"
#include "../GdaException.h"

using namespace std;
using boost::property_tree::ptree;

/**
 * IDataSource is a interface that will be implemented by different kinds of 
 * datasource, such as FileDataSource (.shp, .json,...), 
 * DBDataSource (oracle, mysql..) and WebServiceDataSource(wfs). 
 * 
 * Each inheritant class must take care of following interfaces(functions):
 *   GetOGRConnectStr()  returns a ogr usable connect string
 *   GetType() returns the GdaConst::DataSourceType
 *   ReadPtree() construct itself from a XML tree node
 *   WritePtree() write itself to a XML tree node
 *
 * It also provides some utility functions that will be reused often:
 *  GetDataTypeNameByExt() e.g. given ".shp" return "ESRI Shapefile"
 *  FindDataSourceType()e.g. given "ESRI Shapefile" return GdaConst:ds_shapefile
 *   
 * It also provides a factory function, which  can be called to create a
 * instance from three types DataSource base on given datasource type and 
 * XML tree. e.g. 
 *   reading .gda project file, you can get data type (type_str) and a tree node
 *   of data source (XML description). You can call this function to return a
 *   nstance of one of three datasources. See LayerConfiguration::ReadPtree().
 *   CreateSource("ESRI Shapefile", ptree);
 */
class IDataSource : public PtreeInterface {
public:
    virtual ~IDataSource(){};
	
	virtual wxString GetOGRConnectStr() = 0;	
	
    virtual GdaConst::DataSourceType GetType()=0;
    
    virtual bool IsWritable() = 0;
    
    virtual void UpdateWritable(bool writable) = 0;

    virtual IDataSource* Clone() = 0;
    
    virtual bool IsFileDataSource() = 0;
    
    virtual wxString GetJsonStr() = 0;
    
	virtual wxString ToString() = 0;
    
    /**
     * Read subtree starting from passed in node pt. 
     * @param const ptree& pt: a subtree of "datasource" node
     */
	virtual void ReadPtree(const ptree& pt,
						   const wxString& proj_path) = 0;
	
    /** 
     * Write subtree starting from passed in node pt 
     * @param ptree& pt: a parent node that contains the  "datasource" subtree
     */
	virtual void WritePtree(ptree& pt,
							const wxString& proj_path) = 0;
    
    /// utilities
    static bool IsWritable(GdaConst::DataSourceType ds_type);
    static bool IsTableOnly(GdaConst::DataSourceType ds_type);
    static wxString GetDataTypeNameByExt(wxString ext);
    static wxString GetDataTypeNameByGdaDSType(GdaConst::DataSourceType ds_type);
    static GdaConst::DataSourceType FindDataSourceType(wxString data_type_name);
    
    /// factory, creator of DataSource instance
    static IDataSource* CreateDataSource(wxString data_type_name,
                                         const ptree& subtree,
										 const wxString& proj_path = "");
    
    static IDataSource* CreateDataSource(wxString ds_json_str);
};


/**
 * A implementation of IDataSource, provide a instance for File type datasource
 *
 */
class FileDataSource : public IDataSource {
public:
    virtual ~FileDataSource(){};
    /**
     * Default constructor
     */
	FileDataSource(){}
    /**
     * Constructor, which init a FileDataSource instance from a XML .gda project
     * file.Note: datasource type(_ds_type), which can be read from project file
     * also, should be specified explicitly.
     * @param xml_tree , a tree node of <datasource>...</datasource>
     * @param project_fpath, the file path of project file, in case to rebuild 
     * the absolute path of file datasource name
     */
	FileDataSource(const ptree& xml_tree,
                   GdaConst::DataSourceType _ds_type,
				   const wxString& proj_path);
    /**
     * Constructor, which is used when create a FileDataSource instance from a 
     * data source (e.g. opening a shapefile)
     */
    FileDataSource(wxString ds_path);
	
private:
	wxString file_repository_path;
	GdaConst::DataSourceType ds_type;
	bool is_writable;
    
public:
    /// implementation of IDataSource interfaces
	virtual wxString GetOGRConnectStr();
    
    virtual bool IsWritable(){return is_writable;}
    
    virtual void UpdateWritable(bool writable){ is_writable = writable;}
    
    virtual GdaConst::DataSourceType GetType(){return ds_type;}
    
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
    
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
    
    virtual IDataSource* Clone();
    
    virtual bool IsFileDataSource() {
        return ds_type == GdaConst::ds_sqlite || ds_type == GdaConst::ds_gpkg ? false : true;
    }
    
    virtual wxString GetJsonStr();
    
    /**
     * Return file path.
     */
    wxString GetFilePath() { return file_repository_path;}
    
    
	virtual wxString ToString();
};

/**
 * A implementation of IDataSource, provide a instance for Web service type 
 * of datasource
 *
 */
class WebServiceDataSource: public IDataSource{
public:
    virtual ~WebServiceDataSource(){};
	WebServiceDataSource(){}
	WebServiceDataSource(const ptree& xml_tree,
                         GdaConst::DataSourceType _ds_type,
						 const wxString& proj_path);
	WebServiceDataSource(wxString ws_url){ webservice_url = ws_url; }
    WebServiceDataSource(GdaConst::DataSourceType _ds_type, wxString ws_url);
   
	
private:
	wxString webservice_url;
	GdaConst::DataSourceType ds_type;
	bool is_writable;
    
public:
	virtual wxString GetOGRConnectStr(){ return webservice_url;}
    virtual bool IsWritable(){return is_writable;}
    virtual void UpdateWritable(bool writable){ is_writable = writable;}
    virtual GdaConst::DataSourceType GetType(){return ds_type;}
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
    virtual IDataSource* Clone();
    
    virtual bool IsFileDataSource() {return false;}
    
    virtual wxString GetJsonStr();
    wxString GetURL() { return webservice_url; }
    
	virtual wxString ToString();
};


/**
 * A implementation of IDataSource, provide a instance for database type
 * of datasource
 *
 */
class DBDataSource: public IDataSource{
public:
    virtual ~DBDataSource(){};
	DBDataSource(){}
	DBDataSource(const ptree& xml_tree, GdaConst::DataSourceType _ds_type,
				 const wxString& proj_path);
	DBDataSource(GdaConst::DataSourceType _db_type,
                 wxString _db_name, wxString _db_host, wxString _db_port,
				 wxString _db_user, wxString _db_pwd);
private:
	wxString db_name;
	wxString db_host;
	wxString db_port;
	wxString db_user;
	wxString db_pwd;
	GdaConst::DataSourceType ds_type;
	wxString ogr_conn_str;
    bool is_writable;
	
public:
	virtual wxString GetOGRConnectStr();
    virtual bool IsWritable(){return is_writable;}
    virtual void UpdateWritable(bool writable){ is_writable = writable;}
    virtual GdaConst::DataSourceType GetType(){return ds_type;}
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
    virtual IDataSource* Clone();
    
    virtual bool IsFileDataSource() {return false;}
    
    virtual wxString GetJsonStr();
    
	virtual wxString ToString();
    
    wxString GetDBName() { return db_name; }
    wxString GetDBHost() { return db_host; }
    wxString GetDBPort() { return db_port; }
    wxString GetDBUser() { return db_user; }
    wxString GetDBPwd()  { return db_pwd; }
};


#endif
