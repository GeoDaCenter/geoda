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
#include <boost/math/distributions/normal.hpp> // for normal_distribution
#include <algorithm>
#include <functional>
#include <map>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include <wx/msgdlg.h>

#include "../Algorithms/gpu_lisa.h"
#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/Randik.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../logger.h"
#include "../Project.h"
#include "MLJCCoordinatorObserver.h"
#include "MLJCCoordinator.h"

///////////////////////////////////////////////////////////////////////////////
//
// JCWorkerThread
//
///////////////////////////////////////////////////////////////////////////////
JCWorkerThread::JCWorkerThread(int t_, int obs_start_s, int obs_end_s, uint64_t seed_start_s, JCCoordinator* jc_coord_s, wxMutex* worker_list_mutex_s, wxCondition* worker_list_empty_cond_s, std::list<wxThread*> *worker_list_s,int thread_id_s)
: wxThread(),
t(t_),
obs_start(obs_start_s), obs_end(obs_end_s), seed_start(seed_start_s),
jc_coord(jc_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

JCWorkerThread::~JCWorkerThread()
{
}

wxThread::ExitCode JCWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("JCWorkerThread %d started", thread_id));
	// call work for assigned range of observations
	jc_coord->CalcPseudoP_range(t, obs_start, obs_end, seed_start);
	
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


///////////////////////////////////////////////////////////////////////////////
//
// JCCoordinator
//
///////////////////////////////////////////////////////////////////////////////
JCCoordinator::JCCoordinator(boost::uuids::uuid weights_id, Project* project, const std::vector<GdaVarTools::VarInfo>& var_info_s, const std::vector<int>& col_ids)
: w_man_state(project->GetWManState()),
w_man_int(project->GetWManInt()),
w_id(weights_id),
num_obs(project->GetNumRecords()),
permutations(999),
var_info(var_info_s),
data(var_info_s.size()),
undef_data(var_info_s.size()),
last_seed_used(123456789), reuse_last_seed(true)
{
    reuse_last_seed = GdaConst::use_gda_user_seed;
    if ( GdaConst::use_gda_user_seed) {
        last_seed_used = GdaConst::gda_user_seed;
    }
	TableInterface* table_int = project->GetTableInt();
	for (int i=0; i<var_info.size(); i++) {
		table_int->GetColData(col_ids[i], data[i]);
        table_int->GetColUndefined(col_ids[i], undef_data[i]);
	}
    num_vars = var_info.size();
	weight_name = w_man_int->GetLongDispName(w_id);
    weights = w_man_int->GetGal(w_id);
	SetSignificanceFilter(1);
    
	InitFromVarInfo();
	
	w_man_state->registerObserver(this);
}

JCCoordinator::~JCCoordinator()
{
    if (w_man_state) {
        w_man_state->removeObserver(this);
        w_man_state = NULL;
    }
	DeallocateVectors();
}

void JCCoordinator::DeallocateVectors()
{
    for (int i=0; i<zz_vecs.size(); i++) {
        if (zz_vecs[i]) delete [] zz_vecs[i];
    }
    zz_vecs.clear();
    
    for (int i=0; i<local_jc_vecs.size(); i++) {
        if (local_jc_vecs[i]) delete [] local_jc_vecs[i];
    }
    local_jc_vecs.clear();
    
    for (int i=0; i<sig_local_jc_vecs.size(); i++) {
        if (sig_local_jc_vecs[i]) delete [] sig_local_jc_vecs[i];
    }
    sig_local_jc_vecs.clear();
    
    for (int i=0; i<data_vecs.size(); i++) {
        for (int j=0; j<data_vecs[i].size(); j++) {
            if (data_vecs[i][j]) delete [] data_vecs[i][j];
        }
        data_vecs[i].clear();
    }
    data_vecs.clear();
    
    num_neighbors.clear();
   
    // clear W_vecs
    for (size_t i=0; i<has_undefined.size(); i++) {
        if (has_undefined[i])  delete Gal_vecs[i];
    }
    Gal_vecs.clear();
    Gal_vecs_orig.clear();
}

