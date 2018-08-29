/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.

	C++ wrapper for libcurl's easy interface
*/

#include <boost/make_shared.hpp>
#include "easy.h"
#include "error_code.h"
#include "form.h"
#include "multi.h"
#include "share.h"
#include "string_list.h"

using namespace curl;

easy* easy::from_native(native::CURL* native_easy)
{
	easy* easy_handle;
	native::curl_easy_getinfo(native_easy, native::CURLINFO_PRIVATE, &easy_handle);
	return easy_handle;
}

easy::easy(boost::asio::io_service& io_service):
	io_service_(io_service),
	multi_(0),
	multi_registered_(false)
{
	init();
}

easy::easy(multi& multi_handle):
	io_service_(multi_handle.get_io_service()),
	multi_(&multi_handle),
	multi_registered_(false)
{
	init();
}

easy::~easy()
{
	cancel();

	if (handle_)
	{
		native::curl_easy_cleanup(handle_);
		handle_ = 0;
	}
}

void easy::perform()
{
	boost::system::error_code ec;
	perform(ec);
	boost::asio::detail::throw_error(ec, "perform");
}

void easy::perform(boost::system::error_code& ec)
{
	if (multi_)
	{
		throw std::runtime_error("attempt to perform synchronous operation while being attached to a multi object");
	}

	ec = boost::system::error_code(native::curl_easy_perform(handle_));

	if (sink_)
	{
		sink_->flush();
	}
}

void easy::async_perform(handler_type handler)
{
	if (!multi_)
	{
		throw std::runtime_error("attempt to perform async. operation without assigning a multi object");
	}

	// Cancel all previous async. operations
	cancel();

	// Keep track of all new sockets
	set_opensocket_function(&easy::opensocket);
	set_opensocket_data(this);

	// This one is tricky: Although sockets are opened in the context of an easy object, they can outlive the easy objects and be transferred into a multi object's connection pool. Why there is no connection pool interface in the multi interface to plug into to begin with is still a mystery to me. Either way, the close events have to be tracked by the multi object as sockets are usually closed when curl_multi_cleanup is invoked.
	set_closesocket_function(&easy::closesocket);
	set_closesocket_data(multi_);

	handler_ = handler;
	multi_registered_ = true;

	// Registering the easy handle with the multi handle might invoke a set of callbacks right away which cause the completion event to fire from within this function.
	multi_->add(this);
}

void easy::cancel()
{
	if (multi_registered_)
	{
		handle_completion(boost::system::error_code(boost::asio::error::operation_aborted));
		multi_->remove(this);
	}
}

void easy::set_source(boost::shared_ptr<std::istream> source)
{
	boost::system::error_code ec;
	set_source(source, ec);
	boost::asio::detail::throw_error(ec, "set_source");
}

void easy::set_source(boost::shared_ptr<std::istream> source, boost::system::error_code& ec)
{
	source_ = source;
	set_read_function(&easy::read_function, ec);
	if (!ec) set_read_data(this, ec);
	if (!ec) set_seek_function(&easy::seek_function, ec);
	if (!ec) set_seek_data(this, ec);
}

void easy::set_sink(boost::shared_ptr<std::ostream> sink)
{
	boost::system::error_code ec;
	set_sink(sink, ec);
	boost::asio::detail::throw_error(ec, "set_sink");
}

void easy::set_sink(boost::shared_ptr<std::ostream> sink, boost::system::error_code& ec)
{
	sink_ = sink;
	set_write_function(&easy::write_function);
	if (!ec) set_write_data(this);
}

void easy::unset_progress_callback()
{
	set_no_progress(true);
#if LIBCURL_VERSION_NUM < 0x072000
	set_progress_function(0);
	set_progress_data(0);
#else
	set_xferinfo_function(0);
	set_xferinfo_data(0);
#endif
}

void easy::set_progress_callback(progress_callback_t progress_callback)
{
	progress_callback_ = progress_callback;
	set_no_progress(false);
#if LIBCURL_VERSION_NUM < 0x072000
	set_progress_function(&easy::progress_function);
	set_progress_data(this);
#else
	set_xferinfo_function(&easy::xferinfo_function);
	set_xferinfo_data(this);
#endif
}

