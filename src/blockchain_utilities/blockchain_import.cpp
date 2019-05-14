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

#define GULPS_CAT_MAJOR "blockch_import"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>

#include "blockchain_db/db_types.h"
#include "bootstrap_file.h"
#include "bootstrap_serialization.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_core.h"
#include "include_base_utils.h"
#include "serialization/binary_utils.h" // dump_binary(), parse_binary()
#include "serialization/json_utils.h"   // dump_json()
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "common/gulps.hpp"



namespace
{
// CONFIG
bool opt_batch = true;
bool opt_verify = true; // use add_new_block, which does verification before calling add_block
bool opt_resume = true;
bool opt_testnet = true;
bool opt_stagenet = true;

// number of blocks per batch transaction
// adjustable through command-line argument according to available RAM
#if ARCH_WIDTH != 32
uint64_t db_batch_size = 20000;
#else
// set a lower default batch size, pending possible LMDB issue with large transaction size
uint64_t db_batch_size = 100;
#endif

// when verifying, use a smaller default batch size so progress is more
// frequently saved
uint64_t db_batch_size_verify = 5000;

std::string refresh_string = "\r                                    \r";
}

namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

// db_mode: safe, fast, fastest
int get_db_flags_from_mode(const std::string &db_mode)
{
	int db_flags = 0;
	if(db_mode == "safe")
		db_flags = DBF_SAFE;
	else if(db_mode == "fast")
		db_flags = DBF_FAST;
	else if(db_mode == "fastest")
		db_flags = DBF_FASTEST;
	return db_flags;
}

int parse_db_arguments(const std::string &db_arg_str, std::string &db_type, int &db_flags)
{
	std::vector<std::string> db_args;
	boost::split(db_args, db_arg_str, boost::is_any_of("#"));
	db_type = db_args.front();
	boost::algorithm::trim(db_type);

	if(db_args.size() == 1)
	{
		return 0;
	}
	else if(db_args.size() > 2)
	{
		GULPS_ERROR( "unrecognized database argument format: " , db_arg_str );
		return 1;
	}

	std::string db_arg_str2 = db_args[1];
	boost::split(db_args, db_arg_str2, boost::is_any_of(","));

	// optionally use a composite mode instead of individual flags
	const std::unordered_set<std::string> db_modes{"safe", "fast", "fastest"};
	std::string db_mode;
	if(db_args.size() == 1)
	{
		if(db_modes.count(db_args[0]) > 0)
		{
			db_mode = db_args[0];
		}
	}
	if(!db_mode.empty())
	{
		db_flags = get_db_flags_from_mode(db_mode);
	}
	return 0;
}

int pop_blocks(cryptonote::core &core, int num_blocks)
{
	bool use_batch = opt_batch;

	if(use_batch)
		core.get_blockchain_storage().get_db().batch_start();

	int quit = 0;
	block popped_block;
	std::vector<transaction> popped_txs;
	for(int i = 0; i < num_blocks; ++i)
	{
		// simple_core.m_storage.pop_block_from_blockchain() is private, so call directly through db
		core.get_blockchain_storage().get_db().pop_block(popped_block, popped_txs);
		quit = 1;
	}

	if(use_batch)
	{
		if(quit > 1)
		{
			// There was an error, so don't commit pending data.
			// Destructor will abort write txn.
		}
		else
		{
			core.get_blockchain_storage().get_db().batch_stop();
		}
		core.get_blockchain_storage().get_db().show_stats();
	}

	return num_blocks;
}

