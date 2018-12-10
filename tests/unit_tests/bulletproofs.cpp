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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "device/device.hpp"
#include "misc_log_ex.h"
#include "ringct/bulletproofs.h"
#include "ringct/rctOps.h"
#include "ringct/rctSigs.h"
#include "string_tools.h"
#include "crypto/crypto-ops.h"

TEST(bulletproofs, valid_zero)
{
	rct::Bulletproof proof = bulletproof_PROVE(0, rct::skGen());
	ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_max)
{
	rct::Bulletproof proof = bulletproof_PROVE(0xffffffffffffffff, rct::skGen());
	ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
}

TEST(bulletproofs, valid_random)
{
	for(int n = 0; n < 8; ++n)
	{
		rct::Bulletproof proof = bulletproof_PROVE(crypto::rand<uint64_t>(), rct::skGen());
		ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
	}
}

TEST(bulletproofs, valid_multi_random)
{
	for(int n = 0; n < 8; ++n)
	{
		size_t outputs = 2 + n;
		std::vector<uint64_t> amounts;
		rct::keyV gamma;
		for(size_t i = 0; i < outputs; ++i)
		{
			amounts.push_back(crypto::rand<uint64_t>());
			gamma.push_back(rct::skGen());
		}
		rct::Bulletproof proof = bulletproof_PROVE(amounts, gamma);
		ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
	}
}

TEST(bulletproofs, multi_splitting)
{
	rct::ctkeyV sc, pc;
	rct::ctkey sctmp, pctmp;
	std::vector<unsigned int> index;
	std::vector<uint64_t> inamounts, outamounts;

	std::tie(sctmp, pctmp) = rct::ctskpkGen(6000);
	sc.push_back(sctmp);
	pc.push_back(pctmp);
	inamounts.push_back(6000);
	index.push_back(1);

	std::tie(sctmp, pctmp) = rct::ctskpkGen(7000);
	sc.push_back(sctmp);
	pc.push_back(pctmp);
	inamounts.push_back(7000);
	index.push_back(1);

	const int mixin = 3, max_outputs = 16;

	for(int n_outputs = 1; n_outputs <= max_outputs; ++n_outputs)
	{
		std::vector<uint64_t> outamounts;
		rct::keyV amount_keys;
		rct::keyV destinations;
		rct::key Sk, Pk;
		uint64_t available = 6000 + 7000;
		uint64_t amount;
		rct::ctkeyM mixRing(sc.size());

		//add output
		for(size_t i = 0; i < n_outputs; ++i)
		{
			amount = rct::randRyoAmount(available);
			outamounts.push_back(amount);
			amount_keys.push_back(rct::hash_to_scalar(rct::zero()));
			rct::skpkGen(Sk, Pk);
			destinations.push_back(Pk);
			available -= amount;
		}

		for(size_t i = 0; i < sc.size(); ++i)
		{
			for(size_t j = 0; j <= mixin; ++j)
			{
				if(j == 1)
					mixRing[i].push_back(pc[i]);
				else
					mixRing[i].push_back({rct::scalarmultBase(rct::skGen()), rct::scalarmultBase(rct::skGen())});
			}
		}

		rct::ctkeyV outSk;
		rct::rctSig s = rct::genRctSimple(rct::zero(), sc, destinations, inamounts, outamounts, available, mixRing, amount_keys, NULL, NULL, index, outSk, true, hw::get_device("default"));
		ASSERT_TRUE(rct::verRctSemanticsSimple(s) && rct::verRctNonSemanticsSimple(s));
		for(size_t i = 0; i < n_outputs; ++i)
		{
			rct::key mask;
			rct::decodeRctSimple(s, amount_keys[i], i, mask, hw::get_device("default"));
			ASSERT_TRUE(mask == outSk[i].mask);
		}
	}
}

TEST(bulletproofs, valid_aggregated)
{
	static const size_t N_PROOFS = 8;
	std::vector<rct::Bulletproof> proofs(N_PROOFS);
	for(size_t n = 0; n < N_PROOFS; ++n)
	{
		size_t outputs = 2 + n;
		std::vector<uint64_t> amounts;
		rct::keyV gamma;
		for(size_t i = 0; i < outputs; ++i)
		{
			amounts.push_back(crypto::rand<uint64_t>());
			gamma.push_back(rct::skGen());
		}
		proofs[n] = bulletproof_PROVE(amounts, gamma);
	}
	ASSERT_TRUE(rct::bulletproof_VERIFY(proofs));
}

#define DO_TEST

class bp_blobs : public ::testing::Test 
{
public:
	rct::Bulletproof proof;
	cryptonote::blobdata main_blob;
	cryptonote::blobdata commit_blob;
	
	void PRINT_BULLETPROOF()
	{
		std::ostringstream s;
		binary_archive<true> a(s);
		::serialization::serialize(a, proof);
		std::cout << ::testing::UnitTest::GetInstance()->current_test_info()->name() << std::endl << 
			"blob" << std::endl <<
			epee::string_tools::buff_to_hex_nodelimer(s.str()) << std::endl << 
			"commit" << std::endl <<
			epee::string_tools::pod_to_hex(proof.V[0]) << std::endl;
	}
	
	void VERIFY_BULLETPROOF(bool pass)
	{
		std::stringstream ss;
		ss << main_blob;
		binary_archive<false> ba(ss);

		ASSERT_TRUE(::serialization::serialize(ba, proof));
		rct::key V;
		ASSERT_TRUE(commit_blob.size() == 32);
		memcpy(&V, &commit_blob.front(), 32);
		proof.V.push_back(V);
		ASSERT_EQ(rct::bulletproof_VERIFY(proof), pass);
	}
};

