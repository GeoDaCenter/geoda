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

/**
 NOTE: JCCoordinator and GetisOrdMapNewView implement the
 Observable/Observer interface.  However, we have chosen not to define
 a JCCoordinatorObserver interface for GetisOrdMapNewView to implement
 because JCCoordinator needs to know more details about the
 GetisOrdMapNewView instances that register with it.  In particular, we only
 allow at most 8 different GetisOrdMapNewView instances to be observers, and
 each instance must be a different type according to the options enumerated
 in GetisOrdMapNewView::GMapType.
 */

#ifndef __GEODA_CENTER_MLJC_COORDINATOR_H__
#define __GEODA_CENTER_MLJC_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../VarTools.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"


class JCCoordinatorObserver; 
class JCCoordinator;
class Project;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class JCWorkerThread : public wxThread
{
public:
    JCWorkerThread(int t,
                   int obs_start, int obs_end, uint64_t seed_start,
                   JCCoordinator* jc_coord,
                   wxMutex* worker_list_mutex,
                   wxCondition* worker_list_empty_cond,
                   std::list<wxThread*> *worker_list,
                   int thread_id);
    
	virtual ~JCWorkerThread();
	virtual void* Entry();  // thread execution starts here

    int t;
	int obs_start;
	int obs_end;
	uint64_t seed_start;
	int thread_id;
	
	JCCoordinator* jc_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class JCCoordinator : public WeightsManStateObserver
{
public:
    JCCoordinator(boost::uuids::uuid weights_id, Project* project,
                  const std::vector<GdaVarTools::VarInfo>& var_info,
                  const std::vector<int>& col_ids);
	virtual ~JCCoordinator();
	
	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }
		
	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
	void SetSignificanceFilter(int filter_id);
	int GetSignificanceFilter() { return significance_filter; }
	int permutations; // any number from 9 to 99999, 99 will be default
    double bo; //Bonferroni bound
    double fdr; //False Discovery Rate
    double user_sig_cutoff; // user defined cutoff

	uint64_t GetLastUsedSeed() { return last_seed_used;}
    
	void SetLastUsedSeed(uint64_t seed) {
        reuse_last_seed = true;
        last_seed_used = seed;
        // update global one
        GdaConst::use_gda_user_seed = true;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
        GdaConst::gda_user_seed =  last_seed_used;
        wxString val;
        val << last_seed_used;
        OGRDataAdapter::GetInstance().AddEntry("gda_user_seed", val);
    }
    
	bool IsReuseLastSeed() { return reuse_last_seed; }
	void SetReuseLastSeed(bool reuse) {
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
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const;
	virtual void closeObserver(boost::uuids::uuid id);
	
    vector<vector<double*> > data_vecs;
    vector<vector<bool> > undef_tms;
    vector<int*> zz_vecs;
    vector<double*> local_jc_vecs;
    vector<double*> sig_local_jc_vecs;
    std::vector<std::vector<wxInt64> > num_neighbors;

    std::vector<GalWeight*> Gal_vecs;
    std::vector<GalWeight*> Gal_vecs_orig;
	
	std::vector<bool> has_isolates;
	std::vector<bool> has_undefined;

	boost::uuids::uuid w_id;
	wxString weight_name;

    int num_vars;
	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	// This variable should be empty for GStatMapCanvas
	std::vector<d_array_type> data; // data[variable][time][obs]
	std::vector<b_array_type> undef_data; // data[variable][time][obs]
	
	// All objects synchronize themselves from the following 6 variables.
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
    
	
	bool GetHasIsolates(int time) { return has_isolates[time]; }
	bool GetHasUndefined(int time) { return has_undefined[time]; }
	
	void registerObserver(JCCoordinatorObserver* o);
	void removeObserver(JCCoordinatorObserver* o);
	void notifyObservers();
    
	/** The array of registered observer objects. */
    std::list<JCCoordinatorObserver*> observers;
	
	void CalcPseudoP();
	void CalcPseudoP_range(int t,
                           int obs_start,
                           int obs_end,
                           uint64_t seed_start);
	
	void InitFromVarInfo();
    
	void VarInfoAttributeChange();
	
	void FillClusterCats(int canvas_time,std::vector<wxInt64>& c_val);
    
protected:
	// The following ten are just temporary pointers into the corresponding
	// space-time data arrays below
    
    
    GalWeight* weights;
	

	uint64_t last_seed_used;
	bool reuse_last_seed;
	
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
    
	void DeallocateVectors();
	void AllocateVectors();
    
	void CalcPseudoP_threaded(int t);
    
	void CalcMultiLocalJoinCount();
};

#endif
