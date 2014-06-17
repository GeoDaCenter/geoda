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

#ifndef __GEODA_CENTER_SHP_2_CNT_H__
#define __GEODA_CENTER_SHP_2_CNT_H__

#include <wx/filename.h>
#include "GalWeight.h"
#include <vector>
#include "ShpFile.h"

bool IsLineShapeFile(const wxString& fname);
#define geoda_sqr(x) ( (x) * (x) )
GalElement* HOContiguity(const int p, long obs, GalElement *W, bool Lag);
//GalElement* shp2gal(const wxString& fname, int criteria, bool save= true);
GalElement* shp2gal(Shapefile::Main& main, int criteria, bool save= true,
                    double precision_threshold=0.0);

bool SaveGal(const GalElement *full, const wxString& layer_name, 
			 const wxString& ifname, //<- no need to be file name 
			 const wxString& vname, const std::vector<wxInt64>& id_vec);

void DevFromMean(int, double*);


#endif

