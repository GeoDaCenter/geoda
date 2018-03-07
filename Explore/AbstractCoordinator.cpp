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

AbstractWorkerThread::AbstractWorkerThread(int obs_start_s,
                                           int obs_end_s,
                                           uint64_t	seed_start_s,
                                           AbstractCoordinator* a_coord_s,
                                           wxMutex* worker_list_mutex_s,
                                           wxCondition* worker_list_empty_cond_s,
                                           std::list<wxThread*> *worker_list_s,
                                           int thread_id_s)
: wxThread(),
obs_start(obs_start_s),
obs_end(obs_end_s),
seed_start(seed_start_s),
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

///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////
AbstractCoordinator::AbstractCoordinator()
{
    
}

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
last_seed_used(123456789),
reuse_last_seed(true),
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
	}
    
    undef_tms.resize(var_info_s[0].time_max - var_info_s[0].time_min + 1);
	
	weight_name = w_man_int->GetLongDispName(w_id);
    
    weights = w_man_int->GetGal(w_id);
    
	SetSignificanceFilter(1);
    
	w_man_state->registerObserver(this);
   
    wxLogMessage("Exiting AbstractCoordinator::AbstractCoordinator().");
}

AbstractCoordinator::~AbstractCoordinator()
{
    wxLogMessage("In AbstractCoordinator::~AbstractCoordinator().");
    if (w_man_state) {
        w_man_state->removeObserver(this);
    }
	DeallocateVectors();
}

std::vector<wxString> AbstractCoordinator::GetDefaultCategories()
{
    std::vector<wxString> cats;
    cats.push_back("p = 0.05");
    cats.push_back("p = 0.01");
    cats.push_back("p = 0.001");
    cats.push_back("p = 0.0001");
    return cats;
}

std::vector<double> AbstractCoordinator::GetDefaultCutoffs()
{
    std::vector<double> cutoffs;
    cutoffs.push_back(0.05);
    cutoffs.push_back(0.01);
    cutoffs.push_back(0.001);
    cutoffs.push_back(0.0001);
    return cutoffs;
}

void AbstractCoordinator::DeallocateVectors()
{
    wxLogMessage("Entering AbstractCoordinator::DeallocateVectors()");

	for (int i=0; i<sig_local_vecs.size(); i++) {
		if (sig_local_vecs[i]) delete [] sig_local_vecs[i];
	}
	sig_local_vecs.clear();
    
	for (int i=0; i<sig_cat_vecs.size(); i++) {
		if (sig_cat_vecs[i]) delete [] sig_cat_vecs[i];
	}
	sig_cat_vecs.clear();
    
	for (int i=0; i<cluster_vecs.size(); i++) {
		if (cluster_vecs[i]) delete [] cluster_vecs[i];
	}
	cluster_vecs.clear();

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
    
	sig_local_vecs.resize(tms);
	sig_cat_vecs.resize(tms);
	cluster_vecs.resize(tms);
	map_valid.resize(tms);
	map_error_message.resize(tms);
	has_isolates.resize(tms);
	has_undefined.resize(tms);
    Gal_vecs.resize(tms);
    Gal_vecs_orig.resize(tms);
    
	for (int i=0; i<tms; i++) {
		if (calc_significances) {
			sig_local_vecs[i] = new double[num_obs];
			sig_cat_vecs[i] = new int[num_obs];
		}
		cluster_vecs[i] = new int[num_obs];
		map_valid[i] = true;
		map_error_message[i] = wxEmptyString;
	}
    
    wxLogMessage("Exiting AbstractCoordinator::AllocateVectors()");
}

/** We assume only that var_info is initialized correctly.
 ref_var_index, is_any_time_variant, is_any_sync_with_global_time and
 num_time_vals are first updated based on var_info */ 
void AbstractCoordinator::InitFromVarInfo()
{
    wxLogMessage("Entering AbstractCoordinator::InitFromVarInfo()");
    Init();
    
	DeallocateVectors();
	AllocateVectors();

    Calc();
    if (calc_significances) {
        CalcPseudoP();
    }
    wxLogMessage("Exiting AbstractCoordinator::InitFromVarInfo()");
}

