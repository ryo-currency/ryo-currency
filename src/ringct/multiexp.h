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
// Adapted from Python code by Sarang Noether

#pragma once

#include "cryptonote_config.h"
#include "crypto/crypto.h"
#include "misc_log_ex.h"
#include "rctTypes.h"
#include <vector>
#include <boost/align/aligned_delete.hpp>

//Unfortunately this is a C++17 feature... but we can roll our own
template<typename T>
using aligned_ptr = std::unique_ptr<T, boost::alignment::aligned_delete>;

template<typename T>
inline aligned_ptr<T[]> make_aligned_array(size_t align, size_t n)
{
	return aligned_ptr<T[]>(::new(boost::alignment::aligned_alloc(align, sizeof(T) * n)) T[n]);
}

// This is needed here for a friendship declaration - otherwise it belongs in tests
enum test_multiexp_algorithm
{
	multiexp_bos_coster,
	multiexp_straus,
	multiexp_straus_cached,
	multiexp_pippenger,
	multiexp_pippenger_cached,
};
template <test_multiexp_algorithm algorithm, size_t npoints, size_t c>
class test_multiexp;

namespace rct
{
static constexpr size_t maxN = 64;
static constexpr size_t maxM = cryptonote::common_config::BULLETPROOF_MAX_OUTPUTS;

struct MultiexpData
{
	rct::key scalar;
	ge_p3 point;

	MultiexpData() {}
	MultiexpData(const rct::key &s, const ge_p3 &p) : scalar(s), point(p) {}
	MultiexpData(const rct::key &s, const rct::key &p) : scalar(s)
	{
		CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&point, p.bytes) == 0, "ge_frombytes_vartime failed");
	}
};

rct::key bos_coster_heap_conv(std::vector<MultiexpData> data);
rct::key bos_coster_heap_conv_robust(std::vector<MultiexpData> data);

struct alignas(8) ge_cached_pad
{
	ge_cached gec;
};

// Algorithm specific cache
class pippenger_cache
{
public:
	pippenger_cache() : size(0), alloc_size(0) {}	

	inline ge_cached* pp_offset(size_t n) { return &cache[n].gec; };
	inline const ge_cached& pp_offset(size_t n) const { return cache[n].gec; };

	inline size_t get_size() { return size * sizeof(ge_cached_pad); };
	void init_cache(const std::vector<MultiexpData> &data, size_t N = 0, bool over_alloc = true);

	rct::key pippenger(const std::vector<MultiexpData> &data, aligned_ptr<ge_p3[]>& buckets, size_t c = 0) const;

	static inline size_t get_pippenger_c(size_t N)
	{
		// uncached: 2:1, 4:2, 8:2, 16:3, 32:4, 64:4, 128:5, 256:6, 512:7, 1024:7, 2048:8, 4096:9
		//   cached: 2:1, 4:2, 8:2, 16:3, 32:4, 64:4, 128:5, 256:6, 512:7, 1024:7, 2048:8, 4096:9
		if(N <= 2)
			return 1;
		if(N <= 8)
			return 2;
		if(N <= 16)
			return 3;
		if(N <= 64)
			return 4;
		if(N <= 128)
			return 5;
		if(N <= 256)
			return 6;
		if(N <= 1024)
			return 7;
		if(N <= 2048)
			return 8;
		return 9;
	}

private:
	size_t size;
	size_t alloc_size;
	aligned_ptr<ge_cached_pad[]> cache;
};

// Algorithm specific cache
class straus_cache
{
public:
	straus_cache() : size(0) {}	

	static constexpr size_t STRAUS_C = 4;

	inline ge_cached* st_offset(size_t point, size_t digit) { return &cache[point + size * (digit-1)].gec; }
	inline const ge_cached& st_offset(size_t point, size_t digit) const { return cache[point + size * (digit-1)].gec; };

	inline size_t get_size() { return size * sizeof(ge_cached_pad); };
	void init_cache(const std::vector<MultiexpData> &data, size_t N = 0);

	rct::key straus(const std::vector<MultiexpData> &data, size_t STEP) const;

private:
	size_t size;
	size_t alloc_size;
	aligned_ptr<ge_cached_pad[]> cache;
};

// This cache is global and read-only
class multiexp_cache
{
private:
	static constexpr size_t STRAUS_SIZE_LIMIT = 128;
	static constexpr size_t PIPPENGER_SIZE_LIMIT = maxN * maxM;

	straus_cache s_cache;
	pippenger_cache p_cache;
	alignas(16) rct::key Hi_cache[maxN * maxM], Gi_cache[maxN * maxM];
	alignas(16) ge_p3 Hi_p3_cache[maxN * maxM], Gi_p3_cache[maxN * maxM];

	multiexp_cache();

public:
	inline static const multiexp_cache& inst()
	{
		static_assert(128 <= STRAUS_SIZE_LIMIT, "Straus in precalc mode can only be calculated till STRAUS_SIZE_LIMIT");
		static multiexp_cache th;
		return th;
	}

	inline static const rct::key& Hi(size_t n)
	{
		return inst().Hi_cache[n];
	}

	inline static const rct::key& Gi(size_t n)
	{
		return inst().Gi_cache[n];
	}

	inline static const ge_p3& Hi_p3(size_t n)
	{
		return inst().Hi_p3_cache[n];
	}

	inline static const ge_p3& Gi_p3(size_t n)
	{
		return inst().Gi_p3_cache[n];
	}

	inline static const pippenger_cache& get_pc()
	{
		return inst().p_cache;
	}

	inline static const straus_cache& get_sc()
	{
		return inst().s_cache;
	}
};

// This cache is unique to each proof or verify
class bp_cache
{
template <test_multiexp_algorithm algorithm, size_t npoints, size_t c>
friend class ::test_multiexp;

public:
	inline rct::key multiexp_higi()
	{
		return me_pad.size() <= 128 ? multiexp_cache::get_sc().straus(me_pad, 0) : multiexp_cache::get_pc().pippenger(me_pad, buckets, pippenger_cache::get_pippenger_c(me_pad.size()));
	}

	inline rct::key multiexp_s()
	{
		s_cache.init_cache(me_pad);
		return s_cache.straus(me_pad, 0);
	}

	inline rct::key multiexp_p()
	{
		p_cache.init_cache(me_pad);
		return p_cache.pippenger(me_pad, buckets, pippenger_cache::get_pippenger_c(me_pad.size()));
	}

	inline rct::key multiexp()
	{
		if(me_pad.size() <= 64)
			return multiexp_s();
		else
			return multiexp_p();
	}

	rct::key vector_exponent(const rct::keyV &a, const rct::keyV &b);
	rct::key vector_exponent_custom(const rct::keyV &A, const rct::keyV &B, const rct::keyV &a, const rct::keyV &b);

	//This allows the scratchpad to be filled by the user
	std::vector<MultiexpData> me_pad;
	inline void clear_pad(size_t res_size) { me_pad.clear(); if(me_pad.capacity() < res_size) me_pad.reserve(res_size); }

private:
	 straus_cache s_cache;
	 pippenger_cache p_cache;
	 aligned_ptr<ge_p3[]> buckets;
};
}

