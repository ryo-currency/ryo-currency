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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "net/net_utils_base.h"
#include "p2p_protocol_defs.h"
#include <boost/uuid/uuid.hpp>

namespace nodetool
{

typedef boost::uuids::uuid uuid;
typedef boost::uuids::uuid net_connection_id;

template <class t_connection_context>
struct i_p2p_endpoint
{
	virtual bool relay_notify_to_list(int command, const std::string &data_buff, const std::list<boost::uuids::uuid> &connections) = 0;
	virtual bool relay_notify_to_all(int command, const std::string &data_buff, const epee::net_utils::connection_context_base &context) = 0;
	virtual bool invoke_command_to_peer(int command, const std::string &req_buff, std::string &resp_buff, const epee::net_utils::connection_context_base &context) = 0;
	virtual bool invoke_notify_to_peer(int command, const std::string &req_buff, const epee::net_utils::connection_context_base &context) = 0;
	virtual bool drop_connection(const epee::net_utils::connection_context_base &context) = 0;
	virtual void request_callback(const epee::net_utils::connection_context_base &context) = 0;
	virtual uint64_t get_connections_count() = 0;
	virtual void for_each_connection(std::function<bool(t_connection_context &, peerid_type, uint32_t)> f) = 0;
	virtual bool for_connection(const boost::uuids::uuid &, std::function<bool(t_connection_context &, peerid_type, uint32_t)> f) = 0;
	virtual bool block_host(const epee::net_utils::network_address &address, time_t seconds = 0) = 0;
	virtual bool unblock_host(const epee::net_utils::network_address &address) = 0;
	virtual std::map<std::string, time_t> get_blocked_hosts() = 0;
	virtual bool add_host_fail(const epee::net_utils::network_address &address) = 0;
};

template <class t_connection_context>
struct p2p_endpoint_stub : public i_p2p_endpoint<t_connection_context>
{
	virtual bool relay_notify_to_list(int command, const std::string &data_buff, const std::list<boost::uuids::uuid> &connections)
	{
		return false;
	}
	virtual bool relay_notify_to_all(int command, const std::string &data_buff, const epee::net_utils::connection_context_base &context)
	{
		return false;
	}
	virtual bool invoke_command_to_peer(int command, const std::string &req_buff, std::string &resp_buff, const epee::net_utils::connection_context_base &context)
	{
		return false;
	}
	virtual bool invoke_notify_to_peer(int command, const std::string &req_buff, const epee::net_utils::connection_context_base &context)
	{
		return true;
	}
	virtual bool drop_connection(const epee::net_utils::connection_context_base &context)
	{
		return false;
	}
	virtual void request_callback(const epee::net_utils::connection_context_base &context)
	{
	}
	virtual void for_each_connection(std::function<bool(t_connection_context &, peerid_type, uint32_t)> f)
	{
	}
	virtual bool for_connection(const boost::uuids::uuid &, std::function<bool(t_connection_context &, peerid_type, uint32_t)> f)
	{
		return false;
	}

	virtual uint64_t get_connections_count()
	{
		return false;
	}
	virtual bool block_host(const epee::net_utils::network_address &address, time_t seconds)
	{
		return true;
	}
	virtual bool unblock_host(const epee::net_utils::network_address &address)
	{
		return true;
	}
	virtual std::map<std::string, time_t> get_blocked_hosts()
	{
		return std::map<std::string, time_t>();
	}
	virtual bool add_host_fail(const epee::net_utils::network_address &address)
	{
		return true;
	}
};
}
