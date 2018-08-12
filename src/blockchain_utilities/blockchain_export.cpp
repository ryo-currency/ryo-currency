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

#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/db_types.h"
#include "blocksdat_file.h"
#include "bootstrap_file.h"
#include "common/command_line.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_pool.h"
#include "version.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;

int main(int argc, char *argv[])
{
	TRY_ENTRY();

	epee::string_tools::set_module_name_and_folder(argv[0]);

	std::string default_db_type = "lmdb";

	std::string available_dbs = cryptonote::blockchain_db_types(", ");
	available_dbs = "available: " + available_dbs;

	uint32_t log_level = 0;
	uint64_t block_stop = 0;
	bool blocks_dat = false;

	tools::on_startup();

	boost::filesystem::path output_file_path;

	po::options_description desc_cmd_only("Command line options");
	po::options_description desc_cmd_sett("Command line options and settings options");
	const command_line::arg_descriptor<std::string> arg_output_file = {"output-file", "Specify output file", "", true};
	const command_line::arg_descriptor<std::string> arg_log_level = {"log-level", "0-4 or categories", ""};
	const command_line::arg_descriptor<uint64_t> arg_block_stop = {"block-stop", "Stop at block number", block_stop};
	const command_line::arg_descriptor<std::string> arg_database = {
		"database", available_dbs.c_str(), default_db_type};
	const command_line::arg_descriptor<bool> arg_blocks_dat = {"blocksdat", "Output in blocks.dat format", blocks_dat};

	command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
	command_line::add_arg(desc_cmd_sett, arg_output_file);
	command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
	command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
	command_line::add_arg(desc_cmd_sett, arg_log_level);
	command_line::add_arg(desc_cmd_sett, arg_database);
	command_line::add_arg(desc_cmd_sett, arg_block_stop);
	command_line::add_arg(desc_cmd_sett, arg_blocks_dat);

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

	if(command_line::get_arg(vm, command_line::arg_help))
	{
		std::cout << "Ryo '" << RYO_RELEASE_NAME << "' (" << RYO_VERSION_FULL << ")" << ENDL << ENDL;
		std::cout << desc_options << std::endl;
		return 0;
	}

	mlog_configure(mlog_get_default_log_path("ryo-blockchain-export.log"), true);
	if(!command_line::is_arg_defaulted(vm, arg_log_level))
		mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
	else
		mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());
	block_stop = command_line::get_arg(vm, arg_block_stop);

	LOG_PRINT_L0("Starting...");

	bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
	bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
	if(opt_testnet && opt_stagenet)
	{
		std::cerr << "Can't specify more than one of --testnet and --stagenet" << std::endl;
		return 1;
	}
	bool opt_blocks_dat = command_line::get_arg(vm, arg_blocks_dat);

	std::string m_config_folder;

	m_config_folder = command_line::get_arg(vm, cryptonote::arg_data_dir);

	std::string db_type = command_line::get_arg(vm, arg_database);
	if(!cryptonote::blockchain_valid_db_type(db_type))
	{
		std::cerr << "Invalid database type: " << db_type << std::endl;
		return 1;
	}

	if(command_line::has_arg(vm, arg_output_file))
		output_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_output_file));
	else
		output_file_path = boost::filesystem::path(m_config_folder) / "export" / BLOCKCHAIN_RAW;
	LOG_PRINT_L0("Export output file: " << output_file_path.string());

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
	LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
	Blockchain *core_storage = NULL;
	tx_memory_pool m_mempool(*core_storage);
	core_storage = new Blockchain(m_mempool);

	BlockchainDB *db = new_db(db_type);
	if(db == NULL)
	{
		LOG_ERROR("Attempted to use non-existent database type: " << db_type);
		throw std::runtime_error("Attempting to use non-existent database type");
	}
	LOG_PRINT_L0("database: " << db_type);

	boost::filesystem::path folder(m_config_folder);
	folder /= db->get_db_name();
	const std::string filename = folder.string();

	LOG_PRINT_L0("Loading blockchain from folder " << filename << " ...");
	try
	{
		db->open(filename, DBF_RDONLY);
	}
	catch(const std::exception &e)
	{
		LOG_PRINT_L0("Error opening database: " << e.what());
		return 1;
	}
	r = core_storage->init(db, opt_testnet ? cryptonote::TESTNET : opt_stagenet ? cryptonote::STAGENET : cryptonote::MAINNET);

	CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
	LOG_PRINT_L0("Source blockchain storage initialized OK");
	LOG_PRINT_L0("Exporting blockchain raw data...");

	if(opt_blocks_dat)
	{
		BlocksdatFile blocksdat;
		r = blocksdat.store_blockchain_raw(core_storage, NULL, output_file_path, block_stop);
	}
	else
	{
		BootstrapFile bootstrap;
		r = bootstrap.store_blockchain_raw(core_storage, NULL, output_file_path, block_stop);
	}
	CHECK_AND_ASSERT_MES(r, 1, "Failed to export blockchain raw data");
	LOG_PRINT_L0("Blockchain raw data exported OK");
	return 0;

	CATCH_ENTRY("Export error", 1);
}