/** allocate based on var_info and num_time_vals **/
void JCCoordinator::AllocateVectors()
{
	int tms = num_time_vals;
    
    zz_vecs.resize(tms);
    undef_tms.resize(tms);
    local_jc_vecs.resize(tms);
    sig_local_jc_vecs.resize(tms);
    num_neighbors.resize(tms);

    data_vecs.resize(num_vars);
    for (int i=0; i<num_vars; i++) {
        int tms_at_var = data[i].size();
        data_vecs[i].resize(tms_at_var);
        for (int j=0; j<tms_at_var; j++) {
            data_vecs[i][j] = new double[num_obs];
        }
    }
    
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);

	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    
	for (int i=0; i<tms; i++) {
        undef_tms[i].resize(num_obs, false);
        zz_vecs[i] = new int[num_obs];
        local_jc_vecs[i] = new double[num_obs];
        sig_local_jc_vecs[i] = new double[num_obs];
        num_neighbors[i].resize(num_obs);

        for (int j=0; j<num_obs;j++){
            zz_vecs[i][j] = 1;
            num_neighbors[i][j] = 0;
            local_jc_vecs[i][j] = 0;
            sig_local_jc_vecs[i][j] = 0;
        }
        
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
        has_isolates[i] = false;
        has_undefined[i] = false;
       
        Gal_vecs[i] = 0;
        Gal_vecs_orig[i] = 0;
	}
}

void JCCoordinator::InitFromVarInfo()
{
	DeallocateVectors();
	
	num_time_vals = 1; // for multivariate, time variable is not supported
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	ref_var_index = -1;
    
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
    
	AllocateVectors();
	
    int num_var = data.size();
    for (int i=0; i<num_var; i++) {
        int tms_at_var = data[i].size();
        for (int j=0; j<tms_at_var; j++) {
            for (int k=0; k<num_obs; k++) {
                data_vecs[i][j][k] = data[i][j][k];
            }
        }
    }
    
    //StandardizeData();
    GalElement* w = weights->gal;
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
	
    CalcMultiLocalJoinCount();
    
	CalcPseudoP();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void JCCoordinator::VarInfoAttributeChange()
{
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	ref_var_index = -1;
	num_time_vals = 1;
}

/** The category vector c_val will be filled based on the current
 significance filter and significance values corresponding to specified
 canvas_time.  */
void JCCoordinator::FillClusterCats(int canvas_time, std::vector<wxInt64>& c_val)
{
	int t = 0;
	double* p_val = sig_local_jc_vecs[t];
    const GalElement* W = Gal_vecs[t]->gal;
	c_val.resize(num_obs);

    
	for (int i=0; i<num_obs; i++) {
        if (undef_tms[t][i]) {
            c_val[i] = 5; // undefined
            
        } else if (W[i].Size() == 0) {
			c_val[i] = 4; // isolate
            
		} else if (p_val[i] <= significance_cutoff) {
            //c_val[i] = c_vecs[t][i]; // 1,2,3
            if (local_jc_vecs[t][i] == 0) {
                c_val[i] = 0; // not significant
            } else {
                c_val[i] = 1;
            }

        } else {
			c_val[i] = 0; // not significant
		}
	}
}

void JCCoordinator::CalcMultiLocalJoinCount()
{
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

        for (int i=0; i<num_obs; i++) {
            if (W[i].Size() == 0) {
                has_isolates[t] = true;
                break;
            }
        }

        for (int i=0; i<num_obs; i++) {
            int nn = W[i].Size();
            if (W[i].Check(i)) {
                nn -= 1; // self-neighbor
            }
            num_neighbors[t][i] = nn;
        }
        
        // local join count
        double* local_jc = local_jc_vecs[t];
        
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
       
        int* zz = zz_vecs[t];
		for (int i=0; i<num_obs; i++) {
            if (undefs[i] == true) {
                zz[i] = 0;
                continue;
            }
            for (int v=0; v<num_vars; v++) {
                int _t = local_t[v];
                int _v = data_vecs[v][_t][i];
                zz[i] = zz[i] * _v;
			}
		}
        
        // univariate local join
        if (num_vars == 1) {
            for (int i=0; i<num_obs; i++) {
                if (zz[i]>0) { // x_j = 1
                    for (int j=0, sz=W[i].Size(); j<sz; j++) {
                        int n_id = W[i][j];
                        if (n_id != i) {
                            local_jc[i] += zz[n_id];
                        }
                    }
                }
            }
            return;
        }
        
        // bivariate local join count -- colocation and no-colocation
        // multivariate local join count -- colocation only
        int sum = 0;
        for (int i=0; i<num_obs; i++) {
            sum += zz[i];
        }
        bool nocolocation = sum == 0;
        if (nocolocation) {
            // here only bivariate apply to no-colocation case
            for (int i=0; i<num_obs; i++) {
                if (undefs[i] == true) {
                    zz[i] = 0;
                    continue;
                }
                int _t = local_t[1];
                zz[i] = data_vecs[1][_t][i];
            }
            for (int i=0; i<num_obs; i++) {
                int _t = local_t[0];
                if (data_vecs[0][_t][i]>0) { // x_i.z_i = 1
                    for (int j=0, sz=W[i].Size(); j<sz; j++) {
                        // compute the number of neighbors with
                        // x_j.z_j = 1 (zz=1) as a spatial lag
                        int n_id = W[i][j];
                        if (n_id != i) {
                            local_jc[i] += zz[n_id];
                        }
                    }
                }
            }
            
        } else {
            for (int i=0; i<num_obs; i++) {
                if (zz[i]>0) { // x_i.z_i = 1
                    for (int j=0, sz=W[i].Size(); j<sz; j++) {
                        // compute the number of neighbors with
                        // x_j.z_j = 1 (zz=1) as a spatial lag
                        int n_id = W[i][j];
                        if (n_id != i) {
                            local_jc[i] += zz[n_id];
                        }
                    }
                }
            }
        }
        
	}
}

