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

#ifndef __GEODA_CENTER_LOCALGEARY_COORDINATOR_H__
#define __GEODA_CENTER_LOCALGEARY_COORDINATOR_H__

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


class LocalGearyCoordinatorObserver;
class LocalGearyCoordinator;
class Project;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class LocalGearyWorkerThread : public wxThread
{
public:
	LocalGearyWorkerThread(const GalElement* W,
                     const std::vector<bool>& undefs,
                     int obs_start, int obs_end, uint64_t seed_start,
					 LocalGearyCoordinator* local_geary_coord,
					 wxMutex* worker_list_mutex,
					 wxCondition* worker_list_empty_cond,
					 std::list<wxThread*> *worker_list,
					 int thread_id);
	virtual ~LocalGearyWorkerThread();
	virtual void* Entry();  // thread execution starts here

    const GalElement* W;
    const std::vector<bool>& undefs;
	int obs_start;
	int obs_end;
	uint64_t seed_start;
	int thread_id;
	
	LocalGearyCoordinator* local_geary_coord;
	wxMutex* worker_list_mutex;
	wxCondition* worker_list_empty_cond;
	std::list<wxThread*> *worker_list;
};

class LocalGearyCoordinator : public WeightsManStateObserver
{
public:
    // #9
	enum LocalGearyType { univariate, bivariate, eb_rate_standardized, differential, multivariate };
	
	LocalGearyCoordinator(boost::uuids::uuid weights_id,
                    Project* project,
					const std::vector<GdaVarTools::VarInfo>& var_info,
					const std::vector<int>& col_ids,
					LocalGearyType local_geary_type, bool calc_significances = true,
                    bool row_standardize_s = true);
    
    LocalGearyCoordinator(wxString weights_path,
                    int n,
                    std::vector<std::vector<double> >& vars,
                    int permutations_s = 599,
                    bool calc_significances_s = true,
                    bool row_standardize_s = true);
    
	virtual ~LocalGearyCoordinator();
	
	bool IsOk() { return true; }
	wxString GetErrorMessage() { return "Error Message"; }

	int significance_filter; // 0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
    
	double significance_cutoff; // either 0.05, 0.01, 0.001 or 0.0001
    
	void SetSignificanceFilter(int filter_id);
    
	int GetSignificanceFilter() { return significance_filter; }
    
	int permutations; // any number from 9 to 99999, 99 will be default
	
	uint64_t GetLastUsedSeed() { return last_seed_used; }
    
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
	
protected:
	// The following seven are just temporary pointers into the corresponding
	// space-time data arrays below
	double* lags;
	double*	localGeary;		// The LocalGeary
	double* siglocalGeary;	// The significances / pseudo p-vals
	// The significance category, generated from siglocalGeary and
	// significance cuttoff values below.  When saving results to Table,
	// only results below significance_cuttoff are non-zero, but sigCat
	// results themeslves never change.
	//0: >0.05 1: 0.05, 2: 0.01, 3: 0.001, 4: 0.0001
	int* sigCat;
	// not-sig=0 HH=1, LL=2, HL=3, LH=4, isolate=5, undef=6.  Note: value of
	// 0 never appears in cluster itself, it only appears when
	// saving results to the Table and indirectly in the map legend
	int* cluster;
	double* data1;
	double* data1_square;
	double* data2;
    
    std::vector<double*> current_data;
    std::vector<double*> current_data_square;
	
public:
	std::vector<double*> lags_vecs;
	std::vector<double*> local_geary_vecs;
	std::vector<double*> sig_local_geary_vecs;
	std::vector<int*> sig_cat_vecs;
	std::vector<int*> cluster_vecs;
    
	std::vector<double*> data1_vecs;
	std::vector<double*> data1_square_vecs;
	std::vector<double*> data2_vecs;
	
	boost::uuids::uuid w_id;
    std::vector<GalWeight*> Gal_vecs;
    std::vector<GalWeight*> Gal_vecs_orig;
	//const GalElement* W;
    
	wxString weight_name;
	bool isBivariate;
	LocalGearyType local_geary_type;

    int num_vars;
	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	// These two variables should be empty for LocalGearyMapCanvas
	std::vector<d_array_type> data; // data[variable][time][obs]
	std::vector<b_array_type> undef_data; // undef_data[variable][time][obs]
    
    std::vector<std::vector<bool> > undef_tms;

    // These are for multi variable LocalGeary
    std::vector<std::vector<double*> > data_vecs;
    std::vector<std::vector<double*> > data_square_vecs;
    
	// All LocalGearyMapCanvas objects synchronize themselves
	// from the following 6 variables.
	int ref_var_index;
	std::vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	std::vector<bool> map_valid;
	std::vector<wxString> map_error_message;
	
	bool GetHasIsolates(int time) { return has_isolates[time]; }
	bool GetHasUndefined(int time) { return has_undefined[time]; }
	
	void registerObserver(LocalGearyCoordinatorObserver* o);
	void removeObserver(LocalGearyCoordinatorObserver* o);
	void notifyObservers();
	/** The list of registered observer objects. */
	std::list<LocalGearyCoordinatorObserver*> observers;
	
	void CalcPseudoP();
	void CalcPseudoP_range(const GalElement* W, const std::vector<bool>& undefs,
                           int obs_start, int obs_end, uint64_t seed_start);

	void InitFromVarInfo();
	void VarInfoAttributeChange();
    
    void GetRawData(int time, double* data1, double* data2);

protected:
	void DeallocateVectors();
	void AllocateVectors();
	
	void CalcPseudoP_threaded(const GalElement* W, const std::vector<bool>& undefs);
	void CalcLocalGeary();
	void CalcMultiLocalGeary();
	void StandardizeData();
	std::vector<bool> has_undefined;
	std::vector<bool> has_isolates;
	bool row_standardize;
	bool calc_significances; // if false, then p-vals will never be needed
	uint64_t last_seed_used;
	bool reuse_last_seed;
	
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
    
    GalWeight* weights;
};

#endif
