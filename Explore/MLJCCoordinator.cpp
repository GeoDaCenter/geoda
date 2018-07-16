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
JCWorkerThread::JCWorkerThread(const GalElement* W_, const std::vector<bool>& undefs_, int obs_start_s, int obs_end_s, uint64_t seed_start_s, JCCoordinator* jc_coord_s, wxMutex* worker_list_mutex_s, wxCondition* worker_list_empty_cond_s, std::list<wxThread*> *worker_list_s,int thread_id_s)
: wxThread(),
W(W_),
undefs(undefs_),
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
	jc_coord->CalcPseudoP_range(W, undefs, obs_start, obs_end, seed_start);
	
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
data_undef(var_info_s.size()),
last_seed_used(123456789), reuse_last_seed(true)
{
    reuse_last_seed = GdaConst::use_gda_user_seed;
    if ( GdaConst::use_gda_user_seed) {
        last_seed_used = GdaConst::gda_user_seed;
    }
	TableInterface* table_int = project->GetTableInt();
	for (int i=0; i<var_info.size(); i++) {
		table_int->GetColData(col_ids[i], data[i]);
        table_int->GetColUndefined(col_ids[i], data_undef[i]);
	}
    
	weight_name = w_man_int->GetLongDispName(w_id);
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
	for (int i=0; i<G_vecs.size(); i++) if (G_vecs[i]) delete[] G_vecs[i];
	G_vecs.clear();

	for (int i=0; i<pseudo_p_vecs.size(); i++) {
		if (pseudo_p_vecs[i]) delete [] pseudo_p_vecs[i];
	}
	pseudo_p_vecs.clear();
	
	for (int i=0; i<x_vecs.size(); i++) if (x_vecs[i]) delete[] x_vecs[i];
	x_vecs.clear();
    
	for (int i=0; i<y_vecs.size(); i++) if (y_vecs[i]) delete[] y_vecs[i];
	y_vecs.clear();
    
	for (int i=0; i<c_vecs.size(); i++) if (c_vecs[i]) delete[] c_vecs[i];
	c_vecs.clear();
   
    num_neighbors.clear();
	num_neighbors_x1.clear();
	num_neighbors_y1.clear();
	num_neighbors_xy1.clear();
    
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
    
	G_vecs.resize(tms);
	pseudo_p_vecs.resize(tms);
	x_vecs.resize(tms);
	y_vecs.resize(tms);
    c_vecs.resize(tms);
    x_undefs.resize(tms);
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);
    num_neighbors.resize(tms);
    num_neighbors_x1.resize(tms);
    num_neighbors_y1.resize(tms);
    num_neighbors_xy1.resize(tms);
    
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    
	for (int i=0; i<tms; i++) {
		G_vecs[i] = new double[num_obs];
		pseudo_p_vecs[i] = new double[num_obs];
		x_vecs[i] = new double[num_obs];
		y_vecs[i] = new double[num_obs];
        c_vecs[i] = new int[num_obs];
        
        num_neighbors[i].resize(num_obs);
        num_neighbors_x1[i].resize(num_obs);
        num_neighbors_y1[i].resize(num_obs);
        num_neighbors_xy1[i].resize(num_obs);
        
        for (int j=0; j<num_obs;j++){
            x_undefs[i][j] = false;
            x_vecs[i][j] = 0;
            y_vecs[i][j] = 0;
            c_vecs[i][j] = 0;
            num_neighbors[i][j] = 0;
            num_neighbors_x1[i][j] = 0;
            num_neighbors_y1[i][j] = 0;
            num_neighbors_xy1[i][j] = 0;
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
    
	AllocateVectors();
	
    int x_idx = var_info[0].time - var_info[0].time_min;
    int y_idx = var_info[1].time - var_info[1].time_min;
    
	for (int t=0; t<num_time_vals; t++) {
        bool has_undef = false;
        for (int i=0; i<num_obs; i++) {
            x_vecs[t][i] = data[0][x_idx][i];
            y_vecs[t][i] = data[1][y_idx][i];
            
            bool is_undef = data_undef[0][x_idx][i] || data_undef[1][y_idx][i];
            x_undefs[t][i] = is_undef;
            if (is_undef) has_undef = true;
        }
        has_undefined[t] = has_undef;
	}
    
	for (int t=0; t<num_time_vals; t++) {
        GalElement* W  = NULL;
        if (Gal_vecs.empty() || Gal_vecs[t] == NULL) {
            // local weights copy: no leak see deconstruction function
            GalWeight* gw = NULL;
            if ( has_undefined[t] ) {
                gw = new GalWeight(*w_man_int->GetGal(w_id));
                gw->Update(x_undefs[t]);
            } else {
                gw = w_man_int->GetGal(w_id);
            }
            W = gw->gal ;
            Gal_vecs[t] = gw;
            Gal_vecs_orig[t] = w_man_int->GetGal(w_id);
        }
	}
	
	CalcGs();
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
	double* p_val = pseudo_p_vecs[t];
    const GalElement* W = Gal_vecs[t]->gal;
	c_val.resize(num_obs);

    
	for (int i=0; i<num_obs; i++) {
        if (x_undefs[t][i]) {
            c_val[i] = 5; // undefined
            
        } else if (W[i].Size() == 0) {
			c_val[i] = 4; // isolate
            
		} else if (p_val[i] <= significance_cutoff) {
            //c_val[i] = c_vecs[t][i]; // 1,2,3
            if (G_vecs[t][i] == 0) {
                c_val[i] = 0;
            } else {
                c_val[i] = 1;
            }

        } else {
			c_val[i] = 0; // not significant
		}
	}
}

void JCCoordinator::CalcGs()
{
	for (int t=0; t<num_time_vals; t++) {
		G = G_vecs[t];
		pseudo_p = pseudo_p_vecs[t];
		x = x_vecs[t];
		y = y_vecs[t];
	
        const GalElement* W = Gal_vecs[t]->gal;

        int all_y_1 = 0;
        int all_x_y_1 = 0;
        for (long i=0; i<num_obs; i++) {
            if (y[i] == 1) all_y_1++;
            if (y[i] == 1 && x[i] == 1) all_x_y_1++;
        }
        
		for (long i=0; i<num_obs; i++) {
            if (x_undefs[t][i]) continue;
            // NOTE: the roles of x and z can be reversed
            // 1. no colocation: x_i == 1 always z_i == 0 (vice versa)
            // JC_i = x_i * ( 1 - z_i) * Sum (w_ij * z_j)
            // 2. has colocation: x_i == 1 && z_i == 1
            // JC_i = x_i * z_i * Sum(w_ij * z_j)
            // 3. co-location cluster: (2) && x_j == z_j == 1 for the neighbors
            // JC_i = x_i * z_i * Sum(w_ij * x_j * z_j)
            int jc_type_i = 0;
            G[i] = 0;
            pseudo_p[i] = 0;
            if (x[i] == 1) {
                if (y[i] == 0)
                    jc_type_i = 1;
                else if (y[i] == 1)
                    jc_type_i = 3; // 2 or 3 will determined later
            }
           
            num_neighbors[t][i] = W[i].Size();
            
            int x_1_y_1 = 0;
            int y_1 = 0;
            
            if (W[i].Size() == 0) {
                has_isolates[t] = true;
            } else if (x[i] == 1) {
				double lag = 0;
				bool self_neighbor = false;
				for (size_t j=0, sz=W[i].Size(); j<sz; j++) {
                    int n_id = W[i][j];
					if (W[i][j] != i) {
                        if (jc_type_i == 3 && x[n_id] != y[n_id]) {
                            jc_type_i = 2;
                        }
					} else {
						self_neighbor = true;
					}
                    if (x[n_id]==1) num_neighbors_x1[t][i]++;
                    if (y[n_id]==1) {
                        y_1 += 0;
                        num_neighbors_y1[t][i]++;
                    }
                    if (x[n_id]==1&&y[n_id]==1) {
                        x_1_y_1 += 1;
                        num_neighbors_xy1[t][i]++;
                    }
				}
                
                for (size_t j=0, sz=W[i].Size(); j<sz; j++) {
                    int n_id = W[i][j];
                    if (jc_type_i == 3 ){
                        if (x[n_id] == y[n_id]) lag += y[n_id];
                    } else {
                        if (y[n_id] == 1) lag += y[n_id];
                    }
                }
                G[i] = lag;
			}
            c_vecs[t][i] = jc_type_i;
		}
	}
}

void JCCoordinator::CalcPseudoP()
{
	LOG_MSG("Entering JCCoordinator::CalcPseudoP");
	wxStopWatch sw;
	int nCPUs = wxThread::GetCPUCount();
	
	// To ensure thread safety, only work on one time slice of data
	// at a time.  For each time period t:
	// 1. copy data for time period t into data1 and data2 arrays
	// 2. Perform multi-threaded computation
	// 3. copy results into results array
	
	for (int t=0; t<num_time_vals; t++) {
		G = G_vecs[t];
        x = x_vecs[t];
        y = y_vecs[t];
        c = c_vecs[t];
		pseudo_p = pseudo_p_vecs[t];
        CalcPseudoP_threaded(Gal_vecs[t]->gal, x_undefs[t]);
	}
	LOG_MSG("Exiting JCCoordinator::CalcPseudoP");
}

void JCCoordinator::CalcPseudoP_threaded(const GalElement* W, const std::vector<bool>& undefs)
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
			new JCWorkerThread(W, undefs, a, b, seed_start, this,
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
		CalcPseudoP_range(W, undefs, 0, num_obs-1, last_seed_used);
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
void JCCoordinator::CalcPseudoP_range(const GalElement* W, const std::vector<bool>& undefs, int obs_start, int obs_end, uint64_t seed_start)
{
	GeoDaSet workPermutation(num_obs);
    
	int max_rand = num_obs-1;
    
	for (long i=obs_start; i<=obs_end; i++) {
        if (undefs[i]) continue;

		const int numNeighsI = W[i].Size();
        
        //only compute for non-isolates
		if (numNeighsI > 0) {
			int countGLarger = 0;
			double permutedG = 0;
            
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
				
				double perm_jc_i = 0;
				// use permutation to compute the lags
				for (int j=0; j<numNeighsI; j++) {
                    int perm_idx = workPermutation.Pop();
                    if (c[i]== 1 || c[i]== 2) {
                        perm_jc_i += y[perm_idx];
                        
                    } else if (c[i]== 3) {
                        perm_jc_i  += x[perm_idx] * y[perm_idx];
                    }
				}
		
                // binary weights
                permutedG = perm_jc_i;
				if (permutedG >= G[i]) countGLarger++;
			}
			// pick the smallest
			if (permutations-countGLarger < countGLarger) { 
				countGLarger=permutations-countGLarger;
			}
			pseudo_p[i] = (countGLarger + 1.0)/(permutations+1.0);
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

