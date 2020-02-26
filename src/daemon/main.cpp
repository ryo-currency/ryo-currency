// Copyright (c) 2020, Ryo Currency Project
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
//    public domain on 1st of February 2021
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

#include "blockchain_db/db_types.h"
#include "common/command_line.h"
#include "common/password.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "cryptonote_basic/miner.h"
#include "cryptonote_core/cryptonote_core.h"
#include "daemon/command_line_args.h"
#include "daemon/command_server.h"
#include "daemon/daemon.h"
#include "daemon/executor.h"
#include "daemonizer/daemonizer.h"
#include "p2p/net_node.h"
#include "rpc/core_rpc_server.h"
#include "rpc/rpc_args.h"
#include "version.h"

#ifdef STACK_TRACE
#include "common/stack_trace.h"
#endif // STACK_TRACE

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("main_daemon");

namespace po = boost::program_options;
namespace bf = boost::filesystem;

gulps_log_level log_scr, log_dsk;

int main(int argc, char* argv[])
{
#ifdef WIN32
	std::vector<char*> argptrs;
	command_line::set_console_utf8();
	if(command_line::get_windows_args(argptrs))
	{
		argc = argptrs.size();
		argv = argptrs.data();
	}
#endif

	gulps::inst().set_thread_tag("RYOD_MAIN");

	// Temporary output
	std::unique_ptr<gulps::gulps_output> gout_ptr(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TEXT_ONLY));
	gout_ptr->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { if(msg.lvl != gulps::LEVEL_INFO ) return !printed; return false; });
	uint64_t temp_out_id = gulps::inst().add_output(std::move(gout_ptr));

	try
	{
		// TODO parse the debug options like set log level right here at start

		tools::on_startup();

		epee::string_tools::set_module_name_and_folder(argv[0]);

		// Build argument description
		po::options_description all_options("All");
		po::options_description hidden_options("Hidden");
		po::options_description visible_options("Options");
		po::options_description core_settings("Settings");
		po::positional_options_description positional_options;
		{
			// Misc Options

			command_line::add_arg(visible_options, command_line::arg_help);
			command_line::add_arg(visible_options, command_line::arg_version);
			command_line::add_arg(visible_options, daemon_args::arg_os_version);
			command_line::add_arg(visible_options, daemon_args::arg_config_file);

			// Settings
			command_line::add_arg(core_settings, daemon_args::arg_log_file);
			command_line::add_arg(core_settings, daemon_args::arg_log_file_level);
			command_line::add_arg(core_settings, daemon_args::arg_log_level);
			command_line::add_arg(core_settings, daemon_args::arg_max_concurrency);
			command_line::add_arg(core_settings, daemon_args::arg_zmq_rpc_bind_ip);
			command_line::add_arg(core_settings, daemon_args::arg_zmq_rpc_bind_port);
			command_line::add_arg(core_settings, daemon_args::arg_display_timestamps);

			daemonizer::init_options(hidden_options, visible_options);
			daemonize::t_executor::init_options(core_settings);

			// Hidden options
			command_line::add_arg(hidden_options, daemon_args::arg_command);

			visible_options.add(core_settings);
			all_options.add(visible_options);
			all_options.add(hidden_options);

			// Positional
			positional_options.add(daemon_args::arg_command.name, -1); // -1 for unlimited arguments
		}

		// Do command line parsing
		po::variables_map vm;
		bool ok = command_line::handle_error_helper(visible_options, [&]() {
			boost::program_options::store(
				boost::program_options::command_line_parser(argc, argv)
					.options(all_options)
					.positional(positional_options)
					.run(),
				vm);

			return true;
		});
		if(!ok)
			return 1;

		if(command_line::get_arg(vm, command_line::arg_help))
		{
			GULPSF_PRINT("Ryo '{}' ({})\n", RYO_RELEASE_NAME , RYO_VERSION_FULL );
			GULPSF_PRINT("\nUsage: {} [options|settings] [daemon_command...]\n\n", std::string{argv[0]});
			GULPS_PRINT(visible_options);
			return 0;
		}

		// Ryo Version
		if(command_line::get_arg(vm, command_line::arg_version))
		{
			GULPSF_PRINT("Ryo '{}' ({})\n", RYO_RELEASE_NAME , RYO_VERSION_FULL );
			return 0;
		}

		// OS
		if(command_line::get_arg(vm, daemon_args::arg_os_version))
		{
			GULPS_PRINT("OS: ", tools::get_os_version_string());
			return 0;
		}

		std::string config = command_line::get_arg(vm, daemon_args::arg_config_file);
		boost::filesystem::path config_path(config);
		boost::system::error_code ec;
		if(bf::exists(config_path, ec))
		{
			try
			{
				po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), core_settings), vm);
			}
			catch(const std::exception &e)
			{
				// log system isn't initialized yet
				GULPSF_ERROR("Error parsing config file: {}", e.what());
				throw;
			}
		}
		else if(!command_line::is_arg_defaulted(vm, daemon_args::arg_config_file))
		{
			GULPS_ERROR("Can't find config file ", config);
			return 1;
		}

		const bool testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
		const bool stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
		if(testnet && stagenet)
		{
			GULPS_ERROR("Can't specify more than one of --tesnet and --stagenet");
			return 1;
		}

		std::string db_type = command_line::get_arg(vm, cryptonote::arg_db_type);

		// verify that blockchaindb type is valid
		if(!cryptonote::blockchain_valid_db_type(db_type))
		{
			GULPSF_PRINT("Invalid database type ({}), available types are: {}", db_type, cryptonote::blockchain_db_types(", "));
			return 0;
		}

		// data_dir
		//   if data-dir argument given:
		//     absolute path
		//     relative path: relative to cwd

		// Create data dir if it doesn't exist
		boost::filesystem::path data_dir = boost::filesystem::absolute(
			command_line::get_arg(vm, cryptonote::arg_data_dir));

		// FIXME: not sure on windows implementation default, needs further review
		//bf::path relative_path_base = daemonizer::get_relative_path_base(vm);
		bf::path relative_path_base = data_dir;

		po::notify(vm);

		// log_file_path
		//   default: <data_dir>/<CRYPTONOTE_NAME>.log
		//   if log-file argument given:
		//     absolute path
		//     relative path: relative to data_dir
		bf::path log_file_path{data_dir / std::string(CRYPTONOTE_NAME ".log")};
		if(!command_line::is_arg_defaulted(vm, daemon_args::arg_log_file))
			log_file_path = command_line::get_arg(vm, daemon_args::arg_log_file);
		log_file_path = bf::absolute(log_file_path, relative_path_base);

		tools::create_directories_if_necessary(data_dir.string());

		if(!command_line::is_arg_defaulted(vm, daemon_args::arg_log_level))
		{
			if(!log_scr.parse_cat_string(command_line::get_arg(vm, daemon_args::arg_log_level).c_str()))
			{
				GULPS_ERROR("Failed to parse filter string ", command_line::get_arg(vm, daemon_args::arg_log_level).c_str());
				return 1;
			}
		}
		else
			log_scr.parse_cat_string("");

		if(!command_line::is_arg_defaulted(vm, daemon_args::arg_log_file_level))
		{
			if(!log_dsk.parse_cat_string(command_line::get_arg(vm, daemon_args::arg_log_file_level).c_str()))
			{
				GULPS_ERROR("Failed to parse filter string ", command_line::get_arg(vm, daemon_args::arg_log_file_level).c_str());
				return 1;
			}
		}

		// If there are positional options, we're running a daemon command
		{
			gulps::inst().remove_output(temp_out_id);

			gout_ptr.reset(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TEXT_ONLY));
			gout_ptr->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool {
				if(msg.out == gulps::OUT_USER_1)
					return true;
				return false;
			});
			gulps::inst().add_output(std::move(gout_ptr));

			auto command = command_line::get_arg(vm, daemon_args::arg_command);

			if(command.size())
			{
				const cryptonote::rpc_args::descriptors arg{};
				auto rpc_ip_str = command_line::get_arg(vm, arg.rpc_bind_ip);
				auto rpc_port_str = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_port);

				uint32_t rpc_ip;
				uint16_t rpc_port;
				if(!epee::string_tools::get_ip_int32_from_string(rpc_ip, rpc_ip_str))
				{
					GULPS_ERROR("Invalid IP: ", rpc_ip_str);
					return 1;
				}
				if(!epee::string_tools::get_xtype_from_string(rpc_port, rpc_port_str))
				{
					GULPS_ERROR("Invalid port: ", rpc_port_str);
					return 1;
				}

				boost::optional<tools::login> login{};
				if(command_line::has_arg(vm, arg.rpc_login))
				{
					login = tools::login::parse(
						command_line::get_arg(vm, arg.rpc_login), false, [](bool verify) {
#ifdef HAVE_READLINE
							rdln::suspend_readline pause_readline;
#endif
							return tools::password_container::prompt(verify, "Daemon client password");
						});
					if(!login)
					{
						GULPS_ERROR("Failed to obtain password");
						return 1;
					}
				}

				daemonize::t_command_server rpc_commands{rpc_ip, rpc_port, std::move(login)};
				if(rpc_commands.process_command_vec(command))
				{
					return 0;
				}
				else
				{
					GULPSF_ERROR("Unknown command: {}", command.front());
					return 1;
				}
			}
		}

