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

#ifndef __GEODA_CENTER_CORREL_PARAMS_OBSERVABLE_H__
#define __GEODA_CENTER_CORREL_PARAMS_OBSERVABLE_H__

#include <list>
#include <wx/string.h>
#include "../VarTools.h"
#include "../VarCalc/WeightsMetaInfo.h"

class CorrelParamsObserver; // forward declaration

struct CorrelParams
{
	enum MethodEnum { ALL_PAIRS, ALL_PAIRS_THRESH, RAND_SAMP, RAND_SAMP_THRESH };
	CorrelParams() {
		bins = def_bins_cnst;
		dist_metric = WeightsMetaInfo::DM_euclidean;
		dist_units = WeightsMetaInfo::DU_mile;
		method = RAND_SAMP;
		max_iterations = def_iter_cnst;
		threshold = -1;
	}
	long bins;
	WeightsMetaInfo::DistanceMetricEnum dist_metric;
	WeightsMetaInfo::DistanceUnitsEnum dist_units;
	MethodEnum method;
	long max_iterations;
	double threshold; // if <=0 then maximum distance
	
	static const long max_bins_cnst = 100;
	static const long def_bins_cnst = 10;
	static const long min_bins_cnst = 1;
	static const long max_iter_cnst = 1000000000; // billion
	static const long def_iter_cnst = 1000000; // million
	static const long min_iter_cnst = 10;	
};

class CorrelParamsObservable {
public:
	CorrelParamsObservable(const CorrelParams& correl_params,
												 GdaVarTools::Manager& var_man);
	virtual ~CorrelParamsObservable();
	
	/** Signal that CorrelParamsObservable should be closed, but wait until
	 all observers have deregistered themselves. */
	virtual void closeAndDeleteWhenEmpty();
	
	/** Notify all Observers by calling notifyOfClosing to indicate that 
	 this Observable is closing now.  Each observer should simply
	 remove any reference to this Observable.
	 It is crucial that Observers do not attempt to call removeObserver
	 in the future since this Observable will already be gone. */
	virtual void notifyObserversOfClosing();
	
	virtual void registerObserver(CorrelParamsObserver* o);
	virtual void removeObserver(CorrelParamsObserver* o);
	/** Call update function in all Observers to notify of a change */
	virtual void notifyObservers();
	
	virtual CorrelParams GetCorrelParams() const;
	
protected:
	/** The list of registered CorrelParamsObserver objects. */
	std::list<CorrelParamsObserver*> observers;

	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the CorrelParamsObservable instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
	
	CorrelParams correl_params;
	GdaVarTools::Manager& var_man;
};

#endif