void easy::set_post_fields(const std::string& post_fields)
{
	boost::system::error_code ec;
	set_post_fields(post_fields, ec);
	boost::asio::detail::throw_error(ec, "set_post_fields");
}

void easy::set_post_fields(const std::string& post_fields, boost::system::error_code& ec)
{
	post_fields_ = post_fields;
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_POSTFIELDS, post_fields_.c_str()));

	if (!ec)
		set_post_field_size_large(static_cast<native::curl_off_t>(post_fields_.length()), ec);
}

void easy::set_http_post(boost::shared_ptr<form> form)
{
	boost::system::error_code ec;
	set_http_post(form, ec);
	boost::asio::detail::throw_error(ec, "set_http_post");
}

void easy::set_http_post(boost::shared_ptr<form> form, boost::system::error_code& ec)
{
	form_ = form;

	if (form_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPPOST, form_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPPOST, NULL));
	}
}

void easy::add_header(const std::string& name, const std::string& value)
{
	boost::system::error_code ec;
	add_header(name, value, ec);
	boost::asio::detail::throw_error(ec, "add_header");
}

void easy::add_header(const std::string& name, const std::string& value, boost::system::error_code& ec)
{
	add_header(name + ": " + value, ec);
}

void easy::add_header(const std::string& header)
{
	boost::system::error_code ec;
	add_header(header, ec);
	boost::asio::detail::throw_error(ec, "add_header");
}

void easy::add_header(const std::string& header, boost::system::error_code& ec)
{
	if (!headers_)
	{
		headers_ = boost::make_shared<string_list>();
	}

	headers_->add(header);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()));
}

void easy::set_headers(boost::shared_ptr<string_list> headers)
{
	boost::system::error_code ec;
	set_headers(headers, ec);
	boost::asio::detail::throw_error(ec, "set_headers");
}

void easy::set_headers(boost::shared_ptr<string_list> headers, boost::system::error_code& ec)
{
	headers_ = headers;

	if (headers_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTPHEADER, NULL));
	}
}

void easy::add_http200_alias(const std::string& http200_alias)
{
	boost::system::error_code ec;
	add_http200_alias(http200_alias, ec);
	boost::asio::detail::throw_error(ec, "add_http200_alias");
}

void easy::add_http200_alias(const std::string& http200_alias, boost::system::error_code& ec)
{
	if (!http200_aliases_)
	{
		http200_aliases_ = boost::make_shared<string_list>();
	}

	http200_aliases_->add(http200_alias);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES, http200_aliases_->native_handle()));
}

void easy::set_http200_aliases(boost::shared_ptr<string_list> http200_aliases)
{
	boost::system::error_code ec;
	set_http200_aliases(http200_aliases, ec);
	boost::asio::detail::throw_error(ec, "set_http200_aliases");
}

void easy::set_http200_aliases(boost::shared_ptr<string_list> http200_aliases, boost::system::error_code& ec)
{
	http200_aliases_ = http200_aliases;

	if (http200_aliases)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES, http200_aliases->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES, NULL));
	}
}

void easy::add_mail_rcpt(const std::string& mail_rcpt)
{
	boost::system::error_code ec;
	add_mail_rcpt(mail_rcpt, ec);
	boost::asio::detail::throw_error(ec, "add_mail_rcpt");
}

void easy::add_mail_rcpt(const std::string& mail_rcpt, boost::system::error_code& ec)
{
	if (!mail_rcpts_)
	{
		mail_rcpts_ = boost::make_shared<string_list>();
	}

	mail_rcpts_->add(mail_rcpt);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_MAIL_RCPT, mail_rcpts_->native_handle()));
}

void easy::set_mail_rcpts(boost::shared_ptr<string_list> mail_rcpts)
{
	boost::system::error_code ec;
	set_mail_rcpts(mail_rcpts, ec);
	boost::asio::detail::throw_error(ec, "set_mail_rcpts");
}

void easy::set_mail_rcpts(boost::shared_ptr<string_list> mail_rcpts, boost::system::error_code& ec)
{
	mail_rcpts_ = mail_rcpts;

	if (mail_rcpts_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_MAIL_RCPT, mail_rcpts_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_MAIL_RCPT, NULL));
	}
}

void easy::add_quote(const std::string& quote)
{
	boost::system::error_code ec;
	add_quote(quote, ec);
	boost::asio::detail::throw_error(ec, "add_quote");
}

