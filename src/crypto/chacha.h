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

#include <stddef.h>
#include <stdint.h>

#define CHACHA_KEY_SIZE 32
#define CHACHA_IV_SIZE 8

#if defined(__cplusplus)
#include <memory.h>

#include "pow_hash/cn_slow_hash.hpp"
#include "hash.h"
#include "memwipe.h"

namespace crypto
{
extern "C" {
#endif
void chacha8(const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);
void chacha20(const void *data, size_t length, const uint8_t *key, const uint8_t *iv, char *cipher);
#if defined(__cplusplus)
}

using chacha_key = tools::scrubbed_arr<uint8_t, CHACHA_KEY_SIZE>;

#pragma pack(push, 1)
// MS VC 2012 doesn't interpret `class chacha_iv` as POD in spite of [9.0.10], so it is a struct
struct chacha_iv
{
	uint8_t data[CHACHA_IV_SIZE];
};
#pragma pack(pop)

static_assert(sizeof(chacha_key) == CHACHA_KEY_SIZE && sizeof(chacha_iv) == CHACHA_IV_SIZE, "Invalid structure size");

inline void chacha8(const void *data, std::size_t length, const chacha_key &key, const chacha_iv &iv, char *cipher)
{
	chacha8(data, length, key.data(), reinterpret_cast<const uint8_t *>(&iv), cipher);
}

inline void chacha20(const void *data, std::size_t length, const chacha_key &key, const chacha_iv &iv, char *cipher)
{
	chacha20(data, length, key.data(), reinterpret_cast<const uint8_t *>(&iv), cipher);
}

inline void generate_chacha_key(const void *data, size_t size, chacha_key &key)
{
	static_assert(sizeof(chacha_key) <= sizeof(hash), "Size of hash must be at least that of chacha_key");
	tools::scrubbed_arr<char, HASH_SIZE> pwd_hash;
	cn_pow_hash_v1 kdf_hash;
	kdf_hash.hash(data, size, pwd_hash.data());
	memcpy(&unwrap(key), pwd_hash.data(), sizeof(key));
}

inline void generate_chacha_key_prehashed(const void *data, size_t size, chacha_key &key)
{
	static_assert(sizeof(chacha_key) <= sizeof(hash), "Size of hash must be at least that of chacha_key");
	tools::scrubbed_arr<char, HASH_SIZE> pwd_hash;
	cn_pow_hash_v1 kdf_hash;
	kdf_hash.hash(data, size, pwd_hash.data());
	memcpy(&unwrap(key), pwd_hash.data(), sizeof(key));
}

inline void generate_chacha_key(std::string password, chacha_key &key)
{
	return generate_chacha_key(password.data(), password.size(), key);
}
}

#endif
