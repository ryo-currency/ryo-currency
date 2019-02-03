// Copyright (c) 2014-2018, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
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

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"

#include "multi_tx_test_base.h"

class test_ge_frombytes_vartime : public multi_tx_test_base<2>
{
  public:
	static constexpr size_t out_count = 1;
	static const size_t loop_count = 10000;

	bool init()
	{
		using namespace cryptonote;

		if(!base_class_t::init())
			return false;

		cryptonote::account_base m_alice;
		cryptonote::transaction m_tx;
		std::vector<tx_destination_entry> destinations;

		m_alice.generate_new(0);

		for(size_t i = 0; i < out_count; ++i)
			destinations.push_back(tx_destination_entry(this->m_source_amount / out_count, m_alice.get_keys().m_account_address, false));

		crypto::secret_key tx_key;
		std::vector<crypto::secret_key> additional_tx_keys;
		std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
		subaddresses[this->m_miners[this->real_source_idx].get_keys().m_account_address.m_spend_public_key] = {0, 0};

		if(!cryptonote::construct_tx_and_get_tx_key(this->m_miners[this->real_source_idx].get_keys(), subaddresses, 
			this->m_sources, destinations, cryptonote::account_public_address{}, nullptr, m_tx, 0, tx_key, additional_tx_keys, true, nullptr, true))
		{
			return false;
		}

		const cryptonote::txin_to_key &txin = boost::get<cryptonote::txin_to_key>(m_tx.vin[0]);
		m_key = rct::ki2rct(txin.k_image);

		return true;
	}

	bool test()
	{
		ge_p3 unp;
		return ge_frombytes_vartime(&unp, (const unsigned char *)&m_key) == 0;
	}

  private:
	rct::key m_key;
};
