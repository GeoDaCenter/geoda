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

#ifndef __GEODA_CENTER_VARS_CHOOSER_OBSERVABLE_H__
#define __GEODA_CENTER_VARS_CHOOSER_OBSERVABLE_H__

#include <list>
#include <wx/string.h>
#include "../VarTools.h"

class VarsChooserObserver; // forward declaration

class VarsChooserObservable {
public:
	VarsChooserObservable(GdaVarTools::Manager& var_man);
	virtual ~VarsChooserObservable();
	
	/** Signal that VarsChooserObservable should be closed, but wait until
	 all observers have deregistered themselves. */
	virtual void closeAndDeleteWhenEmpty();
	
	/** Notify all Observers by calling notifyOfClosing to indicate that 
	 this Observable is closing now.  Each observer should simply
	 remove any reference to this Observable.
	 It is crucial that Observers do not attempt to call removeObserver
	 in the future since this Observable will already be gone. */
	virtual void notifyObserversOfClosing();
	
	virtual void registerObserver(VarsChooserObserver* o);
	virtual void removeObserver(VarsChooserObserver* o);
	/** Call update function in all Observers to notify of a change */
	virtual void notifyObservers();
	
protected:
	/** The list of registered VarsChooserObserver objects. */
	std::list<VarsChooserObserver*> observers;

	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the VarsChooserObservable instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
	
	GdaVarTools::Manager& var_man;
};

#endif
