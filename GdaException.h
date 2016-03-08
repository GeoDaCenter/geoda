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

#ifndef __GEODA_CENTER_GDA_EXCEPTION_H__
#define __GEODA_CENTER_GDA_EXCEPTION_H__

#include <exception>
#include <sstream> 

/**
 *This exception is thrown when the sanity checks defined below fail
 */

class GdaException : public std::exception {
public:
    typedef enum { 
		NORMAL = 0, 
		WARNING = 1, 
		CRITICLE = 2,
        FIELD_NAME_EMPTY = 3
	} Type;

    GdaException() {}
    
	GdaException(const char* message, Type _t_=GdaException::CRITICLE)
	{
		std::ostringstream s;
		s << message;
#ifdef DEBUG
		//s << " in file " << __FILE__ << " line#" << __LINE__;
#endif
		
		what_ = s.str();
        t_ = _t_;
	}
	virtual ~GdaException() throw () {}
	virtual const char* what() const throw () {
        if (what_.empty()) return "None";
        return what_.c_str();
    }
    virtual const Type type() const throw() { return t_; }
	
protected:
	std::string what_;
    Type t_;
};

class GdaLocalSeparatorException : public GdaException {
public:
    GdaLocalSeparatorException(const char* message, Type _t_=GdaLocalSeparatorException::CRITICLE)
    {
        what_ = "Invalid Local Separtor found.";
        t_ = _t_;
    }
    
    virtual ~GdaLocalSeparatorException() throw () {}
    virtual const char* what() const throw () {
        if (what_.empty()) return "None";
        return what_.c_str();
    }
    
    virtual const Type type() const throw() { return t_; }
};

#endif