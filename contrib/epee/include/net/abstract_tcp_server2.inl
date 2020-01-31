/**
@file
@author from CrypoNote (see copyright below; Andrey N. Sabelnikov)
@monero rfree
@brief the connection templated-class for one peer connection
*/
// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


//#include "net_utils_base.h"
#include "misc_language.h"
#include "net/local_ip.h"
#include "pragma_comp_defs.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // TODO
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/thread/condition_variable.hpp> // TODO
#include <boost/thread/thread.hpp>			   // TODO
#include <boost/utility/value_init.hpp>
#include <boost/uuid/random_generator.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "../../../../src/cryptonote_core/cryptonote_core.h" // e.g. for the send_stop_signal()

#include "common/gulps.hpp"



#define DEFAULT_TIMEOUT_MS_LOCAL boost::posix_time::milliseconds(120000) // 2 minutes
#define DEFAULT_TIMEOUT_MS_REMOTE boost::posix_time::milliseconds(10000) // 10 seconds
#define TIMEOUT_EXTRA_MS_PER_BYTE 0.2

PRAGMA_WARNING_PUSH
namespace epee
{
namespace net_utils
{
/************************************************************************/
/*                                                                      */
/************************************************************************/
PRAGMA_WARNING_DISABLE_VS(4355)

template <class t_protocol_handler>
connection<t_protocol_handler>::connection(boost::asio::io_service &io_service,
										   typename t_protocol_handler::config_type &config,
										   std::atomic<long> &ref_sock_count, // the ++/-- counter
										   std::atomic<long> &sock_number,	// the only increasing ++ number generator
										   i_connection_filter *&pfilter, t_connection_type connection_type)
	: connection_basic(io_service, ref_sock_count, sock_number),
	  m_protocol_handler(this, config, context),
	  m_pfilter(pfilter),
	  m_connection_type(connection_type),
	  m_throttle_speed_in("speed_in", "throttle_speed_in"),
	  m_throttle_speed_out("speed_out", "throttle_speed_out"),
	  m_timer(io_service),
	  m_local(false)
{
	GULPSF_LOG_L1("test, connection constructor set m_connection_type={}", m_connection_type);
}
PRAGMA_WARNING_DISABLE_VS(4355)
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
connection<t_protocol_handler>::~connection() noexcept(false)
{
	if(!m_was_shutdown)
	{
		GULPS_LOG_L2("[sock ", socket_.native_handle() , "] Socket destroyed without shutdown.");
		shutdown();
	}

	GULPS_LOG_L2("[sock ", socket_.native_handle(), "] Socket destroyed");
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boost::asio::ip::tcp::socket &connection<t_protocol_handler>::socket()
{
	return socket_;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boost::shared_ptr<connection<t_protocol_handler>> connection<t_protocol_handler>::safe_shared_from_this()
{
	try
	{
		return connection<t_protocol_handler>::shared_from_this();
	}
	catch(const boost::bad_weak_ptr &)
	{
		// It happens when the connection is being deleted
		return boost::shared_ptr<connection<t_protocol_handler>>();
	}
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::start(bool is_income, bool is_multithreaded)
{
	GULPS_TRY_ENTRY();

	// Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
	auto self = safe_shared_from_this();
	if(!self)
		return false;

	m_is_multithreaded = is_multithreaded;

	boost::system::error_code ec;
	auto remote_ep = socket_.remote_endpoint(ec);
	GULPS_CHECK_AND_NO_ASSERT_MES(!ec, false, "Failed to get remote endpoint: " , ec.message() , ":" , ec.value());
	GULPS_CHECK_AND_NO_ASSERT_MES(remote_ep.address().is_v4(), false, "IPv6 not supported here");

	auto local_ep = socket_.local_endpoint(ec);
	GULPS_CHECK_AND_NO_ASSERT_MES(!ec, false, "Failed to get local endpoint: " , ec.message() , ":" , ec.value());

	context = boost::value_initialized<t_connection_context>();
	const unsigned long ip_{boost::asio::detail::socket_ops::host_to_network_long(remote_ep.address().to_v4().to_ulong())};
	m_local = epee::net_utils::is_ip_loopback(ip_);

	// create a random uuid
	boost::uuids::uuid random_uuid;
	// that stuff turns out to be included, even though it's from src... Taking advantage
	random_uuid = crypto::rand<boost::uuids::uuid>();

	context.set_details(random_uuid, epee::net_utils::ipv4_network_address(ip_, remote_ep.port()), is_income);
	GULPS_LOG_L3("[sock ", socket_.native_handle(), "] new connection from ", print_connection_context_short(context), " to ", local_ep.address().to_string(), ":", local_ep.port(), ", total sockets objects ", &m_ref_sock_count);

	if(m_pfilter && !m_pfilter->is_remote_host_allowed(context.m_remote_address))
	{
		GULPS_LOG_L1("[sock ", socket_.native_handle(), "] host denied " );
		close();
		return false;
	}

	m_protocol_handler.after_init_connection();

	reset_timer(get_default_time(), false);

	socket_.async_read_some(boost::asio::buffer(buffer_),
							strand_.wrap(
								boost::bind(&connection<t_protocol_handler>::handle_read, self,
											boost::asio::placeholders::error,
											boost::asio::placeholders::bytes_transferred)));
#if !defined(_WIN32) || !defined(__i686)
	// not supported before Windows7, too lazy for runtime check
	// Just exclude for 32bit windows builds
	//set ToS flag
	int tos = get_tos_flag();
	boost::asio::detail::socket_option::integer<IPPROTO_IP, IP_TOS>
		optionTos(tos);
	socket_.set_option(optionTos);
//GULPSF_LOG_L1("Set ToS flag to {}", tos);
#endif

	boost::asio::ip::tcp::no_delay noDelayOption(false);
	socket_.set_option(noDelayOption);

	return true;

	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::start()", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::request_callback()
{
	GULPS_TRY_ENTRY();
	GULPSF_LOG_L1("[{}] request_callback", print_connection_context_short(context) );
	// Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
	auto self = safe_shared_from_this();
	if(!self)
		return false;

	strand_.post(boost::bind(&connection<t_protocol_handler>::call_back_starter, self));
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::request_callback()", false);
	return true;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boost::asio::io_service &connection<t_protocol_handler>::get_io_service()
{
	return GET_IO_SERVICE(socket_);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::add_ref()
{
	GULPS_TRY_ENTRY();

	// Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
	auto self = safe_shared_from_this();
	if(!self)
		return false;
	//GULPSF_LOG_L2("[sock {}] add_ref, m_peer_number={}", socket_.native_handle() , mI->m_peer_number);
	CRITICAL_REGION_LOCAL(self->m_self_refs_lock);
	//GULPSF_LOG_L2("[sock {}] add_ref 2, m_peer_number={}", socket_.native_handle() , mI->m_peer_number);
	if(m_was_shutdown)
		return false;
	m_self_refs.push_back(self);
	return true;
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::add_ref()", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::release()
{
	GULPS_TRY_ENTRY();
	boost::shared_ptr<connection<t_protocol_handler>> back_connection_copy;
	GULPS_LOG_L2(context, "[sock ", socket_.native_handle() , "] release");
	CRITICAL_REGION_BEGIN(m_self_refs_lock);
	GULPS_CHECK_AND_ASSERT_MES(m_self_refs.size(), false, "[sock " , socket_.native_handle() , "] m_self_refs empty at connection<t_protocol_handler>::release() call");
	//erasing from container without additional copy can cause start deleting object, including m_self_refs
	back_connection_copy = m_self_refs.back();
	m_self_refs.pop_back();
	CRITICAL_REGION_END();
	return true;
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::release()", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::call_back_starter()
{
	GULPS_TRY_ENTRY();
	GULPSF_LOG_L1("[{}] fired_callback", print_connection_context_short(context) );
	m_protocol_handler.handle_qued_callback();
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::call_back_starter()", void());
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::save_dbg_log()
{
	std::string address, port;
	boost::system::error_code e;

	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(e);
	if(e)
	{
		address = "<not connected>";
		port = "<not connected>";
	}
	else
	{
		address = endpoint.address().to_string();
		port = boost::lexical_cast<std::string>(endpoint.port());
	}
	GULPSF_LOG_L1(" connection type {} {}:{} <--> {}:{}", to_string(m_connection_type) , socket_.local_endpoint().address().to_string() , socket_.local_endpoint().port()
							   , address , port);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::handle_read(const boost::system::error_code &e,
												 std::size_t bytes_transferred)
{
	GULPS_TRY_ENTRY();
	//GULPS_INFO("[sock " << socket_.native_handle() << "] Async read calledback.");

	if(!e)
	{
		{
			CRITICAL_REGION_LOCAL(m_throttle_speed_in_mutex);
			m_throttle_speed_in.handle_trafic_exact(bytes_transferred);
			context.m_current_speed_down = m_throttle_speed_in.get_current_speed();
		}

		{
			CRITICAL_REGION_LOCAL(epee::net_utils::network_throttle_manager::network_throttle_manager::m_lock_get_global_throttle_in);
			epee::net_utils::network_throttle_manager::network_throttle_manager::get_global_throttle_in().handle_trafic_exact(bytes_transferred);
		}

		double delay = 0; // will be calculated - how much we should sleep to obey speed limit etc

		if(speed_limit_is_enabled())
		{
			do // keep sleeping if we should sleep
			{
				{ //_scopeGULPSF_LOG_L1("CRITICAL_REGION_LOCAL");
					CRITICAL_REGION_LOCAL(epee::net_utils::network_throttle_manager::m_lock_get_global_throttle_in);
					delay = epee::net_utils::network_throttle_manager::get_global_throttle_in().get_sleep_time_after_tick(bytes_transferred);
				}

				delay *= 0.5;
				if(delay > 0)
				{
					long int ms = (long int)(delay * 100);
					reset_timer(boost::posix_time::milliseconds(ms + 1), true);
					boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
				}
			} while(delay > 0);
		} // any form of sleeping

		//GULPS_INFO("[sock " << socket_.native_handle() << "] RECV " << bytes_transferred);
		logger_handle_net_read(bytes_transferred);
		context.m_last_recv = time(NULL);
		context.m_recv_cnt += bytes_transferred;
		bool recv_res = m_protocol_handler.handle_recv(buffer_.data(), bytes_transferred);
		if(!recv_res)
		{
			//GULPS_INFO("[sock " << socket_.native_handle() << "] protocol_want_close");

			//some error in protocol, protocol handler ask to close connection
			boost::interprocess::ipcdetail::atomic_write32(&m_want_close_connection, 1);
			bool do_shutdown = false;
			CRITICAL_REGION_BEGIN(m_send_que_lock);
			if(!m_send_que.size())
				do_shutdown = true;
			CRITICAL_REGION_END();
			if(do_shutdown)
				shutdown();
		}
		else
		{
			reset_timer(get_timeout_from_bytes_read(bytes_transferred), false);
			socket_.async_read_some(boost::asio::buffer(buffer_),
									strand_.wrap(
										boost::bind(&connection<t_protocol_handler>::handle_read, connection<t_protocol_handler>::shared_from_this(),
													boost::asio::placeholders::error,
													boost::asio::placeholders::bytes_transferred)));
			//GULPS_INFO("[sock " << socket_.native_handle() << "]Async read requested.");
		}
	}
	else
	{
		GULPS_LOG_L2("[sock ", socket_.native_handle(), "] Some not success at read: ", e.message(), ":", e.value());
		if(e.value() != 2)
		{
			GULPS_LOG_L2("[sock ", socket_.native_handle(), "] Some problems at read: " , e.message(), ":" , e.value());
			shutdown();
		}
	}
	// If an error occurs then no new asynchronous operations are started. This
	// means that all shared_ptr references to the connection object will
	// disappear and the object will be destroyed automatically after this
	// handler returns. The connection class's destructor closes the socket.
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::handle_read", void());
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::call_run_once_service_io()
{
	GULPS_TRY_ENTRY();
	if(!m_is_multithreaded)
	{
		//single thread model, we can wait in blocked call
		size_t cnt = GET_IO_SERVICE(socket_).run_one();
		if(!cnt) //service is going to quit
			return false;
	}
	else
	{
		//multi thread model, we can't(!) wait in blocked call
		//so we make non blocking call and releasing CPU by calling sleep(0);
		//if no handlers were called
		//TODO: Maybe we need to have have critical section + event + callback to upper protocol to
		//ask it inside(!) critical region if we still able to go in event wait...
		size_t cnt = GET_IO_SERVICE(socket_).poll_one();
		if(!cnt)
			misc_utils::sleep_no_w(0);
	}

	return true;
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::call_run_once_service_io", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::do_send(const void *ptr, size_t cb)
{
	GULPS_TRY_ENTRY();

	// Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
	auto self = safe_shared_from_this();
	if(!self)
		return false;
	if(m_was_shutdown)
		return false;
	// TODO avoid copy

	const double factor = 32;			 // TODO config
	typedef long long signed int t_safe; // my t_size to avoid any overunderflow in arithmetic
	const t_safe chunksize_good = (t_safe)(1024 * std::max(1.0, factor));
	const t_safe chunksize_max = chunksize_good * 2;
	const bool allow_split = (m_connection_type == e_connection_type_RPC) ? false : true; // do not split RPC data

	GULPS_CHECK_AND_ASSERT_MES(!(chunksize_max < 0), false, "Negative chunksize_max"); // make sure it is unsigned before removin sign with cast:
	long long unsigned int chunksize_max_unsigned = static_cast<long long unsigned int>(chunksize_max);

	if(allow_split && (cb > chunksize_max_unsigned))
	{
		{																					// LOCK: chunking
			epee::critical_region_t<decltype(m_chunking_lock)> send_guard(m_chunking_lock); // *** critical ***

			GULPSF_LOG_L1("do_send() will SPLIT into small chunks, from packet={} B for ptr={}", cb , ptr);
			t_safe all = cb; // all bytes to send
			t_safe pos = 0;  // current sending position
			// 01234567890
			// ^^^^        (pos=0, len=4)     ;   pos:=pos+len, pos=4
			//     ^^^^    (pos=4, len=4)     ;   pos:=pos+len, pos=8
			//         ^^^ (pos=8, len=4)    ;

			// const size_t bufsize = chunksize_good; // TODO safecast
			// char* buf = new char[ bufsize ];

			bool all_ok = true;
			while(pos < all)
			{
				t_safe lenall = all - pos;					   // length from here to end
				t_safe len = std::min(chunksize_good, lenall); // take a smaller part
				GULPS_CHECK_AND_ASSERT_MES(len <= chunksize_good, false, "len too large");
				// pos=8; len=4; all=10;	len=3;

				GULPS_CHECK_AND_ASSERT_MES(!(len < 0), false, "negative len"); // check before we cast away sign:
				unsigned long long int len_unsigned = static_cast<long long int>(len);
				GULPS_CHECK_AND_ASSERT_MES(len > 0, false, "len not strictly positive");										// (redundant)
				GULPS_CHECK_AND_ASSERT_MES(len_unsigned < std::numeric_limits<size_t>::max(), false, "Invalid len_unsigned"); // yeap we want strong < then max size, to be sure

				void *chunk_start = ((char *)ptr) + pos;
				GULPSF_LOG_L1("chunk_start={} ptr={} pos={}", chunk_start , ptr , pos);
				GULPS_CHECK_AND_ASSERT_MES(chunk_start >= ptr, false, "Pointer wraparound"); // not wrapped around address?
				//std::memcpy( (void*)buf, chunk_start, len);

				GULPSF_LOG_L1("part of {}: pos={} len={}", lenall , pos , len);

				bool ok = do_send_chunk(chunk_start, len); // <====== ***

				all_ok = all_ok && ok;
				if(!all_ok)
				{
					GULPSF_LOG_L1("do_send() DONE ***FAILED*** from packet={} B for ptr={}", cb , ptr);
					GULPSF_LOG_L1("do_send() SEND was aborted in middle of big package - this is mostly harmless (e.g. peer closed connection) but if it causes trouble tell us at https://github.com/ryo-currency/ryo. {}", cb);
					return false; // partial failure in sending
				}
				pos = pos + len;
				GULPS_CHECK_AND_ASSERT_MES(pos > 0, false, "pos <= 0");

				// (in catch block, or uniq pointer) delete buf;
			} // each chunk

			GULPSF_LOG_L1("do_send() DONE SPLIT from packet={} B for ptr={}", cb , ptr);

			GULPSF_LOG_L1("do_send() m_connection_type = {}", m_connection_type);

			return all_ok; // done - e.g. queued - all the chunks of current do_send call
		}				   // LOCK: chunking
	}					   // a big block (to be chunked) - all chunks
	else
	{								   // small block
		return do_send_chunk(ptr, cb); // just send as 1 big chunk
	}

	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::do_send", false);
} // do_send()

//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::do_send_chunk(const void *ptr, size_t cb)
{
	GULPS_TRY_ENTRY();
	// Use safe_shared_from_this, because of this is public method and it can be called on the object being deleted
	auto self = safe_shared_from_this();
	if(!self)
		return false;
	if(m_was_shutdown)
		return false;
	{
		CRITICAL_REGION_LOCAL(m_throttle_speed_out_mutex);
		m_throttle_speed_out.handle_trafic_exact(cb);
		context.m_current_speed_up = m_throttle_speed_out.get_current_speed();
	}

	//GULPS_INFO("[sock " << socket_.native_handle() << "] SEND " << cb);
	context.m_last_send = time(NULL);
	context.m_send_cnt += cb;
	//some data should be wrote to stream
	//request complete

	// No sleeping here; sleeping is done once and for all in "handle_write"

	m_send_que_lock.lock(); // *** critical ***
	epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&]() { m_send_que_lock.unlock(); });

	long int retry = 0;
	const long int retry_limit = 5 * 4;
	while(m_send_que.size() > ABSTRACT_SERVER_SEND_QUE_MAX_COUNT)
	{
		retry++;

		/* if ( ::cryptonote::core::get_is_stopping() ) { // TODO re-add fast stop
            GULPS_LOG_L1("ABORT queue wait due to stopping");
            return false; // aborted
        }*/

		long int ms = 250 + (rand() % 50);
		GULPSF_LOG_L1("Sleeping because QUEUE is FULL, in {} for {} ms before packet_size={}", __FUNCTION__ , ms , cb); // XXX debug sleep
		m_send_que_lock.unlock();
		boost::this_thread::sleep(boost::posix_time::milliseconds(ms));
		m_send_que_lock.lock();
		GULPSF_LOG_L1("sleep for queue: {}", ms);

		if(retry > retry_limit)
		{
			GULPSF_WARN("send que size is more than ABSTRACT_SERVER_SEND_QUE_MAX_COUNT({}), shutting down connection", ABSTRACT_SERVER_SEND_QUE_MAX_COUNT );
			shutdown();
			return false;
		}
	}

	m_send_que.resize(m_send_que.size() + 1);
	m_send_que.back().assign((const char *)ptr, cb);

	if(m_send_que.size() > 1)
	{ // active operation should be in progress, nothing to do, just wait last operation callback
		auto size_now = cb;
		GULPSF_LOG_L1("do_send() NOW just queues: packet={} B, is added to queue-size={}", size_now , m_send_que.size());
		//do_send_handler_delayed( ptr , size_now ); // (((H))) // empty function

		GULPS_LOG_L2(context, "[sock ", socket_.native_handle(), "] Async send requested ", m_send_que.front().size());
	}
	else
	{ // no active operation

		if(m_send_que.size() != 1)
		{
			GULPS_ERROR("Looks like no active operations, but send que size != 1!!");
			return false;
		}

		auto size_now = m_send_que.front().size();
		GULPSF_LOG_L1("do_send() NOW SENSD: packet={} B", size_now );
		if(speed_limit_is_enabled())
			do_send_handler_write(ptr, size_now); // (((H)))

		GULPS_CHECK_AND_ASSERT_MES(size_now == m_send_que.front().size(), false, "Unexpected queue size");
		reset_timer(get_default_time(), false);
		boost::asio::async_write(socket_, boost::asio::buffer(m_send_que.front().data(), size_now),
								 //strand_.wrap(
								 boost::bind(&connection<t_protocol_handler>::handle_write, self, _1, _2)
								 //)
								 );
		//GULPSF_LOG_L2("(chunk): {}", size_now);
		//logger_handle_net_write(size_now);
		//GULPS_INFO("[sock " << socket_.native_handle() << "] Async send requested " << m_send_que.front().size());
	}

	//do_send_handler_stop( ptr , cb ); // empty function

	return true;

	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::do_send", false);
} // do_send_chunk
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boost::posix_time::milliseconds connection<t_protocol_handler>::get_default_time() const
{
	if(m_local)
		return DEFAULT_TIMEOUT_MS_LOCAL;
	else
		return DEFAULT_TIMEOUT_MS_REMOTE;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boost::posix_time::milliseconds connection<t_protocol_handler>::get_timeout_from_bytes_read(size_t bytes) const
{
	boost::posix_time::milliseconds ms = (boost::posix_time::milliseconds)(unsigned)(bytes * TIMEOUT_EXTRA_MS_PER_BYTE);
	ms += m_timer.expires_from_now();
	if(ms > get_default_time())
		ms = get_default_time();
	return ms;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::reset_timer(boost::posix_time::milliseconds ms, bool add)
{
	if(m_connection_type != e_connection_type_RPC)
		return;
	GULPS_LOG_L2("Setting ", ms, " expiry");
	auto self = safe_shared_from_this();
	if(!self)
	{
		GULPS_ERROR("Resetting timer on a dead object");
		return;
	}
	if(add)
		ms += m_timer.expires_from_now();
	m_timer.expires_from_now(ms);
	m_timer.async_wait([=](const boost::system::error_code &ec) {
		if(ec == boost::asio::error::operation_aborted)
			return;
		GULPS_LOG_L1(context, "connection timeout, closing");
		self->close();
	});
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::shutdown()
{
	// Initiate graceful connection closure.
	m_timer.cancel();
	boost::system::error_code ignored_ec;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	m_was_shutdown = true;
	m_protocol_handler.release_protocol();
	return true;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::close()
{
	GULPS_TRY_ENTRY();
	//GULPS_INFO("[sock " << socket_.native_handle() << "] Que Shutdown called.");
	m_timer.cancel();
	size_t send_que_size = 0;
	CRITICAL_REGION_BEGIN(m_send_que_lock);
	send_que_size = m_send_que.size();
	CRITICAL_REGION_END();
	boost::interprocess::ipcdetail::atomic_write32(&m_want_close_connection, 1);
	if(!send_que_size)
	{
		shutdown();
	}

	return true;
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::close", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool connection<t_protocol_handler>::cancel()
{
	return close();
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::handle_write(const boost::system::error_code &e, size_t cb)
{
	GULPS_TRY_ENTRY();
	GULPS_LOG_L2(context, "[sock ", socket_.native_handle(), "] Async send calledback ", cb);

	if(e)
	{
		GULPS_LOG_L1("[sock ", socket_.native_handle(), "] Some problems at write: ", e.message(), ":", e.value());
		shutdown();
		return;
	}
	logger_handle_net_write(cb);

	// The single sleeping that is needed for correctly handling "out" speed throttling
	if(speed_limit_is_enabled())
	{
		sleep_before_packet(cb, 1, 1);
	}

	bool do_shutdown = false;
	CRITICAL_REGION_BEGIN(m_send_que_lock);
	if(m_send_que.empty())
	{
		GULPS_ERROR("[sock ", socket_.native_handle(), "], m_send_que.size() == 0 at handle_write!");
		return;
	}

	m_send_que.pop_front();
	if(m_send_que.empty())
	{
		if(boost::interprocess::ipcdetail::atomic_read32(&m_want_close_connection))
		{
			do_shutdown = true;
		}
	}
	else
	{
		//have more data to send
		reset_timer(get_default_time(), false);
		auto size_now = m_send_que.front().size();
		GULPS_LOG_L1("handle_write NOW SENDS: packet=", size_now, " B, from  queue size=", m_send_que.size());
		if(speed_limit_is_enabled())
			do_send_handler_write_from_queue(e, m_send_que.front().size(), m_send_que.size()); // (((H)))
		GULPS_CHECK_AND_ASSERT_MES(size_now == m_send_que.front().size(), void(), "Unexpected queue size");
		boost::asio::async_write(socket_, boost::asio::buffer(m_send_que.front().data(), size_now),
								 // strand_.wrap(
								 boost::bind(&connection<t_protocol_handler>::handle_write, connection<t_protocol_handler>::shared_from_this(), _1, _2)
								 // )
								 );
		//GULPSF_LOG_L2("(normal){}", size_now);
	}
	CRITICAL_REGION_END();

	if(do_shutdown)
	{
		shutdown();
	}
	GULPS_CATCH_ENTRY_L0("connection<t_protocol_handler>::handle_write", void());
}

//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void connection<t_protocol_handler>::setRpcStation()
{
	m_connection_type = e_connection_type_RPC;
	GULPS_LOG_L1("set m_connection_type = RPC ");
}

template <class t_protocol_handler>
bool connection<t_protocol_handler>::speed_limit_is_enabled() const
{
	return m_connection_type != e_connection_type_RPC;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

template <class t_protocol_handler>
boosted_tcp_server<t_protocol_handler>::boosted_tcp_server(t_connection_type connection_type) : m_io_service_local_instance(new boost::asio::io_service()),
																								io_service_(*m_io_service_local_instance.get()),
																								acceptor_(io_service_),
																								m_stop_signal_sent(false), m_port(0),
																								m_sock_count(0), m_sock_number(0), m_threads_count(0),
																								m_pfilter(NULL), m_thread_index(0),
																								m_connection_type(connection_type),
																								new_connection_()
{
	create_server_type_map();
	m_thread_name_prefix = "NET";
}

template <class t_protocol_handler>
boosted_tcp_server<t_protocol_handler>::boosted_tcp_server(boost::asio::io_service &extarnal_io_service, t_connection_type connection_type) : io_service_(extarnal_io_service),
																																			  acceptor_(io_service_),
																																			  m_stop_signal_sent(false), m_port(0),
																																			  m_sock_count(0), m_sock_number(0), m_threads_count(0),
																																			  m_pfilter(NULL), m_thread_index(0),
																																			  m_connection_type(connection_type),
																																			  new_connection_()
{
	create_server_type_map();
	m_thread_name_prefix = "NET";
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
boosted_tcp_server<t_protocol_handler>::~boosted_tcp_server()
{
	this->send_stop_signal();
	timed_wait_server_stop(10000);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void boosted_tcp_server<t_protocol_handler>::create_server_type_map()
{
	server_type_map["NET"] = e_connection_type_NET;
	server_type_map["RPC"] = e_connection_type_RPC;
	server_type_map["P2P"] = e_connection_type_P2P;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::init_server(uint32_t port, const std::string address)
{
	GULPS_TRY_ENTRY();
	m_stop_signal_sent = false;
	m_port = port;
	m_address = address;
	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, boost::lexical_cast<std::string>(port), boost::asio::ip::tcp::resolver::query::canonical_name);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
	boost::asio::ip::tcp::endpoint binded_endpoint = acceptor_.local_endpoint();
	m_port = binded_endpoint.port();
	GULPS_LOG_L1("start accept");
	new_connection_.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_sock_number, m_pfilter, m_connection_type));
	acceptor_.async_accept(new_connection_->socket(),
						   boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept, this,
									   boost::asio::placeholders::error));

	return true;
}
catch(const std::exception &e)
{
	GULPS_ERROR("Error starting server: ", e.what());
	return false;
}
catch(...)
{
	GULPS_ERROR("Error starting server");
	return false;
}
}
//-----------------------------------------------------------------------------
// clang-format off
PUSH_WARNINGS
DISABLE_GCC_WARNING(maybe-uninitialized)
// clang-format on
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::init_server(const std::string port, const std::string &address)
{
	uint32_t p = 0;

	if(port.size() && !string_tools::get_xtype_from_string(p, port))
	{
		GULPSF_ERROR("Failed to convert port no = {}", port);
		return false;
	}
	return this->init_server(p, address);
}
POP_WARNINGS
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::worker_thread()
{
	GULPS_TRY_ENTRY();
	uint32_t local_thr_index = boost::interprocess::ipcdetail::atomic_inc32(&m_thread_index);
	std::string thread_name = std::string("[") + m_thread_name_prefix;
	thread_name += boost::to_string(local_thr_index) + "]";
	GULPS_SET_THREAD_NAME(thread_name);
	//   GULPS_LOG_L1("Thread name: ", m_thread_name_prefix);
	while(!m_stop_signal_sent)
	{
		try
		{
			io_service_.run();
		}
		catch(const std::exception &ex)
		{
			GULPS_ERROR("Exception at server worker thread, what=", ex.what());
		}
		catch(...)
		{
			GULPS_ERROR("Exception at server worker thread, unknown execption");
		}
	}
	//GULPS_INFO("Worker thread finished");
	return true;
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::worker_thread", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void boosted_tcp_server<t_protocol_handler>::set_threads_prefix(const std::string &prefix_name)
{
	m_thread_name_prefix = prefix_name;
	auto it = server_type_map.find(m_thread_name_prefix);
	if(it == server_type_map.end())
		throw std::runtime_error("Unknown prefix/server type:" + std::string(prefix_name));
	auto connection_type = it->second; // the value of type
	GULPSF_INFO("Set server type to: {} from name: {}, prefix_name = {}", connection_type , m_thread_name_prefix , prefix_name);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void boosted_tcp_server<t_protocol_handler>::set_connection_filter(i_connection_filter *pfilter)
{
	m_pfilter = pfilter;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::run_server(size_t threads_count, bool wait, const boost::thread::attributes &attrs)
{
	GULPS_TRY_ENTRY();
	m_threads_count = threads_count;
	m_main_thread_id = boost::this_thread::get_id();
	GULPS_SET_THREAD_NAME("[SRV_MAIN]");
	while(!m_stop_signal_sent)
	{

		// Create a pool of threads to run all of the io_services.
		CRITICAL_REGION_BEGIN(m_threads_lock);
		for(std::size_t i = 0; i < threads_count; ++i)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(
				attrs, boost::bind(&boosted_tcp_server<t_protocol_handler>::worker_thread, this)));
			GULPS_LOG_L1("Run server thread name: ", m_thread_name_prefix);
			m_threads.push_back(thread);
		}
		CRITICAL_REGION_END();
		// Wait for all threads in the pool to exit.
		if(wait)
		{
			GULPS_LOG_L1("JOINING all threads");
			for(std::size_t i = 0; i < m_threads.size(); ++i)
			{
				m_threads[i]->join();
			}
			GULPS_LOG_L1("JOINING all threads - almost");
			m_threads.clear();
			GULPS_LOG_L1("JOINING all threads - DONE");
		}
		else
		{
			GULPS_LOG_L1("Reiniting OK.");
			return true;
		}

		if(wait && !m_stop_signal_sent)
		{
			//some problems with the listening socket ?..
			GULPS_LOG_L1("Net service stopped without stop request, restarting...");
			if(!this->init_server(m_port, m_address))
			{
				GULPS_LOG_L1("Reiniting service failed, exit.");
				return false;
			}
			else
			{
				GULPS_LOG_L1("Reiniting OK.");
			}
		}
	}
	return true;
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::run_server", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::is_thread_worker()
{
	GULPS_TRY_ENTRY();
	CRITICAL_REGION_LOCAL(m_threads_lock);
	BOOST_FOREACH(boost::shared_ptr<boost::thread> &thp, m_threads)
	{
		if(thp->get_id() == boost::this_thread::get_id())
			return true;
	}
	if(m_threads_count == 1 && boost::this_thread::get_id() == m_main_thread_id)
		return true;
	return false;
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::is_thread_worker", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop(uint64_t wait_mseconds)
{
	GULPS_TRY_ENTRY();
	boost::chrono::milliseconds ms(wait_mseconds);
	for(std::size_t i = 0; i < m_threads.size(); ++i)
	{
		if(m_threads[i]->joinable() && !m_threads[i]->try_join_for(ms))
		{
			GULPS_LOG_L1("Interrupting thread ", m_threads[i]->get_id());
			m_threads[i]->interrupt();
		}
	}
	return true;
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::timed_wait_server_stop", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void boosted_tcp_server<t_protocol_handler>::send_stop_signal()
{
	m_stop_signal_sent = true;
	GULPS_TRY_ENTRY();
	connections_mutex.lock();
	for(auto &c : connections_)
	{
		c->cancel();
	}
	connections_.clear();
	connections_mutex.unlock();
	io_service_.stop();
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::send_stop_signal()", void());
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::is_stop_signal_sent()
{
	return m_stop_signal_sent;
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
void boosted_tcp_server<t_protocol_handler>::handle_accept(const boost::system::error_code &e)
{
	GULPS_LOG_L1("handle_accept");
	GULPS_TRY_ENTRY();
	if(!e)
	{
		if(m_connection_type == e_connection_type_RPC)
		{
			GULPS_LOG_L1("New server for RPC connections");
			new_connection_->setRpcStation(); // hopefully this is not needed actually
		}
		connection_ptr conn(std::move(new_connection_));
		new_connection_.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_sock_number, m_pfilter, m_connection_type));
		acceptor_.async_accept(new_connection_->socket(),
							   boost::bind(&boosted_tcp_server<t_protocol_handler>::handle_accept, this,
										   boost::asio::placeholders::error));

		boost::asio::socket_base::keep_alive opt(true);
		conn->socket().set_option(opt);

		conn->start(true, 1 < m_threads_count);
		conn->save_dbg_log();
	}
	else
	{
		GULPSF_ERROR("Some problems at accept: {}, connections_count = {}", e.message() , m_sock_count);
	}
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::handle_accept", void());
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
bool boosted_tcp_server<t_protocol_handler>::connect(const std::string &adr, const std::string &port, uint32_t conn_timeout, t_connection_context &conn_context, const std::string &bind_ip)
{
	GULPS_TRY_ENTRY();

	connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_sock_number, m_pfilter, m_connection_type));
	connections_mutex.lock();
	connections_.insert(new_connection_l);
	GULPSF_LOG_L1("connections_ size now {}", connections_.size());
	connections_mutex.unlock();
	epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&]() { CRITICAL_REGION_LOCAL(connections_mutex); connections_.erase(new_connection_l); });
	boost::asio::ip::tcp::socket &sock_ = new_connection_l->socket();

	//////////////////////////////////////////////////////////////////////////
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end;
	if(iterator == end)
	{
		GULPSF_ERROR("Failed to resolve {}", adr);
		return false;
	}
	//////////////////////////////////////////////////////////////////////////

	//boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::address::from_string(addr.c_str()), port);
	boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);

	sock_.open(remote_endpoint.protocol());
	if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "")
	{
		boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(adr.c_str()), 0);
		sock_.bind(local_endpoint);
	}

	/*
    NOTICE: be careful to make sync connection from event handler: in case if all threads suddenly do sync connect, there will be no thread to dispatch events from io service.
    */

	boost::system::error_code ec = boost::asio::error::would_block;

	//have another free thread(s), work in wait mode, without event handling
	struct local_async_context
	{
		boost::system::error_code ec;
		boost::mutex connect_mut;
		boost::condition_variable cond;
	};

	boost::shared_ptr<local_async_context> local_shared_context(new local_async_context());
	local_shared_context->ec = boost::asio::error::would_block;
	boost::unique_lock<boost::mutex> lock(local_shared_context->connect_mut);
	auto connect_callback = [](boost::system::error_code ec_, boost::shared_ptr<local_async_context> shared_context) {
		shared_context->connect_mut.lock();
		shared_context->ec = ec_;
		shared_context->cond.notify_one();
		shared_context->connect_mut.unlock();
	};

	sock_.async_connect(remote_endpoint, boost::bind<void>(connect_callback, _1, local_shared_context));
	while(local_shared_context->ec == boost::asio::error::would_block)
	{
		bool r = local_shared_context->cond.timed_wait(lock, boost::get_system_time() + boost::posix_time::milliseconds(conn_timeout));
		if(m_stop_signal_sent)
		{
			if(sock_.is_open())
				sock_.close();
			return false;
		}
		if(local_shared_context->ec == boost::asio::error::would_block && !r)
		{
			//timeout
			sock_.close();
			GULPSF_LOG_L2("Failed to connect to {}:{}, because of timeout ({})", adr , port , conn_timeout );
			return false;
		}
	}
	ec = local_shared_context->ec;

	if(ec || !sock_.is_open())
	{
		GULPSF_LOG_L2("Some problems at connect, message: {}", ec.message());
		if(sock_.is_open())
			sock_.close();
		return false;
	}

	GULPSF_LOG_L2("Connected success to {}:{}", adr , port);

	// start adds the connection to the config object's list, so we don't need to have it locally anymore
	connections_mutex.lock();
	connections_.erase(new_connection_l);
	connections_mutex.unlock();
	bool r = new_connection_l->start(false, 1 < m_threads_count);
	if(r)
	{
		new_connection_l->get_context(conn_context);
		//new_connection_l.reset(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_pfilter));
	}
	else
	{
		GULPSF_ERROR("[sock {}] Failed to start connection, connections_count = {}", new_connection_l->socket().native_handle() , m_sock_count);
	}

	new_connection_l->save_dbg_log();

	return r;

	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect", false);
}
//---------------------------------------------------------------------------------
template <class t_protocol_handler>
template <class t_callback>
bool boosted_tcp_server<t_protocol_handler>::connect_async(const std::string &adr, const std::string &port, uint32_t conn_timeout, const t_callback &cb, const std::string &bind_ip)
{
	GULPS_TRY_ENTRY();
	connection_ptr new_connection_l(new connection<t_protocol_handler>(io_service_, m_config, m_sock_count, m_sock_number, m_pfilter, m_connection_type));
	connections_mutex.lock();
	connections_.insert(new_connection_l);
	GULPSF_LOG_L1("connections_ size now {}", connections_.size());
	connections_mutex.unlock();
	epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&]() { CRITICAL_REGION_LOCAL(connections_mutex); connections_.erase(new_connection_l); });
	boost::asio::ip::tcp::socket &sock_ = new_connection_l->socket();

	//////////////////////////////////////////////////////////////////////////
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), adr, port, boost::asio::ip::tcp::resolver::query::canonical_name);
	boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end;
	if(iterator == end)
	{
		GULPSF_ERROR("Failed to resolve {}", adr);
		return false;
	}
	//////////////////////////////////////////////////////////////////////////
	boost::asio::ip::tcp::endpoint remote_endpoint(*iterator);

	sock_.open(remote_endpoint.protocol());
	if(bind_ip != "0.0.0.0" && bind_ip != "0" && bind_ip != "")
	{
		boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::address::from_string(adr.c_str()), 0);
		sock_.bind(local_endpoint);
	}

	boost::shared_ptr<boost::asio::deadline_timer> sh_deadline(new boost::asio::deadline_timer(io_service_));
	//start deadline
	sh_deadline->expires_from_now(boost::posix_time::milliseconds(conn_timeout));
	sh_deadline->async_wait([=](const boost::system::error_code &error) {
		if(error != boost::asio::error::operation_aborted)
		{
			GULPSF_LOG_L2("Failed to connect to {}:{}, because of timeout ({})", adr , port , conn_timeout );
			new_connection_l->socket().close();
		}
	});
	//start async connect
	sock_.async_connect(remote_endpoint, [=](const boost::system::error_code &ec_) {
		t_connection_context conn_context = AUTO_VAL_INIT(conn_context);
		boost::system::error_code ignored_ec;
		boost::asio::ip::tcp::socket::endpoint_type lep = new_connection_l->socket().local_endpoint(ignored_ec);
		if(!ec_)
		{ //success
			if(!sh_deadline->cancel())
			{
				cb(conn_context, boost::asio::error::operation_aborted); //this mean that deadline timer already queued callback with cancel operation, rare situation
			}
			else
			{
				GULPSF_LOG_L2("[sock {}] Connected success to {}:{} from {}:{}", new_connection_l->socket().native_handle() , adr , port , lep.address().to_string() , lep.port());

				// start adds the connection to the config object's list, so we don't need to have it locally anymore
				connections_mutex.lock();
				connections_.erase(new_connection_l);
				connections_mutex.unlock();
				bool r = new_connection_l->start(false, 1 < m_threads_count);
				if(r)
				{
					new_connection_l->get_context(conn_context);
					cb(conn_context, ec_);
				}
				else
				{
					GULPSF_LOG_L2("[sock {}] Failed to start connection to {}:{}", new_connection_l->socket().native_handle() , adr , port);
					cb(conn_context, boost::asio::error::fault);
				}
			}
		}
		else
		{
			GULPSF_LOG_L2("[sock {}] Failed to connect to {}:{} from {}:{}: {}:{}", new_connection_l->socket().native_handle() , adr , port , lep.address().to_string() , lep.port() , ec_.message() , ec_.value());
			cb(conn_context, ec_);
		}
	});
	return true;
	GULPS_CATCH_ENTRY_L0("boosted_tcp_server<t_protocol_handler>::connect_async", false);
}

} // namespace
} // namespace
PRAGMA_WARNING_POP
