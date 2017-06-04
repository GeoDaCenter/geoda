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
 NOTE: GStatCoordinator and GetisOrdMapNewView implement the
 Observable/Observer interface.  However, we have chosen not to define
 a GStatCoordinatorObserver interface for GetisOrdMapNewView to implement
 because GStatCoordinator needs to know more details about the
 GetisOrdMapNewView instances that register with it.  In particular, we only
 allow at most 8 different GetisOrdMapNewView instances to be observers, and
 each instance must be a different type according to the options enumerated
 in GetisOrdMapNewView::GMapType.
 */

#ifndef __GEODA_CENTER_G_STAT_COORDINATOR_H__
#define __GEODA_CENTER_G_STAT_COORDINATOR_H__

#include <list>
#include <vector>
#include <boost/multi_array.hpp>
#include <wx/string.h>
#include <wx/thread.h>
#include "../VarTools.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/WeightsManStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"


class GetisOrdMapFrame; // instead of GStatCoordinatorObserver
class GStatCoordinator;
class Project;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class GStatWorkerThread : public wxThread
{
public:
    GStatWorkerThread(const GalElement* W,
                      const std::vector<bool>& undefs,
                      int obs_start, int obs_end, uint64_t seed_start,
                      GStatCoordinator* gstat_coord,
                      wxMutex* worker_list_mutex,
                      wxCondition* worker_list_empty_cond,
                      std::list<wxThread*> *worker_list,
                      int thread_id);
	virtual ~GStatWorkerThread();
	virtual void* Entry();  // thread execution starts here

    const GalElement* W;
    const std::vector<bool>& undefs;
	int obs_start;
	int obs_end;
	uint64_t seed_start;
	int thread_id;
	
	GStatCoordinator* gstat_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class GStatCoordinator : public WeightsManStateObserver
{
public:
	GStatCoordinator(boost::uuids::uuid weights_id, Project* project,
					 const std::vector<GdaVarTools::VarInfo>& var_info,
					 const std::vector<int>& col_ids,
					 bool row_standardize_weights);
	virtual ~GStatCoordinator();
	
	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }
		
	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
	void SetSignificanceFilter(int filter_id);
	int GetSignificanceFilter() { return significance_filter; }
	int permutations; // any number from 9 to 99999, 99 will be default
	
	uint64_t GetLastUsedSeed() {
        return last_seed_used;
    }
    
	void SetLastUsedSeed(uint64_t seed) {
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
	
	std::vector<double> n; // # non-neighborless observations
	
	double x_star_t; // temporary x_star for use in worker threads
	std::vector<double> x_star; // sum of all x_i // threaded
	std::vector<double> x_sstar; // sum of all (x_i)^2
		
	std::vector<double> ExG; // same for all i since we row-standardize W
	std::vector<double> ExGstar; // same for all i since we row-standardize W
	std::vector<double> mean_x; // x hat (overall)
	std::vector<double> var_x; // s^2 overall
	// since W is row-standardized, VarGstar same for all i
	// same as s^2 / (n^2 mean_x ^2)
	std::vector<double> VarGstar;
	// since W is row-standardized, sdGstar same for all i
	std::vector<double> sdGstar;
	
protected:
	// The following ten are just temporary pointers into the corresponding
	// space-time data arrays below
	double* G; //threaded
	bool* G_defined; // check for divide-by-zero //threaded
	double* G_star; //threaded
	// z-val corresponding to each G_i
	double* z;
	// p-val from z_i using standard normal table
	double* p;
	// z-val corresponding to each G_star_i
	double* z_star;
	// p-val from z_i^star using standard normal table
	double* p_star;
	double* pseudo_p; //threaded
	double* pseudo_p_star; //threaded
	double* x; //threaded
	
public:
	std::vector<double*> G_vecs; //threaded
	std::vector<bool*> G_defined_vecs; // check for divide-by-zero //threaded
                                       // as well as undefined values
	std::vector<double*> G_star_vecs; //threaded
	// z-val corresponding to each G_i
	std::vector<double*> z_vecs;
	// p-val from z_i using standard normal table
	std::vector<double*> p_vecs;
	// z-val corresponding to each G_star_i
	std::vector<double*> z_star_vecs;
	// p-val from z_i^star using standard normal table
	std::vector<double*> p_star_vecs;
	std::vector<double*> pseudo_p_vecs; //threaded
	std::vector<double*> pseudo_p_star_vecs; //threaded
	std::vector<double*> x_vecs; //threaded
    std::vector<std::vector<bool> > x_undefs;

	boost::uuids::uuid w_id;
    std::vector<GalWeight*> Gal_vecs;
    std::vector<GalWeight*> Gal_vecs_orig;
	wxString weight_name;

	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	// This variable should be empty for GStatMapCanvas
	std::vector<d_array_type> data; // data[variable][time][obs]
	std::vector<b_array_type> data_undef; // data[variable][time][obs]
	
	// All GetisOrdMapCanvas objects synchronize themselves
	// from the following 6 variables.
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	bool GetHasIsolates(int time) { return has_isolates[time]; }
	bool GetHasUndefined(int time) { return has_undefined[time]; }
	
	void registerObserver(GetisOrdMapFrame* o);
	void removeObserver(GetisOrdMapFrame* o);
	void notifyObservers();
	/** The array of registered observer objects. */
	std::vector<GetisOrdMapFrame*> maps;
	
	void CalcPseudoP();
	void CalcPseudoP_range(const GalElement* W, const std::vector<bool>& undefs,
                           int obs_start, int obs_end, uint64_t seed_start);
	
	void InitFromVarInfo();
	void VarInfoAttributeChange();
	
	void FillClusterCats(int canvas_time, bool is_gi, bool is_perm,
						 std::vector<wxInt64>& c_val);
protected:
	void DeallocateVectors();
	void AllocateVectors();
	
	void CalcPseudoP_threaded(const GalElement* W, const std::vector<bool>& undefs);
	void CalcGs();
	std::vector<bool> has_undefined;
	std::vector<bool> has_isolates;
	bool row_standardize;
	uint64_t last_seed_used;
	bool reuse_last_seed;
	
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
};

#endif
