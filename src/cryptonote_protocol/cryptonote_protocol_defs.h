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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "serialization/keyvalue_serialization.h"
#include <list>
namespace cryptonote
{

#define BC_COMMANDS_POOL_BASE 2000

/************************************************************************/
/* P2P connection info, serializable to json                            */
/************************************************************************/
struct connection_info
{
	bool incoming;
	bool localhost;
	bool local_ip;

	std::string address;
	std::string host;
	std::string ip;
	std::string port;

	std::string peer_id;

	uint64_t recv_count;
	uint64_t recv_idle_time;

	uint64_t send_count;
	uint64_t send_idle_time;

	std::string state;

	uint64_t live_time;

	uint64_t avg_download;
	uint64_t current_download;

	uint64_t avg_upload;
	uint64_t current_upload;

	uint32_t support_flags;

	std::string connection_id;

	uint64_t height;

	BEGIN_KV_SERIALIZE_MAP(connection_info)
	KV_SERIALIZE(incoming)
	KV_SERIALIZE(localhost)
	KV_SERIALIZE(local_ip)
	KV_SERIALIZE(address)
	KV_SERIALIZE(host)
	KV_SERIALIZE(ip)
	KV_SERIALIZE(port)
	KV_SERIALIZE(peer_id)
	KV_SERIALIZE(recv_count)
	KV_SERIALIZE(recv_idle_time)
	KV_SERIALIZE(send_count)
	KV_SERIALIZE(send_idle_time)
	KV_SERIALIZE(state)
	KV_SERIALIZE(live_time)
	KV_SERIALIZE(avg_download)
	KV_SERIALIZE(current_download)
	KV_SERIALIZE(avg_upload)
	KV_SERIALIZE(current_upload)
	KV_SERIALIZE(support_flags)
	KV_SERIALIZE(connection_id)
	KV_SERIALIZE(height)
	END_KV_SERIALIZE_MAP()
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct block_complete_entry
{
	block_complete_entry(blobdata block, std::list<blobdata> txs) :
		block(block), txs(txs) {}

	blobdata block;
	std::list<blobdata> txs;
	BEGIN_KV_SERIALIZE_MAP(block_complete_entry)
	KV_SERIALIZE(block)
	KV_SERIALIZE(txs)
	END_KV_SERIALIZE_MAP()
};

// As far as epee is concerned, those two are interchangeable
struct block_complete_entry_v
{
	block_complete_entry_v(blobdata block, std::vector<blobdata> txs) :
		block(block), txs(txs) {}

	blobdata block;
	std::vector<blobdata> txs;
	BEGIN_KV_SERIALIZE_MAP(block_complete_entry_v)
	KV_SERIALIZE(block)
	KV_SERIALIZE(txs)
	END_KV_SERIALIZE_MAP()
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct NOTIFY_NEW_BLOCK
{
	const static int ID = BC_COMMANDS_POOL_BASE + 1;

	struct request
	{
		block_complete_entry b;
		uint64_t current_blockchain_height;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE(b)
		KV_SERIALIZE(current_blockchain_height)
		END_KV_SERIALIZE_MAP()
	};
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct NOTIFY_NEW_TRANSACTIONS
{
	const static int ID = BC_COMMANDS_POOL_BASE + 2;

	struct request
	{
		std::list<blobdata> txs;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE(txs)
		END_KV_SERIALIZE_MAP()
	};
};
/************************************************************************/
/*                                                                      */
/************************************************************************/
struct NOTIFY_REQUEST_GET_OBJECTS
{
	const static int ID = BC_COMMANDS_POOL_BASE + 3;

	struct request
	{
		std::list<crypto::hash> txs;
		std::list<crypto::hash> blocks;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txs)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(blocks)
		END_KV_SERIALIZE_MAP()
	};
};

struct NOTIFY_RESPONSE_GET_OBJECTS
{
	const static int ID = BC_COMMANDS_POOL_BASE + 4;

	struct request
	{
		std::list<blobdata> txs;
		std::list<block_complete_entry> blocks;
		std::list<crypto::hash> missed_ids;
		uint64_t current_blockchain_height;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE(txs)
		KV_SERIALIZE(blocks)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missed_ids)
		KV_SERIALIZE(current_blockchain_height)
		END_KV_SERIALIZE_MAP()
	};
};

struct CORE_SYNC_DATA
{
	uint64_t current_height;
	uint64_t cumulative_difficulty;
	crypto::hash top_id;
	uint8_t top_version;

	BEGIN_KV_SERIALIZE_MAP(CORE_SYNC_DATA)
	KV_SERIALIZE(current_height)
	KV_SERIALIZE(cumulative_difficulty)
	KV_SERIALIZE_VAL_POD_AS_BLOB(top_id)
	KV_SERIALIZE_OPT(top_version, (uint8_t)0)
	END_KV_SERIALIZE_MAP()
};

struct NOTIFY_REQUEST_CHAIN
{
	const static int ID = BC_COMMANDS_POOL_BASE + 6;

	struct request
	{
		std::list<crypto::hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
		END_KV_SERIALIZE_MAP()
	};
};

struct NOTIFY_RESPONSE_CHAIN_ENTRY
{
	const static int ID = BC_COMMANDS_POOL_BASE + 7;

	struct request
	{
		uint64_t start_height;
		uint64_t total_height;
		uint64_t cumulative_difficulty;
		std::list<crypto::hash> m_block_ids;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE(start_height)
		KV_SERIALIZE(total_height)
		KV_SERIALIZE(cumulative_difficulty)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
		END_KV_SERIALIZE_MAP()
	};
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct NOTIFY_NEW_FLUFFY_BLOCK
{
	const static int ID = BC_COMMANDS_POOL_BASE + 8;

	struct request
	{
		block_complete_entry b;
		uint64_t current_blockchain_height;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE(b)
		KV_SERIALIZE(current_blockchain_height)
		END_KV_SERIALIZE_MAP()
	};
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct NOTIFY_REQUEST_FLUFFY_MISSING_TX
{
	const static int ID = BC_COMMANDS_POOL_BASE + 9;

	struct request
	{
		crypto::hash block_hash;
		uint64_t current_blockchain_height;
		std::vector<uint64_t> missing_tx_indices;

		BEGIN_KV_SERIALIZE_MAP(request)
		KV_SERIALIZE_VAL_POD_AS_BLOB(block_hash)
		KV_SERIALIZE(current_blockchain_height)
		KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missing_tx_indices)
		END_KV_SERIALIZE_MAP()
	};
};
} // namespace cryptonote