void JCCoordinator::CalcPseudoP()
{
	LOG_MSG("Entering JCCoordinator::CalcPseudoP");
	wxStopWatch sw_vd;
    
    if (GdaConst::gda_use_gpu == false) {
        for (int t=0; t<num_time_vals; t++) {
            CalcPseudoP_threaded(t);
        }
    } else {
        for (int t=0; t<num_time_vals; t++) {
            vector<int> local_t;
            for (int v=0; v<num_vars; v++) {
                if (data_vecs[v].size()==1) {
                    local_t.push_back(0);
                } else {
                    local_t.push_back(t);
                }
            }
            
            int* zz = zz_vecs[t];
            
            double* values = new double[num_vars * num_obs];
            for (int v=0; v<num_vars; v++) {
                for (int j=0; j<num_obs; j++) {
                    int _t = local_t[v];
                    values[v*num_obs + j] = data_vecs[v][_t][j];
                }
            }
            
            double* local_jc = local_jc_vecs[t];
            GalElement* w = Gal_vecs[t]->gal;
            double* _sigLocal = sig_local_jc_vecs[t];
            
            wxString exePath = GenUtils::GetExeDir();
            wxString clPath = exePath + "localjc_kernel.cl";
            bool flag = gpu_localjoincount(clPath.mb_str(), num_obs, permutations, last_seed_used, num_vars, zz, local_jc, w, _sigLocal);
            
            delete[] values;
            
            if (!flag) {
                wxMessageDialog dlg(NULL, "GeoDa can't configure GPU device. Default CPU solution will be used instead.", _("Error"), wxOK | wxICON_ERROR);
                dlg.ShowModal();
                CalcPseudoP_threaded(t);
            }
        }
    }
    LOG_MSG(wxString::Format("JCCoordinator::GPU took %ld ms", sw_vd.Time()));
}

void JCCoordinator::CalcPseudoP_threaded(int t)
{
	LOG_MSG("Entering JCCoordinator::CalcPseudoP_threaded");
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
	std::list<wxThread*> worker_list;
	
	// divide up work according to number of observations
	// and number of CPUs
	int work_chunk = num_obs / nCPUs;
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
		uint64_t seed_start = last_seed_used+a;
		uint64_t seed_end = seed_start + ((uint64_t) (b-a));
		int thread_id = i+1;
		
		JCWorkerThread* thread =
			new JCWorkerThread(t, a, b, seed_start, this,
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
		CalcPseudoP_range(t, 0, num_obs-1, last_seed_used);
	} else {
		std::list<wxThread*>::iterator it;
		for (it = worker_list.begin(); it != worker_list.end(); it++) {
			(*it)->Run();
		}
		
		while (!worker_list.empty()) {
			// wait until thread_list might be empty
			worker_list_empty_cond.Wait();
			// We have been woken up. If this was not a false
			// alarm (spurious signal), the loop will exit.
		}
	}
	LOG_MSG("Exiting JCCoordinator::CalcPseudoP_threaded");
}

