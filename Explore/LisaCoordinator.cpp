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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <time.h>
#include <math.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include <wx/msgdlg.h>

#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/Randik.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../logger.h"
#include "../Project.h"
#include "LisaCoordinator.h"

#include "../Algorithms/gpu_lisa.h"

/** 
 Since the user has the ability to synchronise either variable over time,
 we must be able to reapply weights and recalculate lisa values as needed.
 
 1. We will have original data as complete space-time data for both variables
 
 2. From there we will work from info in var_info for both variables.  Must
    determine number of time_steps for canvas.
 
 3. Adjust data1(2)_vecs sizes and initialize from data.
 
 3.5. Resize localMoran, sigLocalMoran, sigCat, and cluster arrays
 
 4. If rates, then calculate rates for working_data1
 
 5. Standardize working_data1 (and 2 if bivariate)
 
 6. Compute LISA for all time-stesp and save in localMoran sp/time array
 
 7. Calc Pseudo P for all time periods.  Results saved in sigLocalMoran,
    sigCat and cluster arrays
 
 8. Notify clients that values have been updated.
   
 */

LisaCoordinator::
LisaCoordinator(boost::uuids::uuid weights_id,
                Project* project,
                const std::vector<GdaVarTools::VarInfo>& var_info_s,
                const std::vector<int>& col_ids,
                LisaType lisa_type_s,
                bool calc_significances_s,
                bool row_standardize_s)
: AbstractCoordinator(weights_id, project, var_info_s, col_ids, calc_significances_s, row_standardize_s),
lisa_type(lisa_type_s),
isBivariate(lisa_type_s == bivariate)
{
    wxLogMessage("Entering LisaCoordinator::LisaCoordinator().");
	for (int i=0; i<var_info.size(); i++) {
        var_info[i].is_moran = true;
	}
    InitFromVarInfo(); // call to init calculation
    wxLogMessage("Exiting LisaCoordinator::LisaCoordinator().");
}


LisaCoordinator::
LisaCoordinator(wxString weights_path,
                int n,
                std::vector<double> vals_1,
                std::vector<double> vals_2,
                int lisa_type_s,
                int permutations_s,
                bool calc_significances_s,
                bool row_standardize_s)
: AbstractCoordinator()
{
    wxLogMessage("Entering LisaCoordinator::LisaCoordinator()2.");
    num_obs = n;
    num_time_vals = 1;
    permutations = permutations_s;
    calc_significances = calc_significances_s;
    row_standardize = row_standardize_s;
    last_seed_used = 0;
    reuse_last_seed = false;
    isBivariate = false;

    // std::vector<GdaVarTools::VarInfo> var_info;
    int num_vars = 1;
    isBivariate = false;
    
    if (lisa_type_s == 0) {
        lisa_type = univariate;
        
    } else if (lisa_type_s == 1) {
        lisa_type = bivariate;
        isBivariate = true;
        num_vars = 2;
        
    } else if (lisa_type_s == 2) {
        lisa_type = eb_rate_standardized;
        num_vars = 2;
        
    } else if (lisa_type_s == 3) {
        lisa_type = differential;
        num_vars = 2;
    }
    
    undef_tms.resize(num_time_vals);
    data.resize(num_vars);
    undef_data.resize(num_vars);
    var_info.resize(num_vars);
    
    // don't handle time variable for now
    for (int i=0; i<var_info.size(); i++) {
        data[i].resize(boost::extents[num_time_vals][num_obs]);
        undef_data[i].resize(boost::extents[num_time_vals][num_obs]);
        var_info[i].is_moran = true;
        var_info[i].is_time_variant = false;
        var_info[i].fixed_scale = true;
        var_info[i].sync_with_global_time  = false;
        var_info[i].time_max = 0;
        var_info[i].time_min = 0;
    }
    
    for (int i=0; i<num_obs; i++) {
        data[0][0][i] = vals_1[i];
        undef_data[0][0][i] = false;
    }
    if (num_vars == 2) {
        for (int i=0; i<num_obs; i++) {
            data[1][0][i] = vals_1[i];
            undef_data[1][0][i] = false;
        }
    }
    
    // create weights
    w_man_state = NULL;
    w_man_int = NULL;
    
    wxString ext = GenUtils::GetFileExt(weights_path).Lower();
    GalElement* tempGal = 0;
    if (ext == "gal") {
        tempGal = WeightUtils::ReadGal(weights_path, NULL);
    } else {
        tempGal = WeightUtils::ReadGwtAsGal(weights_path, NULL);
    }
    
    weights = new GalWeight();
    weights->num_obs = num_obs;
    weights->wflnm = weights_path;
    weights->id_field = "ogc_fid";
    weights->gal = tempGal;
    
    SetSignificanceFilter(1);
    InitFromVarInfo();
    wxLogMessage("Exiting LisaCoordinator::LisaCoordinator()2.");
}