void easy::add_quote(const std::string& quote, boost::system::error_code& ec)
{
	if (!quotes_)
	{
		quotes_ = boost::make_shared<string_list>();
	}

	quotes_->add(quote);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_QUOTE, quotes_->native_handle()));
}

void easy::set_quotes(boost::shared_ptr<string_list> quotes)
{
	boost::system::error_code ec;
	set_quotes(quotes, ec);
	boost::asio::detail::throw_error(ec, "set_quotes");
}

void easy::set_quotes(boost::shared_ptr<string_list> quotes, boost::system::error_code& ec)
{
	quotes_ = quotes;

	if (mail_rcpts_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_QUOTE, quotes_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_QUOTE, NULL));
	}
}

void easy::add_resolve(const std::string& resolved_host)
{
	boost::system::error_code ec;
	add_resolve(resolved_host, ec);
	boost::asio::detail::throw_error(ec, "add_resolve");
}

void easy::add_resolve(const std::string& resolved_host, boost::system::error_code& ec)
{
	if (!resolved_hosts_)
	{
		resolved_hosts_ = boost::make_shared<string_list>();
	}

	resolved_hosts_->add(resolved_host);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE, resolved_hosts_->native_handle()));
}

void easy::set_resolves(boost::shared_ptr<string_list> resolved_hosts)
{
	boost::system::error_code ec;
	set_resolves(resolved_hosts, ec);
	boost::asio::detail::throw_error(ec, "set_resolves");
}

void easy::set_resolves(boost::shared_ptr<string_list> resolved_hosts, boost::system::error_code& ec)
{
	resolved_hosts_ = resolved_hosts;

	if (resolved_hosts_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE, resolved_hosts_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE, NULL));
	}
}

void easy::set_share(boost::shared_ptr<share> share)
{
	boost::system::error_code ec;
	set_share(share, ec);
	boost::asio::detail::throw_error(ec, "set_share");
}

void easy::set_share(boost::shared_ptr<share> share, boost::system::error_code& ec)
{
	share_ = share;

	if (share)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_SHARE, share->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_SHARE, NULL));
	}
}

void easy::add_telnet_option(const std::string& telnet_option)
{
	boost::system::error_code ec;
	add_telnet_option(telnet_option, ec);
	boost::asio::detail::throw_error(ec, "add_telnet_option");
}

void easy::add_telnet_option(const std::string& telnet_option, boost::system::error_code& ec)
{
	if (!telnet_options_)
	{
		telnet_options_ = boost::make_shared<string_list>();
	}

	telnet_options_->add(telnet_option);
	ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS, telnet_options_->native_handle()));
}

void easy::add_telnet_option(const std::string& option, const std::string& value)
{
	boost::system::error_code ec;
	add_telnet_option(option, value, ec);
	boost::asio::detail::throw_error(ec, "add_telnet_option");
}

void easy::add_telnet_option(const std::string& option, const std::string& value, boost::system::error_code& ec)
{
	add_telnet_option(option + "=" + value, ec);
}

void easy::set_telnet_options(boost::shared_ptr<string_list> telnet_options)
{
	boost::system::error_code ec;
	set_telnet_options(telnet_options, ec);
	boost::asio::detail::throw_error(ec, "set_telnet_options");
}

void easy::set_telnet_options(boost::shared_ptr<string_list> telnet_options, boost::system::error_code& ec)
{
	telnet_options_ = telnet_options;

	if (telnet_options_)
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS, telnet_options_->native_handle()));
	}
	else
	{
		ec = boost::system::error_code(native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS, NULL));
	}
}

void easy::handle_completion(const boost::system::error_code& err)
{
	if (sink_)
	{
		sink_->flush();
	}

	multi_registered_ = false;
	io_service_.post(boost::bind(handler_, err));
}

void easy::init()
{
	initref_ = initialization::ensure_initialization();
	handle_ = native::curl_easy_init();

	if (!handle_)
	{
		throw std::bad_alloc();
	}

	set_private(this);
}