TEST_F(bp_blobs, valid_blob)
{
#ifndef DO_TEST
	proof = bulletproof_PROVE(0xcafebabe, rct::skGen());
	PRINT_BULLETPROOF();
#else
	main_blob = cryptonote::blobdata(
		"\x49\xcf\x98\xd8\x21\xb2\x57\x66\x14\xc2\x01\x8f\xb0\x35\x38\xcc\x80\xef\xd4\xb6\x31\x5d\xa9\x1e\xcb\xef\x13\x50\x4c\x43\x6e\xf6"
		"\x63\x44\x24\x09\xff\xb9\xb2\xf3\x12\x56\x90\xa8\x7a\x9d\xbb\x71\xa2\x81\x7e\x94\x06\xcf\xab\x91\xbe\x9b\x7f\x15\x9a\x13\xdf\xb1"
		"\x28\x54\x2f\xea\x6e\x74\x7b\x15\x6e\x21\xcb\x7a\xf4\x53\x38\xd6\xd6\xf8\xe4\x26\x23\x14\x21\x25\x32\x63\x28\xf1\x22\xc3\xed\x79"
		"\x4b\x23\x76\x3b\xd5\xf4\x48\xfd\xad\xb3\x4b\x6b\x9f\x73\x9f\x1f\x2d\x76\xd2\xcf\x88\x92\x6a\x53\x02\xe0\xc7\x20\x9e\x4a\x27\xf4"
		"\x54\x63\x23\x5e\xc0\x16\x1d\x9d\x6b\x8e\x9b\xe0\xb6\xf6\x22\x17\x30\x30\x2f\x97\x08\xde\xb0\xe1\x7b\x7c\x68\xd2\xc4\xde\x5c\x09"
		"\x02\x8d\xf5\xc7\xd0\xae\xb9\x6d\x03\x69\x12\x7b\x55\xef\x72\xd9\x7f\xe7\x9d\xb3\x32\x2c\x52\x19\xe6\x97\x31\x26\x15\xae\xac\x08"
		"\x06\x3f\xc7\x4e\xef\xed\x3c\x8e\x99\x30\x06\x8e\x30\xfd\x01\xc3\xcd\x07\xcc\x64\x14\x03\x46\x41\x3d\xba\x85\x45\x9f\x25\x4f\xf4"
		"\xdc\xb6\x3f\xbc\xea\x3e\xcf\x9f\xd6\xf9\x4a\xc8\x01\x42\x7b\xcd\xfa\xc1\x3e\x8d\x60\x22\x70\x7a\x0d\x60\x08\x1c\x6c\x05\x67\x40"
		"\xec\x0b\x76\xba\x34\x59\x8b\x05\x44\xc1\x7c\x63\x7c\xcc\x4b\x42\x66\x93\xac\x96\x22\x1d\xd1\x90\x42\xe5\xe7\xca\xf2\xf1\x0f\xab"
		"\x07\x45\x07\x4f\xe2\x4f\xaf\xed\x5f\xdb\x0e\xa7\xe9\x4a\xbe\xb0\x23\x77\x0d\x79\xca\xff\x7a\x7c\xcb\x16\x32\xda\x29\xce\x6f\x5e"
		"\xaa\xf1\xeb\x15\xfc\x14\xf0\x7a\x03\xee\x3a\x28\x64\x89\x5b\xdf\x75\x9b\xab\xf7\x99\xcd\x81\x82\x13\x14\x4a\x75\xa0\xc2\x61\xb3"
		"\x0e\xcf\x85\x4b\xe6\x87\x14\x19\x16\x5a\x0c\xcd\x68\xb4\xe3\xe2\x38\xd4\x21\xbe\x98\x36\x1f\x32\xd5\x0d\x09\xc8\x80\x99\xdd\xf9"
		"\x36\x06\x59\xc9\xd7\xbb\xe8\xc3\xfb\xf5\x2b\x7f\xdb\x7f\xb9\x2d\xa6\x38\x10\x92\xd8\x57\x1f\x70\xcf\x73\x32\x7a\xe7\x31\x2a\x4d"
		"\x10\x5e\x82\x4b\x8e\x9e\xc5\x91\xcf\xbb\x02\xfb\x22\x10\x7c\x59\x65\x9e\x51\xd1\xa7\xf9\x3e\xa0\xe7\x7c\xe1\x1b\x83\xd7\xb5\x9d"
		"\x4d\x52\x9d\x68\x45\x0f\xdd\x02\xcc\x5d\xe0\xad\xbd\x7d\x05\x4d\xd5\x4b\x3b\x08\x9e\xdd\x73\x31\x7c\x34\xee\xbc\x3b\x66\x84\x01"
		"\xcd\x61\x52\x80\x7b\x9b\x6b\x2a\x17\x17\xa9\x72\xdf\x4a\xe0\x92\x84\xc1\x53\xe3\x9c\x01\xe1\x74\x9a\x55\xd0\xa3\x8a\xb3\x16\xad"
		"\x9c\x47\x55\x8e\xbf\x24\x48\xba\xd1\xec\x21\x8d\x70\x3b\xa2\x4e\xc4\x34\xeb\xae\x6c\x3a\x1f\x90\x07\xe5\xc5\x76\xc6\x15\x8e\xc9"
		"\x61\xa4\x61\x49\xbc\x76\x26\x66\x62\x41\x0c\x73\xfe\xb4\x65\x44\xde\x39\x44\x2e\xc0\x61\x00\xff\x33\x52\x22\x6f\x2b\x31\x71\xd7"
		"\x02\x49\xd3\x1a\xbf\x35\x39\xfa\x25\x94\x89\xd9\x21\x9c\x3b\x11\x98\xc4\x30\xa5\xb4\x02\x58\x88\x82\x6a\x5c\x8a\x0c\x52\x6a\x92"
		"\xb2\x0a\x96\x94\x83\x1e\xca\x4e\x07\x43\x5f\xd6\x9d\x8c\xa5\x8b\x1b\x58\xd7\xac\xd9\x43\xc6\x27\x00\x82\xae\xed\x54\xcf\xa2\x56"
		"\x1d\x0c\x47\x57\x3b\x6a\xf5\xae\x21\x04\xd3\x6c\x3a\x28\xfc\x57\x14\x89\xb0\x43\xc6\x27\x61\x25\x0b\x8d\x3a\xa5\x26\x0d\x1b\x8d"
		"\x06\x02", 674);
	
	commit_blob = cryptonote::blobdata(
		"\x6b\xda\x3a\xd5\x9f\xcc\x01\xab\x32\x52\x8d\x49\x81\x12\xc0\xa8\xc3\xa4\xe7\x40\x83\xb8\x43\x87\x42\xcf\xda\xd4\x6b\x56\x5f\x68", 32);
#endif
	VERIFY_BULLETPROOF(true);
}

