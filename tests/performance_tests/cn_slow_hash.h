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
#include "crypto/pow_hash/cn_slow_hash.hpp"
#include "cryptonote_basic/cryptonote_basic.h"
#include "string_tools.h"

template <bool VERSION1>
class test_cn_slow_hash
{
  public:
	static const size_t loop_count = 10;

#pragma pack(push, 1)
	struct data_t
	{
		char data[13];
	};
#pragma pack(pop)

	static_assert(13 == sizeof(data_t), "Invalid structure size");

	bool init()
	{
		const char* exp_result = VERSION1 ? "bbec2cacf69866a8e740380fe7b818fc78f8571221742d729d9d02d7f8989b87" :
			"45f1fbd7ecdbbf9a94c1d55ce7e5aa9ca37de9f77568cdde243f77f6663cc278";
		
		if(!epee::string_tools::hex_to_pod("63617665617420656d70746f72", m_data))
			return false;

		if(!epee::string_tools::hex_to_pod(exp_result, m_expected_hash))
			return false;

		return true;
	}

	bool test()
	{
		crypto::hash hash;
		if(VERSION1)
		{
			cn_pow_hash_v1 hb = cn_pow_hash_v2::make_borrowed(m_hash);
			hb.hash(&m_data, sizeof(m_data), &hash);
		}
		else
			m_hash.hash(&m_data, sizeof(m_data), &hash);

		return hash == m_expected_hash;
	}

  private:
	data_t m_data;
	cn_pow_hash_v2 m_hash;
	crypto::hash m_expected_hash;
};
