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

/** Observer Interface
 
 NOTE: We do note recommend using this interface directly.  Please see
 comments in Observable.cpp for more details.
 
 **/

#ifndef __GEODA_CENTER_OBSERVER_H__
#define __GEODA_CENTER_OBSERVER_H__

class Observable;  // forward declaration

class Observer {
 public:
  virtual void update(Observable* o) = 0;
};

#endif
