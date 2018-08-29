/**
	curl-asio: wrapper for integrating libcurl with boost.asio applications
	Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
	See COPYING for license information.

	Integration of libcurl's multi interface with Boost.Asio
*/

#include <boost/bind.hpp>
#include "easy.h"
#include "error_code.h"
#include "multi.h"

using namespace curl;

multi::multi(boost::asio::io_service& io_service):
	io_service_(io_service),
	timeout_(io_service),
	still_running_(0)
{
	initref_ = initialization::ensure_initialization();
	handle_ = native::curl_multi_init();

	if (!handle_)
	{
		throw std::bad_alloc();
	}

	set_socket_function(&multi::socket);
	set_socket_data(this);

	set_timer_function(&multi::timer);
	set_timer_data(this);
}

multi::~multi()
{
	while (!easy_handles_.empty())
	{
		easy_set_type::iterator it = easy_handles_.begin();
		easy* easy_handle = *it;
		easy_handle->cancel();
	}

	if (handle_)
	{
		native::curl_multi_cleanup(handle_);
		handle_ = 0;
	}
}

void multi::add(easy* easy_handle)
{
	easy_handles_.insert(easy_handle);
	add_handle(easy_handle->native_handle());
}

void multi::remove(easy* easy_handle)
{
	easy_set_type::iterator it = easy_handles_.find(easy_handle);

	if (it != easy_handles_.end())
	{
		easy_handles_.erase(it);
		remove_handle(easy_handle->native_handle());
	}
}

void multi::socket_register(boost::shared_ptr<socket_info> si)
{
	socket_type::native_handle_type fd = si->socket->native_handle();
	sockets_.insert(socket_map_type::value_type(fd, si));
}

void multi::socket_cleanup(native::curl_socket_t s)
{
	socket_map_type::iterator it = sockets_.find(s);

	if (it != sockets_.end())
	{
		socket_info_ptr p = it->second;
		monitor_socket(p, CURL_POLL_NONE);
		p->socket.reset();
		sockets_.erase(it);
	}
}

void multi::add_handle(native::CURL* native_easy)
{
	boost::system::error_code ec(native::curl_multi_add_handle(handle_, native_easy));
	boost::asio::detail::throw_error(ec, "add_handle");
}

void multi::remove_handle(native::CURL* native_easy)
{
	boost::system::error_code ec(native::curl_multi_remove_handle(handle_, native_easy));
	boost::asio::detail::throw_error(ec, "remove_handle");
}

void multi::assign(native::curl_socket_t sockfd, void* user_data)
{
	boost::system::error_code ec(native::curl_multi_assign(handle_, sockfd, user_data));
	boost::asio::detail::throw_error(ec, "multi_assign");
}

void multi::socket_action(native::curl_socket_t s, int event_bitmask)
{
	boost::system::error_code ec(native::curl_multi_socket_action(handle_, s, event_bitmask, &still_running_));
	boost::asio::detail::throw_error(ec);

	if (!still_running())
	{
		timeout_.cancel();
	}
}

void multi::set_socket_function(socket_function_t socket_function)
{
	boost::system::error_code ec(native::curl_multi_setopt(handle_, native::CURLMOPT_SOCKETFUNCTION, socket_function));
	boost::asio::detail::throw_error(ec, "set_socket_function");
}

void multi::set_socket_data(void* socket_data)
{
	boost::system::error_code ec(native::curl_multi_setopt(handle_, native::CURLMOPT_SOCKETDATA, socket_data));
	boost::asio::detail::throw_error(ec, "set_socket_data");
}

void multi::set_timer_function(timer_function_t timer_function)
{
	boost::system::error_code ec(native::curl_multi_setopt(handle_, native::CURLMOPT_TIMERFUNCTION, timer_function));
	boost::asio::detail::throw_error(ec, "set_timer_function");
}

void multi::set_timer_data(void* timer_data)
{
	boost::system::error_code ec(native::curl_multi_setopt(handle_, native::CURLMOPT_TIMERDATA, timer_data));
	boost::asio::detail::throw_error(ec, "set_timer_data");
}

void multi::monitor_socket(socket_info_ptr si, int action)
{
	si->monitor_read = !!(action & CURL_POLL_IN);
	si->monitor_write = !!(action & CURL_POLL_OUT);

	if (!si->socket)
	{
		// If libcurl already requested destruction of the socket, then no further action is required.
		return;
	}

	if (si->monitor_read && !si->pending_read_op)
	{
		start_read_op(si);
	}

	if (si->monitor_write && !si->pending_write_op)
	{
		start_write_op(si);
	}

	// The CancelIoEx API is only available on Windows Vista and up. On Windows XP, the cancel operation therefore falls
	// back to CancelIo(), which only works in single-threaded environments and depends on driver support, which is not always available.
	// Therefore, this code section is only enabled when explicitly targeting Windows Vista and up or a non-Windows platform.
	// On Windows XP and previous versions, the I/O handler will be executed and immediately return.
#if !defined(BOOST_WINDOWS_API) || (defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600))
	if (action == CURL_POLL_NONE && (si->pending_read_op || si->pending_write_op))
	{
		si->socket->cancel();
	}
#endif
}

