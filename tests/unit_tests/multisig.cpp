// Copyright (c) 2017-2018, The Monero Project
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

#include "gtest/gtest.h"

#include <cstdint>

#include "common/gulps.hpp"
#include "wallet/wallet2.h"

GULPS_CAT_MAJOR("test");

static const struct
{
	const char *address;
	const char *spendkey;
} test_addresses[] =
	{
		// working with legacy address because new ryo addresses can not be recreated out of the spendkey
		{"RYoTr1KP7tgJFQ4bTWto4MhM3qT4YA8gV3smuJyxucJgBDdCWehV4btgcyNb11MAUC7t9yPDUCwAoREVdyVNankWERkEddsUix3",
			"ee0085dbecc26a02415b0b7abab1ce0ef2b18a393d35e39ef5720dd5ba058806"},
		{"RYoTr2S1o1efcYh16gDXipgq2zypYd6pBJ6iofYemM9P3YUy815iS5ujDfmmNhbWGJPs17nNjxYTNh8LkJdvP7Y4QrD3xC5Xhxy",
			"3fadeb3d58152b98db2e2788bdb57fdc5269e1cd8feb287d915b26a3f8ce610e"},
		{"RYoTqzWU3x38P6uzJpXose4wxRx53RZXc6wipiTFqnuADaZdNYLcV8z9XSSsdcVQjKgkumTat2nJAZ1TYNL6w4bTQqm4apJepaw",
			"2a69fc7813c5b9af92f133d841c0859571956c2e544b5eab58d97c9df59ff000"}};

static void make_wallet(unsigned int idx, tools::wallet2 &wallet)
{
	ASSERT_TRUE(idx < sizeof(test_addresses) / sizeof(test_addresses[0]));

	crypto::secret_key spendkey;
	epee::string_tools::hex_to_pod(test_addresses[idx].spendkey, spendkey);

	try
	{
		wallet.init("");
		wallet.set_subaddress_lookahead(1, 1);
		wallet.generate_legacy("", "", spendkey);
		ASSERT_TRUE(test_addresses[idx].address == wallet.get_account().get_public_address_str(cryptonote::TESTNET));
	}
	catch(const std::exception &e)
	{
		GULPS_ERROR("Error creating test wallet: ", e.what());
		ASSERT_TRUE(0);
	}
}

static void make_M_2_wallet(tools::wallet2 &wallet0, tools::wallet2 &wallet1, unsigned int M)
{
	ASSERT_TRUE(M <= 2);

	make_wallet(0, wallet0);
	make_wallet(1, wallet1);

	std::vector<crypto::secret_key> sk0(1), sk1(1);
	std::vector<crypto::public_key> pk0(1), pk1(1);

	std::string mi0 = wallet0.get_multisig_info();
	std::string mi1 = wallet1.get_multisig_info();

	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi1, sk0[0], pk0[0]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi0, sk1[0], pk1[0]));

	ASSERT_FALSE(wallet0.multisig() || wallet1.multisig());
	wallet0.make_multisig("", sk0, pk0, M);
	wallet1.make_multisig("", sk1, pk1, M);

	ASSERT_TRUE(wallet0.get_account().get_public_address_str(cryptonote::TESTNET) == wallet1.get_account().get_public_address_str(cryptonote::TESTNET));

	bool ready;
	uint32_t threshold, total;
	ASSERT_TRUE(wallet0.multisig(&ready, &threshold, &total));
	ASSERT_TRUE(ready);
	ASSERT_TRUE(threshold == M);
	ASSERT_TRUE(total == 2);
	ASSERT_TRUE(wallet1.multisig(&ready, &threshold, &total));
	ASSERT_TRUE(ready);
	ASSERT_TRUE(threshold == M);
	ASSERT_TRUE(total == 2);
}

