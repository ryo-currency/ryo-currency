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
#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "ringct/rctOps.h"
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

namespace cryptonote
{
//---------------------------------------------------------------
bool construct_miner_tx(network_type nettype, size_t height, size_t median_size, uint64_t already_generated_coins, size_t current_block_size, uint64_t fee, const account_public_address &miner_address, transaction &tx, const blobdata &extra_nonce = blobdata());

struct tx_source_entry
{
	typedef std::pair<uint64_t, rct::ctkey> output_entry;

	std::vector<output_entry> outputs;							 //index + key + optional ringct commitment
	size_t real_output;											 //index in outputs vector of real output_entry
	crypto::public_key real_out_tx_key;							 //incoming real tx public key
	std::vector<crypto::public_key> real_out_additional_tx_keys; //incoming real tx additional public keys
	size_t real_output_in_tx_index;								 //index in transaction outputs vector
	uint64_t amount;											 //money
	bool rct;													 //true if the output is rct
	rct::key mask;												 //ringct amount mask
	rct::multisig_kLRki multisig_kLRki;							 //multisig info

	void push_output(uint64_t idx, const crypto::public_key &k, uint64_t amount) { outputs.push_back(std::make_pair(idx, rct::ctkey({rct::pk2rct(k), rct::zeroCommit(amount)}))); }

	BEGIN_SERIALIZE_OBJECT()
	FIELD(outputs)
	FIELD(real_output)
	FIELD(real_out_tx_key)
	FIELD(real_out_additional_tx_keys)
	FIELD(real_output_in_tx_index)
	FIELD(amount)
	FIELD(rct)
	FIELD(mask)
	FIELD(multisig_kLRki)

	if(real_output >= outputs.size())
		return false;
	END_SERIALIZE()
};

struct tx_destination_entry
{
	uint64_t amount;			 //money
	account_public_address addr; //destination address
	bool is_subaddress;

	tx_destination_entry() : amount(0), addr(AUTO_VAL_INIT(addr)), is_subaddress(false) {}
	tx_destination_entry(uint64_t a, const account_public_address &ad, bool is_subaddress) : amount(a), addr(ad), is_subaddress(is_subaddress) {}

	BEGIN_SERIALIZE_OBJECT()
	VARINT_FIELD(amount)
	FIELD(addr)
	FIELD(is_subaddress)
	END_SERIALIZE()
};

//---------------------------------------------------------------
crypto::public_key get_destination_view_key_pub(const std::vector<tx_destination_entry> &destinations, const boost::optional<cryptonote::account_public_address> &change_addr);
bool construct_tx(const account_keys &sender_account_keys, std::vector<tx_source_entry> &sources, const std::vector<tx_destination_entry> &destinations, const boost::optional<cryptonote::account_public_address> &change_addr, const crypto::uniform_payment_id* payment_id, transaction &tx, uint64_t unlock_time);
bool construct_tx_with_tx_key(const account_keys &sender_account_keys, const std::unordered_map<crypto::public_key, subaddress_index> &subaddresses, std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations, const boost::optional<cryptonote::account_public_address> &change_addr, const crypto::uniform_payment_id* payment_id, transaction &tx, uint64_t unlock_time, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, bool bulletproof = false, rct::multisig_out *msout = NULL, bool use_uniform_pids = false);
bool construct_tx_and_get_tx_key(const account_keys &sender_account_keys, const std::unordered_map<crypto::public_key, subaddress_index> &subaddresses, std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations, const boost::optional<cryptonote::account_public_address> &change_addr, const crypto::uniform_payment_id* payment_id, transaction &tx, uint64_t unlock_time, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys, bool bulletproof = false, rct::multisig_out *msout = NULL, bool use_uniform_pids = false);

bool generate_genesis_block(network_type nettype, block &bl, std::string const &genesis_tx, uint32_t nonce);
}

BOOST_CLASS_VERSION(cryptonote::tx_source_entry, 1)
BOOST_CLASS_VERSION(cryptonote::tx_destination_entry, 1)

namespace boost
{
namespace serialization
{
template <class Archive>
inline void serialize(Archive &a, cryptonote::tx_source_entry &x, const boost::serialization::version_type ver)
{
	a &x.outputs;
	a &x.real_output;
	a &x.real_out_tx_key;
	a &x.real_output_in_tx_index;
	a &x.amount;
	a &x.rct;
	a &x.mask;
	if(ver < 1)
		return;
	a &x.multisig_kLRki;
	a &x.real_out_additional_tx_keys;
}

template <class Archive>
inline void serialize(Archive &a, cryptonote::tx_destination_entry &x, const boost::serialization::version_type ver)
{
	a &x.amount;
	a &x.addr;
	if(ver < 1)
		return;
	a &x.is_subaddress;
}
}
}
