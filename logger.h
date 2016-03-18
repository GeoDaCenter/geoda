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

