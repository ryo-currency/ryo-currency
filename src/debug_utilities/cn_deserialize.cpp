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

#include "common/command_line.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/blockchain.h"
#include "version.h"
#include <boost/filesystem.hpp>

#include "common/gulps.hpp"

namespace po = boost::program_options;
using namespace epee;

using namespace cryptonote;

GULPS_CAT_MAJOR("cn_deser");

static void print_extra_fields(const std::vector<cryptonote::tx_extra_field> &fields)
{
	GULPSF_PRINT("tx_extra has {} field(s)", fields.size());
	for(size_t n = 0; n < fields.size(); ++n)
	{
		GULPSF_PRINT("field {}: ", n);
		if(typeid(cryptonote::tx_extra_padding) == fields[n].type())
			GULPSF_PRINT("extra padding: {} bytes", boost::get<cryptonote::tx_extra_padding>(fields[n]).size);
		else if(typeid(cryptonote::tx_extra_pub_key) == fields[n].type())
			GULPSF_PRINT("extra pub key: {}", boost::get<cryptonote::tx_extra_pub_key>(fields[n]).pub_key);
		else if(typeid(cryptonote::tx_extra_nonce) == fields[n].type())
			GULPSF_PRINT("extra nonce: {}", epee::string_tools::buff_to_hex_nodelimer(boost::get<cryptonote::tx_extra_nonce>(fields[n]).nonce));
		else if(typeid(cryptonote::tx_extra_merge_mining_tag) == fields[n].type())
			GULPSF_PRINT("extra merge mining tag: depth {}, merkle root {}", boost::get<cryptonote::tx_extra_merge_mining_tag>(fields[n]).depth, boost::get<cryptonote::tx_extra_merge_mining_tag>(fields[n]).merkle_root);
		else if(typeid(cryptonote::tx_extra_mysterious_minergate) == fields[n].type())
			GULPSF_PRINT("extra minergate custom: {}", epee::string_tools::buff_to_hex_nodelimer(boost::get<cryptonote::tx_extra_mysterious_minergate>(fields[n]).data));
		else
			GULPS_PRINT("unknown");
	}
}

int main(int argc, char *argv[])
{
	uint32_t log_level = 0;
	std::string input;

	tools::on_startup();

	boost::filesystem::path output_file_path;

	po::options_description desc_cmd_only("Command line options");
	po::options_description desc_cmd_sett("Command line options and settings options");
	const command_line::arg_descriptor<std::string> arg_output_file = {"output-file", "Specify output file", "", true};
	const command_line::arg_descriptor<uint32_t> arg_log_level = {"log-level", "", log_level};
	const command_line::arg_descriptor<std::string> arg_input = {"input", "Specify input has a hexadecimal string", ""};

	command_line::add_arg(desc_cmd_sett, arg_output_file);
	command_line::add_arg(desc_cmd_sett, arg_log_level);
	command_line::add_arg(desc_cmd_sett, arg_input);

	command_line::add_arg(desc_cmd_only, command_line::arg_help);

	po::options_description desc_options("Allowed options");
	desc_options.add(desc_cmd_only).add(desc_cmd_sett);

	po::variables_map vm;
	bool r = command_line::handle_error_helper(desc_options, [&]() {
		po::store(po::parse_command_line(argc, argv, desc_options), vm);
		po::notify(vm);
		return true;
	});
	if(!r)
		return 1;

	std::unique_ptr<gulps::gulps_output> out(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TIMESTAMP_ONLY));
	out->add_filter([](const gulps::message &msg, bool printed, bool logged) -> bool {
		return msg.lvl >= gulps::LEVEL_PRINT;
	});
	gulps::inst().add_output(std::move(out));

	if(command_line::get_arg(vm, command_line::arg_help))
	{
		GULPSF_PRINT("Ryo '{} ({})", RYO_RELEASE_NAME, RYO_VERSION_FULL);
		GULPS_PRINT(desc_options);
		return 0;
	}

	log_level = command_line::get_arg(vm, arg_log_level);
	input = command_line::get_arg(vm, arg_input);
	if(input.empty())
	{
		GULPS_ERROR("--input is mandatory");
		return 1;
	}

	std::string m_config_folder;

	std::ostream *output;
	std::ofstream *raw_data_file = NULL;
	if(command_line::has_arg(vm, arg_output_file))
	{
		output_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_output_file));

		const boost::filesystem::path dir_path = output_file_path.parent_path();
		if(!dir_path.empty())
		{
			if(boost::filesystem::exists(dir_path))
			{
				if(!boost::filesystem::is_directory(dir_path))
				{
					GULPS_ERROR("output directory path is a file: ", dir_path);
					return 1;
				}
			}
			else
			{
				if(!boost::filesystem::create_directory(dir_path))
				{
					GULPS_ERROR("Failed to create directory ", dir_path);
					return 1;
				}
			}
		}

		raw_data_file = new std::ofstream();
		raw_data_file->open(output_file_path.string(), std::ios_base::out | std::ios::trunc);
		if(raw_data_file->fail())
			return 1;
		output = raw_data_file;
	}
	else
	{
		output_file_path = "";
		output = &std::cout;
	}

	cryptonote::blobdata blob;
	if(!epee::string_tools::parse_hexstr_to_binbuff(input, blob))
	{
		GULPS_ERROR("Invalid hex input");
		GULPS_ERROR("Invalid hex input: ", input);
		return 1;
	}

	cryptonote::block block;
	cryptonote::transaction tx;
	std::vector<cryptonote::tx_extra_field> fields;
	if(cryptonote::parse_and_validate_block_from_blob(blob, block))
	{
		GULPS_PRINT("Parsed block:");
		GULPS_PRINT(cryptonote::obj_to_json_str(block));
	}
	else if(cryptonote::parse_and_validate_tx_from_blob(blob, tx))
	{
		GULPS_PRINT("Parsed transaction:");
		GULPS_PRINT(cryptonote::obj_to_json_str(tx));

		bool parsed = cryptonote::parse_tx_extra(tx.extra, fields);
		if(!parsed)
			GULPS_PRINT("Failed to parse tx_extra");

		if(!fields.empty())
		{
			print_extra_fields(fields);
		}
		else
		{
			GULPS_PRINT("No fields were found in tx_extra");
		}
	}
	else if(cryptonote::parse_tx_extra(std::vector<uint8_t>(blob.begin(), blob.end()), fields) && !fields.empty())
	{
		GULPS_PRINT("Parsed tx_extra:");
		print_extra_fields(fields);
	}
	else
	{
		GULPS_ERROR("Not a recognized CN type");
		return 1;
	}

	if(output->fail())
		return 1;
	output->flush();
	if(raw_data_file)
		delete raw_data_file;

	return 0;
}
