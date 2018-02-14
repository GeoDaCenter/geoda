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
#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/RateSmoothing.h"
#include "../ShapeOperations/Randik.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../ShapeOperations/WeightUtils.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../logger.h"
#include "../Project.h"
#include "AbstractCoordinator.h"

AbstractWorkerThread::AbstractWorkerThread(int obs_start_s, int obs_end_s,
								   uint64_t	seed_start_s,
								   AbstractCoordinator* a_coord_s,
								   wxMutex* worker_list_mutex_s,
								   wxCondition* worker_list_empty_cond_s,
								   std::list<wxThread*> *worker_list_s,
								   int thread_id_s)
: wxThread(),
obs_start(obs_start_s), obs_end(obs_end_s), seed_start(seed_start_s),
a_coord(a_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

AbstractWorkerThread::~AbstractWorkerThread()
{
}

wxThread::ExitCode AbstractWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("AbstractWorkerThread %d started", thread_id));

	// call work for assigned range of observations
	a_coord->CalcPseudoP_range(obs_start, obs_end, seed_start);
	
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

AbstractCoordinator::
AbstractCoordinator(boost::uuids::uuid weights_id,
                Project* project,
                const std::vector<GdaVarTools::VarInfo>& var_info_s,
                const std::vector<int>& col_ids,
                bool calc_significances_s,
                bool row_standardize_s)
: w_man_state(project->GetWManState()),
w_man_int(project->GetWManInt()),
w_id(weights_id),
num_obs(project->GetNumRecords()),
permutations(999),
calc_significances(calc_significances_s),
var_info(var_info_s),
data(var_info_s.size()),
undef_data(var_info_s.size()),
last_seed_used(123456789), reuse_last_seed(true),
row_standardize(row_standardize_s),
user_sig_cutoff(0)
{
    wxLogMessage("Entering AbstractCoordinator::AbstractCoordinator().");
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
    
    undef_tms.resize(var_info_s[0].time_max - var_info_s[0].time_min + 1);
	
	weight_name = w_man_int->GetLongDispName(w_id);
    
    weights = w_man_int->GetGal(w_id);
    
	SetSignificanceFilter(1);
    
	InitFromVarInfo();
	w_man_state->registerObserver(this);
    wxLogMessage("Exiting AbstractCoordinator::AbstractCoordinator().");
}


AbstractCoordinator::
AbstractCoordinator(wxString weights_path,
                int n,
                std::vector<double> vals_1,
                std::vector<double> vals_2,
                int lisa_type_s,
                int permutations_s,
                bool calc_significances_s,
                bool row_standardize_s)
{
    wxLogMessage("Entering AbstractCoordinator::AbstractCoordinator()2.");
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
    wxLogMessage("Exiting AbstractCoordinator::AbstractCoordinator()2.");
}

AbstractCoordinator::~AbstractCoordinator()
{
    wxLogMessage("In AbstractCoordinator::~AbstractCoordinator().");
    if (w_man_state) {
        w_man_state->removeObserver(this);
    }
	DeallocateVectors();
}

void AbstractCoordinator::DeallocateVectors()
{
    wxLogMessage("Entering AbstractCoordinator::DeallocateVectors()");
	for (int i=0; i<lags_vecs.size(); i++) {
		if (lags_vecs[i]) delete [] lags_vecs[i];
	}
	lags_vecs.clear();
    
	for (int i=0; i<local_moran_vecs.size(); i++) {
		if (local_moran_vecs[i]) delete [] local_moran_vecs[i];
	}
	local_moran_vecs.clear();
	for (int i=0; i<sig_local_moran_vecs.size(); i++) {
		if (sig_local_moran_vecs[i]) delete [] sig_local_moran_vecs[i];
	}
	sig_local_moran_vecs.clear();
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
	for (int i=0; i<data2_vecs.size(); i++) {
		if (data2_vecs[i]) delete [] data2_vecs[i];
	}
	data2_vecs.clear();
    
    // clear W_vecs
    for (size_t i=0; i<has_undefined.size(); i++) {
        if (has_undefined[i]) {
            // clean the copied weights
            delete Gal_vecs[i];
        }
    }
    Gal_vecs.clear();
    
    Gal_vecs_orig.clear();
    wxLogMessage("Exiting AbstractCoordinator::DeallocateVectors()");
}

