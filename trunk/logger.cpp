// Copyright (c) 2005, 2006
// Seweryn Habdank-Wojewodzki
// Distributed under the Boost Software License,
// Version 1.0.
// (copy at http://www.boost.org/LICENSE_1_0.txt)
#include "logger.h"

#if !defined(CLEANLOG)


#define FTLOG

#if !defined(DEBUG)
#undef FTLOG
#undef TLOG
#endif

#if defined (FTLOG)
#include <fstream>
#else
#include <iostream>
// http://www.msobczak.com/prog/bin/nullstream.zip
#include "nullstream.h"
#endif
logger_t::logger_t()
{}
bool logger_t::is_activated = true;

#if defined(TLOG)
std::auto_ptr<std::ostream> logger_t::outstream_helper_ptr
	= std::auto_ptr<std::ostream>( new NullStream );
std::ostream * logger_t::outstream = &std::cout;

#elif defined (ETLOG)
std::auto_ptr<std::ostream> logger_t::outstream_helper_ptr
	= std::auto_ptr <std::ostream>( new NullStream );
std::ostream * logger_t::outstream = &std::cerr;

#elif defined (FTLOG)
std::auto_ptr <std::ostream> logger_t::outstream_helper_ptr
	= std::auto_ptr<std::ostream>( new std::ofstream ("logger.txt"));
std::ostream * logger_t::outstream = outstream_helper_ptr.get();

// here is a place for user defined output stream
// and compiler flag

#else
std::auto_ptr<std::ostream> logger_t::outstream_helper_ptr
	= std::auto_ptr<std::ostream>( new NullStream );
std::ostream* logger_t::outstream = outstream_helper_ptr.get();
#endif

logger_t & logger()
{
	static logger_t* ans = new logger_t();
	return *ans;
}

#endif // !CLEANLOG

