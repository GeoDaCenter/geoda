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

#ifndef __GEODA_CENTER_ABSTRACT_COORDINATOR_H__
#define __GEODA_CENTER_ABSTRACT_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../VarTools.h"
#include "../ShapeOperations/GeodaWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"


class Project;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;


class AbstractCoordinator;  // forward declaration

class AbstractCoordinatorObserver {
public:
    virtual void update(AbstractCoordinator* o) = 0;
    /** Request for the Observer to close itself */
    virtual void closeObserver(AbstractCoordinator* o) = 0;
};


class AbstractWorkerThread : public wxThread
{
public:
	AbstractWorkerThread(int obs_start,
                         int obs_end,
                         uint64_t seed_start,
                         AbstractCoordinator* a_coord,
                         wxMutex* worker_list_mutex,
                         wxCondition* worker_list_empty_cond,
                         std::list<wxThread*> *worker_list,
                         int thread_id);
	virtual ~AbstractWorkerThread();
	virtual void* Entry();  // thread execution starts here

	int obs_start;
	int obs_end;
	uint64_t seed_start;
	int thread_id;
	
	AbstractCoordinator* a_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class AbstractCoordinator : public WeightsManStateObserver
{
public:
    AbstractCoordinator();
	AbstractCoordinator(boost::uuids::uuid weights_id,
                        Project* project,
                        const std::vector<GdaVarTools::VarInfo>& var_info,
                        const std::vector<int>& col_ids,
                        bool calc_significances = true,
                        bool row_standardize_s = true);
    
	virtual ~AbstractCoordinator();
	
    virtual bool IsOk();
    
    virtual wxString GetErrorMessage();

	virtual void SetSignificanceFilter(int filter_id);
    
    virtual int GetSignificanceFilter();
    
    virtual double GetSignificanceCutoff();
    virtual void SetSignificanceCutoff(double val);
    
    virtual double GetUserCutoff();
    virtual void SetUserCutoff(double val);
    
    virtual double GetBO();
    virtual void SetBO(double val);
    
    virtual double GetFDR();
    virtual void SetFDR(double val);
    
    virtual int GetNumPermutations();
    virtual void SetNumPermutations(int val);

    virtual uint64_t GetLastUsedSeed();
    
    virtual void SetLastUsedSeed(uint64_t seed);
    
    virtual bool IsReuseLastSeed();
    
    virtual void SetReuseLastSeed(bool reuse);

	virtual int numMustCloseToRemove(boost::uuids::uuid id) const;
    
	virtual void closeObserver(boost::uuids::uuid id);
    
    virtual bool GetHasIsolates(int time);
    
    virtual bool GetHasUndefined(int time);
    
    /** Implementation of WeightsManStateObserver interface */
    virtual void update(WeightsManState* o);
    
    virtual void registerObserver(AbstractCoordinatorObserver* o);
    
    virtual void removeObserver(AbstractCoordinatorObserver* o);
    
    virtual void notifyObservers();
    
    virtual void CalcPseudoP_threaded();
    
    virtual void Calc() = 0;
    
    virtual void Init() = 0;
    
    virtual void CalcPseudoP();
    
    virtual void CalcPseudoP_range(int obs_start, int obs_end,
                                   uint64_t seed_start);
    
    virtual void ComputeLarger(int cnt, std::vector<int>& permNeighbors,
                               std::vector<uint64_t>& countLarger) = 0;
    
    virtual std::vector<wxString> GetDefaultCategories();
    
    virtual std::vector<double> GetDefaultCutoffs();
    
    void InitFromVarInfo();
    
    void VarInfoAttributeChange();
    
    void DeallocateVectors();
    
    void AllocateVectors();
    
    double* GetLocalSignificanceValues(int t);
    
    int* GetClusterIndicators(int t);
    
    int* GetSigCatIndicators(int t);
    
    boost::uuids::uuid GetWeightsID();
    
    wxString GetWeightsName(); 
    
protected:
    int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
    int permutations; // any number from 9 to 99999, 99 will be default
    double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
    double bo; //Bonferroni bound
    double fdr; //False Discovery Rate
    double user_sig_cutoff; // user defined cutoff
    std::vector<bool> has_undefined;
    std::vector<bool> has_isolates;
    bool row_standardize;
    bool calc_significances; // if false, then p-vals will never be needed
    uint64_t last_seed_used;
    bool reuse_last_seed;
    
    WeightsManState* w_man_state;
    WeightsManInterface* w_man_int;
    
    GalWeight* weights;
    
    std::vector<double*> sig_local_vecs;
    std::vector<int*> sig_cat_vecs;
    std::vector<int*> cluster_vecs;
    
    boost::uuids::uuid w_id;
    wxString weight_name;
    
public:
    std::vector<GalWeight*> Gal_vecs;
    std::vector<GalWeight*> Gal_vecs_orig;

	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	std::vector<d_array_type> data; // data[variable][time][obs]
	std::vector<b_array_type> undef_data; // undef_data[variable][time][obs]
    std::vector<std::vector<bool> > undef_tms;
	
	// All LisaMapCanvas objects synchronize themselves
	// from the following 6 variables.
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
    
    const static int HH_CLUSTER = 1;
    const static int LL_CLUSTER = 2;
    const static int LH_CLUSTER = 3;
    const static int HL_CLUSTER = 4;
    const static int NEIGHBORLESS_CLUSTER = 5;
    const static int UNDEFINED_CLUSTER = 6;
	
	/** The list of registered observer objects. */
	std::list<AbstractCoordinatorObserver*> observers;
};

#endif