#ifdef STACK_TRACE
		tools::set_stack_trace_log(log_file_path.filename().string());
#endif // STACK_TRACE

		if(!command_line::is_arg_defaulted(vm, daemon_args::arg_max_concurrency))
			tools::set_max_concurrency(command_line::get_arg(vm, daemon_args::arg_max_concurrency));

		gout_ptr.reset(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TEXT_ONLY));
		gout_ptr->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool {
			if(msg.out == gulps::OUT_USER_1)
				return true;
			return false;
		});
		gulps::inst().add_output(std::move(gout_ptr));

		// OS
		if(command_line::get_arg(vm, daemon_args::arg_display_timestamps))
		{
			gout_ptr.reset(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TIMESTAMP_ONLY));
		}
		else
		{
			gout_ptr.reset(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TEXT_ONLY));
		}
		gout_ptr->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool {
			if(printed)
				return false;
			return log_scr.match_msg(msg);
		});
		gulps::inst().add_output(std::move(gout_ptr));

		if(log_dsk.is_active())
		{
			gout_ptr.reset(new gulps::gulps_async_file_output(log_file_path.string()));
			gout_ptr->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { return log_dsk.match_msg(msg); });
			gulps::inst().add_output(std::move(gout_ptr));
		}
		gulps::inst().remove_output(temp_out_id);

		// logging is now set up
		GULPSF_GLOBAL_PRINT("Ryo '{}' ({})", RYO_RELEASE_NAME, RYO_VERSION_FULL);

		GULPS_INFO("Moving from main() into the daemonize now.");

		return daemonizer::daemonize(argc, argv, daemonize::t_executor{}, vm) ? 0 : 1;
	}
	catch(std::exception const &ex)
	{
		GULPSF_LOG_ERROR("Exception in main! {}", ex.what());
	}
	catch(...)
	{
		GULPS_LOG_ERROR("Exception in main!");
	}
	return 1;
}
