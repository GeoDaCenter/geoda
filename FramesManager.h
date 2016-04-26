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

#ifndef __GEODA_CENTER_FRAMES_MANAGER_H__
#define __GEODA_CENTER_FRAMES_MANAGER_H__

#include <list>

class FramesManagerObserver; // forward declaration

class FramesManager {
public:
	FramesManager();
	virtual ~FramesManager();
	
	/** will be called once, only when the project is closing */
	void closeAndDeleteWhenEmpty();
	/** should only be used when closing the project */
	std::list<FramesManagerObserver*> getCopyObservers() {
		return observers;
	}
	int getNumberObservers() { return observers.size(); }
	
	void registerObserver(FramesManagerObserver* o);
	void removeObserver(FramesManagerObserver* o);
	void notifyObservers();
	
private:
	/** The list of registered FramesManagerObserver objects. */
	std::list<FramesManagerObserver*> observers;
	/** When the project is being closed, this is set to true so that
	 when the list of observers is empty, the FrameManager instance
	 will automatically delete itself. */
	bool delete_self_when_empty;
};

#endif
