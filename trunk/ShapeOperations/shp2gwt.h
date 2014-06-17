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

#ifndef __GEODA_CENTER_SHP_2_GWT_H__
#define __GEODA_CENTER_SHP_2_GWT_H__

#include "GwtWeight.h"
#include "shp.h"
#include <wx/string.h>
#include <vector>

class GwtElement;

bool CreateGridShapeFile(wxString otfl, int nRows, int nCols,
						 double *xg, double *yg, myBox myfBox);
bool CreateSHPfromBoundary(wxString ifl, wxString otfl);
double ComputeCutOffPoint(const std::vector<double>& x,
						  const std::vector<double>& y, int method);

double ComputeMaxDistance(const std::vector<double>& x,
						  const std::vector<double>& y, int method);

GwtElement* DynKNN(const std::vector<double>& x, const std::vector<double>& y,
				   int k, int method);

GwtElement* shp2gwt(int Obs, std::vector<double>& x, std::vector<double>& y,
					const double threshold, const int degree,
					int method);

bool WriteGwt(const GwtElement *g,
			  const wxString& layer_name, const wxString& ofname, 
			  const wxString& vname, const std::vector<wxInt64>& id_vec,
			  const int degree, bool gl);



#endif

