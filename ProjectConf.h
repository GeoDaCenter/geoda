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

#ifndef __GEODA_CENTER_PROJECT_CONF_H__
#define __GEODA_CENTER_PROJECT_CONF_H__

#include <vector>

#include "DataViewer/PtreeInterface.h"
#include "DataViewer/DataSource.h"
#include "DataViewer/VarOrderPtree.h"
#include "DataViewer/CustomClassifPtree.h"
#include "ShapeOperations/WeightsManPtree.h"
#include "DefaultVarsPtree.h"

/**
 * LayerConfiguration is a chilld node of ProjectConfiguration in the geoda
 * XML tree structure. It is corresponding to <layer>...</layer> content.
 *
 * Information like datasource details, layer name, or even map window styles,
 * plot window styles etc. can be stored in this file and be manipulated by this
 * class.
 *
 * For creating a project, you need to create a LayerConfiguration instance:
 *   LayerConfiguration* layer_conf = new LayerConfiguration(datasource, var_order);
 *   ProjectConfiguration* project_conf = new ProjectConfiguration();
 *   project_conf->AddLayerConfigure( layer_conf );
 *
 */
class LayerConfiguration : public PtreeInterface
{
private:
    //wxString proj_fpath; //!< project file path: for construct absolute ds path
	wxString layer_name; //!< <layer_name>..</layer_name>
	wxString layer_title; //!< <title>...</title>
	IDataSource* datasource; //!< <datasource>...</datasource>
	VarOrderPtree* variable_order; //!<variableorder>...</variableorder>
	CustomClassifPtree* custom_classifs; //!<custom_classifications>...
                                         //!</custom_classifications>
	WeightsManPtree* spatial_weights; //!<spatial_weights>...</spatial_weights>
	DefaultVarsPtree* default_vars; //!<default_vars>...</default_vars>
	//MapStyleConf* map_style_conf; //!< <mapstyle>...</mapstyle>
    
public:
    LayerConfiguration(wxString _layer_name);
	LayerConfiguration(const boost::property_tree::ptree& xml_tree,
                       const wxString& proj_path);
	LayerConfiguration(wxString layerName, IDataSource* ds);
	LayerConfiguration(wxString layerName, IDataSource* ds,
					   VarOrderPtree* vo, CustomClassifPtree* cc,
					   WeightsManPtree* sp_wts,
					   DefaultVarsPtree* dvs);
	virtual ~LayerConfiguration();

    LayerConfiguration* Clone();
	IDataSource* GetDataSource() { return datasource; }
    void UpdateDataSource(IDataSource* new_datasource);
	VarOrderPtree* GetVarOrderPtree() { return variable_order; }
	CustomClassifPtree* GetCustClassifPtree() { return custom_classifs; }
	WeightsManPtree* GetWeightsManPtree() { return spatial_weights; }
	DefaultVarsPtree* GetDefaultVarsPtree() { return default_vars; }

	wxString GetTitle() { return layer_title; }
	wxString GetName() { return layer_name; }
    void SetName(const wxString& new_name) { layer_name = new_name; }
    
    virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
    
    void SetVariableOrder(VarOrderPtree* new_variable_order);

private:
    void SetDataSource(IDataSource* new_datasource);
    void SetCustomClassifs(CustomClassifPtree* new_custom_classifs);
    void SetSpatialWeights(WeightsManPtree* new_spatial_weights);
    void SetDefaultVars(DefaultVarsPtree* new_default_vars);
};

/**
 * ProjectConfiguration is a class that represents the content that defines in a
 * GeoDa project XML file. 
 * 
 * Reading a project configuration from a XML project file:
 *   ProjectConfiguration* project_conf = new ProjectConfiguration(prj_file_name);
 *   LayerConfiguration* layer_conf = project_conf->GetLayerConfiguration();
 *   IDataSource* datasource = layer_conf->GetDataSource();
 *   VarOrderPtree* variable_order = layer_conf->GetVarOrderPtree();
 *
 * For creating a project. User can create a LayerConfiguration instance first,
 * and then create a ProjectConfiguration using this layer_configuration, then
 * call Save() function.
 * Example:
 *   ProjectConfiguration* project_conf = new ProjectConfiguration();
 *   project_conf->SetTitle("project title");
 *   LayerConfiguration* layer_conf = new LayerConfiguration(datasource, var_order);
 *   project_conf->AddLayerConfigure( layer_conf );
 *   // multi-layer support
 *   //project_conf->AddLayerConfigure( layer_conf );
 *   // save project file
 *   project_conf->Save(new_project_file_name);
 */
class ProjectConfiguration : public PtreeInterface
{
public:
	ProjectConfiguration(){}
    /**
     * Init ProjectConfiguration instance by reading a project file
     */
	ProjectConfiguration(const wxString& proj_path);
    /**
     * Init ProjectConfiguration instance by adding a new datasource
     */
    ProjectConfiguration(wxString prj_title, LayerConfiguration* layer_conf);
	~ProjectConfiguration();
	
private:
    wxString project_fpath; // for build absolute file DS path
	wxString project_title; //< <title>...</title>
	std::vector<LayerConfiguration*> layer_confs; //!< <layers>...</layers>
	
public:
	LayerConfiguration* GetLayerConfiguration() { return layer_confs[0]; }
	void SetTitle(wxString title) { project_title = title; }
    wxString GetFilePath() { return project_fpath;}
    void SetFilePath(const wxString& file_path){ project_fpath = file_path;}
	void AddLayerConfigure(LayerConfiguration* layer_conf) {}
	void Save(wxString saveFileName);
    ProjectConfiguration* Clone();
    
    virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path);
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path);
    
private:
	size_t GetNumLayers(){return layer_confs.size();}
	 
};

#endif
