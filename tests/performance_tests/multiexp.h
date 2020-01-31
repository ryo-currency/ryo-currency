// Copyright (c) 2018, The Monero Project
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

#include "ringct/multiexp.h"
#include "ringct/rctOps.h"
#include <vector>

template <test_multiexp_algorithm algorithm, size_t npoints, size_t c = 0>
class test_multiexp
{
  public:
	static const size_t loop_count = npoints >= 1024 ? 10 : npoints < 256 ? 1000 : 100;

	bool init()
	{
		data.resize(npoints);
		res = rct::identity();
		for(size_t n = 0; n < npoints; ++n)
		{
			data[n].scalar = rct::skGen();
			rct::key point = rct::scalarmultBase(rct::skGen());
			if(ge_frombytes_vartime(&data[n].point, point.bytes))
				return false;
			rct::key kn = rct::scalarmultKey(point, data[n].scalar);
			res = rct::addKeys(res, kn);
		}
		cache_a.s_cache.init_cache(data);
		cache_a.p_cache.init_cache(data);
		return true;
	}

	bool test()
	{
		switch(algorithm)
		{
		case multiexp_bos_coster:
			return res == bos_coster_heap_conv_robust(data);
		case multiexp_straus:
			cache_b.s_cache.init_cache(data);
			return res == cache_b.s_cache.straus(data, 0);
		case multiexp_straus_cached:
			return res == cache_a.s_cache.straus(data, 0);
		case multiexp_pippenger:
			cache_b.p_cache.init_cache(data);
			return res == cache_b.p_cache.pippenger(data, cache_b.buckets, c);
		case multiexp_pippenger_cached:
			return res == cache_a.p_cache.pippenger(data, cache_a.buckets, c);
		default:
			return false;
		}
	}

  private:
	std::vector<rct::MultiexpData> data;
	rct::bp_cache cache_a;
	rct::bp_cache cache_b;
	rct::key res;
};
