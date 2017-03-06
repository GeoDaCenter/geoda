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
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include "../ShapeOperations/GeodaWeight.h"
#include "../ShapeOperations/WeightsManager.h"
#include "../GdaException.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "WeightsManPtree.h"

wxString WeightsPtreeEntry::ToStr() const
{
	wxString s;
	s << "title: " << title << "\n";
	s << "is_default: " << (is_default ? "true" : "false") << "\n";
	s << wmi.ToStr();
	return s;
}

WeightsManPtree::WeightsManPtree()
{
}

WeightsManPtree::WeightsManPtree(const WeightsManPtree& o)
{
    std::list<WeightsPtreeEntry>::const_iterator it;
    for (it = o.weights_list.begin(); it != o.weights_list.end(); ++it) {
        weights_list.push_back(*it);
    }
}

WeightsManPtree::WeightsManPtree(const boost::property_tree::ptree& pt,
								 const wxString& proj_path)
{
    ReadPtree(pt, proj_path);
}

WeightsManPtree::~WeightsManPtree()
{
}

WeightsManPtree* WeightsManPtree::Clone()
{
    return new WeightsManPtree(*this);
}

void WeightsManPtree::ReadPtree(const boost::property_tree::ptree& pt,
								const wxString& proj_path)
{
	using boost::property_tree::ptree;
	using namespace std;
	weights_list.clear();
	try {
		try {
			pt.get_child("weights_entries");
		}
		catch (boost::property_tree::ptree_bad_path& e) {
			// weights_entries is optional
			return;
		}
		
		// iterate over each child of weights_entries
		BOOST_FOREACH(const ptree::value_type &v,
					  pt.get_child("weights_entries")) {
			wxString key = v.first.data();
			if (key == "weights") {
				WeightsPtreeEntry e;
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString key = v.first.data();
					if (key == "title") {
						wxString s = v.second.data();
						e.title = s;
					} else if (key == "default") {
						e.is_default = true;
					} else if (key == "meta_info") {
						BOOST_FOREACH(const ptree::value_type &v, v.second) {
							wxString key = v.first.data();
							if (key == "weights_type") {
								wxString s = v.second.data();
								if (s == "rook") {
									e.wmi.weights_type = WeightsMetaInfo::WT_rook;
								} else if (s == "queen") {
									e.wmi.weights_type = WeightsMetaInfo::WT_queen;
								} else if (s == "threshold") {
									e.wmi.weights_type = WeightsMetaInfo::WT_threshold;
								} else if (s == "knn") {
									e.wmi.weights_type = WeightsMetaInfo::WT_knn;
								} else { // s == "custom"
									e.wmi.weights_type = WeightsMetaInfo::WT_custom;
								}
							} else if (key == "path") {
								wxString s = v.second.data();
								e.wmi.filename = GenUtils::RestorePath(proj_path, s);
                                if (!wxFileExists(e.wmi.filename)) {
                                    wxString msg;
                                    
                                    msg << "The GeoDa project file cannot find one or more associated data sources.\n\n";
                                    msg << "Details: Weights file (" << e.wmi.filename << ") is missing";
                                    msg << "\n\nTip: You can open the .gda project file in a text editor to modify the path(s) of the weights file(s) (.gwt or .gal extension) associated with your project.";
                                    
                                    throw GdaException(msg.mb_str());
                                }
                                
							} else if (key == "id_variable") {
								wxString s = v.second.data();
								e.wmi.id_var = s;
							} else if (key == "symmetry") {
								wxString s = v.second.data();
								if (s == "symmetric") {
									e.wmi.sym_type = WeightsMetaInfo::SYM_symmetric;
								} else if (s == "asymmetric") {
									e.wmi.sym_type = WeightsMetaInfo::SYM_asymmetric;
								} else if (s == "unknown" || s.IsEmpty()) {
									e.wmi.sym_type = WeightsMetaInfo::SYM_unknown;
								} else {
									wxString msg("unrecognized value: ");
									msg << s << " for key: " << key;
									throw GdaException(msg.mb_str());
								}
							} else if (key == "order") {
								long l;
								wxString(v.second.data()).ToLong(&l);
								e.wmi.order = l;
							} else if (key == "inc_lower_orders") {
								wxString s = v.second.data();
								if (s.CmpNoCase("false") == 0) {
									e.wmi.inc_lower_orders = false;
								} else if (s.CmpNoCase("true") == 0) {
									e.wmi.inc_lower_orders = true;
								} else {
									wxString msg("unrecognized value: ");
									msg << s << " for key: " << key;
									throw GdaException(msg.mb_str());
								}
							}  else if (key == "dist_metric") {
								wxString s = v.second.data();
								if (s == "euclidean") {
									e.wmi.dist_metric = WeightsMetaInfo::DM_euclidean;
								} else if (s == "arc") {
									e.wmi.dist_metric = WeightsMetaInfo::DM_arc;
								} else if (s == "unspecified" || s.IsEmpty()) {
									e.wmi.dist_metric = WeightsMetaInfo::DM_unspecified;
								} else {
									wxString msg("unrecognized value: ");
									msg << s << " for key: " << key;
									throw GdaException(msg.mb_str());
								}
							} else if (key == "dist_units") {
								wxString s = v.second.data();
								if (s == "km") {
									e.wmi.dist_units = WeightsMetaInfo::DU_km;
								} else if (s == "mile") {
									e.wmi.dist_units = WeightsMetaInfo::DU_mile;
                                } else {
									e.wmi.dist_units = WeightsMetaInfo::DU_unspecified;
                                    e.wmi.dist_units_str = s;
								}
							} else if (key == "dist_values") {
								wxString s = v.second.data();
								if (s == "centroids") {
									e.wmi.dist_values = WeightsMetaInfo::DV_centroids;
								} else if (s == "mean_centers") {
									e.wmi.dist_values = WeightsMetaInfo::DV_mean_centers;
								} else if (s == "vars") {
									e.wmi.dist_values = WeightsMetaInfo::DV_vars;
								} else if (s == "unspecified" || s.IsEmpty()) {
									e.wmi.dist_values = WeightsMetaInfo::DV_unspecified;
								} else {
									wxString msg("unrecognized value: ");
									msg << s << " for key: " << key;
									throw GdaException(msg.mb_str());
								}
							} else if (key == "dist_var1") {
								wxString s = v.second.data();
								e.wmi.dist_var1 = s;
								e.wmi.dist_values = WeightsMetaInfo::DV_vars;
							} else if (key == "dist_var2") {
								wxString s = v.second.data();
								e.wmi.dist_var2 = s;
								e.wmi.dist_values = WeightsMetaInfo::DV_vars;
							} else if (key == "dist_tm1") {
								long l;
								wxString(v.second.data()).ToLong(&l);
								e.wmi.dist_tm1 = l;
							} else if (key == "dist_tm2") {
								long l;
								wxString(v.second.data()).ToLong(&l);
								e.wmi.dist_tm2 = l;
							} else if (key == "num_neighbors") {
								long l;
								wxString(v.second.data()).ToLong(&l);
								e.wmi.num_neighbors = l;
							} else if (key == "threshold_val") {
								double d;
								wxString(v.second.data()).ToDouble(&d);
								e.wmi.threshold_val = d;
							}
						}
					} else {
						// ignore unrecognized key
						wxString msg("unrecognized key: ");
						msg << key;
					}
				}
                
				weights_list.push_back(e);
			} else {
				wxString msg("unrecognized key: ");
				msg << key;
				throw GdaException(msg.mb_str());
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

void WeightsManPtree::WritePtree(boost::property_tree::ptree& pt,
								 const wxString& proj_path)
{
	using boost::property_tree::ptree;
	using namespace std;
	try {
		ptree& sub = pt.put("weights_entries", "");

		// Write each spatial weights meta info definition
		BOOST_FOREACH(const WeightsPtreeEntry& e, weights_list) {
			ptree& ssub = sub.add("weights", "");
			if (!e.title.IsEmpty()) ssub.put("title", e.title);
			if (e.is_default) ssub.put("default", "");
			ptree& sssub = ssub.add("meta_info", "");
			if (e.wmi.weights_type == WeightsMetaInfo::WT_custom)
			{
				sssub.put("weights_type", "custom");
			} else if (e.wmi.weights_type == WeightsMetaInfo::WT_rook ||
					   e.wmi.weights_type == WeightsMetaInfo::WT_queen)
			{
				sssub.put("weights_type", (e.wmi.weights_type ==
										  WeightsMetaInfo::WT_rook ? "rook"
										  : "queen"));
				sssub.put("order", e.wmi.order);
				if (e.wmi.inc_lower_orders == true) {
					sssub.put("inc_lower_orders", "true");
				} else {
					sssub.put("inc_lower_orders", "false");
				}
			} else if (e.wmi.weights_type == WeightsMetaInfo::WT_threshold ||
					   e.wmi.weights_type == WeightsMetaInfo::WT_knn)
			{
				sssub.put("weights_type", (e.wmi.weights_type ==
										  WeightsMetaInfo::WT_knn ? "knn"
										  : "threshold"));
				if (e.wmi.dist_metric == WeightsMetaInfo::DM_euclidean) {
					sssub.put("dist_metric", "euclidean");
				} else if (e.wmi.dist_metric == WeightsMetaInfo::DM_arc) {
					sssub.put("dist_metric", "arc");
				}
				if (e.wmi.dist_units == WeightsMetaInfo::DU_km) {
					sssub.put("dist_units", "km");
				} else if (e.wmi.dist_units == WeightsMetaInfo::DU_mile) {
					sssub.put("dist_units", "mile");
                } else if (e.wmi.dist_units == WeightsMetaInfo::DU_unspecified) {
                    sssub.put("dist_units", e.wmi.dist_units_str);
                }
				if (e.wmi.dist_values == WeightsMetaInfo::DV_centroids) {
					sssub.put("dist_values", "centroids");
				} else if (e.wmi.dist_values ==
						   WeightsMetaInfo::DV_mean_centers) {
					sssub.put("dist_values", "mean_centers");
				} else if (e.wmi.dist_values == WeightsMetaInfo::DV_vars) {
					sssub.put("dist_values", "vars");
					if (!e.wmi.dist_var1.IsEmpty()) {
						sssub.put("dist_var1", e.wmi.dist_var1);
						if (e.wmi.dist_tm1 >= 0) {
							sssub.put("dist_tm1", e.wmi.dist_tm1);
						}
					}
					if (!e.wmi.dist_var2.IsEmpty()) {
						sssub.put("dist_var2", e.wmi.dist_var2);
						if (e.wmi.dist_tm2 >= 0) {
							sssub.put("dist_tm2", e.wmi.dist_tm2);
						}
					}
				}
				
				if (e.wmi.weights_type == WeightsMetaInfo::WT_knn) {
					sssub.put("num_neighbors", e.wmi.num_neighbors);
				} else {
					sssub.put("threshold_val", e.wmi.threshold_val);
				}
			}
			if (!e.wmi.filename.IsEmpty()) {
				sssub.put("path",
						  GenUtils::SimplifyPath(proj_path, e.wmi.filename));
			}
			if (!e.wmi.id_var.IsEmpty()) {
				sssub.put("id_variable", e.wmi.id_var);
			}
			if (e.wmi.sym_type == WeightsMetaInfo::SYM_symmetric) {
				sssub.put("symmetry", "symmetric");
			} else if (e.wmi.sym_type == WeightsMetaInfo::SYM_asymmetric) {
				sssub.put("symmetry", "asymmetric");
			}
		}	
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

const std::list<WeightsPtreeEntry>&
	WeightsManPtree::GetWeightsMetaInfoList() const
{
	return weights_list;
}

void WeightsManPtree::SetWeightsMetaInfoList(
								const std::list<WeightsPtreeEntry>& w_list)
{
	weights_list.clear();
	weights_list = w_list;
}
 
wxString WeightsManPtree::ToStr() const
{
	wxString s;
	BOOST_FOREACH(const WeightsPtreeEntry& e, weights_list) {
		s << e.ToStr() << "\n";
	}
	return s;
}
