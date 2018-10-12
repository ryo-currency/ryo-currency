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
#define GULPS_CAT_MAJOR "wallet_args"

#include "wallet/wallet_args.h"

#include "common/i18n.h"
#include "common/util.h"
#include "string_tools.h"
#include "version.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#if defined(WIN32)
#include <crtdbg.h>
#include <boost/locale.hpp>
#endif

#include "common/gulps.hpp"

#define GULPS_PRINT_OK(...) GULPS_PRINT(__VA_ARGS__)

// workaround for a suspected bug in pthread/kernel on MacOS X
#ifdef __APPLE__
#define DEFAULT_MAX_CONCURRENCY 1
#else
#define DEFAULT_MAX_CONCURRENCY 0
#endif

namespace wallet_args
{
// Create on-demand to prevent static initialization order fiasco issues.
command_line::arg_descriptor<std::string> arg_generate_from_json()
{
	return {"generate-from-json", wallet_args::tr("Generate wallet from JSON format file"), ""};
}
command_line::arg_descriptor<std::string> arg_wallet_file()
{
	return {"wallet-file", wallet_args::tr("Use wallet <arg>"), ""};
}

const char *tr(const char *str)
{
	return i18n_translate(str, "wallet_args");
}

gulps_log_level log_scr, log_dsk;

boost::optional<boost::program_options::variables_map> main(
	int argc, char **argv,
	const char *const usage,
	const char *const notice,
	boost::program_options::options_description desc_params,
	const boost::program_options::positional_options_description &positional_options,
	const char *default_log_name,
	int &error_code,
	bool log_to_console)

{
	namespace bf = boost::filesystem;
	namespace po = boost::program_options;
#ifdef WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// Activate UTF-8 support for Boost filesystem classes on Windows
	std::locale::global(boost::locale::generator().generate(""));
	boost::filesystem::path::imbue(std::locale());
#endif

	error_code = 1;
	const command_line::arg_descriptor<std::string> arg_log_level = {"log-level", "Screen log level, 0-4 or categories", ""};
	const command_line::arg_descriptor<std::string> arg_file_level = {"log-file-level", "File log level, 0-4 or categories", ""};
	const command_line::arg_descriptor<std::string> arg_log_file = {"log-file", wallet_args::tr("Specify log file"), default_log_name};
	const command_line::arg_descriptor<uint32_t> arg_max_concurrency = {"max-concurrency", wallet_args::tr("Max number of threads to use for a parallel job"), DEFAULT_MAX_CONCURRENCY};
	const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", wallet_args::tr("Config file"), "", true};

	std::string lang = i18n_get_language();
	tools::on_startup();
	tools::set_strict_default_file_permissions(true);

	epee::string_tools::set_module_name_and_folder(argv[0]);

	po::options_description desc_general(wallet_args::tr("General options"));
	command_line::add_arg(desc_general, command_line::arg_help);
	command_line::add_arg(desc_general, command_line::arg_version);

	command_line::add_arg(desc_params, arg_log_file);
	command_line::add_arg(desc_params, arg_log_level);
	command_line::add_arg(desc_params, arg_file_level);
	command_line::add_arg(desc_params, arg_max_concurrency);
	command_line::add_arg(desc_params, arg_config_file);

	i18n_set_language("translations", "ryo", lang);

	po::options_description desc_all;
	desc_all.add(desc_general).add(desc_params);
	po::variables_map vm;
	std::unique_ptr<gulps::gulps_output> file_out;
	bool r = command_line::handle_error_helper(desc_all, [&]() {
		auto parser = po::command_line_parser(argc, argv).options(desc_all).positional(positional_options);
		po::store(parser.run(), vm);

		if(command_line::get_arg(vm, command_line::arg_help))
		{
			GULPS_PRINT_OK("Ryo '", RYO_RELEASE_NAME, "' (", RYO_VERSION_FULL, ")");
			GULPS_PRINT_OK(wallet_args::tr("This is the command line ryo wallet. It needs to connect to a ryo daemon to work correctly."));
			GULPS_PRINT_OK(wallet_args::tr("Usage:"), "\n  ", usage);
			GULPS_PRINT_OK(desc_all);
			error_code = 0;
			return false;
		}
		else if(command_line::get_arg(vm, command_line::arg_version))
		{
			GULPS_PRINT_OK("Ryo '", RYO_RELEASE_NAME, "' (", RYO_VERSION_FULL, ")");
			error_code = 0;
			return false;
		}

		if(command_line::has_arg(vm, arg_config_file))
		{
			std::string config = command_line::get_arg(vm, arg_config_file);
			bf::path config_path(config);
			boost::system::error_code ec;
			if(bf::exists(config_path, ec))
			{
				po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_params), vm);
			}
			else
			{
				GULPS_ERROR(wallet_args::tr("Can't find config file "), config);
				return false;
			}
		}

		if(!command_line::is_arg_defaulted(vm, arg_log_level))
		{
			if(!log_scr.parse_cat_string(command_line::get_arg(vm, arg_log_level).c_str()))
			{
				GULPS_ERROR(wallet_args::tr("Failed to parse filter string "), command_line::get_arg(vm, arg_log_level).c_str());
				return false;
			}
		}

		if(!command_line::is_arg_defaulted(vm, arg_file_level))
		{
			if(!log_dsk.parse_cat_string(command_line::get_arg(vm, arg_file_level).c_str()))
			{
				GULPS_ERROR(wallet_args::tr("Failed to parse filter string "), command_line::get_arg(vm, arg_file_level).c_str());
				return false;
			}
			
			try
			{
				file_out = std::unique_ptr<gulps::gulps_output>(new gulps::gulps_async_file_output(command_line::get_arg(vm, arg_log_file)));
			}
			catch(const std::exception& ex)
			{
				GULPS_ERROR(wallet_args::tr("Could not open file '"), command_line::get_arg(vm, arg_log_file), "' error: ", ex.what());
				return false;
			}
		}

		po::notify(vm);
		error_code = 0;
		return true;
	});
	if(!r)
		return boost::none;

	if(log_scr.is_active())
	{
		std::unique_ptr<gulps::gulps_output> out(new gulps::gulps_print_output(false, gulps::COLOR_WHITE));
		out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { 
				if(msg.out != gulps::OUT_LOG_0 && msg.out != gulps::OUT_USER_0)
					return false;
				if(printed)
					return false;
				return log_scr.match_msg(msg);
				});
		gulps::inst().add_output(std::move(out));
	}

	if(log_dsk.is_active())
	{
		file_out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { 
				if(msg.out != gulps::OUT_LOG_0 && msg.out != gulps::OUT_USER_0)
					return false;
				return log_dsk.match_msg(msg);
				});
		gulps::inst().add_output(std::move(file_out));
	}

	// Check for extra screen logs 
	if(notice)
		GULPS_PRINT_OK(notice);

	if(!command_line::is_arg_defaulted(vm, arg_max_concurrency))
		tools::set_max_concurrency(command_line::get_arg(vm, arg_max_concurrency));

	GULPS_PRINT_OK("Ryo '", RYO_RELEASE_NAME, "' (", RYO_VERSION_FULL, ")");

	error_code = 0;
	return {std::move(vm)};
}
}
