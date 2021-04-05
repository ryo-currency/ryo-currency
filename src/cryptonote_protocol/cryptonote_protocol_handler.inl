/// @file
/// @author rfree (current maintainer/user in monero.cc project - most of code is from CryptoNote)
/// @brief This is the original cryptonote protocol network-events handler, modified by us

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

// (may contain code and/or modifications by other developers)
// developer rfree: this code is caller of our new network code, and is modded; e.g. for rate limiting

//IGNORE
#include <boost/interprocess/detail/atomic.hpp>
#include <ctime>
#include <list>

#include "cryptonote_basic/verification_context.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "net/network_throttle-detail.hpp"
#include "profile_tools.h"

#include "common/gulps.hpp"



#define context_str std::string("[" + epee::net_utils::print_connection_context_short(context) + "]")

#define GULPS_P2P_MESSAGE(...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_INFO, "p2p", gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)

#define BLOCK_QUEUE_NBLOCKS_THRESHOLD 10					// chunks of N blocks
#define BLOCK_QUEUE_SIZE_THRESHOLD (100 * 1024 * 1024)		// MB
#define REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD (5 * 1000000) // microseconds
#define IDLE_PEER_KICK_TIME (600 * 1000000)					// microseconds
#define PASSIVE_PEER_KICK_TIME (60 * 1000000)				// microseconds

