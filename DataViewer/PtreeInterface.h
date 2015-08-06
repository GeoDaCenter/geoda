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


#include <boost/property_tree/ptree_fwd.hpp>
#include <wx/string.h>
#include <string>
#include <sstream>

#ifndef __GEODA_CENTER_PTREE_INTERFACE_H__
#define __GEODA_CENTER_PTREE_INTERFACE_H__

/**
 * Interface for classes that read/write state startup information
 * from Boost Property Trees.  Throws GdaException when errors encountered 
 */
class PtreeInterface {
public:
	/** Read subtree starting from passed in node pt */
	virtual void ReadPtree(const boost::property_tree::ptree& pt,
						   const wxString& proj_path) = 0;
	/** Write subtree starting from passed in node pt */
	virtual void WritePtree(boost::property_tree::ptree& pt,
							const wxString& proj_path) = 0;
};

#endif