int check_flush(cryptonote::core &core, std::list<block_complete_entry> &blocks, bool force)
{
	if(blocks.empty())
		return 0;
	if(!force && blocks.size() < db_batch_size)
		return 0;

	// wait till we can verify a full HOH without extra, for speed
	uint64_t new_height = core.get_blockchain_storage().get_db().height() + blocks.size();
	if(!force && new_height % HASH_OF_HASHES_STEP)
		return 0;

	std::list<crypto::hash> hashes;
	for(const auto &b : blocks)
	{
		cryptonote::block block;
		if(!parse_and_validate_block_from_blob(b.block, block))
		{
			GULPS_ERROR("Failed to parse block: ",
				   epee::string_tools::pod_to_hex(get_blob_hash(b.block)));
			core.cleanup_handle_incoming_blocks();
			return 1;
		}
		hashes.push_back(cryptonote::get_block_hash(block));
	}
	core.prevalidate_block_hashes(core.get_blockchain_storage().get_db().height(), hashes);

	core.prepare_handle_incoming_blocks(blocks);

	for(const block_complete_entry &block_entry : blocks)
	{
		// process transactions
		for(auto &tx_blob : block_entry.txs)
		{
			tx_verification_context tvc = AUTO_VAL_INIT(tvc);
			core.handle_incoming_tx(tx_blob, tvc, true, true, false);
			if(tvc.m_verifivation_failed)
			{
				GULPS_ERROR("transaction verification failed, tx_id = ",
					   epee::string_tools::pod_to_hex(get_blob_hash(tx_blob)));
				core.cleanup_handle_incoming_blocks();
				return 1;
			}
		}

		// process block

		block_verification_context bvc = boost::value_initialized<block_verification_context>();

		core.handle_incoming_block(block_entry.block, bvc, false); // <--- process block

		if(bvc.m_verifivation_failed)
		{
			GULPS_ERROR("Block verification failed, id = ",
				   epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block)));
			core.cleanup_handle_incoming_blocks();
			return 1;
		}
		if(bvc.m_marked_as_orphaned)
		{
			GULPS_ERROR("Block received at sync phase was marked as orphaned");
			core.cleanup_handle_incoming_blocks();
			return 1;
		}

	} // each download block
	if(!core.cleanup_handle_incoming_blocks())
		return 1;

	blocks.clear();
	return 0;
}