TEST_F(bp_blobs, invalid_8)
{
#ifndef DO_TEST
	rct::key invalid_amount = rct::zero();
	invalid_amount[8] = 1;

	proof = bulletproof_PROVE(invalid_amount, rct::skGen());
	PRINT_BULLETPROOF();
#else
	main_blob = cryptonote::blobdata(
		"\xdd\x95\x08\x3d\x50\x67\xdb\x7e\xba\x03\xe2\x6d\x30\xc9\xc7\x73\x4e\x28\x1b\xb4\x66\x22\x47\xd9\x24\x6b\xfe\x0e\x55\xe9\x53\x30"
		"\x03\xeb\x98\x14\x04\x7e\xd7\xe3\x0c\x54\x25\x7d\x28\x11\xc2\x29\x4f\x1a\xc5\x5d\xdb\xae\xe3\x0a\x95\xd6\x51\x41\xa7\xfc\x9c\xb4"
		"\x87\x0e\xa1\x60\xc6\x29\xfc\x53\x5b\xf1\x15\x4d\x35\x35\xbf\x64\xac\xfe\x2c\x58\xd8\xa5\x4b\x9d\xe5\xf7\x41\x76\x18\x35\x3e\x9f"
		"\x07\x94\x6d\x78\x23\xbc\x97\xd2\x97\x46\xa1\x6d\x66\x5f\xa6\x1f\xaf\x0e\x42\xbf\xbd\x4c\xae\x32\x0b\x85\x95\xfa\xa1\xcb\x64\xc1"
		"\xdd\x7e\xde\xa1\x5e\xb3\xe5\x40\xda\x6d\xa6\x7b\x99\x70\x7d\xfd\x6c\x89\x86\x1d\x3c\x7a\x92\xea\x10\x1c\xbf\xcd\x9e\xd9\x64\x04"
		"\x5c\x05\x68\xe3\x73\x04\xfd\xc7\xb9\x34\x53\x62\xe3\x15\x31\x54\xd2\x56\x55\xee\x76\x02\x23\x99\x6a\x5b\xb2\x38\x1c\x72\xa4\x0d"
		"\x06\x03\xe8\xac\xb2\xb0\xbe\xd1\x05\xdf\x4c\x83\xeb\x6d\x59\xbb\x3c\x8d\x2c\x13\x8e\x6f\x6d\xaf\xc5\x5d\x82\x3c\x44\x0a\x4e\x6b"
		"\x45\x22\x93\x9b\x98\x6e\x33\xa9\x7c\x08\xad\x98\x4d\xc1\x1f\x35\x16\x47\x04\x82\xa8\xd3\x74\xb4\x85\x73\x57\x00\xba\xb1\x0b\x43"
		"\x8f\x35\xac\xf2\xbc\x73\xa4\x42\x88\x04\x9a\x1f\xcf\x8d\x4a\x85\x90\x1d\x77\x05\x50\x3c\x26\x9c\xe9\x00\xb1\x33\xd8\x54\x62\xec"
		"\xfa\x7f\x6c\xb0\x46\xf4\x2d\x35\x95\x57\x2e\xa8\x4a\xbd\x04\x16\xde\x0e\x88\x8d\x37\xea\x7a\x77\x90\x1b\x10\x21\xe7\x8c\x07\xa3"
		"\x2f\x1e\x68\xd3\xdf\xad\x10\xdf\xc4\x85\x5a\xa9\x26\x59\xb4\x01\x88\xc0\x8b\x92\x56\xed\xf5\xac\x36\x13\x9a\x5d\x3c\xf8\xdb\x03"
		"\xe8\x76\x83\x7d\x7a\x60\xb6\x09\x4b\x52\x34\x31\xb6\x0c\x90\x12\x68\xa6\x17\x70\xd0\x37\xf2\xc1\x2a\x4f\xc3\xa7\xde\x6a\x96\x8f"
		"\xea\x06\xb3\xaa\x25\xfa\x11\x69\x63\x57\xfc\x11\x52\xd1\x53\x7d\x6d\x74\x64\xf1\x32\x95\x8a\xe0\xea\x42\x4f\xd3\xb3\x0c\xf6\xc8"
		"\xb9\xbe\x04\xac\x1f\xd7\xae\x36\x9f\xad\xc7\x65\xf7\xc0\x08\x0c\x59\x46\x99\xbd\x81\xbc\xee\x8a\xcc\x5f\x63\xc0\x04\x4b\x50\xc2"
		"\x65\x32\xa0\xc5\xfd\x5a\xfd\x76\xc7\xc4\x2c\x37\xe4\x2a\xc1\x37\x71\x13\xd3\x4f\x0f\xf0\x01\xd2\xc7\x6d\x33\x55\x51\x2e\x8d\x59"
		"\x10\xbd\x4a\xf6\x93\x30\xaf\x9e\xf5\xa4\x6b\xa1\x1b\x22\xfe\x27\x5c\xb1\xfd\xb9\x91\xcc\x26\xc0\x39\xfe\x43\xa2\x61\xbb\x32\x98"
		"\xfb\xb0\x4a\xe5\x88\x41\x5b\xa9\x9e\x26\x38\x97\x90\x7b\x1d\x96\x6d\x77\xaf\x7e\xa3\x9d\xe5\x5e\x9c\x99\x2b\x28\xf6\x2a\x51\x27"
		"\x4b\xd1\x68\x60\x46\x9f\x23\x24\x51\x84\x23\x9b\xf7\x81\x0a\xaf\x22\x14\x8c\x62\x1c\xb8\x4c\x56\x34\xb4\x4b\x40\xad\x01\x63\x57"
		"\x2b\x99\x03\xd7\x65\x12\x0f\x3d\xda\x02\x7a\xed\x16\xb0\x1f\x4b\xe2\x26\xb8\x90\xd0\xb4\xaf\xdd\x16\xd9\xcc\x11\x6d\x5c\x66\xfb"
		"\x1a\x04\xbc\x3f\xc2\x61\x14\xcf\xb9\x5d\x9c\xff\x5f\xee\xe2\xb1\x9b\x7e\x75\xe4\xe5\x17\xa5\x83\xe6\x6c\xd5\x88\x28\xbb\xde\xee"
		"\x9b\x00\x1c\xaa\x89\xda\x8e\xa5\x70\xf7\xe6\x21\x1f\xc7\x95\x15\x48\x96\x97\x50\xf9\x90\xdf\x8c\x25\x77\xdb\xb3\x63\x35\xea\xcc"
		"\x25\x0d", 674);
		
	commit_blob = cryptonote::blobdata(
		"\x47\xba\x8e\x0e\x7b\xf7\xe3\x39\x38\x16\x0b\x27\x44\x70\x72\x8a\x44\x81\x50\x78\x3e\xd7\x57\x19\xbd\x30\x92\x35\x46\x46\x35\x5c", 32);
#endif
	VERIFY_BULLETPROOF(false);
}

