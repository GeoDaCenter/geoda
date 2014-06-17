/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

wxString WeightsMetaInfo::ToStr() const
{
	wxString s;
	s << "Weights Meta Info:\n";
	s << "  filename: " << filename << "\n";
	s << "  title: " << title << "\n";
	if (is_default) s << "  is_default: true\n";
	return s;
}

WeightsManPtree::WeightsManPtree()
{
}

WeightsManPtree::WeightsManPtree(const WeightsManPtree& o)
{
    std::list<WeightsMetaInfo>::const_iterator it;
    for (it = o.weights_list.begin(); it != o.weights_list.end(); ++it) {
        weights_list.push_back( *it );
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
	LOG_MSG("Entering WeightsManPtree::ReadPtree");
	using boost::property_tree::ptree;
	using namespace std;
	weights_list.clear();
	try {
		try {
			pt.get_child("spatial_weights");
		}
		catch (boost::property_tree::ptree_bad_path& e) {
			// spatial_weights is optional
			return;
		}
		
		// iterate over each child of spatial_weights
		BOOST_FOREACH(const ptree::value_type &v,
					  pt.get_child("spatial_weights")) {
			wxString key = v.first.data();
			LOG_MSG(key);
			if (key == "weights") {
				WeightsMetaInfo w;
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString key = v.first.data();
					LOG_MSG(key);
					if (key == "title") {
						wxString s = v.second.data();
						w.title = s;
					} else if (key == "path") {
						wxString s = v.second.data();
						w.filename = GenUtils::RestorePath(proj_path, s);
					} else if (key == "default") {
						w.is_default = true;
					} else {
						wxString msg("unrecognized key: ");
						msg << key;
						throw GdaException(msg.mb_str());
					}
				}
				weights_list.push_back(w);
			} else {
				wxString msg("unrecognized key: ");
				msg << key;
				throw GdaException(msg.mb_str());
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
	
	LOG_MSG("Exiting WeightsManPtree::ReadPtree");
}

void WeightsManPtree::WritePtree(boost::property_tree::ptree& pt,
								 const wxString& proj_path)
{
	using boost::property_tree::ptree;
	using namespace std;
	try {
		ptree& sub = pt.put("spatial_weights", "");

		// Write each spatial weights meta info definition
		BOOST_FOREACH(const WeightsMetaInfo& w, weights_list) {
			ptree& ssub = sub.add("weights", "");
			if (!w.title.IsEmpty()) ssub.put("title", w.title);
			ssub.put("path", GenUtils::SimplifyPath(proj_path, w.filename));
			if (w.is_default) ssub.put("default", "");
		}	
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

const std::list<WeightsMetaInfo>&
	WeightsManPtree::GetWeightsMetaInfoList() const
{
	return weights_list;
}

void WeightsManPtree::SetWeightsMetaInfoList(WeightsManager* w_manager)
{
	weights_list.clear();
	if (!w_manager) return;
	int num_weights = w_manager->GetNumWeights();
	for (int i=0; i<num_weights; ++i) {
		GeoDaWeight* gda_wt = w_manager->GetWeight(i);
		WeightsMetaInfo w;
		w.filename = gda_wt->wflnm;
		w.title = gda_wt->title;
		w.is_default = (gda_wt == w_manager->GetCurrWeight() &&
						w_manager->IsDefaultWeight());
		// For now, only save the default weight since there is no way
		// to clear the weights from within GeoDa.
		if (w.is_default) weights_list.push_back(w);
	}
}

wxString WeightsManPtree::ToStr() const
{
	using namespace std;
	wxString s;
	BOOST_FOREACH(const WeightsMetaInfo& w, weights_list) {
		s << w.ToStr();
	}
	return s;
}
