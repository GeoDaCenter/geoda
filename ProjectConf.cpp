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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "DataViewer/DataSource.h"
#include "ProjectConf.h"
#include "GdaConst.h"
#include "GenUtils.h"
#include "GdaException.h"

using boost::property_tree::ptree;
using namespace std;

//------------------------------------------------------------------------------
// LayerConfiguration member functions
//------------------------------------------------------------------------------
LayerConfiguration::~LayerConfiguration()
{
    if (datasource) delete datasource; datasource= 0;
    if (variable_order) delete variable_order;
	if (custom_classifs) delete custom_classifs;
	if (spatial_weights) delete spatial_weights;
}

LayerConfiguration::LayerConfiguration(wxString _layer_name)
: layer_name(_layer_name), datasource(0), variable_order(0), custom_classifs(0),
spatial_weights(0), default_vars(0)
{
}

LayerConfiguration::LayerConfiguration(const ptree& xml_tree,
                                       const wxString& proj_path)
: datasource(0), variable_order(0), custom_classifs(0),
spatial_weights(0), default_vars(0)
{
	ReadPtree(xml_tree, proj_path);
}

LayerConfiguration::LayerConfiguration(wxString layerName,
									   IDataSource* ds)
: layer_name(layerName), datasource(ds)
{
    variable_order = new VarOrderPtree();
    custom_classifs = new CustomClassifPtree();
	spatial_weights = new WeightsManPtree();
	default_vars = new DefaultVarsPtree();
}

LayerConfiguration::LayerConfiguration(wxString layerName,
									   IDataSource* ds,
                                       VarOrderPtree* vo,
									   CustomClassifPtree* cc,
									   WeightsManPtree* sp_wts,
									   DefaultVarsPtree* dvs)
: layer_name(layerName), datasource(ds), variable_order(vo),
custom_classifs(cc), spatial_weights(sp_wts), default_vars(dvs)
{
}

void LayerConfiguration::UpdateDataSource(IDataSource* new_datasource)
{
    if ( datasource ) {
        delete datasource;
        datasource = NULL;
    }
    datasource = new_datasource;
}

void LayerConfiguration::ReadPtree(const ptree& pt,
								   const wxString& proj_path)
{
    layer_title = pt.get("title", "");
	
	// create DataSource instance from <datasource>...
	const ptree& subtree = pt.get_child("datasource");
	string type_str = subtree.get<string>("type");
	datasource = IDataSource::CreateDataSource(type_str, subtree, proj_path);

	layer_name = pt.get("layername", "");

	// create VarOrderPtree instance from <variable_order>...
	if (!variable_order) variable_order = new VarOrderPtree(pt, proj_path);
	// create CustomClassifPtree instance from <custom_classifications>...
	if (!custom_classifs) custom_classifs = new CustomClassifPtree(pt,
																   proj_path);
	// create WeightsManPtree instance from <spatial_weights>...
	if (!spatial_weights) spatial_weights = new WeightsManPtree(pt, proj_path);
	// create DefaultVarsPtree instance from <default_vars>...
	if (!default_vars) default_vars = new DefaultVarsPtree(pt, proj_path);
}


void LayerConfiguration::WritePtree(ptree& pt,
									const wxString& proj_path)
{
    ptree& subtree = pt.put("datasource", "");
    datasource->WritePtree(subtree, proj_path);
    pt.put("layername", layer_name);
    pt.put("title", layer_title);
    if (variable_order) variable_order->WritePtree(pt, proj_path);
	if (custom_classifs) custom_classifs->WritePtree(pt, proj_path);
	if (spatial_weights) spatial_weights->WritePtree(pt, proj_path);
	if (default_vars) default_vars->WritePtree(pt, proj_path);
}

