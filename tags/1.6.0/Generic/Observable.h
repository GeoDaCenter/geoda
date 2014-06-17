/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

/** Observable Interface
 
 NOTE: We do not recommend using this interface directly.  Rather, we
 will keep this class here as a sort of template to follow.  Instead,
 one should define new Observer / Observable classes, for example:
 Model / ModelObserver.  The declartion of Model and ModelObserver might then
 look like:

 class Model {
 public:
	virtual void registerObserver(ModelObserver* o);
	virtual void removeObserver(ModelObserver* o);
	virtual void notifyObservers();
 };
 
 class ModelObserver {
 public:
	virtual void update(Model* o) = 0;
 };
 
 A concrete class, say Foo, that wants observe Model would then implement
 the ModelObserver interface.  Say Foo also wanted to be an observer of
 the Simulation class.  We would then define another interface
 SimulationObserver.  Foo would then implement update(Model* o) and
 update(Simulation* o) and the compiler will take care of making
 sure the correct update method is called.
 
 If we had decided to directly use the Observable/Observer interface, the
 class Foo would implement only update(Observable* o), but then the update
 method would have to use dynamic casting to determine which of the two
 Observable classes actually called update.  This is very poor programming
 practice.
 
 **/

#ifndef __GEODA_CENTER_OBSERVABLE_H__
#define __GEODA_CENTER_OBSERVABLE_H__

class Observer; // forward declaration

class Observable {
public:
	virtual void registerObserver(Observer* o) = 0;
	virtual void removeObserver(Observer* o) = 0;
	virtual void notifyObservers() = 0;
};

#endif
