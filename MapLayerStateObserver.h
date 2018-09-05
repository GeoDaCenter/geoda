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

#ifndef __GEODA_CENTER_CAT_MAPLAYER_STATE_OBSERVER_H__
#define __GEODA_CENTER_CAT_MAPLAYER_STATE_OBSERVER_H__

class MapLayerState;
class MapLayerStateObserver {
public:
    virtual void update(MapLayerState* o) = 0;
};

class MapLayerState {
public:
    MapLayerState() {
        delete_self_when_empty = false;
    }
    virtual ~MapLayerState() {
    }
    
    /** Signal that CatClassifState should be closed, but wait until
     all observers have deregistered themselves. */
    void closeAndDeleteWhenEmpty() {
        delete_self_when_empty = true;
        if (observers.size() == 0) {
            delete this;
        }
    }
    void registerObserver(MapLayerStateObserver* o) {
        observers.push_front(o);
    }
    void removeObserver(MapLayerStateObserver* o) {
        observers.remove(o);
        if (observers.size() == 0 && delete_self_when_empty) delete this;
    }
    void notifyObservers() {
        for (std::list<MapLayerStateObserver*>::iterator it=observers.begin();
             it != observers.end(); ++it) {
            (*it)->update(this);
        }
    }
    int GetNumberObservers() {
        return observers.size();
    }
    
private:
    /** The list of registered CatClassifStateObserver objects. */
    std::list<MapLayerStateObserver*> observers;
    /** When the project is being closed, this is set to true so that
     when the list of observers is empty, the CatClassifState instance
     will automatically delete itself. */
    bool delete_self_when_empty;
};



#endif
