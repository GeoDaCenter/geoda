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
#include "logger.h"
#include "GenUtils.h"
#include "GeneralWxUtils.h"

#include <sstream>
#include <fstream>

inline char separator()
{
#ifdef __WIN32__
    return '\\';
#else
    return '/';
#endif
}

GdaLogger::GdaLogger()
{
//#ifdef DEBUG
    is_activated = true;
	std::ostringstream filepathBuf;
	if (GeneralWxUtils::isMac()) {
		filepathBuf <<  GenUtils::GetBasemapCacheDir() << separator() << "../../../logger.txt";
	} else {
		filepathBuf <<  GenUtils::GetBasemapCacheDir() << separator() << "logger.txt";
	}
	
    outstream_helper_ptr = std::auto_ptr<std::ostream>( new std::ofstream (filepathBuf.str().c_str()));
    outstream = outstream_helper_ptr.get();
//#else
//    is_activated = false;
//#endif
    
}
