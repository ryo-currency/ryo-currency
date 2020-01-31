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

#pragma once

#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_basic/difficulty.h"

namespace cryptonote
{
namespace bootstrap
{

struct file_info
{
	uint8_t major_version;
	uint8_t minor_version;
	uint32_t header_size;

	BEGIN_SERIALIZE_OBJECT()
	FIELD(major_version);
	FIELD(minor_version);
	VARINT_FIELD(header_size);
	END_SERIALIZE()
};

struct blocks_info
{
	// block heights of file's first and last blocks, zero-based indexes
	uint64_t block_first;
	uint64_t block_last;

	// file position, for directly reading last block
	uint64_t block_last_pos;

	BEGIN_SERIALIZE_OBJECT()
	VARINT_FIELD(block_first);
	VARINT_FIELD(block_last);
	VARINT_FIELD(block_last_pos);
	END_SERIALIZE()
};

struct block_package
{
	cryptonote::block block;
	std::vector<transaction> txs;
	size_t block_size;
	difficulty_type cumulative_difficulty;
	uint64_t coins_generated;

	BEGIN_SERIALIZE()
	FIELD(block)
	FIELD(txs)
	VARINT_FIELD(block_size)
	VARINT_FIELD(cumulative_difficulty)
	VARINT_FIELD(coins_generated)
	END_SERIALIZE()
};
} // namespace bootstrap
} // namespace cryptonote
