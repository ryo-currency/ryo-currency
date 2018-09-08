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

#include "gtest/gtest.h"

#include "cryptonote_core/blockchain.h"

using namespace cryptonote;

namespace
{
static uint64_t clamp_fee(uint64_t fee)
{
	static uint64_t mask = 0;
	if(mask == 0)
	{
		mask = 1;
		for(size_t n = PER_KB_FEE_QUANTIZATION_DECIMALS; n < CRYPTONOTE_DISPLAY_DECIMAL_POINT; ++n)
			mask *= 10;
	}
	return (fee + mask - 1) / mask * mask;
}

//--------------------------------------------------------------------------------------------------------------------
class fee : public ::testing::Test
{
};

// try with blocks ~ 1GB. Passing 2 GB will break on 32 bit systems

TEST_F(fee, 10ryo)
{
	// common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE and lower are clamped
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 100), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, 1), 500000);

	// until 4 x CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE the fee stay constant
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4), clamp_fee(500000));

	// higher is inverse proportional
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 10), clamp_fee(500000 * 4 / 10));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 1000), clamp_fee(500000 * 4 / 1000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(10000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 20000ull), clamp_fee(500000 * 4 / 20000));
}

TEST_F(fee, 1ryo)
{
	// common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE and lower are clamped
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 100), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, 1), 500000);

	// until 4 x CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE the fee stay constant
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4), clamp_fee(500000));

	// higher is inverse proportional
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 10), clamp_fee(500000 * 4 / 10));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 1000), clamp_fee(500000 * 4 / 1000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(1000000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 20000ull), clamp_fee(500000 * 4 / 20000));
}

TEST_F(fee, dot3ryo)
{
	// common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE and lower are clamped
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 100), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, 1), 500000);

	// until 4 x CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE the fee stay constant
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 2), clamp_fee(500000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4), clamp_fee(500000));

	// higher is inverse proportional
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 10), clamp_fee(500000 * 4 / 10));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 1000), clamp_fee(500000 * 4 / 1000));
	ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(300000000, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 20000ull), clamp_fee(500000 * 4 / 20000));
}

TEST_F(fee, double_at_full)
{
	static const uint64_t block_rewards[] = {
		20000000000ull, // 20 ryo
		13000000000ull,
		1000000000ull,
		600000000ull, // .6 ryo, minimum reward per block at 2min
		300000000ull, // .3 ryo, minimum reward per block at 1min
	};
	static const uint64_t increase_factors[] = {
		5,
		10,
		1000,
		// with clamping, the formula does not hold for such large blocks and small fees
		// 20000ull
	};

	for(uint64_t block_reward : block_rewards)
	{
		for(uint64_t factor : increase_factors)
		{
			ASSERT_EQ(Blockchain::get_dynamic_per_kb_fee(block_reward, common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * factor), clamp_fee(500000 * 4 / factor));
		}
	}
}
}
