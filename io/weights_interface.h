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

#ifndef __GEODA_CENTER_WEIGHTS_INTERFACE_H__
#define __GEODA_CENTER_WEIGHTS_INTERFACE_H__
#include <fstream>
#include <exception>

#include "../ShapeOperations/GalWeight.h"

using namespace std;

class WeightsNoteValidException: public exception {
    virtual const char* what() const throw() {
        return "weights exception: weights file not valid";
    }
};

class WeightsMismatchObsException: public exception {
    virtual const char* what() const throw() {
        return "weights exception: mismatch obervations";
    }
};

class WeightsKeyNotFoundException: public exception {
    virtual const char* what() const throw() {
        return "weights exception: key not found";
    }
};

class WeightsIdNotFoundException: public exception {
    virtual const char* what() const throw() {
        return "weights exception: id not found";
    }
};

class WeightsInterface
{
    virtual char* ReadIdField() = 0;
    virtual GalElement* Read() = 0;
};


wxString ReadIdFieldFromSwm(const wxString& fname);

GalElement* ReadSwmAsGal(const wxString& fname, TableInterface* table_int);

GalElement* ReadMatAsGal(const wxString& fname, TableInterface* table_int);

#endif
