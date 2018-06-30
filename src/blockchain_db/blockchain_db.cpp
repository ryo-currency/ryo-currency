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

#include <boost/range/adaptor/reversed.hpp>

#include "blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

#include "lmdb/db_lmdb.h"
#ifdef BERKELEY_DB
#include "berkeleydb/db_bdb.h"
#endif

static const char *db_types[] = {
	"lmdb",
#ifdef BERKELEY_DB
	"berkeley",
#endif
	NULL};

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "blockchain.db"

using epee::string_tools::pod_to_hex;

namespace cryptonote
{

bool blockchain_valid_db_type(const std::string &db_type)
{
	int i;
	for(i = 0; db_types[i]; i++)
	{
		if(db_types[i] == db_type)
			return true;
	}
	return false;
}

std::string blockchain_db_types(const std::string &sep)
{
	int i;
	std::string ret = "";
	for(i = 0; db_types[i]; i++)
	{
		if(i)
			ret += sep;
		ret += db_types[i];
	}
	return ret;
}

std::string arg_db_type_description = "Specify database type, available: " + cryptonote::blockchain_db_types(", ");
const command_line::arg_descriptor<std::string> arg_db_type = {
	"db-type", arg_db_type_description.c_str(), DEFAULT_DB_TYPE};
const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
	"db-sync-mode", "Specify sync option, using format [safe|fast|fastest]:[sync|async]:[nblocks_per_sync].", "fast:async:1000"};
const command_line::arg_descriptor<bool> arg_db_salvage = {
	"db-salvage", "Try to salvage a blockchain database if it seems corrupted", false};

BlockchainDB *new_db(const std::string &db_type)
{
	if(db_type == "lmdb")
		return new BlockchainLMDB();
#if defined(BERKELEY_DB)
	if(db_type == "berkeley")
		return new BlockchainBDB();
#endif
	return NULL;
}

void BlockchainDB::init_options(boost::program_options::options_description &desc)
{
	command_line::add_arg(desc, arg_db_type);
	command_line::add_arg(desc, arg_db_sync_mode);
	command_line::add_arg(desc, arg_db_salvage);
}

void BlockchainDB::pop_block()
{
	block blk;
	std::vector<transaction> txs;
	pop_block(blk, txs);
}

void BlockchainDB::add_transaction(const crypto::hash &blk_hash, const transaction &tx, const crypto::hash *tx_hash_ptr)
{
	bool miner_tx = false;
	crypto::hash tx_hash;
	
	if(!tx_hash_ptr)
	{
		// should only need to compute hash for miner transactions
		tx_hash = get_transaction_hash(tx);
		LOG_PRINT_L3("null tx_hash_ptr - needed to compute: " << tx_hash);
	}
	else
	{
		tx_hash = *tx_hash_ptr;
	}

	for(const txin_v &tx_input : tx.vin)
	{
		if(tx_input.type() == typeid(txin_to_key))
		{
			add_spent_key(boost::get<txin_to_key>(tx_input).k_image);
		}
		else if(tx_input.type() == typeid(txin_gen))
		{
			/* nothing to do here */
			miner_tx = true;
		}
		else
		{
			LOG_PRINT_L1("Unsupported input type, removing key images and aborting transaction addition");
			for(const txin_v &tx_input : tx.vin)
			{
				if(tx_input.type() == typeid(txin_to_key))
				{
					remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
				}
			}
			return;
		}
	}

	uint64_t tx_id = add_transaction_data(blk_hash, tx, tx_hash);

	std::vector<uint64_t> amount_output_indices;

	// iterate tx.vout using indices instead of C++11 foreach syntax because
	// we need the index
	for(uint64_t i = 0; i < tx.vout.size(); ++i)
	{
		// miner v2 txes have their coinbase output in one single out to save space,
		// and we store them as rct outputs with an identity mask
		if(miner_tx)
		{
			cryptonote::tx_out vout = tx.vout[i];
			rct::key commitment = rct::zeroCommit(vout.amount);

			if(m_hardfork->get_current_version_num() <= 3 && tx.version > 2)
				vout.amount = 1; //Put those on a different index to fix a bug
			else
				vout.amount = 0;

			amount_output_indices.push_back(add_output(tx_hash, vout, i, tx.unlock_time, &commitment));
		}
		else
		{
			if(tx.vout[i].amount != 0)
			{
				//Better be safe than sorry
				LOG_PRINT_L1("Unsupported index type, removing key images and aborting transaction addition");
				for(const txin_v &tx_input : tx.vin)
				{
					if(tx_input.type() == typeid(txin_to_key))
						remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
				}
				return;
			}

			amount_output_indices.push_back(add_output(tx_hash, tx.vout[i], i, tx.unlock_time, &tx.rct_signatures.outPk[i].mask));
		}
	}

	add_tx_amount_output_indices(tx_id, amount_output_indices);
}

uint64_t BlockchainDB::add_block(const block &blk, const size_t &block_size, const difficulty_type &cumulative_difficulty, const uint64_t &coins_generated, const std::vector<transaction> &txs)
{
	// sanity
	if(blk.tx_hashes.size() != txs.size())
		throw std::runtime_error("Inconsistent tx/hashes sizes");

	block_txn_start(false);

	TIME_MEASURE_START(time1);
	crypto::hash blk_hash = get_block_hash(blk);
	TIME_MEASURE_FINISH(time1);
	time_blk_hash += time1;

	uint64_t prev_height = height();

	// call out to add the transactions

	time1 = epee::misc_utils::get_tick_count();
	add_transaction(blk_hash, blk.miner_tx);
	int tx_i = 0;
	crypto::hash tx_hash = crypto::null_hash;
	for(const transaction &tx : txs)
	{
		tx_hash = blk.tx_hashes[tx_i];
		add_transaction(blk_hash, tx, &tx_hash);
		++tx_i;
	}
	TIME_MEASURE_FINISH(time1);
	time_add_transaction += time1;

	// call out to subclass implementation to add the block & metadata
	time1 = epee::misc_utils::get_tick_count();
	add_block(blk, block_size, cumulative_difficulty, coins_generated, blk_hash);
	TIME_MEASURE_FINISH(time1);
	time_add_block1 += time1;

	m_hardfork->add(blk, prev_height);

	block_txn_stop();

	++num_calls;

	return prev_height;
}

