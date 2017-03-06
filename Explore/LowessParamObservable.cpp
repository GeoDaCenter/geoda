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
#include "LowessParamObserver.h"
#include "LowessParamObservable.h"

LowessParamObservable::LowessParamObservable(double f_, int iter_,
																						 double delta_factor_)
: delete_self_when_empty(false),
f(f_), iter(iter_), delta_factor(delta_factor_)
{
	LOG_MSG("In LowessParamObservable::LowessParamObservable");
}

LowessParamObservable::~LowessParamObservable()
{
	LOG_MSG("In LowessParamObservable::~LowessParamObservable");
}

void LowessParamObservable::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering LowessParamObservable::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
	LOG_MSG("Exiting LowessParamObservable::closeAndDeleteWhenEmpty");
}

void LowessParamObservable::notifyObserversOfClosing()
{
	LOG_MSG("Entering LowessParamObservable::notifyObserversOfClosing");
	for (std::list<LowessParamObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it)
	{
		(*it)->notifyOfClosing(this);
	}
	observers.clear();
}

void LowessParamObservable::registerObserver(LowessParamObserver* o)
{
	observers.push_back(o);
}

void LowessParamObservable::removeObserver(LowessParamObserver* o)
{
	LOG_MSG("Entering LowessParamObservable::removeObserver");
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) delete this;
	LOG_MSG("Exiting LowessParamObservable::removeObserver");
}

void LowessParamObservable::notifyObservers()
{
	for (std::list<LowessParamObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}
