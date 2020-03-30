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

#ifndef __GEODA_CENTER_LISA_COORDINATOR_H__
#define __GEODA_CENTER_LISA_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../VarTools.h"
#include "AbstractCoordinator.h"


class Project;

class LisaCoordinator : public AbstractCoordinator
{
public:
	enum LisaType { univariate, bivariate, eb_rate_standardized, differential };
	
	LisaCoordinator(boost::uuids::uuid weights_id,
                    Project* project,
					const std::vector<GdaVarTools::VarInfo>& var_info,
					const std::vector<int>& col_ids,
					LisaType lisa_type,
                    bool calc_significances = true,
                    bool row_standardize_s = true,
                    bool using_median = false);
    
    LisaCoordinator(wxString weights_path,
                    int n,
                    std::vector<double> vals_1,
                    std::vector<double> vals_2,
                    int lisa_type_s = 0,
                    int permutations_s = 599,
                    bool calc_significances_s = true,
                    bool row_standardize_s = true);
    
	virtual ~LisaCoordinator();
	

protected:
	// The following seven are just temporary pointers into the corresponding
	// space-time data arrays below
	double* lags;
	double*	localMoran;		// The LISA
	double* sigLocalMoran;	// The significances / pseudo p-vals
    bool using_median;

public:
    std::vector<double*> smoothed_results; // LISA EB
	std::vector<double*> lags_vecs;
	std::vector<double*> local_moran_vecs;
	std::vector<double*> data1_vecs;
	std::vector<double*> data2_vecs;

	bool isBivariate;
	LisaType lisa_type;
	
    virtual void ComputeLarger(int cnt, std::vector<int>& permNeighbors,
                               std::vector<uint64_t>& countLarger);
	virtual void Init();
    virtual void Calc();
	virtual void DeallocateVectors();
	virtual void AllocateVectors();
    virtual void CalcPseudoP();
    
    void GetRawData(int time, double* data1, double* data2);
	void StandardizeData();
};

#endif