namespace cryptonote
{

//-----------------------------------------------------------------------------------------------------------------------
template <class t_core>
t_cryptonote_protocol_handler<t_core>::t_cryptonote_protocol_handler(t_core &rcore, nodetool::i_p2p_endpoint<connection_context> *p_net_layout, bool offline) : m_core(rcore),
																																								m_p2p(p_net_layout),
																																								m_syncronized_connections_count(0),
																																								m_synchronized(offline),
																																								m_stopping(false)

{
	if(!m_p2p)
		m_p2p = &m_p2p_stub;
}
//-----------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::init(const boost::program_options::variables_map &vm)
{
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::deinit()
{
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
void t_cryptonote_protocol_handler<t_core>::set_p2p_endpoint(nodetool::i_p2p_endpoint<connection_context> *p2p)
{
	if(p2p)
		m_p2p = p2p;
	else
		m_p2p = &m_p2p_stub;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::on_callback(cryptonote_connection_context &context)
{
	GULPS_LOG_L1(context_str, " callback fired");
	GULPS_CHECK_AND_ASSERT_MES_CONTEXT(context.m_callback_request_count > 0, false, "false callback fired, but context.m_callback_request_count=", context.m_callback_request_count);
	--context.m_callback_request_count;

	if(context.m_state == cryptonote_connection_context::state_synchronizing)
	{
		NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
		m_core.get_short_chain_history(r.block_ids);
		GULPSF_LOG_L1("{} -->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()={}", context_str,  r.block_ids.size());
		post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
	}
	else if(context.m_state == cryptonote_connection_context::state_standby)
	{
		context.m_state = cryptonote_connection_context::state_synchronizing;
		try_add_next_blocks(context);
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::get_stat_info(core_stat_info &stat_inf)
{
	return m_core.get_stat_info(stat_inf);
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
void t_cryptonote_protocol_handler<t_core>::log_connections()
{
	std::stringstream ss;
	ss.precision(1);

	double down_sum = 0.0;
	double down_curr_sum = 0.0;
	double up_sum = 0.0;
	double up_curr_sum = 0.0;

	ss << std::setw(30) << std::left << "Remote Host"
	   << std::setw(20) << "Peer id"
	   << std::setw(20) << "Support Flags"
	   << std::setw(30) << "Recv/Sent (inactive,sec)"
	   << std::setw(25) << "State"
	   << std::setw(20) << "Livetime(sec)"
	   << std::setw(12) << "Down (kB/s)"
	   << std::setw(14) << "Down(now)"
	   << std::setw(10) << "Up (kB/s)"
	   << std::setw(13) << "Up(now)"
	   << "\n";

	m_p2p->for_each_connection([&](const connection_context &cntxt, nodetool::peerid_type peer_id, uint32_t support_flags) {
		bool local_ip = cntxt.m_remote_address.is_local();
		auto connection_time = time(NULL) - cntxt.m_started;
		ss << std::setw(30) << std::left << std::string(cntxt.m_is_income ? " [INC]" : "[OUT]") + cntxt.m_remote_address.str()
		   << std::setw(20) << std::hex << peer_id
		   << std::setw(20) << std::hex << support_flags
		   << std::setw(30) << std::to_string(cntxt.m_recv_cnt) + "(" + std::to_string(time(NULL) - cntxt.m_last_recv) + ")" + "/" + std::to_string(cntxt.m_send_cnt) + "(" + std::to_string(time(NULL) - cntxt.m_last_send) + ")"
		   << std::setw(25) << get_protocol_state_string(cntxt.m_state)
		   << std::setw(20) << std::to_string(time(NULL) - cntxt.m_started)
		   << std::setw(12) << std::fixed << (connection_time == 0 ? 0.0 : cntxt.m_recv_cnt / connection_time / 1024)
		   << std::setw(14) << std::fixed << cntxt.m_current_speed_down / 1024
		   << std::setw(10) << std::fixed << (connection_time == 0 ? 0.0 : cntxt.m_send_cnt / connection_time / 1024)
		   << std::setw(13) << std::fixed << cntxt.m_current_speed_up / 1024
		   << (local_ip ? "[LAN]" : "")
		   << std::left << (cntxt.m_remote_address.is_loopback() ? "[LOCALHOST]" : "") // 127.0.0.1
		   << "\n";

		if(connection_time > 1)
		{
			down_sum += (cntxt.m_recv_cnt / connection_time / 1024);
			up_sum += (cntxt.m_send_cnt / connection_time / 1024);
		}

		down_curr_sum += (cntxt.m_current_speed_down / 1024);
		up_curr_sum += (cntxt.m_current_speed_up / 1024);

		return true;
	});
	ss << "\n"
	   << std::setw(125) << " "
	   << std::setw(12) << down_sum
	   << std::setw(14) << down_curr_sum
	   << std::setw(10) << up_sum
	   << std::setw(13) << up_curr_sum
	   << "\n";
		GULPS_PRINT("Connections:\n", ss.str());
}
//------------------------------------------------------------------------------------------------------------------------
// Returns a list of connection_info objects describing each open p2p connection
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
std::list<connection_info> t_cryptonote_protocol_handler<t_core>::get_connections()
{
	std::list<connection_info> connections;

	m_p2p->for_each_connection([&](const connection_context &cntxt, nodetool::peerid_type peer_id, uint32_t support_flags) {
		connection_info cnx;
		auto timestamp = time(NULL);

		cnx.incoming = cntxt.m_is_income ? true : false;

		cnx.address = cntxt.m_remote_address.str();
		cnx.host = cntxt.m_remote_address.host_str();
		cnx.ip = "";
		cnx.port = "";
		if(cntxt.m_remote_address.get_type_id() == epee::net_utils::ipv4_network_address::ID)
		{
			cnx.ip = cnx.host;
			cnx.port = std::to_string(cntxt.m_remote_address.as<epee::net_utils::ipv4_network_address>().port());
		}

		std::stringstream peer_id_str;
		peer_id_str << std::hex << std::setw(16) << peer_id;
		peer_id_str >> cnx.peer_id;

		cnx.support_flags = support_flags;

		cnx.recv_count = cntxt.m_recv_cnt;
		cnx.recv_idle_time = timestamp - std::max(cntxt.m_started, cntxt.m_last_recv);

		cnx.send_count = cntxt.m_send_cnt;
		cnx.send_idle_time = timestamp - std::max(cntxt.m_started, cntxt.m_last_send);

		cnx.state = get_protocol_state_string(cntxt.m_state);

		cnx.live_time = timestamp - cntxt.m_started;

		cnx.localhost = cntxt.m_remote_address.is_loopback();
		cnx.local_ip = cntxt.m_remote_address.is_local();

		auto connection_time = time(NULL) - cntxt.m_started;
		if(connection_time == 0)
		{
			cnx.avg_download = 0;
			cnx.avg_upload = 0;
		}

		else
		{
			cnx.avg_download = cntxt.m_recv_cnt / connection_time / 1024;
			cnx.avg_upload = cntxt.m_send_cnt / connection_time / 1024;
		}

		cnx.current_download = cntxt.m_current_speed_down / 1024;
		cnx.current_upload = cntxt.m_current_speed_up / 1024;

		cnx.connection_id = epee::string_tools::pod_to_hex(cntxt.m_connection_id);

		cnx.height = cntxt.m_remote_blockchain_height;

		connections.push_back(cnx);

		return true;
	});

	return connections;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::process_payload_sync_data(const CORE_SYNC_DATA &hshd, cryptonote_connection_context &context, bool is_inital)
{
	if(context.m_state == cryptonote_connection_context::state_before_handshake && !is_inital)
		return true;

	if(context.m_state == cryptonote_connection_context::state_synchronizing)
		return true;

	// from v6, if the peer advertises a top block version, reject if it's not what it should be (will only work if no voting)
	if(hshd.current_height > 0)
	{
		const uint8_t version = m_core.get_ideal_hard_fork_version(hshd.current_height - 1);
		if(version >= 6 && version != hshd.top_version)
		{
			if(version < hshd.top_version && version == m_core.get_ideal_hard_fork_version())
				GULPSF_CAT_WARN("global", "{} peer claims higher version that we think ({} for {} instead of {}) - we may be forked from the network and a software upgrade may be needed", context_str, (unsigned)hshd.top_version, (hshd.current_height - 1), (unsigned)version);
			return false;
		}
	}

	context.m_remote_blockchain_height = hshd.current_height;

	uint64_t target = m_core.get_target_blockchain_height();
	if(target == 0)
		target = m_core.get_current_blockchain_height();

	if(m_core.have_block(hshd.top_id))
	{
		context.m_state = cryptonote_connection_context::state_normal;
		if(is_inital && target == m_core.get_current_blockchain_height())
			on_connection_synchronized();
		return true;
	}

	if(hshd.current_height > target)
	{
		/* As I don't know if accessing hshd from core could be a good practice,
    I prefer pushing target height to the core at the same time it is pushed to the user.
    Nz. */
		m_core.set_target_blockchain_height((hshd.current_height));
		int64_t diff = static_cast<int64_t>(hshd.current_height) - static_cast<int64_t>(m_core.get_current_blockchain_height());
		uint64_t abs_diff = std::abs(diff);
		uint64_t max_block_height = std::max(hshd.current_height, m_core.get_current_blockchain_height());
		uint64_t last_block_v1 = m_core.get_nettype() == TESTNET ? 624633 : m_core.get_nettype() == MAINNET ? 1009826 : (uint64_t)-1;
		uint64_t diff_v2 = max_block_height > last_block_v1 ? std::min(abs_diff, max_block_height - last_block_v1) : 0;
		if(is_inital)
		GULPSF_GLOBAL_PRINT("\n{} Sync data returned a new top block candidate: {} -> {} [Your node is {} blocks ({} days {})]\nSYNCHRONIZATION started", context_str, m_core.get_current_blockchain_height(),
					hshd.current_height, abs_diff, ((abs_diff - diff_v2) / (24 * 60 * 60 / common_config::DIFFICULTY_TARGET)) + (diff_v2 / (24 * 60 * 60 / common_config::DIFFICULTY_TARGET)),
					(0 <= diff ? std::string("behind") : std::string("ahead")));
		else
		GULPSF_GLOBAL_PRINT("\n{} Sync data returned a new top block candidate: {} -> {} [Your node is {} blocks ({} days {})]\nSYNCHRONIZATION started", context_str, m_core.get_current_blockchain_height(),
					hshd.current_height, abs_diff, ((abs_diff - diff_v2) / (24 * 60 * 60 / common_config::DIFFICULTY_TARGET)) + (diff_v2 / (24 * 60 * 60 / common_config::DIFFICULTY_TARGET)),
					(0 <= diff ? std::string("behind") : std::string("ahead")));

		if(hshd.current_height >= m_core.get_current_blockchain_height() + 5) // don't switch to unsafe mode just for a few blocks
			m_core.safesyncmode(false);
	}
	GULPSF_INFO("Remote blockchain height: {}, id: {}", hshd.current_height , hshd.top_id);
	context.m_state = cryptonote_connection_context::state_synchronizing;
	//let the socket to send response to handshake, but request callback, to let send request data after response
	GULPS_LOG_L1( context_str, " requesting callback");
	++context.m_callback_request_count;
	m_p2p->request_callback(context);
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(CORE_SYNC_DATA &hshd)
{
	m_core.get_blockchain_top(hshd.current_height, hshd.top_id);
	hshd.top_version = m_core.get_ideal_hard_fork_version(hshd.current_height);
	hshd.cumulative_difficulty = m_core.get_block_cumulative_difficulty(hshd.current_height);
	hshd.current_height += 1;
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::get_payload_sync_data(blobdata &data)
{
	CORE_SYNC_DATA hsd = boost::value_initialized<CORE_SYNC_DATA>();
	get_payload_sync_data(hsd);
	epee::serialization::store_t_to_binary(hsd, data);
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_NEW_BLOCK ({} txes)", arg.b.txs.size() );
	if(context.m_state != cryptonote_connection_context::state_normal)
		return 1;
	if(!is_synchronized()) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
	{
		GULPS_LOG_L1(context_str," Received new block while syncing, ignored");
		return 1;
	}
	m_core.pause_mine();
	std::list<block_complete_entry> blocks;
	blocks.push_back(arg.b);
	m_core.prepare_handle_incoming_blocks(blocks);
	for(auto tx_blob_it = arg.b.txs.begin(); tx_blob_it != arg.b.txs.end(); tx_blob_it++)
	{
		cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
		m_core.handle_incoming_tx(*tx_blob_it, tvc, true, true, false);
		if(tvc.m_verifivation_failed)
		{
			GULPS_INFO( context_str, " Block verification failed: transaction verification failed, dropping connection");
			drop_connection(context, false, false);
			m_core.cleanup_handle_incoming_blocks();
			m_core.resume_mine();
			return 1;
		}
	}

	block_verification_context bvc = boost::value_initialized<block_verification_context>();
	m_core.handle_incoming_block(arg.b.block, bvc); // got block from handle_notify_new_block
	if(!m_core.cleanup_handle_incoming_blocks(true))
	{
		GULPS_PRINT( context_str, " Failure in cleanup_handle_incoming_blocks");
		m_core.resume_mine();
		return 1;
	}
	m_core.resume_mine();
	if(bvc.m_verifivation_failed)
	{
		GULPS_PRINT( context_str, " Block verification failed, dropping connection");
		drop_connection(context, true, false);
		return 1;
	}
	if(bvc.m_added_to_main_chain)
	{
		//TODO: Add here announce protocol usage
		relay_block(arg, context);
	}
	else if(bvc.m_marked_as_orphaned)
	{
		context.m_state = cryptonote_connection_context::state_synchronizing;
		NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
		m_core.get_short_chain_history(r.block_ids);
		GULPSF_LOG_L1("{} -->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()={}", context_str, r.block_ids.size());
		post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
	}

	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_notify_new_fluffy_block(int command, NOTIFY_NEW_FLUFFY_BLOCK::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_NEW_FLUFFY_BLOCK (height {}, {} txes)", arg.current_blockchain_height , arg.b.txs.size() );
	if(context.m_state != cryptonote_connection_context::state_normal)
		return 1;
	if(!is_synchronized()) // can happen if a peer connection goes to normal but another thread still hasn't finished adding queued blocks
	{
		GULPS_LOG_L1( context_str, " Received new block while syncing, ignored");
		return 1;
	}

	m_core.pause_mine();

	block new_block;
	transaction miner_tx;
	if(parse_and_validate_block_from_blob(arg.b.block, new_block))
	{
		// This is a second notification, we must have asked for some missing tx
		if(!context.m_requested_objects.empty())
		{
			// What we asked for != to what we received ..
			if(context.m_requested_objects.size() != arg.b.txs.size())
			{
				GULPSF_LOG_ERROR("{} NOTIFY_NEW_FLUFFY_BLOCK -> request/response mismatch, block = {}, requested = {}, received = {}, dropping connection", context_str, epee::string_tools::pod_to_hex(get_blob_hash(arg.b.block))
					, context.m_requested_objects.size()
					, new_block.tx_hashes.size()
					);

				drop_connection(context, false, false);
				m_core.resume_mine();
				return 1;
			}
		}

		std::list<blobdata> have_tx;

		// Instead of requesting missing transactions by hash like BTC,
		// we do it by index (thanks to a suggestion from moneromooo) because
		// we're way cooler .. and also because they're smaller than hashes.
		//
		// Also, remember to pepper some whitespace changes around to bother
		// moneromooo ... only because I <3 him.
		std::vector<uint64_t> need_tx_indices;

		transaction tx;
		crypto::hash tx_hash;

		for(auto &tx_blob : arg.b.txs)
		{
			if(parse_and_validate_tx_from_blob(tx_blob, tx))
			{
				try
				{
					if(!get_transaction_hash(tx, tx_hash))
					{
						GULPS_INFO(context_str, " NOTIFY_NEW_FLUFFY_BLOCK: get_transaction_hash failed, dropping connection");

						drop_connection(context, false, false);
						m_core.resume_mine();
						return 1;
					}
				}
				catch(...)
				{
					GULPS_INFO( context_str, " NOTIFY_NEW_FLUFFY_BLOCK: get_transaction_hash failed, exception thrown, dropping connection");

					drop_connection(context, false, false);
					m_core.resume_mine();
					return 1;
				}

				// hijacking m_requested objects in connection context to patch up
				// a possible DOS vector pointed out by @monero-moo where peers keep
				// sending (0...n-1) transactions.
				// If requested objects is not empty, then we must have asked for
				// some missing transacionts, make sure that they're all there.
				//
				// Can I safely re-use this field? I think so, but someone check me!
				if(!context.m_requested_objects.empty())
				{
					auto req_tx_it = context.m_requested_objects.find(tx_hash);
					if(req_tx_it == context.m_requested_objects.end())
					{
						GULPSF_LOG_ERROR("{} Peer sent wrong transaction (NOTIFY_NEW_FLUFFY_BLOCK): transaction with id = {} wasn't requested, dropping connection", context_str, tx_hash );

						drop_connection(context, false, false);
						m_core.resume_mine();
						return 1;
					}

					context.m_requested_objects.erase(req_tx_it);
				}

				// we might already have the tx that the peer
				// sent in our pool, so don't verify again..
				if(!m_core.pool_has_tx(tx_hash))
				{
					GULPSF_LOG_L1("Incoming tx {} not in pool, adding", tx_hash );
					cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
					if(!m_core.handle_incoming_tx(tx_blob, tvc, true, true, false) || tvc.m_verifivation_failed)
					{
						GULPS_INFO( context_str, " Block verification failed: transaction verification failed, dropping connection");
						drop_connection(context, false, false);
						m_core.resume_mine();
						return 1;
					}

					//
					// future todo:
					// tx should only not be added to pool if verification failed, but
					// maybe in the future could not be added for other reasons
					// according to monero-moo so keep track of these separately ..
					//
				}
			}
			else
			{
				GULPSF_LOG_ERROR("{} sent wrong tx: failed to parse and validate transaction: {}, dropping connection", context_str, epee::string_tools::buff_to_hex_nodelimer(tx_blob));

				drop_connection(context, false, false);
				m_core.resume_mine();
				return 1;
			}
		}

		// The initial size equality check could have been fooled if the sender
		// gave us the number of transactions we asked for, but not the right
		// ones. This check make sure the transactions we asked for were the
		// ones we received.
		if(context.m_requested_objects.size())
		{
			GULPSF_LOG_ERROR("NOTIFY_NEW_FLUFFY_BLOCK: peer sent the number of transaction requested, but not the actual transactions requested, context.m_requested_objects.size() = {}, dropping connection"
				, context.m_requested_objects.size());

			drop_connection(context, false, false);
			m_core.resume_mine();
			return 1;
		}

		size_t tx_idx = 0;
		for(auto &tx_hash : new_block.tx_hashes)
		{
			cryptonote::blobdata txblob;
			if(m_core.get_pool_transaction(tx_hash, txblob))
			{
				have_tx.push_back(txblob);
			}
			else
			{
				std::vector<crypto::hash> tx_ids;
				std::list<transaction> txes;
				std::list<crypto::hash> missing;
				tx_ids.push_back(tx_hash);
				if(m_core.get_transactions(tx_ids, txes, missing) && missing.empty())
				{
					if(txes.size() == 1)
					{
						have_tx.push_back(tx_to_blob(txes.front()));
					}
					else
					{
						GULPSF_LOG_L1("1 tx requested, none not found, but {} returned", txes.size() );
						m_core.resume_mine();
						return 1;
					}
				}
				else
				{
					GULPSF_LOG_L1("Tx {} not found in pool", tx_hash );
					need_tx_indices.push_back(tx_idx);
				}
			}

			++tx_idx;
		}

		if(!need_tx_indices.empty()) // drats, we don't have everything..
		{
			// request non-mempool txs
			GULPSF_LOG_L1("We are missing {} txes for this fluffy block", need_tx_indices.size() );
			for(auto txidx : need_tx_indices)
				GULPSF_LOG_L1("  tx {}", new_block.tx_hashes[txidx]);
			NOTIFY_REQUEST_FLUFFY_MISSING_TX::request missing_tx_req;
			missing_tx_req.block_hash = get_block_hash(new_block);
			missing_tx_req.current_blockchain_height = arg.current_blockchain_height;
			missing_tx_req.missing_tx_indices = std::move(need_tx_indices);

			m_core.resume_mine();
			post_notify<NOTIFY_REQUEST_FLUFFY_MISSING_TX>(missing_tx_req, context);
		}
		else // whoo-hoo we've got em all ..
		{
			GULPS_LOG_L1("We have all needed txes for this fluffy block");

			block_complete_entry b;
			b.block = arg.b.block;
			b.txs = have_tx;

			std::list<block_complete_entry> blocks;
			blocks.push_back(b);
			m_core.prepare_handle_incoming_blocks(blocks);

			block_verification_context bvc = boost::value_initialized<block_verification_context>();
			m_core.handle_incoming_block(arg.b.block, bvc); // got block from handle_notify_new_block
			if(!m_core.cleanup_handle_incoming_blocks(true))
			{
				GULPS_PRINT( context_str, " Failure in cleanup_handle_incoming_blocks");
				m_core.resume_mine();
				return 1;
			}
			m_core.resume_mine();

			if(bvc.m_verifivation_failed)
			{
				GULPS_PRINT( context_str, " Block verification failed, dropping connection");
				drop_connection(context, true, false);
				return 1;
			}
			if(bvc.m_added_to_main_chain)
			{
				//TODO: Add here announce protocol usage
				NOTIFY_NEW_BLOCK::request reg_arg = AUTO_VAL_INIT(reg_arg);
				reg_arg.current_blockchain_height = arg.current_blockchain_height;
				reg_arg.b = b;
				relay_block(reg_arg, context);
			}
			else if(bvc.m_marked_as_orphaned)
			{
				context.m_state = cryptonote_connection_context::state_synchronizing;
				NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
				m_core.get_short_chain_history(r.block_ids);
				GULPSF_LOG_L1("{} -->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()={}", context_str, r.block_ids.size());
				post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
			}
		}
	}
	else
	{
		GULPSF_LOG_ERROR("sent wrong block: failed to parse and validate block: {}, dropping connection", epee::string_tools::buff_to_hex_nodelimer(arg.b.block)
			);

		m_core.resume_mine();
		drop_connection(context, false, false);

		return 1;
	}

	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_request_fluffy_missing_tx(int command, NOTIFY_REQUEST_FLUFFY_MISSING_TX::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_REQUEST_FLUFFY_MISSING_TX ({} txes), block hash {}", arg.missing_tx_indices.size() , arg.block_hash);

	std::list<std::pair<cryptonote::blobdata, block>> local_blocks;
	std::list<cryptonote::blobdata> local_txs;

	block b;
	if(!m_core.get_block_by_hash(arg.block_hash, b))
	{
		GULPSF_LOG_ERROR("{} failed to find block: {}, dropping connection", context_str, arg.block_hash );
		drop_connection(context, false, false);
		return 1;
	}

	std::vector<crypto::hash> txids;
	NOTIFY_NEW_FLUFFY_BLOCK::request fluffy_response;
	fluffy_response.b.block = t_serializable_object_to_blob(b);
	fluffy_response.current_blockchain_height = arg.current_blockchain_height;
	for(auto &tx_idx : arg.missing_tx_indices)
	{
		if(tx_idx < b.tx_hashes.size())
		{
			GULPSF_LOG_L1("  tx {}", b.tx_hashes[tx_idx]);
			txids.push_back(b.tx_hashes[tx_idx]);
		}
		else
		{
			GULPSF_LOG_ERROR("{} Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, request is asking for a tx whose index is out of bounds , tx index = {}, block tx count {}, block_height = {}, dropping connection",  context_str, tx_idx , b.tx_hashes.size()
				, arg.current_blockchain_height);


			drop_connection(context, false, false);
			return 1;
		}
	}

	std::list<cryptonote::transaction> txs;
	std::list<crypto::hash> missed;
	if(!m_core.get_transactions(txids, txs, missed))
	{
		GULPS_LOG_ERROR( context_str, " Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, failed to get requested transactions");
		drop_connection(context, false, false);
		return 1;
	}
	if(!missed.empty() || txs.size() != txids.size())
	{
		GULPSF_LOG_ERROR("{} Failed to handle request NOTIFY_REQUEST_FLUFFY_MISSING_TX, {} requested transactions not found, dropping connection", context_str, missed.size() );
		drop_connection(context, false, false);
		return 1;
	}

	for(auto &tx : txs)
	{
		fluffy_response.b.txs.push_back(t_serializable_object_to_blob(tx));
	}

	GULPSF_LOG_L1("{} -->>NOTIFY_RESPONSE_FLUFFY_MISSING_TX: , txs.size()={}, rsp.current_blockchain_height={}", context_str, fluffy_response.b.txs.size() , fluffy_response.current_blockchain_height);

	post_notify<NOTIFY_NEW_FLUFFY_BLOCK>(fluffy_response, context);
	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_notify_new_transactions(int command, NOTIFY_NEW_TRANSACTIONS::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_NEW_TRANSACTIONS ({} txes)", arg.txs.size() );
	if(context.m_state != cryptonote_connection_context::state_normal)
		return 1;

	// while syncing, core will lock for a long time, so we ignore
	// those txes as they aren't really needed anyway, and avoid a
	// long block before replying
	if(!is_synchronized())
	{
		GULPS_LOG_L1( context_str, " Received new tx while syncing, ignored");
		return 1;
	}

	for(auto tx_blob_it = arg.txs.begin(); tx_blob_it != arg.txs.end();)
	{
		cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
		m_core.handle_incoming_tx(*tx_blob_it, tvc, false, true, false);
		if(tvc.m_verifivation_failed)
		{
			GULPS_INFO( context_str, " Tx verification failed, dropping connection");
			drop_connection(context, false, false);
			return 1;
		}
		if(tvc.m_should_be_relayed)
			++tx_blob_it;
		else
			arg.txs.erase(tx_blob_it++);
	}

	if(arg.txs.size())
	{
		//TODO: add announce usage here
		relay_transactions(arg, context);
	}

	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_REQUEST_GET_OBJECTS ({} blocks, {} txes)", arg.blocks.size() , arg.txs.size() );
	NOTIFY_RESPONSE_GET_OBJECTS::request rsp;
	if(!m_core.handle_get_objects(arg, rsp, context))
	{
		GULPS_ERROR( context_str," failed to handle request NOTIFY_REQUEST_GET_OBJECTS, dropping connection");
		drop_connection(context, false, false);
		return 1;
	}
	GULPSF_LOG_L1("{} -->>NOTIFY_RESPONSE_GET_OBJECTS: blocks.size()={}, txs.size()={}, rsp.m_current_blockchain_height={}, missed_ids.size()={}", context_str, rsp.blocks.size() , rsp.txs.size() , rsp.current_blockchain_height , rsp.missed_ids.size());
	post_notify<NOTIFY_RESPONSE_GET_OBJECTS>(rsp, context);
	//handler_response_blocks_now(sizeof(rsp)); // XXX
	//handler_response_blocks_now(200);
	return 1;
}
//------------------------------------------------------------------------------------------------------------------------

template <class t_core>
double t_cryptonote_protocol_handler<t_core>::get_avg_block_size()
{
	CRITICAL_REGION_LOCAL(m_buffer_mutex);
	if(m_avg_buffer.empty())
	{
		GULPS_WARN("m_avg_buffer.size() == 0");
		return 500;
	}
	double avg = 0;
	for(const auto &element : m_avg_buffer)
		avg += element;
	return avg / m_avg_buffer.size();
}

template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_RESPONSE_GET_OBJECTS ({} blocks, {} txes)", arg.blocks.size() , arg.txs.size() );

	// calculate size of request
	size_t size = 0;
	for(const auto &element : arg.txs)
		size += element.size();

	size_t blocks_size = 0;
	for(const auto &element : arg.blocks)
	{
		blocks_size += element.block.size();
		for(const auto &tx : element.txs)
			blocks_size += tx.size();
	}
	size += blocks_size;

	for(const auto &element : arg.missed_ids)
		size += sizeof(element.data);

	size += sizeof(arg.current_blockchain_height);
	{
		CRITICAL_REGION_LOCAL(m_buffer_mutex);
		m_avg_buffer.push_back(size);
	}
	GULPSF_LOG_L1("{} downloaded {} bytes worth of blocks", context_str, size);

	/*using namespace boost::chrono;
      auto point = steady_clock::now();
      auto time_from_epoh = point.time_since_epoch();
      auto sec = duration_cast< seconds >( time_from_epoh ).count();*/

	//epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

	if(context.m_last_response_height > arg.current_blockchain_height)
	{
		GULPSF_LOG_ERROR("{} sent wrong NOTIFY_HAVE_OBJECTS: arg.m_current_blockchain_height={} < m_last_response_height={}, dropping connection", context_str, arg.current_blockchain_height
																							  , context.m_last_response_height );
		drop_connection(context, false, false);
		return 1;
	}

	context.m_remote_blockchain_height = arg.current_blockchain_height;
	if(context.m_remote_blockchain_height > m_core.get_target_blockchain_height())
		m_core.set_target_blockchain_height(context.m_remote_blockchain_height);

	std::vector<crypto::hash> block_hashes;
	block_hashes.reserve(arg.blocks.size());
	const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
	uint64_t start_height = std::numeric_limits<uint64_t>::max();
	cryptonote::block b;
	for(const block_complete_entry &block_entry : arg.blocks)
	{
		if(m_stopping)
		{
			return 1;
		}

		if(!parse_and_validate_block_from_blob(block_entry.block, b))
		{
			GULPSF_LOG_ERROR("{} sent wrong block: failed to parse and validate block: {}, dropping connection", context_str, epee::string_tools::buff_to_hex_nodelimer(block_entry.block) );
			drop_connection(context, false, false);
			return 1;
		}
		if(b.miner_tx.vin.size() != 1 || b.miner_tx.vin.front().type() != typeid(txin_gen))
		{
			GULPSF_LOG_ERROR("{} sent wrong block: block: miner tx does not have exactly one txin_gen input{}, dropping connection", context_str, epee::string_tools::buff_to_hex_nodelimer(block_entry.block) );
			drop_connection(context, false, false);
			return 1;
		}
		if(start_height == std::numeric_limits<uint64_t>::max())
			start_height = boost::get<txin_gen>(b.miner_tx.vin[0]).height;

		const crypto::hash block_hash = get_block_hash(b);
		auto req_it = context.m_requested_objects.find(block_hash);
		if(req_it == context.m_requested_objects.end())
		{
			GULPSF_LOG_ERROR("{} sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id={} wasn't requested, dropping connection", context_str, epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
																						);
			drop_connection(context, false, false);
			return 1;
		}
		if(b.tx_hashes.size() != block_entry.txs.size())
		{
			GULPSF_LOG_ERROR("{} sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id={}, tx_hashes.size()={} mismatch with block_complete_entry.m_txs.size()={}, dropping connection"
													, context_str, epee::string_tools::pod_to_hex(get_blob_hash(block_entry.block))
													, b.tx_hashes.size() , block_entry.txs.size());
			drop_connection(context, false, false);
			return 1;
		}

		context.m_requested_objects.erase(req_it);
		block_hashes.push_back(block_hash);
	}

	if(context.m_requested_objects.size())
	{
		GULPSF_LOG_L1("returned not all requested objects (context.m_requested_objects.size()={}), dropping connection", context.m_requested_objects.size() );
		drop_connection(context, false, false);
		return 1;
	}

	// get the last parsed block, which should be the highest one
	const crypto::hash last_block_hash = cryptonote::get_block_hash(b);
	if(m_core.have_block(last_block_hash))
	{
		const uint64_t subchain_height = start_height + arg.blocks.size();
		GULPSF_LOG_L1("{} These are old blocks, ignoring: blocks {} - {}, blockchain height {}", context_str, start_height , (subchain_height - 1) , m_core.get_current_blockchain_height());
		m_block_queue.remove_spans(context.m_connection_id, start_height);
		goto skip;
	}

	{
		GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_YELLOW, "{} Got NEW BLOCKS inside of {}: size: {}, blocks: {} - {}",
			context_str, __FUNCTION__, arg.blocks.size(), start_height, (start_height + arg.blocks.size() - 1));

		// add that new span to the block queue
		const boost::posix_time::time_duration dt = now - context.m_last_request_time;
		const float rate = size * 1e6 / (dt.total_microseconds() + 1);
		GULPSF_LOG_L1("{} adding span: {} at height {}, {} seconds, {} kB/s, size now {} MB", context_str, arg.blocks.size() , start_height , dt.total_microseconds() / 1e6 , (rate / 1e3) , (m_block_queue.get_data_size() + blocks_size) / 1048576.f );
		m_block_queue.add_blocks(start_height, arg.blocks, context.m_connection_id, rate, blocks_size);

		context.m_last_known_hash = last_block_hash;

		if(!m_core.get_test_drop_download() || !m_core.get_test_drop_download_height())
		{ // DISCARD BLOCKS for testing
			return 1;
		}
	}

skip:
	try_add_next_blocks(context);
	return 1;
}

template <class t_core>
int t_cryptonote_protocol_handler<t_core>::try_add_next_blocks(cryptonote_connection_context &context)
{
	bool force_next_span = false;

	{
		// We try to lock the sync lock. If we can, it means no other thread is
		// currently adding blocks, so we do that for as long as we can from the
		// block queue. Then, we go back to download.
		const boost::unique_lock<boost::mutex> sync{m_sync_lock, boost::try_to_lock};
		if(!sync.owns_lock())
		{
			GULPS_INFO("Failed to lock m_sync_lock, going back to download");
			goto skip;
		}
		GULPS_LOG_L1( context_str," lock m_sync_lock, adding blocks to chain...");

		{
			m_core.pause_mine();
			epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler(
				boost::bind(&t_core::resume_mine, &m_core));

			while(1)
			{
				const uint64_t previous_height = m_core.get_current_blockchain_height();
				uint64_t start_height;
				std::list<cryptonote::block_complete_entry> blocks;
				boost::uuids::uuid span_connection_id;
				if(!m_block_queue.get_next_span(start_height, blocks, span_connection_id))
				{
					GULPS_LOG_L1( context_str," no next span found, going back to download");
					break;
				}
				GULPSF_LOG_L1("{} next span in the queue has blocks {}-{}, we need {}", context_str, start_height , (start_height + blocks.size() - 1) , previous_height);

				if(blocks.empty())
				{
					GULPS_ERROR("Next span has no blocks");
					m_block_queue.remove_spans(span_connection_id, start_height);
					break;
				}

				block new_block;
				if(!parse_and_validate_block_from_blob(blocks.front().block, new_block))
				{
					GULPS_ERROR("Failed to parse block, but it should already have been parsed");
					m_block_queue.remove_spans(span_connection_id, start_height);
					break;
				}
				bool parent_known = m_core.have_block(new_block.prev_id);
				if(!parent_known)
				{
					// it could be:
					//  - later in the current chain
					//  - later in an alt chain
					//  - orphan
					// if it was requested, then it'll be resolved later, otherwise it's an orphan
					bool parent_requested = m_block_queue.requested(new_block.prev_id);
					if(!parent_requested)
					{
						// this can happen if a connection was sicced onto a late span, if it did not have those blocks,
						// since we don't know that at the sic time
						GULPS_ERROR( context_str, " Got block with unknown parent which was not requested - querying block hashes");
						m_block_queue.remove_spans(span_connection_id, start_height);
						context.m_needed_objects.clear();
						context.m_last_response_height = 0;
						goto skip;
					}

					// parent was requested, so we wait for it to be retrieved
					GULPS_INFO(" parent was requested, we'll get back to it");
					break;
				}

				const boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();
				context.m_last_request_time = start;

				m_core.prepare_handle_incoming_blocks(blocks);

				uint64_t block_process_time_full = 0, transactions_process_time_full = 0;
				size_t num_txs = 0;
				for(const block_complete_entry &block_entry : blocks)
				{
					if(m_stopping)
					{
						m_core.cleanup_handle_incoming_blocks();
						return 1;
					}

					// process transactions
					TIME_MEASURE_START(transactions_process_time);
					num_txs += block_entry.txs.size();
					std::vector<tx_verification_context> tvc;
					m_core.handle_incoming_txs(block_entry.txs, tvc, true, true, false);
					if(tvc.size() != block_entry.txs.size())
					{
						GULPS_ERROR( context_str, " Internal error: tvc.size() != block_entry.txs.size()");
						return 1;
					}
					std::list<blobdata>::const_iterator it = block_entry.txs.begin();
					for(size_t i = 0; i < tvc.size(); ++i, ++it)
					{
						if(tvc[i].m_verifivation_failed)
						{
							if(!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t f) -> bool {
								   GULPSF_LOG_ERROR("{} transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, tx_id = {}, dropping connection", context_str, epee::string_tools::pod_to_hex(get_blob_hash(*it)) );
								   drop_connection(context, false, true);
								   return 1;
							   }))
								GULPS_ERROR( context_str, " span connection id not found");

							if(!m_core.cleanup_handle_incoming_blocks())
							{
								GULPS_ERROR( context_str," Failure in cleanup_handle_incoming_blocks");
								return 1;
							}
							// in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
							m_block_queue.remove_spans(span_connection_id, start_height);
							return 1;
						}
					}
					TIME_MEASURE_FINISH(transactions_process_time);
					transactions_process_time_full += transactions_process_time;

					// process block

					TIME_MEASURE_START(block_process_time);
					block_verification_context bvc = boost::value_initialized<block_verification_context>();

					m_core.handle_incoming_block(block_entry.block, bvc, false); // <--- process block

					if(bvc.m_verifivation_failed)
					{
						if(!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t f) -> bool {
							   GULPS_INFO( context_str, " Block verification failed, dropping connection");
							   drop_connection(context, true, true);
							   return 1;
						   }))
							GULPS_ERROR( context_str, " span connection id not found");

						if(!m_core.cleanup_handle_incoming_blocks())
						{
							GULPS_ERROR( context_str, " Failure in cleanup_handle_incoming_blocks");
							return 1;
						}

						// in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
						m_block_queue.remove_spans(span_connection_id, start_height);
						return 1;
					}
					if(bvc.m_marked_as_orphaned)
					{
						if(!m_p2p->for_connection(span_connection_id, [&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t f) -> bool {
							   GULPS_INFO( context_str, " Block received at sync phase was marked as orphaned, dropping connection");
							   drop_connection(context, true, true);
							   return 1;
						   }))
							GULPS_ERROR( context_str, " span connection id not found");

						if(!m_core.cleanup_handle_incoming_blocks())
						{
							GULPS_ERROR( context_str, " Failure in cleanup_handle_incoming_blocks");
							return 1;
						}

						// in case the peer had dropped beforehand, remove the span anyway so other threads can wake up and get it
						m_block_queue.remove_spans(span_connection_id, start_height);
						return 1;
					}

					TIME_MEASURE_FINISH(block_process_time);
					block_process_time_full += block_process_time;

				} // each download block

				GULPSF_CAT_INFO("sync-info", "Block process time ({} blocks P{} txs):{} ({}/{}) ms", blocks.size(), num_txs, block_process_time_full + transactions_process_time_full, transactions_process_time_full, block_process_time_full);


				if(!m_core.cleanup_handle_incoming_blocks())
				{
					GULPS_ERROR( context_str, " Failure in cleanup_handle_incoming_blocks");
					return 1;
				}

				m_block_queue.remove_spans(span_connection_id, start_height);

				if(m_core.get_current_blockchain_height() > previous_height)
				{
					const boost::posix_time::time_duration dt = boost::posix_time::microsec_clock::universal_time() - start;
					GULPSF_GLOBAL_PRINT_CLR(gulps::COLOR_BOLD_YELLOW, "{} Synced {}/{} ({} sec, {} blocks/sec), {} MB queued", 
						context_str, m_core.get_current_blockchain_height(), m_core.get_target_blockchain_height(), 
						std::to_string(dt.total_microseconds() / 1e6), 
						std::to_string((m_core.get_current_blockchain_height() - previous_height) * 1e6 / dt.total_microseconds()),
						std::to_string(m_block_queue.get_data_size() / 1048576.f));
					GULPS_CAT_LOG_L1("global","", m_block_queue.get_overview());
				}
			}
		}

		if(should_download_next_span(context))
		{
			context.m_needed_objects.clear();
			context.m_last_response_height = 0;
			force_next_span = true;
		}
	}

skip:
	if(!request_missing_objects(context, true, force_next_span))
	{
		GULPS_ERROR( context_str, " Failed to request missing objects, dropping connection");
		drop_connection(context, false, false);
		return 1;
	}
	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::on_idle()
{
	m_idle_peer_kicker.do_call(boost::bind(&t_cryptonote_protocol_handler<t_core>::kick_idle_peers, this));
	return m_core.on_idle();
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::kick_idle_peers()
{
	GULPS_LOG_L2("Checking for idle peers...");
	std::vector<boost::uuids::uuid> kick_connections;
	m_p2p->for_each_connection([&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) -> bool {
		if(context.m_state == cryptonote_connection_context::state_synchronizing || context.m_state == cryptonote_connection_context::state_before_handshake)
		{
			const bool passive = context.m_state == cryptonote_connection_context::state_before_handshake;
			const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
			const boost::posix_time::time_duration dt = now - context.m_last_request_time;
			const int64_t threshold = passive ? PASSIVE_PEER_KICK_TIME : IDLE_PEER_KICK_TIME;
			if(dt.total_microseconds() > threshold)
			{
				GULPS_INFO(context_str, " kicking ", passive ? "passive" : "idle", "peer");
				kick_connections.push_back(context.m_connection_id);
			}
		}
		return true;
	});
	for(const boost::uuids::uuid &conn_id : kick_connections)
	{
		m_p2p->for_connection(conn_id, [this](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) {
			drop_connection(context, false, false);
			return true;
		});
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_REQUEST_CHAIN ({} blocks", arg.block_ids.size() );
	NOTIFY_RESPONSE_CHAIN_ENTRY::request r;
	if(!m_core.find_blockchain_supplement(arg.block_ids, r))
	{
		GULPS_ERROR( context_str, " Failed to handle NOTIFY_REQUEST_CHAIN.");
		drop_connection(context, false, false);
		return 1;
	}
	GULPSF_LOG_L1("{}-->>NOTIFY_RESPONSE_CHAIN_ENTRY: m_start_height={}, m_total_height={}, m_block_ids.size()={}", context_str, r.start_height , r.total_height , r.m_block_ids.size());
	post_notify<NOTIFY_RESPONSE_CHAIN_ENTRY>(r, context);
	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::should_download_next_span(cryptonote_connection_context &context) const
{
	std::list<crypto::hash> hashes;
	boost::uuids::uuid span_connection_id;
	boost::posix_time::ptime request_time;
	std::pair<uint64_t, uint64_t> span;

	span = m_block_queue.get_start_gap_span();
	if(span.second > 0)
	{
		GULPS_LOG_L1( context_str," we should download it as there is a gap");
		return true;
	}

	// if the next span is not scheduled (or there is none)
	span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, request_time);
	if(span.second == 0)
	{
		// we might be in a weird case where there is a filled next span,
		// but it starts higher than the current height
		uint64_t height;
		std::list<cryptonote::block_complete_entry> bcel;
		if(!m_block_queue.get_next_span(height, bcel, span_connection_id, true))
			return false;
		if(height > m_core.get_current_blockchain_height())
		{
			GULPS_LOG_L1( context_str," we should download it as the next block isn't scheduled");
			return true;
		}
		return false;
	}
	// if it was scheduled by this particular peer
	if(span_connection_id == context.m_connection_id)
		return false;

	float span_speed = m_block_queue.get_speed(span_connection_id);
	float speed = m_block_queue.get_speed(context.m_connection_id);
	GULPSF_LOG_L1("{} next span is scheduled for {}, speed {}, ours {}", context_str, boost::uuids::to_string(span_connection_id), span_speed , speed);

	// we try for that span too if:
	//  - we're substantially faster, or:
	//  - we're the fastest and the other one isn't (avoids a peer being waaaay slow but yet unmeasured)
	//  - the other one asked at least 5 seconds ago
	if(span_speed < .25 && speed > .75f)
	{
		GULPS_LOG_L1( context_str, " we should download it as we're substantially faster");
		return true;
	}
	if(speed == 1.0f && span_speed != 1.0f)
	{
		GULPS_LOG_L1( context_str, " we should download it as we're the fastest peer");
		return true;
	}
	const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
	if((now - request_time).total_microseconds() > REQUEST_NEXT_SCHEDULED_SPAN_THRESHOLD)
	{
		GULPS_LOG_L1( context_str, " we should download it as this span was requested long ago");
		return true;
	}
	return false;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::request_missing_objects(cryptonote_connection_context &context, bool check_having_blocks, bool force_next_span)
{
	// flush stale spans
	std::set<boost::uuids::uuid> live_connections;
	m_p2p->for_each_connection([&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) -> bool {
		live_connections.insert(context.m_connection_id);
		return true;
	});
	m_block_queue.flush_stale_spans(live_connections);

	// if we don't need to get next span, and the block queue is full enough, wait a bit
	bool start_from_current_chain = false;
	if(!force_next_span)
	{
		bool first = true;
		while(1)
		{
			size_t nblocks = m_block_queue.get_num_filled_spans();
			size_t size = m_block_queue.get_data_size();
			if(nblocks < BLOCK_QUEUE_NBLOCKS_THRESHOLD || size < BLOCK_QUEUE_SIZE_THRESHOLD)
			{
				if(!first)
				{
					GULPSF_LOG_L1("{} Block queue is {} and {}, resuming", context_str, nblocks , size );
				}
				break;
			}

			// this one triggers if all threads are in standby, which should not happen,
			// but happened at least once, so we unblock at least one thread if so
			const boost::unique_lock<boost::mutex> sync{m_sync_lock, boost::try_to_lock};
			if(sync.owns_lock())
			{
				GULPS_LOG_L1( context_str, " No other thread is adding blocks, resuming");
				break;
			}

			if(should_download_next_span(context))
			{
				GULPS_LOG_L1( context_str, " we should try for that next span too, we think we could get it faster, resuming");
				force_next_span = true;
				break;
			}

			if(first)
			{
				GULPSF_LOG_L1("{} Block queue is {} and {}, pausing", context_str, nblocks , size );
				first = false;
				context.m_state = cryptonote_connection_context::state_standby;
			}

			// this needs doing after we went to standby, so the callback knows what to do
			bool filled;
			if(m_block_queue.has_next_span(context.m_connection_id, filled) && !filled)
			{
				GULPS_LOG_L1( context_str, " we have the next span, and it is scheduled, resuming");
				++context.m_callback_request_count;
				m_p2p->request_callback(context);
				return true;
			}

			for(size_t n = 0; n < 50; ++n)
			{
				if(m_stopping)
					return true;
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
		}
		context.m_state = cryptonote_connection_context::state_synchronizing;
	}

	GULPSF_LOG_L1("{} request_missing_objects: check {}, force_next_span {}, m_needed_objects {} lrh {}, chain {}", context_str, check_having_blocks , force_next_span , context.m_needed_objects.size() , context.m_last_response_height , m_core.get_current_blockchain_height());
	if(context.m_needed_objects.size() || force_next_span)
	{
		//we know objects that we need, request this objects
		NOTIFY_REQUEST_GET_OBJECTS::request req;
		bool is_next = false;
		size_t count = 0;
		const size_t count_limit = m_core.get_block_sync_size(m_core.get_current_blockchain_height());
		std::pair<uint64_t, uint64_t> span = std::make_pair(0, 0);
		{
			GULPS_LOG_L1(" checking for gap");
			span = m_block_queue.get_start_gap_span();
			if(span.second > 0)
			{
				const uint64_t first_block_height_known = context.m_last_response_height - context.m_needed_objects.size() + 1;
				const uint64_t last_block_height_known = context.m_last_response_height;
				const uint64_t first_block_height_needed = span.first;
				const uint64_t last_block_height_needed = span.first + std::min(span.second, (uint64_t)count_limit) - 1;
				GULPSF_LOG_L1("{} gap found, span: {} - {} ({})", context_str, span.first , span.first + span.second - 1 , last_block_height_needed );
				GULPSF_LOG_L1("{} current known hashes from {} to {}", context_str, first_block_height_known , last_block_height_known);
				if(first_block_height_needed < first_block_height_known || last_block_height_needed > last_block_height_known)
				{
					GULPS_LOG_L1( context_str, " we are missing some of the necessary hashes for this gap, requesting chain again");
					context.m_needed_objects.clear();
					context.m_last_response_height = 0;
					start_from_current_chain = true;
					goto skip;
				}
				GULPS_LOG_L1( context_str, " we have the hashes for this gap");
			}
		}
		if(force_next_span)
		{
			if(span.second == 0)
			{
				std::list<crypto::hash> hashes;
				boost::uuids::uuid span_connection_id;
				boost::posix_time::ptime time;
				span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, time);
				if(span.second > 0)
				{
					is_next = true;
					for(const auto &hash : hashes)
					{
						req.blocks.push_back(hash);
						context.m_requested_objects.insert(hash);
					}
				}
			}
		}
		if(span.second == 0)
		{
			GULPS_LOG_L1( context_str, " span size is 0");
			if(context.m_last_response_height + 1 < context.m_needed_objects.size())
			{
				GULPSF_LOG_L1("{} ERROR: inconsistent context: lrh {}, nos {}", context_str, context.m_last_response_height , context.m_needed_objects.size());
				context.m_needed_objects.clear();
				context.m_last_response_height = 0;
				goto skip;
			}
			// take out blocks we already have
			while(!context.m_needed_objects.empty() && m_core.have_block(context.m_needed_objects.front()))
			{
				// if we're popping the last hash, record it so we can ask again from that hash,
				// this prevents never being able to progress on peers we get old hash lists from
				if(context.m_needed_objects.size() == 1)
					context.m_last_known_hash = context.m_needed_objects.front();
				context.m_needed_objects.pop_front();
			}
			const uint64_t first_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
			span = m_block_queue.reserve_span(first_block_height, context.m_last_response_height, count_limit, context.m_connection_id, context.m_needed_objects);
			GULPSF_LOG_L1("{} span from {}: {}/{}", context_str, first_block_height , span.first , span.second);
		}
		if(span.second == 0 && !force_next_span)
		{
			GULPS_LOG_L1( context_str, " still no span reserved, we may be in the corner case of next span scheduled and everything else scheduled/filled");
			std::list<crypto::hash> hashes;
			boost::uuids::uuid span_connection_id;
			boost::posix_time::ptime time;
			span = m_block_queue.get_next_span_if_scheduled(hashes, span_connection_id, time);
			if(span.second > 0)
			{
				is_next = true;
				for(const auto &hash : hashes)
				{
					req.blocks.push_back(hash);
					++count;
					context.m_requested_objects.insert(hash);
					// that's atrocious O(n) wise, but this is rare
					auto i = std::find(context.m_needed_objects.begin(), context.m_needed_objects.end(), hash);
					if(i != context.m_needed_objects.end())
						context.m_needed_objects.erase(i);
				}
			}
		}
		GULPSF_LOG_L1("{} span: {}/{} ({} - {})", context_str, span.first , span.second , span.first , (span.first + span.second - 1) );
		if(span.second > 0)
		{
			if(!is_next)
			{
				const uint64_t first_context_block_height = context.m_last_response_height - context.m_needed_objects.size() + 1;
				uint64_t skip = span.first - first_context_block_height;
				if(skip > context.m_needed_objects.size())
				{
					GULPS_ERROR("ERROR: skip {}, m_needed_objects {}, first_context_block_height{}", skip , context.m_needed_objects.size() , first_context_block_height);
					return false;
				}
				while(skip--)
					context.m_needed_objects.pop_front();
				if(context.m_needed_objects.size() < span.second)
				{
					GULPS_ERROR("ERROR: span {}/{}, m_needed_objects {}", span.first , span.second , context.m_needed_objects.size());
					return false;
				}

				auto it = context.m_needed_objects.begin();
				for(size_t n = 0; n < span.second; ++n)
				{
					req.blocks.push_back(*it);
					++count;
					context.m_requested_objects.insert(*it);
					auto j = it++;
					context.m_needed_objects.erase(j);
				}
			}

			context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
			GULPSF_INFO("{} -->>NOTIFY_REQUEST_GET_OBJECTS: blocks.size()={}, txs.size()={}requested blocks count={} / {} from {}, first hash {}", context_str, req.blocks.size() , req.txs.size() , count , count_limit , span.first , req.blocks.front());
			//epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());

			post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);
			return true;
		}
	}

skip:
	context.m_needed_objects.clear();
	if(context.m_last_response_height < context.m_remote_blockchain_height - 1)
	{ //we have to fetch more objects ids, request blockchain entry

		NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
		m_core.get_short_chain_history(r.block_ids);
		GULPS_CHECK_AND_ASSERT_MES(!r.block_ids.empty(), false, "Short chain history is empty");

		if(!start_from_current_chain)
		{
			// we'll want to start off from where we are on that peer, which may not be added yet
			if(context.m_last_known_hash != crypto::null_hash && r.block_ids.front() != context.m_last_known_hash)
				r.block_ids.push_front(context.m_last_known_hash);
		}

		handler_request_blocks_history(r.block_ids); // change the limit(?), sleep(?)

		//std::string blob; // for calculate size of request
		//epee::serialization::store_t_to_binary(r, blob);
		//epee::net_utils::network_throttle_manager::get_global_throttle_inreq().logger_handle_net("log/dr-monero/net/req-all.data", sec, get_avg_block_size());
		//GULPS_INFO( context_str, "r = ", 200);

		context.m_last_request_time = boost::posix_time::microsec_clock::universal_time();
		GULPSF_INFO("{} -->>NOTIFY_REQUEST_CHAIN: m_block_ids.size()={}, start_from_current_chain {}", context_str, r.block_ids.size() , start_from_current_chain);
		post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
	}
	else
	{
		GULPS_CHECK_AND_ASSERT_MES(context.m_last_response_height == context.m_remote_blockchain_height - 1 && !context.m_needed_objects.size() && !context.m_requested_objects.size(), false, "request_missing_blocks final condition failed!"
																																															 ,"\r\nm_last_response_height=", context.m_last_response_height
																																															 ,"\r\nm_remote_blockchain_height=", context.m_remote_blockchain_height
																																															 ,"\r\nm_needed_objects.size()=", context.m_needed_objects.size()
																																															 ,"\r\nm_requested_objects.size()=", context.m_requested_objects.size()
																																															 ,"\r\non connection [", epee::net_utils::print_connection_context_short(context), "]");

		context.m_state = cryptonote_connection_context::state_normal;
		if(context.m_remote_blockchain_height >= m_core.get_target_blockchain_height())
		{
			if(m_core.get_current_blockchain_height() >= m_core.get_target_blockchain_height())
			{
				GULPS_GLOBAL_PRINT_CLR(gulps::COLOR_GREEN, "SYNCHRONIZED OK");
				on_connection_synchronized();
			}
		}
		else
		{
			GULPS_INFO( context_str, " we've reached this peer's blockchain height");
		}
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::on_connection_synchronized()
{
	bool val_expected = false;
	if(m_synchronized.compare_exchange_strong(val_expected, true))
	{
		GULPS_GLOBAL_PRINT_CLR(gulps::COLOR_BOLD_GREEN, "\n**********************************************************************\n",
						   "You are now synchronized with the network. You may now start ryo-wallet-cli.\n\n",
						   "Use the \"help\" command to see the list of available commands.\n",
						   "**********************************************************************\n");
		m_core.on_synchronized();
	}
	m_core.safesyncmode(true);
	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
size_t t_cryptonote_protocol_handler<t_core>::get_synchronizing_connections_count()
{
	size_t count = 0;
	m_p2p->for_each_connection([&](cryptonote_connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) -> bool {
		if(context.m_state == cryptonote_connection_context::state_synchronizing)
			++count;
		return true;
	});
	return count;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
int t_cryptonote_protocol_handler<t_core>::handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request &arg, cryptonote_connection_context &context)
{
	GULPS_P2P_MESSAGE("Received NOTIFY_RESPONSE_CHAIN_ENTRY: m_block_ids.size()={}, m_start_height={}, m_total_height={}", arg.m_block_ids.size() , arg.start_height , arg.total_height);

	if(!arg.m_block_ids.size())
	{
		GULPS_ERROR( context_str, "sent empty m_block_ids, dropping connection");
		drop_connection(context, true, false);
		return 1;
	}
	if(arg.total_height < arg.m_block_ids.size() || arg.start_height > arg.total_height - arg.m_block_ids.size())
	{
		GULPS_ERROR( context_str, " sent invalid start/nblocks/height, dropping connection");
		drop_connection(context, true, false);
		return 1;
	}

	context.m_remote_blockchain_height = arg.total_height;
	context.m_last_response_height = arg.start_height + arg.m_block_ids.size() - 1;
	if(context.m_last_response_height > context.m_remote_blockchain_height)
	{
		GULPSF_LOG_ERROR("{} sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with m_total_height={}, m_start_height={}, m_block_ids.size()={}", context_str
																						  , arg.total_height
																						  , arg.start_height
																						  , arg.m_block_ids.size());
		drop_connection(context, false, false);
		return 1;
	}

	uint64_t n_use_blocks = m_core.prevalidate_block_hashes(arg.start_height, arg.m_block_ids);
	if(n_use_blocks + HASH_OF_HASHES_STEP <= arg.m_block_ids.size())
	{
		GULPS_ERROR( context_str, " Most blocks are invalid, dropping connection");
		drop_connection(context, true, false);
		return 1;
	}

	uint64_t added = 0;
	for(auto &bl_id : arg.m_block_ids)
	{
		context.m_needed_objects.push_back(bl_id);
		if(++added == n_use_blocks)
			break;
	}
	context.m_last_response_height -= arg.m_block_ids.size() - n_use_blocks;

	if(!request_missing_objects(context, false))
	{
		GULPS_ERROR( context_str, " Failed to request missing objects, dropping connection");
		drop_connection(context, false, false);
		return 1;
	}

	if(arg.total_height > m_core.get_target_blockchain_height())
		m_core.set_target_blockchain_height(arg.total_height);

	return 1;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::relay_block(NOTIFY_NEW_BLOCK::request &arg, cryptonote_connection_context &exclude_context)
{
	NOTIFY_NEW_FLUFFY_BLOCK::request fluffy_arg = AUTO_VAL_INIT(fluffy_arg);
	fluffy_arg.current_blockchain_height = arg.current_blockchain_height;
	std::list<blobdata> fluffy_txs;
	fluffy_arg.b = arg.b;
	fluffy_arg.b.txs = fluffy_txs;

	// pre-serialize them
	std::string fullBlob, fluffyBlob;
	epee::serialization::store_t_to_binary(arg, fullBlob);
	epee::serialization::store_t_to_binary(fluffy_arg, fluffyBlob);

	// sort peers between fluffy ones and others
	std::list<boost::uuids::uuid> fullConnections, fluffyConnections;
	m_p2p->for_each_connection([this, &exclude_context, &fullConnections, &fluffyConnections](connection_context &context, nodetool::peerid_type peer_id, uint32_t support_flags) {
		if(peer_id && exclude_context.m_connection_id != context.m_connection_id)
		{
			if(m_core.fluffy_blocks_enabled() && (support_flags & P2P_SUPPORT_FLAG_FLUFFY_BLOCKS))
			{
				GULPS_LOG_L1( context_str, " PEER SUPPORTS FLUFFY BLOCKS - RELAYING THIN/COMPACT WHATEVER BLOCK");
				fluffyConnections.push_back(context.m_connection_id);
			}
			else
			{
				GULPS_LOG_L1( context_str, " PEER DOESN'T SUPPORT FLUFFY BLOCKS - RELAYING FULL BLOCK");
				fullConnections.push_back(context.m_connection_id);
			}
		}
		return true;
	});

	// send fluffy ones first, we want to encourage people to run that
	m_p2p->relay_notify_to_list(NOTIFY_NEW_FLUFFY_BLOCK::ID, fluffyBlob, fluffyConnections);
	m_p2p->relay_notify_to_list(NOTIFY_NEW_BLOCK::ID, fullBlob, fullConnections);

	return true;
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
bool t_cryptonote_protocol_handler<t_core>::relay_transactions(NOTIFY_NEW_TRANSACTIONS::request &arg, cryptonote_connection_context &exclude_context)
{
	// no check for success, so tell core they're relayed unconditionally
	for(auto tx_blob_it = arg.txs.begin(); tx_blob_it != arg.txs.end(); ++tx_blob_it)
		m_core.on_transaction_relayed(*tx_blob_it);
	return relay_post_notify<NOTIFY_NEW_TRANSACTIONS>(arg, exclude_context);
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
void t_cryptonote_protocol_handler<t_core>::drop_connection(cryptonote_connection_context &context, bool add_fail, bool flush_all_spans)
{
	if(add_fail)
		m_p2p->add_host_fail(context.m_remote_address);

	m_block_queue.flush_spans(context.m_connection_id, flush_all_spans);
	m_p2p->drop_connection(context);
}
//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
void t_cryptonote_protocol_handler<t_core>::on_connection_close(cryptonote_connection_context &context)
{
	uint64_t target = 0;
	m_p2p->for_each_connection([&](const connection_context &cntxt, nodetool::peerid_type peer_id, uint32_t support_flags) {
		if(cntxt.m_state >= cryptonote_connection_context::state_synchronizing && cntxt.m_connection_id != context.m_connection_id)
			target = std::max(target, cntxt.m_remote_blockchain_height);
		return true;
	});
	const uint64_t previous_target = m_core.get_target_blockchain_height();
	if(target < previous_target)
	{
		GULPSF_INFO("Target height decreasing from {} to {}", previous_target , target);
		m_core.set_target_blockchain_height(target);
	}

	m_block_queue.flush_spans(context.m_connection_id, false);
}

//------------------------------------------------------------------------------------------------------------------------
template <class t_core>
void t_cryptonote_protocol_handler<t_core>::stop()
{
	m_stopping = true;
	m_core.stop();
}
} // namespace