LayerConfiguration* LayerConfiguration::Clone()
{
    LayerConfiguration* new_layer_conf = new LayerConfiguration(layer_name);
    if (datasource)
        new_layer_conf->SetDataSource(datasource->Clone());
    if (variable_order)
        new_layer_conf->SetVariableOrder(variable_order->Clone());
    if (custom_classifs)
        new_layer_conf->SetCustomClassifs(custom_classifs->Clone());
    if (spatial_weights)
        new_layer_conf->SetSpatialWeights(spatial_weights->Clone());
    if (default_vars)
        new_layer_conf->SetDefaultVars(default_vars->Clone());
    return new_layer_conf;
}

void LayerConfiguration::SetDataSource(IDataSource *new_datasource)
{
    if (datasource) {
        delete datasource;
        datasource = NULL;
    }
    datasource = new_datasource;
}

void LayerConfiguration::SetVariableOrder(VarOrderPtree *new_variable_order)
{
    if (variable_order) {
        delete variable_order;
        variable_order = NULL;
    }
    variable_order = new_variable_order;
}

void
LayerConfiguration::SetCustomClassifs(CustomClassifPtree *new_custom_classifs)
{
    if (custom_classifs) {
        delete custom_classifs;
        custom_classifs = NULL;
    }
    custom_classifs = new_custom_classifs;
}

void LayerConfiguration::SetSpatialWeights(WeightsManPtree *new_spatial_weights)
{
    if (spatial_weights) {
        delete spatial_weights;
        spatial_weights = NULL;
    }
    spatial_weights = new_spatial_weights;
}

void LayerConfiguration::SetDefaultVars(DefaultVarsPtree *new_default_vars)
{
    if (default_vars) {
        delete default_vars;
        default_vars = NULL;
    }
    default_vars = new_default_vars;
}

//------------------------------------------------------------------------------
// ProjectConfiguration member functions
//------------------------------------------------------------------------------
ProjectConfiguration::~ProjectConfiguration()
{
    for (size_t i=0, sz=layer_confs.size(); i<sz; ++i) {
        if (layer_confs[i]) delete layer_confs[i];
    }
}

ProjectConfiguration::ProjectConfiguration(const wxString& proj_path)
{
    project_fpath = proj_path;
	ptree xml_tree;
#ifdef __WIN32__
	std::wstring ws(project_fpath.fn_str());
	std::string s(ws.begin(), ws.end());
	read_xml(s, xml_tree);
#else
	read_xml(std::string(GET_ENCODED_FILENAME(project_fpath)), xml_tree);
#endif
    ReadPtree(xml_tree, proj_path);
}

ProjectConfiguration::ProjectConfiguration(wxString prj_title,
                                           LayerConfiguration* layer_conf)
{
    project_title = prj_title;	
    layer_confs.push_back(layer_conf);
}

void ProjectConfiguration::ReadPtree(const ptree& pt,
									 const wxString& proj_path)
{
	project_title = pt.get("project.title", "");
	
	const ptree& subtree = pt.get_child("project.layers.layer");
	LayerConfiguration* layer_conf = new LayerConfiguration(subtree,
                                                            proj_path);
	layer_confs.push_back(layer_conf);

}

void ProjectConfiguration::Save(wxString saveFileName)
{
    ptree pt;
    WritePtree(pt, saveFileName);
    //boost::property_tree::xml_writer_settings<char> settings(' ', 4);
    boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
	try {
#ifdef __WIN32__
		std::wstring ws(saveFileName.fn_str());
		std::string s(ws.begin(), ws.end());
		write_xml(s, pt, std::locale(), settings);
#else
		write_xml(std::string(GET_ENCODED_FILENAME(saveFileName)), pt,
			      std::locale(), settings);
#endif
	} catch (const boost::property_tree::ptree_error& e) {
		throw GdaException(e.what());
	}
}

void ProjectConfiguration::WritePtree(ptree& pt,
									  const wxString& proj_path)
{
    pt.put("project.title", project_title);
    ptree& subtree = pt.put("project.layers.layer","");
    layer_confs[0]->WritePtree(subtree, proj_path);
}

ProjectConfiguration* ProjectConfiguration::Clone()
{
    LayerConfiguration* layer_conf = GetLayerConfiguration();
    return new ProjectConfiguration(project_title, layer_conf->Clone());
}
