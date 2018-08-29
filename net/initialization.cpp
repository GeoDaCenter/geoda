/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.

	Helper to automatically initialize and cleanup libcurl resources
*/

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/weak_ptr.hpp>
#include "initialization.h"
#include "native.h"

using namespace curl;

boost::weak_ptr<initialization> helper_instance;
boost::mutex helper_lock;

initialization::ptr initialization::ensure_initialization()
{
	ptr result = helper_instance.lock();
	
	if (!result)
	{
		boost::lock_guard<boost::mutex> lock(helper_lock);
		result = helper_instance.lock();

		if (!result)
		{
			result = boost::shared_ptr<initialization>(new initialization());
			helper_instance = result;
		}
	}

	return result;
}

initialization::initialization()
{
	native::CURLcode ec = native::curl_global_init(CURL_GLOBAL_DEFAULT);

	if (ec != native::CURLE_OK)
	{
		throw std::runtime_error("curl_global_init failed with error code " + boost::lexical_cast<std::string>(ec));
	}
}

initialization::~initialization()
{
	native::curl_global_cleanup();
}