void BlockchainDB::set_hard_fork(HardFork *hf)
{
	m_hardfork = hf;
}

void BlockchainDB::pop_block(block &blk, std::vector<transaction> &txs)
{
	blk = get_top_block();

	remove_block();

	for(const auto &h : boost::adaptors::reverse(blk.tx_hashes))
	{
		txs.push_back(get_tx(h));
		remove_transaction(h);
	}
	remove_transaction(get_transaction_hash(blk.miner_tx));
}

bool BlockchainDB::is_open() const
{
	return m_open;
}

void BlockchainDB::remove_transaction(const crypto::hash &tx_hash)
{
	transaction tx = get_tx(tx_hash);

	for(const txin_v &tx_input : tx.vin)
	{
		if(tx_input.type() == typeid(txin_to_key))
		{
			remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
		}
	}

	// need tx as tx.vout has the tx outputs, and the output amounts are needed
	remove_transaction_data(tx_hash, tx);
}

block BlockchainDB::get_block_from_height(const uint64_t &height) const
{
	blobdata bd = get_block_blob_from_height(height);
	block b;
	if(!parse_and_validate_block_from_blob(bd, b))
		throw DB_ERROR("Failed to parse block from blob retrieved from the db");

	return b;
}

block BlockchainDB::get_block(const crypto::hash &h) const
{
	blobdata bd = get_block_blob(h);
	block b;
	if(!parse_and_validate_block_from_blob(bd, b))
		throw DB_ERROR("Failed to parse block from blob retrieved from the db");

	return b;
}

bool BlockchainDB::get_tx(const crypto::hash &h, cryptonote::transaction &tx) const
{
	blobdata bd;
	if(!get_tx_blob(h, bd))
		return false;
	if(!parse_and_validate_tx_from_blob(bd, tx))
		throw DB_ERROR("Failed to parse transaction from blob retrieved from the db");

	return true;
}

transaction BlockchainDB::get_tx(const crypto::hash &h) const
{
	transaction tx;
	if(!get_tx(h, tx))
		throw TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str());
	return tx;
}

void BlockchainDB::reset_stats()
{
	num_calls = 0;
	time_blk_hash = 0;
	time_tx_exists = 0;
	time_add_block1 = 0;
	time_add_transaction = 0;
	time_commit1 = 0;
}

void BlockchainDB::show_stats()
{
	LOG_PRINT_L1(ENDL
				 << "*********************************"
				 << ENDL
				 << "num_calls: " << num_calls
				 << ENDL
				 << "time_blk_hash: " << time_blk_hash << "ms"
				 << ENDL
				 << "time_tx_exists: " << time_tx_exists << "ms"
				 << ENDL
				 << "time_add_block1: " << time_add_block1 << "ms"
				 << ENDL
				 << "time_add_transaction: " << time_add_transaction << "ms"
				 << ENDL
				 << "time_commit1: " << time_commit1 << "ms"
				 << ENDL
				 << "*********************************"
				 << ENDL);
}

void BlockchainDB::fixup()
{
	if(is_read_only())
	{
		LOG_PRINT_L1("Database is opened read only - skipping fixup check");
		return;
	}

	set_batch_transactions(true);
	batch_start();

	// block 685498 (13 key images in one transaction)
	static const char *const premine_key_images[] =
		{
			"a42fa875b187e7f9e8d25ad158187458cdcce0db9582b5ccd02e9a5d99a79239", //txid 29b65a4ddd5ad2502f4a5e536651de577837fef2b62e1eeccf518806a5195d98
			"89018dae2d6283edecf4df800bfdc56f1768fef450034dc9868388ac83cd045b", //txid c06ef81adb318383fef0af64620b35a781a6489b8ab90b574290057a5decd245
			"7caa8fe70872c26069bf1375c24c610702607568f5bc767d58ce027a8aa5ce93", //txid 97c5a2f7aef725270ed86e2407958fb72bcd9845e5293bd8177504f0ec659f86
			"d06f99b903d2633a44bd660fb3482172d4c699dba4b152f2232f25cc4b89fc40", //txid 29b65a4ddd5ad2502f4a5e536651de577837fef2b62e1eeccf518806a5195d98
			"2b241e39ee08d1f4292083fd7cfb0021f285a511933a1eaae3172dbf43fcf60d", //txid c06ef81adb318383fef0af64620b35a781a6489b8ab90b574290057a5decd245
			"5f14a472fe32f1c9da214133f58da9df2926b815945effdbf6fadd00d171a51a", //txid 97c5a2f7aef725270ed86e2407958fb72bcd9845e5293bd8177504f0ec659f86
		};

	for(const auto &kis : premine_key_images)
	{
		crypto::key_image ki;
		epee::string_tools::hex_to_pod(kis, ki);
		if(!has_key_image(ki))
		{
			LOG_PRINT_L1("Fixup: adding missing spent key " << ki);
			add_spent_key(ki);
		}
	}
	batch_stop();
}

} // namespace cryptonote
