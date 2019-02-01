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

#include "rpc/core_rpc_server.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{

class t_rpc final
{
  public:
	static void init_options(boost::program_options::options_description &option_spec)
	{
		cryptonote::core_rpc_server::init_options(option_spec);
	}

  private:
	cryptonote::core_rpc_server m_server;
	const std::string m_description;

  public:
	t_rpc(
		boost::program_options::variables_map const &vm, t_core &core, t_p2p &p2p, const bool restricted, const cryptonote::network_type nettype, const std::string &port, const std::string &description)
		: m_server{core.get(), p2p.get()}, m_description{description}
	{
		MGINFO("Initializing " << m_description << " RPC server...");

		if(!m_server.init(vm, restricted, nettype, port))
		{
			throw std::runtime_error("Failed to initialize " + m_description + " RPC server.");
		}
		MGINFO(m_description << " RPC server initialized OK on port: " << m_server.get_binded_port());
	}

	void run()
	{
		MGINFO("Starting " << m_description << " RPC server...");
		if(!m_server.run(2, false))
		{
			throw std::runtime_error("Failed to start " + m_description + " RPC server.");
		}
		MGINFO(m_description << " RPC server started ok");
	}

	void stop()
	{
		MGINFO("Stopping " << m_description << " RPC server...");
		m_server.send_stop_signal();
		m_server.timed_wait_server_stop(5000);
	}

	cryptonote::core_rpc_server *get_server()
	{
		return &m_server;
	}

	~t_rpc()
	{
		MGINFO("Deinitializing " << m_description << " RPC server...");
		try
		{
			m_server.deinit();
		}
		catch(...)
		{
			MERROR("Failed to deinitialize " << m_description << " RPC server...");
		}
	}
};
}
