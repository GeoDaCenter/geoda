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
#ifdef DEBUG
    is_activated = true;
	std::ostringstream filepathBuf;
	if (GeneralWxUtils::isMac) {
		filepathBuf <<  GenUtils::GetBasemapCacheDir() << separator() << "../../../logger.txt";
	} else {
		filepathBuf <<  GenUtils::GetBasemapCacheDir() << separator() << "logger.txt";
	}
	
    outstream_helper_ptr = std::auto_ptr<std::ostream>( new std::ofstream (filepathBuf.str().c_str()));
    outstream = outstream_helper_ptr.get();
#else
    is_activated = false;
#endif
    
}