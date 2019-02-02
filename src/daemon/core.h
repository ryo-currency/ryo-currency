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

#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "misc_log_ex.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{

class t_core final
{
  public:
	static void init_options(boost::program_options::options_description &option_spec)
	{
		cryptonote::core::init_options(option_spec);
	}

  private:
	typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
	cryptonote::core m_core;
	// TEMPORARY HACK - Yes, this creates a copy, but otherwise the original
	// variable map could go out of scope before the run method is called
	boost::program_options::variables_map const m_vm_HACK;

  public:
	t_core(
		boost::program_options::variables_map const &vm)
		: m_core{nullptr}, m_vm_HACK{vm}
	{
	}

	// TODO - get rid of circular dependencies in internals
	void set_protocol(t_protocol_raw &protocol)
	{
		m_core.set_cryptonote_protocol(&protocol);
	}

	std::string get_config_subdir() const
	{
		bool testnet = command_line::get_arg(m_vm_HACK, cryptonote::arg_testnet_on);
		bool stagenet = command_line::get_arg(m_vm_HACK, cryptonote::arg_stagenet_on);
		bool mainnet = !testnet && !stagenet;
		std::string port = command_line::get_arg(m_vm_HACK, nodetool::arg_p2p_bind_port);
		if((mainnet && port != std::to_string(cryptonote::config<cryptonote::MAINNET>::P2P_DEFAULT_PORT)) || (testnet && port != std::to_string(cryptonote::config<cryptonote::TESTNET>::P2P_DEFAULT_PORT)) || (stagenet && port != std::to_string(cryptonote::config<cryptonote::STAGENET>::P2P_DEFAULT_PORT)))
		{
			return port;
		}
		return std::string();
	}

	bool run()
	{
		//initialize core here
		MGINFO("Initializing core...");
		std::string config_subdir = get_config_subdir();
		if(!m_core.init(m_vm_HACK, config_subdir.empty() ? NULL : config_subdir.c_str()))
		{
			return false;
		}
		MGINFO("Core initialized OK");
		return true;
	}

	cryptonote::core &get()
	{
		return m_core;
	}

	~t_core()
	{
		MGINFO("Deinitializing core...");
		try
		{
			m_core.deinit();
			m_core.set_cryptonote_protocol(nullptr);
		}
		catch(...)
		{
			MERROR("Failed to deinitialize core...");
		}
	}
};
}
