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
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include "../DataViewer/TableInterface.h"
#include "../ShapeOperations/Randik.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../logger.h"
#include "../Project.h"
#include "GetisOrdMapNewView.h"
#include "GStatCoordinator.h"

GStatWorkerThread::GStatWorkerThread(int obs_start_s, int obs_end_s,
									 uint64_t seed_start_s,
									 GStatCoordinator* gstat_coord_s,
									 wxMutex* worker_list_mutex_s,
									 wxCondition* worker_list_empty_cond_s,
									 std::list<wxThread*> *worker_list_s,
									 int thread_id_s)
: wxThread(),
obs_start(obs_start_s), obs_end(obs_end_s), seed_start(seed_start_s),
gstat_coord(gstat_coord_s),
worker_list_mutex(worker_list_mutex_s),
worker_list_empty_cond(worker_list_empty_cond_s),
worker_list(worker_list_s),
thread_id(thread_id_s)
{
}

GStatWorkerThread::~GStatWorkerThread()
{
}

wxThread::ExitCode GStatWorkerThread::Entry()
{
	LOG_MSG(wxString::Format("GStatWorkerThread %d started", thread_id));
	
	// call work for assigned range of observations
	gstat_coord->CalcPseudoP_range(obs_start, obs_end, seed_start);
	
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


GStatCoordinator::
GStatCoordinator(boost::uuids::uuid weights_id,
                 Project* project,
                 const std::vector<GdaVarTools::VarInfo>& var_info_s,
                 const std::vector<int>& col_ids,
                 bool row_standardize_weights,
                 bool _is_local_joint_count)
: w_man_state(project->GetWManState()),
w_man_int(project->GetWManInt()),
w_id(weights_id),
num_obs(project->GetNumRecords()),
row_standardize(row_standardize_weights),
permutations(999),
var_info(var_info_s),
data(var_info_s.size()),
data_undef(var_info_s.size()),
last_seed_used(123456789), reuse_last_seed(true),
is_local_joint_count(_is_local_joint_count)
{
    wxLogMessage("Entering GStatCoordinator::GStatCoordinator().");
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
	
	maps.resize(8);
	for (int i=0, iend=maps.size(); i<iend; i++) {
		maps[i] = (GetisOrdMapFrame*) 0;
	}
	w_man_state->registerObserver(this);
    wxLogMessage("Exiting GStatCoordinator::GStatCoordinator().");
}

GStatCoordinator::~GStatCoordinator()
{
	wxLogMessage("In GStatCoordinator::~GStatCoordinator");
	w_man_state->removeObserver(this);
	DeallocateVectors();
}

void GStatCoordinator::DeallocateVectors()
{
	wxLogMessage("In GStatCoordinator::DeallocateVectors");
	for (int i=0; i<G_vecs.size(); i++) if (G_vecs[i]) delete [] G_vecs[i];
	G_vecs.clear();

	for (int i=0; i<G_defined_vecs.size(); i++) {
		if (G_defined_vecs[i]) delete [] G_defined_vecs[i];
	}
	G_defined_vecs.clear();

	for (int i=0; i<G_star_vecs.size(); i++) {
		if (G_star_vecs[i]) delete [] G_star_vecs[i];
	}
	G_star_vecs.clear();

	for (int i=0; i<z_vecs.size(); i++) if (z_vecs[i]) delete [] z_vecs[i];
	z_vecs.clear();

	for (int i=0; i<p_vecs.size(); i++) if (p_vecs[i]) delete [] p_vecs[i];
	p_vecs.clear();
	
	for (int i=0; i<z_star_vecs.size(); i++) {
		if (z_star_vecs[i]) delete [] z_star_vecs[i];
	}
	z_star_vecs.clear();
	
	for (int i=0; i<p_star_vecs.size(); i++) {
		if (p_star_vecs[i]) delete [] p_star_vecs[i];
	}
	p_star_vecs.clear();
	
	for (int i=0; i<pseudo_p_vecs.size(); i++) {
		if (pseudo_p_vecs[i]) delete [] pseudo_p_vecs[i];
	}
	pseudo_p_vecs.clear();
	
	for (int i=0; i<pseudo_p_star_vecs.size(); i++) {
		if (pseudo_p_star_vecs[i]) delete [] pseudo_p_star_vecs[i];
	}
	pseudo_p_star_vecs.clear();
	
	for (int i=0; i<x_vecs.size(); i++) if (x_vecs[i]) delete [] x_vecs[i];
	x_vecs.clear();
    
	for (int i=0; i<ep_vals.size(); i++) if (ep_vals[i]) delete [] ep_vals[i];
	ep_vals.clear();
    
	for (int i=0; i<num_neighbors_1.size(); i++) if (num_neighbors_1[i]) delete [] num_neighbors_1[i];
	num_neighbors_1.clear();
    
    // clear W_vecs
    for (size_t i=0; i<has_undefined.size(); i++) {
        if (has_undefined[i]) {
            delete Gal_vecs[i];
        }
    }
    Gal_vecs.clear();
    x_undefs.clear();
    Gal_vecs_orig.clear();
	wxLogMessage("Out GStatCoordinator::DeallocateVectors");
}

/** allocate based on var_info and num_time_vals **/
void GStatCoordinator::AllocateVectors()
{
	wxLogMessage("In GStatCoordinator::AllocateVectors");
	int tms = num_time_vals;
	G_vecs.resize(tms);
	G_defined_vecs.resize(tms);
	G_star_vecs.resize(tms);
	z_vecs.resize(tms);
	p_vecs.resize(tms);
	z_star_vecs.resize(tms);
	p_star_vecs.resize(tms);
	pseudo_p_vecs.resize(tms);
	pseudo_p_star_vecs.resize(tms);
	x_vecs.resize(tms);
    
    x_undefs.resize(tms);
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);
    
	
	n.resize(tms, 0);
	x_star.resize(tms, 0);
	x_sstar.resize(tms, 0);
	ExG.resize(tms);
	ExGstar.resize(tms);
	mean_x.resize(tms);
	var_x.resize(tms);
	VarGstar.resize(tms);
	sdGstar.resize(tms);
	
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    
    num_neighbors.resize(num_obs);
    num_neighbors_1.resize(tms);
    ep_vals.resize(tms);
    
	for (int i=0; i<tms; i++) {
        num_neighbors_1[i] = new wxInt64[num_obs];
        ep_vals[i] = new double[num_obs];
		G_vecs[i] = new double[num_obs];
		G_defined_vecs[i] = new bool[num_obs];
		for (int j=0; j<num_obs; j++) G_defined_vecs[i][j] = true;
		G_star_vecs[i] = new double[num_obs];
		z_vecs[i] = new double[num_obs];
		p_vecs[i] = new double[num_obs];
		z_star_vecs[i] = new double[num_obs];
		p_star_vecs[i] = new double[num_obs];
		pseudo_p_vecs[i] = new double[num_obs];
		pseudo_p_star_vecs[i] = new double[num_obs];
		x_vecs[i] = new double[num_obs];
		
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
        
        Gal_vecs[i] = NULL;
	}
	wxLogMessage("Out GStatCoordinator::AllocateVectors");
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void GStatCoordinator::InitFromVarInfo()
{
	wxLogMessage("In GStatCoordinator::InitFromVarInfo");
	DeallocateVectors();
	
	num_time_vals = 1;
	is_any_time_variant = var_info[0].is_time_variant;
	is_any_sync_with_global_time = false;
	ref_var_index = -1;
	if (var_info[0].is_time_variant &&
		var_info[0].sync_with_global_time) {
		num_time_vals = (var_info[0].time_max - var_info[0].time_min) + 1;
		is_any_sync_with_global_time = true;
		ref_var_index = 0;
	}

	AllocateVectors();
	
    bool has_undef = false;
    
	for (int t=var_info[0].time_min; t<=var_info[0].time_max; t++) {
		int d_t = t - var_info[0].time_min;
        vector<bool> undefs(num_obs);
        for (int i=0; i<num_obs; i++) {
            x_vecs[d_t][i] = data[0][t][i];
            undefs[i] = data_undef[0][t][i];
            G_defined_vecs[d_t][i] = !undefs[i];
            if (undefs[i]) has_undef = true;
        }
        x_undefs[d_t] = undefs;
        has_undefined[d_t] = has_undef;
	}
    
	for (int t=0; t<num_time_vals; t++) {
        GalElement* W  = NULL;
        if (!Gal_vecs.empty() && Gal_vecs[t] == NULL) {
            // local weights copy
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
		x = x_vecs[t];
        if (is_local_joint_count) {
            int num_obs_1s = 0;
            int num_obs_0s = 0;
            int valid_num_obs = 0;
            
            for (int i=0; i<num_obs; i++) {
                if (x_undefs[t][i])
                    continue;
                valid_num_obs ++;
            }
            // count neighbors and neighors with 1
            for (int i=0; i<num_obs; i++) {
                if (x_undefs[t][i])
                    continue;
                num_neighbors[i] = W[i].Size();
                num_neighbors_1[t][i] = 0;
                for (int j=0; j< W[i].Size(); j++) {
                    if (x[W[i][j]] == 1) {
                        num_neighbors_1[t][i] += 1;
                    }
                }
                if (x[i] ==1)  num_obs_1s +=1;
                else num_obs_0s += 1;
            }
            for (int i=0; i<num_obs; i++) {
                if (x_undefs[t][i]) {
                    ep_vals[t][i] = 0;
                    continue;
                }
                int nn = W[i].Size();
                int n_1s = num_neighbors_1[t][i];
                int n_0s = nn - n_1s;
                
                double mm_all = Gda::nChoosek(valid_num_obs-1, nn);
                double mm_1s = Gda::nChoosek(num_obs_1s-1, n_1s);
                double mm_0s = Gda::nChoosek(num_obs_0s, n_0s);
                
                double hg = (mm_1s * mm_0s) / mm_all;
                ep_vals[t][i] = hg;
            }
        }
		for (int i=0; i<num_obs; i++) {
            if (x_undefs[t][i])
                continue;
			if ( W[i].Size() > 0 ) {
				n[t]++;
				x_star[t] += x[i];
				x_sstar[t] += x[i] * x[i];
			}
		}
		ExG[t] = 1.0/(n[t]-1); // same for all i when W is row-standardized
		ExGstar[t] = 1.0/n[t]; // same for all i when W is row-standardized
		mean_x[t] = x_star[t] / n[t]; // x hat (overall)
		var_x[t] = x_sstar[t]/n[t] - mean_x[t]*mean_x[t]; // s^2 overall
		
		// when W is row-standardized, VarGstar same for all i
		// same as s^2 / (n^2 mean_x ^2)
		VarGstar[t] = var_x[t] / (n[t]*n[t] * mean_x[t]*mean_x[t]);
		// when W is row-standardized, sdGstar same for all i
		sdGstar[t] = sqrt(VarGstar[t]);		
	}
	
	CalcGs();
	CalcPseudoP();
	wxLogMessage("Out GStatCoordinator::InitFromVarInfo");
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void GStatCoordinator::VarInfoAttributeChange()
{
	wxLogMessage("In GStatCoordinator::VarInfoAttributeChange");
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
	wxLogMessage("Out GStatCoordinator::VarInfoAttributeChange");
}

/** The category vector c_val will be filled based on the current
 significance filter and significance values corresponding to specified
 canvas_time.  */
void GStatCoordinator::FillClusterCats(int canvas_time, bool is_gi, bool is_perm, std::vector<wxInt64>& c_val)
{
	wxLogMessage("In GStatCoordinator::FillClusterCats()");
    
	int t = canvas_time;
	double* p_val = 0;
	if (is_gi && is_perm) p_val = pseudo_p_vecs[t];
	if (is_gi && !is_perm) p_val = p_vecs[t];
	if (!is_gi && is_perm) p_val = pseudo_p_star_vecs[t];
	if (!is_gi && !is_perm) p_val = p_star_vecs[t];
	double* z_val = is_gi ? z_vecs[t] : z_star_vecs[t];
	
    const GalElement* W = Gal_vecs[t]->gal;
    
	c_val.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
        if (!G_defined_vecs[t][i]) {
            c_val[i] = 4; // undefined
            
        } else if (W[i].Size() == 0) {
			c_val[i] = 3; // isolate
            
		} else if (p_val[i] <= significance_cutoff) {
            if (is_local_joint_count == false) {
                c_val[i] = z_val[i] > 0 ? 1 : 2; // high = 1, low = 2
                
            } else {
                if (x_vecs[t][i] == 1 && z_val[i] > 0)
                    c_val[i] = 1;
                else
                    c_val[i] = 0;
            }
		} else {
			c_val[i] = 0; // not significant
		}
	}
	wxLogMessage("Out GStatCoordinator::FillClusterCats()");
}


/** Initialize Gi and Gi_star.  We handle either binary or row-standardized
 binary weights.  Weights with self-neighbors are handled correctly. */
void GStatCoordinator::CalcGs()
{
	wxLogMessage("In GStatCoordinator::CalcGs()");
	using boost::math::normal; // typedef provides default type is double.
	// Construct a standard normal distribution std_norm_dist
	normal std_norm_dist; // default mean = zero, and s.d. = unity
	
	for (int t=0; t<num_time_vals; t++) {
		G = G_vecs[t];
		G_defined = G_defined_vecs[t];
		G_star = G_star_vecs[t];
		z = z_vecs[t];
		p = p_vecs[t];
		z_star = z_star_vecs[t];
		p_star = p_star_vecs[t];
		pseudo_p = pseudo_p_vecs[t];
		pseudo_p_star = pseudo_p_star_vecs[t];
		x = x_vecs[t];
		
		has_isolates[t] = false;
        
        const GalElement* W = Gal_vecs[t]->gal;
		double n_expr = sqrt((n[t]-1)*(n[t]-1)*(n[t]-2));
        
		for (long i=0; i<num_obs; i++) {
            if (x_undefs[t][i]) continue;
            
			const GalElement& elm_i = W[i];
			if ( elm_i.Size() > 0 ) {
				double lag = 0;
				bool self_neighbor = false;
				for (size_t j=0, sz=W[i].Size(); j<sz; j++) {
					if (elm_i[j] != i) {
						lag += x[elm_i[j]];
					} else {
						self_neighbor = true;
					}
				}
				double Wi = self_neighbor ? W[i].Size()-1 : W[i].Size();
				if (row_standardize) {
					lag /= elm_i.Size();
					Wi /= elm_i.Size();
				}
				double xd_i = x_star[t] - x[i];
				if (xd_i != 0) {
					G[i] = lag / xd_i;
				}
				double x_hat_i = xd_i * ExG[t]; // (x_star - x[i])/(n-1)
				double ExGi = Wi/(n[t]-1);
				// location-specific variance
				double ss_i = ((x_sstar[t] - x[i]*x[i])/(n[t]-1)-x_hat_i*x_hat_i);
				double sdG_i = sqrt(Wi*(n[t]-1-Wi)*ss_i)/(n_expr * x_hat_i);
				
				// compute z and one-sided p-val from standard-normal table
				if (G_defined[i]) {
					z[i] = (G[i] - ExGi)/sdG_i;
					if (z[i] >= 0) {
						p[i] = 1.0-cdf(std_norm_dist, z[i]);
					} else {
						p[i] = cdf(std_norm_dist, z[i]);
					}
				}
			} else {
				has_isolates[t] = true;
			}
		}
		if (row_standardize) {
			for (long i=0; i<num_obs; i++) {
                if (x_undefs[t][i]) {
                    G_star[i] = 0;
                    z_star[i] = 0;
                    continue;
                }
				const GalElement& elm_i = W[i];
				double lag = 0;
				bool self_neighbor = false;
				int sz_i=W[i].Size();
				for (int j=0; j<sz_i; j++) {
                    if (elm_i[j] == i) {
                        self_neighbor = true;
                    }
					lag += x[elm_i[j]];
				}
				G_star[i] = self_neighbor ? lag / (sz_i * x_star[t]) : (lag+x[i]) / ((sz_i+1) * x_star[t]);
				z_star[i] = (G_star[i] - ExGstar[t])/sdGstar[t];
			}
            
		} else { // binary weights
			double n_expr_mean_x = n[t] * sqrt(n[t]-1) * mean_x[t];
			for (long i=0; i<num_obs; i++) {
                if (x_undefs[t][i]) {
                    G_star[i] = 0;
                    z_star[i] = 0;
                    continue;
                }
				const GalElement& elm_i = W[i];
				double lag = 0;
				bool self_neighbor = false;
				for (int j=0, sz=elm_i.Size(); j<sz; j++) {
                    if (elm_i[j] == i) {
                        self_neighbor = true;
                    }
					lag += x[elm_i[j]];
				}
                if (!self_neighbor) {
                    lag += x[i];
                }
				G_star[i] = lag / x_star[t];
				double Wi = self_neighbor ? W[i].Size() : W[i].Size()+1;
				// location-specific mean
				double ExGi_star = Wi/n[t];
				// location-specific variance
				double sdG_i_star = sqrt(Wi*(n[t]-Wi)*var_x[t])/n_expr_mean_x;
				z_star[i] = (G_star[i] - ExGi_star)/sdG_i_star;
			}
		}
	
		for (long i=0; i<num_obs; i++) {
			// compute z and one-sided p-val from standard-normal table
			if (z_star[i] >= 0) {
				p_star[i] = 1.0-cdf(std_norm_dist, z_star[i]);
			} else {
				p_star[i] = cdf(std_norm_dist, z_star[i]);
			}
		}
	}
	wxLogMessage("Out GStatCoordinator::CalcGs()");
}

void GStatCoordinator::CalcPseudoP()
{
	wxLogMessage("Entering GStatCoordinator::CalcPseudoP");
	wxStopWatch sw;
	int nCPUs = wxThread::GetCPUCount();
	
    if (nCPUs <= 1) {
        if (!reuse_last_seed) last_seed_used = time(0);
        CalcPseudoP_range(0, num_obs-1, last_seed_used);
    } else {
        CalcPseudoP_threaded();
    }
	wxLogMessage("Exiting GStatCoordinator::CalcPseudoP");
}

void GStatCoordinator::CalcPseudoP_threaded()
{
	LOG_MSG("Entering GStatCoordinator::CalcPseudoP_threaded");
	int nCPUs = wxThread::GetCPUCount();
	
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
		int thread_id = i+1;
		
		GStatWorkerThread* thread =
			new GStatWorkerThread(a, b, last_seed_used, this,
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
			// alarm (spurious signal), the loop will exit.
		}
	}
	LOG_MSG("Exiting GStatCoordinator::CalcPseudoP_threaded");
}

/** In the code that computes Gi and Gi*, we specifically checked for 
 self-neighbors and handled the situation appropriately.  For the
 permutation code, we will disallow self-neighbors. */
void GStatCoordinator::CalcPseudoP_range(int obs_start, int obs_end,uint64_t seed_start)
{
	GeoDaSet workPermutation(num_obs);
	int max_rand = num_obs-1;
    
	for (long i=obs_start; i<=obs_end; i++) {
        std::vector<uint64_t> countGLarger(num_time_vals, 0);
        std::vector<uint64_t> countGStarLarger(num_time_vals, 0);
        
        // get full neighbors even if has undefined value
        int numNeighbors = 0;
        for (int t=0; t<num_time_vals; t++) {
            GalElement* w = Gal_vecs[t]->gal;
            if (w[i].Size() > numNeighbors)
                numNeighbors = w[i].Size();
        }
        if (numNeighbors == 0)
            continue;
        
        uint64_t seed = seed_start + i;
        seed = Gda::ThomasWangHashUInt64(seed);
        
        for (int perm=0; perm < permutations; perm++) {
            int rand = 0;
            while (rand < numNeighbors) {
                // computing 'perfect' permutation of given size
                double rng_val = Gda::ThomasWangDouble(seed) * max_rand;
                // round is needed to fix issue
                //https://github.com/GeoDaCenter/geoda/issues/488
                int newRandom = (int) (rng_val < 0.0 ? ceil(rng_val - 0.5) : floor(rng_val + 0.5));
                if (newRandom != i && !workPermutation.Belongs(newRandom)) {
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
                std::vector<bool>& undefs = x_undefs[t];
                double permutedG = 0;
                double permutedGStar = 0;
                double* _G = G_vecs[t];
                double* _G_star = G_star_vecs[t];
                double* _x = x_vecs[t];
                double _x_star_t = x_star[t];
               
                if (undefs[i])
                    continue;
                
                double xd_i = _x_star_t - _x[i];
                int validNeighbors = 0;
                double lag_i=0;
                
                // use permutation to compute the lags
                for (int j=0; j<numNeighbors; j++) {
                    int nb = permNeighbors[j];
                    if (!undefs[nb]) {
                        lag_i += _x[nb];
                        validNeighbors ++;
                    }
                }
                if (validNeighbors > 0 && row_standardize) {
                    permutedG = lag_i / (validNeighbors * xd_i);
                    permutedGStar = (lag_i+_x[i]) / ((validNeighbors+1.0)*_x_star_t);
                } else { // binary weights
                    // Wi = numNeighsD // assume no self-neighbors
                    permutedG = lag_i / xd_i;
                    permutedGStar = (lag_i+_x[i]) / _x_star_t;
                }
                
                if (permutedG >= _G[i]) countGLarger[t]++;
                if (permutedGStar >= _G_star[i]) countGStarLarger[t]++;
            }
        }
        
        for (int t=0; t<num_time_vals; t++) {
            double* p_t = pseudo_p_vecs[t];
            double* ps_t = pseudo_p_star_vecs[t];
            // pick the smallest
            if (permutations-countGLarger[t] < countGLarger[t]) {
                countGLarger[t] = permutations-countGLarger[t];
            }
            p_t[i] = (countGLarger[t] + 1.0)/(permutations+1.0);
            
            if (permutations-countGStarLarger[t] < countGStarLarger[t]) {
                countGStarLarger[t] = permutations-countGStarLarger[t];
            }
            ps_t[i] = (countGStarLarger[t] + 1.0)/(permutations+1.0);
        }
	}
}

void GStatCoordinator::SetSignificanceFilter(int filter_id)
{
	wxLogMessage("In GStatCoordinator::SetSignificanceFilter");
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
	wxLogMessage("Exiting GStatCoordinator::SetSignificanceFilter");
}

void GStatCoordinator::update(WeightsManState* o)
{
	weight_name = w_man_int->GetLongDispName(w_id);
}

int GStatCoordinator::numMustCloseToRemove(boost::uuids::uuid id) const
{
	int n=0;
	if (id == w_id) {
		for (std::vector<GetisOrdMapFrame*>::const_iterator i=maps.begin();
			 i != maps.end(); ++i) {
			if (*i != 0) ++n;
		}
	}
	return n;
}

void GStatCoordinator::closeObserver(boost::uuids::uuid id)
{
	if (numMustCloseToRemove(id) == 0) return;
	std::vector<GetisOrdMapFrame*> maps_cpy = maps;
	for (std::vector<GetisOrdMapFrame*>::iterator i=maps_cpy.begin();
		 i != maps_cpy.end(); ++i) {
		if (*i != 0) {
			(*i)->closeObserver(this);
		}
	}
}

void GStatCoordinator::registerObserver(GetisOrdMapFrame* o)
{
	maps[o->map_type] = o;
}

void GStatCoordinator::removeObserver(GetisOrdMapFrame* o)
{
	LOG_MSG("Entering GStatCoordinator::removeObserver");
	maps[o->map_type] = 0;
	int num_observers=0;
	for (int i=0, iend=maps.size(); i<iend; i++) if (maps[i]) num_observers++;
	if (num_observers == 0) {
		delete this;
	}
	LOG_MSG("Exiting GStatCoordinator::removeObserver");
}

void GStatCoordinator::notifyObservers()
{
	for (int i=0, iend=maps.size(); i<iend; i++) {
		if (maps[i]) maps[i]->update(this);
	}
}