int import_from_file(cryptonote::core &core, const std::string &import_file_path, uint64_t block_stop = 0)
{
	// Reset stats, in case we're using newly created db, accumulating stats
	// from addition of genesis block.
	// This aligns internal db counts with importer counts.
	core.get_blockchain_storage().get_db().reset_stats();

	boost::filesystem::path fs_import_file_path(import_file_path);
	boost::system::error_code ec;
	if(!boost::filesystem::exists(fs_import_file_path, ec))
	{
		GULPS_ERROR("bootstrap file not found: ", fs_import_file_path);
		return false;
	}

	uint64_t start_height = 1, seek_height;
	if(opt_resume)
		start_height = core.get_blockchain_storage().get_current_blockchain_height();

	seek_height = start_height;
	BootstrapFile bootstrap;
	std::streampos pos;
	// BootstrapFile bootstrap(import_file_path);
	uint64_t total_source_blocks = bootstrap.count_blocks(import_file_path, pos, seek_height);
	GULPS_INFOF("bootstrap file last block number: {} (zero-based height)  total blocks:{}", std::to_string(total_source_blocks - 1), std::to_string(total_source_blocks));

	if(total_source_blocks - 1 <= start_height)
	{
		return false;
	}

	GULPS_PRINT( "\nPreparing to read blocks...\n" );

	std::ifstream import_file;
	import_file.open(import_file_path, std::ios_base::binary | std::ifstream::in);

	uint64_t h = 0;
	uint64_t num_imported = 0;
	if(import_file.fail())
	{
		GULPS_ERROR("import_file.open() fail");
		return false;
	}

	// 4 byte magic + (currently) 1024 byte header structures
	bootstrap.seek_to_first_chunk(import_file);

	std::string str1;
	char buffer1[1024];
	char buffer_block[BUFFER_SIZE];
	block b;
	transaction tx;
	int quit = 0;
	uint64_t bytes_read;

	// Note that a new blockchain will start with block number 0 (total blocks: 1)
	// due to genesis block being added at initialization.

	if(!block_stop)
	{
		block_stop = total_source_blocks - 1;
	}

	// These are what we'll try to use, and they don't have to be a determination
	// from source and destination blockchains, but those are the defaults.
	GULPS_INFO("start block: {}  stop block: {}" , std::to_string(start_height), std::to_string(block_stop));

	bool use_batch = opt_batch && !opt_verify;

	GULPS_INFO("Reading blockchain from bootstrap file...\n");

	std::list<block_complete_entry> blocks;

	// Skip to start_height before we start adding.
	{
		bool q2 = false;
		import_file.seekg(pos);
		bytes_read = bootstrap.count_bytes(import_file, start_height - seek_height, h, q2);
		if(q2)
		{
			quit = 2;
			goto quitting;
		}
		h = start_height;
	}

	if(use_batch)
	{
		uint64_t bytes, h2;
		bool q2;
		pos = import_file.tellg();
		bytes = bootstrap.count_bytes(import_file, db_batch_size, h2, q2);
		if(import_file.eof())
			import_file.clear();
		import_file.seekg(pos);
		core.get_blockchain_storage().get_db().batch_start(db_batch_size, bytes);
	}
	while(!quit)
	{
		uint32_t chunk_size;
		import_file.read(buffer1, sizeof(chunk_size));
		// TODO: bootstrap.read_chunk();
		if(!import_file)
		{
			GULPS_PRINT( refresh_string);
			GULPS_INFO("End of file reached");
			quit = 1;
			break;
		}
		bytes_read += sizeof(chunk_size);

		str1.assign(buffer1, sizeof(chunk_size));
		if(!::serialization::parse_binary(str1, chunk_size))
		{
			throw std::runtime_error("Error in deserialization of chunk size");
		}
		GULPS_LOGF_L1("chunk_size: {}" , chunk_size);

		if(chunk_size > BUFFER_SIZE)
		{
			GULPS_WARNF("WARNING: chunk_size {} > BUFFER_SIZE {}", chunk_size, BUFFER_SIZE);
			throw std::runtime_error("Aborting: chunk size exceeds buffer size");
		}
		if(chunk_size > CHUNK_SIZE_WARNING_THRESHOLD)
		{
			GULPS_INFOF("NOTE: chunk_size {} > {}",  chunk_size, CHUNK_SIZE_WARNING_THRESHOLD);
		}
		else if(chunk_size == 0)
		{
			GULPS_ERROR("ERROR: chunk_size == 0");
			return 2;
		}
		import_file.read(buffer_block, chunk_size);
		if(!import_file)
		{
			if(import_file.eof())
			{
				GULPS_PRINT( refresh_string);
				GULPS_INFO("End of file reached - file was truncated");
				quit = 1;
				break;
			}
			else
			{
				GULPS_ERRORF("ERROR: unexpected end of file: bytes read before error: {} of chunk_size {}",
						import_file.gcount(), chunk_size);
				return 2;
			}
		}
		bytes_read += chunk_size;
		GULPS_LOGF_L1("Total bytes read: {}" , bytes_read);

		if(h > block_stop)
		{
			GULPS_PRINTF(refresh_string, "block {} / {}\n\n", h - 1, block_stop);
			GULPS_INFOF("Specified block number reached - stopping.  block: {}  total blocks: {}", h - 1, h);
			quit = 1;
			break;
		}

		try
		{
			str1.assign(buffer_block, chunk_size);
			bootstrap::block_package bp;
			if(!::serialization::parse_binary(str1, bp))
				throw std::runtime_error("Error in deserialization of chunk");

			int display_interval = 1000;
			int progress_interval = 10;
			// NOTE: use of NUM_BLOCKS_PER_CHUNK is a placeholder in case multi-block chunks are later supported.
			for(int chunk_ind = 0; chunk_ind < NUM_BLOCKS_PER_CHUNK; ++chunk_ind)
			{
				++h;
				if((h - 1) % display_interval == 0)
				{
					GULPS_PRINT( refresh_string);
					GULPS_LOGF_L1("loading block number {}" , h - 1);
				}
				else
				{
					GULPS_LOGF_L1("loading block number {}" , h - 1);
				}
				b = bp.block;
				GULPS_LOGF_L1("block prev_id: {}", b.prev_id);

				if((h - 1) % progress_interval == 0)
				{
					GULPS_PRINTF("{}block {} / {}\r", refresh_string,  h - 1, block_stop);
				}

				if(opt_verify)
				{
					cryptonote::blobdata block;
					cryptonote::block_to_blob(bp.block, block);
					std::list<cryptonote::blobdata> txs;
					for(const auto &tx : bp.txs)
					{
						txs.push_back(cryptonote::blobdata());
						cryptonote::tx_to_blob(tx, txs.back());
					}
					blocks.push_back({block, txs});
					int ret = check_flush(core, blocks, false);
					if(ret)
					{
						quit = 2; // make sure we don't commit partial block data
						break;
					}
				}
				else
				{
					std::vector<transaction> txs;
					std::vector<transaction> archived_txs;

					archived_txs = bp.txs;

					// tx number 1: coinbase tx
					// tx number 2 onwards: archived_txs
					for(const transaction &tx : archived_txs)
					{
						// add blocks with verification.
						// for Blockchain and blockchain_storage add_new_block().
						// for add_block() method, without (much) processing.
						// don't add coinbase transaction to txs.
						//
						// because add_block() calls
						// add_transaction(blk_hash, blk.miner_tx) first, and
						// then a for loop for the transactions in txs.
						txs.push_back(tx);
					}

					size_t block_size;
					difficulty_type cumulative_difficulty;
					uint64_t coins_generated;

					block_size = bp.block_size;
					cumulative_difficulty = bp.cumulative_difficulty;
					coins_generated = bp.coins_generated;

					try
					{
						core.get_blockchain_storage().get_db().add_block(b, block_size, cumulative_difficulty, coins_generated, txs);
					}
					catch(const std::exception &e)
					{
						GULPS_PRINT( refresh_string);
						GULPS_ERROR("Error adding block to blockchain: ", e.what());
						quit = 2; // make sure we don't commit partial block data
						break;
					}

					if(use_batch)
					{
						if((h - 1) % db_batch_size == 0)
						{
							uint64_t bytes, h2;
							bool q2;
							GULPS_PRINT( refresh_string);
							// zero-based height
							GULPS_PRINTF("\n[- batch commit at height {} -]\n", h - 1);
							core.get_blockchain_storage().get_db().batch_stop();
							pos = import_file.tellg();
							bytes = bootstrap.count_bytes(import_file, db_batch_size, h2, q2);
							import_file.seekg(pos);
							core.get_blockchain_storage().get_db().batch_start(db_batch_size, bytes);
							GULPS_PRINT( "\n");
							core.get_blockchain_storage().get_db().show_stats();
						}
					}
				}
				++num_imported;
			}
		}
		catch(const std::exception &e)
		{
			GULPS_PRINT( refresh_string);
			GULPS_ERRORF("exception while reading from file, height={}: {}", h, e.what());
			return 2;
		}
	} // while

quitting:
	import_file.close();

	if(opt_verify)
	{
		int ret = check_flush(core, blocks, true);
		if(ret)
			return ret;
	}

	if(use_batch)
	{
		if(quit > 1)
		{
			// There was an error, so don't commit pending data.
			// Destructor will abort write txn.
		}
		else
		{
			core.get_blockchain_storage().get_db().batch_stop();
		}
	}

	core.get_blockchain_storage().get_db().show_stats();
	GULPS_INFOF("Number of blocks imported: {}" , num_imported);
	if(h > 0)
		// TODO: if there was an error, the last added block is probably at zero-based height h-2
		GULPS_INFOF("Finished at block: {}  total blocks: {}", h - 1, h);

	GULPS_PRINT( "\n");
	return 0;
}

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
	uint64_t num_blocks = 0;
	uint64_t block_stop = 0;
	std::string m_config_folder;
	std::string db_arg_str;

	tools::on_startup();

	std::string import_file_path;

	po::options_description desc_cmd_only("Command line options");
	po::options_description desc_cmd_sett("Command line options and settings options");
	const command_line::arg_descriptor<std::string> arg_input_file = {"input-file", "Specify input file", "", true};
	const command_line::arg_descriptor<std::string> arg_log_level = {"log-level", "0-4 or categories", ""};
	const command_line::arg_descriptor<uint64_t> arg_block_stop = {"block-stop", "Stop at block number", block_stop};
	const command_line::arg_descriptor<uint64_t> arg_batch_size = {"batch-size", "", db_batch_size};
	const command_line::arg_descriptor<uint64_t> arg_pop_blocks = {"pop-blocks", "Remove blocks from end of blockchain", num_blocks};
	const command_line::arg_descriptor<bool> arg_drop_hf = {"drop-hard-fork", "Drop hard fork subdbs", false};
	const command_line::arg_descriptor<bool> arg_count_blocks = {
		"count-blocks", "Count blocks in bootstrap file and exit", false};
	const command_line::arg_descriptor<std::string> arg_database = {
		"database", available_dbs.c_str(), default_db_type};
	const command_line::arg_descriptor<bool> arg_verify = {"guard-against-pwnage",
														   "Verify blocks and transactions during import (only disable if you exported the file yourself)", true};
	const command_line::arg_descriptor<bool> arg_batch = {"batch",
														  "Batch transactions for faster import", true};
	const command_line::arg_descriptor<bool> arg_resume = {"resume",
														   "Resume from current height if output database already exists", true};

	command_line::add_arg(desc_cmd_sett, arg_input_file);
	command_line::add_arg(desc_cmd_sett, arg_log_level);
	command_line::add_arg(desc_cmd_sett, arg_database);
	command_line::add_arg(desc_cmd_sett, arg_batch_size);
	command_line::add_arg(desc_cmd_sett, arg_block_stop);

	command_line::add_arg(desc_cmd_only, arg_count_blocks);
	command_line::add_arg(desc_cmd_only, arg_pop_blocks);
	command_line::add_arg(desc_cmd_only, arg_drop_hf);
	command_line::add_arg(desc_cmd_only, command_line::arg_help);

	// call add_options() directly for these arguments since
	// command_line helpers support only boolean switch, not boolean argument
	desc_cmd_sett.add_options()(arg_verify.name, make_semantic(arg_verify), arg_verify.description)(arg_batch.name, make_semantic(arg_batch), arg_batch.description)(arg_resume.name, make_semantic(arg_resume), arg_resume.description);

	po::options_description desc_options("Allowed options");
	desc_options.add(desc_cmd_only).add(desc_cmd_sett);
	cryptonote::core::init_options(desc_options);

	po::variables_map vm;
	bool r = command_line::handle_error_helper(desc_options, [&]() {
		po::store(po::parse_command_line(argc, argv, desc_options), vm);
		po::notify(vm);
		return true;
	});
	if(!r)
		return 1;

	opt_verify = command_line::get_arg(vm, arg_verify);
	opt_batch = command_line::get_arg(vm, arg_batch);
	opt_resume = command_line::get_arg(vm, arg_resume);
	block_stop = command_line::get_arg(vm, arg_block_stop);
	db_batch_size = command_line::get_arg(vm, arg_batch_size);

	m_config_folder = command_line::get_arg(vm, cryptonote::arg_data_dir);
	db_arg_str = command_line::get_arg(vm, arg_database);

	gulps::inst().set_thread_tag("BLOCKCH_IMPORT");

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
			GULPS_ERRORF("Failed to parse filter string {}", log_level);
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
		GULPS_PRINT("Ryo '", RYO_RELEASE_NAME, "' (", RYO_VERSION_FULL, ")\n\n");
		GULPS_PRINT( desc_options );
		return 0;
	}

	if(!opt_batch && !command_line::is_arg_defaulted(vm, arg_batch_size))
	{
		GULPS_ERROR( "Error: batch-size set, but batch option not enabled" );
		return 1;
	}
	if(!db_batch_size)
	{
		GULPS_ERROR( "Error: batch-size must be > 0" );
		return 1;
	}
	if(opt_verify && command_line::is_arg_defaulted(vm, arg_batch_size))
	{
		// usually want batch size default lower if verify on, so progress can be
		// frequently saved.
		//
		// currently, with Windows, default batch size is low, so ignore
		// default db_batch_size_verify unless it's even lower
		if(db_batch_size > db_batch_size_verify)
		{
			db_batch_size = db_batch_size_verify;
		}
	}
	opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
	opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
	if(opt_testnet && opt_stagenet)
	{
		GULPS_ERROR( "Error: Can't specify more than one of --testnet and --stagenet" );
		return 1;
	}

	GULPS_INFO("Starting...");

	boost::filesystem::path fs_import_file_path;

	if(command_line::has_arg(vm, arg_input_file))
		fs_import_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_input_file));
	else
		fs_import_file_path = boost::filesystem::path(m_config_folder) / "export" / BLOCKCHAIN_RAW;

	import_file_path = fs_import_file_path.string();

	if(command_line::has_arg(vm, arg_count_blocks))
	{
		BootstrapFile bootstrap;
		bootstrap.count_blocks(import_file_path);
		return 0;
	}

	std::string db_type;
	int db_flags = 0;
	int res = 0;
	res = parse_db_arguments(db_arg_str, db_type, db_flags);
	if(res)
	{
		GULPS_ERROR( "Error parsing database argument(s)" );
		return 1;
	}

	if(!cryptonote::blockchain_valid_db_type(db_type))
	{
		GULPS_ERROR( "Invalid database type: " ,  db_type );
		return 1;
	}

	GULPS_INFO("database: " , db_type);
	GULPS_INFOF("database flags: {}" , db_flags);
	GULPS_INFO("verify:  " , std::boolalpha , opt_verify , std::noboolalpha);
	if(opt_batch)
	{
		GULPS_INFO("batch:   ", std::boolalpha, opt_batch, std::noboolalpha, " batch size: ", db_batch_size);
	}
	else
	{
		GULPS_INFO("batch:   " , std::boolalpha , opt_batch , std::noboolalpha);
	}
	GULPS_INFO("resume:  " , std::boolalpha , opt_resume , std::noboolalpha);
	GULPS_INFO("nettype: ", (opt_testnet ? "testnet" : opt_stagenet ? "stagenet" : "mainnet"));

	GULPS_INFO("bootstrap file path: " , import_file_path);
	GULPS_INFO("database path:       " , m_config_folder);

	cryptonote::cryptonote_protocol_stub pr; //TODO: stub only for this kind of test, make real validation of relayed objects
	cryptonote::core core(&pr);

	try
	{

		core.disable_dns_checkpoints(true);
		if(!core.init(vm, NULL))
		{
			GULPS_ERROR( "Failed to initialize core" );
			return 1;
		}
		core.get_blockchain_storage().get_db().set_batch_transactions(true);

		if(!command_line::is_arg_defaulted(vm, arg_pop_blocks))
		{
			num_blocks = command_line::get_arg(vm, arg_pop_blocks);
			GULPS_INFOF("height: {}" , core.get_blockchain_storage().get_current_blockchain_height());
			pop_blocks(core, num_blocks);
			GULPS_INFOF("height: {}" , core.get_blockchain_storage().get_current_blockchain_height());
			return 0;
		}

		if(!command_line::is_arg_defaulted(vm, arg_drop_hf))
		{
			GULPS_INFO("Dropping hard fork tables...");
			core.get_blockchain_storage().get_db().drop_hard_fork_info();
			core.deinit();
			return 0;
		}

		import_from_file(core, import_file_path, block_stop);

		// ensure db closed
		//   - transactions properly checked and handled
		//   - disk sync if needed
		//
		core.deinit();
	}
	catch(const DB_ERROR &e)
	{
		GULPS_PRINT("Error loading blockchain db: ", e.what(), " -- shutting down now" );
		core.deinit();
		return 1;
	}

	return 0;

	GULPS_CATCH_ENTRY("Import error", 1);
}