LisaCoordinator::~LisaCoordinator()
{
    wxLogMessage("In LisaCoordinator::~LisaCoordinator().");
	DeallocateVectors();
}

void LisaCoordinator::DeallocateVectors()
{
    wxLogMessage("Entering LisaCoordinator::DeallocateVectors()");
	for (int i=0; i<lags_vecs.size(); i++) {
		if (lags_vecs[i]) delete [] lags_vecs[i];
	}
	lags_vecs.clear();
    
	for (int i=0; i<local_moran_vecs.size(); i++) {
		if (local_moran_vecs[i]) delete [] local_moran_vecs[i];
	}
	local_moran_vecs.clear();
	
	for (int i=0; i<data1_vecs.size(); i++) {
		if (data1_vecs[i]) delete [] data1_vecs[i];
	}
	data1_vecs.clear();
	for (int i=0; i<data2_vecs.size(); i++) {
		if (data2_vecs[i]) delete [] data2_vecs[i];
	}
	data2_vecs.clear();

    for (int i=0; i<smoothed_results.size(); i++) {
        if (smoothed_results[i]) delete [] smoothed_results[i];
    }
    smoothed_results.clear();

    wxLogMessage("Exiting LisaCoordinator::DeallocateVectors()");
}

/** allocate based on var_info and num_time_vals **/
void LisaCoordinator::AllocateVectors()
{
    wxLogMessage("Entering LisaCoordinator::AllocateVectors()");
	int tms = num_time_vals;
    
	lags_vecs.resize(tms);
	local_moran_vecs.resize(tms);
	data1_vecs.resize(tms);
    smoothed_results.resize(tms);

	for (int i=0; i<tms; i++) {
		lags_vecs[i] = new double[num_obs];
		local_moran_vecs[i] = new double[num_obs];
		data1_vecs[i] = new double[num_obs];
        smoothed_results[i] = new double[num_obs];
	}
	
	if (lisa_type == bivariate) {
        int stps = var_info[1].time_max - var_info[1].time_min + 1;
		data2_vecs.resize(stps);
		for (int i=0; i<data2_vecs.size(); i++) {
			data2_vecs[i] = new double[num_obs];
		}
	}
    wxLogMessage("Exiting LisaCoordinator::AllocateVectors()");
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void LisaCoordinator::Init()
{
    wxLogMessage("Entering LisaCoordinator::Init()");
    
	num_time_vals = 1;
    is_any_time_variant = false;
    is_any_sync_with_global_time = false;
    ref_var_index = -1;
    
    if (lisa_type != differential) {
        for (int i=0; i<var_info.size(); i++) {
            if (var_info[i].is_time_variant && var_info[i].sync_with_global_time)
            {
                num_time_vals = (var_info[i].time_max - var_info[i].time_min) + 1;
                is_any_sync_with_global_time = true;
                ref_var_index = i;
                break;
            }
        }
        for (int i=0; i<var_info.size(); i++) {
            if (var_info[i].is_time_variant) {
                is_any_time_variant = true;
                break;
            }
        }
    }
    
    DeallocateVectors();
    AllocateVectors();

    if (lisa_type == differential) {
        int t=0;
        for (int i=0; i<num_obs; i++) {
            int t0 = var_info[0].time;
            int t1 = var_info[1].time;
            data1_vecs[0][i] = data[0][t0][i] - data[0][t1][i];
        }
        
    } else if (lisa_type == univariate || lisa_type == bivariate) {
		for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
			int d1_t = t - var_info[0].time_min;
            for (int i=0; i<num_obs; i++) {
                data1_vecs[d1_t][i] = data[0][t][i];
            }
		}
		if (lisa_type == bivariate) {
			for (int t=var_info[1].time_min; t<=var_info[1].time_max; t++) {
				int d2_t = t - var_info[1].time_min;
				for (int i=0; i<num_obs; i++) {
					data2_vecs[d2_t][i] = data[1][t][i];
				}
			}
		}
	} else { // lisa_type == eb_rate_standardized
		std::vector<bool> undef_res(num_obs, false);

		double* E = new double[num_obs]; // E corresponds to var_info[0]
		double* P = new double[num_obs]; // P corresponds to var_info[1]
		// we will only fill data1 for eb_rate_standardized and
		// further lisa calcs will treat as univariate
		for (int t=0; t<num_time_vals; t++) {
            double* local_smoothed_results = smoothed_results[t];
			int v0_t = var_info[0].time_min;
			if (var_info[0].is_time_variant &&
				var_info[0].sync_with_global_time) {
				v0_t += t;
			}
			for (int i=0; i<num_obs; i++) E[i] = data[0][v0_t][i];
			int v1_t = var_info[1].time_min;
			if (var_info[1].is_time_variant &&
				var_info[1].sync_with_global_time) {
				v1_t += t;
			}
            for (int i=0; i<num_obs; i++) {
                P[i] = data[1][v1_t][i];
            }
			bool success = GdaAlgs::RateStandardizeEB(num_obs, P, E, local_smoothed_results, undef_res);
			if (!success) {
                for (int i=0; i<num_obs; i++) {
                    undef_data[0][t][i] = undef_data[0][t][i] || undef_res[i];
                }
			}
            for (int i=0; i<num_obs; i++) {
                data1_vecs[t][i] = local_smoothed_results[i];
            }
		}
		if (E) delete [] E;
		if (P) delete [] P;
	}
	
	StandardizeData();

    wxLogMessage("Exiting LisaCoordinator::Init()");
}

