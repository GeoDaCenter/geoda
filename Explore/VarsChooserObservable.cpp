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

#include "../logger.h"
#include "../GenUtils.h"
#include "VarsChooserObserver.h"
#include "VarsChooserObservable.h"

VarsChooserObservable::VarsChooserObservable(GdaVarTools::Manager& var_man_)
: delete_self_when_empty(false),
var_man(var_man_)
{
	LOG_MSG("In VarsChooserObservable::VarsChooserObservable");
}

VarsChooserObservable::~VarsChooserObservable()
{
	LOG_MSG("In VarsChooserObservable::~VarsChooserObservable");
}

void VarsChooserObservable::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering VarsChooserObservable::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
	LOG_MSG("Exiting VarsChooserObservable::closeAndDeleteWhenEmpty");
}

void VarsChooserObservable::notifyObserversOfClosing()
{
	LOG_MSG("Entering VarsChooserObservable::notifyObserversOfClosing");
	for (std::list<VarsChooserObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it)
	{
		(*it)->notifyOfClosing(this);
	}
	observers.clear();
}

void VarsChooserObservable::registerObserver(VarsChooserObserver* o)
{
	observers.push_back(o);
}

void VarsChooserObservable::removeObserver(VarsChooserObserver* o)
{
	LOG_MSG("Entering VarsChooserObservable::removeObserver");
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) delete this;
	LOG_MSG("Exiting VarsChooserObservable::removeObserver");
}

void VarsChooserObservable::notifyObservers()
{
	for (std::list<VarsChooserObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}
