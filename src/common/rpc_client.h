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

#pragma once

#include <boost/optional/optional.hpp>

#include "common/http_connection.h"
#include "common/scoped_message_writer.h"
#include "net/http_auth.h"
#include "net/http_client.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"
#include "string_tools.h"

namespace tools
{
class t_rpc_client final
{
  private:
	epee::net_utils::http::http_simple_client m_http_client;

  public:
	t_rpc_client(
		uint32_t ip, uint16_t port, boost::optional<epee::net_utils::http::login> user)
		: m_http_client{}
	{
		m_http_client.set_server(
			epee::string_tools::get_ip_string_from_int32(ip), std::to_string(port), std::move(user));
	}

	template <typename T_req, typename T_res>
	bool basic_json_rpc_request(
		T_req &req, T_res &res, std::string const &method_name)
	{
		t_http_connection connection(&m_http_client);

		bool ok = connection.is_open();
		if(!ok)
		{
			fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
			return false;
		}
		ok = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, res, m_http_client, t_http_connection::TIMEOUT());
		if(!ok)
		{
			fail_msg_writer() << "basic_json_rpc_request: Daemon request failed";
			return false;
		}
		else
		{
			return true;
		}
	}

	template <typename T_req, typename T_res>
	bool json_rpc_request(
		T_req &req, T_res &res, std::string const &method_name, std::string const &fail_msg)
	{
		t_http_connection connection(&m_http_client);

		bool ok = connection.is_open();
		if(!ok)
		{
			fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
			return false;
		}
		ok = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, res, m_http_client, t_http_connection::TIMEOUT());
		if(!ok || res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
		{
			fail_msg_writer() << fail_msg << " -- json_rpc_request: " << res.status;
			return false;
		}
		else
		{
			return true;
		}
	}

	template <typename T_req, typename T_res>
	bool rpc_request(
		T_req &req, T_res &res, std::string const &relative_url, std::string const &fail_msg)
	{
		t_http_connection connection(&m_http_client);

		bool ok = connection.is_open();
		if(!ok)
		{
			fail_msg_writer() << "Couldn't connect to daemon: " << m_http_client.get_host() << ":" << m_http_client.get_port();
			return false;
		}
		ok = epee::net_utils::invoke_http_json(relative_url, req, res, m_http_client, t_http_connection::TIMEOUT());
		if(!ok || res.status != CORE_RPC_STATUS_OK) // TODO - handle CORE_RPC_STATUS_BUSY ?
		{
			fail_msg_writer() << fail_msg << "-- rpc_request: " << res.status;
			return false;
		}
		else
		{
			return true;
		}
	}

	bool check_connection()
	{
		t_http_connection connection(&m_http_client);
		return connection.is_open();
	}
};
}