/* used by scatter plot */
void LisaCoordinator::GetRawData(int time, double* data1, double* data2)
{
    wxLogMessage("Entering LisaCoordinator::GetRawData()");
    if (lisa_type == differential) {
        int t=0;
        for (int i=0; i<num_obs; i++) {
            int t0 = var_info[0].time;
            int t1 = var_info[1].time;
            data1[i] = data[0][t0][i] - data[0][t1][i];
        }
        
    } else if (lisa_type == univariate || lisa_type == bivariate) {
        for (int i=0; i<num_obs; i++) {
            data1[i] = data[0][time][i];
        }
        if (lisa_type == bivariate) {
            for (int i=0; i<num_obs; i++) {
                data2[i] = data[1][time][i];
            }
        }
    } else { // lisa_type == eb_rate_standardized
        std::vector<bool> undef_res(num_obs, false);
        double* local_smoothed_results = smoothed_results[time];
        double* E = new double[num_obs]; // E corresponds to var_info[0]
        double* P = new double[num_obs]; // P corresponds to var_info[1]
        // we will only fill data1 for eb_rate_standardized and
        // further lisa calcs will treat as univariate
        for (int i=0; i<num_obs; i++) {
            E[i] = data[0][time][i];
        }
        for (int i=0; i<num_obs; i++) {
            P[i] = data[1][time][i];
        }
        bool success = GdaAlgs::RateStandardizeEB(num_obs, P, E,
                                                  local_smoothed_results,
                                                  undef_res);
        if (!success) {
            for (int i=0; i<num_obs; i++) {
                undef_data[0][time][i] = undef_data[0][time][i] || undef_res[i];
            }
        }
        for (int i=0; i<num_obs; i++) {
            data1[i] = local_smoothed_results[i];
        }
        if (E) delete [] E;
        if (P) delete [] P;
    }
    wxLogMessage("Exiting LisaCoordinator::GetRawData()");
}

