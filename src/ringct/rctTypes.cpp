// Copyright (c) 2019, Ryo Currency Project
// Portions copyright (c) 2016, Monero Research Labs
//
// Author: Shen Noether <shen.noether@gmx.com>
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

#include "rctTypes.h"
#include "common/gulps.hpp"
#include "cryptonote_config.h"
using namespace crypto;
using namespace std;

namespace rct
{

GULPS_CAT_MAJOR("rctTypes");

//dp
//Debug printing for the above types
//Actually use DP(value) and #define DBG

void dp(key a)
{
	int j = 0;
	GULPS_PRINT("\"");
	for(j = 0; j < 32; j++)
	{
		GULPSF_PRINT("{0:x}", (unsigned char)a.bytes[j]);
	}
	GULPS_PRINT("\"");
	GULPS_PRINT("\n");
}

void dp(bool a)
{
	GULPSF_PRINT(" ... {} ... ", a ? "true" : "false");
	GULPS_PRINT("\n");
}

void dp(const char *a, int l)
{
	int j = 0;
	GULPS_PRINT("\"");
	for(j = 0; j < l; j++)
	{
		GULPSF_PRINT("{0:x}", (unsigned char)a[j]);
	}
	GULPS_PRINT("\"");
	GULPS_PRINT("\n");
}
void dp(keyV a)
{
	size_t j = 0;
	GULPS_PRINT("[");
	for(j = 0; j < a.size(); j++)
	{
		dp(a[j]);
		if(j < a.size() - 1)
		{
			GULPS_PRINT(",");
		}
	}
	GULPS_PRINT("]");
	GULPS_PRINT("\n");
}
void dp(keyM a)
{
	size_t j = 0;
	GULPS_PRINT("[");
	for(j = 0; j < a.size(); j++)
	{
		dp(a[j]);
		if(j < a.size() - 1)
		{
			GULPS_PRINT(",");
		}
	}
	GULPS_PRINT("]");
	GULPS_PRINT("\n");
}
void dp(ryo_amount vali)
{
	GULPS_PRINT("x: ", vali, "\n\n");
}

void dp(int vali)
{
	GULPSF_PRINT("x: {}\n", vali);
	GULPS_PRINT("\n");
}
void dp(bits amountb)
{
	for(int i = 0; i < 64; i++)
	{
		GULPS_PRINT(amountb[i]);
	}
	GULPS_PRINT("\n");
}

void dp(const char *st)
{
	GULPSF_PRINT("{}\n", st);
}

//Various Conversions

//uint long long to 32 byte key
void d2h(key &amounth, const ryo_amount in)
{
	sc_0(amounth.bytes);
	ryo_amount val = in;
	int i = 0;
	while(val != 0)
	{
		amounth[i] = (unsigned char)(val & 0xFF);
		i++;
		val /= (ryo_amount)256;
	}
}

//uint long long to 32 byte key
key d2h(const ryo_amount in)
{
	key amounth;
	sc_0(amounth.bytes);
	ryo_amount val = in;
	int i = 0;
	while(val != 0)
	{
		amounth[i] = (unsigned char)(val & 0xFF);
		i++;
		val /= (ryo_amount)256;
	}
	return amounth;
}

//uint long long to int[64]
void d2b(bits amountb, ryo_amount val)
{
	int i = 0;
	while(val != 0)
	{
		amountb[i] = val & 1;
		i++;
		val >>= 1;
	}
	while(i < 64)
	{
		amountb[i] = 0;
		i++;
	}
}

//32 byte key to uint long long
// if the key holds a value > 2^64
// then the value in the first 8 bytes is returned
ryo_amount h2d(const key &test)
{
	ryo_amount vali = 0;
	int j = 0;
	for(j = 7; j >= 0; j--)
	{
		vali = (ryo_amount)(vali * 256 + (unsigned char)test.bytes[j]);
	}
	return vali;
}

//32 byte key to int[64]
void h2b(bits amountb2, const key &test)
{
	int val = 0, i = 0, j = 0;
	for(j = 0; j < 8; j++)
	{
		val = (unsigned char)test.bytes[j];
		i = 8 * j;
		while(val != 0)
		{
			amountb2[i] = val & 1;
			i++;
			val >>= 1;
		}
		while(i < 8 * (j + 1))
		{
			amountb2[i] = 0;
			i++;
		}
	}
}

//int[64] to 32 byte key
void b2h(key &amountdh, const bits amountb2)
{
	int byte, i, j;
	for(j = 0; j < 8; j++)
	{
		byte = 0;
		i = 8 * j;
		for(i = 7; i > -1; i--)
		{
			byte = byte * 2 + amountb2[8 * j + i];
		}
		amountdh[j] = (unsigned char)byte;
	}
	for(j = 8; j < 32; j++)
	{
		amountdh[j] = (unsigned char)(0x00);
	}
}

//int[64] to uint long long
ryo_amount b2d(bits amountb)
{
	ryo_amount vali = 0;
	int j = 0;
	for(j = 63; j >= 0; j--)
	{
		vali = (ryo_amount)(vali * 2 + amountb[j]);
	}
	return vali;
}

size_t n_bulletproof_amounts(const Bulletproof &proof)
{
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() >= 6, 0, "Invalid bulletproof L size");
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() == proof.R.size(), 0, "Mismatched bulletproof L/R size");
	static const size_t extra_bits = 4;
	static_assert((1 << extra_bits) == cryptonote::common_config::BULLETPROOF_MAX_OUTPUTS, "log2(BULLETPROOF_MAX_OUTPUTS) is out of date");
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() <= 6 + extra_bits, 0, "Invalid bulletproof L size");
	GULPS_CHECK_AND_ASSERT_MES(proof.V.size() <= (1u << (proof.L.size() - 6)), 0, "Invalid bulletproof V/L");
	GULPS_CHECK_AND_ASSERT_MES(proof.V.size() * 2 > (1u << (proof.L.size() - 6)), 0, "Invalid bulletproof V/L");
	GULPS_CHECK_AND_ASSERT_MES(proof.V.size() > 0, 0, "Empty bulletproof");
	return proof.V.size();
}

size_t n_bulletproof_amounts(const std::vector<Bulletproof> &proofs)
{
	size_t n = 0;
	for(const Bulletproof &proof : proofs)
	{
		size_t n2 = n_bulletproof_amounts(proof);
		GULPS_CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
		if(n2 == 0)
			return 0;
		n += n2;
	}
	return n;
}

size_t n_bulletproof_max_amounts(const Bulletproof &proof)
{
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() >= 6, 0, "Invalid bulletproof L size");
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() == proof.R.size(), 0, "Mismatched bulletproof L/R size");
	static const size_t extra_bits = 4;
	static_assert((1 << extra_bits) == cryptonote::common_config::BULLETPROOF_MAX_OUTPUTS, "log2(BULLETPROOF_MAX_OUTPUTS) is out of date");
	GULPS_CHECK_AND_ASSERT_MES(proof.L.size() <= 6 + extra_bits, 0, "Invalid bulletproof L size");
	return 1 << (proof.L.size() - 6);
}

size_t n_bulletproof_max_amounts(const std::vector<Bulletproof> &proofs)
{
	size_t n = 0;
	for(const Bulletproof &proof : proofs)
	{
		size_t n2 = n_bulletproof_max_amounts(proof);
		GULPS_CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
		if(n2 == 0)
			return 0;
		n += n2;
	}
	return n;
}

} // namespace rct