void multi::process_messages()
{
	native::CURLMsg* msg;
	int msgs_left;

	while ((msg = native::curl_multi_info_read(handle_, &msgs_left)))
	{
		if (msg->msg == native::CURLMSG_DONE)
		{
			easy* easy_handle = easy::from_native(msg->easy_handle);
			boost::system::error_code ec;

			if (msg->data.result != native::CURLE_OK)
			{
				ec = boost::system::error_code(msg->data.result);
			}

			remove(easy_handle);
			easy_handle->handle_completion(ec);
		}
	}
}

bool multi::still_running()
{
	return (still_running_ > 0);
}

void multi::start_read_op(socket_info_ptr si)
{
	si->pending_read_op = true;
	si->socket->async_read_some(boost::asio::null_buffers(), boost::bind(&multi::handle_socket_read, this, boost::asio::placeholders::error, si));
}

void multi::handle_socket_read(const boost::system::error_code& err, socket_info_ptr si)
{
    if (!si->socket)
    {
        si->pending_read_op = false;
        return;
    }

	if (!err)
	{
		socket_action(si->socket->native_handle(), CURL_CSELECT_IN);
		process_messages();

		if (si->monitor_read)
			start_read_op(si);
		else
			si->pending_read_op = false;
	}
	else
	{
		if (err != boost::asio::error::operation_aborted)
		{
			socket_action(si->socket->native_handle(), CURL_CSELECT_ERR);
			process_messages();			
		}

		si->pending_read_op = false;
	}
}

void multi::start_write_op(socket_info_ptr si)
{
	si->pending_write_op = true;
	si->socket->async_write_some(boost::asio::null_buffers(), boost::bind(&multi::handle_socket_write, this, boost::asio::placeholders::error, si));
}

void multi::handle_socket_write(const boost::system::error_code& err, socket_info_ptr si)
{
    if (!si->socket)
    {
        si->pending_write_op = false;
        return;
    }

	if (!err)
	{
		socket_action(si->socket->native_handle(), CURL_CSELECT_OUT);
		process_messages();

		if (si->monitor_write)
			start_write_op(si);
		else
			si->pending_write_op = false;
	}
	else
	{
		if (err != boost::asio::error::operation_aborted)
		{
			socket_action(si->socket->native_handle(), CURL_CSELECT_ERR);
			process_messages();
		}

		si->pending_write_op = false;
	}
}

void multi::handle_timeout(const boost::system::error_code& err)
{
	if (!err)
	{
		socket_action(CURL_SOCKET_TIMEOUT, 0);
		process_messages();
	}
}

multi::socket_info_ptr multi::get_socket_from_native(native::curl_socket_t native_socket)
{
	socket_map_type::iterator it = sockets_.find(native_socket);

	if (it != sockets_.end())
	{
		return it->second;
	}
	else
	{
		return socket_info_ptr();
	}
}

int multi::socket(native::CURL* native_easy, native::curl_socket_t s, int what, void* userp, void* socketp)
{
	multi* self = static_cast<multi*>(userp);

	if (what == CURL_POLL_REMOVE)
	{
		// stop listening for events
		socket_info_ptr* si = static_cast<socket_info_ptr*>(socketp);
		self->monitor_socket(*si, CURL_POLL_NONE);
		delete si;
	}
	else if (socketp)
	{
		// change direction
		socket_info_ptr* si = static_cast<socket_info_ptr*>(socketp);
		(*si)->handle = easy::from_native(native_easy);
		self->monitor_socket(*si, what);
	}
	else if (native_easy)
	{
		// register the socket
		socket_info_ptr si = self->get_socket_from_native(s);
		if (!si)
			throw std::invalid_argument("bad socket");
		si->handle = easy::from_native(native_easy);
		self->assign(s, new socket_info_ptr(si));
		self->monitor_socket(si, what);
	}
	else
	{
		throw std::invalid_argument("neither socketp nor native_easy were set");
	}

	return 0;
}

int multi::timer(native::CURLM* native_multi, long timeout_ms, void* userp)
{
	multi* self = static_cast<multi*>(userp);

	if (timeout_ms > 0)
	{
		self->timeout_.expires_from_now(boost::posix_time::millisec(timeout_ms));
		self->timeout_.async_wait(boost::bind(&multi::handle_timeout, self, boost::asio::placeholders::error));
	}
	else
	{
		self->timeout_.cancel();
		self->handle_timeout(boost::system::error_code());
	}

	return 0;
}