void AbstractCoordinator::SetLastUsedSeed(uint64_t seed)
{
    reuse_last_seed = true;
    last_seed_used = seed;
    // update global one
    GdaConst::use_gda_user_seed = true;
    OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
    GdaConst::gda_user_seed =  last_seed_used;
    wxString val;
    val << last_seed_used;
    OGRDataAdapter::GetInstance().AddEntry("gda_user_seed", val.ToStdString());
}

bool AbstractCoordinator::GetHasIsolates(int time)
{
    return has_isolates[time];
}

bool AbstractCoordinator::GetHasUndefined(int time)
{
    return has_undefined[time];
    
}

bool AbstractCoordinator::IsOk()
{
    return true;
}

wxString AbstractCoordinator::GetErrorMessage()
{
    return "Error Message";
}

int AbstractCoordinator::GetSignificanceFilter()
{
    return significance_filter;
}

double AbstractCoordinator::GetSignificanceCutoff()
{
    return significance_cutoff;
}

void AbstractCoordinator::SetSignificanceCutoff(double val)
{
    significance_cutoff = val;
}

double AbstractCoordinator::GetUserCutoff()
{
    return user_sig_cutoff;
}
void AbstractCoordinator::SetUserCutoff(double val)
{
    user_sig_cutoff = val;
}

double AbstractCoordinator::GetFDR()
{
    return fdr;
}
void AbstractCoordinator::SetFDR(double val)
{
    fdr = val;
}

double AbstractCoordinator::GetBO()
{
    return bo;
}
void AbstractCoordinator::SetBO(double val)
{
    bo = val;
}

int AbstractCoordinator::GetNumPermutations()
{
    return permutations;
}
void AbstractCoordinator::SetNumPermutations(int val)
{
    permutations = val;
}

double* AbstractCoordinator::GetLocalSignificanceValues(int t)
{
    return sig_local_vecs[t];
}

int* AbstractCoordinator::GetClusterIndicators(int t)
{
    return cluster_vecs[t];
}

int* AbstractCoordinator::GetSigCatIndicators(int t)
{
    return sig_cat_vecs[t];
}

boost::uuids::uuid AbstractCoordinator::GetWeightsID()
{
    return w_id;
}

wxString AbstractCoordinator::GetWeightsName()
{
    return weight_name;
}

uint64_t AbstractCoordinator::GetLastUsedSeed()
{
    return last_seed_used;
}

bool AbstractCoordinator::IsReuseLastSeed()
{
    return reuse_last_seed;
}

void AbstractCoordinator::SetReuseLastSeed(bool reuse)
{
    reuse_last_seed = reuse;
    // update global one
    GdaConst::use_gda_user_seed = reuse;
    if (reuse) {
        last_seed_used = GdaConst::gda_user_seed;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
    } else {
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "0");
    }
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
        if (var_info[i].is_time_variant) {
            is_any_time_variant = true;
        }
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

void AbstractCoordinator::CalcPseudoP_range(int obs_start, int obs_end,
                                            uint64_t seed_start)
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
            double rng_val;
            int newRandom;
			while (rand < numNeighbors) {
				// computing 'perfect' permutation of given size
                rng_val = Gda::ThomasWangHashDouble(seed_start++) * max_rand;
                // round is needed to fix issue
                // https://github.com/GeoDaCenter/geoda/issues/488
				newRandom = (int)(rng_val<0.0?ceil(rng_val - 0.5):floor(rng_val + 0.5));
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
            ComputeLarger(cnt, permNeighbors, countLarger);
		}
        for (int t=0; t<num_time_vals; t++) {
            double* _sigLocal = sig_local_vecs[t];
            int* _sigCat = sig_cat_vecs[t];

    		// pick the smallest
    		if (permutations-countLarger[t] <= countLarger[t]) {
    			countLarger[t] = permutations-countLarger[t];
    		}
    		
    		_sigLocal[cnt] = (countLarger[t]+1.0)/(permutations+1);
    		// 'significance' of local Moran
    		if (_sigLocal[cnt] <= 0.0001) _sigCat[cnt] = 4;
    		else if (_sigLocal[cnt] <= 0.001) _sigCat[cnt] = 3;
    		else if (_sigLocal[cnt] <= 0.01) _sigCat[cnt] = 2;
    		else if (_sigLocal[cnt] <= 0.05) _sigCat[cnt]= 1;
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
    //return 0;
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

