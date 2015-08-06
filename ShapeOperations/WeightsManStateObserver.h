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

#ifndef __GEODA_CENTER_WEIGHTS_MAN_STATE_OBSERVER_H__
#define __GEODA_CENTER_WEIGHTS_MAN_STATE_OBSERVER_H__

#include <boost/uuid/uuid.hpp>
class WeightsManState;  // forward declaration

class WeightsManStateObserver {
public:
	virtual void update(WeightsManState* o) = 0;
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const = 0;
	/** A request has been made to delete the Observable object.
	 For any Observer where numMustCloseToRemove > 0,
	 a subsequent call to closeObserver must result in the
	 Observer closing or deleting itself.  It must also call
	 call WeightsManState::removeObserver before the method returns. */
	virtual void closeObserver(boost::uuids::uuid id) = 0;
};

#endif
