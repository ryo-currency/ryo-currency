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

#include "daemon/daemon.h"
#include "misc_log_ex.h"
#include "rpc/daemon_handler.h"
#include "rpc/zmq_server.h"
#include <boost/algorithm/string/split.hpp>
#include <memory>
#include <stdexcept>

#include "common/password.h"
#include "common/util.h"
#include "daemon/command_line_args.h"
#include "daemon/command_server.h"
#include "daemon/command_server.h"
#include "daemon/core.h"
#include "daemon/p2p.h"
#include "daemon/protocol.h"
#include "daemon/rpc.h"
#include "version.h"

using namespace epee;

#include <functional>

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{

struct t_internals
{
  private:
	t_protocol protocol;

  public:
	t_core core;
	t_p2p p2p;
	std::vector<std::unique_ptr<t_rpc>> rpcs;

	t_internals(
		boost::program_options::variables_map const &vm)
		: core{vm}, protocol{vm, core, command_line::get_arg(vm, cryptonote::arg_offline)}, p2p{vm, protocol}
	{
		// Handle circular dependencies
		protocol.set_p2p_endpoint(p2p.get());
		core.set_protocol(protocol.get());

		const auto testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
		const auto stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
		const auto restricted = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_restricted_rpc);
		const auto main_rpc_port = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_port);
		rpcs.emplace_back(new t_rpc{vm, core, p2p, restricted, testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET, main_rpc_port, "core"});

		auto restricted_rpc_port_arg = cryptonote::core_rpc_server::arg_rpc_restricted_bind_port;
		if(!command_line::is_arg_defaulted(vm, restricted_rpc_port_arg))
		{
			auto restricted_rpc_port = command_line::get_arg(vm, restricted_rpc_port_arg);
			rpcs.emplace_back(new t_rpc{vm, core, p2p, true, testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET, restricted_rpc_port, "restricted"});
		}
	}
};

void t_daemon::init_options(boost::program_options::options_description &option_spec)
{
	t_core::init_options(option_spec);
	t_p2p::init_options(option_spec);
	t_rpc::init_options(option_spec);
}

t_daemon::t_daemon(
	boost::program_options::variables_map const &vm)
	: mp_internals{new t_internals{vm}}
{
	zmq_rpc_bind_port = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_bind_port);
	zmq_rpc_bind_address = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_bind_ip);
}

t_daemon::~t_daemon() = default;

// MSVC is brain-dead and can't default this...
t_daemon::t_daemon(t_daemon &&other)
{
	if(this != &other)
	{
		mp_internals = std::move(other.mp_internals);
		other.mp_internals.reset(nullptr);
	}
}

// or this
t_daemon &t_daemon::operator=(t_daemon &&other)
{
	if(this != &other)
	{
		mp_internals = std::move(other.mp_internals);
		other.mp_internals.reset(nullptr);
	}
	return *this;
}

bool t_daemon::run(bool interactive)
{
	if(nullptr == mp_internals)
	{
		throw std::runtime_error{"Can't run stopped daemon"};
	}
	tools::signal_handler::install(std::bind(&daemonize::t_daemon::stop_p2p, this));

	try
	{
		if(!mp_internals->core.run())
			return false;

		for(auto &rpc : mp_internals->rpcs)
			rpc->run();

		std::unique_ptr<daemonize::t_command_server> rpc_commands;
		if(interactive && mp_internals->rpcs.size())
		{
			// The first three variables are not used when the fourth is false
			rpc_commands.reset(new daemonize::t_command_server(0, 0, boost::none, false, mp_internals->rpcs.front()->get_server()));
			rpc_commands->start_handling(std::bind(&daemonize::t_daemon::stop_p2p, this));
		}

		cryptonote::rpc::DaemonHandler rpc_daemon_handler(mp_internals->core.get(), mp_internals->p2p.get());
		cryptonote::rpc::ZmqServer zmq_server(rpc_daemon_handler);

		if(!zmq_server.addTCPSocket(zmq_rpc_bind_address, zmq_rpc_bind_port))
		{
			LOG_ERROR(std::string("Failed to add TCP Socket (") + zmq_rpc_bind_address + ":" + zmq_rpc_bind_port + ") to ZMQ RPC Server");

			if(rpc_commands)
				rpc_commands->stop_handling();

			for(auto &rpc : mp_internals->rpcs)
				rpc->stop();

			return false;
		}

		MINFO("Starting ZMQ server...");
		zmq_server.run();

		MINFO(std::string("ZMQ server started at ") + zmq_rpc_bind_address + ":" + zmq_rpc_bind_port + ".");

		mp_internals->p2p.run(); // blocks until p2p goes down

		if(rpc_commands)
			rpc_commands->stop_handling();

		zmq_server.stop();

		for(auto &rpc : mp_internals->rpcs)
			rpc->stop();
		mp_internals->core.get().get_miner().stop();
		MGINFO("Node stopped.");
		return true;
	}
	catch(std::exception const &ex)
	{
		MFATAL("Uncaught exception! " << ex.what());
		return false;
	}
	catch(...)
	{
		MFATAL("Uncaught exception!");
		return false;
	}
}

void t_daemon::stop()
{
	if(nullptr == mp_internals)
	{
		throw std::runtime_error{"Can't stop stopped daemon"};
	}
	mp_internals->core.get().get_miner().stop();
	mp_internals->p2p.stop();
	for(auto &rpc : mp_internals->rpcs)
		rpc->stop();

	mp_internals.reset(nullptr); // Ensure resources are cleaned up before we return
}

void t_daemon::stop_p2p()
{
	if(nullptr == mp_internals)
	{
		throw std::runtime_error{"Can't send stop signal to a stopped daemon"};
	}
	mp_internals->p2p.get().send_stop_signal();
}

} // namespace daemonize
