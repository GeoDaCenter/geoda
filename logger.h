/**
 * geoda tm, copyright (c) 2011-2015 by luc anselin - all rights reserved
 *
 * this file is part of geoda.
 * 
 * geoda is free software: you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * geoda is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * gnu general public license for more details.
 *
 * you should have received a copy of the gnu general public license
 * along with this program.  if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __GEODA_CENTER_LOGGER_H__
#define __GEODA_CENTER_LOGGER_H__

#include <ostream>
#include <iomanip>
#include <memory>
#include <ctime>

class GdaLogger {
public:
    static GdaLogger & GetInstance() {
        static GdaLogger instance;
        return instance;
    }
        
	bool is_activated;
	std::auto_ptr < std::ostream > outstream_helper_ptr;
	std::ostream * outstream;

private:
    GdaLogger ();
	//GdaLogger ( const logger_t & );
	//GdaLogger & operator= ( const logger_t & );
};

#define LOG(name)do {if (GdaLogger::GetInstance().is_activated ){\
        *GdaLogger::GetInstance().outstream << __FILE__ \
        << " [" << __LINE__ << "] : " << #name << " = " \
        << (name) << std::endl;} }while(false)

#define LOG_MSG(name)do {if (GdaLogger::GetInstance().is_activated ){\
        std::time_t now = std::time(0);\
        std::tm* ltm = std::localtime(&now);\
        *GdaLogger::GetInstance().outstream << "[" << ltm->tm_hour \
        << ":" << std::setw(2) << std::setfill('0') << ltm->tm_min \
        << ":" << std::setw(2) << std::setfill('0') << ltm->tm_sec \
        << ", line " << __LINE__ << "] : " << name << std::endl;} }while(false)

#endif