TEST_F(bp_blobs, invalid_31)
{
#ifndef DO_TEST
	rct::key invalid_amount = rct::zero();
	invalid_amount[31] = 1;

	proof = bulletproof_PROVE(invalid_amount, rct::skGen());
	PRINT_BULLETPROOF();
#else
	main_blob = cryptonote::blobdata(
		"\xaf\x4a\xc4\xa4\x5b\xf5\x53\xd6\x0b\x7f\xc1\x89\xad\xa2\xb9\x1d\x17\xb7\x25\xc4\x69\x4d\x83\xa3\x36\x84\xdc\x3a\xe8\xd9\xdc\x7a"
		"\x2c\x17\xca\x02\x71\x6d\x98\x71\x6c\x1a\x04\xf8\xf2\x8f\x6d\x1d\x62\xac\xfe\x6b\xbf\xa2\x6c\x7e\xb6\x8c\x2c\xdf\x18\x19\x30\x0d"
		"\xa2\x7e\x55\x86\x36\x48\xed\x5d\x4d\xd5\xf7\x93\xd5\x8e\x76\xd3\x0f\xff\x8a\x78\x19\x9e\x53\x47\x87\xe4\x2d\x94\x93\x43\x8d\x8b"
		"\x4f\xa8\xc2\xec\xaa\xa4\x04\xea\xc3\xa0\xcc\xd7\x20\x98\x07\xc9\x45\xa9\x68\x13\xb0\x92\xdc\x12\x72\xac\x43\xc2\xd1\x6e\x27\x14"
		"\x6c\xa8\x0a\x8c\x92\x79\xc5\x98\x28\x4d\x50\xe4\xe4\xb7\x76\x99\xc0\x5b\x6e\x19\x4e\x71\xfd\xf8\x23\x39\x26\x4b\x52\x4c\x40\x04"
		"\x59\xef\xa1\xc5\x79\x41\x61\xf0\xc1\x12\x20\x20\xe5\x3a\x2a\xd2\x3c\xe5\x0e\x0f\x4a\xeb\xce\x87\xc7\x50\xaa\xb3\xf3\xba\x4d\x0e"
		"\x06\x9e\xae\xb5\xfb\x27\x12\x83\x79\xa5\x7d\xab\xe0\xed\xc4\x40\x52\x60\x75\xac\x9b\xd6\x2c\xb2\x4f\x52\x43\x28\xad\x83\x4d\xb8"
		"\x06\xd9\xf9\x56\xea\xdd\xaf\xd6\x9a\x94\xfc\x1a\x4e\xf7\x15\x34\x4d\xe9\x18\x25\x5d\x20\x10\xf9\x00\x0e\x7d\x26\xf5\xd6\x60\xf2"
		"\x00\xc8\x4c\x60\x36\x4e\x8d\x8e\xb0\x21\xc6\x21\xc0\xdf\x4e\xe7\xf1\x91\xe0\x52\x91\xe3\x3f\xa6\x4d\x89\x46\x92\x3f\xec\xdf\xe5"
		"\x1e\x5c\x7e\x55\x07\xd2\x44\xee\x41\xbc\x41\xb5\x8a\x7f\xa1\x76\xde\xc8\x7c\xe2\xff\x97\x4e\xf1\xaa\x45\x97\xe2\x48\x45\xe3\xe6"
		"\x5c\x50\xd7\x5b\x3d\x67\xde\x9f\xb0\x2c\xd7\x55\x69\x59\x48\x99\xc2\x0e\x6b\x21\x44\x41\xb7\x5c\x68\x58\x5e\xd8\xbb\x88\xb0\x32"
		"\x00\x94\x4d\xa8\xcd\x68\x26\x51\xcf\x39\x95\xe9\x42\x12\x8b\x0c\xad\x30\x31\xa6\xe2\xd5\x72\x9b\x6f\x24\x11\xe5\xcb\x7c\x20\x22"
		"\x75\x06\x6f\x39\xb8\xf9\x41\x6c\x47\xf4\xbc\x0f\x70\x29\x38\x39\x20\x51\x93\xe0\x7d\x61\xa8\xa3\x15\xce\x4d\xb2\x60\xda\x65\x67"
		"\xd6\xcc\x7f\x1d\xc7\x27\x60\x5a\x65\x37\x60\xd6\x08\x41\x34\x2a\x7e\x8b\x38\x5f\x9c\x0d\xb8\x27\xb1\xc9\x40\xaa\xb0\x9c\xe4\xd1"
		"\x14\x31\xab\x17\x3f\xc3\x06\x79\x92\xb9\xb9\x1d\xc9\x6a\x5f\x02\x3c\x28\xda\x09\x75\x94\x03\x6f\xec\x78\x9c\x57\xb4\x10\xaf\x91"
		"\x5f\xee\xe3\xfe\x57\x8d\xb2\xbe\x96\x91\x64\xe7\x8e\x36\x05\x24\x54\x9a\x4a\xf3\xbd\xdd\x1f\xa3\xae\xae\xbe\x20\x63\x49\x86\x30"
		"\x46\xc0\x8c\xa3\xfe\x39\x7a\xd8\x2e\xc2\xe2\x2b\xb6\xd4\xad\x3b\xfe\x46\xf6\x3c\x79\x37\x02\x83\xd3\x4e\x17\x9d\xab\x6b\x09\x87"
		"\xad\x81\x09\x9d\xbc\x62\xf4\xd3\xce\x25\x4f\x03\x85\x5c\x25\x0b\x0b\x4a\x15\xf9\x28\x2f\x32\x37\x45\x42\x08\xad\x93\x90\xb1\xf5"
		"\x70\x14\x5a\x31\x30\xdb\x0c\x00\xc9\x3a\x49\xf4\x03\xba\xfe\x3d\xd5\x62\x24\x85\x24\x07\x5f\x5d\x06\x4a\x21\x46\x47\x7c\x5a\x71"
		"\xd3\x0f\xe9\x27\xa3\xa8\xce\xa3\x7a\xd0\xe1\x13\x6f\x66\x2b\x2b\x85\xa4\x50\x2f\x28\x3f\xab\x01\x7c\xc1\x19\x4a\xa5\x45\xe1\x9b"
		"\x8e\x0b\xe0\x68\x3a\x0d\x98\x71\x0f\xf5\x62\x51\x71\x40\x2d\xe0\x9c\xaf\x95\x80\x55\x06\x01\xef\xcf\x3d\x26\x32\xf6\x13\x0c\xa7"
		"\xfe\x03", 674);
	
	commit_blob = cryptonote::blobdata(
		"\x3b\x7d\x17\x0e\x49\x8a\xca\x83\xa3\xfa\xab\xa8\x53\xdb\x21\x63\xc7\x6f\x06\xc0\xd5\xb9\xf7\x1a\xe7\x87\x4b\x6f\x18\x67\xb3\xef", 32);
#endif
	VERIFY_BULLETPROOF(false);
}