void LisaCoordinator::StandardizeData()
{
    wxLogMessage("Entering LisaCoordinator::StandardizeData()");
    GalElement* w = weights->gal;
    
	for (int t=0; t<data1_vecs.size(); t++) {
        undef_tms[t].resize(num_obs);
        
        for (int i=0; i<num_obs; i++) {
            undef_tms[t][i] = undef_tms[t][i] || undef_data[0][t][i];
        }
        if (isBivariate) {
            for (int i=0; i<num_obs; i++) {
                if ( undef_data[1].size() > t ) {
                    undef_tms[t][i] = undef_tms[t][i] || undef_data[1][t][i];
                }
            }
        }
        
        // the isolates should be excluded as undefined
        for (int i=0; i<num_obs; i++) {
            if (w[i].Size() == 0) {
                undef_tms[t][i] = true;
            }
        }
    }
    
	for (int t=0; t<data1_vecs.size(); t++) {
		GenUtils::StandardizeData(num_obs, data1_vecs[t], undef_tms[t]);
        if (isBivariate) {
            if (data2_vecs.size() > t)
                GenUtils::StandardizeData(num_obs, data2_vecs[t], undef_tms[t]);
        }
	}
    wxLogMessage("Exiting LisaCoordinator::StandardizeData()");
}

/** assumes StandardizeData already called on data1 and data2 */
void LisaCoordinator::Calc()
{
    wxLogMessage("Entering LisaCoordinator::Calc()");
    double *data1 = NULL;
    double *data2 = NULL;
    int* cluster;
    
	for (int t=0; t<num_time_vals; t++) {
		data1 = data1_vecs[t];
		if (isBivariate) {
			data2 = data2_vecs[0];
            if (var_info[1].is_time_variant && var_info[1].sync_with_global_time) {
                data2 = data2_vecs[t];
            }
		}
		lags = lags_vecs[t];
		localMoran = local_moran_vecs[t];
		cluster = cluster_vecs[t];
	
		has_isolates[t] = false;
    
        // get undefs of objects/values at this time step
        std::vector<bool> undefs;
        bool has_undef = false;
        bool has_isolate = false;
        for (int i=0; i<undef_data[0][t].size(); i++){
            bool is_undef = undef_data[0][t][i];
            if (isBivariate) {
                if (undef_data[1].size() > t) {
                    is_undef = is_undef || undef_data[1][t][i];
                }
            }
            if (is_undef && !has_undef) {
                has_undef = true;
            }
            if (weights->gal[i].Size() == 0) {
                is_undef = true;
                has_isolate = true;
            }
            undefs.push_back(is_undef);
        }
        has_undefined[t] = has_undef;
       
        // local weights copy
        GalWeight* gw = NULL;
        if ( has_undef || has_isolate ) {
            gw = new GalWeight(*weights);
            gw->Update(undefs);
        } else {
            gw = weights;
        }
        GalElement* W = gw->gal;
        Gal_vecs[t] = gw;
        Gal_vecs_orig[t] = weights;
	
		for (int i=0; i<num_obs; i++) {
            if (undefs[i] == true) {
                lags[i] = 0;
                localMoran[i] = 0;
                cluster[i] = 6; // undefined value
                
                if (W[i].Size() == 0) {
                    has_isolates[t] = true;
                    cluster[i] = 5; // neighborless
                }
                continue;
            }
            
			double Wdata = 0;
			if (isBivariate) {
                if (data2) Wdata = W[i].SpatialLag(data2);
			} else {
				if (data1) Wdata = W[i].SpatialLag(data1);
			}
            
			lags[i] = Wdata;
            if (data1) {
                localMoran[i] = data1[i] * Wdata;
            }
				
			// assign the cluster
			//if (W[i].Size() > 0) {
            if (data1) {
                if (data1[i] > 0 && Wdata < 0) cluster[i] = 4;
                else if (data1[i] < 0 && Wdata > 0) cluster[i] = 3;
                else if (data1[i] < 0 && Wdata < 0) cluster[i] = 2;
                else cluster[i] = 1; //data1[i] > 0 && Wdata > 0
            }
		}
	}
    wxLogMessage("Exiting LisaCoordinator::Calc()");
}

