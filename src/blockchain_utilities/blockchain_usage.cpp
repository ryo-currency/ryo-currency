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

#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/db_types.h"
#include "common/command_line.h"
#include "common/varint.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_pool.h"
#include "version.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("blockch_usage");

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

struct output_data
{
	uint64_t amount;
	uint64_t index;
	mutable bool coinbase;
	mutable uint64_t height;
	output_data(uint64_t a, uint64_t i, bool cb, uint64_t h) : amount(a), index(i), coinbase(cb), height(h) {}
	bool operator==(const output_data &other) const { return other.amount == amount && other.index == index; }
	void info(bool c, uint64_t h) const
	{
		coinbase = c;
		height = h;
	}
};
namespace std
{
template <>
struct hash<output_data>
{
	size_t operator()(const output_data &od) const
	{
		const uint64_t data[2] = {od.amount, od.index};
		crypto::hash h;
		crypto::cn_fast_hash(data, 2 * sizeof(uint64_t), h);
		return reinterpret_cast<const std::size_t &>(h);
	}
};
}

struct reference
{
	uint64_t height;
	uint64_t ring_size;
	uint64_t position;
	reference(uint64_t h, uint64_t rs, uint64_t p) : height(h), ring_size(rs), position(p) {}
};

gulps_log_level log_scr;

