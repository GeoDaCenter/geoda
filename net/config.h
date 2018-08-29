/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.
*/

#pragma once

#ifndef __GEODA_NET_CONFIG__
#define __GEODA_NET_CONFIG__

// Disable unused boost components. If your program depends on any of these
// components, include them prior to including curl-asio.
#ifndef BOOST_ASIO_DISABLE_BOOST_REGEX
#	define BOOST_ASIO_DISABLE_BOOST_REGEX
#endif

#endif