TEST_F(bp_blobs, invalid_gamma_0)
{
#ifndef DO_TEST
	rct::key invalid_amount = rct::zero();
	invalid_amount[8] = 1;
	rct::key gamma = rct::zero();

	proof = bulletproof_PROVE(invalid_amount, gamma);
	PRINT_BULLETPROOF();
#else
	main_blob = cryptonote::blobdata(
		"\x3f\x42\x19\x1d\xc0\x90\x7d\xfb\x4e\xc4\x0f\xde\xb8\x3f\xba\xd0\x07\xa4\x99\xdb\x33\x72\xce\xe4\x9f\x96\x4d\x65\x8d\x78\x37\x97"
		"\xd5\x47\xdd\x00\x51\x8b\xe3\x5e\x8e\x4f\xa0\x6d\x10\x72\x02\x32\x12\xa7\x20\xe0\xb1\x33\x10\xf5\xb2\xa3\xcf\xbb\xf1\xd1\x23\x76"
		"\x69\xbe\xe1\x3c\xa5\xd0\x70\xe7\x02\x1b\x2b\xe1\x88\x75\x35\xcc\x37\x8a\xa4\x07\x4e\x6f\x73\xe6\x70\x3a\x61\x96\x56\x0a\xc8\x5b"
		"\x61\x01\x19\x22\x77\x45\x67\x5c\xbb\x15\x77\x31\x2f\xab\x3d\x00\xf3\x02\x7b\x81\xcb\xa2\x37\xbf\xd6\x63\x14\x6f\x4b\x18\x58\x41"
		"\xc9\x81\xb1\xf5\xfe\xd2\x65\xcb\x6d\x99\x83\x68\x9e\x91\xf1\x35\xc2\xf6\xba\x1f\x01\x02\x84\x89\x68\xf4\x5b\x68\x19\x1b\xc9\x02"
		"\x14\x2c\x70\x6c\xf4\x9e\xf4\x28\xc5\x15\xb7\x51\x01\x37\xe5\xaf\xa5\xfd\x69\x63\xcc\x32\x45\xdb\xc1\x10\xe3\xce\x79\xc7\xd6\x01"
		"\x06\x84\x55\x1c\xe8\xc8\xbd\x1c\x80\xd5\xce\xe9\xb5\xd4\x37\x6a\x09\xb2\xe3\x0c\x1b\xef\xa6\x5a\xe2\x5c\x65\x26\x4d\xe4\xe2\x62"
		"\x7a\xae\xbd\xff\x82\xe6\x07\x76\xc4\x73\xeb\x0a\x94\xcf\xe4\x6a\xa5\x86\xa7\xfa\x8d\x5b\x88\x52\x6a\x54\xea\x13\xc7\xc3\x21\x55"
		"\x32\xd9\xa9\xac\xed\x3d\x0a\x43\x2c\x20\xd3\x87\x3c\xe0\x03\x14\x32\x7b\x8b\xc5\x6a\xbe\xb8\xfb\xf3\xe8\xda\xe7\x4f\x87\x8e\xff"
		"\x84\x6e\x03\xfe\xc0\x78\x5e\xe5\xb1\xad\x98\xde\xd2\xe5\x8f\xc6\x71\xc2\xe7\x20\xa8\xa1\x51\x49\xd8\x4f\x11\x5b\x21\x2d\x6c\x7b"
		"\x0e\x08\x9e\xcf\xfc\xfc\x9d\x5e\xfa\xee\xa1\x69\x65\x54\xa6\x55\xae\xcf\xa6\x75\x03\x2e\x2d\x45\x81\x72\x84\x39\xc5\xda\x0c\x8c"
		"\x94\xe5\x49\x64\x1f\x6e\x55\x50\x4c\x81\x7a\x28\xaf\x8e\x5f\x1e\x64\xe6\x63\x15\x9c\x07\x46\x6e\xd0\x50\x8d\x91\x8c\x15\x16\x3f"
		"\x7d\x06\x97\x2a\x57\x97\xdd\xe5\x00\xb9\xcb\x0c\xc5\x79\x2f\xa7\xe9\x32\x7c\x94\x99\x19\x7e\x48\x24\x10\xa9\x40\xa0\x02\x10\x9a"
		"\x95\x1f\x66\x83\x21\xb1\xee\x05\x17\x8d\x18\x13\xc0\x6f\x9b\x8e\x99\xab\x86\x23\x4b\x8a\x92\xd5\xba\x8d\x22\x7a\xc8\x0d\xad\x07"
		"\x02\xea\x55\x67\x2b\x44\x1a\xe8\x62\x69\xcd\xde\xb3\xd4\x7e\xc7\xfc\x8f\xd0\x89\x09\x4f\x0b\x06\xe4\x97\xc5\xa4\x2c\x8b\x61\x5c"
		"\x95\xff\xbe\x0a\x62\x24\x55\x9b\x43\xae\x7d\xb6\x39\xb4\xda\x84\x51\x72\x1e\x4a\x04\xe9\x0b\x8d\x4e\x2a\x67\xd8\xfa\xa1\xa5\x84"
		"\x1d\xd2\xb1\x1b\x7c\xcc\x09\x0b\x1d\x57\x78\x69\x54\x3c\xaf\x2b\x0b\x0b\x1b\x20\xf8\xd9\xe4\x22\x35\xea\x68\xa0\x28\xd1\xd5\x67"
		"\x0f\xe9\x6d\x33\x75\x2c\x51\xe7\x37\x60\x31\x55\xd5\x37\x48\xba\xfd\x48\x59\xa8\x9c\xc0\x4e\x69\x0c\x20\xae\x45\xc7\x7a\x84\x98"
		"\xe3\x8c\x79\xa7\x1c\xa4\xe7\x4d\x56\xa3\xe5\x60\xb0\xae\xa1\xe2\xdd\x50\x20\xeb\x57\x84\x20\x34\x94\x43\xce\xc5\xea\x98\xf2\x8f"
		"\xc5\x06\x98\x96\xa9\x94\x49\xa9\x65\x7e\xa4\xf8\xb2\x7c\xe3\xce\xbe\x19\xae\x65\x84\x33\x5e\x96\x12\xf0\xf4\x17\xa9\x89\x85\xc8"
		"\xfc\x06\x2b\x35\x4b\x66\x3c\xfb\x69\x12\xdc\x3a\x21\x7a\xa7\xae\x27\xc5\x94\xb1\x74\xb1\x8c\xee\xfb\x16\x9b\x65\x1c\xe9\xc5\x9c"
		"\xd2\x0c", 674);
	
	commit_blob = cryptonote::blobdata(
		"\xe5\xb5\x2b\xc9\x27\x46\x8d\xf7\x18\x93\xeb\x81\x97\xef\x82\x0c\xf7\x6c\xb0\xaa\xf6\xe8\xe4\xfe\x93\xad\x62\xd8\x03\x98\x31\x04", 32);
#endif
	VERIFY_BULLETPROOF(false);
}

