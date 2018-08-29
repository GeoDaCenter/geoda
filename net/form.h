/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.

	C++ wrapper for constructing libcurl forms
*/

#pragma once

#ifndef __GEODA_NET_FORM__
#define __GEODA_NET_FORM__

#include "config.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <string>
#include "initialization.h"
#include "native.h"

namespace curl
{
	class form:
		public boost::enable_shared_from_this<form>,
		public boost::noncopyable
	{
	public:
		form();
		~form();

		inline native::curl_httppost* native_handle() { return post_; };

		void add_content(const std::string& key, const std::string& content);
		void add_content(const std::string& key, const std::string& content, boost::system::error_code& ec);
		void add_content(const std::string& key, const std::string& content, const std::string& content_type);
		void add_content(const std::string& key, const std::string& content, const std::string& content_type, boost::system::error_code& ec);
		void add_file(const std::string& key, const std::string& file_path);
		void add_file(const std::string& key, const std::string& file_path, boost::system::error_code& ec);
		void add_file(const std::string& key, const std::string& file_path, const std::string& content_type);
		void add_file(const std::string& key, const std::string& file_path, const std::string& content_type, boost::system::error_code& ec);
		void add_file_using_name(const std::string& key, const std::string& file_path, const std::string& file_name);
		void add_file_using_name(const std::string& key, const std::string& file_path, const std::string& file_name, boost::system::error_code& ec);
		void add_file_using_name(const std::string& key, const std::string& file_path, const std::string& file_name, const std::string& content_type);
		void add_file_using_name(const std::string& key, const std::string& file_path, const std::string& file_name, const std::string& content_type, boost::system::error_code& ec);
		void add_file_content(const std::string& key, const std::string& file_path);
		void add_file_content(const std::string& key, const std::string& file_path, boost::system::error_code& ec);
		void add_file_content(const std::string& key, const std::string& file_path, const std::string& content_type);
		void add_file_content(const std::string& key, const std::string& file_path, const std::string& content_type, boost::system::error_code& ec);

	private:
		initialization::ptr initref_;
		native::curl_httppost* post_;
		native::curl_httppost* last_;
	};
}

#endif