/** In the code that computes Gi and Gi*, we specifically checked for 
 self-neighbors and handled the situation appropriately.  For the
 permutation code, we will disallow self-neighbors. */
void JCCoordinator::CalcPseudoP_range(int t, int obs_start, int obs_end, uint64_t seed_start)
{
	GeoDaSet workPermutation(num_obs);
    
	int max_rand = num_obs-1;
    
    GalElement* W = Gal_vecs[t]->gal;
    int* zz = zz_vecs[t];
    double* local_jc = local_jc_vecs[t];
    std::vector<bool>& undefs = undef_tms[t];
    double* pseudo_p = sig_local_jc_vecs[t];
    
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
    
    for (long i=obs_start; i<=obs_end; i++) {
        if (undefs[i]) continue;

        if (local_jc[i] ==0) {
            pseudo_p[i] = 0;
            continue;
        }
        int numNeighsI = W[i].Size();
        if (W[i].Check(i)) {
            numNeighsI -= 1; // self-neighbor
        }
        //only compute for non-isolates
		if (numNeighsI > 0) {
			int countLarger = 0;
			double permuted = 0;
            
			for (int perm=0; perm < permutations; perm++) {
				int rand = 0;
				while (rand < numNeighsI) {
					// computing 'perfect' permutation of given size
                    double rng_val = Gda::ThomasWangHashDouble(seed_start++) * max_rand;
                    // round is needed to fix issue
                    //https://github.com/GeoDaCenter/geoda/issues/488
                    int newRandom = (int) (rng_val < 0.0 ? ceil(rng_val - 0.5) : floor(rng_val + 0.5));
                    
					if (newRandom != i &&
                        !workPermutation.Belongs(newRandom) &&
                        undefs[newRandom] == false)
					{
						workPermutation.Push(newRandom);
						rand++;
					}
				}
				
				double perm_jc = 0;
				// use permutation to compute the lags
				for (int j=0; j<numNeighsI; j++) {
                    int perm_idx = workPermutation.Pop();
                    perm_jc += zz[perm_idx];
				}
		
                // binary weights
                permuted = perm_jc;
				if (permuted >= local_jc[i]) countLarger++;
			}
			// pick the smallest
			if (permutations-countLarger < countLarger) {
				countLarger=permutations - countLarger;
			}
			pseudo_p[i] = (countLarger + 1.0)/(permutations+1.0);
		}
	}
}

void JCCoordinator::SetSignificanceFilter(int filter_id)
{
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
}

void JCCoordinator::update(WeightsManState* o)
{
	weight_name = w_man_int->GetLongDispName(w_id);
}

int JCCoordinator::numMustCloseToRemove(boost::uuids::uuid id) const
{
	return id == w_id ? observers.size() : 0;
}

void JCCoordinator::closeObserver(boost::uuids::uuid id)
{
	if (numMustCloseToRemove(id) == 0) return;
	std::list<JCCoordinatorObserver*> observers_cpy = observers;
	for (std::list<JCCoordinatorObserver*>::iterator i=observers_cpy.begin();
		 i != observers_cpy.end(); ++i) {
		if (*i != 0) {
			(*i)->closeObserver(this);
		}
	}
}

void JCCoordinator::registerObserver(JCCoordinatorObserver* o)
{
	observers.push_front(o);
}

void JCCoordinator::removeObserver(JCCoordinatorObserver* o)
{
    observers.remove(o);
    if (observers.size() == 0) {
        delete this;
    }
}

void JCCoordinator::notifyObservers()
{
    for (std::list<JCCoordinatorObserver*>::iterator  it=observers.begin();
         it != observers.end(); ++it) {
        (*it)->update(this);
    }
}