static const char *const torsion_elements[] =
	{
		"c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa",
		"0000000000000000000000000000000000000000000000000000000000000000",
		"26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85",
		"ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f",
		"26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05",
		"0000000000000000000000000000000000000000000000000000000000000080",
		"c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a",
};

bool isInMainSubgroup(const rct::key& key)
{
	ge_p3 p3;
	if(ge_frombytes_vartime(&p3, key.bytes))
		return false;
	ge_p2 R;
	ge_scalarmult(&R, rct::curveOrder().bytes, &p3);
	rct::key tmp;
	ge_tobytes(tmp.bytes, &R);
	return tmp == rct::identity();
}

TEST(bulletproofs, invalid_torsion)
{
	rct::Bulletproof proof = bulletproof_PROVE(7329838943733, rct::skGen());
	ASSERT_TRUE(rct::bulletproof_VERIFY(proof));
	for(const auto &xs : torsion_elements)
	{
		rct::key x;
		ASSERT_TRUE(epee::string_tools::hex_to_pod(xs, x));
		ASSERT_FALSE(isInMainSubgroup(x));
		for(auto &k : proof.V)
		{
			const rct::key org_k = k;
			rct::addKeys(k, org_k, x);
			ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
			k = org_k;
		}
		for(auto &k : proof.L)
		{
			const rct::key org_k = k;
			rct::addKeys(k, org_k, x);
			ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
			k = org_k;
		}
		for(auto &k : proof.R)
		{
			const rct::key org_k = k;
			rct::addKeys(k, org_k, x);
			ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
			k = org_k;
		}
		const rct::key org_A = proof.A;
		rct::addKeys(proof.A, org_A, x);
		ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
		proof.A = org_A;
		const rct::key org_S = proof.S;
		rct::addKeys(proof.S, org_S, x);
		ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
		proof.S = org_S;
		const rct::key org_T1 = proof.T1;
		rct::addKeys(proof.T1, org_T1, x);
		ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
		proof.T1 = org_T1;
		const rct::key org_T2 = proof.T2;
		rct::addKeys(proof.T2, org_T2, x);
		ASSERT_FALSE(rct::bulletproof_VERIFY(proof));
		proof.T2 = org_T2;
	}
}