void LisaCoordinator::CalcPseudoP()
{
    wxStopWatch sw_vd;
    
    if (GdaConst::gda_use_gpu == false) {
        if (!calc_significances)
            return;
        CalcPseudoP_threaded();
        
    } else {
        double* values = data1_vecs[0];
        double* local_moran = local_moran_vecs[0];
        GalElement* w = weights->gal;
        double* _sigLocal = sig_local_vecs[0];
        
        wxString exePath = GenUtils::GetExeDir();
        wxString clPath = exePath + "lisa_kernel.cl";
        bool flag = gpu_lisa(clPath.mb_str(), num_obs, permutations, last_seed_used, values, local_moran, w, _sigLocal);
        
		if (flag) {
		   for (int cnt=0; cnt<num_obs; cnt++) {
		       int numNeighbors = w[cnt].Size();
		      int* _sigCat = sig_cat_vecs[0];
			    if (_sigLocal[cnt] <= 0.0001) _sigCat[cnt] = 4;
			  else if (_sigLocal[cnt] <= 0.001) _sigCat[cnt] = 3;
			 else if (_sigLocal[cnt] <= 0.01) _sigCat[cnt] = 2;
			 else if (_sigLocal[cnt] <= 0.05) _sigCat[cnt]= 1;
            else _sigCat[cnt]= 0;
            
            if (numNeighbors == 0) {
                _sigCat[cnt] = 5;
            }
		 }
		} else {
			wxMessageDialog dlg(NULL, "GeoDa can't configure GPU device. Default CPU solution will be used instead.", _("Error"), wxOK | wxICON_ERROR);
			dlg.ShowModal();
			if (!calc_significances)
				return;
			CalcPseudoP_threaded();
		}
    }
    LOG_MSG(wxString::Format("GPU took %ld ms", sw_vd.Time()));
}

void LisaCoordinator::ComputeLarger(int cnt, std::vector<int>& permNeighbors, std::vector<uint64_t>& countLarger)
{
    // for each time step, reuse permuation
    for (int t=0; t<num_time_vals; t++) {
        double *data1;
        double *data2;
        double *localMoran = local_moran_vecs[t];
        std::vector<bool>& undefs = undef_tms[t];
        
        data1 = data1_vecs[t];
        if (isBivariate) {
            data2 = data2_vecs[0];
            if (var_info[1].is_time_variant && var_info[1].sync_with_global_time)
                data2 = data2_vecs[t];
        }
        
        int validNeighbors = 0;
        double permutedLag = 0;
        int numNeighbors = permNeighbors.size();
        // use permutation to compute the lag
        // compute the lag for binary weights
        if (isBivariate) {
            for (int cp=0; cp<numNeighbors; cp++) {
                int nb = permNeighbors[cp];
                if (!undefs[nb]) {
                    permutedLag += data2[nb];
                    validNeighbors ++;
                }
            }
        } else {
            for (int cp=0; cp<numNeighbors; cp++) {
                int nb = permNeighbors[cp];
                if (!undefs[nb]) {
                    permutedLag += data1[nb];
                    validNeighbors ++;
                }
            }
        }
        
        //NOTE: we shouldn't have to row-standardize or
        // multiply by data1[cnt]
        if (validNeighbors > 0 && row_standardize) {
            permutedLag /= validNeighbors;
        }
        const double localMoranPermuted = permutedLag * data1[cnt];
        if (localMoranPermuted >= localMoran[cnt]) {
            countLarger[t]++;
        }
    }
}
