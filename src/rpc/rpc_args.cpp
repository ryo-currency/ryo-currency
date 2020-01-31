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

#include "rpc_args.h"

#include "common/command_line.h"
#include "common/i18n.h"
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>

#include "common/gulps.hpp"

namespace cryptonote
{
GULPS_CAT_MAJOR("rpc_args");
rpc_args::descriptors::descriptors() :
	rpc_bind_ip({"rpc-bind-ip", rpc_args::tr("Specify IP to bind RPC server"), "127.0.0.1"}), rpc_login({"rpc-login", rpc_args::tr("Specify username[:password] required for RPC server"), "", true}), confirm_external_bind({"confirm-external-bind", rpc_args::tr("Confirm rpc-bind-ip value is NOT a loopback (local) IP")}), rpc_access_control_origins({"rpc-access-control-origins", rpc_args::tr("Specify a comma separated list of origins to allow cross origin resource sharing"), ""})
{
}

const char *rpc_args::tr(const char *str) { return i18n_translate(str, "cryptonote::rpc_args"); }

void rpc_args::init_options(boost::program_options::options_description &desc)
{
	const descriptors arg{};
	command_line::add_arg(desc, arg.rpc_bind_ip);
	command_line::add_arg(desc, arg.rpc_login);
	command_line::add_arg(desc, arg.confirm_external_bind);
	command_line::add_arg(desc, arg.rpc_access_control_origins);
}

boost::optional<rpc_args> rpc_args::process(const boost::program_options::variables_map &vm)
{
	const descriptors arg{};
	rpc_args config{};

	config.bind_ip = command_line::get_arg(vm, arg.rpc_bind_ip);
	if(!config.bind_ip.empty())
	{
		// always parse IP here for error consistency
		boost::system::error_code ec{};
		const auto parsed_ip = boost::asio::ip::address::from_string(config.bind_ip, ec);
		if(ec)
		{
			GULPS_ERROR(tr("Invalid IP address given for --"), arg.rpc_bind_ip.name);
			return boost::none;
		}

		if(!parsed_ip.is_loopback() && !command_line::get_arg(vm, arg.confirm_external_bind))
		{
			GULPS_ERROR(
				"--", arg.rpc_bind_ip.name, tr(" permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --"), arg.confirm_external_bind.name);
			return boost::none;
		}
	}

	if(command_line::has_arg(vm, arg.rpc_login))
	{
		config.login = tools::login::parse(command_line::get_arg(vm, arg.rpc_login), true, [](bool verify) {
			return tools::password_container::prompt(verify, "RPC server password");
		});
		if(!config.login)
			return boost::none;

		if(config.login->username.empty())
		{
			GULPS_ERROR(tr("Username specified with --"), arg.rpc_login.name, tr(" cannot be empty"));
			return boost::none;
		}
	}

	auto access_control_origins_input = command_line::get_arg(vm, arg.rpc_access_control_origins);
	if(!access_control_origins_input.empty())
	{
		if(!config.login)
		{
			GULPS_ERROR(arg.rpc_access_control_origins.name, tr(" requires RFC server password --"), arg.rpc_login.name, tr(" cannot be empty"));
			return boost::none;
		}

		std::vector<std::string> access_control_origins;
		boost::split(access_control_origins, access_control_origins_input, boost::is_any_of(","));
		std::for_each(access_control_origins.begin(), access_control_origins.end(), boost::bind(&boost::trim<std::string>, _1, std::locale::classic()));
		config.access_control_origins = std::move(access_control_origins);
	}

	return {std::move(config)};
}
} // namespace cryptonote