int main(int argc, char *argv[])
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

	GULPS_TRY_ENTRY();

	epee::string_tools::set_module_name_and_folder(argv[0]);

	std::string default_db_type = "lmdb";

	std::string available_dbs = cryptonote::blockchain_db_types(", ");
	available_dbs = "available: " + available_dbs;

	uint32_t log_level = 0;

	tools::on_startup();

	boost::filesystem::path output_file_path;

	po::options_description desc_cmd_only("Command line options");
	po::options_description desc_cmd_sett("Command line options and settings options");
	const command_line::arg_descriptor<std::string> arg_log_level = {"log-level", "0-4 or categories", ""};
	const command_line::arg_descriptor<std::string> arg_database = {
		"database", available_dbs.c_str(), default_db_type};
	const command_line::arg_descriptor<bool> arg_rct_only = {"rct-only", "Only work on ringCT outputs", false};
	const command_line::arg_descriptor<std::string> arg_input = {"input", ""};

	command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
	command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
	command_line::add_arg(desc_cmd_sett, arg_log_level);
	command_line::add_arg(desc_cmd_sett, arg_database);
	command_line::add_arg(desc_cmd_sett, arg_rct_only);
	command_line::add_arg(desc_cmd_sett, arg_input);
	command_line::add_arg(desc_cmd_only, command_line::arg_help);

	po::options_description desc_options("Allowed options");
	desc_options.add(desc_cmd_only).add(desc_cmd_sett);

	po::positional_options_description positional_options;
	positional_options.add(arg_input.name, -1);

	po::variables_map vm;
	bool r = command_line::handle_error_helper(desc_options, [&]() {
		auto parser = po::command_line_parser(argc, argv).options(desc_options).positional(positional_options);
		po::store(parser.run(), vm);
		po::notify(vm);
		return true;
	});
	if(!r)
		return 1;

	gulps::inst().set_thread_tag("BLOCKCH_USAGE");

	//Temp error output
	std::unique_ptr<gulps::gulps_output> out(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TIMESTAMP_ONLY));
	out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { return msg.lvl >= gulps::LEVEL_ERROR; });
	auto temp_handle = gulps::inst().add_output(std::move(out));

	if(!command_line::is_arg_defaulted(vm, arg_log_level))
	{
		if(!log_scr.parse_cat_string(command_line::get_arg(vm, arg_log_level).c_str()))
		{
			GULPS_ERROR("Failed to parse filter string ", command_line::get_arg(vm, arg_log_level).c_str());
			return 1;
		}
	}
	else
	{
		if(!log_scr.parse_cat_string(std::to_string(log_level).c_str()))
		{
			GULPSF_ERROR("Failed to parse filter string {}", log_level);
			return 1;
		}
	}

	gulps::inst().remove_output(temp_handle);

	if(log_scr.is_active())
	{
		std::unique_ptr<gulps::gulps_output> out(new gulps::gulps_print_output(gulps::COLOR_WHITE, gulps::TEXT_ONLY));
		out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool {
				if(msg.out != gulps::OUT_LOG_0 && msg.out != gulps::OUT_USER_0)
					return false;
				if(printed)
					return false;
				return log_scr.match_msg(msg);
				});
		gulps::inst().add_output(std::move(out));
	}

	if(command_line::get_arg(vm, command_line::arg_help))
	{
		GULPS_PRINT("Ryo '", RYO_RELEASE_NAME, "' (", RYO_VERSION_FULL, ")");
		GULPS_PRINT(desc_options);
		return 0;
	}

	GULPS_PRINT("Starting...");

	bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
	bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
	network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
	bool opt_rct_only = command_line::get_arg(vm, arg_rct_only);

	std::string db_type = command_line::get_arg(vm, arg_database);
	if(!cryptonote::blockchain_valid_db_type(db_type))
	{
		GULPS_ERROR("Invalid database type: ", db_type);
		return 1;
	}

	// If we wanted to use the memory pool, we would set up a fake_core.

	// Use Blockchain instead of lower-level BlockchainDB for two reasons:
	// 1. Blockchain has the init() method for easy setup
	// 2. exporter needs to use get_current_blockchain_height(), get_block_id_by_height(), get_block_by_hash()
	//
	// cannot match blockchain_storage setup above with just one line,
	// e.g.
	//   Blockchain* core_storage = new Blockchain(NULL);
	// because unlike blockchain_storage constructor, which takes a pointer to
	// tx_memory_pool, Blockchain's constructor takes tx_memory_pool object.
	GULPS_PRINT("Initializing source blockchain (BlockchainDB)");
	const std::string input = command_line::get_arg(vm, arg_input);
	std::unique_ptr<Blockchain> core_storage;
	tx_memory_pool m_mempool(*core_storage);
	core_storage.reset(new Blockchain(m_mempool));
	BlockchainDB *db = new_db(db_type);
	if(db == NULL)
	{
		GULPS_LOG_ERROR("Attempted to use non-existent database type: ", db_type);
		throw std::runtime_error("Attempting to use non-existent database type");
	}
	GULPS_PRINT("database: " , db_type);

	const std::string filename = input;
	GULPS_PRINT("Loading blockchain from folder " , filename , " ...");

	try
	{
		db->open(filename, DBF_RDONLY);
	}
	catch(const std::exception &e)
	{
		GULPS_ERROR("Error opening database: " , e.what());
		return 1;
	}
	r = core_storage->init(db, net_type);

	GULPS_CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
	GULPS_PRINT("Source blockchain storage initialized OK");

	GULPS_PRINT("Building usage patterns...");

	size_t done = 0;
	std::unordered_map<output_data, std::list<reference>> outputs;
	std::unordered_map<uint64_t, uint64_t> indices;

	GULPS_PRINT("Reading blockchain from " , input);
	core_storage->for_all_transactions([&](const crypto::hash &hash, const cryptonote::transaction &tx) -> bool {
		const bool coinbase = tx.vin.size() == 1 && tx.vin[0].type() == typeid(txin_gen);
		const uint64_t height = core_storage->get_db().get_tx_block_height(hash);

		// create new outputs
		for(const auto &out : tx.vout)
		{
			if(opt_rct_only && out.amount)
				continue;
			uint64_t index = indices[out.amount]++;
			output_data od(out.amount, indices[out.amount], coinbase, height);
			auto itb = outputs.emplace(od, std::list<reference>());
			itb.first->first.info(coinbase, height);
		}

		for(const auto &in : tx.vin)
		{
			if(in.type() != typeid(txin_to_key))
				continue;
			const auto &txin = boost::get<txin_to_key>(in);
			if(opt_rct_only && txin.amount != 0)
				continue;

			const std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
			for(size_t n = 0; n < txin.key_offsets.size(); ++n)
			{
				output_data od(txin.amount, absolute[n], coinbase, height);
				outputs[od].push_back(reference(height, txin.key_offsets.size(), n));
			}
		}
		return true;
	});

	std::unordered_map<uint64_t, uint64_t> counts;
	size_t total = 0;
	for(const auto &out : outputs)
	{
		counts[out.second.size()]++;
		total++;
	}
	for(const auto &c : counts)
	{
		float percent = 100.f * c.second / total;
		GULPSF_INFO("{} outputs used {} times ({}%)", c.second, c.first, percent);
	}

	GULPS_PRINT("Blockchain usage exported OK");
	return 0;

	GULPS_CATCH_ENTRY("Export error", 1);
}
