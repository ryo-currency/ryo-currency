// Copyright (c) 2019, Ryo Currency Project
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

#include "wallet2.h"
#include "crypto/crypto.h"
#include "device/device_default.hpp"

namespace tools
{
GULPS_CAT_MAJOR("wallet_tx_scan");

std::unique_ptr<wallet2::wallet_rpc_scan_data> wallet2::pull_blocks(uint64_t start_height, const std::list<crypto::hash> &short_chain_history)
{
	cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
	cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
	req.block_ids = short_chain_history;
	req.prune = true;
	req.start_height = start_height;
	m_daemon_rpc_mutex.lock();
	bool r = epee::net_utils::invoke_http_bin("/getblocks.bin", req, res, m_http_client, rpc_timeout);
	m_daemon_rpc_mutex.unlock();
	THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "getblocks.bin");
	THROW_WALLET_EXCEPTION_IF(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getblocks.bin");
	THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
	THROW_WALLET_EXCEPTION_IF(res.blocks.size() != res.output_indices.size(), error::wallet_internal_error,
							  "mismatched blocks (" + boost::lexical_cast<std::string>(res.blocks.size()) + ") and output_indices (" +
								  boost::lexical_cast<std::string>(res.output_indices.size()) + ") sizes from daemon");

	GULPSF_LOG_L1("Pulled blocks {}-{} / {}.", res.start_height, std::max(res.start_height+res.blocks.size()-1, res.start_height), res.current_height);

	std::unique_ptr<wallet2::wallet_rpc_scan_data> ret(new wallet_rpc_scan_data);
	ret->blocks_start_height = res.start_height;
	ret->blocks_bin = std::move(res.blocks);
	ret->o_indices = std::move(res.output_indices);
	return ret;
}

void wallet2::block_download_thd(wallet2::wallet_block_dl_ctx& ctx)
{
	size_t try_count = 0;
	size_t last_top_height = 0;
	size_t current_top_height = 0;
	bool first_round = true;
	std::unique_ptr<wallet2::wallet_rpc_scan_data> pull_res;
	ctx.refreshed = false;
	ctx.error = false;
	ctx.cancelled = false;
	size_t dl_order = 0;

	while(m_run.load(std::memory_order_relaxed))
	{
		try
		{
			GULPS_LOG_L1("Pulling blocks...");
			pull_res = pull_blocks(ctx.start_height, ctx.short_chain_history);

			if(pull_res->blocks_bin.empty())
			{
				// TODO store short_chain_history or last height to retry pulling blocks
				GULPS_LOG_ERROR("Empty blocks vector from daemon.");
				try_count = 3;
				throw std::exception();
			}

			current_top_height = pull_res->blocks_start_height + pull_res->blocks_bin.size()-1;
			if(last_top_height == current_top_height)
			{
				GULPS_LOG_L1("No more blocks from daemon.");
				ctx.refresh_ctx.m_scan_in_queue.set_finish_flag();
				ctx.refreshed = true; 
				return;
			}

			last_top_height = current_top_height;
			pull_res->dl_order = dl_order++;
			if(first_round)
			{
				// Use chain history from now on
				ctx.start_height = 0;
				first_round = false;
			}
	
			drop_from_short_history(ctx.short_chain_history, 3);
			// prepend the last 3 blocks, should be enough to guard against a block or two's reorg
			cryptonote::block bl;
			for(auto it=pull_res->blocks_bin.rbegin(); it != pull_res->blocks_bin.rend() && std::distance(pull_res->blocks_bin.rbegin(), it) < 3; ++it)
			{
				bool ok = cryptonote::parse_and_validate_block_from_blob(it->block, bl);
				THROW_WALLET_EXCEPTION_IF(!ok, error::block_parse_error, it->block);
				ctx.short_chain_history.push_front(cryptonote::get_block_hash(bl));
			}

			ctx.refresh_ctx.m_scan_in_queue.wait_for_size(ctx.scan_thd_cnt + 1);
			ctx.refresh_ctx.m_scan_in_queue.push(std::move(pull_res));
			GULPS_LOG_L1("Pushed blocks...");
		}
		catch(...)
		{
			if(try_count < 3)
			{
				GULPS_LOG_L1("Another try pull_blocks (try_count=", try_count, ")...");
				++try_count;
			}
			else
			{
				GULPS_LOG_ERROR("pull_blocks failed, try_count=", try_count);

				// Exit due to an error
				ctx.refresh_ctx.m_scan_in_queue.set_finish_flag();
				ctx.error = true;
				return;
			}
		}
	}

	ctx.refresh_ctx.m_scan_in_queue.set_finish_flag();
	if(ctx.error)
	{
		GULPS_LOG_L1("Stop downloading blocks due to an error");
	}
	else
	{
		ctx.cancelled = true;
		GULPS_LOG_L1("Stop downloading blocks due to user cancel");
	}
}

bool wallet2::block_scan_tx(const wallet_scan_ctx& ctx, const crypto::hash& txid, const cryptonote::transaction& tx, bloom_filter& in_kimg, std::unordered_set<crypto::key_image>& inc_kimg)
{
	hw::core::device_default dummy_dev;
	const cryptonote::account_keys &keys = ctx.account.get_keys();
	GULPS_LOG_L2("Scanning tx ", txid);
	for(const auto& tx_in : tx.vin)
	{
		if(tx_in.type() != typeid(cryptonote::txin_to_key))
			continue;

		const crypto::key_image& ki = boost::get<cryptonote::txin_to_key>(tx_in).k_image;
		in_kimg.add_element(&ki, sizeof(crypto::key_image));
	}

	std::vector<cryptonote::tx_extra_field> tx_extra_fields;
	if(!parse_tx_extra(tx.extra, tx_extra_fields))
	{
		// Extra may only be partially parsed, it's OK if tx_extra_fields contains public key
		GULPS_LOG_L0("Transaction extra has unsupported format: ", txid);
	}

	cryptonote::tx_extra_pub_key pub_key_field;
	if(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field))
	{
		GULPS_LOG_L0("Public key wasn't found in the transaction extra. Skipping transaction ", txid);
		return false;
	}

