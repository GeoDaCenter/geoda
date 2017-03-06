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
#include "../Explore/CatClassifManager.h"
#include "../GdaException.h"
#include "../logger.h"
#include "CustomClassifPtree.h"
#include "CustomClassifPtree.h"

CustomClassifPtree::CustomClassifPtree()
{
    LOG_MSG("Entering CustomClassifPtree::CustomClassifPtree()");
    LOG_MSG("Exiting CustomClassifPtree::CustomClassifPtree()");
}

CustomClassifPtree::CustomClassifPtree(const CustomClassifPtree& o)
{
    LOG_MSG("Entering CustomClassifPtree::CustomClassifPtree(const CustomClassifPtree& o)");
    
    std::list<CatClassifDef>::const_iterator it;
    for (it = o.cc.begin(); it != o.cc.end(); ++it) {
        cc.push_back( *it );
    }
    
    LOG_MSG("Exiting CustomClassifPtree::CustomClassifPtree(const CustomClassifPtree& o)");
}

CustomClassifPtree::CustomClassifPtree(const boost::property_tree::ptree& pt,
									   const wxString& proj_path)
{
    ReadPtree(pt, proj_path);
}

CustomClassifPtree::~CustomClassifPtree()
{
}

CustomClassifPtree* CustomClassifPtree::Clone()
{
    return new CustomClassifPtree(*this);
}

