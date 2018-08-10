// Copyright (c) 2018, Ryo Currency Project
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
//    public domain on 1st of February 2019
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

#include "node_rpc_proxy.h"
#include "common/json_util.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"

using namespace epee;

namespace tools
{

static const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

NodeRPCProxy::NodeRPCProxy(epee::net_utils::http::http_simple_client &http_client, boost::mutex &mutex)
	: m_http_client(http_client), m_daemon_rpc_mutex(mutex), m_height(0), m_height_time(0), m_earliest_height(), m_dynamic_per_kb_fee_estimate(0), m_dynamic_per_kb_fee_estimate_cached_height(0), m_dynamic_per_kb_fee_estimate_grace_blocks(0), m_rpc_version(0), m_target_height(0), m_target_height_time(0)
{
}

void NodeRPCProxy::invalidate()
{
	m_height = 0;
	m_height_time = 0;
	for(size_t n = 0; n < 256; ++n)
		m_earliest_height[n] = 0;
	m_dynamic_per_kb_fee_estimate = 0;
	m_dynamic_per_kb_fee_estimate_cached_height = 0;
	m_dynamic_per_kb_fee_estimate_grace_blocks = 0;
	m_rpc_version = 0;
	m_target_height = 0;
	m_target_height_time = 0;
}

boost::optional<std::string> NodeRPCProxy::get_rpc_version(uint32_t &rpc_version) const
{
	if(m_rpc_version == 0)
	{
		cryptonote::COMMAND_RPC_GET_VERSION::request req_t = AUTO_VAL_INIT(req_t);
		cryptonote::COMMAND_RPC_GET_VERSION::response resp_t = AUTO_VAL_INIT(resp_t);
		m_daemon_rpc_mutex.lock();
		bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_version", req_t, resp_t, m_http_client, rpc_timeout);
		m_daemon_rpc_mutex.unlock();
		CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get daemon RPC version");
		m_rpc_version = resp_t.version;
	}
	rpc_version = m_rpc_version;
	return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_height(uint64_t &height) const
{
	const time_t now = time(NULL);
	if(m_height == 0 || now >= m_height_time + 30) // re-cache every 30 seconds
	{
		cryptonote::COMMAND_RPC_GET_HEIGHT::request req = AUTO_VAL_INIT(req);
		cryptonote::COMMAND_RPC_GET_HEIGHT::response res = AUTO_VAL_INIT(res);

		m_daemon_rpc_mutex.lock();
		bool r = net_utils::invoke_http_json("/getheight", req, res, m_http_client, rpc_timeout);
		m_daemon_rpc_mutex.unlock();
		CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_BUSY, res.status, "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(res.status == CORE_RPC_STATUS_OK, res.status, "Failed to get current blockchain height");
		m_height = res.height;
		m_height_time = now;
	}
	height = m_height;
	return boost::optional<std::string>();
}

void NodeRPCProxy::set_height(uint64_t h)
{
	m_height = h;
}

boost::optional<std::string> NodeRPCProxy::get_target_height(uint64_t &height) const
{
	const time_t now = time(NULL);
	if(m_target_height == 0 || now >= m_target_height_time + 30) // re-cache every 30 seconds
	{
		cryptonote::COMMAND_RPC_GET_INFO::request req_t = AUTO_VAL_INIT(req_t);
		cryptonote::COMMAND_RPC_GET_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

		m_daemon_rpc_mutex.lock();
		bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_info", req_t, resp_t, m_http_client, rpc_timeout);
		m_daemon_rpc_mutex.unlock();

		CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get target blockchain height");
		m_target_height = resp_t.target_height;
		m_target_height_time = now;
	}
	height = m_target_height;
	return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_earliest_height(uint8_t version, uint64_t &earliest_height) const
{
	if(m_earliest_height[version] == 0)
	{
		cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req_t = AUTO_VAL_INIT(req_t);
		cryptonote::COMMAND_RPC_HARD_FORK_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

		m_daemon_rpc_mutex.lock();
		req_t.version = version;
		bool r = net_utils::invoke_http_json_rpc("/json_rpc", "hard_fork_info", req_t, resp_t, m_http_client, rpc_timeout);
		m_daemon_rpc_mutex.unlock();
		CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
		CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get hard fork status");
		m_earliest_height[version] = resp_t.earliest_height;
	}

	earliest_height = m_earliest_height[version];
	return boost::optional<std::string>();
}
}
