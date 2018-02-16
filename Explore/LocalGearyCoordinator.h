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

using namespace std;

class LocalGearyCoordinatorObserver;
class LocalGearyCoordinator;
class Project;
class WeightsManState;
typedef boost::multi_array<double, 2> d_array_type;
typedef boost::multi_array<bool, 2> b_array_type;

class LocalGearyWorkerThread : public wxThread
{
public:
    LocalGearyWorkerThread(int obs_start, int obs_end, uint64_t seed_start,
                           LocalGearyCoordinator* local_geary_coord,
                           wxMutex* worker_list_mutex,
                           wxCondition* worker_list_empty_cond,
                           std::list<wxThread*> *worker_list,
                           int thread_id);
    virtual ~LocalGearyWorkerThread();
    virtual void* Entry();  // thread execution starts here
    
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
	enum LocalGearyType {
        univariate,
        bivariate,
        eb_rate_standardized,
        differential,
        multivariate };
	
	LocalGearyCoordinator(boost::uuids::uuid weights_id,
                    Project* project,
					const vector<GdaVarTools::VarInfo>& var_info,
					const vector<int>& col_ids,
					LocalGearyType local_geary_type, bool calc_significances = true,
                    bool row_standardize_s = true);
    
    LocalGearyCoordinator(wxString weights_path,
                    int n,
                    vector<vector<double> >& vars,
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
    
    double bo; //Bonferroni bound
    
    double fdr; //False Discovery Rate
    double user_sig_cutoff; // user defined cutoff
	
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
	
public:
	vector<double*> lags_vecs;
	vector<double*> local_geary_vecs;
	vector<double*> sig_local_geary_vecs;
	vector<int*> sig_cat_vecs;
	vector<int*> cluster_vecs;
    
	vector<double*> data1_vecs;
	vector<double*> data1_square_vecs;
	vector<double*> data2_vecs;
	
	boost::uuids::uuid w_id;
    vector<GalWeight*> Gal_vecs;
    vector<GalWeight*> Gal_vecs_orig;
	//const GalElement* W;
    
	wxString weight_name;
	bool isBivariate;
	LocalGearyType local_geary_type;

    int num_vars;
	int num_obs; // total # obs including neighborless obs
	int num_time_vals; // number of valid time periods based on var_info
	
	// These two variables should be empty for LocalGearyMapCanvas
	vector<d_array_type> data; // data[variable][time][obs]
	vector<b_array_type> undef_data; // undef_data[variable][time][obs]
    
    vector<vector<bool> > undef_tms;

    // These are for multi variable LocalGeary
    vector<vector<double*> > data_vecs;
    vector<vector<double*> > data_square_vecs;
    
	// All LocalGearyMapCanvas objects synchronize themselves
	// from the following 6 variables.
	int ref_var_index;
	vector<GdaVarTools::VarInfo> var_info;
	bool is_any_time_variant;
	bool is_any_sync_with_global_time;
	vector<bool> map_valid;
	vector<wxString> map_error_message;
	
	bool GetHasIsolates(int time) { return has_isolates[time]; }
	bool GetHasUndefined(int time) { return has_undefined[time]; }
	
	void registerObserver(LocalGearyCoordinatorObserver* o);
	void removeObserver(LocalGearyCoordinatorObserver* o);
	void notifyObservers();
	/** The list of registered observer objects. */
	list<LocalGearyCoordinatorObserver*> observers;
	
	void CalcPseudoP();
	void CalcPseudoP_range(int obs_start,
                           int obs_end,
                           uint64_t seed_start);

	void InitFromVarInfo();
	void VarInfoAttributeChange();
    
    void GetRawData(int time, double* data1, double* data2);

protected:
	void DeallocateVectors();
	void AllocateVectors();
	
	void CalcPseudoP_threaded();
	void CalcLocalGeary();
	void CalcMultiLocalGeary();
	void StandardizeData();
	vector<bool> has_undefined;
	vector<bool> has_isolates;
	bool row_standardize;
	bool calc_significances; // if false, then p-vals will never be needed
	uint64_t last_seed_used;
	bool reuse_last_seed;
	
	WeightsManState* w_man_state;
	WeightsManInterface* w_man_int;
    
    GalWeight* weights;
};

#endif
