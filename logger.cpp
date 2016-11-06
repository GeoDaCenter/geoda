// Copyright (c) 2005, 2006
// Seweryn Habdank-Wojewodzki
// Distributed under the Boost Software License,
// Version 1.0.
// (copy at http://www.boost.org/LICENSE_1_0.txt)

#include "logger.h"
#include <fstream>

logger_t::logger_t()
{
}

bool logger_t::is_activated = true;

std::auto_ptr <std::ostream> logger_t::outstream_helper_ptr = std::auto_ptr<std::ostream>( new std::ofstream ("logger.txt"));

std::ostream * logger_t::outstream = outstream_helper_ptr.get();

logger_t & logger()
{
    static logger_t* ans = new logger_t();
    return *ans;
}

