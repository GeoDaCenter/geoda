// Copyright (c) 2005, 2006
// Seweryn Habdank-Wojewodzki
// Distributed under the Boost Software License,
// Version 1.0.
// ( copy at http://www.boost.org/LICENSE_1_0.txt )
#ifndef __GEODA_LOGGER_H__
#define __GEODA_LOGGER_H__
#include <ostream>
#include <iomanip>
#include <memory>
#include <ctime>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <wx/datetime.h>

class logger_t {
public:
    static bool is_activated;
    static std::auto_ptr < std::ostream > outstream_helper_ptr;
    static std::ostream * outstream;
    logger_t ();private:
    logger_t ( const logger_t & );
    logger_t & operator= ( const logger_t & );
};

extern logger_t & logger();

#define LOG(name)do {if (logger().is_activated ){\
*logger().outstream << __FILE__ \
<< " [" << __LINE__ << "] : " << #name << " = " \
<< (name) << std::endl;} }while(false)

//#define LOG_MSG(name)do {if (logger().is_activated ){\
//*logger().outstream << "[line " << __LINE__ << "]: "\
//<< name << std::endl;} }while(false)

#define LOG_MSG(name)do {if (logger().is_activated ){\
std::time_t now = std::time(0);\
std::tm* ltm = std::localtime(&now);\
*logger().outstream << "[line " << __LINE__ << "] : " << name << std::endl;} }while(false)

//#define LOG_MSG(name)do {if (logger().is_activated ){\
//wxDateTime now = wxDateTime::UNow();\
//*logger().outstream << "[" << now.Format("%H:%M:%S", wxDateTime::MST).mb_str() \
//<< "." << std::setw(3) << std::setfill('0') << now.GetMillisecond() \
//<< ", line " << __LINE__ << "] : " << name << std::endl;} }while(false)

//#define LOG_MSG(name)do {if (logger().is_activated ){\
//using namespace boost::posix_time;\
//ptime now = second_clock::local_time();\
//time_duration tod = now.time_of_day();\
//*logger().outstream << "[" << tod.hours() \
//<< ":" << std::setw(2) << std::setfill('0') << tod.minutes() \
//<< ":" << std::setw(2) << std::setfill('0') << tod.seconds() \
//<< ", line " << __LINE__ << "] : " << name << std::endl;} }while(false)


namespace logger_n {
    template < typename T1, typename T2, typename T3, typename T4 >
    void put_debug_info ( logger_t & log, \
                         T1 const & t1, T2 const & t2, \
                         T3 const & t3, T4 const & t4 )
    {
        if ( log.is_activated )
        {
            *(log.outstream) << t1 << " (" << t2 << ") : ";
            *(log.outstream) << t3 << " = " << t4 << std::endl;
        }
    }
}
#define LOG_FN(name) logger_n::put_debug_info (logger(), __FILE__, __LINE__, #name, (name))
// place for user defined logger formating data
#define LOG_ON() do { logger().is_activated = true; } while(false)
#define LOG_OFF() do { logger().is_activated = false; } while(false)

#if defined(CLEANLOG)
#undef LOG
#undef LOG_ON
#undef LOG_OFF
#undef LOG_FN
#define LOG(name) do{}while(false)
#define LOG_FN(name) do{}while(false)
#define LOG_ON() do{}while(false)
#define LOG_OFF() do{}while(false)
#endif
#endif
// __GEODA_LOGGER_H__