TEST(bulletproof, from_blob_1)
{
	static const char *tx_hex = "02000102000b849b08f2b70b9891019707a8081bc7040d9f0b55d3019669afc83528a6e18454cf13ca392a581098c067df30e66dee8aaddf14c61a8f020002775faa070d3b3ab1d9de66deb402f635aca2580191bce277c26fef7c00cb3f3500025c9c10a978bfe085d42a7b73980f53eab4cbfde73d8023e21978ec8a467375e22101a340cd8bc95636a0ba6ffe5ebfda5eb637d44ad73c32150a469008cb870d22aa03d0cca632f376c5417327569d497d42f09386c5dd4b5efecd9dd20719861ef5aed810e70d824e8e77189c35e6d79993eeeea77b219106df29dd9e77370e7f2fb5ead175064ba8a59397a3ce6804bde23b4d90039c5ad4d1282bc23f791221bc185d70b30d84dda556348a3b9af09513946a03c190b9c53fbeb970a286b1ff8d462630ef0a2737ff40f238461e8ed3eedb8f2a01492abcb96e116ae9d51c4b35e9ba2f3bbe78228618f17a5708c0e30a47b7ed15d4a20ded508f9daddd92e07c6e74167cdf0100000099c4e562de6abd309b4cc26ab41aac39eb0eb252468f79bc5369eae8ba7f94ef2d795fb6b61a0e69e6a95dd3e257615188e80bc1c90c5f571028bb9d2b99c13d41a1e1a770e592ae7a9cda9014f6d4f3233d30f062b774a7241b6e0bb0b83b4a3e36200234a288fcf65cf8a35dfd7710dc5ece5d7abb5ec58451f1cbd41513b1bb6190c609c25e2a2b94eadfe22e8a9eb28ea3d16fa49cb1eb4d7f5c3706b50e7ae60cedf6af2c3e8dc8f96113c029749ae2b266090cc2e6650cf0a869f6c20b0792987702834ff278516dccbd3cff94a6ff36361178a302b37a62c9134b50739228430306ff2bc6a6d282d4cfa9bf6b92486f0e0dd594f2334296e248514c28436b3e86f9d527a8b1ed9f6ed09fa48514364df41d50cb3d376b71b3585cad9de30c465302ae91818ce42eb77e26a31242b4f1255f455df49409197a6d0e468f2c2d781684bb697a785ac77d41950901e9b67a2a4d6a3ec05fffec9e3a0313c972120ac3f5e01f1bc595438d7e07ff6de4ede96915a8696bcbaf449fae978565eceaebe2c3bd2f8315c535ff25fa8924fc2d49e0cb7ecc1c3fd72ce821513fa113078fda233e1588022c6267ba2f78a8a4f9ac8c7ea2dc4dca464902f46fb92702db8d26afa628f2aa182c2b34768a2b0581e7196ce041e73924af51d713db75093bf292e4263be8fc08a0b2f531e1a10ce79b95ab1fab726478cea8e79e0313ffc895069938ecf7ed14a037577f4f461ae6cde9bae6ade8a1d9e46040321b250d7ff9f3612b278757717596040dc58e7f68687b72c1ba71f36daeeb7ebdcbfd77d3518dff7d0fee252887ee38db33dffd714924d5823c539288d581eba17053beb273a13ca6f43132da705308bdc53c80c45e347bffb5c1fae7907369598660ce2c70d34083fec197b914c3b77f50e57ec54d89d0031df92a1241d40f9ea3ed14008ecc339323118ad22adca5c56687f854bc5fd47a3223016eee46e7d94b31a101df22d87b1404bbceaaaab2a8bde72aa318d3364e8926119d792cad21e51faf0cbd5ea0bbe939c5bcfbaa489dfda38aa124f3fc007b9e58f55ad8acd25d17a40bd4c1c17e03610fecb789702b0b8a4aa3a79028a7292212c550dec72f2c356f02bc0f2a0513ae07892143b8aa5ab30e9f6d71eeb3df2ea64a839b5b857000db043bf506a26953a909116b10cdce03a27d549db2f51f9a341c721bb0e442b5d0034038fbb0cd2ef27fb48f5acbd6b4104af18a98a1692d10d59884fcd2eb4641000ac32df57b5dcf387c4c097e5e7e702b2f07cdb18a69d5c69a5f7e135a9f8e020670758a1e4d955878de2f93181adfddd8cff4d20365c4663e870ff09d6b15065bbd81555d6aeb92e07ebbeae426cd0ab982a03ffeec31627ae140cd1e78f60ab6a55811d9d4051d50050c9e920e0b11c526530e613e0d3f925271f90ef0990e3df2c46170153e553a0035c0e8e87d957f40f072fd6b1ff30ee7aca3af88c40f1c255b3546dba9d23f352c729a0466729918336560df233843734e7dad57960f8d5592a299f6b762efdbd37aa0ff5310c940d03622023146a042079c8097fe01606594ab3578d0c0a90f8088d5c93504896ed80e809d22bf9483bf62398feb06099904cc23480b27709845ef1e26059d4730aeb5c2bb34c2ff34bff3c1a1c10a5898584fac078225bd435541fd2f4244e14118c8a08af7a3027d41b7af62420d12ba05466f905fe49882db44994180a1a549acfec42549254feda65aa6ee0c0e35e5a7525ae373ea0053fd536d4b6605ee833a0fa85e863807c30f02b46fde0305864da7d10f60b44ec1c2944a45de27912a39cebdc0ae18034397e4f5cfaf0ebe9ea5b225e80075f1bf6ac2211b7512870cc556e685a2464bf91100b36e5d0ea64af85d92d2aa1c2625e5bcbe93352a92dec8d735e54a2e6dfba6a91cc7c40e5c883d932769ce2d57b21ba898a2437ae6a39cfda1f3adefab0241548ad88104cbf113df4d1a243a5ae639b75169ae60b2c0dd1091a994e2a4d6d3536e3f4405a723c50ba4e9f822a2de189fd8158b0aa94c4b6255e5d4b504f789e4036d4206e8afd25693198f7bb3b04c23a6dc83f09260ae7c83726d4d524e7f9f851c39f5";
	cryptonote::blobdata bd;
	ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(std::string(tx_hex), bd));
	cryptonote::transaction tx;
	crypto::hash tx_hash, tx_prefix_hash;
	ASSERT_TRUE(parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash));
	ASSERT_TRUE(tx.version == 2);
	ASSERT_TRUE(tx.rct_signatures.type == rct::RCTTypeBulletproof);
	ASSERT_TRUE(rct::verRctSemanticsSimple(tx.rct_signatures));
}

