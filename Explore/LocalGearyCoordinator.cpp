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

#include <time.h>
#include <math.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/Randik.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/WeightUtils.h"

#include "../logger.h"
#include "../Project.h"
#include "LocalGearyCoordinatorObserver.h"
#include "LocalGearyCoordinator.h"

using namespace std;

LocalGearyWorkerThread::LocalGearyWorkerThread(int obs_start_s, int obs_end_s,
                                               uint64_t	seed_start_s,
                                               LocalGearyCoordinator* local_geary_coord_s,
                                               wxMutex* worker_list_mutex_s,
                                               wxCondition* worker_list_empty_cond_s,
                                               std::list<wxThread*> *worker_list_s,
                                               int thread_id_s)
: wxThread(),
obs_start(obs_start_s), obs_end(obs_end_s), seed_start(seed_start_s),
local_geary_coord(local_geary_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

LocalGearyWorkerThread::~LocalGearyWorkerThread()
{
}

wxThread::ExitCode LocalGearyWorkerThread::Entry()
{
    LOG_MSG(wxString::Format("LocalGearyWorkerThread %d started", thread_id));
    
    // call work for assigned range of observations
    local_geary_coord->CalcPseudoP_range(obs_start, obs_end, seed_start);
    
    wxMutexLocker lock(*worker_list_mutex);
    
    // remove ourself from the list
    worker_list->remove(this);
    
    // if empty, signal on empty condition since only main thread
    // should be waiting on this condition
    if (worker_list->empty()) {
        worker_list_empty_cond->Signal();
    }
    
    return NULL;
}

LocalGearyCoordinator::LocalGearyCoordinator(boost::uuids::uuid weights_id,
                                Project* project,
                                const vector<GdaVarTools::VarInfo>& var_info_s,
                                const vector<int>& col_ids,
                                LocalGearyType local_geary_type_s,
                                bool calc_significances_s,
                                bool row_standardize_s)
: w_man_state(project->GetWManState()),
w_man_int(project->GetWManInt()),
w_id(weights_id),
num_obs(project->GetNumRecords()),
permutations(999),
local_geary_type(local_geary_type_s),
calc_significances(calc_significances_s),
isBivariate(local_geary_type_s == bivariate),
var_info(var_info_s),
data(var_info_s.size()),
undef_data(var_info_s.size()),
last_seed_used(123456789),
reuse_last_seed(true),
row_standardize(row_standardize_s)
{
    wxLogMessage("In LocalGearyCoordinator::LocalGearyCoordinator()");
    reuse_last_seed = GdaConst::use_gda_user_seed;
    if ( GdaConst::use_gda_user_seed) {
        last_seed_used = GdaConst::gda_user_seed;
    }
	TableInterface* table_int = project->GetTableInt();
	for (int i=0; i<var_info.size(); i++) {
		table_int->GetColData(col_ids[i], data[i]);
        table_int->GetColUndefined(col_ids[i], undef_data[i]);
        var_info[i].is_moran = true;
	}
    num_vars = var_info.size();
	weight_name = w_man_int->GetLongDispName(w_id);
    weights = w_man_int->GetGal(w_id);
	SetSignificanceFilter(1);
	InitFromVarInfo();
	w_man_state->registerObserver(this);
}

LocalGearyCoordinator::LocalGearyCoordinator(wxString weights_path, int n, vector<vector<double> >& vars, int permutations_s, bool calc_significances_s, bool row_standardize_s)
{
    wxLogMessage("In LocalGearyCoordinator::LocalGearyCoordinator()2");
    reuse_last_seed = GdaConst::use_gda_user_seed;
    if ( GdaConst::use_gda_user_seed) {
        last_seed_used = GdaConst::gda_user_seed;
    }
    isBivariate = false;
    num_obs = n;
    num_time_vals = 1;
    permutations = permutations_s;
    calc_significances = calc_significances_s;
    row_standardize = row_standardize_s;
    last_seed_used = 0;
    reuse_last_seed = false;
    num_vars = vars.size();
    
    if (num_vars == 1) {
        local_geary_type = univariate;
    } else {
        local_geary_type = multivariate;
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
    for (int i=0; i < num_vars; i++) {
        for (int j=0; j<num_obs; j++) {
            data[i][0][j] = vars[i][j];
            undef_data[i][0][j] = false;
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
}

LocalGearyCoordinator::~LocalGearyCoordinator()
{
    wxLogMessage("In LocalGearyCoordinator::~LocalGearyCoordinator()");
    if (w_man_state) {
        w_man_state->removeObserver(this);
    }
	DeallocateVectors();
}

void LocalGearyCoordinator::DeallocateVectors()
{
    wxLogMessage("In LocalGearyCoordinator::DeallocateVectors()");
	for (int i=0; i<lags_vecs.size(); i++) {
		if (lags_vecs[i]) delete [] lags_vecs[i];
	}
	lags_vecs.clear();
    
	for (int i=0; i<local_geary_vecs.size(); i++) {
		if (local_geary_vecs[i]) delete [] local_geary_vecs[i];
	}
	local_geary_vecs.clear();
	for (int i=0; i<sig_local_geary_vecs.size(); i++) {
		if (sig_local_geary_vecs[i]) delete [] sig_local_geary_vecs[i];
	}
	sig_local_geary_vecs.clear();
	for (int i=0; i<sig_cat_vecs.size(); i++) {
		if (sig_cat_vecs[i]) delete [] sig_cat_vecs[i];
	}
	sig_cat_vecs.clear();
	for (int i=0; i<cluster_vecs.size(); i++) {
		if (cluster_vecs[i]) delete [] cluster_vecs[i];
	}
	cluster_vecs.clear();
	for (int i=0; i<data1_vecs.size(); i++) {
		if (data1_vecs[i]) delete [] data1_vecs[i];
	}
    
	data1_vecs.clear();
	for (int i=0; i<data1_square_vecs.size(); i++) {
		if (data1_square_vecs[i]) delete [] data1_square_vecs[i];
	}
	data1_square_vecs.clear();
	for (int i=0; i<data2_vecs.size(); i++) {
		if (data2_vecs[i]) delete [] data2_vecs[i];
	}
	data2_vecs.clear();
    
    for (int i=0; i<data_vecs.size(); i++) {
        for (int j=0; j<data_vecs[i].size(); j++) {
            if (data_vecs[i][j]) delete [] data_vecs[i][j];
        }
        data_vecs[i].clear();
    }
    data_vecs.clear();
    for (int i=0; i<data_square_vecs.size(); i++) {
        for (int j=0; j<data_square_vecs[i].size(); j++) {
            if (data_square_vecs[i][j]) delete [] data_square_vecs[i][j];
        }
        data_square_vecs[i].clear();
    }
    data_square_vecs.clear();
    
    // clear W_vecs
    for (size_t i=0; i<has_undefined.size(); i++) {
        if (has_undefined[i]) {
            // clean the copied weights
            delete Gal_vecs[i];
        }
    }
    Gal_vecs.clear();
    
    Gal_vecs_orig.clear();
}

/** allocate based on var_info and num_time_vals **/
void LocalGearyCoordinator::AllocateVectors()
{
    wxLogMessage("In LocalGearyCoordinator::AllocateVectors()");
	int tms = num_time_vals;
    
	lags_vecs.resize(tms);
	local_geary_vecs.resize(tms);
	sig_local_geary_vecs.resize(tms);
	sig_cat_vecs.resize(tms);
	cluster_vecs.resize(tms);
    undef_tms.resize(tms);
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);
   
    if (local_geary_type == multivariate) {
        data_vecs.resize(num_vars);
        data_square_vecs.resize(num_vars);
        for (int i=0; i<num_vars; i++) {
            int tms_at_var = data[i].size();
            data_vecs[i].resize(tms_at_var);
            data_square_vecs[i].resize(tms_at_var);
            for (int j=0; j<tms_at_var; j++) {
                data_vecs[i][j] = new double[num_obs];
                data_square_vecs[i][j] = new double[num_obs];
            }
        }
    } else {
    	data1_vecs.resize(tms);
    	data1_square_vecs.resize(tms);
    }
    
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    
	for (int i=0; i<tms; i++) {
        undef_tms[i].resize(num_obs, false);
		lags_vecs[i] = new double[num_obs];
		local_geary_vecs[i] = new double[num_obs];
		if (calc_significances) {
			sig_local_geary_vecs[i] = new double[num_obs];
			sig_cat_vecs[i] = new int[num_obs];
		}
		cluster_vecs[i] = new int[num_obs];
        
        if (local_geary_type != multivariate) {
    		data1_vecs[i] = new double[num_obs];
    		data1_square_vecs[i] = new double[num_obs];
        }
        
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
	}
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void LocalGearyCoordinator::InitFromVarInfo()
{
    wxLogMessage("In LocalGearyCoordinator::InitFromVarInfo()");
	DeallocateVectors();
	
	num_time_vals = 1;
    is_any_time_variant = false;
    is_any_sync_with_global_time = false;
    ref_var_index = -1;
    
    if (local_geary_type != differential) {
        for (int i=0; i<var_info.size(); i++) {
            if (var_info[i].is_time_variant && var_info[i].sync_with_global_time) {
                num_time_vals = (var_info[i].time_max - var_info[i].time_min) + 1;
                if (num_time_vals < var_info[i].min.size()) {
                    num_time_vals = var_info[i].min.size();
                }
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
	AllocateVectors();
	
    if (local_geary_type == differential) {
        
        int t=0;
        for (int i=0; i<num_obs; i++) {
            int t0 = var_info[0].time;
            int t1 = var_info[1].time;
            data1_vecs[0][i] = data[0][t0][i] - data[0][t1][i];
        }
        
    } else if (local_geary_type == univariate || local_geary_type == bivariate) {
		for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
			int d1_t = t - var_info[0].time_min;
            for (int i=0; i<num_obs; i++) {
                data1_vecs[d1_t][i] = data[0][t][i];
            }
		}
		if (local_geary_type == bivariate) {
			for (int t=var_info[1].time_min; t<=var_info[1].time_max; t++) {
				int d2_t = t - var_info[1].time_min;
				for (int i=0; i<num_obs; i++) {
					data2_vecs[d2_t][i] = data[1][t][i];
				}
			}
		}
        
    } else if (local_geary_type == multivariate) {
        int num_var = data.size();
        for (int i=0; i<num_var; i++) {
            int tms_at_var = data[i].size();
            for (int j=0; j<tms_at_var; j++) {
                for (int k=0; k<num_obs; k++) {
                    data_vecs[i][j][k] = data[i][j][k];
                }
            }
        }
            
	} else { // local_geary_type == eb_rate_standardized
		vector<bool> undef_res(num_obs, false);
		double* smoothed_results = new double[num_obs];
		double* E = new double[num_obs]; // E corresponds to var_info[0]
		double* P = new double[num_obs]; // P corresponds to var_info[1]
		// we will only fill data1 for eb_rate_standardized and
		// further local_geary calcs will treat as univariate
		for (int t=0; t<num_time_vals; t++) {
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
			for (int i=0; i<num_obs; i++) P[i] = data[1][v1_t][i];
			bool success = GdaAlgs::RateStandardizeEB(num_obs, P, E,
														smoothed_results,
														undef_res);
			if (!success) {
				map_valid[t] = false;
				map_error_message[t] << "Emprical Bayes Rate ";
				map_error_message[t] << "Standardization failed.";
			} else {
				for (int i=0; i<num_obs; i++) {
					data1_vecs[t][i] = smoothed_results[i];
				}
			}
		}
		if (smoothed_results) delete [] smoothed_results;
		if (E) delete [] E;
		if (P) delete [] P;
	}
	
	StandardizeData();

    if (local_geary_type == multivariate) {
        for (int v=0; v<data_vecs.size(); v++) {
            for (int t=0; t<data_vecs[v].size(); t++) {
                for (int i=0; i<num_obs; i++) {
                    data_square_vecs[v][t][i] = data_vecs[v][t][i] * data_vecs[v][t][i];
                }
            }
        }
        CalcMultiLocalGeary();
        
    } else {
        for (int t=0; t<num_time_vals; t++) {
            for (int i=0; i<num_obs; i++) {
                data1_square_vecs[t][i] = data1_vecs[t][i] * data1_vecs[t][i];
            }
        }
        CalcLocalGeary();
    }
    
    if (calc_significances) {
        CalcPseudoP();
    }
}

void LocalGearyCoordinator::GetRawData(int time, double* data1, double* data2)
{
    wxLogMessage("In LocalGearyCoordinator::GetRawData()");
    if (local_geary_type == differential) {
        int t=0;
        for (int i=0; i<num_obs; i++) {
            int t0 = var_info[0].time;
            int t1 = var_info[1].time;
            data1[i] = data[0][t0][i] - data[0][t1][i];
        }
        
    } else if (local_geary_type == univariate || local_geary_type == bivariate) {
        for (int i=0; i<num_obs; i++) {
            data1[i] = data[0][time][i];
        }
        if (local_geary_type == bivariate) {
            for (int i=0; i<num_obs; i++) {
                data2[i] = data[1][time][i];
            }
        }
    } else { // local_geary_type == eb_rate_standardized
        vector<bool> undef_res(num_obs, false);
        double* smoothed_results = new double[num_obs];
        double* E = new double[num_obs]; // E corresponds to var_info[0]
        double* P = new double[num_obs]; // P corresponds to var_info[1]
        // we will only fill data1 for eb_rate_standardized and
        // further local_geary calcs will treat as univariate
        for (int i=0; i<num_obs; i++) {
            E[i] = data[0][time][i];
        }
        for (int i=0; i<num_obs; i++) {
            P[i] = data[1][time][i];
        }
        bool success = GdaAlgs::RateStandardizeEB(num_obs, P, E, smoothed_results, undef_res);
        if (success) {
            for (int i=0; i<num_obs; i++) {
                data1[i] = smoothed_results[i];
            }
        }
        if (smoothed_results) delete [] smoothed_results;
        if (E) delete [] E;
        if (P) delete [] P;
    }
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void LocalGearyCoordinator::VarInfoAttributeChange()
{
    wxLogMessage("In LocalGearyCoordinator::VarInfoAttributeChange()");
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
}

void LocalGearyCoordinator::StandardizeData()
{
    wxLogMessage("In LocalGearyCoordinator::StandardizeData()");
    GalElement* w = weights->gal;
    if (local_geary_type == multivariate) {
        // get undef_tms across multi-variables
    	for (int v=0; v<data_vecs.size(); v++) {
            for (int t=0; t<data_vecs[v].size(); t++) {
                for (int i=0; i<num_obs; i++) {
                    undef_tms[t][i] = undef_tms[t][i] || undef_data[v][t][i];
                }
                // the isolates should be excluded as undefined
                for (int i=0; i<num_obs; i++) {
                    if (w[i].Size() == 0) {
                        undef_tms[t][i] = true;
                    }
                }
            }
        }
        
    	for (int v=0; v<data_vecs.size(); v++) {
        	for (int t=0; t<data_vecs[v].size(); t++) {
        		GenUtils::StandardizeData(num_obs, data_vecs[v][t], undef_tms[t]);
        	}
        }
    } else {
        
    	for (int t=0; t<data1_vecs.size(); t++) {
            for (int i=0; i<num_obs; i++) {
                undef_tms[t][i] = undef_tms[t][i] || undef_data[0][t][i];
            }
            if (isBivariate) {
                for (int i=0; i<num_obs; i++) {
                    undef_tms[t][i] = undef_tms[t][i] || undef_data[1][t][i];
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
                GenUtils::StandardizeData(num_obs, data2_vecs[t], undef_tms[t]);
            }
    	}
    }
}

void LocalGearyCoordinator::CalcMultiLocalGeary()
{
    wxLogMessage("In LocalGearyCoordinator::CalcMultiLocalGeary()");
    for (int t=0; t<num_time_vals; t++) {
        // get undefs of objects/values at this time step
        vector<bool> undefs;
        bool has_undef = false;
        for (int i=0; i<num_obs; i++){
            bool is_undef = false;
            for (int v=0; v<undef_data.size(); v++) {
                for (int var_t=0; var_t<undef_data[v].size(); var_t++){
                    is_undef = is_undef || undef_data[v][var_t][i];
                    if (is_undef && !has_undef) {
                        has_undef = true;
                    }
                }
            }
            undefs.push_back(is_undef);
        }
        has_undefined[t] = has_undef;
       
        // local weights copy
        GalWeight* gw = NULL;
        if ( has_undef ) {
            gw = new GalWeight(*weights);
            gw->Update(undefs);
        } else {
            gw = weights;
        }
        GalElement* W = gw->gal;
        Gal_vecs[t] = gw;
        Gal_vecs_orig[t] = weights;
      
        
        // local geary
        double* lags = lags_vecs[t];
        has_isolates[t] = false;
        int* cluster = cluster_vecs[t];
        double* localGeary = local_geary_vecs[t];
        
        vector<int> local_t;
        int delta_t = t - var_info[0].time;
        for (int v=0; v<num_vars; v++) {
            if (data_vecs[v].size()==1) {
                local_t.push_back(0);
            } else {
                int _t = var_info[v].time + delta_t;
                if (_t < var_info[v].time_min) _t = var_info[v].time_min;
                else if (_t > var_info[v].time_max) _t = var_info[v].time_max;
                local_t.push_back(_t);
            }
        }
        
        for (int i=0; i<num_obs; i++) {
            localGeary[i] = 0;
            lags[i] = 0;
            
            if (undefs[i] == true) {
                cluster[i] = MULTIVAR_UNDEFINED_CLUSTER; // undefined value
                continue;
            }
            bool is_binary = true;
            for (int v=0; v<num_vars; v++) {
                int _t = local_t[v];
                double wx = W[i].SpatialLag(data_vecs[v][_t], is_binary, i);
                double wx2 = W[i].SpatialLag(data_square_vecs[v][_t], is_binary, i);
               
                lags[i] += wx;
                localGeary[i] += data_square_vecs[v][_t][i] - 2.0 * data_vecs[v][_t][i] * wx +  wx2;
            }
            
            lags[i] /= num_vars;
            localGeary[i] /= num_vars;
            
            // assign the cluster
            int nn = W[i].Size();
            if (W[i].Check(i)) {
                nn -= 1; // self-neighbor
            }
            if (nn > 0) {
                cluster[i] = 0; // don't assign cluster in multi-var settings
            } else {
                has_isolates[t] = true;
                cluster[i] = MULTIVAR_NEIGHBORLESS_CLUSTER; // neighborless
            }
        }
    }
    wxLogMessage("End LocalGearyCoordinator::CalcMultiLocalGeary()");
}

/** assumes StandardizeData already called on data1 and data2 */
void LocalGearyCoordinator::CalcLocalGeary()
{
    wxLogMessage("In LocalGearyCoordinator::CalcLocalGeary()");
	for (int t=0; t<num_time_vals; t++) {
		double* data1 = data1_vecs[t];
        double* data1_square = data1_square_vecs[t];
        double* data2 = NULL;
        
		if (isBivariate) {
			data2 = data2_vecs[0];
			if (var_info[1].is_time_variant && var_info[1].sync_with_global_time)
                data2 = data2_vecs[t];
		}
		double* lags = lags_vecs[t];
		double* localGeary = local_geary_vecs[t];
		int* cluster = cluster_vecs[t];
	
		has_isolates[t] = false;
    
        // get undefs of objects/values at this time step
        vector<bool> undefs;
        bool has_undef = false;
        for (int i=0; i<undef_data[0][t].size(); i++){
            bool is_undef = undef_data[0][t][i];
            if (isBivariate) {
                is_undef = is_undef || undef_data[1][t][i];
            }
            if (is_undef && !has_undef) {
                has_undef = true;
            }
            undefs.push_back(is_undef);
        }
        has_undefined[t] = has_undef;
       
        // local weights copy
        GalWeight* gw = NULL;
        if ( has_undef ) {
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
                localGeary[i] = 0;
                cluster[i] = UNDEFINED_CLUSTER; // undefined value
                continue;
            }
            
			double Wdata = 0;
            double Wdata2 = 0;
            bool is_binary = true;
			if (isBivariate) {
				Wdata = W[i].SpatialLag(data2, is_binary, i);
			} else {
				Wdata = W[i].SpatialLag(data1, is_binary, i);
                Wdata2 = W[i].SpatialLag(data1_square);
			}
            
			lags[i] = Wdata;
			localGeary[i] = data1_square[i] - 2.0 * data1[i] * Wdata + Wdata2;
				
			// assign the cluster
            int nn = W[i].Size();
            if (W[i].Check(i)) {
                nn -= 1; // self-neighbor
            }
			if (nn > 0) {
				if (data1[i] > 0 && Wdata > 0) cluster[i] = HH_CLUSTER;
				else if (data1[i] < 0 && Wdata > 0) cluster[i] = OTHER_POSITIVE_CLUSTER;
				else if (data1[i] < 0 && Wdata < 0) cluster[i] = LL_CLUSTER;
				else cluster[i] = NEGATIVE_CLUSTER; //data1[i] > 0 && Wdata < 0
			} else {
				has_isolates[t] = true;
				cluster[i] = NEIGHBORLESS_CLUSTER; // neighborless
			}
		}
	}
    wxLogMessage("End LocalGearyCoordinator::CalcLocalGeary()");
}

void LocalGearyCoordinator::CalcPseudoP()
{
    wxLogMessage("In LocalGearyCoordinator::CalcPseudoP()");
	if (!calc_significances)
        return;
    
    CalcPseudoP_threaded();
    wxLogMessage("End LocalGearyCoordinator::CalcPseudoP()");
}

void LocalGearyCoordinator::CalcPseudoP_threaded()
{
    wxLogMessage("In LocalGearyCoordinator::CalcPseudoP_threaded()");
    int nCPUs = GdaConst::gda_cpu_cores;
    if (!GdaConst::gda_set_cpu_cores)
        nCPUs = wxThread::GetCPUCount();
	
	// mutext protects access to the worker_list
    wxMutex worker_list_mutex;
	// signals that worker_list is empty
	wxCondition worker_list_empty_cond(worker_list_mutex);
	worker_list_mutex.Lock(); // mutex should be initially locked
	
    // List of all the threads currently alive.  As soon as the thread
	// terminates, it removes itself from the list.
	list<wxThread*> worker_list;
	
	// divide up work according to number of observations
	// and number of CPUs
	int work_chunk = num_obs / nCPUs;
    if (work_chunk == 0) work_chunk = 1;
    
	int obs_start = 0;
	int obs_end = obs_start + work_chunk;

    bool is_thread_error = false;
	int quotient = num_obs / nCPUs;
	int remainder = num_obs % nCPUs;
	int tot_threads = (quotient > 0) ? nCPUs : remainder;
	if (!reuse_last_seed) last_seed_used = time(0);
    
	for (int i=0; i<tot_threads && !is_thread_error; i++) {
		int a=0;
		int b=0;
		if (i < remainder) {
			a = i*(quotient+1);
			b = a+quotient;
		} else {
			a = remainder*(quotient+1) + (i-remainder)*quotient;
			b = a+quotient-1;
		}
		uint64_t seed_start = last_seed_used + a;
		uint64_t seed_end = seed_start + ((uint64_t) (b-a));
        int thread_id = i+1;
        LocalGearyWorkerThread* thread =
        new LocalGearyWorkerThread(a, b, seed_start, this,
                                   &worker_list_mutex,
                                   &worker_list_empty_cond,
                                   &worker_list, thread_id);
        if ( thread->Create() != wxTHREAD_NO_ERROR ) {
            delete thread;
            is_thread_error = true;
        } else {
            worker_list.push_front(thread);
        }
	}
    if (is_thread_error) {
        // fall back to single thread calculation mode
        CalcPseudoP_range(0, num_obs-1, last_seed_used);
    } else {
        std::list<wxThread*>::iterator it;
        for (it = worker_list.begin(); it != worker_list.end(); it++) {
            (*it)->Run();
        }
        
        while (!worker_list.empty()) {
            // wait until thread_list might be empty
            worker_list_empty_cond.Wait();
            // We have been woken up. If this was not a false
            // alarm (sprious signal), the loop will exit.
        }
    }
    wxLogMessage("End LocalGearyCoordinator::CalcPseudoP_threaded()");
}

void LocalGearyCoordinator::CalcPseudoP_range(int obs_start, int obs_end, uint64_t seed_start)
{
	GeoDaSet workPermutation(num_obs);
	int max_rand = num_obs-1;
    
	for (int cnt=obs_start; cnt<=obs_end; cnt++) {
        std::vector<uint64_t> countLarger(num_time_vals, 0);
        std::vector<std::vector<double> > gci(num_time_vals);
        std::vector<double> gci_sum(num_time_vals, 0);
        
        for (int t=0; t<num_time_vals; t++) gci[t].resize(permutations, 0);
       
        // get full neighbors even if has undefined value
        int numNeighbors = 0;
        GalElement* w ;
        for (int t=0; t<num_time_vals; t++) {
            w = Gal_vecs[t]->gal;
            if (w[cnt].Size() > numNeighbors) {
                numNeighbors = w[cnt].Size();
                if (w[cnt].Check(cnt)) {
                    // exclude self from neighbors
                    numNeighbors -= 1;
                }
            }
        }
        
        if (numNeighbors == 0) {
            continue;
        }
       
		for (int perm=0; perm<permutations; perm++) {
			int rand=0;
			while (rand < numNeighbors) {
				// computing 'perfect' permutation of given size
                double rng_val = Gda::ThomasWangHashDouble(seed_start++) * max_rand;
                // round is needed to fix issue
                //https://github.com/GeoDaCenter/geoda/issues/488
				int newRandom = (int) (rng_val < 0.0 ? ceil(rng_val - 0.5) : floor(rng_val + 0.5));
				if (newRandom != cnt && !workPermutation.Belongs(newRandom) && w[newRandom].Size()>0) {
					workPermutation.Push(newRandom);
					rand++;
				}
			}
            std::vector<int> permNeighbors(numNeighbors);
            for (int cp=0; cp<numNeighbors; cp++) {
                permNeighbors[cp] = workPermutation.Pop();
            }
            // for each time step, reuse permuation
            for (int t=0; t<num_time_vals; t++) {
                std::vector<bool>& undefs = undef_tms[t];
                double* _data1 = NULL;
                double* _data1_square = NULL;
                double* _data2 = NULL;
                vector<double*> current_data(num_vars);
                vector<double*> current_data_square(num_vars);
                
                if (local_geary_type == multivariate) {
                    vector<int> local_t;
                    int delta_t = t - var_info[0].time;
                    for (int v=0; v<num_vars; v++) {
                        if (data_vecs[v].size()==1) {
                            local_t.push_back(0);
                        } else {
                            int _t = var_info[v].time + delta_t;
                            if (_t < var_info[v].time_min) _t = var_info[v].time_min;
                            else if (_t > var_info[v].time_max) _t = var_info[v].time_max;
                            local_t.push_back(_t);
                        }
                    }
                    for (int v=0; v<num_vars; v++) {
                        int _t = local_t[v];
                        current_data[v] = data_vecs[v][_t];
                        current_data_square[v] = data_square_vecs[v][_t];
                    }
                } else {
                    _data1 = data1_vecs[t];
                    _data1_square = data1_square_vecs[t];
                    if (isBivariate) {
                        _data2 = data2_vecs[0];
                        if (var_info[1].is_time_variant &&
                            var_info[1].sync_with_global_time)
                            _data2 = data2_vecs[t];
                    }
                }
                
                double permutedLag=0;
                int validNeighbors = 0;
                if (local_geary_type == multivariate) {
                    double* m_wwx = new double[num_vars];
                    double* m_wwx2 = new double[num_vars];
                    for (int v=0; v<num_vars; v++) {
                        m_wwx[v] = 0;
                        m_wwx2[v] = 0;
                    }
                    for (int cp=0; cp<numNeighbors; cp++) {
                        // xx2 - 2.0 * xx * wwx + wwx2
                        int perm_idx = permNeighbors[cp];
                        if (!undefs[perm_idx]) {
                            validNeighbors ++;
                            for (int v=0; v<num_vars; v++) {
                                m_wwx[v] += current_data[v][perm_idx];
                                m_wwx2[v] += current_data_square[v][perm_idx];
                            }
                        }
                    }
                    if (validNeighbors && row_standardize) {
                        double var_gci = 0;
                        for (int v=0; v<num_vars; v++) {
                            var_gci += current_data_square[v][cnt] - 2.0* current_data[v][cnt]*m_wwx[v]/validNeighbors + m_wwx2[v]/validNeighbors;
                        }
                        var_gci /= num_vars;
                        gci[t][perm] = var_gci;
                    }
                    delete[] m_wwx;
                    delete[] m_wwx2;
                    
                } else {
                    double wwx =0;
                    double wwx2 = 0;
                    if (isBivariate) {
                        for (int cp=0; cp<numNeighbors; cp++) {
                            int perm_idx = permNeighbors[cp];
                            if (!undefs[perm_idx]) {
                                validNeighbors ++;
                                if (_data2) permutedLag += _data2[perm_idx];
                            }
                        }
                    } else {
                        for (int cp=0; cp<numNeighbors; cp++) {
                            // xx2 - 2.0 * xx * wwx + wwx2
                            int perm_idx = permNeighbors[cp];
                            if (!undefs[perm_idx]) {
                                validNeighbors ++;
                                if (_data1 && _data1_square) {
                                    wwx += _data1[perm_idx];
                                    wwx2 += _data1_square[perm_idx];
                                }
                            }
                        }
                    }
                    //NOTE: we shouldn't have to row-standardize or multiply by data1[cnt]
                    if (validNeighbors && row_standardize) {
                        if (_data1_square && _data1) {
                            gci[t][perm] = _data1_square[cnt] - 2.0*_data1[cnt]*wwx/validNeighbors + wwx2/validNeighbors;
                        }
                    }
                }
                gci_sum[t] += gci[t][perm];
            }
		}
        // end permutation
        // for each time step, reuse permuation
        for (int t=0; t<num_time_vals; t++) {
            double* _localGeary = local_geary_vecs[t];
            double* _siglocalGeary = sig_local_geary_vecs[t];
            int* _sigCat = sig_cat_vecs[t];
            int* _cluster = cluster_vecs[t];
            // calc mean of gci
            double gci_mean = gci_sum[t] / permutations;
            if (_localGeary[cnt] <= gci_mean) {
                // positive lisasign[cnt] = 1
                for (int perm=0; perm<permutations; perm++) {
                    if (gci[t][perm] <= _localGeary[cnt]) {
                        countLarger[t] += 1;
                    }
                }
                if (local_geary_type == multivariate) {
                    if (_cluster[cnt] < 2 ) { // ignore neighborless & undefined
                        _cluster[cnt] = MULTIVAR_POSITIVE;
                    }
                } else {
                    // positive && high-high if (cluster[cnt] == 1) cluster[cnt] = 1;
                    // positive && low-low if (cluster[cnt] == 2) cluster[cnt] = 2;
                    // positive && but in outlier qudrant: other pos
                    if (_cluster[cnt] > 2 && _cluster[cnt] < 5) {
                        // ignore neighborless & undefined
                        _cluster[cnt] = OTHER_POSITIVE_CLUSTER;
                    }
                }
            } else {
                // negative lisasign[cnt] = -1
                for (int perm=0; perm<permutations; perm++) {
                    if (gci[t][perm] > _localGeary[cnt]) {
                        countLarger[t] += 1;
                    }
                }
                if (local_geary_type == multivariate) {
                    if (_cluster[cnt] < 2) // ignore neighborless & undefined
                        _cluster[cnt] = MULTIVAR_NEGATIVE; // for multivar, only show significant positive (similar)
                } else {
                    // negative
                    if (_cluster[cnt] < 5) // ignore neighborless & undefined
                        _cluster[cnt] = NEGATIVE_CLUSTER;
                }
            }
            int kp = local_geary_type == multivariate ? num_vars : 1;
            _siglocalGeary[cnt] = (countLarger[t]+1.0)/(permutations+1);
            
            // 'significance' of local Moran
            if (_siglocalGeary[cnt] <= 0.00001) _sigCat[cnt] = 5;
            else if (_siglocalGeary[cnt] <= 0.0001) _sigCat[cnt] = 4;
            else if (_siglocalGeary[cnt] <= 0.001) _sigCat[cnt] = 3;
            else if (_siglocalGeary[cnt] <= 0.01) _sigCat[cnt] = 2;
            else if (_siglocalGeary[cnt] <= 0.05) _sigCat[cnt]= 1;
            else _sigCat[cnt]= 0;
            
            // observations with no neighbors get marked as isolates
            // NOTE: undefined should be marked as well, however, since undefined_cat has covered undefined category, we don't need to handle here
            if (numNeighbors == 0) {
                _sigCat[cnt] = 6;
            }

        }
        
    }
}

void LocalGearyCoordinator::SetSignificanceFilter(int filter_id)
{
    wxLogMessage("In LocalGearyCoordinator::SetSignificanceFilter()");
    if (filter_id == -1) {
        // user input cutoff
        significance_filter = filter_id;
        return;
    }
	// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001 5: 0.00001
	if (filter_id < 1 || filter_id > 5) return;
	significance_filter = filter_id;
    
    //int kp = local_geary_type == multivariate ? num_vars : 1;
    
	if (filter_id == 1) significance_cutoff = 0.05;
	if (filter_id == 2) significance_cutoff = 0.01;
	if (filter_id == 3) significance_cutoff = 0.001;
	if (filter_id == 4) significance_cutoff = 0.0001;
    if (filter_id == 5) significance_cutoff = 0.00001;
}

void LocalGearyCoordinator::update(WeightsManState* o)
{
    wxLogMessage("In LocalGearyCoordinator::update(WeightsManState)");
    if (w_man_int) {
        weight_name = w_man_int->GetLongDispName(w_id);
    }
}

int LocalGearyCoordinator::numMustCloseToRemove(boost::uuids::uuid id) const
{
    wxLogMessage("In LocalGearyCoordinator::numMustCloseToRemove()");
	return id == w_id ? observers.size() : 0;
}

void LocalGearyCoordinator::closeObserver(boost::uuids::uuid id)
{
    wxLogMessage("In LocalGearyCoordinator::closeObserver()");
	if (numMustCloseToRemove(id) == 0) return;
	list<LocalGearyCoordinatorObserver*> obs_cpy = observers;
	for (list<LocalGearyCoordinatorObserver*>::iterator i=obs_cpy.begin();
		 i != obs_cpy.end(); ++i) {
		(*i)->closeObserver(this);
	}
}

void LocalGearyCoordinator::registerObserver(LocalGearyCoordinatorObserver* o)
{
    wxLogMessage("In LocalGearyCoordinator::registerObserver()");
    
	observers.push_front(o);
}

void LocalGearyCoordinator::removeObserver(LocalGearyCoordinatorObserver* o)
{
    wxLogMessage("In LocalGearyCoordinator::removeObserver()");
	observers.remove(o);
	if (observers.size() == 0) {
		delete this;
	}
}

void LocalGearyCoordinator::notifyObservers()
{
    wxLogMessage("In LocalGearyCoordinator::notifyObservers()");
	for (list<LocalGearyCoordinatorObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

