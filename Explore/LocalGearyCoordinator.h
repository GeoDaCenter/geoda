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

#ifndef __GEODA_CENTER_LOCALGEARY_COORDINATOR_H__
#define __GEODA_CENTER_LOCALGEARY_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../VarTools.h"
#include "AbstractCoordinator.h"

using namespace std;

class Project;


class LocalGearyCoordinator : public AbstractCoordinator
{
public:
	enum LocalGearyType {
        univariate,
        bivariate,
        eb_rate_standardized,
        differential,
        multivariate };
	
    LocalGearyCoordinator(boost::uuids::uuid weights_id,
                          Project* project,
                          const vector<GdaVarTools::VarInfo>& var_info,
                          const vector<int>& col_ids,
                          LocalGearyType local_geary_type,
                          bool calc_significances = true,
                          bool row_standardize_s = true);
    
    LocalGearyCoordinator(wxString weights_path,
                          int n,
                          vector<vector<double> >& vars,
                          int permutations_s = 599,
                          bool calc_significances_s = true,
                          bool row_standardize_s = true);
    
	virtual ~LocalGearyCoordinator();
	
	vector<double*> lags_vecs;
	vector<double*> local_geary_vecs;
	vector<double*> data1_vecs;
	vector<double*> data1_square_vecs;
	vector<double*> data2_vecs;
    // These are for multi variable LocalGeary
    vector<vector<double*> > data_vecs;
    vector<vector<double*> > data_square_vecs;
	
    LocalGearyType local_geary_type;
    
	bool isBivariate;
    int num_vars;
   
    virtual void Init();
	virtual void Calc();
   
    virtual void CalcPseudoP_range(int obs_start, int obs_end,
                                   uint64_t seed_start);
    virtual void ComputeLarger(int cnt, std::vector<int> permNeighbors,
                               std::vector<uint64_t>& countLarger) {}
    
    
	virtual void DeallocateVectors();
	virtual void AllocateVectors();
    
    void GetRawData(int time, double* data1, double* data2);
	void StandardizeData();
    
    void CalcMultiLocalGeary();
    void CalcLocalGeary();
};

#endif
