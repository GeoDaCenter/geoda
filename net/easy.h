/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.

	C++ wrapper for libcurl's easy interface
*/

#pragma once

#ifndef __GEODA_NET_EASY__
#define __GEODA_NET_EASY__

#include "config.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>
#include "error_code.h"
#include "initialization.h"

#define IMPLEMENT_CURL_OPTION(FUNCTION_NAME, OPTION_NAME, OPTION_TYPE) \
	inline void FUNCTION_NAME(OPTION_TYPE arg) \
	{ \
		boost::system::error_code ec; \
		FUNCTION_NAME(arg, ec); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
	} \
	inline void FUNCTION_NAME(OPTION_TYPE arg, boost::system::error_code& ec) \
	{ \
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, OPTION_NAME, arg)); \
	}

#define IMPLEMENT_CURL_OPTION_BOOLEAN(FUNCTION_NAME, OPTION_NAME) \
	inline void FUNCTION_NAME(bool enabled) \
	{ \
		boost::system::error_code ec; \
		FUNCTION_NAME(enabled, ec); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
	} \
	inline void FUNCTION_NAME(bool enabled, boost::system::error_code& ec) \
	{ \
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, OPTION_NAME, enabled ? 1L : 0L)); \
	}

#define IMPLEMENT_CURL_OPTION_ENUM(FUNCTION_NAME, OPTION_NAME, ENUM_TYPE, OPTION_TYPE) \
	inline void FUNCTION_NAME(ENUM_TYPE arg) \
	{ \
		boost::system::error_code ec; \
		FUNCTION_NAME(arg, ec); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
	} \
	inline void FUNCTION_NAME(ENUM_TYPE arg, boost::system::error_code& ec) \
	{ \
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, OPTION_NAME, (OPTION_TYPE)arg)); \
	}

#define IMPLEMENT_CURL_OPTION_STRING(FUNCTION_NAME, OPTION_NAME) \
	inline void FUNCTION_NAME(const char* str) \
	{ \
		boost::system::error_code ec; \
		FUNCTION_NAME(str, ec); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
	} \
	inline void FUNCTION_NAME(const char* str, boost::system::error_code& ec) \
	{ \
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, OPTION_NAME, str)); \
	} \
	inline void FUNCTION_NAME(const std::string& str) \
	{ \
		boost::system::error_code ec; \
		FUNCTION_NAME(str, ec); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
	} \
	inline void FUNCTION_NAME(const std::string& str, boost::system::error_code& ec) \
	{ \
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, OPTION_NAME, str.c_str())); \
	}

#define IMPLEMENT_CURL_OPTION_GET_STRING(FUNCTION_NAME, OPTION_NAME) \
	inline std::string FUNCTION_NAME() \
	{ \
		char *info = NULL; \
		boost::system::error_code ec = boost::system::error_code(native::curl_easy_getinfo(handle_, OPTION_NAME, &info)); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
		return info; \
	}

#define IMPLEMENT_CURL_OPTION_GET_DOUBLE(FUNCTION_NAME, OPTION_NAME) \
	inline double FUNCTION_NAME() \
	{ \
		double info; \
		boost::system::error_code ec = boost::system::error_code(native::curl_easy_getinfo(handle_, OPTION_NAME, &info)); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
		return info; \
	}

#define IMPLEMENT_CURL_OPTION_GET_LONG(FUNCTION_NAME, OPTION_NAME) \
	inline long FUNCTION_NAME() \
	{ \
		long info; \
		boost::system::error_code ec = boost::system::error_code(native::curl_easy_getinfo(handle_, OPTION_NAME, &info)); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
		return info; \
	}

#define IMPLEMENT_CURL_OPTION_GET_LIST(FUNCTION_NAME, OPTION_NAME) \
	inline std::vector<std::string> FUNCTION_NAME() \
	{ \
		struct native::curl_slist *info; \
		std::vector<std::string> results; \
		boost::system::error_code ec = boost::system::error_code(native::curl_easy_getinfo(handle_, OPTION_NAME, &info)); \
		boost::asio::detail::throw_error(ec, BOOST_PP_STRINGIZE(FUNCTION_NAME)); \
		struct native::curl_slist *it = info; \
		while (it) \
		{ \
			results.push_back(std::string(it->data)); \
			it = it->next; \
		} \
		native::curl_slist_free_all(info); \
		return results; \
	}

namespace curl
{
	class form;
	class multi;
	class share;
	class string_list;

