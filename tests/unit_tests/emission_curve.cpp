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


#include "gtest/gtest.h"
#include "gtest/internal/gtest-internal.h"
#include "unit_tests_utils.h"

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include <fstream>

using namespace cryptonote;

namespace
{

TEST(emission_curve, validate_with_lookup_table)
{
	uint64_t circ_supply = 0;
	uint64_t already_generated_coins = 0;
	uint64_t dev_fund = 0u;
	uint64_t height = 0u;

	std::ifstream file(std::string(unit_test::data_dir.string()) + "/ryo_emission.lookup");
	std::string line;
	getline(file, line);

	while( circ_supply < MONEY_SUPPLY + 500000) //: # loop through block 0 to a few years after tail
	{
		uint64_t dev_block_reward = 0u;
		get_dev_fund_amount<MAINNET>(height, dev_block_reward);

		uint64_t block_reward = 0u;
		bool err = get_block_reward(MAINNET, 0, 0, already_generated_coins, block_reward, height);
		ASSERT_TRUE(err);

		already_generated_coins += block_reward;
		dev_fund += dev_block_reward;

		circ_supply = already_generated_coins + dev_fund - common_config::PREMINE_BURN_AMOUNT;

		if(int(height % (COIN_EMISSION_HEIGHT_INTERVAL/6)) == 0)
		{
			getline(file, line);
			std::vector< std::string > vs;
			testing::internal::SplitString(line, '\t', &vs);
			ASSERT_EQ(std::stoul(vs[0]), height);
			ASSERT_EQ(std::stoul(vs[1]), block_reward);
			ASSERT_EQ(std::stoul(vs[2]), circ_supply);
			ASSERT_EQ(std::stoul(vs[4]), dev_fund);
		}
		height++;
	}
}


// For the test case the blocksize is not taken in account.
bool get_block_reward_old(network_type nettype, uint64_t already_generated_coins, uint64_t &reward, uint64_t height)
{
	// old config values form the file cryptonight_config.h
	constexpr uint64_t PEAK_COIN_EMISSION_YEAR = 4;
	constexpr uint64_t PEAK_COIN_EMISSION_HEIGHT = ((uint64_t)(((12 * 30.4375 * 24 * 3600) / common_config::DIFFICULTY_TARGET) * PEAK_COIN_EMISSION_YEAR)); // = (# of heights emmitted per year) * PEAK_COIN_EMISSION_YEAR

	uint64_t base_reward;
	uint64_t round_factor = 10000000; // 1 * pow(10, 7)
	if(height > 0)
	{
		if(height < (PEAK_COIN_EMISSION_HEIGHT + COIN_EMISSION_HEIGHT_INTERVAL))
		{
			uint64_t interval_num = height / COIN_EMISSION_HEIGHT_INTERVAL;
			double money_supply_pct = 0.1888 + interval_num * (0.023 + interval_num * 0.0032);
			base_reward = ((uint64_t)(MONEY_SUPPLY * money_supply_pct)) >> EMISSION_SPEED_FACTOR;
		}
		else
		{
			base_reward = (MONEY_SUPPLY - already_generated_coins) >> EMISSION_SPEED_FACTOR;
		}
	}
	else
	{
		base_reward = GENESIS_BLOCK_REWARD;
	}

	if(base_reward < FINAL_SUBSIDY)
	{
		if(MONEY_SUPPLY > already_generated_coins)
		{
			base_reward = FINAL_SUBSIDY;
		}
		else
		{
			base_reward = FINAL_SUBSIDY / 2;
		}
	}

	// rounding (floor) base reward
	reward = base_reward / round_factor * round_factor;

	return true;
}

TEST(emission_curve, old_vs_table)
{
	uint64_t already_generated_coins = 0;
	uint64_t dev_fund = 0u;
	uint64_t height = 0u;

	while(height < 394470) // run until the old reward function increases over the new plateau function
	{
		uint64_t dev_block_reward = 0u;
		get_dev_fund_amount<MAINNET>(height, dev_block_reward);

		uint64_t block_reward = 0u;
		bool err = get_block_reward(MAINNET, 0, 0, already_generated_coins, block_reward, height);
		ASSERT_TRUE(err);

		uint64_t block_reward_old = 0u;
		err = get_block_reward_old(MAINNET, already_generated_coins, block_reward_old, height);
		ASSERT_TRUE(err);

		already_generated_coins += block_reward;
		dev_fund += dev_block_reward;
		ASSERT_EQ(block_reward_old, block_reward);

		height++;
	}
}
}