native::curl_socket_t easy::open_tcp_socket(native::curl_sockaddr* address)
{
	boost::system::error_code ec;
	std::auto_ptr<socket_type> socket(new socket_type(io_service_));

	switch (address->family)
	{
	case AF_INET:
		socket->open(boost::asio::ip::tcp::v4(), ec);
		break;

	case AF_INET6:
		socket->open(boost::asio::ip::tcp::v6(), ec);
		break;

	default:
		return CURL_SOCKET_BAD;
	}

	if (ec)
	{
		return CURL_SOCKET_BAD;
	}
	else
	{
		boost::shared_ptr<socket_info> si(new socket_info(this, socket));
		multi_->socket_register(si);
		return si->socket->native_handle();
	}
}

size_t easy::write_function(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	easy* self = static_cast<easy*>(userdata);
	size_t actual_size = size * nmemb;

	if (!actual_size)
	{
		return 0;
	}

	if (!self->sink_->write(ptr, actual_size))
	{
		return 0;
	}
	else
	{
		return actual_size;
	}
}

size_t easy::read_function(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	// FIXME readsome doesn't work with TFTP (see cURL docs)

	easy* self = static_cast<easy*>(userdata);
	size_t actual_size = size * nmemb;

	if (self->source_->eof())
	{
		return 0;
	}

	std::streamsize chars_stored = self->source_->readsome(static_cast<char*>(ptr), actual_size);

	if (!self->source_)
	{
		return CURL_READFUNC_ABORT;
	}
	else
	{
		return static_cast<size_t>(chars_stored);
	}
}

int easy::seek_function(void* instream, native::curl_off_t offset, int origin)
{
	// TODO we could allow the user to define an offset which this library should consider as position zero for uploading chunks of the file
	// alternatively do tellg() on the source stream when it is first passed to use_source() and use that as origin

	easy* self = static_cast<easy*>(instream);

	std::ios::seekdir dir;

	switch (origin)
	{
	case SEEK_SET:
		dir = std::ios::beg;
		break;

	case SEEK_CUR:
		dir = std::ios::cur;
		break;

	case SEEK_END:
		dir = std::ios::end;
		break;

	default:
		return CURL_SEEKFUNC_FAIL;
	}

	if (!self->source_->seekg(offset, dir))
	{
		return CURL_SEEKFUNC_FAIL;
	}
	else
	{
		return CURL_SEEKFUNC_OK;
	}
}

#if LIBCURL_VERSION_NUM < 0x072000
int easy::progress_function(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	easy* self = static_cast<easy*>(clientp);
	return self->progress_callback_(
		static_cast<native::curl_off_t>(dltotal),
		static_cast<native::curl_off_t>(dlnow),
		static_cast<native::curl_off_t>(ultotal),
		static_cast<native::curl_off_t>(ulnow)
		) ? 0 : 1;
}
#else
int easy::xferinfo_function(void* clientp, native::curl_off_t dltotal, native::curl_off_t dlnow, native::curl_off_t ultotal, native::curl_off_t ulnow)
{
	easy* self = static_cast<easy*>(clientp);
	return self->progress_callback_(dltotal, dlnow, ultotal, ulnow) ? 0 : 1;
}
#endif

native::curl_socket_t easy::opensocket(void* clientp, native::curlsocktype purpose, struct native::curl_sockaddr* address)
{
	easy* self = static_cast<easy*>(clientp);

	switch (purpose)
	{
	case native::CURLSOCKTYPE_IPCXN:
		switch (address->socktype)
		{
		case SOCK_STREAM:
			// Note to self: Why is address->protocol always set to zero?
			return self->open_tcp_socket(address);

		case SOCK_DGRAM:
			// TODO implement - I've seen other libcurl wrappers with UDP implementation, but have yet to read up on what this is used for
			return CURL_SOCKET_BAD;

		default:
			// unknown or invalid socket type
			return CURL_SOCKET_BAD;
		}
		break;

#ifdef CURLSOCKTYPE_ACCEPT
	case native::CURLSOCKTYPE_ACCEPT:
		// TODO implement - is this used for active FTP?
		return CURL_SOCKET_BAD;
#endif

	default:
		// unknown or invalid purpose
		return CURL_SOCKET_BAD;
	}
}

int easy::closesocket(void* clientp, native::curl_socket_t item)
{
	multi* multi_handle = static_cast<multi*>(clientp);
	multi_handle->socket_cleanup(item);
	return 0;
}