void CustomClassifPtree::ReadPtree(const boost::property_tree::ptree& pt,
								   const wxString& proj_path)
{
	LOG_MSG("Entering CustomClassifPtree::ReadPtree");
	using boost::property_tree::ptree;
	using namespace std;
	
	cc.clear();
	try {
		try {
			pt.get_child("custom_classifications");
		}
		catch (boost::property_tree::ptree_bad_path& e) {
			// custom_classifications is optional
			return;
		}
		
		// iterate over each child of custom_classifications
		BOOST_FOREACH(const ptree::value_type &v,
					  pt.get_child("custom_classifications")) {
			wxString key = v.first.data();
			if (key == "classification_definition") {
				bool first_name = false;
				bool first_color = false;
				CatClassifDef cc_data;
				BOOST_FOREACH(const ptree::value_type &v, v.second) {
					wxString key = v.first.data();
					if (key == "title") {
						wxString s = v.second.data();
						cc_data.title = s;
					} else if (key == "type") {
						wxString s = v.second.data();
						CatClassification::BreakValsType t;
						if (s == "no_theme") {
							t = CatClassification::no_theme_break_vals;
						} else if (s == "hinge_15") {
							t = CatClassification::hinge_15_break_vals;
						} else if (s == "hinge_30") {
							t = CatClassification::hinge_30_break_vals;
						} else if (s == "quantile") {
							t = CatClassification::quantile_break_vals;
						} else if (s == "percentile") {
							t = CatClassification::percentile_break_vals;
						} else if (s == "stddev") {
							t = CatClassification::stddev_break_vals;
						} else if (s == "unique_values") {
							t = CatClassification::unique_values_break_vals;
						} else if (s == "natural_breaks") {
							t = CatClassification::natural_breaks_break_vals;
						} else if (s == "equal_intervals") {
							t = CatClassification::equal_intervals_break_vals;
						} else if (s == "custom") {
							t = CatClassification::custom_break_vals;
						} else {
							wxString msg("unrecognized type: ");
							msg << s;
							throw GdaException(msg.mb_str());
						}
						cc_data.cat_classif_type = CatClassification::custom;
						cc_data.break_vals_type = t;
					} else if (key == "num_cats") {
						wxString s = v.second.data();
						long n;
						if (!s.ToLong(&n)) {
							wxString msg("expected a valid integer: ");
							msg << s;
							throw GdaException(msg.mb_str());
						}
						cc_data.num_cats = n;	
					} else if (key == "automatic_labels") {
						cc_data.automatic_labels =
							GenUtils::StrToBool(v.second.data());
					} else if (key == "assoc_db_fld_name") {
						cc_data.assoc_db_fld_name = v.second.data();
					} else if (key == "uniform_dist_min") {
						wxString s = v.second.data();
						double n;
						if (!s.ToDouble(&n)) {
							wxString msg("expected a valid double: ");
							msg << s;
							throw GdaException(msg.mb_str());
						}
						cc_data.uniform_dist_min = n;
					} else if (key == "uniform_dist_max") {
						wxString s = v.second.data();
						double n;
						if (!s.ToDouble(&n)) {
							wxString msg("expected a valid double: ");
							msg << s;
							throw GdaException(msg.mb_str());
						}
						cc_data.uniform_dist_max = n;
					} else if (key == "breaks") {
						BOOST_FOREACH(const ptree::value_type &v, v.second) {
							wxString key = v.first.data();
							if (key == "break") {
								wxString s = v.second.data();
								double n;
								if (!s.ToDouble(&n)) {
									wxString msg("expected a valid double: ");
									msg << s;
									throw GdaException(msg.mb_str());
								}
								cc_data.breaks.push_back(n);
							} else {
								wxString msg("unrecognized key: ");
								msg << key;
								throw GdaException(msg.mb_str());
							}
						}
					} else if (key == "names") {
						BOOST_FOREACH(const ptree::value_type &v, v.second) {
							wxString key = v.first.data();
							if (key == "name") {
								if (!first_name) {
									cc_data.names.clear();
									first_name = true;
								}
								cc_data.names.push_back(v.second.data());
							} else {
								wxString msg("unrecognized key: ");
								msg << key;
								throw GdaException(msg.mb_str());
							}
						}
					} else if (key == "colors") {
						BOOST_FOREACH(const ptree::value_type &v, v.second) {
							wxString key = v.first.data();
							if (key == "color") {
								if (!first_color) {
									cc_data.colors.clear();
									first_color = true;
								}
								long red=0;
								long blue=0;
								long green=0;
								BOOST_FOREACH(const ptree::value_type &v,
											  v.second)
								{
									wxString key = v.first.data();
									if (key == "red") {
										wxString s = v.second.data();
										if (!s.ToLong(&red)) {
											wxString msg("expected integer: ");
											msg << s;
											throw GdaException(msg.mb_str());
										}
									} else if (key == "green") {
										wxString s = v.second.data();
										if (!s.ToLong(&green)) {
											wxString msg("expected integer: ");
											msg << s;
											throw GdaException(msg.mb_str());
										}
									} else if (key == "blue") {
										wxString s = v.second.data();
										if (!s.ToLong(&blue)) {
											wxString msg("expected integer: ");
											msg << s;
											throw GdaException(msg.mb_str());
										}
									} else {
										wxString msg("unrecognized key: ");
										msg << key;
										throw GdaException(msg.mb_str());
									}
								}
								wxColour c((unsigned char) red,
										   (unsigned char) green,
										   (unsigned char) blue);
								cc_data.colors.push_back(c);
							} else {
								wxString msg("unrecognized key: ");
								msg << key;
								throw GdaException(msg.mb_str());
							}
						}
					} else if (key == "color_scheme") {
						wxString s = v.second.data();
						if (s == "sequential") {
							cc_data.color_scheme =
							CatClassification::sequential_color_scheme;
						} else if (s == "diverging") {
							cc_data.color_scheme =
							CatClassification::diverging_color_scheme;
						} else if (s == "qualitative") {
							cc_data.color_scheme =
							CatClassification::qualitative_color_scheme;
						} else { // s == "custom"
							cc_data.color_scheme =
							CatClassification::custom_color_scheme;
						}
					} else {
						wxString msg("unrecognized key: ");
						msg << key;
						throw GdaException(msg.mb_str());
					}
				}
				cc.push_back(cc_data);
			} else {
				wxString msg("unrecognized key: ");
				msg << key;
				throw GdaException(msg.mb_str());
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
	
	LOG_MSG("Exiting CustomClassifPtree::ReadPtree");
}

void CustomClassifPtree::WritePtree(boost::property_tree::ptree& pt,
									const wxString& proj_path)
{
	using boost::property_tree::ptree;
	using namespace std;
	try {
		ptree& subtree = pt.put("custom_classifications", "");

		// Write each custom classification definition
		BOOST_FOREACH(const CatClassifDef& c, cc) {
			ptree& sub = subtree.add("classification_definition", "");
			sub.put("title", c.title);
			CatClassification::BreakValsType type = c.break_vals_type;
			if (type == CatClassification::no_theme_break_vals) {
				sub.put("type", "no_theme");
			} else if (type == CatClassification::hinge_15_break_vals) {
				sub.put("type", "hinge_15");
			} else if (type == CatClassification::hinge_30_break_vals) {
				sub.put("type", "hinge_30");
			} else if (type == CatClassification::quantile_break_vals) {
				sub.put("type", "quantile");
			} else if (type == CatClassification::percentile_break_vals) {
				sub.put("type", "percentile");
			} else if (type == CatClassification::stddev_break_vals) {
				sub.put("type", "stddev");
			} else if (type == CatClassification::unique_values_break_vals) {
				sub.put("type", "unique_values");
			} else if (type == CatClassification::natural_breaks_break_vals) {
				sub.put("type", "natural_breaks");
			} else if (type == CatClassification::equal_intervals_break_vals) {
				sub.put("type", "equal_intervals");
			} else if (type == CatClassification::custom_break_vals) {
				sub.put("type", "custom");
			} else {
				// unknown type
			}
			sub.put("num_cats", c.num_cats);
			sub.put("automatic_labels",
					GenUtils::BoolToStr(c.automatic_labels));
			sub.put("assoc_db_fld_name", c.assoc_db_fld_name);
			if (!c.assoc_db_fld_name.IsEmpty()) {
				sub.put("uniform_dist_min", c.uniform_dist_min);
				sub.put("uniform_dist_max", c.uniform_dist_max);
			}
			BOOST_FOREACH(double b, c.breaks) sub.add("breaks.break", b);
			BOOST_FOREACH(const wxString& s, c.names) sub.add("names.name",s);
			subtree.put("classification_definition.colors", "");
			if (c.color_scheme == CatClassification::sequential_color_scheme) {
				sub.put("color_scheme", "sequential");
			} else if (c.color_scheme ==
					   CatClassification::diverging_color_scheme) {
				sub.put("color_scheme", "diverging");
			} else if (c.color_scheme ==
					   CatClassification::qualitative_color_scheme) {
				sub.put("color_scheme", "qualitative");
			} else { // CatClassification::custom_color_scheme 
				sub.put("color_scheme", "custom");
			}
			if (c.color_scheme == CatClassification::custom_color_scheme) {
				BOOST_FOREACH(const wxColour& clr, c.colors) {
					ptree& ssub = sub.add("colors.color", "");
					ssub.put("red", (int) clr.Red());
					ssub.put("green", (int) clr.Green());
					ssub.put("blue", (int) clr.Blue());
				}
			}
		}
	} catch (std::exception &e) {
		throw GdaException(e.what());
	}
}

const std::list<CatClassifDef>& CustomClassifPtree::GetCatClassifList() const
{
	return cc;
}

void CustomClassifPtree::SetCatClassifList(CatClassifManager* cc_manager)
{
	cc.clear();
	if (!cc_manager) return;
	std::vector<wxString> titles;
	cc_manager->GetTitles(titles);
	BOOST_FOREACH(const wxString& t, titles) {
		CatClassifState* ccs = cc_manager->FindClassifState(t);
		if (ccs) cc.push_back(ccs->GetCatClassif());
	}
}

wxString CustomClassifPtree::ToStr() const
{
	using namespace std;
	wxString ss;
	BOOST_FOREACH(const CatClassifDef& c, cc) {
		ss << c.ToStr();
	}
	return ss;
}