TEST(bulletproof, from_blob_2)
{
	static const char *tx_hex = "02000102000be98714944aeb01c006c80cbd0aaa04e5023e9003fa089669afc83528a6e18454cf13ca392a581098c067df30e66dee8aaddf14c61a8f040002377a0483ad63d58e7667a8325349c89e41e9ad5dce5aef30204fd4a6dc8eb7a100022a518afc3d690a992150646c559a24add698c98e87e732244cf2855cb7d0cff10002485a8de9d099c96fce8f26ad320cd627a6bb188f719c380517861c031aa65011000218ebf7b40a5ba25fb98ad7c6543239c2a3343b9e7cfd5140280e587c8f930f0d2101b330267408724dcf7fc2902e7d74a7962995e7905cc5d043fa1c8c6379e0eccd03e089bb498d0fffca61afd0d61f4875e8b32fa63f8729c654bba5f167199b7b518433680242640504c7568d273b2a74fe2d204dbb97eea4724ee5a2cf19eb3349ec3a2602a244de2a33c62bbfaa0b4cb85bf36863f765b237138929e43462e4bf19684d0924fd73c30bee474f0e927e8eb84dd6cc987acaf41e19e2f1e07381bd95e0f504964c8d10793972e88d64683a4a3960a9645735a76cc62d99e7a87b62c3cb590ca9dc27f91e9103c1b55ece5d5a932a04c99bf019463455b5d78397bd2295be075af5ae9bf0e43e724e11f83f336ca3c1bd9601c7fd6642795e8618b5c5b9d0045a6766daf2118f994b418504c6939b94c72e875423989ea7069d73e8d02f7b0bd9c1c7eb2289eaeda5fabd8142ee0ebafcbe101c58e99034d0c9ca34a703180e1dd7000e9f11cb4bcbe11f0c0041a0cc30f5b8b3bd7f2ace0266dd0282aea17f088c88e98a22f764e32507d1c900ef50b1157b49dbfda2fd9a2ea3be5182fb10fa590b464907049d88ff9c33fbe6d8b05898abf196dd097e1009d9bd1a1997830100000006ccdcc8aa53e183578656540fa393e6d9f96c07497b9dd009b48e8f990a75be18f5f37a47ff07f3c4d6427626afc5d24897ad31a98d01cc44476fba41f6bc3dc3de91d2655130090393e3ebcb7f436470edefff5aa11fa1016fecccf1824a5cdd66ea51dcd8f0193a6e507309d6a13680605febb971c3df4cceac5078be996c0d686c72627696e6961447145143e23cae2c97686524c0587b6cce7b05851b087d658a795bd22de18c0f68e824e1673c47f4b7f4ba7d4bedd95f46ffc22d9409087a58a80088679d3775e46f75dc6dc48f485a2c35a8dd60e7dabe29f656cbaad8d25a5e01d165fa9df29acd6e2471c4880d3129fd110066788de9c979f03d9c2e46ab80bf0dbd24ac6aaba2db0d723e1ba3a002efd2aebf245a2fd53767fa490244396284826b64a4a3f069f21047486a27c5ebdf802c23d8d276b9c83fa2e329aefb953ad34f382975204706c14249a496791cd3d20f4bd98d8f6325f5e7d7aa2d1f8b23361434f74584136cf1ead94365a6ce134159141fa4c68660a99ad90caa8c711abe5411dffa7132f8ce71dba63619e1410380c56766c79ff8e433eb806f49bca6f1bcb0c66dfd61c3133e7c095a11abd068b6a5774a02a5825cbd82a408d5580ee4dbe9a4ba07282f5a764279ad27f7ac27e7cccb3c76b5dd64be7bbdf3ecc4abcc29bc561e81cbc502ed3a4ce277b567a6eb09dfd8454c4d4e8c038b9dd6042a0515b0d1dfbb45585e79ca5705a22fcb3c67bc0261cc0cec6998354448e83fa7ff8706178e14a482e73719df33c9d753757131f3560391be2dd6c40391e3e7882ea07bb23c4d2d157349965082e1447e94849fde224452f6c98efa44f6438b731859fac8f49761e4447e8d34275e7dd9ae01d8550dcc75284715e026d25e9c444265fb4fee3f783ff2a2a5c414714a57525738884bbdbd8d997dbbcafcafd8e283b524bef0ded141160f47ce352b2104257b312d12594de45a0241d9753ec19e2f8603b5fc8682d72bf1de51d7f4caa7026989a7e46b9dd41075ad480df9de6a952e8562e548e3576e9c9230cb2cd0ee7f955e2d29240d7fa55b8e0b0b6c92823d9636592c460af670bb0b8714ca626497c68403793fe8495a7542c60587d117e3adb3644e62053817fa600910e2dfad97b2a7492ac6fa13c0a9a03e0ce12c3d12a09a4e22b9d0d74b9d431d53252fcbd06cb119d128646042eef81002fc6e9ff5006e06247f40f0391ad095c0d50a78863c975edbf0498e58ba7e6e0505d5eaa4d8ba2aa3f40e728f1a6aef6b2f9f5705cc3d2591bb5b878c258b4107857dc6ee75d591ea7af7b16196cd6c979a0bf819db39658491889c83a41c2c0035116fcac23ac45144731592ea0e3a11c335a278b2a6798d46828c8590b92701468e9d9c3560ed58c3ab861995aca439ee49ecf4a5b0ad160a12bd23a437f90c383095e85a95bb441bcb8dc1752e805e85d1ee0f6967ce95dcd888efa7ac440813c78b11d56d2a1b870c59d36430b10cd28bc693c4f64e769acfbcff27d8e904e0ca7e73aab5439de571b66f91fe9c05264e070aa223b68e5de763d838985e0ec0e8ac0dd0b1f6eb1e145c4473f9edda5732b1f9d3627423b5c60e055377bd044ff30017d25b3d26b5590b53d8aeaf10ce73d86fe4c40fa14e6f710f72c7da0600e7fc495a75a875d1aeee246b70cf24a8a85fba2d31f96faa42ece112b9030987ce0e735ab51eb4222a48bc51ab69d644bda77fb2aa0cd3a0477a2a2d92510103dbd58ee1c28eb20cfb31f5268f4a70a431ff4aedbfdfc59ea6709283a51902202effba960da6170b1a25c26a52890da54757c93156d250540590266eed8c00647270cb302cff7cbe4a8ed27da21dbaa303d1aea0eb152e1f6fd24dbdfaea0c5b0f5d6acb6724cf711ec3194a94f52f8cce13e1e3d1d7758d3d7e3cd37fd1011265199eb4126975687ce958dd1a75b6a71cf397fb618003e85af842dc3ff50a134411bbe18a1dea4beeb1e8d1ca5ac67f7f6ce2bbdeb2efcf6dcfdef64b360d4fb1849947800a3595e0a8029b631a06508b5d9f4f6e6a1be110524e5584f209b9db1651ddc8571102a58e7823dcf026f89d59ca213b6c6e32088d6c4967b20b28ffe86aed6c11d6aa0072691ff133d7bbc6d013629faebadc087c0f4f84d106677013893be4ca55018fbafcc2cee8be4ad0bcf1ad8762ec0c285e8c414bb204";
	cryptonote::blobdata bd;
	ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(std::string(tx_hex), bd));
	cryptonote::transaction tx;
	crypto::hash tx_hash, tx_prefix_hash;
	ASSERT_TRUE(parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash));
	ASSERT_TRUE(tx.version == 2);
	ASSERT_TRUE(tx.rct_signatures.type == rct::RCTTypeBulletproof);
	ASSERT_TRUE(rct::verRctSemanticsSimple(tx.rct_signatures));
}
