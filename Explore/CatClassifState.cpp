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

#include "CatClassifStateObserver.h"
#include "CatClassifState.h"

CatClassifState::CatClassifState()
: delete_self_when_empty(false)
{
}

CatClassifState::~CatClassifState()
{
}

void CatClassifState::closeAndDeleteWhenEmpty()
{
	delete_self_when_empty = true;
	if (observers.size() == 0) {
		delete this;
	}
}

void CatClassifState::registerObserver(CatClassifStateObserver* o)
{
	observers.push_front(o);
}

void CatClassifState::removeObserver(CatClassifStateObserver* o)
{
	observers.remove(o);
	if (observers.size() == 0 && delete_self_when_empty) delete this;
}

void CatClassifState::notifyObservers()
{
	for (std::list<CatClassifStateObserver*>::iterator it=observers.begin();
		 it != observers.end(); ++it) {
		(*it)->update(this);
	}
}

CatClassifDef& CatClassifState::GetCatClassif()
{
	return cc_data;
}

void CatClassifState::SetCatClassif(const CatClassifDef& data)
{
	cc_data = data;
}