	class easy:
		public boost::noncopyable
	{
	public:
		typedef boost::function<void(const boost::system::error_code& err)> handler_type;

		static easy* from_native(native::CURL* native_easy);

		easy(boost::asio::io_service& io_service);
		easy(multi& multi_handle);
		~easy();

		inline native::CURL* native_handle() { return handle_; }

		void perform();
		void perform(boost::system::error_code& ec);
		void async_perform(handler_type handler);
		void cancel();
		void set_source(boost::shared_ptr<std::istream> source);
		void set_source(boost::shared_ptr<std::istream> source, boost::system::error_code& ec);
		void set_sink(boost::shared_ptr<std::ostream> sink);
		void set_sink(boost::shared_ptr<std::ostream> sink, boost::system::error_code& ec);

		typedef boost::function<bool(native::curl_off_t dltotal, native::curl_off_t dlnow, native::curl_off_t ultotal, native::curl_off_t ulnow)> progress_callback_t;
		void unset_progress_callback();
		void set_progress_callback(progress_callback_t progress_callback);

		// behavior options

		IMPLEMENT_CURL_OPTION_BOOLEAN(set_verbose, native::CURLOPT_VERBOSE);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_header, native::CURLOPT_HEADER);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_progress, native::CURLOPT_NOPROGRESS);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_signal, native::CURLOPT_NOSIGNAL);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_wildcard_match, native::CURLOPT_WILDCARDMATCH);

		// callback options

		typedef size_t (*write_function_t)(char* ptr, size_t size, size_t nmemb, void* userdata);
		IMPLEMENT_CURL_OPTION(set_write_function, native::CURLOPT_WRITEFUNCTION, write_function_t);
		IMPLEMENT_CURL_OPTION(set_write_data, native::CURLOPT_WRITEDATA, void*);
		typedef size_t (*read_function_t)(void* ptr, size_t size, size_t nmemb, void* userdata);
		IMPLEMENT_CURL_OPTION(set_read_function, native::CURLOPT_READFUNCTION, read_function_t);
		IMPLEMENT_CURL_OPTION(set_read_data, native::CURLOPT_READDATA, void*);
		typedef native::curlioerr (*ioctl_function_t)(native::CURL* handle, int cmd, void* clientp);
		IMPLEMENT_CURL_OPTION(set_ioctl_function, native::CURLOPT_IOCTLFUNCTION, ioctl_function_t);
		IMPLEMENT_CURL_OPTION(set_ioctl_data, native::CURLOPT_IOCTLDATA, void*);
		typedef int (*seek_function_t)(void* instream, native::curl_off_t offset, int origin);
		IMPLEMENT_CURL_OPTION(set_seek_function, native::CURLOPT_SEEKFUNCTION, seek_function_t);
		IMPLEMENT_CURL_OPTION(set_seek_data, native::CURLOPT_SEEKDATA, void*);
		typedef int (*sockopt_function_t)(void* clientp, native::curl_socket_t curlfd, native::curlsocktype purpose);
		IMPLEMENT_CURL_OPTION(set_sockopt_function, native::CURLOPT_SOCKOPTFUNCTION, sockopt_function_t);
		IMPLEMENT_CURL_OPTION(set_sockopt_data, native::CURLOPT_SOCKOPTDATA, void*);
		typedef native::curl_socket_t (*opensocket_function_t)(void* clientp, native::curlsocktype purpose, struct native::curl_sockaddr* address);
		IMPLEMENT_CURL_OPTION(set_opensocket_function, native::CURLOPT_OPENSOCKETFUNCTION, opensocket_function_t);
		IMPLEMENT_CURL_OPTION(set_opensocket_data, native::CURLOPT_OPENSOCKETDATA, void*);
		typedef int (*closesocket_function_t)(void* clientp, native::curl_socket_t item);
		IMPLEMENT_CURL_OPTION(set_closesocket_function, native::CURLOPT_CLOSESOCKETFUNCTION, closesocket_function_t);
		IMPLEMENT_CURL_OPTION(set_closesocket_data, native::CURLOPT_CLOSESOCKETDATA, void*);
		typedef int (*progress_function_t)(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
		IMPLEMENT_CURL_OPTION(set_progress_function, native::CURLOPT_PROGRESSFUNCTION, progress_function_t);
		IMPLEMENT_CURL_OPTION(set_progress_data, native::CURLOPT_PROGRESSDATA, void*);
#if LIBCURL_VERSION_NUM >= 0x072000
		typedef int (*xferinfo_function_t)(void* clientp, native::curl_off_t dltotal, native::curl_off_t dlnow, native::curl_off_t ultotal, native::curl_off_t ulnow);
		IMPLEMENT_CURL_OPTION(set_xferinfo_function, native::CURLOPT_XFERINFOFUNCTION, xferinfo_function_t);
		IMPLEMENT_CURL_OPTION(set_xferinfo_data, native::CURLOPT_XFERINFODATA, void*);
#endif
		typedef size_t (*header_function_t)(void* ptr, size_t size, size_t nmemb, void* userdata);
		IMPLEMENT_CURL_OPTION(set_header_function, native::CURLOPT_HEADERFUNCTION, header_function_t);
		IMPLEMENT_CURL_OPTION(set_header_data, native::CURLOPT_HEADERDATA, void*);
		typedef int (*debug_callback_t)(native::CURL*, native::curl_infotype, char*, size_t, void*);
		IMPLEMENT_CURL_OPTION(set_debug_callback, native::CURLOPT_DEBUGFUNCTION, debug_callback_t);
		IMPLEMENT_CURL_OPTION(set_debug_data, native::CURLOPT_DEBUGDATA, void*);
		typedef native::CURLcode (*ssl_ctx_function_t)(native::CURL* curl, void* sslctx, void* parm);
		IMPLEMENT_CURL_OPTION(set_ssl_ctx_function, native::CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_function_t);
		IMPLEMENT_CURL_OPTION(set_ssl_ctx_data, native::CURLOPT_SSL_CTX_DATA, void*);
		typedef size_t (*interleave_function_t)(void* ptr, size_t size, size_t nmemb, void* userdata);
		IMPLEMENT_CURL_OPTION(set_interleave_function, native::CURLOPT_INTERLEAVEFUNCTION, interleave_function_t);
		IMPLEMENT_CURL_OPTION(set_interleave_data, native::CURLOPT_INTERLEAVEDATA, void*);

		// error options

		IMPLEMENT_CURL_OPTION(set_error_buffer, native::CURLOPT_ERRORBUFFER, char*);
		IMPLEMENT_CURL_OPTION(set_stderr, native::CURLOPT_STDERR, FILE*);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_fail_on_error, native::CURLOPT_FAILONERROR);

		// network options

		IMPLEMENT_CURL_OPTION_STRING(set_url, native::CURLOPT_URL);
		IMPLEMENT_CURL_OPTION(set_protocols, native::CURLOPT_PROTOCOLS, long);
		IMPLEMENT_CURL_OPTION(set_redir_protocols, native::CURLOPT_REDIR_PROTOCOLS, long);
		IMPLEMENT_CURL_OPTION_STRING(set_proxy, native::CURLOPT_PROXY);
		IMPLEMENT_CURL_OPTION(set_proxy_port, native::CURLOPT_PROXYPORT, long);
		IMPLEMENT_CURL_OPTION(set_proxy_type, native::CURLOPT_PROXYTYPE, long);
		IMPLEMENT_CURL_OPTION_STRING(set_no_proxy, native::CURLOPT_NOPROXY);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_proxy_tunnel, native::CURLOPT_HTTPPROXYTUNNEL);
		IMPLEMENT_CURL_OPTION_STRING(set_socks5_gsapi_service, native::CURLOPT_SOCKS5_GSSAPI_SERVICE);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_socks5_gsapi_nec, native::CURLOPT_SOCKS5_GSSAPI_NEC);
		IMPLEMENT_CURL_OPTION_STRING(set_interface, native::CURLOPT_INTERFACE);
		IMPLEMENT_CURL_OPTION(set_local_port, native::CURLOPT_LOCALPORT, long);
		IMPLEMENT_CURL_OPTION(set_local_port_range, native::CURLOPT_LOCALPORTRANGE, long);
		IMPLEMENT_CURL_OPTION(set_dns_cache_timeout, native::CURLOPT_DNS_CACHE_TIMEOUT, long);
		IMPLEMENT_CURL_OPTION(set_buffer_size, native::CURLOPT_BUFFERSIZE, long);
		IMPLEMENT_CURL_OPTION(set_port, native::CURLOPT_PORT, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_tcp_no_delay, native::CURLOPT_TCP_NODELAY);
		IMPLEMENT_CURL_OPTION(set_address_scope, native::CURLOPT_ADDRESS_SCOPE, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_tcp_keep_alive, native::CURLOPT_TCP_KEEPALIVE);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_tcp_keep_idle, native::CURLOPT_TCP_KEEPIDLE);
		IMPLEMENT_CURL_OPTION(set_tcp_keep_intvl, native::CURLOPT_TCP_KEEPINTVL, long);

		// authentication options

		enum netrc_t { netrc_optional = native::CURL_NETRC_OPTIONAL, netrc_ignored = native::CURL_NETRC_IGNORED, netrc_required = native::CURL_NETRC_REQUIRED };
		IMPLEMENT_CURL_OPTION_ENUM(set_netrc, native::CURLOPT_NETRC, netrc_t, long);
		IMPLEMENT_CURL_OPTION_STRING(set_netrc_file, native::CURLOPT_NETRC_FILE);
		IMPLEMENT_CURL_OPTION_STRING(set_user, native::CURLOPT_USERNAME);
		IMPLEMENT_CURL_OPTION_STRING(set_password, native::CURLOPT_PASSWORD);
		IMPLEMENT_CURL_OPTION_STRING(set_proxy_user, native::CURLOPT_PROXYUSERNAME);
		IMPLEMENT_CURL_OPTION_STRING(set_proxy_password, native::CURLOPT_PROXYPASSWORD);
		enum httpauth_t { auth_basic = CURLAUTH_BASIC, auth_digest, auth_digest_ie, auth_gss_negotiate, auth_ntml, auth_nhtml_wb, auth_any, auth_any_safe };
		inline void set_http_auth(httpauth_t auth, bool auth_only)
		{
			boost::system::error_code ec;
			set_http_auth(auth, auth_only, ec);
			boost::asio::detail::throw_error(ec, "set_http_auth");
		}
		inline void set_http_auth(httpauth_t auth, bool auth_only, boost::system::error_code& ec)
		{
			long l = ((long)auth | (auth_only ? CURLAUTH_ONLY : 0L));
			ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPAUTH, l));
		}
		IMPLEMENT_CURL_OPTION(set_tls_auth_type, native::CURLOPT_TLSAUTH_TYPE, long);
		IMPLEMENT_CURL_OPTION_STRING(set_tls_auth_user, native::CURLOPT_TLSAUTH_USERNAME);
		IMPLEMENT_CURL_OPTION_STRING(set_tls_auth_password, native::CURLOPT_TLSAUTH_PASSWORD);
		IMPLEMENT_CURL_OPTION(set_proxy_auth, native::CURLOPT_PROXYAUTH, long);

		// HTTP options

		IMPLEMENT_CURL_OPTION_BOOLEAN(set_auto_referrer, native::CURLOPT_AUTOREFERER);
		IMPLEMENT_CURL_OPTION_STRING(set_accept_encoding, native::CURLOPT_ACCEPT_ENCODING);
		IMPLEMENT_CURL_OPTION_STRING(set_transfer_encoding, native::CURLOPT_TRANSFER_ENCODING);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_follow_location, native::CURLOPT_FOLLOWLOCATION);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_unrestricted_auth, native::CURLOPT_UNRESTRICTED_AUTH);
		IMPLEMENT_CURL_OPTION(set_max_redirs, native::CURLOPT_MAXREDIRS, long);
		IMPLEMENT_CURL_OPTION(set_post_redir, native::CURLOPT_POSTREDIR, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_post, native::CURLOPT_POST);
		void set_post_fields(const std::string& post_fields);
		void set_post_fields(const std::string& post_fields, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION(set_post_fields, native::CURLOPT_POSTFIELDS, void*);
		IMPLEMENT_CURL_OPTION(set_post_field_size, native::CURLOPT_POSTFIELDSIZE, long);
		IMPLEMENT_CURL_OPTION(set_post_field_size_large, native::CURLOPT_POSTFIELDSIZE_LARGE, native::curl_off_t);
		void set_http_post(boost::shared_ptr<form> form);
		void set_http_post(boost::shared_ptr<form> form, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION_STRING(set_referer, native::CURLOPT_REFERER);
		IMPLEMENT_CURL_OPTION_STRING(set_user_agent, native::CURLOPT_USERAGENT);
		void add_header(const std::string& name, const std::string& value);
		void add_header(const std::string& name, const std::string& value, boost::system::error_code& ec);
		void add_header(const std::string& header);
		void add_header(const std::string& header, boost::system::error_code& ec);
		void set_headers(boost::shared_ptr<string_list> headers);
		void set_headers(boost::shared_ptr<string_list> headers, boost::system::error_code& ec);
		void add_http200_alias(const std::string& http200_alias);
		void add_http200_alias(const std::string& http200_alias, boost::system::error_code& ec);
		void set_http200_aliases(boost::shared_ptr<string_list> http200_aliases);
		void set_http200_aliases(boost::shared_ptr<string_list> http200_aliases, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION_STRING(set_cookie, native::CURLOPT_COOKIE);
		IMPLEMENT_CURL_OPTION_STRING(set_cookie_file, native::CURLOPT_COOKIEFILE);
		IMPLEMENT_CURL_OPTION_STRING(set_cookie_jar, native::CURLOPT_COOKIEJAR);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_cookie_session, native::CURLOPT_COOKIESESSION);
		IMPLEMENT_CURL_OPTION_STRING(set_cookie_list, native::CURLOPT_COOKIELIST);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_get, native::CURLOPT_HTTPGET);
		enum http_version_t { http_version_none = native::CURL_HTTP_VERSION_NONE, http_version_1_0 = native::CURL_HTTP_VERSION_1_0, http_version_1_1 = native::CURL_HTTP_VERSION_1_1 };
		IMPLEMENT_CURL_OPTION_ENUM(set_http_version, native::CURLOPT_HTTP_VERSION, http_version_t, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ignore_content_length, native::CURLOPT_IGNORE_CONTENT_LENGTH);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_content_decoding, native::CURLOPT_HTTP_CONTENT_DECODING);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_transfer_decoding, native::CURLOPT_HTTP_TRANSFER_DECODING);

		// SMTP options

		IMPLEMENT_CURL_OPTION_STRING(set_mail_from, native::CURLOPT_MAIL_FROM);
		void add_mail_rcpt(const std::string& mail_rcpt);
		void add_mail_rcpt(const std::string& mail_rcpt, boost::system::error_code& ec);
		void set_mail_rcpts(boost::shared_ptr<string_list> mail_rcpts);
		void set_mail_rcpts(boost::shared_ptr<string_list> mail_rcpts, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION_STRING(set_mail_auth, native::CURLOPT_MAIL_AUTH);

		// TFTP options

		IMPLEMENT_CURL_OPTION(set_tftp_blksize, native::CURLOPT_TFTP_BLKSIZE, long);

		// FTP options

		IMPLEMENT_CURL_OPTION_STRING(set_ftp_port, native::CURLOPT_FTPPORT);
		void add_quote(const std::string& quote);
		void add_quote(const std::string& quote, boost::system::error_code& ec);
		void set_quotes(boost::shared_ptr<string_list> quotes);
		void set_quotes(boost::shared_ptr<string_list> quotes, boost::system::error_code& ec);
		/*void add_post_quote(const std::string& pre_quote);
		void set_post_quotes(boost::shared_ptr<string_list> pre_quotes);
		void add_pre_quote(const std::string& pre_quote);
		void set_pre_quotes(boost::shared_ptr<string_list> pre_quotes);*/
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_dir_list_only, native::CURLOPT_DIRLISTONLY);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_append, native::CURLOPT_APPEND);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ftp_use_eprt, native::CURLOPT_FTP_USE_EPRT);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ftp_use_epsv, native::CURLOPT_FTP_USE_EPSV);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ftp_use_pret, native::CURLOPT_FTP_USE_PRET);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ftp_create_missing_dirs, native::CURLOPT_FTP_CREATE_MISSING_DIRS);
		IMPLEMENT_CURL_OPTION(set_ftp_response_timeout, native::CURLOPT_FTP_RESPONSE_TIMEOUT, long);
		IMPLEMENT_CURL_OPTION_STRING(set_ftp_alternative_to_user, native::CURLOPT_FTP_ALTERNATIVE_TO_USER);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ftp_skip_pasv_ip, native::CURLOPT_FTP_SKIP_PASV_IP);
		enum ftp_ssl_auth_t { ftp_auth_default = native::CURLFTPAUTH_DEFAULT, ftp_auth_ssl = native::CURLFTPAUTH_SSL, ftp_auth_tls = native::CURLFTPAUTH_TLS };
		IMPLEMENT_CURL_OPTION_ENUM(set_ftp_ssl_auth, native::CURLOPT_FTPSSLAUTH, ftp_ssl_auth_t, long);
		enum ftp_ssl_ccc_t { ftp_ssl_ccc_none = native::CURLFTPSSL_CCC_NONE, ftp_ssl_ccc_passive = native::CURLFTPSSL_CCC_PASSIVE, ftp_ssl_ccc_active = native::CURLFTPSSL_CCC_ACTIVE };
		IMPLEMENT_CURL_OPTION_ENUM(set_ftp_ssl_ccc, native::CURLOPT_FTP_SSL_CCC, ftp_ssl_ccc_t, long);
		IMPLEMENT_CURL_OPTION_STRING(set_ftp_account, native::CURLOPT_FTP_ACCOUNT);
		enum ftp_file_method_t { ftp_method_multi_cwd = native::CURLFTPMETHOD_MULTICWD , ftp_method_no_cwd = native::CURLFTPMETHOD_NOCWD, ftp_method_single_cwd = native::CURLFTPMETHOD_SINGLECWD };
		IMPLEMENT_CURL_OPTION_ENUM(set_ftp_file_method, native::CURLOPT_FTP_FILEMETHOD, ftp_file_method_t, long);

		// RTSP options

		enum rtsp_request_t
		{
			rtsp_request_options = native::CURL_RTSPREQ_OPTIONS,
			rtsp_request_describe = native::CURL_RTSPREQ_DESCRIBE,
			rtsp_request_announce = native::CURL_RTSPREQ_ANNOUNCE,
			rtsp_request_setup = native::CURL_RTSPREQ_SETUP,
			rtsp_request_play = native::CURL_RTSPREQ_PLAY,
			rtsp_request_pause = native::CURL_RTSPREQ_PAUSE,
			rtsp_request_teardown = native::CURL_RTSPREQ_TEARDOWN,
			rtsp_request_get_parameter = native::CURL_RTSPREQ_GET_PARAMETER,
			rtsp_request_set_parameter = native::CURL_RTSPREQ_SET_PARAMETER,
			rtsp_request_record = native::CURL_RTSPREQ_RECORD,
			rtsp_request_receive = native::CURL_RTSPREQ_RECEIVE
		};
		IMPLEMENT_CURL_OPTION_ENUM(set_rtsp_request, native::CURLOPT_RTSP_REQUEST, rtsp_request_t, long);
		IMPLEMENT_CURL_OPTION_STRING(set_rtsp_session_id, native::CURLOPT_RTSP_SESSION_ID);
		IMPLEMENT_CURL_OPTION_STRING(set_rtsp_stream_uri, native::CURLOPT_RTSP_STREAM_URI);
		IMPLEMENT_CURL_OPTION_STRING(set_rtsp_transport, native::CURLOPT_RTSP_TRANSPORT);
		IMPLEMENT_CURL_OPTION(set_rtsp_client_cseq, native::CURLOPT_RTSP_CLIENT_CSEQ, long);
		IMPLEMENT_CURL_OPTION(set_rtsp_server_cseq, native::CURLOPT_RTSP_SERVER_CSEQ, long);

		// protocol options

		IMPLEMENT_CURL_OPTION_BOOLEAN(set_transfer_text, native::CURLOPT_TRANSFERTEXT);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_transfer_mode, native::CURLOPT_PROXY_TRANSFER_MODE);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_crlf, native::CURLOPT_CRLF);
		IMPLEMENT_CURL_OPTION_STRING(set_range, native::CURLOPT_RANGE);
		IMPLEMENT_CURL_OPTION(set_resume_from, native::CURLOPT_RESUME_FROM, long);
		IMPLEMENT_CURL_OPTION(set_resume_from_large, native::CURLOPT_RESUME_FROM_LARGE, native::curl_off_t);
		IMPLEMENT_CURL_OPTION_STRING(set_custom_request, native::CURLOPT_CUSTOMREQUEST);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_file_time, native::CURLOPT_FILETIME);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_body, native::CURLOPT_NOBODY);
		IMPLEMENT_CURL_OPTION(set_in_file_size, native::CURLOPT_INFILESIZE, long);
		IMPLEMENT_CURL_OPTION(set_in_file_size_large, native::CURLOPT_INFILESIZE_LARGE, native::curl_off_t);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_upload, native::CURLOPT_UPLOAD);
		IMPLEMENT_CURL_OPTION(set_max_file_size, native::CURLOPT_MAXFILESIZE, long);
		IMPLEMENT_CURL_OPTION(set_max_file_size_large, native::CURLOPT_MAXFILESIZE_LARGE, native::curl_off_t);
		enum time_condition_t { if_modified_since = native::CURL_TIMECOND_IFMODSINCE, if_unmodified_since = native::CURL_TIMECOND_IFUNMODSINCE };
		IMPLEMENT_CURL_OPTION_ENUM(set_time_condition, native::CURLOPT_TIMECONDITION, time_condition_t, long);
		IMPLEMENT_CURL_OPTION(set_time_value, native::CURLOPT_TIMEVALUE, long);

		// connection options

		IMPLEMENT_CURL_OPTION(set_timeout, native::CURLOPT_TIMEOUT, long);
		IMPLEMENT_CURL_OPTION(set_timeout_ms, native::CURLOPT_TIMEOUT_MS, long);
		IMPLEMENT_CURL_OPTION(set_low_speed_limit, native::CURLOPT_LOW_SPEED_LIMIT, long);
		IMPLEMENT_CURL_OPTION(set_low_speed_time, native::CURLOPT_LOW_SPEED_TIME, long);
		IMPLEMENT_CURL_OPTION(set_max_send_speed_large, native::CURLOPT_MAX_SEND_SPEED_LARGE, native::curl_off_t);
		IMPLEMENT_CURL_OPTION(set_max_recv_speed_large, native::CURLOPT_MAX_RECV_SPEED_LARGE, native::curl_off_t);
		IMPLEMENT_CURL_OPTION(set_max_connects, native::CURLOPT_MAXCONNECTS, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_fresh_connect, native::CURLOPT_FRESH_CONNECT);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_forbot_reuse, native::CURLOPT_FORBID_REUSE);
		IMPLEMENT_CURL_OPTION(set_connect_timeout, native::CURLOPT_CONNECTTIMEOUT, long);
		IMPLEMENT_CURL_OPTION(set_connect_timeout_ms, native::CURLOPT_CONNECTTIMEOUT_MS, long);
		enum ip_resolve_t { ip_resolve_whatever = CURL_IPRESOLVE_WHATEVER, ip_resolve_v4 = CURL_IPRESOLVE_V4, ip_resolve_v6 = CURL_IPRESOLVE_V6 };
		IMPLEMENT_CURL_OPTION_ENUM(set_ip_resolve, native::CURLOPT_IPRESOLVE, ip_resolve_t, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_connect_only, native::CURLOPT_CONNECT_ONLY);
		enum use_ssl_t { use_ssl_none = native::CURLUSESSL_NONE, use_ssl_try = native::CURLUSESSL_TRY, use_ssl_control = native::CURLUSESSL_CONTROL, use_ssl_all = native::CURLUSESSL_ALL };
		IMPLEMENT_CURL_OPTION_ENUM(set_use_ssl, native::CURLOPT_USE_SSL, use_ssl_t, long);
		void add_resolve(const std::string& resolved_host);
		void add_resolve(const std::string& resolved_host, boost::system::error_code& ec);
		void set_resolves(boost::shared_ptr<string_list> resolved_hosts);
		void set_resolves(boost::shared_ptr<string_list> resolved_hosts, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION_STRING(set_dns_servers, native::CURLOPT_DNS_SERVERS);
		IMPLEMENT_CURL_OPTION(set_accept_timeout_ms, native::CURLOPT_ACCEPTTIMEOUT_MS, long);

		// SSL and security options

		IMPLEMENT_CURL_OPTION_STRING(set_ssl_cert, native::CURLOPT_SSLCERT);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_cert_type, native::CURLOPT_SSLCERTTYPE);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_key, native::CURLOPT_SSLKEY);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_key_type, native::CURLOPT_SSLKEYTYPE);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_key_passwd, native::CURLOPT_KEYPASSWD);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_engine, native::CURLOPT_SSLENGINE);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_engine_default, native::CURLOPT_SSLENGINE_DEFAULT);
		enum ssl_version_t { ssl_version_default = native::CURL_SSLVERSION_DEFAULT, ssl_version_tls_v1 = native::CURL_SSLVERSION_TLSv1, ssl_version_ssl_v2 = native::CURL_SSLVERSION_SSLv2, ssl_version_ssl_v3 = native::CURL_SSLVERSION_SSLv3 };
		IMPLEMENT_CURL_OPTION_ENUM(set_ssl_version, native::CURLOPT_SSLVERSION, ssl_version_t, long);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ssl_verify_peer, native::CURLOPT_SSL_VERIFYPEER);
		IMPLEMENT_CURL_OPTION_STRING(set_ca_info, native::CURLOPT_CAINFO);
		IMPLEMENT_CURL_OPTION_STRING(set_issuer_cert, native::CURLOPT_ISSUERCERT);
		IMPLEMENT_CURL_OPTION_STRING(set_ca_file, native::CURLOPT_CAPATH);
		IMPLEMENT_CURL_OPTION_STRING(set_crl_file, native::CURLOPT_CRLFILE);
		inline void set_ssl_verify_host(bool verify_host)
		{
			boost::system::error_code ec;
			set_ssl_verify_host(verify_host, ec);
			boost::asio::detail::throw_error(ec);
		}
		inline void set_ssl_verify_host(bool verify_host, boost::system::error_code& ec)
		{
			ec = boost::system::error_code(curl_easy_setopt(handle_, native::CURLOPT_SSL_VERIFYHOST, verify_host ? 2L : 0L));
		}
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_cert_info, native::CURLOPT_CERTINFO);
		IMPLEMENT_CURL_OPTION_STRING(set_random_file, native::CURLOPT_RANDOM_FILE);
		IMPLEMENT_CURL_OPTION_STRING(set_edg_socket, native::CURLOPT_EGDSOCKET);
		IMPLEMENT_CURL_OPTION_STRING(set_ssl_cipher_list, native::CURLOPT_SSL_CIPHER_LIST);
		IMPLEMENT_CURL_OPTION_BOOLEAN(set_ssl_session_id_cache, native::CURLOPT_SSL_SESSIONID_CACHE);
		IMPLEMENT_CURL_OPTION(set_ssl_options, native::CURLOPT_SSL_OPTIONS, long);
		IMPLEMENT_CURL_OPTION_STRING(set_krb_level, native::CURLOPT_KRBLEVEL);
		IMPLEMENT_CURL_OPTION(set_gssapi_delegation, native::CURLOPT_GSSAPI_DELEGATION, long);

		// SSH options

		IMPLEMENT_CURL_OPTION(set_ssh_auth_types, native::CURLOPT_SSH_AUTH_TYPES, long);
		IMPLEMENT_CURL_OPTION_STRING(set_ssh_host_public_key_md5, native::CURLOPT_SSH_HOST_PUBLIC_KEY_MD5);
		IMPLEMENT_CURL_OPTION_STRING(set_ssh_public_key_file, native::CURLOPT_SSH_PUBLIC_KEYFILE);
		IMPLEMENT_CURL_OPTION_STRING(set_ssh_private_key_file, native::CURLOPT_SSH_PRIVATE_KEYFILE);
		IMPLEMENT_CURL_OPTION_STRING(set_ssh_known_hosts, native::CURLOPT_SSH_KNOWNHOSTS);
		IMPLEMENT_CURL_OPTION(set_ssh_key_function, native::CURLOPT_SSH_KEYFUNCTION, void*); // TODO curl_sshkeycallback?
		IMPLEMENT_CURL_OPTION(set_ssh_key_data, native::CURLOPT_SSH_KEYDATA, void*);

		// other options

		IMPLEMENT_CURL_OPTION(set_private, native::CURLOPT_PRIVATE, void*);
		void set_share(boost::shared_ptr<share> share);
		void set_share(boost::shared_ptr<share> share, boost::system::error_code& ec);
		IMPLEMENT_CURL_OPTION(set_new_file_perms, native::CURLOPT_NEW_FILE_PERMS, long);
		IMPLEMENT_CURL_OPTION(set_new_directory_perms, native::CURLOPT_NEW_DIRECTORY_PERMS, long);

		// telnet options

		void add_telnet_option(const std::string& option, const std::string& value);
		void add_telnet_option(const std::string& option, const std::string& value, boost::system::error_code& ec);
		void add_telnet_option(const std::string& telnet_option);
		void add_telnet_option(const std::string& telnet_option, boost::system::error_code& ec);
		void set_telnet_options(boost::shared_ptr<string_list> telnet_options);
		void set_telnet_options(boost::shared_ptr<string_list> telnet_options, boost::system::error_code& ec);

		// getters

		IMPLEMENT_CURL_OPTION_GET_STRING(get_effective_url, native::CURLINFO_EFFECTIVE_URL);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_reponse_code, native::CURLINFO_RESPONSE_CODE);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_http_connectcode, native::CURLINFO_HTTP_CONNECTCODE);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_filetime, native::CURLINFO_FILETIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_total_time, native::CURLINFO_TOTAL_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_namelookup_time, native::CURLINFO_NAMELOOKUP_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_connect_time, native::CURLINFO_CONNECT_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_appconnect_time, native::CURLINFO_APPCONNECT_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_pretransfer_time, native::CURLINFO_PRETRANSFER_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_starttransfer_time, native::CURLINFO_STARTTRANSFER_TIME);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_redirect_time, native::CURLINFO_REDIRECT_TIME);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_redirect_count, native::CURLINFO_REDIRECT_COUNT);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_redirect_url, native::CURLINFO_REDIRECT_URL);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_size_upload, native::CURLINFO_SIZE_UPLOAD);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_size_download, native::CURLINFO_SIZE_DOWNLOAD);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_speed_download, native::CURLINFO_SPEED_DOWNLOAD);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_speed_upload, native::CURLINFO_SPEED_UPLOAD);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_header_size, native::CURLINFO_HEADER_SIZE);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_request_size, native::CURLINFO_REQUEST_SIZE);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_ssl_verifyresult, native::CURLINFO_SSL_VERIFYRESULT);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_content_length_download, native::CURLINFO_CONTENT_LENGTH_DOWNLOAD);
		IMPLEMENT_CURL_OPTION_GET_DOUBLE(get_content_length_upload, native::CURLINFO_CONTENT_LENGTH_UPLOAD);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_content_type, native::CURLINFO_CONTENT_TYPE);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_httpauth_avail, native::CURLINFO_HTTPAUTH_AVAIL);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_proxyauth_avail, native::CURLINFO_PROXYAUTH_AVAIL);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_os_errno, native::CURLINFO_OS_ERRNO);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_num_connects, native::CURLINFO_NUM_CONNECTS);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_primary_ip, native::CURLINFO_PRIMARY_IP);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_primary_port, native::CURLINFO_PRIMARY_PORT);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_local_ip, native::CURLINFO_LOCAL_IP);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_local_port, native::CURLINFO_LOCAL_PORT);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_last_socket, native:: CURLINFO_LASTSOCKET);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_ftp_entry_path, native::CURLINFO_FTP_ENTRY_PATH);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_condition_unmet, native:: CURLINFO_CONDITION_UNMET);
		IMPLEMENT_CURL_OPTION_GET_STRING(get_rtsp_session_id, native::CURLINFO_RTSP_SESSION_ID);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_client_cseq, native::CURLINFO_RTSP_CLIENT_CSEQ);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_server_cseq, native::CURLINFO_RTSP_SERVER_CSEQ);
		IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_cseq_recv, native::CURLINFO_RTSP_CSEQ_RECV);
		IMPLEMENT_CURL_OPTION_GET_LIST(get_ssl_engines, native::CURLINFO_SSL_ENGINES);
		IMPLEMENT_CURL_OPTION_GET_LIST(get_cookielist, native::CURLINFO_COOKIELIST);
		// CURLINFO_PRIVATE
		// CURLINFO_CERTINFO
		// CURLINFO_TLS_SESSION

		inline bool operator<(const easy& other) const
		{
			return (this < &other);
		}

		void handle_completion(const boost::system::error_code& err);

	private:
		void init();
		native::curl_socket_t open_tcp_socket(native::curl_sockaddr* address);

		static size_t write_function(char* ptr, size_t size, size_t nmemb, void* userdata);
		static size_t read_function(void* ptr, size_t size, size_t nmemb, void* userdata);
		static int seek_function(void* instream, native::curl_off_t offset, int origin);