/** allocate based on var_info and num_time_vals **/
void AbstractCoordinator::AllocateVectors()
{
    wxLogMessage("Entering AbstractCoordinator::AllocateVectors()");
	int tms = num_time_vals;
    
	lags_vecs.resize(tms);
	local_moran_vecs.resize(tms);
	sig_local_moran_vecs.resize(tms);
	sig_cat_vecs.resize(tms);
	cluster_vecs.resize(tms);
	data1_vecs.resize(tms);
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);
    
	for (int i=0; i<tms; i++) {
		lags_vecs[i] = new double[num_obs];
		local_moran_vecs[i] = new double[num_obs];
		if (calc_significances) {
			sig_local_moran_vecs[i] = new double[num_obs];
			sig_cat_vecs[i] = new int[num_obs];
		}
		cluster_vecs[i] = new int[num_obs];
		data1_vecs[i] = new double[num_obs];
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
	}
	
	if (lisa_type == bivariate) {
		data2_vecs.resize((var_info[1].time_max - var_info[1].time_min) + 1);
		for (int i=0; i<data2_vecs.size(); i++) {
			data2_vecs[i] = new double[num_obs];
		}
	}
    wxLogMessage("Exiting AbstractCoordinator::AllocateVectors()");
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void AbstractCoordinator::InitFromVarInfo()
{
    wxLogMessage("Entering AbstractCoordinator::InitFromVarInfo()");
	DeallocateVectors();
	
	num_time_vals = 1;
    is_any_time_variant = false;
    is_any_sync_with_global_time = false;
    ref_var_index = -1;
    
    if (lisa_type != differential) {
        for (int i=0; i<var_info.size(); i++) {
            if (var_info[i].is_time_variant && var_info[i].sync_with_global_time) {
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
		double* smoothed_results = new double[num_obs];
		double* E = new double[num_obs]; // E corresponds to var_info[0]
		double* P = new double[num_obs]; // P corresponds to var_info[1]
		// we will only fill data1 for eb_rate_standardized and
		// further lisa calcs will treat as univariate
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
	
    CalcLisa();
    
    if (calc_significances) {
        CalcPseudoP();
    }
    
    wxLogMessage("Exiting AbstractCoordinator::InitFromVarInfo()");
}

void AbstractCoordinator::GetRawData(int time, double* data1, double* data2)
{
    wxLogMessage("Entering AbstractCoordinator::GetRawData()");
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
        double* smoothed_results = new double[num_obs];
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
                                                  smoothed_results,
                                                  undef_res);
        if (success) {
            for (int i=0; i<num_obs; i++) {
                data1[i] = smoothed_results[i];
            }
        }
        if (smoothed_results) delete [] smoothed_results;
        if (E) delete [] E;
        if (P) delete [] P;
    }
    wxLogMessage("Exiting AbstractCoordinator::GetRawData()");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void AbstractCoordinator::VarInfoAttributeChange()
{
    wxLogMessage("Entering AbstractCoordinator::VarInfoAttributeChange()");
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
    wxLogMessage("Exiting AbstractCoordinator::VarInfoAttributeChange()");
}

void AbstractCoordinator::StandardizeData()
{
    wxLogMessage("Entering AbstractCoordinator::StandardizeData()");
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
    }
    
	for (int t=0; t<data1_vecs.size(); t++) {
		GenUtils::StandardizeData(num_obs, data1_vecs[t], undef_tms[t]);
        if (isBivariate) {
            if (data2_vecs.size() > t)
                GenUtils::StandardizeData(num_obs, data2_vecs[t], undef_tms[t]);
        }
	}
    wxLogMessage("Exiting AbstractCoordinator::StandardizeData()");
}

/** assumes StandardizeData already called on data1 and data2 */
void AbstractCoordinator::CalcLisa()
{
    wxLogMessage("Entering AbstractCoordinator::CalcLisa()");
	for (int t=0; t<num_time_vals; t++) {
		data1 = data1_vecs[t];
		if (isBivariate) {
			data2 = data2_vecs[0];
			if (var_info[1].is_time_variant && var_info[1].sync_with_global_time)
                data2 = data2_vecs[t];
		}
		lags = lags_vecs[t];
		localMoran = local_moran_vecs[t];
		cluster = cluster_vecs[t];
	
		has_isolates[t] = false;
    
        // get undefs of objects/values at this time step
        std::vector<bool> undefs;
        bool has_undef = false;
        for (int i=0; i<undef_data[0][t].size(); i++){
            bool is_undef = undef_data[0][t][i];
            if (isBivariate) {
                if (undef_data[1].size() > t)
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
                localMoran[i] = 0;
                cluster[i] = 6; // undefined value
                continue;
            }
            
			double Wdata = 0;
			if (isBivariate) {
				Wdata = W[i].SpatialLag(data2);
			} else {
				Wdata = W[i].SpatialLag(data1);
			}
			lags[i] = Wdata;
			localMoran[i] = data1[i] * Wdata;
				
			// assign the cluster
			if (W[i].Size() > 0) {
				if (data1[i] > 0 && Wdata < 0) cluster[i] = 4;
				else if (data1[i] < 0 && Wdata > 0) cluster[i] = 3;
				else if (data1[i] < 0 && Wdata < 0) cluster[i] = 2;
				else cluster[i] = 1; //data1[i] > 0 && Wdata > 0
			} else {
				has_isolates[t] = true;
				cluster[i] = 5; // neighborless
			}
		}
	}
    wxLogMessage("Exiting AbstractCoordinator::CalcLisa()");
}

void AbstractCoordinator::CalcPseudoP()
{
	wxLogMessage("Entering AbstractCoordinator::CalcPseudoP()");
	if (!calc_significances)
        return;
    CalcPseudoP_threaded();
	wxLogMessage("Exiting AbstractCoordinator::CalcPseudoP()");
}

void AbstractCoordinator::CalcPseudoP_threaded()
{
	wxLogMessage("Entering AbstractCoordinator::CalcPseudoP_threaded()");
    int nCPUs = GdaConst::gda_cpu_cores;
    if (!GdaConst::gda_set_cpu_cores)
        nCPUs = wxThread::GetCPUCount();

	// mutext protects access to the worker_list
    wxMutex worker_list_mutex;
	// signals that worker_list is empty
	wxCondition worker_list_empty_cond(worker_list_mutex);
    // mutex should be initially locked
	worker_list_mutex.Lock();
	
    // List of all the threads currently alive.  As soon as the thread
	// terminates, it removes itself from the list.
	std::list<wxThread*> worker_list;

	// divide up work according to number of observations
	// and number of CPUs
	int work_chunk = num_obs / nCPUs;
    
    if (work_chunk == 0) {
        work_chunk = 1;
    }
    
	int obs_start = 0;
	int obs_end = obs_start + work_chunk;
	
	bool is_thread_error = false;
	int quotient = num_obs / nCPUs;
	int remainder = num_obs % nCPUs;
	int tot_threads = (quotient > 0) ? nCPUs : remainder;
	
    //boost::thread_group threadPool;
    
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
		uint64_t seed_start = last_seed_used+a;
		uint64_t seed_end = seed_start + ((uint64_t) (b-a));
        int thread_id = i+1;
        //boost::thread* worker = new boost::thread(boost::bind(&AbstractCoordinator::CalcPseudoP_range,this, a, b, seed_start));
        //threadPool.add_thread(worker);
        AbstractWorkerThread* thread =
        new AbstractWorkerThread(a, b, seed_start, this,
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
    //threadPool.join_all();
	wxLogMessage("Exiting AbstractCoordinator::CalcPseudoP_threaded()");
}

void AbstractCoordinator::CalcPseudoP_range(int obs_start, int obs_end, uint64_t seed_start)
{
	GeoDaSet workPermutation(num_obs);
    int max_rand = num_obs-1;
    
	for (int cnt=obs_start; cnt<=obs_end; cnt++) {
        std::vector<uint64_t> countLarger(num_time_vals, 0);
        
        // get full neighbors even if has undefined value
        int numNeighbors = 0;
        for (int t=0; t<num_time_vals; t++) {
            GalElement* w = Gal_vecs[t]->gal;
            if (w[cnt].Size() > numNeighbors)
                numNeighbors = w[cnt].Size();
        }
	
		for (int perm=0; perm<permutations; perm++) {
			int rand=0;
			while (rand < numNeighbors) {
				// computing 'perfect' permutation of given size
                double rng_val = Gda::ThomasWangHashDouble(seed_start++) * max_rand;
                // round is needed to fix issue
                //https://github.com/GeoDaCenter/geoda/issues/488
				int newRandom = (int)(rng_val<0.0?ceil(rng_val - 0.5):floor(rng_val + 0.5));
				if (newRandom != cnt && !workPermutation.Belongs(newRandom)) {
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
                double permutedLag=0;
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
        for (int t=0; t<num_time_vals; t++) {
            double* _sigLocalMoran = sig_local_moran_vecs[t];
            int* _sigCat = sig_cat_vecs[t];

    		// pick the smallest
    		if (permutations-countLarger[t] <= countLarger[t]) {
    			countLarger[t] = permutations-countLarger[t];
    		}
    		
    		_sigLocalMoran[cnt] = (countLarger[t]+1.0)/(permutations+1);
    		// 'significance' of local Moran
    		if (_sigLocalMoran[cnt] <= 0.0001) _sigCat[cnt] = 4;
    		else if (_sigLocalMoran[cnt] <= 0.001) _sigCat[cnt] = 3;
    		else if (_sigLocalMoran[cnt] <= 0.01) _sigCat[cnt] = 2;
    		else if (_sigLocalMoran[cnt] <= 0.05) _sigCat[cnt]= 1;
    		else _sigCat[cnt]= 0;
    		
    		// observations with no neighbors get marked as isolates
            // NOTE: undefined should be marked as well, however, since undefined_cat has covered undefined category, we don't need to handle here
    		if (numNeighbors == 0) {
    			_sigCat[cnt] = 5;
    		}
        }
	}
}

void AbstractCoordinator::SetSignificanceFilter(int filter_id)
{
	wxLogMessage("Entering AbstractCoordinator::SetSignificanceFilter()");
    if (filter_id == -1) {
        // user input cutoff
        significance_filter = filter_id;
        return;
    }
	// 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	if (filter_id < 1 || filter_id > 4) return;
	significance_filter = filter_id;
	if (filter_id == 1) significance_cutoff = 0.05;
	if (filter_id == 2) significance_cutoff = 0.01;
	if (filter_id == 3) significance_cutoff = 0.001;
	if (filter_id == 4) significance_cutoff = 0.0001;
	wxLogMessage("Exiting AbstractCoordinator::SetSignificanceFilter()");
}

void AbstractCoordinator::update(WeightsManState* o)
{
	wxLogMessage("In AbstractCoordinator::update()");
    if (w_man_int) {
        weight_name = w_man_int->GetLongDispName(w_id);
    }
}

int AbstractCoordinator::numMustCloseToRemove(boost::uuids::uuid id) const
{
	wxLogMessage("In AbstractCoordinator::numMustCloseToRemove()");
	return id == w_id ? observers.size() : 0;
}

void AbstractCoordinator::closeObserver(boost::uuids::uuid id)
{
	wxLogMessage("In AbstractCoordinator::closeObserver()");
	if (numMustCloseToRemove(id) == 0) return;
	std::list<AbstractCoordinatorObserver*> obs_cpy = observers;
	for (std::list<AbstractCoordinatorObserver*>::iterator i=obs_cpy.begin();
		 i != obs_cpy.end(); ++i) {
		(*i)->closeObserver(this);
	}
}

void AbstractCoordinator::registerObserver(AbstractCoordinatorObserver* o)
{
	wxLogMessage("In AbstractCoordinator::registerObserver()");
	observers.push_front(o);
}

void AbstractCoordinator::removeObserver(AbstractCoordinatorObserver* o)
{
	wxLogMessage("Entering AbstractCoordinator::removeObserver");
	observers.remove(o);
	LOG(observers.size());
	if (observers.size() == 0) {
		delete this;
	}
	wxLogMessage("Exiting AbstractCoordinator::removeObserver");
}

void AbstractCoordinator::notifyObservers()
{
	wxLogMessage("In AbstractCoordinator::notifyObservers");
	for (std::list<AbstractCoordinatorObserver*>::iterator  it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

