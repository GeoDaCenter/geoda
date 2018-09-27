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
#include "CorrelParamsObserver.h"
#include "CorrelParamsObservable.h"

CorrelParamsObservable::CorrelParamsObservable(
			const CorrelParams& correl_params_,
			GdaVarTools::Manager& var_man_)
: delete_self_when_empty(false),
correl_params(correl_params_), var_man(var_man_)
{
	LOG_MSG("In CorrelParamsObservable::CorrelParamsObservable");
}

CorrelParamsObservable::~CorrelParamsObservable()
{
	LOG_MSG("In CorrelParamsObservable::~CorrelParamsObservable");
}

void CorrelParamsObservable::closeAndDeleteWhenEmpty()
{
	LOG_MSG("Entering CorrelParamsObservable::closeAndDeleteWhenEmpty");
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
	LOG_MSG("Exiting CorrelParamsObservable::closeAndDeleteWhenEmpty");
}

void CorrelParamsObservable::notifyObserversOfClosing()
{
	LOG_MSG("Entering CorrelParamsObservable::notifyObserversOfClosing");
	for (std::list<CorrelParamsObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it)
	{
		(*it)->notifyOfClosing(this);
	}
	observers.clear();
}

void CorrelParamsObservable::registerObserver(CorrelParamsObserver* o)
{
	observers.push_back(o);
}

void CorrelParamsObservable::removeObserver(CorrelParamsObserver* o)
{
	LOG_MSG("Entering CorrelParamsObservable::removeObserver");
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty)
        delete this;
	LOG_MSG("Exiting CorrelParamsObservable::removeObserver");
}

void CorrelParamsObservable::notifyObservers()
{
	for (std::list<CorrelParamsObserver*>::iterator it=observers.begin();
			 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

CorrelParams CorrelParamsObservable::GetCorrelParams() const
{
	return correl_params;
}