	const crypto::public_key& tx_pub_key = pub_key_field.pub_key;
	crypto::key_derivation derivation;
	bool derivation_res;
#ifdef HAVE_EC_64
	derivation_res = crypto::generate_key_derivation_64(tx_pub_key, keys.m_view_secret_key, derivation);
#else
	derivation_res = crypto::generate_key_derivation(tx_pub_key, keys.m_view_secret_key, derivation);
#endif
	
	if(!derivation_res)
	{
		GULPS_WARN("Failed to generate key derivation from tx pubkey, skipping");
		memcpy(&derivation, rct::identity().bytes, sizeof(derivation));
	}

	std::vector<crypto::public_key> additional_tx_pub_keys = get_additional_tx_pub_keys_from_extra(tx);
	std::vector<crypto::key_derivation> additional_derivations;
	additional_derivations.resize(additional_tx_pub_keys.size());

	// additional tx pubkeys and derivations for multi-destination transfers involving one or more subaddresses
	for(size_t i = 0; i < additional_tx_pub_keys.size(); ++i)
	{
#ifdef HAVE_EC_64
		derivation_res = crypto::generate_key_derivation_64(additional_tx_pub_keys[i], keys.m_view_secret_key, additional_derivations[i]);
#else
		derivation_res = crypto::generate_key_derivation(additional_tx_pub_keys[i], keys.m_view_secret_key, additional_derivations[i]);
#endif
		if(!derivation_res)
		{
			GULPS_WARN("Failed to generate key derivation from extra pubkey ", i, ", skipping");
			additional_derivations.pop_back();
		}
	}