static void make_M_3_wallet(tools::wallet2 &wallet0, tools::wallet2 &wallet1, tools::wallet2 &wallet2, unsigned int M)
{
	ASSERT_TRUE(M <= 3);

	make_wallet(0, wallet0);
	make_wallet(1, wallet1);
	make_wallet(2, wallet2);

	std::vector<crypto::secret_key> sk0(2), sk1(2), sk2(2);
	std::vector<crypto::public_key> pk0(2), pk1(2), pk2(2);

	std::string mi0 = wallet0.get_multisig_info();
	std::string mi1 = wallet1.get_multisig_info();
	std::string mi2 = wallet2.get_multisig_info();

	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi1, sk0[0], pk0[0]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi2, sk0[1], pk0[1]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi0, sk1[0], pk1[0]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi2, sk1[1], pk1[1]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi0, sk2[0], pk2[0]));
	ASSERT_TRUE(tools::wallet2::verify_multisig_info(mi1, sk2[1], pk2[1]));

	ASSERT_FALSE(wallet0.multisig() || wallet1.multisig() || wallet2.multisig());
	std::string mxi0 = wallet0.make_multisig("", sk0, pk0, M);
	std::string mxi1 = wallet1.make_multisig("", sk1, pk1, M);
	std::string mxi2 = wallet2.make_multisig("", sk2, pk2, M);

	const size_t nset = !mxi0.empty() + !mxi1.empty() + !mxi2.empty();
	ASSERT_TRUE((M < 3 && nset == 3) || (M == 3 && nset == 0));

	if(nset > 0)
	{
		std::unordered_set<crypto::public_key> pkeys;
		std::vector<crypto::public_key> signers(3, crypto::null_pkey);
		ASSERT_TRUE(tools::wallet2::verify_extra_multisig_info(mxi0, pkeys, signers[0]));
		ASSERT_TRUE(tools::wallet2::verify_extra_multisig_info(mxi1, pkeys, signers[1]));
		ASSERT_TRUE(tools::wallet2::verify_extra_multisig_info(mxi2, pkeys, signers[2]));
		ASSERT_TRUE(pkeys.size() == 3);
		ASSERT_TRUE(wallet0.finalize_multisig("", pkeys, signers));
		ASSERT_TRUE(wallet1.finalize_multisig("", pkeys, signers));
		ASSERT_TRUE(wallet2.finalize_multisig("", pkeys, signers));
	}

	ASSERT_TRUE(wallet0.get_account().get_public_address_str(cryptonote::TESTNET) == wallet1.get_account().get_public_address_str(cryptonote::TESTNET));
	ASSERT_TRUE(wallet0.get_account().get_public_address_str(cryptonote::TESTNET) == wallet2.get_account().get_public_address_str(cryptonote::TESTNET));

	bool ready;
	uint32_t threshold, total;
	ASSERT_TRUE(wallet0.multisig(&ready, &threshold, &total));
	ASSERT_TRUE(ready);
	ASSERT_TRUE(threshold == M);
	ASSERT_TRUE(total == 3);
	ASSERT_TRUE(wallet1.multisig(&ready, &threshold, &total));
	ASSERT_TRUE(ready);
	ASSERT_TRUE(threshold == M);
	ASSERT_TRUE(total == 3);
	ASSERT_TRUE(wallet2.multisig(&ready, &threshold, &total));
	ASSERT_TRUE(ready);
	ASSERT_TRUE(threshold == M);
	ASSERT_TRUE(total == 3);
}

TEST(multisig, make_2_2)
{
	tools::wallet2 wallet0, wallet1;
	make_M_2_wallet(wallet0, wallet1, 2);
}

TEST(multisig, make_3_3)
{
	tools::wallet2 wallet0, wallet1, wallet2;
	make_M_3_wallet(wallet0, wallet1, wallet2, 3);
}

TEST(multisig, make_2_3)
{
	tools::wallet2 wallet0, wallet1, wallet2;
	make_M_3_wallet(wallet0, wallet1, wallet2, 2);
}
