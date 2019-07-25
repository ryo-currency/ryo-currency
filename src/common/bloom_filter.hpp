// Copyright (c) 2019, Ryo Currency Project
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

#include <vector>
#include <random>

#include "crypto/hash.h"

class bloom_filter
{
public:
	bloom_filter() {}
	
	inline void init(size_t num_of_elements)
	{
		bits_number = num_of_elements * 16;
		hash_number = 11;
		bytes.resize(bits_number/8, 0);
	}

	inline void add_element(const void* data, size_t data_len)
	{
		crypto::hash h = crypto::cn_fast_hash(data, data_len);

		uint64_t seed;
		memcpy(&seed, h.data, sizeof(seed));

		std::mt19937_64 mtgen(seed);
		std::uniform_int_distribution<uint16_t> dist(0, bits_number-1);

		for(size_t i=0; i < hash_number; i++)
			set_bit(dist(mtgen));
	}

	inline bool not_present(const void* data, size_t data_len)
	{
		crypto::hash h = crypto::cn_fast_hash(data, data_len);

		uint64_t seed;
		memcpy(&seed, h.data, sizeof(seed));

		std::mt19937_64 mtgen(seed);
		std::uniform_int_distribution<uint16_t> dist(0, bits_number-1);

		for(size_t i=0; i < hash_number; i++)
		{
			if(!read_bit(dist(mtgen)))
				return true;
		}
		return false;
	}

private:
	size_t hash_number;
	size_t bits_number;
	std::vector<uint8_t> bytes;

	inline bool read_bit(size_t pos)
	{
		size_t byte = pos / 8;
		size_t bit = pos % 8;

		if(byte >= bytes.size())
		    return false;

		return bytes[byte] & (1<<bit);
	}

	inline void set_bit(size_t pos)
	{
		size_t byte = pos / 8;
		size_t bit = pos % 8;

		if(byte < bytes.size())
			bytes[byte] |= (1<<bit);
	}
};
