/// @file
/// @author rfree (current maintainer in monero.cc project)
/// @brief This is the place to implement our handlers for protocol network actions, e.g. for ratelimit for download-requests

// Copyright (c) 2019, Ryo Currency Project
// Portions copyright (c) 2014-2018, The Monero Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This includes
//    posting the work online, or hosting copies of the modified work for download.
//
// 4. Any derivative version of this work is also covered by this license, including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2020
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <atomic>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <memory>

#include "syncobj.h"

#include "misc_language.h"
#include "misc_log_ex.h"
#include "net/net_utils_base.h"
#include "pragma_comp_defs.h"
#include <algorithm>
#include <boost/asio/deadline_timer.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/uuid/random_generator.hpp>
#include <iomanip>
#include <sstream>

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/ip/unicast.hpp>

#include "cryptonote_protocol_handler.h"
#include "net/network_throttle.hpp"

#include "cryptonote_core/cryptonote_core.h" // e.g. for the send_stop_signal()

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "net.cn"

// ################################################################################################
// ################################################################################################
// the "header part". Not separated out for .hpp because point of this modification is
// to rebuild just 1 translation unit while working on this code.
// (But maybe common parts will be separated out later though - if needed)
// ################################################################################################
// ################################################################################################

namespace cryptonote
{

class cryptonote_protocol_handler_base_pimpl
{ // placeholer if needed
  public:
};

} // namespace

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

namespace cryptonote
{

double cryptonote_protocol_handler_base::estimate_one_block_size() noexcept
{								 // for estimating size of blocks to downloa
	const double size_min = 500; // XXX 500
	//const int history_len = 20; // how many blocks to average over

	double avg = 0;
	try
	{
		avg = get_avg_block_size(/*history_len*/);
	}
	catch(...)
	{
	}
	avg = std::max(size_min, avg);
	return avg;
}

cryptonote_protocol_handler_base::cryptonote_protocol_handler_base()
{
}

cryptonote_protocol_handler_base::~cryptonote_protocol_handler_base()
{
}

void cryptonote_protocol_handler_base::handler_request_blocks_history(std::list<crypto::hash> &ids)
{
}

void cryptonote_protocol_handler_base::handler_response_blocks_now(size_t packet_size)
{
	using namespace epee::net_utils;
	double delay = 0; // will be calculated
	MDEBUG("Packet size: " << packet_size);
	do
	{ // rate limiting
		//XXX
		/*if (::cryptonote::core::get_is_stopping()) { 
			MDEBUG("We are stopping - so abort sleep");
			return;
		}*/
		/*if (m_was_shutdown) { 
			MDEBUG("m_was_shutdown - so abort sleep");
			return;
		}*/

		{
			CRITICAL_REGION_LOCAL(network_throttle_manager::m_lock_get_global_throttle_out);
			delay = network_throttle_manager::get_global_throttle_out().get_sleep_time_after_tick(packet_size);
		}

		delay *= 0.50;
		//delay = 0; // XXX
		if(delay > 0)
		{
			//delay += rand2*0.1;
			long int ms = (long int)(delay * 1000);
			MDEBUG("Sleeping for " << ms << " ms before packet_size=" << packet_size); // XXX debug sleep
			boost::this_thread::sleep(boost::posix_time::milliseconds(ms));			   // TODO randomize sleeps
		}
	} while(delay > 0);

	// XXX LATER XXX
	{
		CRITICAL_REGION_LOCAL(network_throttle_manager::m_lock_get_global_throttle_out);
		network_throttle_manager::get_global_throttle_out().handle_trafic_tcp(packet_size); // increase counter - global
																							//epee::critical_region_t<decltype(m_throttle_global_lock)> guard(m_throttle_global_lock); // *** critical ***
																							//m_throttle_global.m_out.handle_trafic_tcp( packet_size ); // increase counter - global
	}
}

} // namespace