	bool spend_unknown = keys.m_spend_secret_key == crypto::null_skey || !keys.m_multisig_keys.empty();
	bool ret = false;
	for(size_t out_idx = 0; out_idx < tx.vout.size(); out_idx++)
	{
		if(tx.vout[out_idx].target.type() != typeid(cryptonote::txout_to_key))
		{
			GULPS_LOG_L0("Wrong type id in transaction out");
			continue;
		}

		// try the shared tx pubkey
		crypto::public_key subaddress_spendkey;
		const crypto::public_key& out_pubkey = boost::get<cryptonote::txout_to_key>(tx.vout[out_idx].target).key;
#ifdef HAVE_EC_64
		crypto::derive_subaddress_public_key_64(out_pubkey, derivation, out_idx, subaddress_spendkey);
#else
		crypto::derive_subaddress_public_key(out_pubkey, derivation, out_idx, subaddress_spendkey);
#endif

		auto found = m_subaddresses.find(subaddress_spendkey);
		if(found != m_subaddresses.end())
		{
			if(!spend_unknown)
			{
				cryptonote::keypair eph;
				crypto::key_image ki;
				bool r = cryptonote::generate_key_image_helper_precomp(keys, out_pubkey, derivation, out_idx, found->second, eph, ki, dummy_dev);
				THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "Failed to generate key image");
				THROW_WALLET_EXCEPTION_IF(eph.pub != out_pubkey,
									error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");
				THROW_WALLET_EXCEPTION_IF(!inc_kimg.insert(ki).second, error::wallet_internal_error, "Duplicate key image");
			}
			ret = true;
			continue;
		}

		// try additional tx pubkeys if available
		if(!additional_derivations.empty())
		{
			if(out_idx >= additional_derivations.size())
			{
				GULPS_LOG_L0("Wrong number of additional derivations");
				return false;
			}
#ifdef HAVE_EC_64
			crypto::derive_subaddress_public_key_64(out_pubkey, additional_derivations[out_idx], out_idx, subaddress_spendkey);
#else
			crypto::derive_subaddress_public_key(out_pubkey, additional_derivations[out_idx], out_idx, subaddress_spendkey);
#endif
			found = m_subaddresses.find(subaddress_spendkey);
			if(found != m_subaddresses.end())
			{
				if(!spend_unknown)
				{
					cryptonote::keypair eph;
					crypto::key_image ki;
					bool r = cryptonote::generate_key_image_helper_precomp(keys, out_pubkey, derivation, out_idx, found->second, eph, ki, dummy_dev);
					THROW_WALLET_EXCEPTION_IF(!r, error::wallet_internal_error, "Failed to generate key image");
					THROW_WALLET_EXCEPTION_IF(eph.pub != out_pubkey,
										error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");
					THROW_WALLET_EXCEPTION_IF(!inc_kimg.insert(ki).second, error::wallet_internal_error, "Duplicate key image");
				}
				ret = true;
				continue;
			}
		}
	}

	return ret;
}

void wallet2::block_scan_thd(const wallet_scan_ctx& ctx)
{
	try
	{
		std::unique_lock<std::mutex> lck;
		while(ctx.refresh_ctx.m_scan_in_queue.wait_for_pop(lck))
		{
			std::unique_ptr<wallet2::wallet_rpc_scan_data> pull_res = ctx.refresh_ctx.m_scan_in_queue.pop(lck);

			GULPSF_LOG_L1("Scanning blocks {} - {}", pull_res->blocks_start_height, pull_res->blocks_start_height+pull_res->blocks_bin.size()-1);
			
			if(ctx.refresh_ctx.m_scan_error)
			{
				GULPS_LOG_L1("block_scan_thd exits due to m_scan_error.");
				break;
			}

			THROW_WALLET_EXCEPTION_IF(pull_res->blocks_bin.size() != pull_res->o_indices.size(), error::wallet_internal_error, "size mismatch");

			size_t blk_i=0;
			size_t in_count = 0;
			pull_res->blocks_parsed.resize(pull_res->blocks_bin.size());
			for(const cryptonote::block_complete_entry_v& bl : pull_res->blocks_bin)
			{
				wallet_rpc_scan_data::block_complete_entry_parsed& blke = pull_res->blocks_parsed[blk_i];
				const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices& blk_o_idx = pull_res->o_indices[blk_i];
				blke.block_height = pull_res->blocks_start_height + blk_i;

				bool r = cryptonote::parse_and_validate_block_from_blob(bl.block, blke.block);
				THROW_WALLET_EXCEPTION_IF(!r, error::block_parse_error, bl.block);
				blke.block_hash = get_block_hash(blke.block);
				THROW_WALLET_EXCEPTION_IF(bl.txs.size() + 1 != blk_o_idx.indices.size(), error::wallet_internal_error,
					"block transactions=" + std::to_string(bl.txs.size()) + " not match with daemon response size=" + 
					std::to_string(blk_o_idx.indices.size()) + " for block " + std::to_string(blke.block_height));
				THROW_WALLET_EXCEPTION_IF(bl.txs.size() != blke.block.tx_hashes.size(), error::wallet_internal_error, 
										  "Wrong amount of transactions for block");

				blk_i++;
				if(!ctx.explicit_refresh && (blke.block.timestamp + 60 * 60 * 24 <= ctx.wallet_create_time || blke.block_height  < ctx.refresh_height))
				{
					if(blke.block_height % 100 == 0)
						GULPS_LOG_L2("Skipped block by timestamp, height: ", blke.block_height, ", block time ", 
									 blke.block.timestamp, ", account time ", ctx.wallet_create_time);
					blke.skipped = true;
					continue;
				}
				blke.skipped = false;

				blke.miner_tx_hash = cryptonote::get_transaction_hash(blke.block.miner_tx);
				size_t tx_i =0;
				blke.txes.resize(bl.txs.size());
				for(const cryptonote::blobdata& tx_blob : bl.txs)
				{
					cryptonote::transaction& tx = blke.txes[tx_i];
					bool r = parse_and_validate_tx_base_from_blob(tx_blob, tx);
					THROW_WALLET_EXCEPTION_IF(!r, error::tx_parse_error, tx_blob);
					in_count += tx.vin.size();
					tx_i++;
				}
			}

			pull_res->key_images.init(in_count);
			for(size_t i=0; i < blk_i; i++)
			{
				wallet_rpc_scan_data::block_complete_entry_parsed& blke = pull_res->blocks_parsed[i];

				if(blke.skipped)
					continue;

				if(ctx.scan_type != RefreshNoCoinbase)
				{
					if(block_scan_tx(ctx, blke.miner_tx_hash, blke.block.miner_tx, pull_res->key_images, pull_res->incoming_kimg))
						pull_res->indices_found.emplace_back(i, 0);
				}

				for(size_t txi=0; txi < blke.txes.size(); txi++)
				{
					if(block_scan_tx(ctx, blke.block.tx_hashes[txi], blke.txes[txi], pull_res->key_images, pull_res->incoming_kimg))
						pull_res->indices_found.emplace_back(i, txi+1);
				}
			}

			ctx.refresh_ctx.m_scan_out_queue.push(std::move(pull_res));
		}
	}
	catch(std::exception& e)
	{
		ctx.refresh_ctx.m_scan_error = true;
		m_run = false;
		GULPSF_LOG_ERROR("Blocks scanning thread exception: {}", e.what());
	}

	if(ctx.refresh_ctx.m_running_scan_thd_cnt.fetch_sub(1)-1 == 0)
	{
		GULPS_LOG_L1("Blocks scanning threads complete");
		ctx.refresh_ctx.m_scan_out_queue.set_finish_flag();
	}
	else
	{
		size_t left = ctx.refresh_ctx.m_running_scan_thd_cnt;
		bool error = ctx.refresh_ctx.m_scan_error;
		GULPSF_LOG_L1("block_scan_thd exits {} left, m_scan_error state", left, error);
	}
}
}