#if LIBCURL_VERSION_NUM < 0x072000
		static int progress_function(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
#else
		static int xferinfo_function(void* clientp, native::curl_off_t dltotal, native::curl_off_t dlnow, native::curl_off_t ultotal, native::curl_off_t ulnow);
#endif
		static native::curl_socket_t opensocket(void* clientp, native::curlsocktype purpose, struct native::curl_sockaddr* address);
		static int closesocket(void* clientp, native::curl_socket_t item);

		boost::asio::io_service& io_service_;
		initialization::ptr initref_;
		native::CURL* handle_;
		multi* multi_;
		bool multi_registered_;
		handler_type handler_;
		boost::shared_ptr<std::istream> source_;
		boost::shared_ptr<std::ostream> sink_;
		std::string post_fields_;
		boost::shared_ptr<form> form_;
		boost::shared_ptr<string_list> headers_;
		boost::shared_ptr<string_list> http200_aliases_;
		boost::shared_ptr<string_list> mail_rcpts_;
		boost::shared_ptr<string_list> quotes_;
		boost::shared_ptr<string_list> resolved_hosts_;
		boost::shared_ptr<share> share_;
		boost::shared_ptr<string_list> telnet_options_;
		progress_callback_t progress_callback_;
	};
}

#undef IMPLEMENT_CURL_OPTION
#undef IMPLEMENT_CURL_OPTION_BOOLEAN
#undef IMPLEMENT_CURL_OPTION_ENUM
#undef IMPLEMENT_CURL_OPTION_STRING
#undef IMPLEMENT_CURL_OPTION_GET_STRING
#undef IMPLEMENT_CURL_OPTION_GET_DOUBLE
#undef IMPLEMENT_CURL_OPTION_GET_LONG
#undef IMPLEMENT_CURL_OPTION_GET_LIST

#endif
